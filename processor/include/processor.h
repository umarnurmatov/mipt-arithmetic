#pragma once

#include <stdio.h>

#include "commands.h"

#ifdef _DEBUG

#ifndef IF_DEBUG
#define IF_DEBUG(statement) statement
#endif // _IF_DEBUG

#else

#define IF_DEBUG(statement)

#endif // _DEBUG

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
    PROCESSOR_ERR_METADATA      = 1 << 6,
    PROCESSOR_ERR_CMDBUF_NULL   = 1 << 7,
    PROCESSOR_ERR_REGFILE_NULL  = 1 << 8,
    PROCESSOR_ERR_REG_UNKNOWN   = 1 << 9,
    PROCESSOR_ERR_ZERO_DIV      = 1 << 10,
    PROCESSOR_ERR_DOMAIN_ERR    = 1 << 11,
    PROCESSOR_ERR_INVALID_PC    = 1 << 12,
    PROCESSOR_ERR_RAM_OVERFLOW  = 1 << 14,
    PROCESSOR_ERR_RAM_NULL      = 1 << 13,
    PROCESSOR_ERR_GUI           = 1 << 14
} processor_err_t;

processor_err_t processor_ctor(processor_t* proc, FILE* file);

processor_err_t processor_run(processor_t* proc);

void processor_dtor(processor_t* proc);

void processor_set_err(processor_err_t* err, processor_err_t err_new);

int processor_is_err(processor_err_t err, processor_err_t is_set);

void processor_set_dump_file(processor_t* proc, FILE* stream);

const char * processor_strerr(processor_err_t onehot);

#ifdef _DEBUG

processor_err_t processor_vldtr(processor_t* proc);

void processor_dump(FILE* stream, processor_t* proc, processor_err_t err, const char* msg, const char* file, const char* func, int line);

#endif // _DEBUG
