#include <stdlib.h>

#include "commands.h"
#include "processor.h"
#include "optutils.h"
#include "logutils.h"
#include "colorutils.h"
#include "ioutils.h"
#include "memutils.h"

#define LOG_CATEGORY_OPT "CLI OPTIONS"
#define LOG_CATEGORY_FILEOP "FILE OPERATIONS"
#define LOG_CATEGORY_PROCESSOR "PROCESSOR"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "in"  , NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "dump", NULL, 0, 0 },
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
        UTILS_LOGE(LOG_CATEGORY_OPT, "specify input file");
        return EXIT_FAILURE;
    }

    FILE* input_file = open_file(long_opts[0].arg, "r");
    if(input_file == NULL) {
        UTILS_LOGE(LOG_CATEGORY_FILEOP, "could not open input file");
        return EXIT_FAILURE;
    }

    FILE* dump_file = open_file(long_opts[1].arg, "w");
    if(dump_file == NULL) {
        UTILS_LOGE(LOG_CATEGORY_FILEOP, "could not open dump file");
        return EXIT_FAILURE;
    }

    processor_t processor = {
        .STACK_INITLIST(stack),
        .cmdbuf = NULL,
        .cmdbuf_size = 0
    };

    if(processor_ctor(&processor, input_file) != PROCESSOR_ERR_NONE) {
        UTILS_LOGE(LOG_CATEGORY_PROCESSOR, "processor init error");
        return EXIT_FAILURE;
    }

    processor_set_dump_file(&processor, dump_file);

    fclose(input_file);

    if(processor_run(&processor) != PROCESSOR_ERR_NONE) {
        UTILS_LOGE(LOG_CATEGORY_PROCESSOR, "processor runtime error");
        return EXIT_FAILURE;
    }

    processor_dtor(&processor);

    fclose(dump_file);

    utils_end_log();

    return EXIT_SUCCESS;
}
