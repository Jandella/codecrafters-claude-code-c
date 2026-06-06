#include <stdio.h>
#include <string.h>
#include "bash_tool.h"

struct bash_parameters
{
    char *command;
};

static char *bash_tool_name = "bash";

cAgentTool *cAgentTool_createBashTool(void)
{
    cAgentTool *node = cAgentTool_create(bash_tool_name);
    if (node)
    {
        node->execute_tool = execute_bash;
        node->add_to_json = add_bash_tool;
    }
    return node;
}
/* frees allocated memory by the bash tool */
void cAgentTool_destroyBashTool(cAgentTool *tool)
{
}
/**
 * Adds bash tool object definition to LLM json request
 */
void add_bash_tool(cJSON *tools)
{
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddItemToArray(tools, tool);
    cJSON_AddStringToObject(tool, "type", "function");
    cJSON *function = cJSON_AddObjectToObject(tool, "function");
    cJSON_AddStringToObject(function, "name", bash_tool_name);
    cJSON_AddStringToObject(function, "description", "Execute a shell command.");
    cJSON *parameters = cJSON_AddObjectToObject(function, "parameters");
    cJSON *properties = cJSON_AddObjectToObject(parameters, "properties");
    cJSON_AddStringToObject(parameters, "type", "object");
    cJSON *command = cJSON_AddObjectToObject(properties, "command");
    cJSON_AddStringToObject(command, "type", "string");
    cJSON_AddStringToObject(command, "description", "The command to execute");
    cJSON *required_array = cJSON_AddArrayToObject(parameters, "required");
    cJSON *required_command_element = cJSON_CreateString("command");
    cJSON_AddItemToArray(required_array, required_command_element);
}

static int retrieve_argument_from_tool(char *raw_arguments, struct bash_parameters *parsed_parameters)
{
    cJSON *arguments_json = cJSON_Parse(raw_arguments);
    if (!arguments_json)
    {
        return 1;
    }

    cJSON *command_object = cJSON_GetObjectItem(arguments_json, "command");
    char *command = cJSON_GetStringValue(command_object);

    if (!command)
    {
        cJSON_Delete(arguments_json);
        fprintf(stderr, "command  not found in tool arguments %s\n", raw_arguments);
        return 1;
    }
    parsed_parameters->command = strdup(command);

    cJSON_Delete(arguments_json);
    return 0;
}

ExecuteToolCode execute_bash(cAgentTool *tool, cJSON *tool_call, cAgentToolResult *result)
{
    cJSON *function_object = cJSON_GetObjectItem(tool_call, "function");
    cJSON *name = cJSON_GetObjectItem(function_object, "name");
    char *name_write = cJSON_GetStringValue(name);
    fprintf(stderr, "%s\n", name_write);

    if (strcmp(bash_tool_name, name_write) != 0)
    {
        fprintf(stderr, "Incorrect tool name. Expected %s, actual %s\n", bash_tool_name, name_write);
        return ExecuteTool_WrongTool;
    }

    cJSON *id = cJSON_GetObjectItem(tool_call, "id");
    char *id_read = cJSON_GetStringValue(id);
    result->tool_call_id = strdup(id_read);
    cJSON *arguments_dictionary = cJSON_GetObjectItem(function_object, "arguments");
    char *raw_arguments = cJSON_GetStringValue(arguments_dictionary);
    fprintf(stderr, "raw args: %s\n", raw_arguments);
    struct bash_parameters p = { NULL };
    int retrieve_argument_result = retrieve_argument_from_tool(raw_arguments, &p);
    if (retrieve_argument_result != 0)
    {
        fprintf(stderr, "Failed to retrieve argument from tool\n");
        return ExecuteTool_Fail;
    }

    fprintf(stderr, "Bash tool to execute command %s\n", p.command);
    result->content = NULL;
    char cmd_with_stderr[8192];
    // 2: sterr
    // >&: redirect the stream to the file descriptor 
    // 1: stdout
    // so 2>&1 means redirect stderr to the stdout: I read both in a single stream.
    snprintf(cmd_with_stderr, sizeof(cmd_with_stderr), "%s 2>&1",
             p.command);
    FILE *fp_proc = popen(cmd_with_stderr, "r");
    if (!fp_proc)
    {
        return ExecuteTool_Fail;
    }

    size_t cap = 4096;
    size_t len = 0;
    result->content = NULL;
    char *out = malloc(cap);
    if(!out) {
        fprintf(stderr, "Failed to allocate bash command output buffer.");
        return ExecuteTool_Fail;
    }
    char chunk[256];
    int error = 0;
    while (fgets(chunk, sizeof(chunk), fp_proc) != NULL)
    {
        size_t chunk_len = strlen(chunk);
        if (len + chunk_len >= cap)
        {
            cap *= 2;
            char *tmp = realloc(out, cap);
            if(!tmp) 
            {
                fprintf(stderr, "Failed to reallocate bash command output buffer.");
                error = 1;
                break;
            }
            out = tmp;
        }
        strcpy(out + len, chunk);
        len += chunk_len;
    }
    free(p.command);

    if(error > 0)
    {
        free(out);
        return ExecuteTool_Fail;
    }

    pclose(fp_proc);
    result->content = strdup(out);
    free(out);
    return ExecuteTool_OK;
}