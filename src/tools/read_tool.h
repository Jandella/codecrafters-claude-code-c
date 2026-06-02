#ifndef READ_TOOL_H
#define READ_TOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "tool.h"

/*Creates the read tool*/
cAgentTool *cAgentTool_createReadTool(void);
void cAgentTool_destroyReadTool(cAgentTool * tool);
/**
 * Adds read tool object definition to LLM json request
 */
void add_read_tool(cJSON *tools);

/**
 * Returns the file size. Can return -1 if error happens.
 */
long get_file_size(FILE *fptr);


size_t retrieve_argument_from_tool(char *raw_arguments, char *buffer, size_t buffer_size);

ExecuteToolCode execute_read(cAgentTool *tool, cJSON *tool_call, cAgentToolResult * result);

#endif //READ_TOOL_H