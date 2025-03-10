#ifndef __CUSTOMER_CMD
#define __CUSTOMER_CMD

#define AT_TEST     1
#define AT_EXEC     2
#define AT_WRITE    4
#define AT_READ     8

typedef enum {
    AT_OK = 0,
    AT_ERROR = 1,
} at_result_code_t;


typedef enum {
    AT_COMMAND_STATE = 1,
    AT_ONLINE_DATA_STATE = 2,
    AT_ONLINE_COMMAND_STATE = 3
} at_state_t;

typedef enum {
    AT_ARG_TYPE_NUMBER,
    AT_ARG_TYPE_STRING,
    AT_ARG_NOT_SPECIFIED
}at_arg_type_t;

typedef struct {
    at_arg_type_t type;
    union {
        int number;
        const char * string;
    } value;
} at_arg_t;

typedef int  (*user_cmdhandler_t)(void* prv, void* group, int kind, size_t argc, at_arg_t* argv);

typedef struct
{
    const char* name;
    user_cmdhandler_t fn;
    int flags;
} at_command_list;

void at_register_commands(const at_command_list* list,int cmd_num);

void at_emit_basic_result(at_result_code_t result);

void at_emit_information_response(const char* response, size_t size);

void at_emit_extended_result_code(const char* response, size_t size, int reset_command_pending);

void at_emit_extended_result_code_with_args(const char* command_name, \
                                            size_t size, const at_arg_t* args, \
                                            size_t argc, int reset_command_pending,  \
                                            bool arg_in_brackets);

void AT_command_init(void);

void AT_command_callback(char c);

at_state_t AT_command_get_state(void);

void AT_command_data_callback(char* buff, size_t size);

// customer register an AT command example,can see that in main.c of AT project:

// at_result_code_t at_handle_CUSTOMER_TEST(void* dce, void* group, int kind, size_t argc, at_arg_t* argv)
// {
    
//     if (kind & AT_READ){ //AT COMMAND:"AT+CUSTOMER_TEST?
//         at_emit_information_response("READ CMD");
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     } else if(kind & AT_WRITE) {  //AT COMMAND:"AT+CUSTOMER_TEST=1
//         at_emit_extended_result_code_with_args("WRITE CMD ARGC = %ld, argv = %d",argc,argv[0].value.number);
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     } else if(kind & AT_EXEC){ //AT COMMAND:"AT+CUSTOMER_TEST
//         at_emit_information_response("EXE CMD");
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     } else if(kind & AT_TEST){ //AT COMMAND:"AT+CUSTOMER_TEST=?"
//         at_emit_information_response("TEST CMD");
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     }

// }

// static const at_command_list CUS_commands[] = {
//     {"CUSTOMER_TEST"     , &at_handle_CUSTOMER_TEST   , AT_TEST| AT_EXEC |AT_WRITE | AT_READ},
// };
// at_register_commands(CUS_commands, 1);

#endif
