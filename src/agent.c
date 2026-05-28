#include "agent.h"
#include <stdlib.h>
#include <string.h>

static int DEFAULT_SIZE = 10;

cAgent *cAgent_createAgent(void)
{
    cAgent *node = (cAgent *)malloc(sizeof(cAgent));
    if (node)
    {
        node->messages = NULL;
    }

    return node;
}

void cAgent_destroyAgent(cAgent *agent)
{
    if (!agent)
        return;

    if (agent->messages)
    {
        messages_list *ma = agent->messages;
        if (ma->first)
        {
            // need to free strings
            for (size_t i = 0; i < ma->count; i++)
            {
                if (ma->first[i].role)
                    free(ma->first[i].role);
                if (ma->first[i].tool_id)
                    free(ma->first[i].tool_id);
                if (ma->first[i].content)
                    free(ma->first[i].content);
            }
            // free the messages array
            free(ma->first);
        }
        // free array containter
        free(ma);
        agent->messages = NULL;
    }
    
    // free agent
    free(agent);
}

/*internal constructor for messages array inside agent struct */
static void create_messages(cAgent *agent)
{
    if (!agent->messages)
    {
        agent->messages = (messages_list *)malloc(sizeof(messages_list));
        agent->messages->first = (message_prompt *)malloc(DEFAULT_SIZE * sizeof(message_prompt));
        memset(agent->messages->first, '\0', (DEFAULT_SIZE * sizeof(message_prompt)));
        agent->messages->count = 0;
        agent->messages->current_size = DEFAULT_SIZE;
    }
}

/*internal function to add a generic message*/
static message_prompt *add_message(cAgent *agent, char *role, char *tool_id, char *content)
{
    if (!agent)
        return NULL;
    if (!agent->messages)
        create_messages(agent);
    messages_list *ma = agent->messages;
    int index = agent->messages->count;
    if (index >= ma->current_size)
    {
        ma->first = realloc(ma->first, ma->current_size + (DEFAULT_SIZE * sizeof(message_prompt)));
        ma->current_size += DEFAULT_SIZE;
    }

    ma->first[index].role = NULL;
    ma->first[index].tool_id = NULL;
    ma->first[index].content = NULL;
    // MEMO: strdup allocs memory
    if (role)
    {
        ma->first[index].role = strdup(role);
    }
    if (tool_id)
    {
        ma->first[index].tool_id = strdup(tool_id);
    }
    if (content)
    {
        ma->first[index].content = strdup(content);
    }
    ma->count++;
    message_prompt *ptr = &ma->first[index];
    return ptr;
}

message_prompt *cAgent_addPrompt(cAgent *agent, char *content)
{
    return add_message(agent, "user", NULL, content);
}
message_prompt *cAgent_addPromptTool(cAgent *agent, char *tool_id, char *content)
{
    return add_message(agent, "tool", tool_id, content);
}


