#include "processor.h"

#include "utils.h"

cmd_callback_ret_t cmd_add(cmdarg_t a, cmdarg_t b);

cmd_callback_ret_t cmd_sub(cmdarg_t a, cmdarg_t b);

cmd_callback_ret_t cmd_mul(cmdarg_t a, cmdarg_t b);

cmd_callback_ret_t cmd_div(cmdarg_t a, cmdarg_t b);

cmd_callback_ret_t cmd_hlt(ATTR_UNUSED cmdarg_t a, ATTR_UNUSED cmdarg_t b);
