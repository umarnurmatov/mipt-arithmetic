#include "assembler.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "assertutils.h"
#include "colorutils.h"
#include "commands.h"
#include "fileline_arr.h"
#include "hashutils.h"
#include "logutils.h"
#include "memutils.h"
#include "stack.h"

#define ASSEMBLER_ASSERT_OK(asmblr)                         \
    utils_assert(asmblr);                                   \
    utils_assert(asmblr->cmdbuf);                           \
    utils_assert(asmblr->cmdbuf_ind < asmblr->cmdbuf_size); \
    utils_assert(asmblr->lblbuf.buf);                       \

#define ASSEMBLER_VERIFY_OR_RETURN(expr, err)                   \
if(!(expr)) {                                                   \
    UTILS_LLOGE(LOG_CATEGORY_ASM, "%s", assembler_strerr(err)); \
    return err;                                                 \
}

static const size_t METAINFO_LENGTH = 2;
static const size_t LBL_MAX_LENGTH = 20;
static const size_t LBLBUF_INIT_SIZE = 10;

static assembler_err_t _assembler_write_metainfo(assembler_t* asmblr);

static assembler_err_t _assembler_alloc_cmd_tbl(assembler_t* asmblr);
static assembler_err_t _assembler_alloc_reg_tbl(assembler_t* asmblr);

static const command_t* _assembler_match_cmd(assembler_t* asmblr, const char* name);
static const proc_reg_t* _assembler_match_reg(assembler_t* asmblr, const char* name);

static assembler_expr_t _assembler_get_expr_type(assembler_t* asmblr);

static assembler_err_t _assembler_parse_cmd(const command_t** cmd, assembler_t* asmblr);
static assembler_err_t _assembler_parse_cmd_arg(const command_t* cmd, assembler_t* asmblr);
static assembler_err_t _assembler_parse_lbl_arg(assembler_t* asmblr);

static assembler_err_t _assembler_parse_cmd_reg_arg(const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr);
static assembler_err_t _assembler_parse_cmd_call_arg(const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr);
static assembler_err_t _assembler_parse_cmd_ram_arg(const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr);
static assembler_err_t _assembler_parse_cmd_othr_arg(ATTR_UNUSED const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr);

static assembler_err_t _assembler_add_lbl(assembler_t* asmblr, assembler_lbl_t* lbl);
static assembler_lbl_t* _assembler_find_lbl(assembler_t* asmblr, char* lblstr);

static assembler_err_t _assembler_realloc_lblbuf(assembler_t* asmblr, size_t ncapacity);
static void _assembler_free_lblbuf(assembler_t* asmblr);

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

    assembler_err_t err = ASSEMBLER_ERR_NONE;

#define ASSEMBLER_VERIFY_INIT_OR_RETURN(func, ...)                            \
    ASSEMBLER_VERIFY_OR_RETURN(                                               \
        (err = func(asmblr __VA_OPT__(,) __VA_ARGS__)) == ASSEMBLER_ERR_NONE, \
        err                                                                   \
    )

    ASSEMBLER_VERIFY_INIT_OR_RETURN(_assembler_realloc_lblbuf, LBLBUF_INIT_SIZE);
    
    ASSEMBLER_VERIFY_INIT_OR_RETURN(_assembler_alloc_cmd_tbl);

    ASSEMBLER_VERIFY_INIT_OR_RETURN(_assembler_alloc_reg_tbl);

#undef ASSEMBLER_VERIFY_INIT_OR_RETURN

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
        case ASSEMBLER_ERR_HASH_COLLISION:
            return "hash collision occured";
        default:
            return "unknown";
    }
}

void assembler_dtor(assembler_t* asmblr)
{
    utils_assert(asmblr);

    fileline_arr_free(&asmblr->filearr);
    _assembler_free_lblbuf(asmblr);

    NFREE(asmblr->cmdbuf);
    NFREE(asmblr->cmd_tbl);
    NFREE(asmblr->reg_tbl);
}

void assembler_dump_syntax_err(assembler_t* asmblr, const char* msg)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(asmblr->line_ptr);

    utils_colored_fprintf(stderr, ANSI_COLOR_RED, "line %lu: %s [%s]\n", asmblr->line_ptr->lnum + 1, asmblr->line_ptr->str, msg);
}

