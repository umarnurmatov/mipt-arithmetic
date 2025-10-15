#include "processor.h"

#include <cstdio>
#include <math.h>

#include "assertutils.h"
#include "colorutils.h"
#include "commands.h"
#include "ioutils.h"
#include "logutils.h"
#include "memutils.h"
#include "utils.h"
#include "assertutils.h"
#include "stack.h"

#ifdef _DEBUG

#define PROCESSOR_ASSERT_OK_OR_RETURN_ERR(proc, err)                           \
    if((err = processor_vldtr(proc)) != PROCESSOR_ERR_NONE) {                  \
        processor_dump(stderr, proc, err, "", __FILE__, __func__, __LINE__);   \
        return err;                                                            \
    }

#define PROCESSOR_VERIFY_OK_OR_RETURN_ERR(expr, proc, err, msg)                \
    if(!(expr)) {                                                              \
        processor_dump(stderr, proc, err, msg, __FILE__, __func__, __LINE__);  \
        return err;                                                            \
    }

#else

#define PROCESSOR_VERIFY_OK_OR_RETURN_ERR(expr, proc, err, msg) \
    if(!(expr)) {                                               \
        return err;                                             \
    }

#endif // _DEBUG

static const size_t METAINFO_LENGTH = 2;

static const size_t PROCESSOR_DUMP_BYTES_PER_LINE = 4;

processor_err_t _processor_verify_metadata(processor_t* proc);

processor_err_t processor_ctor(processor_t* proc, FILE* file)
{
    utils_assert(file);
    utils_assert(proc);

    PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
        stack_ctor(&proc->stack, 1) == STACK_ERR_NONE,
        proc,
        PROCESSOR_ERR_CMD_STACK_ERR,
        ""
    );

    size_t file_size_b = get_file_size(file);
    command_data_t* cmdbuf_tmp =
        (command_data_t*)calloc(1, file_size_b);
    
    PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
        cmdbuf_tmp != NULL, 
        proc, 
        PROCESSOR_ERR_ALLOC_FAIL, 
        "error allocating command bufer"
    );

    PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
        file_size_b % sizeof cmdbuf_tmp[0] == 0,
        proc,
        PROCESSOR_ERR_PARSE_ERR,
        "file size is not multiple of cmd size"
    );
    
    proc->cmdbuf      = cmdbuf_tmp;
    proc->cmdbuf_size = file_size_b / sizeof cmdbuf_tmp[0];

    size_t bytes_rd = 
        fread(
            cmdbuf_tmp, 
            sizeof(proc->cmdbuf[0]), 
            proc->cmdbuf_size, 
            file
        );

    PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
        bytes_rd >= proc->cmdbuf_size,
        proc,
        PROCESSOR_ERR_READ_ERR,
        ""
    );
        
    PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
        _processor_verify_metadata(proc) == PROCESSOR_ERR_NONE,
        proc,
        PROCESSOR_ERR_METADATA,
        "bad metadata"
    );

    proc->pc = METAINFO_LENGTH;

    command_data_t* regfile_tmp = (command_data_t*)calloc(SIZEOF(proc_regs), sizeof(command_data_t));

    PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
        cmdbuf_tmp != NULL, 
        proc, 
        PROCESSOR_ERR_ALLOC_FAIL, 
        "error allocating register file"
    );

    proc->regfile = regfile_tmp;

    return PROCESSOR_ERR_NONE;
}

processor_err_t processor_run(processor_t *proc)
{
    utils_assert(proc);
    utils_assert(cmdarr);

    processor_err_t err = PROCESSOR_ERR_NONE;

    PROCESSOR_ASSERT_OK_OR_RETURN_ERR(proc, err);

    command_data_t cmdcode  = 0;
    command_data_t cmdarg_a = 0;
    command_data_t cmdarg_b = 0;

    for( ;; ) {
        PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
            proc->pc < proc->cmdbuf_size,
            proc,
            PROCESSOR_ERR_END_OF_BUFFER,
            ""
        );

        cmdcode = proc->cmdbuf[proc->pc++];
    
        PROCESSOR_VERIFY_OK_OR_RETURN_ERR(
            (unsigned) cmdcode < SIZEOF(cmdarr),
            proc,
            PROCESSOR_ERR_CMD_UNKNOWN,
            ""
        );

        
        if(cmdarr[cmdcode].arg_cnt == 2) {
            cmdarg_a = proc->cmdbuf[proc->pc++];
            cmdarg_b = proc->cmdbuf[proc->pc++];
        }
        else if(cmdarr[cmdcode].arg_cnt == 1)
            cmdarg_a = proc->cmdbuf[proc->pc++];

        cmd_callback_err_t ret = 
            cmdarr[cmdcode].callback(proc, cmdarg_a, cmdarg_b);

        if(ret.ret == CMD_CALLBACK_ERR)
            return (processor_err_t) ret.err;

        else if(ret.ret == CMD_CALLBACK_HALT)
            break;

    }

    return PROCESSOR_ERR_NONE;
}

