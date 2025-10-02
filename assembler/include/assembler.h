#pragma once

#include <stdint.h>

#include "fileline_arr.h"

typedef int bytecode_t;

typedef struct command_t
{
    const char* name;
    bytecode_t code;
    size_t arg_cnt;
} command_t;

typedef enum assembler_err_t
{
    ASSEMBLER_ERR_NONE,
    ASSEMBLER_ERR_PARSE_FAIL,
    ASSEMBLER_ERR_ALLOC_FAIL,
    ASSEMBLER_ERR_CMD_UNKNOWN,
    ASSEMBLER_ERR_WRITE
} assembler_err_t;

assembler_err_t assembler_assemble_file(fileline_arr_t* filearr, const command_t* cmdarr, size_t cmdarr_size, bytecode_t** cmdbuf, size_t* cmdbuf_size);

assembler_err_t assembler_write_to_file(FILE* file, bytecode_t* cmdbuf, size_t cmdbuf_size);

const char* assembler_strerr(assembler_err_t err);
