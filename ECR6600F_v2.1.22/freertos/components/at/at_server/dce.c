#include "stdlib.h"
#include "stdio.h"
#include "dce.h"
#include "dce_private.h"
#include "dce_utils.h"
#include "dce_basic_commands.h"
#include "at_common.h"
#include "at_def.h"

#define AT_ESCAPE '\\'
#define DCE_CONTINUE -1


static const char* dce_result_code_v0[] = {
    "0",
    "1",
};

static const char* dce_result_code_v1[] = {
    "OK",
    "ERROR"
};

dce_data_input_callback data_input_cb = NULL;


dce_t*  dce_init(size_t rx_buffer_size)
{
    dce_t* ctx = (dce_t*) malloc(sizeof(dce_t));
    
    ctx->rx_buffer_size  = rx_buffer_size;
    ctx->rx_buffer       = (char*) malloc(rx_buffer_size);
    ctx->rx_buffer_pos   = 0;
    
    ctx->command_line_buf_size = rx_buffer_size;
    ctx->command_line_buf      = (char*) malloc(rx_buffer_size);
    ctx->command_line_length   = 0;
    
    ctx->command_groups_count  = 0;
    ctx->state                 = COMMAND_STATE;
    ctx->command_pending       = 0;
    
    dce_init_defaults(ctx);
    
    return ctx;
}

void  dce_init_defaults(dce_t* ctx)
{
    ctx->cr              = 13;
    ctx->lf              = 10;
    ctx->bs              = 8;
    ctx->echo            = 1;
    ctx->suppress_rc     = 0;
    ctx->response_fmt    = 1;
}


void  dce_register_command_group(dce_t* ctx, const char* groupname, const command_desc_t* desc, int ndesc, void* group_ctx)
{
    if (ctx->command_groups_count == DCE_MAX_COMMAND_GROUPS)
    {
        os_printf(LM_APP, LL_INFO, "command groups limit exceeded");
        return;
    }
    command_group_t* next = ctx->command_groups + ctx->command_groups_count;
    next->commands       = desc;
    next->commands_count = ndesc;
    next->groupname      = groupname;
    next->context        = group_ctx;

    ctx->command_groups_count++;
}

void  dce_emit_response_prefix(dce_t* dce)
{
    const char crlf[] = {dce->cr, dce->lf};
    if (dce->response_fmt == 1) // 6.2.6 DCE response format
    {
        target_dce_transmit(crlf, 2);
    }
}

void  dce_emit_response_suffix(dce_t* dce)
{
    if (dce->response_fmt == 1) // 6.2.6 DCE response format
    {
        const char crlf[] = {dce->cr, dce->lf};
        target_dce_transmit(crlf, 2);
    }
    else
    {
        target_dce_transmit(&dce->cr, 1);
    }
}

