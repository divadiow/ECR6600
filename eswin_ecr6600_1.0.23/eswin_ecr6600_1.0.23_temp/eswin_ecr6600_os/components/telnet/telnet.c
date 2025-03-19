#include "stdio.h"
#include "oshal.h"
#include "cli.h"
#include "lwip/sockets.h"
#include "telnet.h"
#include "rtc.h"



#define MAX_CMD_LEN 128
#define MAX_CMD_RINGBUFFER_LEN 256
#define MAX_LOG_RINGBUFFER_LEN 5120   //5K
#define MAX_LOG_LEN 128
#define INVALID_HANDLE_FD -1



struct telnet_RingBuffer telnet_cmd_RingBuffer={0};
struct telnet_RingBuffer telnet_log_RingBuffer={0};
char tmp_log_buf[MAX_LOG_LEN+1];

os_sem_handle_t telnet_wait_log_sem;
os_sem_handle_t telnet_wait_close_sem;

uint8_t telnet_is_inited = 0;
int telnet_task_handle=INVALID_HANDLE_FD;
int telnet_printf_task_handle=INVALID_HANDLE_FD;

int telnet_accept_fd=INVALID_HANDLE_FD;

int telnet_state=STATE_NORMAL;
int telnet_negotiate_success=0;
int telnet_timeStamp = 1;


int write_telnet_RingBuffer(struct telnet_RingBuffer* ptcmd, char *data, int len)
{
	int i=0;
	unsigned long flags;
	
	if(ptcmd->write_pos==(ptcmd->read_pos+ptcmd->buffer_len-1)%ptcmd->buffer_len)
	{
		return -1;
	}
	
	flags = system_irq_save();
	for(i=0;i<len;i++)
	{
		*(ptcmd->buffer + ptcmd->write_pos)=data[i];
		ptcmd->write_pos=(ptcmd->write_pos+1)%ptcmd->buffer_len;
		if(ptcmd->write_pos==ptcmd->read_pos)
		{
			break;
		}
	}
	system_irq_restore(flags);
	
	return len;
}

int read_telnet_RingBuffer(struct telnet_RingBuffer* ptcmd, char *data, int len)
{
	int i=0;
	if(ptcmd->write_pos==ptcmd->read_pos)
	{
		return -1;
	}
	for(i=0;i<len;i++)
	{
		data[i]=*(ptcmd->buffer + ptcmd->read_pos);
		ptcmd->read_pos=(ptcmd->read_pos+1)%ptcmd->buffer_len;
		if(ptcmd->write_pos==ptcmd->read_pos)
		{
			break;
		}
	}
	return i+1;
}

int telnet_sendopt(int sockfd, uint8_t option, uint8_t value)
{
	uint8_t optbuf[3];
	int ret;

	optbuf[0] = TELNET_IAC;
	optbuf[1] = option;
	optbuf[2] = value;

	ret = send(sockfd, optbuf, 3, 0);
	return ret;
}

int telnet_send(int sockfd, char*data,int datalen)
{
	int ret;
	ret = send(sockfd, data, datalen, 0);
	return ret;
}

int telnet_getchar(uint8_t ch, char *dest, int *nread)
{
	int end=0;
	static int recived_cr=0;

	if (ch == 0)
	{
		return 0;
	}
	else if(ch == TELNET_CR)
	{
		recived_cr=1;
		*(dest+*nread)=0;
		end=1;
	}
	else if(ch == TELNET_NL)
	{
		if(recived_cr==1)
		{
			recived_cr=0;
			return 0;
		}
		else
		{
			*(dest+*nread)=0;
			end=1;
		}
	}
	else
	{
		*(dest+*nread)=ch; 
		recived_cr=0;
	}

	*nread=*nread+1;
	return end;
}

