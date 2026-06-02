#ifndef WRITE_TOOL_H
#define WRITE_TOOL_H

#include <cjson/cJSON.h>
#include "tool.h"

/*Creates the write tool*/
cAgentTool *cAgentTool_createWriteTool(void);
void cAgentTool_destroyWriteTool(cAgentTool * tool);
/**
 * Adds write tool object definition to LLM json request
 */
void add_write_tool(cJSON *tools);

ExecuteToolCode execute_write(cAgentTool *tool, cJSON *tool_call, cAgentToolResult * result);


#endif //WRITE_TOOL_H