void  dce_emit_basic_result_code(dce_t* dce, dce_result_code_t result)
{
    dce->command_pending = 0;
    
    if (dce->suppress_rc)   // 6.2.5 Result code suppression
        return;

    dce_emit_response_prefix(dce);
    const char* text = ((dce->response_fmt == 0)?dce_result_code_v0:dce_result_code_v1)[result];
    size_t length = strlen(text);
    target_dce_transmit(text, length);
    dce_emit_response_suffix(dce);
}
void  TG_dce_emit_extended_result_code_with_args(dce_t* dce, const char* command_name, size_t size, const arg_t* args, size_t argc, int reset_command_pending, bool arg_in_brackets,int data_len)
{
    if (reset_command_pending)
        dce->command_pending = 0;
    
    if (dce->suppress_rc)   // 6.2.5 Result code suppression
        return;
    //dce_emit_response_prefix(dce);
    
    if (size == -1)
        size = strlen(command_name);
    target_dce_transmit("+", 1);
    target_dce_transmit(command_name, size);
    target_dce_transmit(":", 1);

    if(arg_in_brackets == true){
        target_dce_transmit("(", 1);
    }

    size_t iarg;
	if (argc==1)
		{
		os_printf(LM_APP, LL_INFO, "----argc=1---\r\n");

		if (args->type == ARG_TYPE_STRING)
        {
            target_dce_transmit("\"", 1);
            // TODO: escape any quotes in arg->value.string
           const  char* str = args->value.string;
			size_t str_size = strlen(str);
		
			str_size =(size_t) data_len;
	
			os_printf(LM_APP, LL_INFO, "----str_size=%d---\r\n",str_size);
            target_dce_transmit(str, str_size);
			
            target_dce_transmit("\"", 1);
			
    		dce_emit_response_suffix(dce); 
        }
		return;
		}
    for (iarg = 0; iarg < argc; ++iarg)
    {
        const arg_t* arg = args + iarg;
        if (arg->type == ARG_TYPE_STRING)
        {
            target_dce_transmit("\"", 1);
            // TODO: escape any quotes in arg->value.string
           const  char* str = arg->value.string;
			size_t str_size = strlen(str);
			
			
			if(iarg==3)
				 str_size =(size_t) data_len;
           		
		   
			
            target_dce_transmit(str, str_size);
			
            target_dce_transmit("\"", 1);
        }
        else if (arg->type == ARG_TYPE_NUMBER)
        {
            char buf[12];
            size_t str_size;
            dce_itoa(arg->value.number, buf, sizeof(buf), &str_size);
            target_dce_transmit(buf, str_size);
        }
        if (iarg != argc - 1)
            target_dce_transmit(",", 1);
    }
    if(arg_in_brackets == true){
        target_dce_transmit(")", 1);
    }
    dce_emit_response_suffix(dce); 
}

void  dce_emit_extended_result_code_with_args(dce_t* dce, const char* command_name, size_t size, const arg_t* args, size_t argc, int reset_command_pending, bool arg_in_brackets)
{
    if (reset_command_pending)
        dce->command_pending = 0;
    
    if (dce->suppress_rc)   // 6.2.5 Result code suppression
        return;
    //dce_emit_response_prefix(dce);
    
    if (size == -1)
        size = strlen(command_name);
    target_dce_transmit("+", 1);
    target_dce_transmit(command_name, size);
    target_dce_transmit(":", 1);

    if(arg_in_brackets == true){
        target_dce_transmit("(", 1);
    }

    size_t iarg;
    for (iarg = 0; iarg < argc; ++iarg)
    {
        const arg_t* arg = args + iarg;
        if (arg->type == ARG_TYPE_STRING)
        {
            target_dce_transmit("\"", 1);
            // TODO: escape any quotes in arg->value.string
           const char* str = arg->value.string;
			
           size_t str_size = strlen(str);
		   
			//system_printf("str_size=-------%d------------\n",str_size);
			//for(int i =0;i<str_size;i++)
				//{
				//os_printf(LM_APP, LL_INFO, "str_size[%d]=%02x-------------------\n",i,(unsigned char )str[i]);
				//}
            target_dce_transmit(str, str_size);
            target_dce_transmit("\"", 1);
        }
        else if (arg->type == ARG_TYPE_NUMBER)
        {
            char buf[12];
            size_t str_size;
            dce_itoa(arg->value.number, buf, sizeof(buf), &str_size);
            target_dce_transmit(buf, str_size);
        }
        if (iarg != argc - 1)
            target_dce_transmit(",", 1);
    }
    if(arg_in_brackets == true){
        target_dce_transmit(")", 1);
    }
    dce_emit_response_suffix(dce); 
}

void  dce_emit_extended_result_code(dce_t* dce, const char* response, size_t size, int reset_command_pending)
{
    if (reset_command_pending)
        dce->command_pending = 0;
    
    if (dce->suppress_rc)   // 6.2.5 Result code suppression
        return;
    
    // dce_emit_response_prefix(dce);
    if (size == -1)
        size = strlen(response);
    target_dce_transmit(response, size);
    dce_emit_response_suffix(dce);
}

void  dce_emit_information_response(dce_t* dce, const char* response, size_t size)
{
    // dce_emit_response_prefix(dce);
    if (size == -1)
        size = strlen(response);
    target_dce_transmit(response, size);
    dce_emit_response_suffix(dce);
}