void processor_dtor(processor_t* proc)
{
    stack_dtor(&proc->stack);
    NFREE(proc->cmdbuf);
    NFREE(proc->regfile);
}

void processor_set_err(processor_err_t* err, processor_err_t err_new)
{
    *err = (processor_err_t)(*err | err_new);
} 

int processor_is_err(processor_err_t err, processor_err_t is_set)
{
    return err & is_set;
}

const char * processor_strerr(processor_err_t onehot)
{
    switch(onehot) {
        case PROCESSOR_ERR_NONE:
            return "none";
        case PROCESSOR_ERR_READ_ERR:
            return "file read err";
        case PROCESSOR_ERR_PARSE_ERR:
            return "parse err";
        case PROCESSOR_ERR_END_OF_BUFFER:
            return "buffer end reached meeting no halt";
        case PROCESSOR_ERR_ALLOC_FAIL:
            return "buffer allocation fail";
        case PROCESSOR_ERR_CMD_STACK_ERR:
            return "stack err (see stack dump)";
        case PROCESSOR_ERR_CMD_UNKNOWN:
            return "unknown command";
        case PROCESSOR_ERR_METADATA:
            return "invalid metadata";
        case PROCESSOR_ERR_CMDBUF_NULL:
            return "command buffer is NULL";
        case PROCESSOR_ERR_REGFILE_NULL:
            return "register file buffer is NULL";
        case PROCESSOR_ERR_REG_UNKNOWN:
            return "unknown register";
        case PROCESSOR_ERR_ZERO_DIV:
            return "division by zero";
        case PROCESSOR_ERR_DOMAIN_ERR:
            return "func domain error";
        case PROCESSOR_ERR_INVALID_PC:
            return "invalid address";
        default:
            return "unknown";
    }
}

processor_err_t _processor_verify_metadata(processor_t* proc)
{
    if(proc->cmdbuf[0] != SIGNATURE)
        return PROCESSOR_ERR_METADATA;
    if(proc->cmdbuf[1] != BYTECODE_VERSION)
        return PROCESSOR_ERR_METADATA;

    return PROCESSOR_ERR_NONE;
}

#ifdef _DEBUG

processor_err_t processor_vldtr(processor_t* proc)
{
    processor_err_t err = PROCESSOR_ERR_NONE;
    if(proc->cmdbuf == NULL)
        processor_set_err(&err, PROCESSOR_ERR_CMDBUF_NULL);

    if(proc->regfile == NULL)
        processor_set_err(&err, PROCESSOR_ERR_REGFILE_NULL);

    return err;
}

void processor_dump(FILE* stream, processor_t* proc, processor_err_t err, const char* msg, const char* file, const char* func, int line)
{
    utils_colored_fprintf(stream, ANSI_COLOR_BOLD_RED, "========== processor dump ==========\n\n");
    utils_colored_fprintf(stream, ANSI_COLOR_BOLD_RED, "== error ==\n");
    utils_colored_fprintf(stream, ANSI_COLOR_BLUE, "    from: %s:%d %s()\n", file, line, func);
    utils_colored_fprintf(stream, ANSI_COLOR_RED, "    err: %s\n    what: %s\n", processor_strerr(err), msg);

    fprintf(stream, "\n");

    BEGIN {
        utils_colored_fprintf(stream, ANSI_COLOR_BOLD_RED, "== regfile[%p] == \n", proc->regfile);

        if(processor_is_err(err, PROCESSOR_ERR_REGFILE_NULL)) GOTO_END;

        for(size_t regi = 0; regi < SIZEOF(proc_regs); ++regi) {
            utils_colored_fprintf(stream, ANSI_COLOR_RED, "%s: ", proc_regs[regi].name);
            utils_colored_fprintf(stream, ANSI_COLOR_BLUE, "%08x\n", (unsigned) proc->regfile[regi]);
        }

        fprintf(stream, "\n");

        utils_colored_fprintf(stream, ANSI_COLOR_BOLD_RED, "== cmdbuf[%p] == \n", proc->cmdbuf);
        
        if(processor_is_err(err, PROCESSOR_ERR_CMDBUF_NULL)) GOTO_END;

        for(unsigned int bufi = 0; bufi < proc->cmdbuf_size; ++bufi) {
            if(bufi % PROCESSOR_DUMP_BYTES_PER_LINE == 0)
                fprintf(stream, "[0x%08x] ", bufi);

            if(bufi < METAINFO_LENGTH)
                utils_colored_fprintf(stream, ANSI_COLOR_GREEN, "%08x ", (unsigned) proc->cmdbuf[bufi]);
            else if(bufi == proc->pc)
                utils_colored_fprintf(stream, ANSI_COLOR_RED, "%08x ", (unsigned) proc->cmdbuf[bufi]);
            else 
                utils_colored_fprintf(stream, ANSI_COLOR_BLUE, "%08x ", (unsigned) proc->cmdbuf[bufi]);

            if((bufi + 1) % PROCESSOR_DUMP_BYTES_PER_LINE == 0)
                fprintf(stream, "\n");
        }
    } END;

    fprintf(stream, "\n");

    fprintf(stream, "\n");
}

