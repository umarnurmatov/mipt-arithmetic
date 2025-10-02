#include "fileline_arr.h"

#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "ioutils.h"
#include "logutils.h"
#include "memutils.h"
#include "stringutils.h"

io_err_t fileline_arr_read(fileline_arr_t* filearr, FILE* stream)
{
    utils_assert(filearr != NULL);
    utils_assert(stream  != NULL);

    size_t file_size_b = get_file_size(stream);
    // TODO remove ul postfix
    char* buffer = (char*)calloc(1ul, file_size_b + 1ul);
    if(buffer == NULL) {
        // TODO add fileline and filenae to utils_log
        utils_log(
            LOG_LEVEL_ERR, 
            "failed to allocate memory"
        );
        return IO_ERR_ALLOCATION_FAIL;
    }

    size_t bytes_read = fread(buffer, sizeof(buffer[0]), file_size_b, stream);
    // TODO fix for windows
    if(bytes_read < file_size_b) {
        utils_log(
            LOG_LEVEL_ERR, 
            "failed to read file"
        );
        return IO_ERR_EOF_REACHED;
    }

    // count lines except blank
    // because of strtok()
    //
    // TODO make a function
    size_t lcnt = 0ul;
    char buffer_c_prev = '\n', buffer_c_curr = '\n';
    for(size_t bi = 0ul; bi < file_size_b; ++bi) {
        buffer_c_curr = buffer[bi];
        if(buffer_c_curr == '\n' && buffer_c_prev != '\n')
            ++lcnt;
        buffer_c_prev = buffer_c_curr;
    }

    filearr->arr = 
        (fileline_t*)calloc(lcnt, sizeof(filearr->arr[0]));
    if(buffer == NULL) {
        utils_log(
            LOG_LEVEL_ERR, 
            "failed to allocate memory"
        );
        return IO_ERR_ALLOCATION_FAIL;
    }
    filearr->lcnt = lcnt;
    filearr->buffer = buffer;
    // filearr->buffer_size =  // TODO;
    lcnt = 0ul;
    // TODO use strchar
    char* line = strtok(filearr->buffer, "\n");
    while(line) {
        filearr->arr[lcnt].str = line;
        filearr->arr[lcnt].lnum = lcnt;

        line = strtok(NULL, "\n");

        if(line == NULL)
            filearr->arr[lcnt].len = 
                utils_strlen(filearr->arr[lcnt].str);
        else 
            filearr->arr[lcnt].len = 
                (size_t)(line - filearr->arr[lcnt].str) - 1ul;

        ++lcnt;
    }

    return IO_ERR_NONE;
}

fileline_t* fileline_arr_get(fileline_arr_t* filearr, size_t i)
{
    utils_assert(filearr != NULL);
    utils_assert(i < filearr->lcnt);

    return filearr->arr + i;
}

void fileline_arr_swap(fileline_arr_t* filearr, size_t ia, size_t ib)
{
    utils_assert(filearr != NULL);
    utils_assert(ia < filearr->lcnt);
    utils_assert(ib < filearr->lcnt);

    fileline_t tmp_line = filearr->arr[ia];
    filearr->arr[ia]    = filearr->arr[ib];
    filearr->arr[ib]    = tmp_line;
}

int fileline_arr_linecmp(const fileline_t* line_a, const fileline_t* line_b)
{
    utils_assert(line_a != NULL);
    utils_assert(line_b != NULL);

    return strcmp(line_a->str, line_b->str);
}


int fileline_arr_linercmp(const fileline_t* line_a, const fileline_t* line_b)
{
    utils_assert(line_a != NULL);
    utils_assert(line_b != NULL);

    char* str_a = line_a->str;
    char* str_b = line_b->str;

    char* str_a_ptr = str_a + line_a->len;
    char* str_b_ptr = str_b + line_b->len;

    for( ;; )
    {
        if(!isalpha(*str_a_ptr)) {
            if(str_a_ptr == str_a)
                return -1;

            --str_a_ptr;
            continue;
        }

        if(!isalpha(*str_b_ptr)) {
            if(str_b_ptr == str_b)
                return 1;

            --str_b_ptr;
            continue;
        }

        if(*str_a_ptr > *str_b_ptr)
            return 1;
        else if(*str_a_ptr < *str_b_ptr)
            return -1;

        if((str_a_ptr == str_a) || (str_b_ptr == str_b))
            break;

        --str_a_ptr;
        --str_b_ptr;
    }

    return 0;
}

int fileline_arr_seqcmp(const fileline_t* line_a, const fileline_t* line_b)
{
    utils_assert(line_a != NULL);
    utils_assert(line_b != NULL);

    return line_a->lnum > line_b->lnum;
}

io_err_t fileline_arr_put(fileline_arr_t* filearr, FILE* stream)
{
    for(size_t li = 0ul; li < filearr->lcnt; ++li)
    {
        if(fprintf(stream, "%s\n", filearr->arr[li].str) < 0) {
            utils_log(
                LOG_LEVEL_ERR, 
                "file write error"
            );
            return IO_ERR_EOF_REACHED;
        }
    }

    return IO_ERR_NONE;
}

void fileline_arr_free(fileline_arr_t* filearr)
{
    utils_assert(filearr         != NULL);
    utils_assert(filearr->buffer != NULL);
    utils_assert(filearr->arr    != NULL);

    NFREE(filearr->buffer);
    NFREE(filearr->arr);
}

