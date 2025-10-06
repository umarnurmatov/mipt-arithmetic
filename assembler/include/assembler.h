#pragma once

#include "commands.h"
#include "fileline_arr.h"

const command_data_t BYTECODE_VERSION = 0x00000001;

typedef enum assembler_err_t
{
    ASSEMBLER_ERR_NONE,
    ASSEMBLER_ERR_PARSE_FAIL,
    ASSEMBLER_ERR_ALLOC_FAIL,
    ASSEMBLER_ERR_CMD_UNKNOWN,
    ASSEMBLER_ERR_WRITE
} assembler_err_t;

assembler_err_t assembler_assemble_file(fileline_arr_t* filearr, const command_t* cmdarr, size_t cmdarr_size, command_data_t** cmdbuf, size_t* cmdbuf_size);

assembler_err_t assembler_write_to_file(FILE* file, command_data_t* cmdbuf, size_t cmdbuf_size);

const char* assembler_strerr(assembler_err_t err);