#endif // _DEBUG

#define CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err)                \
    if(stk_err != STACK_ERR_NONE) {                                            \
        processor_set_err(&err, PROCESSOR_ERR_CMD_STACK_ERR);                  \
        processor_dump(stderr, proc, err, "", __FILE__, __func__, __LINE__);   \
        return { CMD_CALLBACK_ERR, err };                                      \
    }

#define CALLBACK_VERIFY_OK_OR_RETURN_ERR(proc, expr, err, err_set)             \
    if(!(expr)) {                                                              \
        processor_set_err(&err, err_set);                                      \
        processor_dump(stderr, proc, err, "", __FILE__, __func__, __LINE__);   \
        return { CMD_CALLBACK_ERR, err };                                      \
    }

cmd_callback_err_t cmd_push(processor_t* proc, command_data_t a, ATTR_UNUSED command_data_t b)
{
    processor_err_t err = PROCESSOR_ERR_NONE;
    stack_err_t stk_err = stack_push(&proc->stack, a);

    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    return { CMD_CALLBACK_CONTINUE, err };
}

cmd_callback_err_t cmd_pushr(processor_t* proc, command_data_t a, ATTR_UNUSED command_data_t b)
{
    processor_err_t err = PROCESSOR_ERR_NONE;

    CALLBACK_VERIFY_OK_OR_RETURN_ERR(
        proc,
        (unsigned) a < SIZEOF(proc_regs), 
        err, 
        PROCESSOR_ERR_REG_UNKNOWN
    );

    command_data_t regdata = proc->regfile[a];

    stack_err_t stk_err = stack_push(&proc->stack, regdata);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    return { CMD_CALLBACK_CONTINUE, err };
}

cmd_callback_err_t cmd_popr(processor_t* proc, command_data_t a, ATTR_UNUSED command_data_t b)
{
    processor_err_t err = PROCESSOR_ERR_NONE;

    CALLBACK_VERIFY_OK_OR_RETURN_ERR(
        proc, 
        (unsigned) a < SIZEOF(proc_regs), 
        err, 
        PROCESSOR_ERR_REG_UNKNOWN
    );
    
    stack_data_t stkdata = 0;
    stack_err_t stk_err = stack_pop(&proc->stack, &stkdata);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    proc->regfile[a] = stkdata;

    return { CMD_CALLBACK_CONTINUE, err };
}

#define PROCESSOR_GENERATE_CALLBACK_ARITHM_BINARY_(name, op)                                                     \
    cmd_callback_err_t cmd_##name(processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b) \
    {                                                                                                            \
        processor_err_t err = PROCESSOR_ERR_NONE;                                                                \
        stack_err_t stk_err = STACK_ERR_NONE;                                                                    \
                                                                                                                 \
        stack_data_t lhs = 0, rhs = 0;                                                                           \
                                                                                                                 \
        stk_err = stack_pop(&proc->stack, &rhs);                                                                 \
        CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;                                                \
                                                                                                                 \
        stk_err = stack_pop(&proc->stack, &lhs);                                                                 \
        CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;                                                \
                                                                                                                 \
        stk_err = stack_push(&proc->stack, lhs op rhs);                                                          \
        CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;                                                \
                                                                                                                 \
        return { CMD_CALLBACK_CONTINUE, err };                                                                   \
    }                                                                                                            \

PROCESSOR_GENERATE_CALLBACK_ARITHM_BINARY_(add, +);
PROCESSOR_GENERATE_CALLBACK_ARITHM_BINARY_(sub, -);
PROCESSOR_GENERATE_CALLBACK_ARITHM_BINARY_(mul, *);