#define ASSEMBLER_ALLOC_TBL_(pref, arr)                                                     \
    static assembler_err_t _assembler_alloc_##pref##_tbl(assembler_t* asmblr)               \
    {                                                                                       \
        size_t tbl_size_tmp = SIZEOF(arr);                                                  \
        assembler_##pref##_tbl_t* tbl_tmp =                                                 \
            (assembler_##pref##_tbl_t*)calloc(tbl_size_tmp, sizeof(asmblr->pref##_tbl[0])); \
                                                                                            \
        ASSEMBLER_VERIFY_OR_RETURN(tbl_tmp, ASSEMBLER_ERR_ALLOC_FAIL);                      \
                                                                                            \
        for(ssize_t cmdi = 0; (unsigned) cmdi < SIZEOF(arr); ++cmdi) {                      \
            tbl_tmp[cmdi].ptr = &arr[cmdi];                                                 \
            tbl_tmp[cmdi].hash =                                                            \
                utils_djb2_hash(&arr[cmdi].name, SIZEOF(arr[cmdi].name));                   \
                                                                                            \
            if(cmdi < 1) continue;                                                          \
                                                                                            \
            ssize_t cmdj = cmdi - 1;                                                        \
            assembler_##pref##_tbl_t key = tbl_tmp[cmdi];                                   \
            while(key.hash < tbl_tmp[cmdj].hash) {                                          \
                tbl_tmp[cmdj + 1] = tbl_tmp[cmdj];                                          \
                                                                                            \
                --cmdj;                                                                     \
                if(cmdj < 0) break;                                                         \
            }                                                                               \
                                                                                            \
            if(cmdj >= 0 && tbl_tmp[cmdj].hash == key.hash) {                               \
                NFREE(tbl_tmp);                                                             \
                return ASSEMBLER_ERR_HASH_COLLISION;                                        \
            }                                                                               \
            tbl_tmp[cmdj + 1] = key;                                                        \
        }                                                                                   \
                                                                                            \
        asmblr->pref##_tbl      = tbl_tmp;                                                  \
        asmblr->pref##_tbl_size = tbl_size_tmp;                                             \
                                                                                            \
        return ASSEMBLER_ERR_NONE;                                                          \
    }                                                                                       \

ASSEMBLER_ALLOC_TBL_(cmd, cmdarr);
ASSEMBLER_ALLOC_TBL_(reg, proc_regs);

#undef ASSEMBLER_ALLOC_TBL_ 

#define ASSEMBLER_MATCH_(pref, type, arr)                                                   \
    static const type* _assembler_match_##pref(assembler_t* asmblr, const char* name)       \
    {                                                                                       \
        utils_assert(name);                                                                 \
                                                                                            \
        utils_hash_t hash = utils_djb2_hash(name, SIZEOF(arr[0].name));                     \
                                                                                            \
        ssize_t l = 0, m = 0;                                                               \
        ssize_t r = (ssize_t)asmblr->pref##_tbl_size - 1;                                   \
        while(l <= r) {                                                                     \
            m = l + (r - l) / 2;                                                            \
            size_t pivot_hash = asmblr->pref##_tbl[m].hash;                                 \
            if(pivot_hash < hash)                                                           \
                l = m + 1;                                                                  \
            else if(pivot_hash > hash)                                                      \
                r = m - 1;                                                                  \
            else                                                                            \
                return asmblr->pref##_tbl[m].ptr;                                           \
        }                                                                                   \
                                                                                            \
        return NULL;                                                                        \
    }                                                                                       \

ASSEMBLER_MATCH_(cmd, command_t, cmdarr);
ASSEMBLER_MATCH_(reg, proc_reg_t, proc_regs);

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

    char cmdstr[MAX_CMD_NAME_LEN] = "";

    int bytes_rd = 0;
    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind];
    if(sscanf(str_ptr, "%s%n", cmdstr, &bytes_rd) != 1) {
        assembler_dump_syntax_err(asmblr, "expected command");
        return ASSEMBLER_ERR_SYNTAX;
    }

    const command_t* cmd_tmp = _assembler_match_cmd(asmblr, cmdstr);
    
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
    int bytes_rd = 0;

