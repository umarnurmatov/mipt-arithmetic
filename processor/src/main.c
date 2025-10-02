#include <stdlib.h>

#include "processor.h"
#include "optutils.h"
#include "logutils.h"
#include "colorutils.h"
#include "ioutils.h"
#include "callbacks.h"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "in", NULL, 0, 0 },
};

static const command_t commands[] = 
{
    { "ADD",  0x00, 2, cmd_add },
    { "SUB",  0x01, 2, cmd_sub },
    { "DIV",  0x02, 2 },
    { "MUL",  0x03, 2 },
    { "SQRT", 0x04, 1 },
    { "HLT",  0x05, 0 },
    { "OUT",  0x06, 1 }
};

int main(int argc, char* argv[])
{
    utils_init_log("log.txt", "log");

    utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts));

    if(!long_opts[0].is_set) {
        utils_colored_fprintf(stderr, ANSI_COLOR_RED, "[ERROR] [OPT] Specify input file\b");
        return EXIT_FAILURE;
    }

    FILE* input_file = open_file(long_opts[0].arg, "r");
    if(input_file == NULL)
        return EXIT_FAILURE;

    fclose(input_file);

    utils_end_log();

    return EXIT_SUCCESS;
}
