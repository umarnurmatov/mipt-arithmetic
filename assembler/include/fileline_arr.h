#pragma once

#include "ioutils.h"

#define FILELINE_ARR_MAKE(varname) \
    fileline_arr_t varname =    {  \
        .lcnt        = 0,          \
        .arr         = NULL,       \
        .buffer      = NULL,       \
        .buffer_size = 0           \
    }                              \

typedef struct fileline_t
{
    size_t len;
    size_t lnum;
    char* str;
} fileline_t;

typedef struct fineline_arr_t
{
    size_t lcnt;
    fileline_t* arr;
    char* buffer;
    size_t buffer_size;
} fileline_arr_t;

typedef int(*fileline_arr_cmp_t)(fileline_t*, fileline_t*);

io_err_t fileline_arr_read(fileline_arr_t* filearr, FILE* stream);

fileline_t* fileline_arr_get(fileline_arr_t* filearr, size_t i);

void fileline_arr_swap(fileline_arr_t* filearr, size_t ia, size_t ib);

int fileline_arr_linecmp(const fileline_t* line_a, const fileline_t* line_b);

int fileline_arr_linercmp(const fileline_t* line_a, const fileline_t* line_b);

int fileline_arr_seqcmp(const fileline_t* line_a, const fileline_t* line_b);

io_err_t fileline_arr_put(fileline_arr_t* filearr, FILE* stream);

void fileline_arr_free(fileline_arr_t* filearr);