#define SYNTAX_VERIFIED(func) \
    if(func(cmd, &cmdarg, &bytes_rd, asmblr) != ASSEMBLER_ERR_NONE) \
        return ASSEMBLER_ERR_SYNTAX;

    for(size_t arg_i = 0; arg_i < cmd->arg_cnt; ++arg_i) {

        switch(cmd->cmd_type) {
            case COMMAND_TYPE_REGISTER:
                SYNTAX_VERIFIED(_assembler_parse_cmd_reg_arg);
                break;
            case COMMAND_TYPE_CALL:
            case COMMAND_TYPE_JUMP:
                SYNTAX_VERIFIED(_assembler_parse_cmd_call_arg);
                break;
            case COMMAND_TYPE_RAM:
                SYNTAX_VERIFIED(_assembler_parse_cmd_ram_arg);
                break;
            case COMMAND_TYPE_ARITHMETIC_BINARY:
            case COMMAND_TYPE_ARITHMETIC_UNARY:
            case COMMAND_TYPE_CONTROL:
            case COMMAND_TYPE_STACK:
            case COMMAND_TYPE_RET:
                SYNTAX_VERIFIED(_assembler_parse_cmd_othr_arg);
                break;
            default:
                break;
        }

        asmblr->cmdbuf[asmblr->cmdbuf_ind++] = cmdarg;

        asmblr->str_ind += (size_t) bytes_rd;
    } 

#undef SYNTAX_VERIFIED

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

    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind + 1]; 

    char* str_buf = (char*)calloc(LBL_MAX_LENGTH, sizeof str_buf[0]);

    assembler_err_t err = ASSEMBLER_ERR_NONE;

    size_t stri = 0;
    for(; stri < LBL_MAX_LENGTH; ++stri) {
        if(isspace(str_ptr[stri]) || str_ptr[stri] == '\0') break;

        else if(!isgraph(str_ptr[stri])) {
            assembler_dump_syntax_err(asmblr, "lbl must be only of alphanumeric chars");
            return ASSEMBLER_ERR_SYNTAX;
        }

        str_buf[stri] = str_ptr[stri];
    }

    assembler_lbl_t lbl = {
        .lblstr = str_buf,
        .lblstr_len = stri,
        .addr = (command_data_t) asmblr->cmdbuf_ind,
    };

    if((err = _assembler_add_lbl(asmblr, &lbl)) != ASSEMBLER_ERR_NONE) {
        NFREE(str_buf);
        return err;
    }

    return ASSEMBLER_ERR_NONE;

}

