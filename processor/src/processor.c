#include "processor.h"

#include <cstdio>
#include <math.h>

#include "assertutils.h"
#include "commands.h"
#include "ioutils.h"
#include "logutils.h"
#include "utils.h"
#include "stack.h"

static STACK_MAKE(cmd_stack);

processor_err_t processor_load(FILE* file, command_data_t** cmdbuf, size_t* cmdbuf_size)
{
    utils_assert(file);
    utils_assert(cmdbuf);
    utils_assert(cmdbuf_size);

    size_t file_size_b = get_file_size(file);
    command_data_t* cmdbuf_tmp =
        (command_data_t*)calloc(1, file_size_b);

    if(cmdbuf_tmp == NULL) {
        utils_log(
            LOG_LEVEL_ERR, 
            "failed to allocate command buffer"
        );
        return PROCESSOR_ERR_ALLOC_FAIL;
    }

    if(file_size_b % sizeof cmdbuf_tmp[0] != 0) {
        utils_log(
            LOG_LEVEL_ERR, 
            "file size [%lu] is not multiple of cmd size [%lu]",
            file_size_b,
            sizeof cmdbuf_tmp[0]
        );
        return PROCESSOR_ERR_PARSE_ERR;
    }

    *cmdbuf      = cmdbuf_tmp;
    *cmdbuf_size = file_size_b / sizeof cmdbuf_tmp[0];

    size_t bytes_rd = fread(cmdbuf_tmp, sizeof(cmdbuf[0]), *cmdbuf_size, file);

    if(bytes_rd < *cmdbuf_size)
        return PROCESSOR_ERR_READ_ERR;

    return PROCESSOR_ERR_NONE;
}

processor_err_t processor_run(command_data_t *cmdbuf, size_t cmdbuf_size, const command_t* cmdarr, size_t cmdarr_size)
{
    utils_assert(cmdbuf);
    utils_assert(cmdarr);

    if(stack_ctor(&cmd_stack, 1) != STACK_ERR_NONE) {
        utils_log(LOG_LEVEL_ERR, "failed to initialize command stack");
        return PROCESSOR_ERR_CMD_STACK_ERR;
    }

    command_data_t cmdcode  = 0;
    command_data_t cmdarg_a = 0;
    command_data_t cmdarg_b = 0;

    size_t cmdbuf_ind = 0;

    for( ;; ) {
        cmdcode = cmdbuf[cmdbuf_ind++];

        if((unsigned)cmdcode >= cmdarr_size) {
            utils_log(
                LOG_LEVEL_ERR,
                "unknown command occured"
            );
            return PROCESSOR_ERR_CMD_UNKNOWN;
        }
        
        if     (cmdarr[cmdcode].arg_cnt == 2) {
            cmdarg_a = cmdbuf[cmdbuf_ind++];
            cmdarg_b = cmdbuf[cmdbuf_ind++];
        }
        else if(cmdarr[cmdcode].arg_cnt == 1)
            cmdarg_a = cmdbuf[cmdbuf_ind++];

        cmd_callback_ret_t ret = 
            cmdarr[cmdcode].callback(cmdarg_a, cmdarg_b);

        if(ret == CMD_CALLBACK_HALT)
            break;

        if(cmdbuf_ind >= cmdbuf_size) {
            utils_log(
                LOG_LEVEL_WARN, 
                "buffer end reached before programm halt"
            );
            return PROCESSOR_ERR_END_OF_BUFFER;
        }

    }

    stack_dtor(&cmd_stack);

    return PROCESSOR_ERR_NONE;
}

cmd_callback_ret_t cmd_push(            command_data_t a, ATTR_UNUSED command_data_t b) 
{
    stack_push(&cmd_stack, a);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_add (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&cmd_stack, &lhs);
    stack_pop (&cmd_stack, &rhs);
    stack_push(&cmd_stack, lhs + rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_sub (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&cmd_stack, &rhs);
    stack_pop (&cmd_stack, &lhs);
    stack_push(&cmd_stack, lhs - rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_mul (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&cmd_stack, &lhs);
    stack_pop (&cmd_stack, &rhs);
    stack_push(&cmd_stack, lhs * rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_div (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&cmd_stack, &lhs);
    stack_pop (&cmd_stack, &rhs);
    stack_push(&cmd_stack, lhs / rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_sqr (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t val = 0;
    stack_pop (&cmd_stack, &val);
    stack_push(&cmd_stack, (stack_data_t)sqrtf(val));

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_hlt (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    return CMD_CALLBACK_HALT;
}

cmd_callback_ret_t cmd_out (ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t val = 0;
    stack_pop (&cmd_stack, &val);
    printf("%d\n", val);
    stack_push(&cmd_stack, val);

    return CMD_CALLBACK_CONTINUE;
}
