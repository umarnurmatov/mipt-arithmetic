#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "utils.h"
#include "stack.h"

const size_t MAX_CMD_ARG_CNT  = 2;
const size_t MAX_REG_NAME_LEN = 3;

typedef int32_t command_data_t;

const command_data_t SIGNATURE = (command_data_t)0xd1dfaedf;

struct processor_t;

typedef enum command_type_t
{
    COMMAND_TYPE_ARITHMETIC_BINARY,
    COMMAND_TYPE_ARITHMETIC_UNARY,
    COMMAND_TYPE_CONTROL,
    COMMAND_TYPE_STACK,
    COMMAND_TYPE_REGISTER,
    COMMAND_TYPE_JUMP,
    COMMAND_TYPE_CALL,
    COMMAND_TYPE_RET
} command_type_t;

typedef enum cmd_callback_ret_t
{
    CMD_CALLBACK_HALT,
    CMD_CALLBACK_CONTINUE,
    CMD_CALLBACK_ERR
} cmd_callback_ret_t;

typedef struct cmd_callback_err_t
{
    cmd_callback_ret_t ret;
    int err;
} cmd_callback_err_t;

typedef cmd_callback_err_t (*cmd_callback)(processor_t* proc, command_data_t, command_data_t);

typedef struct command_t
{
    const char*    name;
    command_data_t code;
    size_t         arg_cnt;
    command_type_t cmd_type;
    cmd_callback   callback;
} command_t;

typedef struct proc_reg_t
{
    const char*    name;
    command_data_t num;
} proc_reg_t;

typedef struct processor_t
{
    stack_t         stack;
    command_data_t* cmdbuf;
    size_t          cmdbuf_size;
    size_t          pc;
    command_data_t* regfile;
} processor_t;

extern cmd_callback_err_t cmd_push (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_pushr(            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_popr (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_add  (            processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_sub  (            processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_mul  (            processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_div  (            processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_sqr  (            processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_out  (            processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_hlt  (ATTR_UNUSED processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_jmp  (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_jb   (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_jbe  (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_ja   (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_jae  (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_je   (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_jne  (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_call (            processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_err_t cmd_ret  (            processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);

// TODO make validator
const command_t cmdarr[] = 
{
    { "PUSH" , 0x00, 1, COMMAND_TYPE_STACK            , cmd_push  },
    { "PUSHR", 0x01, 1, COMMAND_TYPE_REGISTER         , cmd_pushr },
    { "POPR" , 0x02, 1, COMMAND_TYPE_REGISTER         , cmd_popr  },
    { "ADD"  , 0x03, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_add   },
    { "SUB"  , 0x04, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_sub   },
    { "MUL"  , 0x05, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_mul   },
    { "DIV"  , 0x06, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_div   },
    { "SQR"  , 0x07, 0, COMMAND_TYPE_ARITHMETIC_UNARY , cmd_sqr   },
    { "SQR"  , 0x08, 0, COMMAND_TYPE_ARITHMETIC_UNARY , cmd_sqr   },
    { "HLT"  , 0x09, 0, COMMAND_TYPE_CONTROL          , cmd_hlt   },
    { "OUT"  , 0x0A, 0, COMMAND_TYPE_CONTROL          , cmd_out   },
    { "JMP"  , 0x0B, 1, COMMAND_TYPE_JUMP             , cmd_jmp   },
    { "JB"   , 0x0C, 1, COMMAND_TYPE_JUMP             , cmd_jb    },
    { "JBE"  , 0x0D, 1, COMMAND_TYPE_JUMP             , cmd_jbe   },
    { "JA"   , 0x0E, 1, COMMAND_TYPE_JUMP             , cmd_ja    },
    { "JAE"  , 0x0F, 1, COMMAND_TYPE_JUMP             , cmd_jae   },
    { "JE"   , 0x10, 1, COMMAND_TYPE_JUMP             , cmd_je    },
    { "JNE"  , 0x11, 1, COMMAND_TYPE_JUMP             , cmd_jne   },
    { "CALL" , 0x12, 1, COMMAND_TYPE_CALL             , cmd_call  },
    { "RET"  , 0x13, 0, COMMAND_TYPE_RET              , cmd_ret   },
};

const proc_reg_t proc_regs[] = 
{
    { "RAX", 0x00 },
    { "RBX", 0x01 },
    { "RCX", 0x02 },
    { "RDX", 0x03 }
};

