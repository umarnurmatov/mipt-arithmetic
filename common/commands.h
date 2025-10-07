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
    COMMAND_TYPE_REGISTER
} command_type_t;

typedef enum cmd_callback_ret_t
{
    CMD_CALLBACK_HALT,
    CMD_CALLBACK_CONTINUE
} cmd_callback_ret_t;

typedef cmd_callback_ret_t (*cmd_callback)(processor_t* proc, command_data_t, command_data_t);

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
    command_data_t* regfile;
} processor_t;

extern cmd_callback_ret_t cmd_push (processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_pushr(processor_t* proc,             command_data_t a,             command_data_t b);
extern cmd_callback_ret_t cmd_popr (processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_add  (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_sub  (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_mul  (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_div  (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_sqr  (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_out  (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_hlt  (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);

// TODO make validator
const command_t commands[] = 
{
    { "PUSH" , 0x00, 1, COMMAND_TYPE_CONTROL          , cmd_push  },
    { "PUSHR", 0x01, 2, COMMAND_TYPE_REGISTER         , cmd_pushr },
    { "POPR" , 0x02, 1, COMMAND_TYPE_REGISTER         , cmd_popr  },
    { "ADD"  , 0x03, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_add   },
    { "SUB"  , 0x04, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_sub   },
    { "MUL"  , 0x05, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_mul   },
    { "DIV"  , 0x06, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_div   },
    { "SQR"  , 0x07, 0, COMMAND_TYPE_ARITHMETIC_UNARY , cmd_sqr   },
    { "SQR"  , 0x08, 0, COMMAND_TYPE_ARITHMETIC_UNARY , cmd_sqr   },
    { "HLT"  , 0x09, 0, COMMAND_TYPE_CONTROL          , cmd_hlt   },
    { "OUT"  , 0x0A, 0, COMMAND_TYPE_CONTROL          , cmd_out   }
};

const proc_reg_t proc_regs[] = 
{
    { "RAX", 0x00 },
    { "RBX", 0x01 },
    { "RCX", 0x02 }
};

