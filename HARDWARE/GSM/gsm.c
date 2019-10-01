#include "gsm.h"
#include "usart.h"		
#include "delay.h"	 
#include "string.h" 
#include "usart.h" 
#include "math.h"
#include "stdio.h"
#include "dma.h"
#include "FreeRTOS.h"
#include "task.h"
#include "print.h"

u8 Flag_Rec_Message = 0;
u8 SIM_CON_OK;

// usmart透传
void sim_at_response(u8 mode)
{
	if(USART2_RX_STA & 0X8000)
	{
		USART2_RX_BUF[USART2_RX_STA&0X7FFF] = 0;
		printf("%s",USART2_RX_BUF);
		if(mode)USART2_RX_STA = 0;
	} 
}

u8* sim800c_check_cmd(u8 *str)
{
	char *strx = 0;
	if(USART2_RX_STA & 0X8000)
	{
		USART2_RX_BUF[USART2_RX_STA&0X7FFF] = 0;	// 添加结束符
		strx = strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}

u8 sim800c_send_cmd(u8 *cmd, u8 *ack, u16 waittime)
{
	u8 res = 0;
	
	USART2_RX_STA = 0;

	vTaskSuspendAll();
	if((u32)cmd<=0XFF)
	{
		while(DMA1_Channel7->CNDTR != 0);			// 等待通道7传输完成   
		USART2->DR=(u32)cmd;
	}
	else
	{
		if(cmd[0]=='+'&&cmd[1]=='+')
			USART2_vspf("%s",cmd);
		else USART2_vspf("%s\r\n",cmd);				// 发送命令
	}
	xTaskResumeAll();
	
	// 等待应答
	if(ack && waittime)
	{
		while(--waittime)
		{
			vTaskDelay(20);
			if(USART2_RX_STA & 0X8000)
			{
			//	printf("USART2_RX_BUF receiv:%s\r\n",USART2_RX_BUF);
				if(sim800c_check_cmd(ack))break;	// 得到有效数据 
				USART2_RX_STA = 0;
			}
		}
		if(waittime == 0)res = 1;					// 超时
	}	
	UART_DMA_Enable(DMA1_Channel6, USART2_MAX_LEN);
	USART2_RX_STA = 0;
	return res;
} 

u8 sim800c_work_test(void)
{
	vTaskDelay(1000/portTICK_RATE_MS);  
	sim800c_send_cmd((u8 *)"+++",(u8 *)"OK",100);  //AT+CPIN?
	vTaskDelay(1000/portTICK_RATE_MS);
	
//	
//	sim800c_send_cmd((u8 *)"AT+CSQ",(u8 *)"OK",100);//
//  vTaskDelay(1000/portTICK_RATE_MS);	
//	
//	sim800c_send_cmd((u8 *)"AT+CREG?",(u8 *)"OK",100);//
//  vTaskDelay(1000/portTICK_RATE_MS);	
//	
//	sim800c_send_cmd((u8 *)"AT+COPS?",(u8 *)"OK",100);//
//  vTaskDelay(1000/portTICK_RATE_MS);	
//	
//	sim800c_send_cmd((u8 *)"AT+CGATT=1",(u8 *)"OK",100);//
//  vTaskDelay(1000/portTICK_RATE_MS);	
	
	if(sim800c_send_cmd((u8 *)"AT",(u8 *)"OK",100))
	{
		if(sim800c_send_cmd((u8 *)"AT",(u8 *)"OK",100))return SIM_COMMUNTION_ERR;	// 通信不上
	}		
	if(sim800c_send_cmd((u8 *)"AT+CPIN?",(u8 *)"READY",400))return SIM_CPIN_ERR;	// 没有SIM卡
	if(sim800c_send_cmd((u8 *)"AT+CREG?",(u8 *)"0,1",400)){}
	else return SIM_CREG_FAIL;														// 注册失败
	
	return SIM_OK;
}

u8 gsm_dect(void)
{
	u8 res;
	
	res = sim800c_work_test();	
	switch(res)
	{
		case SIM_OK:
			Printf("GSM模块自检成功\r\n");
			break;
		case SIM_COMMUNTION_ERR:
			SIM_CON_OK = 0;
			Printf("与GSM模块未通讯成功，请等待\r\n");
			break;
		case SIM_CPIN_ERR:
			SIM_CON_OK = 0;
			Printf("没检测到SIM卡\r\n");
			break;
		default:
			break;
	}
	return res;
}

void gsm_reset(void)
{
//	Printf("GSM模块重启...\r\n");
//	GPIO_ResetBits(GPIOC,GPIO_Pin_9);
//	vTaskDelay(2000/portTICK_RATE_MS); // 拉低2秒可以重新开关机
//	GPIO_SetBits(GPIOC,GPIO_Pin_9);
//	vTaskDelay(2000/portTICK_RATE_MS);
}

u8 SIM800C_CONNECT_SERVER(u8 *servip,u8 *port)
{
	u8 dtbufa[50];
	if(sim800c_send_cmd((u8 *)"AT+CGATT?",(u8 *)"+CGATT: 1",200))
	if(sim800c_send_cmd((u8 *)"AT+CIPMUX=0",(u8 *)"OK",400))	return 3;	// 单路连接
	if(sim800c_send_cmd((u8 *)"AT+CGATT=1",(u8 *)"OK",400))		return 4;	// 将MT附着GPRS业务
	if(sim800c_send_cmd((u8 *)"AT+CIPSHUT",(u8 *)"OK",400))		return 5;	// 关闭移动场景
	if(sim800c_send_cmd((u8 *)"AT+CIPMODE=1",(u8 *)"OK",200))	return 6;	// 透明模式
	// 重传次数5,时间间隔2*100ms,数据字节1024,开启转义序列1,Rxmode=0,Rxsize=1460,Rxtime=50
	if(sim800c_send_cmd((u8 *)"AT+CIPCCFG=5,2,1024,1,0,1460,50",(u8 *)"OK",500))		return 7;
	// 联通接入点名称uninet	移动cmnet
	if(sim800c_send_cmd((u8 *)"AT+CSTT=\"uninet\",\"NULL\",\"NULL\"",(u8 *)"OK",200))	return 8;
	if(sim800c_send_cmd((u8 *)"AT+CIICR",(u8 *)"OK",2000))		return 9;	// 激活移动场景
	if(!sim800c_send_cmd((u8 *)"AT+CIFSR",(u8 *)"ERROR",200))	return 10;	// 获取本地IP
	
	sprintf((char*)dtbufa,"AT+CIPSTART=TCP,%s,%s\r\n",servip,port);			// TCP连接,IP,端口
	
	if(sim800c_send_cmd((u8 *)dtbufa,(u8 *)"CONNECT",200))	return 12;
	
	return 0;
}

//  SIM800C_CONNECT_SERVER  by Wired test 
//u8 SIM800C_CONNECT_SERVER(u8 *servip,u8 *port)
//{
//  struct tcp_pcb *tcppcb;  	//定义一个TCP服务器控制块
//	struct ip_addr rmtipaddr;  	//远端ip地址
//  char inifile[] = "1:cfg.ini";  
//	MSG msg;
//	u8 *tbuf;
// 	u8 key;
//	u8 res=0;		
//	u8 t=0; 
//	u8 connflag=0;		//连接标记	
//	int data_len;
//	static int Count_packet=0;	
//	tcp_client_set_remoteip();//先选择IP 
//	tbuf=(u8_t *)pvPortMalloc(200);
//	if(tbuf==NULL)return ;		//内存申请失败了,直接退出
//	
//	//lwipdev.remoteip[3]=ini_getl("server",	"ip3",	142,inifile);	
//	//printf("lwipdev.remoteip[3]=%d\n",lwipdev.remoteip[3]);
//	
//	
//	sprintf((char*)tbuf,"Local IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//服务器IP
//	sprintf((char*)tbuf,"Remote IP:%d.%d.%d.%d",lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],200);//远端IP
//  //TCP_CLIENT_PORT=ini_getl("PORT",	"port0",	8090,	inifile);	
//	sprintf((char*)tbuf,"Remotewo Port:%d",TCP_CLIENT_PORT);//客户端端口号
//	tcppcb=tcp_new();	//创建一个新的pcb
//	if(tcppcb)			//创建成功
//	{
//		IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],200); 
//		tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);  //连接到目的地址的指定端口上,当连接成功后回调tcp_client_connected()函数
// 	}else res=1;
//	while(res==0)
//	{
//		if(key==4)break;
//     if(tcp_close_flag&(1<<7))
//		{
//		 tcp_close_flag&=~(1<<7);
////     tcp_client_connection_close(tcppcb,0);//关闭TCP Client连接 				
//     tcp_close(tcppcb);
//     vTaskResume(cameraTask_Handler);//恢复camera任务	
//     break;			
//		}				
//		if(tcp_client_flag&1<<6)//是否收到数据?		
//		{			
//			printf("recv data *\n");	
//			tcp_client_flag&=~(1<<6);//标记数据已经被处理了.
//		}		
//		if(tcp_client_flag&1<<5)//是否连接上?
//		{
//			if(connflag==0)
//			{ 								
//				connflag=1;//标记连接了
//			} 
//		}else if(connflag)
//		{
//			connflag=0;	//标记连接断开了
//		} 
//		delay_ms(2);
//		t++;
//		if(t==200)
//		{
//			if(connflag==0&&(tcp_client_flag&1<<5)==0)//未连接上,则尝试重连
//			{ 
//				tcp_client_connection_close(tcppcb,0);//关闭连接
//				tcppcb=tcp_new();	//创建一个新的pcb
//				if(tcppcb)			//创建成功
//				{ 
//					tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);//连接到目的地址的指定端口上,当连接成功后回调tcp_client_connected()函数
//				}
//			}
//			t=0;		
//		}		
//	}
//}
