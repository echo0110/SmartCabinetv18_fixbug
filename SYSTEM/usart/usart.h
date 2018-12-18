#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 

#define USART_TX_BUF_LENGTH		80
#define USART1_MAX_LEN		200
#define USART2_MAX_LEN		200
#define USART3_MAX_LEN		250		// GNRMC最大帧70B
//#define USART4_MAX_LEN		1024
#define UART4_MAX_LEN		1024

#define DMA_Rec_Len  			512//255  	//定义最大接收字节数 


extern u8  Usart1_Over;//串口1接收一帧完成标志位


extern u8  USART1_RX_BUF[USART1_MAX_LEN];
extern u8  USART2_RX_BUF[USART2_MAX_LEN];
//extern u8  USART3_RX_BUF[USART3_MAX_LEN];
extern u8  DMA_Rece_Buf[DMA_Rec_Len];	   //DMA接收串口数据缓冲区

extern u16 USART1_RX_STA;
extern u16 USART2_RX_STA;
//extern u16 USART3_RX_STA;

//extern  u16 Usart4_Rec_Cnt;             //本帧数据长度	
extern  u8  Uart4_Over;//串口1接收一帧完成标志位
extern u16 Uart4_Rec_Cnt;             //本帧数据长度

extern u8 USART2_RX_REC_ATCOMMAD;

struct _Usart_t
{
	volatile u8 TxWr;					// 发送缓冲区写入指针
	volatile u8 TxRd;					// 发送缓冲区读出指针
	volatile u8	TxBufLen;				// 发送缓冲区中数据的长度
	u16 TxBuffer[USART_TX_BUF_LENGTH];	// 指向发送数据缓冲区的指针
};

void USART1_Init(u32 bound);
void SerialPutChar(u16 cOutChar);
void SerialPutString(const u16 *pcString, int usStringLength);

void USART2_Init(u32 bound);
void USART2_vspf(char* fmt,...);
int sendMQTTData(u8* buf, int buflen, u16 waittime);
int getMQTTData(u8* buf, int count);

extern void USART3_Init(u32 bound);
extern void UART4_Init(u32 bound);

extern u8  Usart1_Over;//串口1接收一帧完成标志位

#endif
