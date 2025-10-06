#include "processor.h"

#include <cstdio>
#include <math.h>

#include "assertutils.h"
#include "commands.h"
#include "ioutils.h"
#include "logutils.h"
#include "memutils.h"
#include "utils.h"
#include "stack.h"

static const size_t METAINFO_LENGTH = 2;

processor_err_t _processor_verify_metadata(processor_t* proc);

processor_err_t processor_ctor(processor_t* proc, FILE* file)
{
    utils_assert(file);
    utils_assert(proc);

    if(stack_ctor(&proc->stack, 1) != STACK_ERR_NONE) {
        utils_log(LOG_LEVEL_ERR, "failed to initialize command stack");
        return PROCESSOR_ERR_CMD_STACK_ERR;
    }

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

    proc->cmdbuf      = cmdbuf_tmp;
    proc->cmdbuf_size = file_size_b / sizeof cmdbuf_tmp[0];

    size_t bytes_rd = 
        fread(
            cmdbuf_tmp, 
            sizeof(proc->cmdbuf[0]), 
            proc->cmdbuf_size, 
            file
        );

    if(bytes_rd < proc->cmdbuf_size)
        return PROCESSOR_ERR_READ_ERR;

    if(_processor_verify_metadata(proc) != PROCESSOR_ERR_NONE) {
        utils_log(
            LOG_LEVEL_ERR, 
            "bad metadata"
        );
        return PROCESSOR_ERR_METADATA;
    }

    return PROCESSOR_ERR_NONE;
}

processor_err_t processor_run(processor_t *proc, const command_t* cmdarr, size_t cmdarr_size)
{
    utils_assert(proc);
    utils_assert(cmdarr);

    command_data_t cmdcode  = 0;
    command_data_t cmdarg_a = 0;
    command_data_t cmdarg_b = 0;

    size_t cmdbuf_ind = METAINFO_LENGTH;

    for( ;; ) {
        cmdcode = proc->cmdbuf[cmdbuf_ind++];

        if((unsigned)cmdcode >= cmdarr_size) {
            utils_log(
                LOG_LEVEL_ERR,
                "unknown command occured"
            );
            return PROCESSOR_ERR_CMD_UNKNOWN;
        }
        
        if     (cmdarr[cmdcode].arg_cnt == 2) {
            cmdarg_a = proc->cmdbuf[cmdbuf_ind++];
            cmdarg_b = proc->cmdbuf[cmdbuf_ind++];
        }
        else if(cmdarr[cmdcode].arg_cnt == 1)
            cmdarg_a = proc->cmdbuf[cmdbuf_ind++];

        cmd_callback_ret_t ret = 
            cmdarr[cmdcode].callback(proc, cmdarg_a, cmdarg_b);

        if(ret == CMD_CALLBACK_HALT)
            break;

        if(cmdbuf_ind >= proc->cmdbuf_size) {
            utils_log(
                LOG_LEVEL_WARN, 
                "buffer end reached before programm halt"
            );
            return PROCESSOR_ERR_END_OF_BUFFER;
        }

    }

    return PROCESSOR_ERR_NONE;
}

void processor_dtor(processor_t* proc)
{
    stack_dtor(&proc->stack);
    NFREE(proc->cmdbuf);
}

processor_err_t _processor_verify_metadata(processor_t* proc)
{
    if(proc->cmdbuf[0] != SIGNATURE)
        return PROCESSOR_ERR_METADATA;
    if(proc->cmdbuf[1] != BYTECODE_VERSION)
        return PROCESSOR_ERR_METADATA;

    return PROCESSOR_ERR_NONE;
}

cmd_callback_ret_t cmd_push(processor_t* proc,             command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_push(&proc->stack, a);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_add (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&proc->stack, &lhs);
    stack_pop (&proc->stack, &rhs);
    stack_push(&proc->stack, lhs + rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_sub (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&proc->stack, &rhs);
    stack_pop (&proc->stack, &lhs);
    stack_push(&proc->stack, lhs - rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_mul (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&proc->stack, &lhs);
    stack_pop (&proc->stack, &rhs);
    stack_push(&proc->stack, lhs * rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_div (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t lhs = 0, rhs = 0;
    stack_pop (&proc->stack, &lhs);
    stack_pop (&proc->stack, &rhs);
    stack_push(&proc->stack, lhs / rhs);

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_sqr (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t val = 0;
    stack_pop (&proc->stack, &val);
    stack_push(&proc->stack, (stack_data_t)sqrtf(val));

    return CMD_CALLBACK_CONTINUE;
}

cmd_callback_ret_t cmd_hlt (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    return CMD_CALLBACK_HALT;
}

cmd_callback_ret_t cmd_out (processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    stack_data_t val = 0;
    stack_pop (&proc->stack, &val);
    printf("%d\n", val);
    stack_push(&proc->stack, val);

    return CMD_CALLBACK_CONTINUE;
}
