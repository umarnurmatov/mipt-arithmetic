#pragma once

#include <stdio.h>

#include "commands.h"

const command_data_t BYTECODE_VERSION = 0x00000001;

typedef enum processor_err_t
{
    PROCESSOR_ERR_NONE         ,
    PROCESSOR_ERR_READ_ERR     ,
    PROCESSOR_ERR_PARSE_ERR    , 
    PROCESSOR_ERR_END_OF_BUFFER,
    PROCESSOR_ERR_ALLOC_FAIL   ,
    PROCESSOR_ERR_CMD_STACK_ERR,
    PROCESSOR_ERR_CMD_UNKNOWN
} processor_err_t;

processor_err_t processor_load(FILE* file, command_data_t** cmdbuf, size_t* cmdbuf_size);

processor_err_t processor_run(command_data_t *cmdbuf, size_t cmdbuf_size, const command_t* cmdarr, size_t cmdarr_size);
