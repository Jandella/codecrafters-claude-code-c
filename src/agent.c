#include "agent.h"
#include <stdlib.h>
#include <string.h>

static int DEFAULT_SIZE = 10;

cAgent *cAgent_createAgent(void)
{
    cAgent *node = (cAgent *)malloc(sizeof(cAgent));
    if (node)
    {
        node->json_messages = cJSON_CreateArray();
        if(!node->json_messages){
            //todo: manage error or retry in the add function ?
        }
    }

    return node;
}

void cAgent_destroyAgent(cAgent *agent)
{
    if (!agent)
        return;
    if (agent->json_messages)
    {
        cJSON_Delete(agent->json_messages);
    }
    // free agent
    free(agent);
}



static void add_json_tool(cAgent *agent, char *tool_id, char *content)
{
    cJSON *msg = cJSON_CreateObject();
    if(!msg)
    {
        //todo: manage errors
        return;
    }
    cJSON_AddStringToObject(msg, "role", "tool");
    cJSON_AddStringToObject(msg, "tool_call_id", tool_id);
    cJSON_AddStringToObject(msg, "content", content);
    cJSON_AddItemToArray(agent->json_messages, msg);
}

void cAgent_addUserPrompt(cAgent *agent, char *content) {
    cJSON *msg = cJSON_CreateObject();
    if(!msg)
    {
        //todo: manage errors
        return;
    }
    cJSON_AddStringToObject(msg, "role", "user");
    cJSON_AddStringToObject(msg, "content", content);
    cJSON_AddItemToArray(agent->json_messages, msg);
}

void cAgent_addPromptTool(cAgent *agent, char *tool_id, char *content)
{
    add_json_tool(agent, tool_id, content);
}

void cAgent_addJsonPrompt(cAgent *agent, cJSON *json_message)
{
    cJSON *message_copy = cJSON_Duplicate(json_message, 1);
    // todo: manage errors
    if (message_copy)
        cJSON_AddItemToArray(agent->json_messages, message_copy);
}
