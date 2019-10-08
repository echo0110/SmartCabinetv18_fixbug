#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "sys.h"
#include "usart.h"
#include "dma.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "message.h"
#include "MQTTGPRSClient.h"
#include "task.h"
#include "transport.h"

#define MQTTWAITTIME	6000	// 6000=1min

extern xQueueHandle MainTaskQueue;

u8 USART2_RX_BUF[USART2_MAX_LEN], USART2_TX_BUF[USART2_MAX_LEN];
u16 USART2_RX_STA;

void USART2_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);

	USART_DeInit(USART2);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;					// PA2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;					// PA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART2, &USART_InitStructure);
	
	// USART2空闲中断设置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_SYS_INTERRUPT_PRIORITY + 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
	USART_Cmd(USART2, ENABLE);
	
	// DMA发送中断设置
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;  
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
//	DMA_ITConfig(DMA1_Channel7,DMA1_IT_TC7,ENABLE);				// DMA发送完毕中断
	USART_DMACmd(USART2,USART_DMAReq_Tx,ENABLE);				// 使能串口2的DMA发送
	UART_DMA_Config(DMA1_Channel7,(u32)&USART2->DR,(u32)USART2_TX_BUF, DMA_DIR_PeripheralDST);
	
	USART_DMACmd(USART2,USART_DMAReq_Rx,ENABLE);				// 使能串口2的DMA接收
	UART_DMA_Config(DMA1_Channel6,(u32)&USART2->DR,(u32)USART2_RX_BUF, DMA_DIR_PeripheralSRC);
	
	UART_DMA_Enable(DMA1_Channel6, USART2_MAX_LEN);				// 开始接收
//	UART_DMA_Enable(DMA1_Channel7, USART2_MAX_LEN);				// 开始发送
	
	USART2_RX_STA = 0;
}

int sendMQTTData(u8* buf, int buflen, u16 waittime)
{
	int res = 0;
	
	USART2_RX_STA = 0;
	reset();

	sendMQTT(buf, buflen, USART2_TX_BUF);
	while(DMA1_Channel7->CNDTR != 0);				// 等待通道7传输完成
	UART_DMA_Enable(DMA1_Channel7, buflen);			// 恢复发送
	UART_DMA_Enable(DMA1_Channel6, USART2_MAX_LEN);	// 恢复接收
	if(waittime)
	{
		while(--waittime)
		{
			vTaskDelay(20);
			if(USART2_RX_STA & 0X8000)break;
		}
		if(waittime == 0)res = -1;
	}
	USART2_RX_STA &= 0xff;
	reset();
	
	return res;
}

int getMQTTData(u8* buf, int count)
{
	//getMQTT(USART2_RX_BUF, count, buf);
	transport_getdata(buf,count);
	
	return count;
}

void USART2_IRQHandler(void)
{
//	BaseType_t xSwitchRequired = pdFALSE;
//	MSG msg;
	
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)		// 空闲中断（接收完成）
	{
		USART_ReceiveData(USART2);
		USART2_RX_STA = USART2_MAX_LEN - DMA_GetCurrDataCounter(DMA1_Channel6);
		USART2_RX_STA |= 0x8000;
//		if(MainTaskQueue != NULL)msg.Msg = MSG_UART2_RX_FRAME;
		USART_ClearITPendingBit(USART2, USART_IT_IDLE);         // 清除中断标志
	}
	
//	portEND_SWITCHING_ISR(xSwitchRequired);
}

void USART2_vspf(char* fmt,...)
{
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)USART2_TX_BUF,fmt,ap);
	va_end(ap);
	while(DMA1_Channel7->CNDTR!=0);
	UART_DMA_Enable(DMA1_Channel7,strlen((const char*)USART2_TX_BUF));
}
