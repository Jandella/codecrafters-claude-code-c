#include "read_tool.h"

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
 * Returns the file size. Can return -1 if error happens.
 */
long get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);
    return fsize;
}

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
    if(!file_path)
    {
        file_path_object = cJSON_GetObjectItem(parsed_arguments, "file_path");
        file_path = cJSON_GetStringValue(file_path_object);
    }

    if(!file_path){
        cJSON_Delete(parsed_arguments);
        fprintf(stderr, "file_path or parameter not found in tool arguments %s\n", raw_arguments);
        return -1;
    }
    size_t length = (size_t)strlen(file_path);
    if(length > buffer_size){
        cJSON_Delete(parsed_arguments);
        fprintf(stderr, "The size of the argument is greather than the buffer length (%d > %d)\n", length, buffer_size);
        return -1;
    }
    strcpy(buffer, file_path);
    cJSON_Delete(parsed_arguments);
    return length;
}