#pragma once

#include "commands.h"
#include "fileline_arr.h"

#define LOG_CATEGORY_ASM "ASSEMBLER"

const command_data_t BYTECODE_VERSION = 0x00000001;

typedef enum assembler_err_t
{
    ASSEMBLER_ERR_NONE,
    ASSEMBLER_ERR_PARSE_FAIL,
    ASSEMBLER_ERR_ALLOC_FAIL,
    ASSEMBLER_ERR_WRITE,
    ASSEMBLER_ERR_SYNTAX
} assembler_err_t;

typedef enum assembler_expr_t
{
    ASSEMBLER_EXPR_CMD,
    ASSEMBLER_EXPR_LBL
} assembler_expr_t;

typedef struct assembler_lbl_t
{
    char* lblstr;
    size_t lblstr_len;
    command_data_t addr;
} assembler_lbl_t;

typedef struct assembler_lblbuf_t
{
    assembler_lbl_t* buf;
    size_t size;
    size_t capacity;
} assembler_lblbuf_t;

typedef struct assembler_t
{
    command_data_t* cmdbuf;
    size_t          cmdbuf_size;
    size_t          cmdbuf_ind;

    command_data_t* lblbuf;
    size_t          lblbuf_size;

    fileline_arr_t  filearr;
    fileline_t*     line_ptr;
    size_t          str_ind;
} assembler_t;

assembler_err_t assembler_ctor(FILE* file, assembler_t* asmblr);

assembler_err_t assembler_assemble(assembler_t* asmblr, int dump_listing);

assembler_err_t assembler_write_to_file(FILE* file, assembler_t* asmblr);

void assembler_dtor(assembler_t* asmblr);

const char* assembler_strerr(assembler_err_t err);

void assembler_dump_syntax_err(assembler_t* asmblr, const char* msg);
