#include <cstdlib>
#include <cstring>
#include <stdlib.h>

#include "fileline_arr.h"
#include "assembler.h"
#include "memutils.h"
#include "optutils.h"
#include "logutils.h"
#include "utils.h"

#define LOG_CATEGORY_OPT    "CLI OPTIONS"
#define LOG_CATEGORY_FILEOP "FILE OPERATIONS"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "in"      , NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "out"     , NULL, 0, 0 },
    { OPT_ARG_NONE    , "listing" , NULL, 0, 0 },
};

int main(int argc, char* argv[])
{
    utils_init_log_stream(stderr);

    utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts));

    if(!long_opts[0].is_set) {
        UTILS_LOGE(LOG_CATEGORY_OPT, "specify input file");
        return EXIT_FAILURE;
    }

    if(!long_opts[1].is_set) {
        UTILS_LOGE(LOG_CATEGORY_OPT, "specify output file");
        return EXIT_FAILURE;
    }

    assembler_t asmblr {
        .cmdbuf = NULL,
        .cmdbuf_size = 0,
        .cmdbuf_ind = 0,
        .lblbuf = {
            .buf = NULL,
            .size = 0,
            .capacity = 0
        },
        .FILELINE_ARR_INITLIST(filearr)
    };

    FILE* input_file = open_file(long_opts[0].arg, "r");
    if(input_file == NULL) {
        UTILS_LOGE(LOG_CATEGORY_FILEOP, "could not open input file");
        return EXIT_FAILURE;
    }

    assembler_err_t asm_err = ASSEMBLER_ERR_NONE;

    asm_err = assembler_ctor(input_file, &asmblr);
    if(asm_err != ASSEMBLER_ERR_NONE) {
        assembler_dtor(&asmblr);
        fclose(input_file);
        return EXIT_FAILURE;
    }

    fclose(input_file);

    asm_err = assembler_assemble(&asmblr, long_opts[2].is_set);
    if(asm_err != ASSEMBLER_ERR_NONE) {
        assembler_dtor(&asmblr);
        return EXIT_FAILURE;
    }

    FILE* output_file = open_file(long_opts[1].arg, "w");
    if(output_file == NULL) {
        assembler_dtor(&asmblr);
        return EXIT_FAILURE;
    }

    assembler_write_to_file(output_file, &asmblr);

    fclose(output_file);

    assembler_dtor(&asmblr);

    utils_end_log();

    return EXIT_SUCCESS;
}
