#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "utils.h"

const size_t MAX_CMD_ARG_CNT = 2;

typedef int32_t command_data_t;

const command_data_t SIGNATURE = (command_data_t)0xd1dfaedf;

typedef enum command_type_t
{
    COMMAND_TYPE_ARITHMETIC_BINARY,
    COMMAND_TYPE_ARITHMETIC_UNARY,
    COMMAND_TYPE_CONTROL
} command_type_t;

typedef enum cmd_callback_ret_t
{
    CMD_CALLBACK_HALT,
    CMD_CALLBACK_CONTINUE
} cmd_callback_ret_t;

typedef cmd_callback_ret_t (*cmd_callback)(command_data_t, command_data_t);

typedef struct command_t
{
    const char*    name;
    command_data_t code;
    size_t         arg_cnt;
    command_type_t cmd_type;
    cmd_callback   callback;
} command_t;

extern cmd_callback_ret_t cmd_push(            command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_add (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_sub (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_mul (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_div (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_sqr (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_out (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);
extern cmd_callback_ret_t cmd_hlt (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b);

// TODO make validator
const command_t commands[] = 
{
    { "PUSH", 0x00, 1, COMMAND_TYPE_CONTROL          , cmd_push },
    { "ADD",  0x01, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_add  },
    { "SUB",  0x02, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_sub  },
    { "MUL",  0x03, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_mul  },
    { "DIV",  0x04, 0, COMMAND_TYPE_ARITHMETIC_BINARY, cmd_div  },
    { "SQR",  0x05, 0, COMMAND_TYPE_ARITHMETIC_UNARY , cmd_sqr  },
    { "HLT",  0x06, 0, COMMAND_TYPE_CONTROL          , cmd_hlt  },
    { "OUT",  0x07, 0, COMMAND_TYPE_CONTROL          , cmd_out  }
};