void  dce_emit_pure_response(dce_t* dce, const char* response, size_t size)
{
    //dce_emit_response_prefix(dce);
    if (size == -1)
        size = strlen(response);
    target_dce_transmit(response, size);
}

void  dce_continue_information_response(dce_t* dce, const char* response, size_t size)
{
    if (size == -1)
        size = strlen(response);
    target_dce_transmit(response, size);
    dce_emit_response_suffix(dce);
}

void dce_get_strPos(const command_desc_t* command, char *strPos)
{
	return;
}

dce_result_t  dce_parse_args(const char* cbuf, size_t size, const command_desc_t* command, size_t* pargc, arg_t* args)
{
    // we'll be parsing arguments in place
    char* buf = (char*) cbuf; //it actually is a modifiable rx buffer, so TODO: remove const everywhere up the call chain
    int argc = 0;
    int strUsed    = 0;
    char strPos[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    dce_get_strPos(command, strPos);
    //os_printf(LM_APP, LL_INFO, "strPos:%d,%d,%d,%d\r\n", strPos[0], strPos[1], strPos[2], strPos[3]);
            
    while (size > 0)
    {
        char c = *buf;
        arg_t arg;
        arg.type = ARG_NOT_SPECIFIED;
        arg.value.string = 0;

        if(argc == strPos[strUsed])
        {
            char* result;
            strUsed++;
            if (dce_expect_string_no_mark(&buf, &size, &result) != 0)
                return DCE_INVALID_INPUT;
            arg.type = ARG_TYPE_STRING;
            arg.value.string = result;
            if (size > 0 && *buf == ',')
            {
                ++buf;
                --size;
            }            
        }
        else if (c == '"')    // it's a string
        {
            char* result;
            if (dce_expect_string(&buf, &size, &result) != 0)
                return DCE_INVALID_INPUT;
            arg.type = ARG_TYPE_STRING;
            arg.value.string = result;
            if (size > 0 && *buf == ',')
            {
                ++buf;
                --size;
            }
        }
        // TODO: add support for hex and binary numbers (5.4.2.1)
        else if (c == '-'){
            size--;
            buf++;
            arg.value.number = dce_expect_number((const char**)&buf, &size, 0);
            arg.value.number = -arg.value.number;
            
            arg.type = ARG_TYPE_NUMBER;
            if (size > 0 && *buf == ',')
            {
                ++buf;
                --size;
            }
        }
        else if (c >= '0' && c <= '9') // it's a number
        {
            arg.value.number = dce_expect_number((const char**)&buf, &size, 0);
            arg.type = ARG_TYPE_NUMBER;
            if (size > 0 && *buf == ',')
            {
                ++buf;
                --size;
            }
        }
        else if (c != ',')
        {

            os_printf(LM_APP, LL_INFO, "expected ,");
            return DCE_INVALID_INPUT;
        }
        else if (c == ','){
            arg.type = ARG_NOT_SPECIFIED;
            --size;
            ++buf;
        }
        if (argc == DCE_MAX_ARGS)
        {
            os_printf(LM_APP, LL_INFO, "too many arguments");
            return DCE_INVALID_INPUT;
        }
        args[argc++] = arg;
    }
    *pargc = argc;
    return DCE_OK;
}

dce_result_code_t  dce_process_args_run_command(dce_t* ctx, const command_group_t* group, const command_desc_t* command, const char* buf, size_t size)
{
    int flags = command->flags;

    if (size == 0)    // 5.4.3.1 Action execution, no arguments
    {
        if (!(flags & DCE_EXEC))
        {
            dce_emit_basic_result_code(ctx, DCE_RC_ERROR);
            return DCE_OK;
        }
        return (*command->fn)(ctx, group->context, DCE_EXEC, 0, 0);
    }
    
    char c = buf[0];
    if (c == '?')   // 5.4.4.3 parameter read (AT+FOO?)
    {
 
        if (!(flags & DCE_READ))
        {
            dce_emit_basic_result_code(ctx, DCE_RC_ERROR);
            return DCE_OK;
        }
        return (*command->fn)(ctx, group->context, DCE_READ, 0, 0);
    }

    else if (c == '=')
    {
        if (size > 1 && buf[1] == '?')  // 5.4.3.2, 5.4.4.4 action or parameter test (AT+FOO=?)
        {
            if (!(flags & DCE_TEST))
            {
                // paramter or action is supported, but does not respond to queries
                dce_emit_basic_result_code(ctx, DCE_RC_OK);
                return DCE_OK;
            }
            return (*command->fn)(ctx, group->context, DCE_TEST, 0, 0);
        }
        else    // 5.4.4.2, 5.4.3.1 paramter set or execute action with subparameters (AT+FOO=params)
        {
            size_t argc = 0;
            arg_t args[DCE_MAX_ARGS];
            int rc = dce_parse_args(buf + 1, size - 1, command, &argc, args);
            if (rc != DCE_OK || argc == 0)
            {
                os_printf(LM_APP, LL_INFO, "failed to parse args");
                dce_emit_basic_result_code(ctx, DCE_RC_ERROR);
                return DCE_OK;
            }
            int kind = (argc >= 1) ? DCE_WRITE : DCE_EXEC;
            
            return (*command->fn)(ctx, group->context, kind, argc, args);
        }
    }
    os_printf(LM_APP, LL_INFO, "should be unreachable");
    return DCE_OK;
}


dce_result_t  dce_process_extended_format_command(dce_t* ctx, const char* buf, size_t size)
{
    int igrp;
//    cli_printf("ext grp sum:%d \n", ctx->command_groups);
//    for(igrp=0; igrp<size; igrp++)
//    {
//        cli_printf("%c", buf[igrp]);
//    }
//    cli_printf("\n");

    for (igrp = 0; igrp < ctx->command_groups_count; ++igrp)
    {
        const command_group_t* group = ctx->command_groups + igrp;
        int pos;
        int icmd;
        for (icmd = 0; icmd < group->commands_count; ++icmd)
        {
            const command_desc_t* command = group->commands + icmd;

            for (pos = 0; pos < size && command->name[pos] != 0 && buf[pos] == command->name[pos]; ++pos);
            
            if (command->name[pos] != 0)
                continue;
            
            if (pos < size && buf[pos] != '=' && buf[pos] != '?')
                continue;
            return dce_process_args_run_command(ctx, group, command, buf + pos, size - pos);
        }
    }

    os_printf(LM_APP, LL_INFO, "command handler not found");
    dce_emit_basic_result_code(ctx, DCE_RC_ERROR);
    return DCE_OK;
}

dce_result_t  dce_process_command_line(dce_t* ctx)
{
    ctx->command_pending = 1;
    
    // 5.2.1 command line format
    size_t size = ctx->command_line_length;
    char *buf = ctx->command_line_buf;
    
    // dce_emit_pure_response(ctx, buf, size);
    //cli_printf("cmd:%c%c%c%c\n",buf[0],buf[1],buf[2],buf[3]);
    if (size < 2)
    {
        //os_printf(LM_APP, LL_INFO, "line does not start with AT\n");
        dce_emit_basic_result_code(ctx, DCE_RC_ERROR);
        return DCE_OK;
    }
    
    // command line should start with 'AT' prefix
    // TODO: implement support for command line repetition (5.2.4)
    if (!BEGINS(buf, 'A', 'T'))
    {
        //os_printf(LM_APP, LL_INFO, "line does not start with AT\n");
        dce_emit_basic_result_code(ctx, DCE_RC_ERROR);
        return DCE_OK;
    }
    if (size == 3)   // 'AT' was sent
    {
        dce_emit_basic_result_code(ctx, DCE_RC_OK);
        return DCE_OK;
    }
    
    buf += 2;
    size -= 2;
    // process single command
    // TODO: implement support for multiple commands per line
    
    char c = buf[0];
    if (c == '+')
        return dce_process_extended_format_command(ctx, buf + 1, size - 2);

    if (c == 'S')  // S-parameter (5.3.2)
        return dce_process_sparameter_command(ctx, buf + 1, size - 2);
    
    return dce_process_basic_command(ctx, buf, size);
}


dce_result_t  dce_handle_command_state_input(dce_t* ctx, const char* cmd, size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
    {
        char c = cmd[i];
        if (c == ctx->bs)       // command line editing (5.2.2)
        {
            if (ctx->rx_buffer_pos > 0)
            {
                --ctx->rx_buffer_pos;
            }
            if (ctx->echo)
            {
                unsigned int uart_base_reg = ctx->at_uart_base;
                while(drv_uart_tx_ready(uart_base_reg));
                drv_uart_tx_putc(uart_base_reg, '\b');
                drv_uart_tx_putc(uart_base_reg, ' ');
                drv_uart_tx_putc(uart_base_reg, '\b');
            }
            continue;
        }
        if (ctx->rx_buffer_pos >= ctx->rx_buffer_size)
        {
            //os_printf(LM_APP, LL_INFO, "rx buffer overflow");
            //dce_emit_basic_result_code(ctx, DCE_RC_ERROR);
            ctx->rx_buffer_pos = 0;
            return DCE_OK;
        }
        if (ctx->rx_buffer_pos == 0 && c == ctx->lf)
        {
            // consume LF that some terminals send after CR
            if (ctx->echo)
            {
                unsigned int uart_base_reg = ctx->at_uart_base;
                while(drv_uart_tx_ready(uart_base_reg));
                drv_uart_tx_putc(uart_base_reg, c);
            }
            continue;
        }
        if (ctx->echo)
        {
        	unsigned int uart_base_reg = ctx->at_uart_base;
            while(drv_uart_tx_ready(uart_base_reg));
            drv_uart_tx_putc(uart_base_reg, c);
        }
        if (c == ctx->lf)
        {            
            memcpy(ctx->command_line_buf, ctx->rx_buffer, ctx->rx_buffer_pos);
            ctx->command_line_length = ctx->rx_buffer_pos;
            ctx->rx_buffer_pos = 0;
            target_dce_request_process_command_line(ctx);
        }
        else
        {
            ctx->rx_buffer[ctx->rx_buffer_pos++] = c;
        }
    }
    return DCE_OK;
}

dce_result_t dce_register_data_input_cb(dce_data_input_callback func)
{
    data_input_cb = func;
    return DCE_OK;
}

dce_result_t  dce_handle_data_state_input(dce_t* ctx, const char* cmd, size_t size)
{
    if (data_input_cb) {
        data_input_cb(ctx, cmd, size);
    }
    return DCE_OK;
}

dce_result_t  dce_handle_input(dce_t* ctx, const char* cmd, size_t size)
{   
    if (IS_COMMAND_STATE(ctx->state))
    {
        return dce_handle_command_state_input(ctx, cmd, size);
    }
    else
    {
        return dce_handle_data_state_input(ctx, cmd, size);
    }
}

void  dce_uninit(dce_t* ctx)
{
    free(ctx->rx_buffer);
    free(ctx->command_line_buf);
    free(ctx);
}

dce_result_t dce_handle_flashEn_arg(int kind, size_t *p_argc, arg_t* argv, bool *p_flash_en)
{
    if(kind & DCE_WRITE)
    {
        if((*p_argc>=2) && (ARG_TYPE_STRING ==argv[*p_argc-1].type))
        {
            if(0 == strcmp("s.y", argv[*p_argc-1].value.string))
            {
                (*p_argc)--;
                *p_flash_en = true;
                os_printf(LM_APP, LL_INFO, "DEF set\r\n", sizeof("DEF set\r\n"));
            }
            else if(0 == strcmp("s.n", argv[*p_argc-1].value.string))
            {
                (*p_argc)--;
                *p_flash_en = false;
                os_printf(LM_APP, LL_INFO, "CUR set\r\n", sizeof("CUR set\r\n"));
            }
            else
            {
                return DCE_RC_ERROR;
            }
        }
        else
        {
            return DCE_RC_ERROR;
        }
    }
    return DCE_RC_OK;
}
