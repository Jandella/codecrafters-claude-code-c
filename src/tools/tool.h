#ifndef TOOL_H
#define TOOL_H

#include <stdlib.h>
#include <cjson/cJSON.h>

typedef enum {
    ExecuteTool_OK = 0,
    ExecuteTool_Fail = 1,
    ExecuteTool_WrongTool = 2
} ExecuteToolCode;

typedef struct cAgentToolResult {
    char *tool_call_id;
    char *content;
} cAgentToolResult;

typedef struct cAgentTool
{
    char *name;
    /*tool properties*/
    void *properties;
    /*pointer to function that destroy potentally allocated properties of tool*/
    void (*destroy_properties)(void *properties);
    /*pointer to function that executes the tool. Need toll_call json response and the result*/
    ExecuteToolCode (*execute_tool)(struct cAgentTool *self, cJSON *tool_call, cAgentToolResult *result);
    void (*add_to_json)(cJSON *json);
} cAgentTool;




cAgentTool *cAgentTool_create(char *name);
void cAgentTool_destroy(cAgentTool *tool);


#endif // TOOL_H
