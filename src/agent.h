#ifndef AGENT_H
#define AGENT_H
#include "tools/tool.h"
#include <cjson/cJSON.h>

typedef struct cToolList
{
    cAgentTool *element;
    struct cToolList * next;
} cToolList;


typedef struct cAgent
{
    cJSON *json_messages;
    cToolList *tools;
    int availableTools;
} cAgent;



/*Create an agent and allocate its memory*/
cAgent * cAgent_createAgent(void);
/*Free all agent allocated memory*/
void cAgent_destroyAgent(cAgent* agent);

/* Adds a user prompt to the agent message array */
void cAgent_addUserPrompt(cAgent *agent, char *content);
/* Adds a json message to the agent message array */
void cAgent_addJsonPrompt(cAgent * agent, cJSON *json_message);
/*Adds a tool prompt to the agent message array*/
void cAgent_addToolPrompt(cAgent *agent, char * tool_id, char *content);

/* Adds a tool capability to the agent */
void cAgent_addTool(cAgent *agent, cAgentTool * tool);
/* Execute the given tool and write the result */
ExecuteToolCode cAgent_executeTool(cAgent *cAgent, cJSON *tool_call, cAgentToolResult *result);

#endif