static assembler_err_t _assembler_parse_cmd_reg_arg(const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(cmdarg);
    utils_assert(bytes_rd);

    utils_assert(cmd->cmd_type == COMMAND_TYPE_REGISTER);

    static char cmdstr[MAX_REG_NAME_LEN + 1] = "";
    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind];

    if(cmd->cmd_type == COMMAND_TYPE_REGISTER) {
        if(sscanf(str_ptr, "%s%n", cmdstr, bytes_rd) != 1) {
            assembler_dump_syntax_err(asmblr, "expected register name");
            return ASSEMBLER_ERR_SYNTAX;
        }

        const proc_reg_t* reg = _assembler_match_reg(asmblr, cmdstr);
        if(!reg) {
            assembler_dump_syntax_err(asmblr, "unknown register name");
            return ASSEMBLER_ERR_SYNTAX;
        }
            
        *cmdarg = reg->num;
    }

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_parse_cmd_call_arg(const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(cmdarg);
    utils_assert(bytes_rd);

    utils_assert(
        cmd->cmd_type == COMMAND_TYPE_JUMP || 
        cmd->cmd_type == COMMAND_TYPE_CALL
    );

    static char lblstr[LBL_MAX_LENGTH + 1] = "";

    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind];
    char* lbl_start_ch = strchr(str_ptr, ':');

    if(lbl_start_ch) { 
        if(sscanf(++lbl_start_ch, "%s%n", lblstr, bytes_rd) != 1) {
            assembler_dump_syntax_err(asmblr, "expected label as command argument");
            return ASSEMBLER_ERR_SYNTAX;
        }
        
        assembler_lbl_t* lbl = _assembler_find_lbl(asmblr, lblstr);

        if(lbl != NULL)
            *cmdarg = lbl->addr;
    }
    else {
        if(sscanf(str_ptr, "%d%n", cmdarg, bytes_rd) != 1) {
            assembler_dump_syntax_err(asmblr, "");
            return ASSEMBLER_ERR_SYNTAX;
        }
    }

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_parse_cmd_ram_arg(const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(cmdarg);
    utils_assert(bytes_rd);

    utils_assert(cmd->cmd_type == COMMAND_TYPE_RAM);

    static char cmdstr[MAX_REG_NAME_LEN + 1] = "";
    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind];

    char* addr_start_ch = strchr(str_ptr, '[');

    if(addr_start_ch) { 

        if(sscanf(++addr_start_ch, "%[^]]%n", cmdstr, bytes_rd) != 1) {
            assembler_dump_syntax_err(asmblr, "expected register name");
            return ASSEMBLER_ERR_SYNTAX;
        }

        const proc_reg_t* reg = _assembler_match_reg(asmblr, cmdstr);
        if(!reg) {
            assembler_dump_syntax_err(asmblr, "unknown register name");
            return ASSEMBLER_ERR_SYNTAX;
        }
            
        *cmdarg = reg->num;
    }
    else {
        assembler_dump_syntax_err(asmblr, "expected [<reg>]");
        return ASSEMBLER_ERR_SYNTAX;
    }

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_parse_cmd_othr_arg(ATTR_UNUSED const command_t* cmd, command_data_t* cmdarg, int* bytes_rd, assembler_t* asmblr)
{
    ASSEMBLER_ASSERT_OK(asmblr);
    utils_assert(cmdarg);
    utils_assert(bytes_rd);

    char* str_ptr = &asmblr->line_ptr->str[asmblr->str_ind];

    if(sscanf(str_ptr, "%d%n", cmdarg, bytes_rd) != 1) {
        assembler_dump_syntax_err(asmblr, "expected argument");
        return ASSEMBLER_ERR_SYNTAX;
    }

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_add_lbl(assembler_t* asmblr, assembler_lbl_t* lbl)
{
    if(asmblr->lblbuf.size > asmblr->lblbuf.capacity / 2) {
        ASSEMBLER_VERIFY_OR_RETURN(
            _assembler_realloc_lblbuf(asmblr, asmblr->lblbuf.capacity * 2) == ASSEMBLER_ERR_NONE,
            ASSEMBLER_ERR_ALLOC_FAIL
        );
    }

    asmblr->lblbuf.buf[asmblr->lblbuf.size++] = *lbl;

    return ASSEMBLER_ERR_NONE;
}

static assembler_lbl_t* _assembler_find_lbl(assembler_t* asmblr, char* lblstr)
{
    for(size_t bufi = 0; bufi < asmblr->lblbuf.size; ++bufi) 
        if(strncmp(lblstr, asmblr->lblbuf.buf[bufi].lblstr, LBL_MAX_LENGTH) == 0)
                return &asmblr->lblbuf.buf[bufi];
    return NULL;
}

static assembler_err_t _assembler_realloc_lblbuf(assembler_t* asmblr, size_t ncapacity)
{
    utils_assert(asmblr); 

    if(!asmblr->lblbuf.buf) {
        asmblr->lblbuf.capacity = 0;
        asmblr->lblbuf.size     = 0;
    }

    assembler_lbl_t* lblbuf_tmp = 
        (assembler_lbl_t*)realloc(asmblr->lblbuf.buf, ncapacity * sizeof(asmblr->lblbuf.buf[0]));

    if(lblbuf_tmp == NULL)
        return ASSEMBLER_ERR_ALLOC_FAIL;

    asmblr->lblbuf.buf      = lblbuf_tmp;
    asmblr->lblbuf.capacity = ncapacity;

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

        utils_colored_fprintf(stdout, ANSI_COLOR_GREEN, "%20s", hexbuf);
    }
    else {
        utils_colored_fprintf(stdout, ANSI_COLOR_GREEN, "  [label]           ");
    }


    utils_colored_fprintf(stdout, ANSI_COLOR_MAGENTA, "%s\n", asmblr->line_ptr->str);
}
