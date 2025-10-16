#include "assembler.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "assertutils.h"
#include "colorutils.h"
#include "commands.h"
#include "fileline_arr.h"
#include "logutils.h"
#include "memutils.h"

#define ASSEMBLER_ASSERT_OK(asmblr)                         \
    utils_assert(asmblr);                                   \
    utils_assert(asmblr->cmdbuf);                           \
    utils_assert(asmblr->cmdbuf_ind < asmblr->cmdbuf_size); \
    utils_assert(asmblr->lblbuf);                           \

#define ASSEMBLER_VERIFY_OR_RETURN(expr, err)                   \
if(!(expr)) {                                                   \
    UTILS_LLOGE(LOG_CATEGORY_ASM, "%s", assembler_strerr(err)); \
    return err;                                                 \
}

// FIXME
#define ASSEMBLER_DUMP_SYNTAX_AND_RETURN(msg) \


static const size_t MAX_CMD_LENGTH  = sizeof(command_data_t) * MAX_CMD_ARG_CNT;
static const size_t METAINFO_LENGTH = 2;
static const size_t LBLBUF_INIT_SIZE = 10;
static const size_t LBLBUF_MAX_SIZE  = 100;
static const command_data_t LBLBUF_PLACEHOLDER = -1;

static assembler_err_t _assembler_write_metainfo(assembler_t* asmblr);

static const command_t*  _assembler_match_cmd(const char* name);
static const proc_reg_t* _assembler_match_reg(char* regname);

static assembler_expr_t _assembler_get_expr_type(assembler_t* asmblr);

static assembler_err_t _assembler_parse_cmd(const command_t** cmd, assembler_t* asmblr);
static assembler_err_t _assembler_parse_cmd_arg(const command_t* cmd, assembler_t* asmblr);
static assembler_err_t _assembler_parse_lbl_arg(assembler_t* asmblr);

static assembler_err_t _assembler_realloc_lblbuf(assembler_t* asmblr, size_t new_size);

static assembler_err_t _assembler_assemble_once(assembler_t* asmblr, int dump_listing);

static void _assembler_dump_line(assembler_t* asmblr, size_t bytes_assembled);

assembler_err_t assembler_ctor(FILE* file, assembler_t* asmblr)
{
    utils_assert(file);
    utils_assert(asmblr);

    fileline_arr_read(&asmblr->filearr, file);

    size_t cmdbuf_tmp_size = asmblr->filearr.lcnt * (MAX_CMD_ARG_CNT + 1) + METAINFO_LENGTH;
    command_data_t* cmdbuf_tmp = (command_data_t*)calloc(cmdbuf_tmp_size, sizeof(asmblr->cmdbuf[0]));

    ASSEMBLER_VERIFY_OR_RETURN(cmdbuf_tmp, ASSEMBLER_ERR_ALLOC_FAIL);

    asmblr->cmdbuf      = cmdbuf_tmp;
    asmblr->cmdbuf_size = cmdbuf_tmp_size;
    asmblr->cmdbuf_ind  = 0;
    
    ASSEMBLER_VERIFY_OR_RETURN(
        _assembler_realloc_lblbuf(asmblr, LBLBUF_INIT_SIZE) == ASSEMBLER_ERR_NONE, 
        ASSEMBLER_ERR_ALLOC_FAIL
    );

    asmblr->line_ptr = NULL;
    asmblr->str_ind  = 0;

    return ASSEMBLER_ERR_NONE;
}

assembler_err_t assembler_assemble(assembler_t* asmblr, int dump_listing)
{
    assembler_err_t err = ASSEMBLER_ERR_NONE;

    err = _assembler_assemble_once(asmblr, 0);
    if(err != ASSEMBLER_ERR_NONE)
        return err;

    asmblr->cmdbuf_ind = 0;

    err = _assembler_assemble_once(asmblr, dump_listing);
    if(err != ASSEMBLER_ERR_NONE)
        return err;

    return err;
}

assembler_err_t assembler_write_to_file(FILE* file, assembler_t* asmblr)
{
    utils_assert(file);
    utils_assert(asmblr);

    size_t bytecode_s = asmblr->cmdbuf_ind;
    size_t bytes_wr = fwrite(asmblr->cmdbuf, sizeof(asmblr->cmdbuf[0]), bytecode_s, file);
    if(bytes_wr < bytecode_s)
        return ASSEMBLER_ERR_WRITE;

    return ASSEMBLER_ERR_NONE;
}

