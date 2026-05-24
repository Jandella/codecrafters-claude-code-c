#include <stdio.h>
#include <cjson/cJSON.h>

/**
 * Adds read tool object definition to LLM json request
 */
void add_read_tool(cJSON *tools);

/**
 * Returns the file size. Can return -1 if error happens.
 */
long get_file_size(FILE *fptr);