#include "read_tool.h"

cAgentTool *cAgentTool_createReadTool(void)
{
    cAgentTool *node = cAgentTool_create("Read");
    if (node)
    {
        node->execute_tool = execute_read;
    }
    return node;
}

void add_read_tool(cJSON *tools)
{
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddItemToArray(tools, tool);
    cJSON_AddStringToObject(tool, "type", "function");
    cJSON *function = cJSON_AddObjectToObject(tool, "function");
    cJSON_AddStringToObject(function, "name", "Read");
    cJSON_AddStringToObject(function, "description", "Read and return the contents of a file");
    cJSON *parameters = cJSON_AddObjectToObject(function, "parameters");
    cJSON_AddStringToObject(function, "type", "object");
    cJSON *properties = cJSON_AddObjectToObject(parameters, "properties");
    cJSON *file_path = cJSON_AddObjectToObject(properties, "file_path");
    cJSON_AddStringToObject(file_path, "type", "string");
    cJSON_AddStringToObject(file_path, "description", "The path to the file to read");
    cJSON *required_array = cJSON_AddArrayToObject(parameters, "required");
    cJSON *required_element = cJSON_CreateString("file_path");
    cJSON_AddItemToArray(required_array, required_element);
}
/**
 * Manages the opening of the file with error&reasource clean up
 */
static ExecuteToolCode using_file(const char *file_name, const char *open_mode, cAgentToolResult *result, ExecuteToolCode (*callback)(FILE *, cAgentToolResult *))
{
    FILE *fptr = fopen(file_name, open_mode);
    ExecuteToolCode code = ExecuteTool_Fail;
    if (fptr)
    {
        code = callback(fptr, result);
        fclose(fptr);
    }
    else
    {
        // perror(sprintf("Error opening file in \"%s\" mode", open_mode));
        fprintf(stderr, "Error opening file in \"%s\" mode\n", open_mode);
    }
    return code;
}
/**
 * Returns the file size. Can return -1 if error happens.
 */
long get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);
    return fsize;
}

/**
 * Parses the argument of the tool and writes in the buffer the result.
 * Returns -1 if it fails, the argument length if it succeded
 */
size_t retrieve_argument_from_tool(char *raw_arguments, char *buffer, size_t buffer_size)
{
    cJSON *parsed_arguments = cJSON_Parse(raw_arguments);
    if (!parsed_arguments)
    {
        fprintf(stderr, "Failed to parse function arguments\n");
        return -1;
    }

    // with ollama it's "file_path", with claude it's "parameter"
    cJSON *file_path_object = cJSON_GetObjectItem(parsed_arguments, "parameter");
    char *file_path = cJSON_GetStringValue(file_path_object);
    if (!file_path)
    {
        file_path_object = cJSON_GetObjectItem(parsed_arguments, "file_path");
        file_path = cJSON_GetStringValue(file_path_object);
    }

    if (!file_path)
    {
        cJSON_Delete(parsed_arguments);
        fprintf(stderr, "file_path or parameter not found in tool arguments %s\n", raw_arguments);
        return -1;
    }
    size_t length = (size_t)strlen(file_path);
    if (length > buffer_size)
    {
        cJSON_Delete(parsed_arguments);
        fprintf(stderr, "The size of the argument is greather than the buffer length (%d > %d)\n", length, buffer_size);
        return -1;
    }
    strcpy(buffer, file_path);
    cJSON_Delete(parsed_arguments);
    return length;
}

/*
 * Executes the read of the file with fread, allocating the memory to store the data read.
 * Caller should free the memory in the buffer result.
 */
static ExecuteToolCode execute_read_internal(FILE *stream, cAgentToolResult *result)
{
    long file_size = get_file_size(stream);
    if (file_size == -1)
    {
        fprintf(stderr, "Failed to get the file size\n");
        return ExecuteTool_Fail;
    }
    result->content = malloc(sizeof(char) * file_size + 1);
    if (!result->content)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        return ExecuteTool_Fail;
    }
    size_t how_many = fread(result->content, file_size, 1, stream);
    if (how_many == 0 && ferror(stream))
    {
        int error = ferror(stream);
        fprintf(stderr, "%x\n", error);
        return ExecuteTool_Fail;
    }
    result->content[file_size] = '\0';
    fprintf(stderr, "File content:\n%s\n", result->content);
    return ExecuteTool_OK;
}

ExecuteToolCode execute_read(cAgentTool *tool, cJSON *tool_call, cAgentToolResult *result)
{
    cJSON *function_object = cJSON_GetObjectItem(tool_call, "function");
    cJSON *name = cJSON_GetObjectItem(function_object, "name");
    char *name_read = cJSON_GetStringValue(name);
    fprintf(stderr, "%s\n", name_read);

    if (strcmp("Read", name_read) != 0)
    {
        fprintf(stderr, "Incorrect tool name. Expected Read, actual %s\n", name_read);
        return ExecuteTool_Fail;
    }
    cJSON *id = cJSON_GetObjectItem(tool_call, "id");
    char *id_read = cJSON_GetStringValue(id);
    result->tool_call_id = strdup(id_read);
    cJSON *arguments_dictionary = cJSON_GetObjectItem(function_object, "arguments");
    char *raw_arguments = cJSON_GetStringValue(arguments_dictionary);
    fprintf(stderr, "raw args: %s\n", raw_arguments);
    char file_path[100];
    size_t length = retrieve_argument_from_tool(raw_arguments, file_path, 100);
    if (length < 0)
    {
        fprintf(stderr, "Failed to retrieve argument from tool\n");
        return ExecuteTool_Fail;
    }

    fprintf(stderr, "Read tool to read file at path %s\n", file_path);
    result->content = NULL;
    ExecuteToolCode codeResult = using_file(file_path, "rb", result, execute_read_internal);
    return codeResult;
}