const char* assembler_strerr(assembler_err_t err)
{
    switch(err) {
        case ASSEMBLER_ERR_NONE:
            return "none";
        case ASSEMBLER_ERR_PARSE_FAIL:
            return "file parsing failed";
        case ASSEMBLER_ERR_ALLOC_FAIL:
            return "buffer allocation failed";
        case ASSEMBLER_ERR_WRITE:
            return "write error";
        case ASSEMBLER_ERR_SYNTAX:
            return "syntax error";
        default:
            return "unknown";
    }
}

void assembler_dtor(assembler_t* asmblr)
{
    utils_assert(asmblr);

    fileline_arr_free(&asmblr->filearr);
    NFREE(asmblr->cmdbuf);
    NFREE(asmblr->lblbuf);
}

void assembler_dump_syntax_err(assembler_t* asmblr, const char* msg)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(asmblr->line_ptr);

    utils_colored_fprintf(stderr, ANSI_COLOR_RED, "line %lu: %s [%s]\n", asmblr->line_ptr->lnum + 1, asmblr->line_ptr->str, msg);
}

static const command_t* _assembler_match_cmd(const char* name)
{
    utils_assert(name);

    for(size_t cmdi = 0; cmdi < SIZEOF(cmdarr); ++cmdi)
        if(!strncmp(cmdarr[cmdi].name, name, MAX_CMD_LENGTH))
            return &cmdarr[cmdi];
    return NULL;
}

static const proc_reg_t* _assembler_match_reg(char* regname)
{
    utils_assert(regname);

    for(size_t regi = 0; regi < SIZEOF(proc_regs); ++regi) {
        if(!strcmp(proc_regs[regi].name, regname))
            return &proc_regs[regi];
    }
    return NULL;
}