cmd_callback_err_t cmd_div(processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    processor_err_t err = PROCESSOR_ERR_NONE;
    stack_err_t stk_err = STACK_ERR_NONE;

    stack_data_t lhs = 0, rhs = 0;

    stk_err = stack_pop(&proc->stack, &rhs);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    stk_err = stack_pop(&proc->stack, &lhs);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    CALLBACK_VERIFY_OK_OR_RETURN_ERR(proc, rhs != 0, err, PROCESSOR_ERR_ZERO_DIV);

    stk_err = stack_push(&proc->stack, lhs / rhs);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    return { CMD_CALLBACK_CONTINUE, err };
}

cmd_callback_err_t cmd_sqr(processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    processor_err_t err = PROCESSOR_ERR_NONE;
    stack_err_t stk_err = STACK_ERR_NONE;

    stack_data_t val = 0;
    stk_err = stack_pop(&proc->stack, &val);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    CALLBACK_VERIFY_OK_OR_RETURN_ERR(proc, val > 0, err, PROCESSOR_ERR_DOMAIN_ERR);

    stk_err = stack_push(&proc->stack, (stack_data_t) sqrtf((float) val));
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    return { CMD_CALLBACK_CONTINUE, err };
}

cmd_callback_err_t cmd_hlt(ATTR_UNUSED processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    return { CMD_CALLBACK_HALT, PROCESSOR_ERR_NONE };
}

cmd_callback_err_t cmd_out(processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    processor_err_t err = PROCESSOR_ERR_NONE;
    stack_err_t stk_err = STACK_ERR_NONE;

    stack_data_t val = 0;
    stk_err = stack_pop(&proc->stack, &val);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    printf("%d\n", val);

    stk_err = stack_push(&proc->stack, val);
    CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;

    return { CMD_CALLBACK_CONTINUE, err };
}

cmd_callback_err_t cmd_jmp(processor_t* proc, ATTR_UNUSED command_data_t a, ATTR_UNUSED command_data_t b)
{
    processor_err_t err = PROCESSOR_ERR_NONE;

    CALLBACK_VERIFY_OK_OR_RETURN_ERR(                                                               
        proc,                                                                                       
        (size_t) a < proc->cmdbuf_size,                                                             
        err,                                                                                        
        PROCESSOR_ERR_INVALID_PC                                                                    
    );                                                                                              
                                                                                                    
    CALLBACK_VERIFY_OK_OR_RETURN_ERR(                                                               
        proc,                                                                                       
        (size_t) a < proc->cmdbuf_size,                                                             
        err,                                                                                        
        PROCESSOR_ERR_INVALID_PC                                                                    
    );                                                                                              

    proc->pc = (size_t) a;

    return { CMD_CALLBACK_CONTINUE, err };
}

#define PROCESSOR_GENERATE_CALLBACK_J_(name, sign)                                                      \
    cmd_callback_err_t cmd_j##name(processor_t* proc, command_data_t a, ATTR_UNUSED command_data_t b)   \
    {                                                                                                   \
        processor_err_t err = PROCESSOR_ERR_NONE;                                                       \
        stack_err_t stk_err = STACK_ERR_NONE;                                                           \
                                                                                                        \
        CALLBACK_VERIFY_OK_OR_RETURN_ERR(                                                               \
            proc,                                                                                       \
            (size_t) a < proc->cmdbuf_size,                                                             \
            err,                                                                                        \
            PROCESSOR_ERR_INVALID_PC                                                                    \
        );                                                                                              \
                                                                                                        \
        CALLBACK_VERIFY_OK_OR_RETURN_ERR(                                                               \
            proc,                                                                                       \
            (size_t) a < proc->cmdbuf_size,                                                             \
            err,                                                                                        \
            PROCESSOR_ERR_INVALID_PC                                                                    \
        );                                                                                              \
                                                                                                        \
        stack_data_t lhs = 0, rhs = 0;                                                                  \
                                                                                                        \
        stk_err = stack_pop(&proc->stack, &rhs);                                                        \
        CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;                                       \
                                                                                                        \
        stk_err = stack_pop(&proc->stack, &lhs);                                                        \
        CALLBACK_VERIFY_STACK_OR_RETURN_ERR(proc, stk_err, err);;                                       \
                                                                                                        \
        if(lhs sign rhs)                                                                                \
            proc->pc = (size_t) a;                                                                      \
                                                                                                        \
        return { CMD_CALLBACK_CONTINUE, err };                                                          \
    }                                                 

PROCESSOR_GENERATE_CALLBACK_J_(b , < );
PROCESSOR_GENERATE_CALLBACK_J_(be, <=);
PROCESSOR_GENERATE_CALLBACK_J_(a , > );
PROCESSOR_GENERATE_CALLBACK_J_(ae, >=);
PROCESSOR_GENERATE_CALLBACK_J_(e , ==);
PROCESSOR_GENERATE_CALLBACK_J_(ne, !=);

