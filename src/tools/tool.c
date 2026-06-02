#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tool.h"

cAgentTool *cAgentTool_create(char *name)
{
    cAgentTool *node = (cAgentTool *)malloc(sizeof(cAgentTool));
    if (node)
    {
        node->name = strdup(name);
        node->properties = NULL;
        node->destroy_properties = NULL;
        node->execute_tool = NULL;
    }

    return node;
}
void cAgentTool_destroy(cAgentTool *tool)
{
    if (!tool)
        return;

    if (tool->name)
        free(tool->name);
    if (tool->properties && tool->destroy_properties)
        tool->destroy_properties(tool->properties);
    tool->execute_tool = NULL;

    free(tool);
}