int telnet_RingBuffer_parse(struct telnet_RingBuffer* ptcmd, char*cmd, int *cmdlen)
{
	uint8_t ch;
	int end=0;
	while(read_telnet_RingBuffer(ptcmd, (char*)&ch, 1)>0)
	{

		switch (telnet_state)
		{
			case STATE_IAC:
				if (ch == TELNET_IAC)
				{
					telnet_state = STATE_NORMAL;
				}
				else
				{
					switch (ch)
					{
						case TELNET_WILL:
							telnet_state = STATE_WILL;
							break;

						case TELNET_WONT:
							telnet_state = STATE_WONT;
							break;

						case TELNET_DO:
							telnet_state = STATE_DO;
							break;

						case TELNET_DONT:
							telnet_state = STATE_DONT;
							break;

						default:
							telnet_state = STATE_NORMAL;
							break;
					}
				}
				break;

			case STATE_WILL:
				telnet_sendopt(telnet_accept_fd, TELNET_DONT, ch);
				os_printf(LM_APP,LL_INFO,"Suppress: 0x%02X (%d)\n", ch, ch);
				telnet_state = STATE_NORMAL;
				break;

			case STATE_WONT:
				telnet_sendopt(telnet_accept_fd, TELNET_DONT, ch);
				telnet_state = STATE_NORMAL;
				break;

			case STATE_DO:
				if (ch == TELNET_SGA || ch == TELNET_ECHO)
				{
					/* If it received 'ECHO' or 'Suppress Go Ahead', then do
					* nothing.
					*/
				}
				else
				{
					/* Reply with a WONT */

					telnet_sendopt(telnet_accept_fd, TELNET_WONT, ch);
					os_printf(LM_APP,LL_INFO,"WONT: 0x%02X\n", ch);
				}
				telnet_state = STATE_NORMAL;
				break;

			case STATE_DONT:
				/* Reply with a WONT */
				telnet_sendopt(telnet_accept_fd, TELNET_WONT, ch);
				telnet_state = STATE_NORMAL;
				break;

			case STATE_NORMAL:
				if (ch == TELNET_IAC)
				{
					telnet_state = STATE_IAC;
				}
				else
				{
					if(telnet_getchar(ch, cmd, cmdlen))
					{
						end=1;
					}
				}
				break;
		}
		if(end)
		{
			return 1;
		}
	}
	return 0;
}

