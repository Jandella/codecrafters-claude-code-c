#ifndef READ_TOOL_H
#define READ_TOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "tool.h"

typedef enum {
    ReadTool_OK = 0,
    ReadTool_Invalid = 1
} ReadToolCode;
/**
 * Adds read tool object definition to LLM json request
 */
void add_read_tool(cJSON *tools);

/**
 * Returns the file size. Can return -1 if error happens.
 */
long get_file_size(FILE *fptr);

/**
 * Parses the argument of the tool and writes in the buffer the result.
 * Returns -1 if it fails, the argument length if it succeded
 */
size_t retrieve_argument_from_tool(char *raw_arguments, char *buffer, size_t buffer_size);

ReadToolCode execute_read(cJSON *tool_call, cAgentTool * result);

#endif //READ_TOOL_H