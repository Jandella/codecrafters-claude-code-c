#include <stdio.h>
#include <string.h>
#include "write_tool.h"
/* this struct will hold parameter values retrieved from JSON tool call */
struct write_parameters
{
    char *file_path;
    char *file_content;
};

static char *write_tool_name = "write_file";

cAgentTool *cAgentTool_createWriteTool(void)
{
    cAgentTool *node = cAgentTool_create(write_tool_name);
    if (node)
    {
        node->execute_tool = execute_write;
        node->add_to_json = add_write_tool;
    }
    return node;
}
void cAgentTool_destroyWriteTool(cAgentTool *tool)
{
}
/**
 * Adds write tool object definition to LLM json request
 */
void add_write_tool(cJSON *tools)
{
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddItemToArray(tools, tool);
    cJSON_AddStringToObject(tool, "type", "function");
    cJSON *function = cJSON_AddObjectToObject(tool, "function");
    cJSON_AddStringToObject(function, "name", write_tool_name);
    cJSON_AddStringToObject(function, "description", "Write content to a new file or ovewrite content to the existing file.");
    cJSON *parameters = cJSON_AddObjectToObject(function, "parameters");
    cJSON *properties = cJSON_AddObjectToObject(parameters, "properties");
    cJSON_AddStringToObject(parameters, "type", "object");
    cJSON *file_path = cJSON_AddObjectToObject(properties, "file_path");
    cJSON_AddStringToObject(file_path, "type", "string");
    cJSON_AddStringToObject(file_path, "description", "The path to the file to write to");
    cJSON *content = cJSON_AddObjectToObject(parameters, "content");
    cJSON_AddStringToObject(content, "type", "string");
    cJSON_AddStringToObject(content, "description", "The content to write to the file");
    cJSON *required_array = cJSON_AddArrayToObject(parameters, "required");
    cJSON *required_file_path_element = cJSON_CreateString("file_path");
    cJSON *required_content_element = cJSON_CreateString("content");
    cJSON_AddItemToArray(required_array, required_file_path_element);
    cJSON_AddItemToArray(required_array, required_content_element);
}
static int retrieve_argument_from_tool(char *raw_arguments, struct write_parameters *parsed_parameters)
{
    cJSON *arguments_json = cJSON_Parse(raw_arguments);
    if (!arguments_json)
    {
        return 1;
    }

    // with ollama it's "file_path", with claude it's "parameter"
    cJSON *file_path_object = cJSON_GetObjectItem(arguments_json, "file_path");
    char *file_path = cJSON_GetStringValue(file_path_object);
    if (!file_path)
    {
        file_path_object = cJSON_GetObjectItem(arguments_json, "parameter");
        file_path = cJSON_GetStringValue(file_path_object);
    }

    if (!file_path)
    {
        cJSON_Delete(arguments_json);
        fprintf(stderr, "file_path or parameter not found in tool arguments %s\n", raw_arguments);
        return 1;
    }

    cJSON *file_content_object = cJSON_GetObjectItem(arguments_json, "content");
    char *content = cJSON_GetStringValue(file_content_object);
    if(!content)
    {
        cJSON_Delete(arguments_json);
        fprintf(stderr, "content not found in tool arguments %s\n", raw_arguments);
        return 1;
    }

    parsed_parameters->file_path = strdup(file_path);
    parsed_parameters->file_content = strdup(content);

    cJSON_Delete(arguments_json);
    return 0;
}

/**
 * Manages the opening of the file with error&reasource clean up
 */
static ExecuteToolCode using_file(const char *file_name, const char *open_mode, char *content, cAgentToolResult *result, ExecuteToolCode (*callback)(FILE *, char *, cAgentToolResult *))
{
    FILE *fptr = fopen(file_name, open_mode);
    ExecuteToolCode code = ExecuteTool_Fail;
    if (fptr)
    {
        code = callback(fptr, content, result);
        fclose(fptr);
    }
    else
    {
        // perror(sprintf("Error opening file in \"%s\" mode", open_mode));
        fprintf(stderr, "Error opening file in \"%s\" mode\n", open_mode);
    }
    return code;
}

/*
* Executes the writing of the file with fprintf.
* Caller should free the memory in the buffer result.
*/
static ExecuteToolCode execute_write_internal(FILE *stream, char* content, cAgentToolResult *result) {
    fprintf(stream, content);
    result->content = strdup("Done");
    return ExecuteTool_OK;
}

ExecuteToolCode execute_write(cAgentTool *tool, cJSON *tool_call, cAgentToolResult *result)
{
    cJSON *function_object = cJSON_GetObjectItem(tool_call, "function");
    cJSON *name = cJSON_GetObjectItem(function_object, "name");
    char *name_write = cJSON_GetStringValue(name);
    fprintf(stderr, "%s\n", name_write);

    if (strcmp(write_tool_name, name_write) != 0)
    {
        fprintf(stderr, "Incorrect tool name. Expected %s, actual %s\n", write_tool_name,  name_write);
        return ExecuteTool_WrongTool;
    }

    cJSON *id = cJSON_GetObjectItem(tool_call, "id");
    char *id_read = cJSON_GetStringValue(id);
    result->tool_call_id = strdup(id_read);
    cJSON *arguments_dictionary = cJSON_GetObjectItem(function_object, "arguments");
    char *raw_arguments = cJSON_GetStringValue(arguments_dictionary);
    fprintf(stderr, "raw args: %s\n", raw_arguments);
    struct write_parameters p = {NULL, NULL};
    int retrieve_argument_result = retrieve_argument_from_tool(raw_arguments, &p);
    if (retrieve_argument_result != 0)
    {
        fprintf(stderr, "Failed to retrieve argument from tool\n");
        return ExecuteTool_Fail;
    }

    fprintf(stderr, "Write tool to read file at path %s\n", p.file_path);
    result->content = NULL;
    ExecuteToolCode codeResult = using_file(p.file_path, "w", p.file_content, result, execute_write_internal);
    //clean up
    free(p.file_path);
    free(p.file_content);
    return codeResult;
}
