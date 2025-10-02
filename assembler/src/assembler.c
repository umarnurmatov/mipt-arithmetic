#include "assembler.h"

#include <stdio.h>
#include <string.h>

#include "assertutils.h"
#include "utils.h"

static const size_t MAX_CMD_LENGTH  = 20;
static const size_t MAX_CMD_ARG_CNT = 3;

static const command_t* _assembler_match_cmd(const char* name, const command_t* cmdarr, size_t cmdcnt);

assembler_err_t assembler_assemble_file(fileline_arr_t* filearr, const command_t* cmdarr, size_t cmdarr_size, bytecode_t** cmdbuf, size_t* cmdbuf_size)
{
    utils_assert(filearr);
    utils_assert(cmdarr);
    utils_assert(cmdbuf_size);

    char cmdstr[MAX_CMD_LENGTH] = "";
    
    fileline_t* line = NULL;

    size_t cmdbuf_tmp_size = filearr->lcnt * (MAX_CMD_ARG_CNT + 1);
    bytecode_t* cmdbuf_tmp = (bytecode_t*)calloc(cmdbuf_tmp_size, sizeof(cmdbuf[0]));
    if(cmdbuf_tmp == NULL)
        return ASSEMBLER_ERR_ALLOC_FAIL;
    
    size_t cmdbuf_i = 0;
    int bytes_rd = 0;
    for(size_t line_i = 0; line_i < filearr->lcnt; ++line_i) {
        line = fileline_arr_get(filearr, line_i);

        if(line == NULL)
            return ASSEMBLER_ERR_PARSE_FAIL;
        if(sscanf(line->str, "%s%n", cmdstr, &bytes_rd) != 2)
            return ASSEMBLER_ERR_PARSE_FAIL;

        const command_t* cmd = 
            _assembler_match_cmd(cmdstr, cmdarr, cmdarr_size);
        
        if(cmd == NULL)
            return ASSEMBLER_ERR_CMD_UNKNOWN;

        utils_assert(cmd->arg_cnt > MAX_CMD_ARG_CNT);

        cmdbuf_tmp[cmdbuf_i++] = cmd->code;
        
        bytecode_t cmdarg = 0;
        char* cmdstr_ptr = cmdstr + bytes_rd;
        for(size_t arg_i = 0; arg_i < cmd->arg_cnt; ++arg_i) {
            if(sscanf(cmdstr_ptr, "%d%n", &cmdarg, &bytes_rd) != 1)
                return ASSEMBLER_ERR_PARSE_FAIL;
            cmdbuf_tmp[cmdbuf_i++] = cmdarg;
            cmdstr_ptr += bytes_rd;
        } 
    }
    
    *cmdbuf_size = cmdbuf_tmp_size;
    *cmdbuf      = cmdbuf_tmp;

    return ASSEMBLER_ERR_NONE;
}
assembler_err_t assembler_write_to_file(FILE* file, bytecode_t* cmdbuf, size_t cmdbuf_size)
{
    utils_assert(file);
    utils_assert(cmdbuf);
    
    size_t bytes_wr = fwrite(cmdbuf, sizeof(cmdbuf[0]), cmdbuf_size, file);
    if(bytes_wr < cmdbuf_size)
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

static const command_t* _assembler_match_cmd(const char* name, const command_t* cmdarr, size_t cmdcnt)
{
    utils_assert(name);
    utils_assert(cmdarr);

    for(size_t cmdi = 0; cmdi < cmdcnt; ++cmdi)
        if(!strncmp(cmdarr[cmdi].name, name, MAX_CMD_LENGTH))
            return &cmdarr[cmdi];
    return NULL;
}
