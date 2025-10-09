#include "assembler.h"

#include <stdio.h>
#include <string.h>

#include "assertutils.h"
#include "commands.h"
#include "logutils.h"

static const size_t MAX_CMD_LENGTH  = sizeof(command_data_t) * MAX_CMD_ARG_CNT;
static const size_t METAINFO_LENGTH = 2;

static assembler_err_t _assembler_write_metainfo(assembler_t* asmblr);

static const command_t*  _assembler_match_cmd(const char* name);
static const proc_reg_t* _assembler_match_reg(char* regname);

static assembler_err_t _assembler_parse_cmd(const command_t** cmd, assembler_t* asmblr, char** str);
static assembler_err_t _assembler_parse_arg(const command_t* cmd, assembler_t* asmblr, char** str);


assembler_err_t assembler_assemble_file(fileline_arr_t* filearr, assembler_t* asmblr)
{
    utils_assert(filearr);
    utils_assert(asmblr);

    fileline_t* line = NULL;

    size_t cmdbuf_tmp_size = filearr->lcnt * (MAX_CMD_ARG_CNT + 1) + METAINFO_LENGTH;
    command_data_t* cmdbuf_tmp = (command_data_t*)calloc(cmdbuf_tmp_size, sizeof(asmblr->cmdbuf[0]));
    if(cmdbuf_tmp == NULL) {
        utils_log(LOG_LEVEL_ERR, "failed to allocate command buffer");
        return ASSEMBLER_ERR_ALLOC_FAIL;
    }

    asmblr->cmdbuf      = cmdbuf_tmp;
    asmblr->cmdbuf_size = cmdbuf_tmp_size;
    asmblr->cmdbuf_ptr  = cmdbuf_tmp;

    const command_t* cmd     = NULL;
    char*            str_ptr = NULL;

    assembler_err_t err = ASSEMBLER_ERR_NONE;

    err = _assembler_write_metainfo(asmblr);
    if(err != ASSEMBLER_ERR_NONE)
        return err;

    for(size_t line_i = 0; line_i < filearr->lcnt; ++line_i) {
        line = fileline_arr_get(filearr, line_i);

        if(line == NULL)
            return ASSEMBLER_ERR_PARSE_FAIL;

        str_ptr = line->str;
        err = _assembler_parse_cmd(&cmd, asmblr, &str_ptr);
        if(err != ASSEMBLER_ERR_NONE)
            return err;

        err = _assembler_parse_arg(cmd, asmblr, &str_ptr);
        if(err != ASSEMBLER_ERR_NONE)
            return err;
    }
    
    return ASSEMBLER_ERR_NONE;
}
assembler_err_t assembler_write_to_file(FILE* file, assembler_t* asmblr)
{
    utils_assert(file);
    utils_assert(asmblr);
    
    size_t bytes_wr = fwrite(asmblr->cmdbuf, sizeof(asmblr->cmdbuf[0]), asmblr->cmdbuf_size, file);
    if(bytes_wr < asmblr->cmdbuf_size)
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
        case ASSEMBLER_ERR_CMD_UNKNOWN:
            return "unknown command occured";
        case ASSEMBLER_ERR_WRITE:
            return "write error";
        default:
            return "unknown";
    }
}

static const command_t* _assembler_match_cmd(const char* name)
{
    utils_assert(name);
    utils_assert(cmdarr);

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
    *(asmblr->cmdbuf_ptr++) = SIGNATURE;
    *(asmblr->cmdbuf_ptr++) = BYTECODE_VERSION;

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_parse_cmd(const command_t** cmd, assembler_t* asmblr, char** str)
{
    utils_assert(asmblr);
    utils_assert(str);

    static char cmdstr[MAX_CMD_LENGTH] = "";

    int bytes_rd = 0;
    if(sscanf(*str, "%s%n", cmdstr, &bytes_rd) != 1) {
        utils_log(LOG_LEVEL_ERR, "sscanf() failed");
        return ASSEMBLER_ERR_PARSE_FAIL;
    }

    const command_t* cmd_tmp = _assembler_match_cmd(cmdstr);
    
    if(cmd == NULL) {
        utils_log(LOG_LEVEL_ERR, "unknown command %s", cmdstr);
        return ASSEMBLER_ERR_CMD_UNKNOWN;
    }

    *(asmblr->cmdbuf_ptr++) = cmd_tmp->code;

    *cmd =  cmd_tmp;
    *str += bytes_rd;

    return ASSEMBLER_ERR_NONE;
}

static assembler_err_t _assembler_parse_arg(const command_t* cmd, assembler_t* asmblr, char** str)
{
    utils_assert(cmd);
    utils_assert(str);
    utils_assert(asmblr);

    command_data_t cmdarg = 0;

    static char cmdstr[MAX_REG_NAME_LEN + 1] = "";
    
    // TODO add register name check
    int bytes_rd = 0;
    for(size_t arg_i = 0; arg_i < cmd->arg_cnt; ++arg_i) {
        if(cmd->cmd_type == COMMAND_TYPE_REGISTER && arg_i == 0) {
            if(sscanf(*str, "%s%n", cmdstr, &bytes_rd) != 1) {
                utils_log(LOG_LEVEL_ERR, "register name read err");
                return ASSEMBLER_ERR_PARSE_FAIL;
            }
            cmdarg = _assembler_match_reg(cmdstr)->num;
        }
        else {
            if(sscanf(*str, "%d%n", &cmdarg, &bytes_rd) != 1) {
                utils_log(LOG_LEVEL_ERR, "parsing failed");
                return ASSEMBLER_ERR_PARSE_FAIL;
            }
        }

        *(asmblr->cmdbuf_ptr) =  cmdarg;

        *str += bytes_rd;
    } 

    return ASSEMBLER_ERR_NONE;
}


