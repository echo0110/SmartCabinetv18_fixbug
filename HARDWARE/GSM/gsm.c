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
	
	if(sim800c_send_cmd((u8 *)"AT+CGATT?",(u8 *)"+CGATT: 1",100))
	if(sim800c_send_cmd((u8 *)"AT+CIPMUX=0",(u8 *)"OK",200))	return 3;	// 单路连接
	if(sim800c_send_cmd((u8 *)"AT+CGATT=1",(u8 *)"OK",100))		return 4;	// 将MT附着GPRS业务
	if(sim800c_send_cmd((u8 *)"AT+CIPSHUT",(u8 *)"OK",400))		return 5;	// 关闭移动场景
	if(sim800c_send_cmd((u8 *)"AT+CIPMODE=1",(u8 *)"OK",100))	return 6;	// 透明模式
	// 重传次数5,时间间隔2*100ms,数据字节1024,开启转义序列1,Rxmode=0,Rxsize=1460,Rxtime=50
	if(sim800c_send_cmd((u8 *)"AT+CIPCCFG=5,2,1024,1,0,1460,50",(u8 *)"OK",500))		return 7;
	// 联通接入点名称uninet	移动cmnet
	if(sim800c_send_cmd((u8 *)"AT+CSTT=\"uninet\",\"NULL\",\"NULL\"",(u8 *)"OK",200))	return 8;
	if(sim800c_send_cmd((u8 *)"AT+CIICR",(u8 *)"OK",900))		return 9;	// 激活移动场景
	if(!sim800c_send_cmd((u8 *)"AT+CIFSR",(u8 *)"ERROR",200))	return 10;	// 获取本地IP
	
	sprintf((char*)dtbufa,"AT+CIPSTART=TCP,%s,%s\r\n",servip,port);			// TCP连接,IP,端口
	
	if(sim800c_send_cmd((u8 *)dtbufa,(u8 *)"CONNECT",200))	return 12;
	
	return 0;
}
