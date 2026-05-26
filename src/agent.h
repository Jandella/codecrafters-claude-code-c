#ifndef AGENT_H
#define AGENT_H

typedef struct message_prompt {
    char *role;
    char *tool_id;
    char *content;
} message_prompt;

typedef struct messages_array {
    message_prompt *first;
    int count;
    int current_size;
} messages_array;

typedef struct cAgent
{
    messages_array *messages;
} cAgent;




cAgent * cAgent_createAgent(void);
void cAgent_destroyAgent(cAgent* agent);

message_prompt * cAgent_addPrompt(cAgent *agent, char* content);
message_prompt * cAgent_addPromptTool(cAgent *agent, char * tool_id, char *content);

#endif