void telnet_printf_task()
{
	char logbuf[513];
	int loglen;
	
	while(1)
	{
		os_sem_wait(telnet_wait_log_sem, WAIT_FOREVER);
		if(INVALID_HANDLE_FD!=telnet_accept_fd)
		{
			while(1)
			{
				loglen=read_telnet_RingBuffer(&telnet_log_RingBuffer,logbuf,512);
				logbuf[512]=0;
				if(loglen>0)
				{
					telnet_send(telnet_accept_fd, logbuf, loglen);
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			os_sem_post(telnet_wait_close_sem);
		}
	}
}


int telnet_vprintf(const char *f,  va_list ap)
{
        //unsigned long flags;
        int len=0;
        
        struct rtc_time time={0};

		if(0==telnet_negotiate_success)
		{
			return -1;
		}

        //flags = system_irq_save();
		
        if(telnet_timeStamp)
        {
                telnet_timeStamp=0;
                drv_rtc_get_time(&time);
                len = snprintf( &tmp_log_buf[0], MAX_LOG_LEN, "[%02d:%02d:%02d.%03d]", time.hour,time.min,time.sec,time.cnt_32k/33);
        }
        len += vsnprintf( &tmp_log_buf[len], MAX_LOG_LEN-len, f, ap);
        if(tmp_log_buf[len-1] == 0x0a)
        {
                telnet_timeStamp=1;
        }
		if(write_telnet_RingBuffer(&telnet_log_RingBuffer, tmp_log_buf, len) > 0)
		{
			os_sem_post(telnet_wait_log_sem);
		}


        //system_irq_restore(flags); 
		return 0;


}

int telnet_log_write(char *log, int len)
{
	if(telnet_negotiate_success)
	{
		if(write_telnet_RingBuffer(&telnet_log_RingBuffer, log, len) > 0)
		{
			os_sem_post(telnet_wait_log_sem);
		}
		return 0;
	}
	else
	{
		return -1;
	}
	
}
int log_task_init()
{
	if (INVALID_HANDLE_FD == telnet_printf_task_handle)
	{
		telnet_wait_log_sem = os_sem_create(1,0);
		telnet_wait_close_sem =os_sem_create(1,0);

		telnet_printf_task_handle=os_task_create((const char *)"telnet_printf_task",TELNET_LOG_TASK_PRIOPRITY,TELNET_LOG_TASK_STACK_SIZE,telnet_printf_task,NULL);
		if(INVALID_HANDLE_FD == telnet_printf_task_handle)
		{
			os_task_delete(telnet_printf_task_handle);
			os_sem_destroy(telnet_wait_log_sem);
			os_printf(LM_APP,LL_INFO,"create telnet_printf_task failed \r\n");
		
			
		}else{
			telnet_log_RingBuffer.buffer = (char*)os_malloc(MAX_LOG_RINGBUFFER_LEN);
			telnet_log_RingBuffer.buffer_len = MAX_LOG_RINGBUFFER_LEN;

			if(NULL==telnet_log_RingBuffer.buffer)
			{
				os_task_delete(telnet_printf_task_handle);
				os_sem_destroy(telnet_wait_log_sem);
				os_sem_destroy(telnet_wait_close_sem);
				telnet_printf_task_handle=INVALID_HANDLE_FD;
				os_printf(LM_APP,LL_INFO,"create telnet_printf_task failed \r\n");
			}
			else
			{
				os_printf(LM_APP,LL_INFO,"create telnet_printf_task ok\r\n");
			}
		}
	}
	return telnet_printf_task_handle;

}

void parse_cmd(char*cmd)
{
	char password_str[]="eswin";
	char* wellcome_log0 = "\
			\r\n>> **************************\
			\r\n>>    ___  __                            \
			\r\n>>   |__  /__` |  | | |\\ |\
			\r\n>>   |___ .__/ |/\\| | | \\|\
			\r\n>>\
			\r\n>> **************************\
			\r\n>> Wellcome use telnet @eswin\r\n>> ";
	
	char* wellcome_log = "\r\n>> Wrong password! ! !\r\npassword>> ";

	char *retry_log = "\r\n>> log task failed to create, please retry\r\n>>";

	if(0 == telnet_negotiate_success)
	{
		if(0 == strcmp(password_str, cmd))
		{

			if(log_task_init() < 0)
			{
				telnet_send(telnet_accept_fd, cmd, strlen(cmd));        
				telnet_send(telnet_accept_fd, retry_log, strlen(retry_log));
			}
			else
			{
				telnet_send(telnet_accept_fd, cmd, strlen(cmd));        
				telnet_send(telnet_accept_fd, wellcome_log0, strlen(wellcome_log0));
			
				telnet_negotiate_success=1;
			}
		}
		else
		{
			telnet_send(telnet_accept_fd, cmd, strlen(cmd));
			telnet_send(telnet_accept_fd, wellcome_log, strlen(wellcome_log));
		}
	}
	else
	{
		telnet_timeStamp = 0;
		os_printf(LM_APP,LL_INFO,"%s\n",cmd);
		telnet_timeStamp = 1;
		//os_printf(LM_APP,LL_INFO,"%s:", PROMPT_SYMBOL);
		if(cmd[0])
		{
			cli_cmd_deliver(cmd);
		}

	}
}


void telnet_task(void* arg)
{
	
	int ret;
	char* wellcome_log = "password>> ";

	struct sockaddr_in  local_addr;
	struct sockaddr_in  peer_addr;	
	socklen_t addrlen;
	
	int listen_fd=INVALID_HANDLE_FD;
	int accept_fd=INVALID_HANDLE_FD;

	
	struct linger ling;
	int optval;
	
	fd_set reads;
	fd_set excepts;
	
	ssize_t read_s;
	
	char cmdbuf[MAX_CMD_LEN]={0};
	char telnet_cmd[MAX_CMD_LEN]={0};
	int telnet_cmd_len=0;
	

	telnet_cmd_RingBuffer.buffer = (char*)os_malloc(MAX_CMD_RINGBUFFER_LEN);
	telnet_cmd_RingBuffer.buffer_len = MAX_CMD_RINGBUFFER_LEN;

	if(NULL==telnet_cmd_RingBuffer.buffer)
	{
		goto telnet_task_exit;
	}
	
	telnet_negotiate_success=0;

	local_addr.sin_family      = AF_INET;
    local_addr.sin_port        = htons(23);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    addrlen              = sizeof(struct sockaddr_in);

	
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd<0)
	{
		os_printf(LM_APP,LL_INFO,"socket:%s listensd:%d\r\n", strerror(errno), listen_fd);
		goto telnet_task_exit;
	}
	
	optval = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,(void *)&optval, sizeof(int)) < 0)
	{
		os_printf(LM_APP,LL_INFO,"setsockopt:%s optval:%d\r\n", strerror(errno), optval);
		goto telnet_task_exit;
	}

	if(bind(listen_fd, (struct sockaddr*)&local_addr, addrlen))
	{
		os_printf(LM_APP,LL_INFO,"bind:%s\n",strerror(errno));
		goto telnet_task_exit;
	}
	
	if(listen(listen_fd, 5))
	{
		os_printf(LM_APP,LL_INFO,"listen:%s\n",strerror(errno));
		goto telnet_task_exit;
	}
	
	while(1)
	{
		addrlen = sizeof(peer_addr);
	    telnet_accept_fd = accept(listen_fd, (struct sockaddr*)&peer_addr, &addrlen);
		if(telnet_accept_fd<0)
		{
			os_printf(LM_APP,LL_INFO,"accept:%s, task exit\n",strerror(errno));
			goto telnet_task_exit;
		}
		
		ling.l_onoff  = 1;
	    ling.l_linger = 30;     /* timeout is seconds */
	    setsockopt(telnet_accept_fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(struct linger));
		
		ret=telnet_sendopt(telnet_accept_fd, TELNET_WILL, TELNET_SGA);
	  	if(ret<0)
		{
			os_printf(LM_APP,LL_INFO,"send TELNET_SGA:%s\n",strerror(errno));
			goto errproc;
		}
		ret=telnet_sendopt(telnet_accept_fd, TELNET_WILL, TELNET_ECHO);
		if(ret<0)
		{
			os_printf(LM_APP,LL_INFO,"send TELNET_ECHO:%s\n",strerror(errno));
			goto errproc;
		}
		os_printf(LM_APP,LL_INFO,"################## telnet is ready to communicate ##################\r\n");
		ret = telnet_send(telnet_accept_fd, wellcome_log, strlen(wellcome_log));
		if(ret<0)
		{
			os_printf(LM_APP,LL_INFO,"send wellcome_log:%s\n",strerror(errno));
			goto errproc;
		}
		while(1)
		{
			FD_ZERO(&reads);
			FD_ZERO(&excepts);
			FD_SET(telnet_accept_fd, &reads);
			FD_SET(telnet_accept_fd, &excepts);

			ret= select(telnet_accept_fd+1, &reads, NULL, &excepts, NULL);
			if(ret>0)
			{
				if(FD_ISSET(telnet_accept_fd, &excepts))
				{
					os_printf(LM_APP,LL_INFO,"select exception:%s\n",strerror(errno));
					break;
				}
				if(FD_ISSET(telnet_accept_fd, &reads))
				{
					read_s = read(telnet_accept_fd, cmdbuf, sizeof(cmdbuf));
					if(read_s>0)
					{
						write_telnet_RingBuffer(&telnet_cmd_RingBuffer, cmdbuf, read_s);
						while(telnet_RingBuffer_parse(&telnet_cmd_RingBuffer, telnet_cmd, &telnet_cmd_len))
						{
							parse_cmd(telnet_cmd);
							telnet_cmd_len=0;
							
						}
					}
					else if(read_s<0)
					{
						os_printf(LM_APP,LL_INFO,"select:%s\n",strerror(errno));
						break;
					}
				}
			}
			else if(ret<0&&errno!=EINTR)
			{
				os_printf(LM_APP,LL_INFO,"select:%s\n",strerror(errno));
				break;
			}
		}
errproc:
		accept_fd=telnet_accept_fd;
		if(telnet_negotiate_success)
		{
			while(0==os_sem_wait(telnet_wait_close_sem,0));
			telnet_negotiate_success=0;
			telnet_accept_fd=INVALID_HANDLE_FD;
			os_sem_post(telnet_wait_log_sem);
			os_sem_wait(telnet_wait_close_sem,WAIT_FOREVER);
			
		}
		
		close(accept_fd);
		telnet_accept_fd=INVALID_HANDLE_FD;
		
		os_printf(LM_APP,LL_INFO,"################## telnet is failed to communicate ##################\r\n");
	}
telnet_task_exit:
	close(listen_fd);
	os_free(telnet_cmd_RingBuffer.buffer);
	memset(&telnet_cmd_RingBuffer,0x00,sizeof(telnet_cmd_RingBuffer));
	telnet_is_inited=0;
	os_printf(LM_APP,LL_INFO,"################## telnet exit ##################\r\n");
	os_task_delete((int)NULL);

}

int telnet_init(void)
{	

	telnet_task_handle=os_task_create((const char *)"telnet_task", TELNET_TASK_PRIOPRITY, TELNET_TASK_STACK_SIZE, telnet_task, NULL);
	if(telnet_task_handle == INVALID_HANDLE_FD)
	{
		os_printf(LM_APP,LL_INFO,"create telnet_task faile \r\n");
		return 0;
	}
	else
	{
		os_printf(LM_APP,LL_INFO,"create telnet_task ok\r\n");
		return 1;
	}
}


