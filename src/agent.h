#ifndef AGENT_H
#define AGENT_H
#include "tool.h"

typedef struct message_prompt {
    char *role;
    char *tool_id;
    char *content;
} message_prompt;

typedef struct messages_list {
    message_prompt *first;
    int count;
    int current_size;
} messages_list;


typedef struct cAgent
{
    messages_list *messages;
} cAgent;



/*Create an agent and allocate its memory*/
cAgent * cAgent_createAgent(void);
/*Free all agent allocated memory*/
void cAgent_destroyAgent(cAgent* agent);

/*Adds a user prompt to the agent message array*/
message_prompt * cAgent_addPrompt(cAgent *agent, char* content);
/*Adds a tool prompt to the agent message array*/
message_prompt * cAgent_addPromptTool(cAgent *agent, char * tool_id, char *content);

#endif