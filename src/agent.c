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
        if (!node->json_messages)
        {
            // todo: manage error or retry in the add function ?
        }
        node->tools = NULL;
        node->availableTools = 0;
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
    if (agent->tools)
    {
        cToolList *l = agent->tools;
        for (int i = 0; i < agent->availableTools; i++)
        {
            if (l && l->element)
            {
                cAgentTool_destroy(l->element);
                l->element = NULL;
                l = l->next;
            }
        }
        free(agent->tools);
    }
    // free agent
    free(agent);
}

static void add_json_tool(cAgent *agent, char *tool_id, char *content)
{
    cJSON *msg = cJSON_CreateObject();
    if (!msg)
    {
        // todo: manage errors
        return;
    }
    cJSON_AddStringToObject(msg, "role", "tool");
    cJSON_AddStringToObject(msg, "tool_call_id", tool_id);
    cJSON_AddStringToObject(msg, "content", content);
    cJSON_AddItemToArray(agent->json_messages, msg);
}

void cAgent_addUserPrompt(cAgent *agent, char *content)
{
    cJSON *msg = cJSON_CreateObject();
    if (!msg)
    {
        // todo: manage errors
        return;
    }
    cJSON_AddStringToObject(msg, "role", "user");
    cJSON_AddStringToObject(msg, "content", content);
    cJSON_AddItemToArray(agent->json_messages, msg);
}

void cAgent_addToolPrompt(cAgent *agent, char *tool_id, char *content)
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

void cAgent_addTool(cAgent *agent, cAgentTool *tool)
{
    cToolList *current;
    cToolList *new_node = malloc(sizeof(cToolList));
    if (!new_node)
    {
        // todo: manage error
        return;
    }
    new_node->element = tool;
    new_node->next = NULL;
    if (!agent->tools)
    {
        agent->tools = new_node;
        return;
    }
    current = agent->tools;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = new_node;
    agent->availableTools++;
}
ExecuteToolCode cAgent_executeTool(cAgent *agent, cJSON *tool_call, cAgentToolResult *result)
{
    if (!agent)
        return ExecuteTool_Fail;
    if (agent->availableTools == 0)
    {
        // log ? create reason for failure?
        return ExecuteTool_Fail;
    }

    int found = 0;
    int i = 0;
    ExecuteToolCode final_result = ExecuteTool_Fail;
    cToolList *current_tool = agent->tools;
    while (!found && i <= agent->availableTools)
    {
        if (current_tool)
        {
            if (current_tool->element)
            {
                final_result = current_tool->element->execute_tool(current_tool->element, tool_call, result);
                if (final_result == ExecuteTool_OK)
                {
                    found = 1;
                }
            }
            current_tool = current_tool->next;
        }

        i++;
    }

    return final_result;
}
