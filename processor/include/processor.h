#pragma once

#include <stdio.h>

#include "commands.h"

const command_data_t BYTECODE_VERSION = 0x00000001;

typedef enum processor_err_t
{
    PROCESSOR_ERR_NONE          =      0,
    PROCESSOR_ERR_READ_ERR      = 1 << 0,
    PROCESSOR_ERR_PARSE_ERR     = 1 << 1, 
    PROCESSOR_ERR_END_OF_BUFFER = 1 << 2,
    PROCESSOR_ERR_ALLOC_FAIL    = 1 << 3,
    PROCESSOR_ERR_CMD_STACK_ERR = 1 << 4,
    PROCESSOR_ERR_CMD_UNKNOWN   = 1 << 5,
    PROCESSOR_ERR_METADATA      = 1 << 6
} processor_err_t;

void processor_set_err(processor_err_t err, processor_err_t err_new);

int processor_is_err(processor_err_t err, processor_err_t is_set);

processor_err_t processor_ctor(processor_t* proc, FILE* file);

processor_err_t processor_run(processor_t* proc, const command_t* cmdarr, size_t cmdarr_size);

void processor_dtor(processor_t* proc);

void processor_dump(processor_t* proc);
