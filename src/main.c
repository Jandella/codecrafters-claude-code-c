#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "read_tool.h"

struct response_buf {
    char *data;
    size_t size;
};

static size_t curl_write_response(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    struct response_buf *buf = (struct response_buf *)userp;
    char *tmp = realloc(buf->data, buf->size + total + 1);
    if (!tmp) return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->size, contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

int main(int argc, char *argv[]) {
    const char *prompt = NULL;
    if (getopt(argc, argv, "p:") == 'p') prompt = optarg;
    if (!prompt) {
        fprintf(stderr, "error: -p flag is required\n");
        return 1;
    }

    const char *api_key = getenv("OPENROUTER_API_KEY");
    const char *base_url = getenv("OPENROUTER_BASE_URL");
    const char *local_model = getenv("LOCAL_MODEL");
    if (!base_url || !*base_url) base_url = "https://openrouter.ai/api/v1";
    if(!local_model || !*local_model) local_model = "anthropic/claude-haiku-4.5";
    if (!api_key || !*api_key) {
        fprintf(stderr, "OPENROUTER_API_KEY is not set\n");
        return 1;
    }
    

    cJSON *req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "model", local_model);
    cJSON *messages = cJSON_AddArrayToObject(req, "messages");
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "role", "user");
    cJSON_AddStringToObject(msg, "content", prompt);
    cJSON_AddItemToArray(messages, msg);
    cJSON *tools = cJSON_AddArrayToObject(req, "tools");
    add_read_tool(tools);

    char *body = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);

    char url[512];
    snprintf(url, sizeof(url), "%s/chat/completions", base_url);

    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    struct response_buf resp = {NULL, 0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(body);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
        free(resp.data);
        return 1;
    }

    cJSON *json = cJSON_Parse(resp.data);
    free(resp.data);
    if (!json) {
        fprintf(stderr, "Failed to parse response JSON\n");
        return 1;
    }

    cJSON *choices = cJSON_GetObjectItem(json, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        fprintf(stderr, "no choices in response\n");
        cJSON_Delete(json);
        return 1;
    }

    cJSON *first = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItem(first, "message");
    cJSON *content = cJSON_GetObjectItem(message, "content");

    char *response_data = cJSON_Print(json);
    printf("%s\n", response_data);
    free(response_data);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    fprintf(stderr, "Logs from your program will appear here!\n");
    

    cJSON *tool_calls = cJSON_GetObjectItem(message, "tool_calls");
    if (!cJSON_IsArray(tool_calls) || cJSON_GetArraySize(tool_calls) == 0) {
        // no tool calls -> print the message content
        printf("%s", cJSON_GetStringValue(content));
    }
    else {
        //handles the tool call
        fprintf(stderr, "Handling tool call\n");
        cJSON *first_tool_call = cJSON_GetArrayItem(tool_calls, 0);
        cJSON *function_object = cJSON_GetObjectItem(first_tool_call, "function");
        cJSON *name = cJSON_GetObjectItem(function_object, "name");
        char *name_read = cJSON_GetStringValue(name);
        fprintf(stderr, "%s\n", name_read);
        if(strcmp("Read", name_read) == 0) {
            fprintf(stderr, "inside if read command here\n");
            cJSON *arguments_dictionary = cJSON_GetObjectItem(function_object, "arguments");
            char *raw_arguments = cJSON_GetStringValue(arguments_dictionary);
            fprintf(stderr, "raw args: %s\n", raw_arguments);
            cJSON *parsed_arguments = cJSON_Parse(raw_arguments);
            if (!parsed_arguments) {
                fprintf(stderr, "Failed to parse function arguments\n");
                return 1;
            }
            cJSON *file_path_object = cJSON_GetObjectItem(parsed_arguments, "file_path");
            char *file_path = cJSON_GetStringValue(file_path_object);
            
            fprintf(stderr, "Read tool to read file at path %s\n", file_path);
            FILE *fptr;
            fptr = fopen(file_path, "r");
            if(fptr == NULL){
                fclose(fptr);
                fprintf(stderr, "Failed to open file %s\n", file_path);
                return 1;
            }
            char *fcontent = NULL;
            long file_size = get_file_size(fptr);
            if(file_size == -1){
                fclose(fptr);
                fprintf(stderr, "Failed to get the file size\n");
                return 1;
            }
            fcontent = malloc(sizeof(char) * file_size + 1);
            fread(fcontent, 1, file_size, fptr);
            fclose(fptr);
            fcontent[file_size] = '\0';
            printf("%s", fcontent);
            cJSON_Delete(parsed_arguments);
            free(fcontent);
        }
        

    }
    

    cJSON_Delete(json);
    return 0;
}
