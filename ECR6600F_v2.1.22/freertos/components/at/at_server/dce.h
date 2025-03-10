#ifndef __DCE_H
#define __DCE_H

#include "stddef.h"
#include "stdbool.h"

#define VERSION_STRING "0.1"

#define DCE_OK                 0
#define DCE_BUFFER_OVERFLOW    1
#define DCE_INVALID_INPUT      2
#define DCE_FAILED             3

#define DCE_TEST     1
#define DCE_EXEC     2
#define DCE_WRITE    4
#define DCE_READ     8

#define DCE_MAX_COMMAND_GROUPS 16
#define DCE_MAX_ARGS 16

#define IS_COMMAND_STATE(state) (state & 1)
#define BEGINS(buf, c1, c2) (buf[0] == (c1) && buf[1] == (c2))

typedef enum {
	COMMAND_CUR,
	COMMAND_DEF,
} cmd_type_e;

typedef enum {
    ARG_TYPE_NUMBER,
    ARG_TYPE_STRING,
    ARG_NOT_SPECIFIED
} arg_type_t;

typedef enum {
    COMMAND_STATE = 1,
    ONLINE_DATA_STATE = 2,
    ONLINE_COMMAND_STATE = 3,
    TCP_ONLINE_DATA_STATE = 4
} state_t;

typedef enum {
    DCE_RC_OK = 0,
    DCE_RC_ERROR = 1,
} dce_result_code_t;

typedef struct {
    arg_type_t type;
    union {
        int number;
        const char * string;
    } value;
} arg_t;

typedef struct dce_ dce_t;
typedef int dce_result_t;
typedef void (*dce_data_input_callback)(void *, const char *, size_t);
typedef int  (*cmdhandler_t)(dce_t* dce, void* group_context, int kind, size_t argc, arg_t* argv);
typedef struct
{
    const char* name;
    cmdhandler_t fn;
    int flags;
} command_desc_t;

typedef struct {
    const char* groupname;
    const command_desc_t* commands;
    int commands_count;
    void* context;
} command_group_t;

struct dce_
{
    size_t  rx_buffer_size;
    char*   rx_buffer;
    size_t  rx_buffer_pos;
    
    char*   command_line_buf;
    size_t  command_line_buf_size;
    size_t  command_line_length;
    
    command_group_t command_groups[DCE_MAX_COMMAND_GROUPS];
    size_t  command_groups_count;
    
    state_t state;
    
    char    cr;             /// 6.2.1 parameter S3
    char    lf;             /// 6.2.2 parameter S4
    char    bs;             /// 6.2.3 parameter S5
    char    echo;           /// 6.2.4 parameter E
    char    suppress_rc;    /// 6.2.5 parameter Q
    char    response_fmt;   /// 6.2.6 parameter V
    
    char    command_pending;
    unsigned int  at_uart_base;
};

void dce_register_command_group(dce_t* dce, const char* groupname, const command_desc_t* desc, int ndesc, void* ctx);
void dce_emit_extended_result_code_with_args(dce_t* dce, const char* command_name, size_t size, const arg_t* args, size_t argc, int reset_command_pending,  bool arg_in_brackets);
void TG_dce_emit_extended_result_code_with_args(dce_t* dce, const char* command_name, size_t size, const arg_t* args, size_t argc, int reset_command_pending,  bool arg_in_brackets,int data_len);

void dce_init_defaults(dce_t* dce);
void dce_emit_response_prefix(dce_t* dce);
void dce_emit_response_suffix(dce_t* dce);

dce_t* dce_init(size_t rx_buffer_size);
dce_result_t dce_handle_input(dce_t* dce, const char* cmd, size_t size);
dce_result_t dce_process_command_line(dce_t* dce);
dce_result_t dce_register_data_input_cb(dce_data_input_callback func);

void dce_emit_basic_result_code(dce_t* dce, dce_result_code_t result);
// use size=-1 for zero-terminated strings if size is not known
void dce_emit_information_response(dce_t* dce, const char* response, size_t size);
void dce_continue_information_response(dce_t* dce, const char* response, size_t size);
void dce_emit_extended_result_code(dce_t* dce, const char* response, size_t size, int reset_command_pending);
void dce_emit_pure_response(dce_t* dce, const char* response, size_t size);
dce_result_t dce_handle_flashEn_arg(int kind, size_t *p_argc, arg_t* argv, bool *p_flash_en);

void dce_uninit(dce_t* dce);

#endif //__DCE_H
