#include <stdlib.h>

#include "commands.h"
#include "processor.h"
#include "optutils.h"
#include "logutils.h"
#include "colorutils.h"
#include "ioutils.h"
#include "memutils.h"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "in", NULL, 0, 0 },
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

    processor_t processor = {
        .STACK_INITLIST(stack),
        .cmdbuf = NULL,
        .cmdbuf_size = 0
    };

    if(processor_ctor(&processor, input_file) != PROCESSOR_ERR_NONE)
        abort();

    fclose(input_file);

    processor_run(&processor, commands, SIZEOF(commands));

    processor_dtor(&processor);

    utils_end_log();

    return EXIT_SUCCESS;
}
