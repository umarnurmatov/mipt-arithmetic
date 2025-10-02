#include "processor.h"

#include "stack.h"

processor_err_t processor_run(FILE* file, bytecode_t* bytecode, command_t* cmdarr, size_t cmdarr_size)
{
    STACK_MAKE(cmd_stack);

    bytecode_t cmdcode;
    while(fscanf(file, "%d", &cmdcode) != EOF) {
        cmdarr[cmdcode].callback();
    }
}