static assembler_err_t _assembler_write_metainfo(assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);

    asmblr->cmdbuf[asmblr->cmdbuf_ind++] = SIGNATURE;
    asmblr->cmdbuf[asmblr->cmdbuf_ind++] = BYTECODE_VERSION;

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_parse_cmd(const command_t** cmd, assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(asmblr->str_ind < asmblr->line_ptr->len);

    static char cmdstr[MAX_CMD_LENGTH] = "";

    int bytes_rd = 0;
    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind];
    if(sscanf(str_ptr, "%s%n", cmdstr, &bytes_rd) != 1) {
        assembler_dump_syntax_err(asmblr, "expected command");
        return ASSEMBLER_ERR_SYNTAX;
    }

    const command_t* cmd_tmp = _assembler_match_cmd(cmdstr);
    
    if(cmd_tmp == NULL) {
        assembler_dump_syntax_err(asmblr, "unknown command");
        return ASSEMBLER_ERR_SYNTAX;
    }

    asmblr->cmdbuf[asmblr->cmdbuf_ind++] = cmd_tmp->code;

    *cmd = cmd_tmp;

    asmblr->str_ind += (size_t) bytes_rd;

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_parse_cmd_arg(const command_t* cmd, assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(asmblr->str_ind < asmblr->line_ptr->len);

    command_data_t cmdarg = 0;

    static char cmdstr[MAX_REG_NAME_LEN + 1] = "";
    
    int bytes_rd = 0;
    for(size_t arg_i = 0; arg_i < cmd->arg_cnt; ++arg_i) {

        char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind];

        if(cmd->cmd_type == COMMAND_TYPE_REGISTER && arg_i == 0) {
            if(sscanf(str_ptr, "%s%n", cmdstr, &bytes_rd) != 1) {
                assembler_dump_syntax_err(asmblr, "expected register name");
                return ASSEMBLER_ERR_SYNTAX;
            }

            const proc_reg_t* reg = _assembler_match_reg(cmdstr);
            if(!reg) {
                assembler_dump_syntax_err(asmblr, "unknown register name");
                return ASSEMBLER_ERR_SYNTAX;
            }
                
            cmdarg = reg->num;
        }

        else if(cmd->cmd_type == COMMAND_TYPE_JUMP || cmd->cmd_type == COMMAND_TYPE_CALL) {

            char* lbl_start_ch = strchr(str_ptr, ':');

            if(lbl_start_ch) { 

                command_data_t lblcode = 0;

                if(sscanf(++lbl_start_ch, "%d%n", &lblcode, &bytes_rd) != 1) {
                    assembler_dump_syntax_err(asmblr, "expected label as command argument");
                    return ASSEMBLER_ERR_SYNTAX;
                }
                if((unsigned) lblcode >= LBLBUF_MAX_SIZE) {
                    assembler_dump_syntax_err(asmblr, "label out of bound");
                    return ASSEMBLER_ERR_SYNTAX;
                }

                if((unsigned) lblcode < asmblr->lblbuf_size 
                        && asmblr->lblbuf[lblcode] != LBLBUF_PLACEHOLDER)
                    cmdarg = asmblr->lblbuf[lblcode];
            }
            else {
                if(sscanf(str_ptr, "%d%n", &cmdarg, &bytes_rd) != 1) {
                    assembler_dump_syntax_err(asmblr, "");
                    return ASSEMBLER_ERR_SYNTAX;
                }
            }
        }

        else if(cmd->cmd_type == COMMAND_TYPE_RAM) {
            char* addr_start_ch = strchr(str_ptr, '[');

            if(addr_start_ch) { 

                if(sscanf(++addr_start_ch, "%[^]]%n", cmdstr, &bytes_rd) != 1) {
                    assembler_dump_syntax_err(asmblr, "expected register name");
                    return ASSEMBLER_ERR_SYNTAX;
                }

                const proc_reg_t* reg = _assembler_match_reg(cmdstr);
                if(!reg) {
                    assembler_dump_syntax_err(asmblr, "unknown register name");
                    return ASSEMBLER_ERR_SYNTAX;
                }
                    
                cmdarg = reg->num;
            }
            else {
                assembler_dump_syntax_err(asmblr, "expected [<reg>]");
                return ASSEMBLER_ERR_SYNTAX;
            }
        }

        else {
            if(sscanf(str_ptr, "%d%n", &cmdarg, &bytes_rd) != 1) {
                assembler_dump_syntax_err(asmblr, "expected argument");
                return ASSEMBLER_ERR_SYNTAX;
            }
        }

        asmblr->cmdbuf[asmblr->cmdbuf_ind++] = cmdarg;

        asmblr->str_ind += (size_t) bytes_rd;
    } 

    return ASSEMBLER_ERR_NONE;
}

static assembler_expr_t _assembler_get_expr_type(assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(asmblr->str_ind < asmblr->line_ptr->len);

    return asmblr->line_ptr->str[0] == ':' 
            ? ASSEMBLER_EXPR_LBL
            : ASSEMBLER_EXPR_CMD;
}

static assembler_err_t _assembler_parse_lbl_arg(assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(asmblr->str_ind < asmblr->line_ptr->len);

    int lblcode = 0;
    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind + 1]; 

    if(sscanf(str_ptr, "%d", &lblcode) != 1) {
        assembler_dump_syntax_err(asmblr, "expected label");
        return ASSEMBLER_ERR_SYNTAX;
    }
    
    if((unsigned) lblcode >= LBLBUF_MAX_SIZE) {
        assembler_dump_syntax_err(asmblr, "label out of bound");
        return ASSEMBLER_ERR_SYNTAX;
    }
    
    if((unsigned) lblcode > asmblr->lblbuf_size) {
        while(asmblr->lblbuf_size < (unsigned) lblcode) 
            ASSEMBLER_VERIFY_OR_RETURN(
                _assembler_realloc_lblbuf(asmblr, asmblr->lblbuf_size * 2) == ASSEMBLER_ERR_NONE,
                ASSEMBLER_ERR_ALLOC_FAIL
            ); 
    }

    asmblr->lblbuf[lblcode] = (command_data_t) (asmblr->cmdbuf_ind);
    return ASSEMBLER_ERR_NONE;

static assembler_lbl_t* _assembler_find_lbl(assembler_t* asmblr, char* lblstr)
{
    for(size_t bufi = 0; bufi < asmblr->lblbuf.size; ++bufi) 
        if(strncmp(lblstr, asmblr->lblbuf.buf[bufi].lblstr, LBL_MAX_LENGTH) == 0)
                return &asmblr->lblbuf.buf[bufi];
}

