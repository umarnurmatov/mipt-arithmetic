#include <cstdlib>
#include <stdlib.h>

#include "fileline_arr.h"
#include "assembler.h"
#include "memutils.h"
#include "optutils.h"
#include "logutils.h"
#include "utils.h"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "in", NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "out" , NULL, 0, 0 },
};

int main(int argc, char* argv[])
{
    utils_init_log("log.txt", "log");

    utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts));

    if(!long_opts[0].is_set) {
        utils_colored_fprintf(stderr, ANSI_COLOR_RED, "[ERROR] [OPT] Specify input file\b");
        return EXIT_FAILURE;
    }

    if(!long_opts[1].is_set) {
        utils_colored_fprintf(stderr, ANSI_COLOR_RED, "[ERROR] [OPT] Specify output file\n");
        return EXIT_FAILURE;
    }

    FILE* input_file = open_file(long_opts[0].arg, "r");
    if(input_file == NULL)
        return EXIT_FAILURE;

    FILELINE_ARR_MAKE(filearr);
    
    fileline_arr_read(&filearr, input_file);
    fclose(input_file);

    command_data_t* cmdbuf = NULL;
    size_t cmdbuf_size = 0;

    assembler_err_t asm_err = assembler_assemble_file(&filearr, commands, SIZEOF(commands), &cmdbuf, &cmdbuf_size);
    if(asm_err != ASSEMBLER_ERR_NONE) {
        utils_colored_fprintf(stderr, ANSI_COLOR_RED, "[ERROR] [ASM] %s\n", assembler_strerr(asm_err));
        NFREE(cmdbuf);
        return EXIT_FAILURE;
    }

    fileline_arr_free(&filearr);

    FILE* output_file = open_file(long_opts[1].arg, "w");
    if(output_file == NULL) {
        NFREE(cmdbuf);
        return EXIT_FAILURE;
    }

    assembler_write_to_file(output_file, cmdbuf, cmdbuf_size);

    fclose(output_file);

    NFREE(cmdbuf);

    utils_end_log();

    return EXIT_SUCCESS;
}
