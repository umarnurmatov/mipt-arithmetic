#pragma once

#include <stdio.h>

#include "commands.h"

typedef enum processor_err_t
{
    PROCESSOR_ERR_NONE,
    PROCESSOR_ERR_PARSE_ERR,
    PROCESSOR_ERR_EOF,
    PROCESSOR_ERR_ALLOC_FAIL,
    PROCESSOR_ERR_CMD_STACK_ERR
} processor_err_t;

processor_err_t processor_load(FILE* file, command_data_t** cmdbuf, size_t* cmdbuf_size);

processor_err_t processor_run(command_data_t *cmdbuf, size_t cmdbuf_size, const command_t* cmdarr, size_t cmdarr_size);