{
    utils_assert(asmblr); 

    if(!asmblr->lblbuf.buf) {
        asmblr->lblbuf.capacity = 0;
        asmblr->lblbuf.size     = 0;

    assembler_lbl_t* lblbuf_tmp = 

    if(lblbuf_tmp == NULL)
        return ASSEMBLER_ERR_ALLOC_FAIL;

    asmblr->lblbuf.buf      = lblbuf_tmp;

    return ASSEMBLER_ERR_NONE;
}

static void _assembler_free_lblbuf(assembler_t* asmblr)
{
    for(size_t bufi = 0; bufi < asmblr->lblbuf.size; ++bufi)
        free(asmblr->lblbuf.buf[bufi].lblstr);
    NFREE(asmblr->lblbuf.buf);
}
static assembler_err_t _assembler_assemble_once(assembler_t* asmblr, int dump_listing)
{
    ASSEMBLER_ASSERT_OK(asmblr)

    assembler_err_t err = ASSEMBLER_ERR_NONE;

    err = _assembler_write_metainfo(asmblr);
    ASSEMBLER_VERIFY_OR_RETURN(err == ASSEMBLER_ERR_NONE, err); 

    const command_t* cmd = NULL;
    for(size_t line_i = 0; line_i < asmblr->filearr.lcnt; ++line_i) {
        asmblr->line_ptr = fileline_arr_get(&asmblr->filearr, line_i);

        ASSEMBLER_VERIFY_OR_RETURN(asmblr->line_ptr, ASSEMBLER_ERR_PARSE_FAIL); 

        asmblr->str_ind = 0;

        assembler_expr_t expr_type = _assembler_get_expr_type(asmblr);

        size_t cmdbuf_ind_prev = asmblr->cmdbuf_ind;

        switch(expr_type) {
            case ASSEMBLER_EXPR_LBL:

                err = _assembler_parse_lbl_arg(asmblr);
                ASSEMBLER_VERIFY_OR_RETURN(err == ASSEMBLER_ERR_NONE, err); 

                if(dump_listing)
                    _assembler_dump_line(asmblr, 0);

                break;
            case ASSEMBLER_EXPR_CMD:

                err = _assembler_parse_cmd(&cmd, asmblr);
                ASSEMBLER_VERIFY_OR_RETURN(err == ASSEMBLER_ERR_NONE, err); 
                
                if(cmd->arg_cnt != 0) {
                    err = _assembler_parse_cmd_arg(cmd, asmblr);
                    ASSEMBLER_VERIFY_OR_RETURN(err == ASSEMBLER_ERR_NONE, err); 
                }

                if(dump_listing)
                    _assembler_dump_line(asmblr, asmblr->cmdbuf_ind - cmdbuf_ind_prev);

                break;
            default:
                break;
        }

    }
    
    return ASSEMBLER_ERR_NONE;
}

static void _assembler_dump_line(assembler_t* asmblr, size_t bytes_assembled)
{
    ASSEMBLER_ASSERT_OK(asmblr);

    static char hexbuf[(sizeof(command_data_t) * 8 + 1) * (MAX_CMD_ARG_CNT + 1)];
    char* hexbuf_ptr = hexbuf;
    
    if(bytes_assembled > 0)
        utils_colored_fprintf(stdout, ANSI_COLOR_BOLD_WHITE, "[%08zx] ", asmblr->cmdbuf_ind - bytes_assembled);
    else
        utils_colored_fprintf(stdout, ANSI_COLOR_BOLD_WHITE, "[xxxxxxxx] ");

    if(bytes_assembled > 0) {
        for(size_t bufi = asmblr->cmdbuf_ind - bytes_assembled; bufi < asmblr->cmdbuf_ind; ++bufi)
            hexbuf_ptr += sprintf(hexbuf_ptr, "%08x ", (unsigned) asmblr->cmdbuf[bufi]);

        utils_colored_fprintf(stdout, ANSI_COLOR_BLUE, "%20s", hexbuf);
    }
    else {
        utils_colored_fprintf(stdout, ANSI_COLOR_BLUE, "  [label]           ");
    }


    utils_colored_fprintf(stdout, ANSI_COLOR_YELLOW, "%s\n", asmblr->line_ptr->str);
}
