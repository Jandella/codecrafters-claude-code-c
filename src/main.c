#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "agent.h"
#include "read_tool.h"

typedef struct api_call_params
{
    char *api_key;
    char *base_url;
    char *local_model;
} api_call_params;

struct response_buf
{
    char *data;
    size_t size;
};

static api_call_params apiCfg = {NULL, NULL, NULL};

static size_t curl_write_response(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total = size * nmemb;
    struct response_buf *buf = (struct response_buf *)userp;
    char *tmp = realloc(buf->data, buf->size + total + 1);
    if (!tmp)
        return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->size, contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static CURLcode api_call(api_call_params *apiCfg, cAgent *agent, CURL* curl, struct response_buf *resp);

static int loop(cAgent *agent, api_call_params *apiCfg, struct response_buf *final_response);

int main(int argc, char *argv[])
{
    char *prompt = NULL;
    if (getopt(argc, argv, "p:") == 'p')
        prompt = optarg;
    if (!prompt)
    {
        fprintf(stderr, "error: -p flag is required\n");
        return 1;
    }

    apiCfg.api_key = getenv("OPENROUTER_API_KEY");
    apiCfg.base_url = getenv("OPENROUTER_BASE_URL");
    apiCfg.local_model = getenv("LOCAL_MODEL");
    if (!apiCfg.base_url || !*apiCfg.base_url)
        apiCfg.base_url = "https://openrouter.ai/api/v1";
    if (!apiCfg.local_model || !*apiCfg.local_model)
        apiCfg.local_model = "anthropic/claude-haiku-4.5";
    if (!apiCfg.api_key || !*apiCfg.api_key)
    {
        fprintf(stderr, "OPENROUTER_API_KEY is not set\n");
        return 1;
    }

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    fprintf(stderr, "Logs from your program will appear here!\n");

    cAgent *agent = cAgent_createAgent();
    cAgent_addPrompt(agent, prompt);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    struct response_buf final_response = {NULL, 0};
    int final_result = loop(agent, &apiCfg, &final_response);

    curl_global_cleanup();
    cAgent_destroyAgent(agent);

    if(final_response.size == 0) {
        fprintf(stderr, "Final response empty!\n");
        return 1;
    }
    printf("%s", final_response.data);
    free(final_response.data);
    
    return final_result;
}

static CURLcode api_call(api_call_params *cfg, cAgent *agent, CURL *curl, struct response_buf *resp)
{
    cJSON *req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "model", cfg->local_model);
    cJSON *messages = cJSON_AddArrayToObject(req, "messages");
    messages_list *ma = agent->messages;
    for (size_t i = 0; i < ma->count; i++)
    {
        message_prompt item = ma->first[i];
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", item.role);
        if (item.tool_id)
        {
            cJSON_AddStringToObject(msg, "tool_call_id", item.tool_id);
        }
        cJSON_AddStringToObject(msg, "content", item.content);
        cJSON_AddItemToArray(messages, msg);
    }

    cJSON *tools = cJSON_AddArrayToObject(req, "tools");
    add_read_tool(tools);

    char *body = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);
    fprintf(stderr, "Request body:\n%s\n", body);

    char url[512];
    snprintf(url, sizeof(url), "%s/chat/completions", cfg->base_url);

    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", cfg->api_key);

    if (!curl)
    {
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
    }
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(body);
    return res;
}

static int loop(cAgent *agent, api_call_params *apiCfg, struct response_buf *final_response)
{
    cAgentTool *readTool = cAgentTool_createReadTool();
    cAgentToolResult toolResult = {NULL, NULL};
    int done = 0;
    int agent_tries = 0;
    CURL *curl = NULL;
    while (!done)
    {
        struct response_buf resp = {NULL, 0};
        CURLcode res = api_call(apiCfg, agent, curl, &resp);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
            return 1;
        }

        cJSON *json = cJSON_Parse(resp.data);
        free(resp.data);
        resp.size = 0;
        if (!json)
        {
            fprintf(stderr, "Failed to parse response JSON\n");
            return 1;
        }
        char *response_data = cJSON_Print(json);
        fprintf(stderr, "response data:\n%s\n", response_data);
        free(response_data);

        cJSON *choices = cJSON_GetObjectItem(json, "choices");
        if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0)
        {
            fprintf(stderr, "no choices in response\n");
            cJSON_Delete(json);
            return 1;
        }

        cJSON *first = cJSON_GetArrayItem(choices, 0);
        cJSON *message = cJSON_GetObjectItem(first, "message");
        cJSON *content = cJSON_GetObjectItem(message, "content");

        cJSON *tool_calls = cJSON_GetObjectItem(message, "tool_calls");
        if (!cJSON_IsArray(tool_calls) || cJSON_GetArraySize(tool_calls) == 0)
        {
            fprintf(stderr, "no tool call, coping answer to buffer result\n");
            // no tool calls -> print the message content
            char * response = cJSON_GetStringValue(content);
            final_response->data = strdup(response);
            final_response->size = strlen(response);
            done = 1;
        }
        else
        {
            //testing why I'm getting internal server error from remote agent
            agent_tries++;
            // int totalToolCalled = cJSON_GetArraySize(tool_calls);
            // fprintf(stderr, "tool call to execute: %d\n", totalToolCalled);
            // for (int i = 0; i < totalToolCalled; i++)
            // {
            //     cJSON *current_tool_call = cJSON_GetArrayItem(tool_calls, i);
            //     ExecuteToolCode tool_result = readTool->execute_tool(readTool, current_tool_call, &toolResult);
            //     if (tool_result != ExecuteTool_OK)
            //     {
            //         fprintf(stderr, "failed to execute tool %s\n", readTool->name);
            //         cJSON_Delete(json);
            //         return 1;
            //     }
            //     cAgent_addPromptTool(agent, toolResult.tool_call_id, toolResult.content);
            //     free(toolResult.tool_call_id);
            //     free(toolResult.content);
            // }
        }
        if(agent_tries > 2){
            done = 1;
        }

        cJSON_Delete(json);
    }

    return 0;
}