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
    PROCESSOR_ERR_CMD_UNKNOWN  ,
    PROCESSOR_ERR_METADATA     ,
} processor_err_t;

processor_err_t processor_ctor(processor_t* proc, FILE* file);

processor_err_t processor_run(processor_t* proc, const command_t* cmdarr, size_t cmdarr_size);

void processor_dtor(processor_t* proc);
