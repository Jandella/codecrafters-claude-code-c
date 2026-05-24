#include "read_tool.h"

void add_read_tool(cJSON *tools) {
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
long get_file_size(FILE *fptr) {
    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);
    return fsize;
}