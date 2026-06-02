#ifndef AGENT_H
#define AGENT_H
#include "tool.h"
#include <cjson/cJSON.h>


typedef struct cAgent
{
    cJSON *json_messages;
} cAgent;



/*Create an agent and allocate its memory*/
cAgent * cAgent_createAgent(void);
/*Free all agent allocated memory*/
void cAgent_destroyAgent(cAgent* agent);

void cAgent_addUserPrompt(cAgent *agent, char *content);
/* Adds a json message to the message agent list */
void cAgent_addJsonPrompt(cAgent * agent, cJSON *json_message);
/*Adds a tool prompt to the agent message array*/
void cAgent_addPromptTool(cAgent *agent, char * tool_id, char *content);


#endif