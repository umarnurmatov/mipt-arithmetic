#pragma once

#include <stdio.h>

typedef enum processor_err_t
{
    PROCESSOR_ERR_NONE,
    PROCESSOR_ERR_PARSE_ERR
} processor_err_t;

typedef int bytecode_t;
typedef int cmdarg_t;

typedef struct cmd_callback_ret_t
{
    cmdarg_t value;
    int code;
} cmd_callback_ret_t;

typedef cmd_callback_ret_t (*cmd_callback)(cmdarg_t, cmdarg_t);

typedef struct command_t
{
    const char* name;
    bytecode_t code;
    size_t arg_cnt;
    cmd_callback callback;
} command_t;

processor_err_t processor_run(FILE* file, bytecode_t* bytecode, command_t* cmdarr, size_t cmdarr_size);
