#ifndef BASH_TOOL_H
#define BASH_TOOL_H

#include <cjson/cJSON.h>
#include "tool.h"

/*Creates the bash tool*/
cAgentTool *cAgentTool_createBashTool(void);
/* frees allocated memory by the bash tool */
void cAgentTool_destroyBashTool(cAgentTool * tool);
/**
 * Adds bash tool object definition to LLM json request
 */
void add_bash_tool(cJSON *tools);

ExecuteToolCode execute_bash(cAgentTool *tool, cJSON *tool_call, cAgentToolResult * result);

#endif //BASH_TOOL_H