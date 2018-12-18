
#include "delay.h"
#include "usart.h"
#include "stdarg.h"	 	 
#include "stdio.h"	 	 
#include "string.h"	 
//#include "timer.h"
#include "adc.h"
//#include "dma.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "etharp.h"
#include "camera.h"

//#include "gps.h"

u16 Uart4_Rec_Cnt=0;             //本帧数据长度	
u8  Uart4_Over=0;//串口1接收一帧完成标志位

//u8 USART3_RX_BUF[USART3_MAX_LEN];
//u8 USART3_TX_BUF[USART3_MAX_LEN];
// [15]-接收完成未处理标志
// [14-0]-接收字节数
//u16 USART3_RX_STA;

u8  DMA_Rece_Buf[DMA_Rec_Len];	   //DMA接收串口数据缓冲区

void uart4_init(u32 bound);

void UART4_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

	USART_DeInit(UART4);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;					// PC10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;					// PC11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART4, &USART_InitStructure);
	
	USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);				// 空闲中断
	
	USART_Cmd(UART4, ENABLE);
	USART_ClearFlag(UART4, USART_FLAG_TC);        // ???(???)
               
	
  //相应的DMA配置
	DMA_DeInit(DMA2_Channel3);   //将DMA的通道5寄存器重设为缺省值  串口1对应的是DMA通道5
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&UART4->DR;//0x40004C04;//(u32)&UART4->DR;  //DMA外设ADC基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)DMA_Rece_Buf;  //DMA内存基地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  //数据传输方向，从外设读取发送到内存
	DMA_InitStructure.DMA_BufferSize = DMA_Rec_Len;  //DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //数据宽度为8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //数据宽度为8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//DMA_Mode_Normal;  //工作在正常缓存模式
	DMA_InitStructure.DMA_Priority =DMA_Priority_High;//DMA_Priority_VeryHigh;//DMA_Priority_Medium; //DMA通道 x拥有中优先级 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMA通道x没有设置为内存到内存传输
	DMA_Init(DMA2_Channel3, &DMA_InitStructure);  //根据DMA_InitStruct中指定的参数初始化DMA的通道USART1_Tx_DMA_Channel所标识的寄存器
	

  DMA_ITConfig(DMA2_Channel3, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA2_Channel3, DMA_IT_TE, ENABLE);
	USART_DMACmd(UART4,USART_DMAReq_Rx,ENABLE);   //使能串口1 DMA接收
	DMA_Cmd(DMA2_Channel3, ENABLE);  //正式驱动DMA传输
	
	
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =7;//3;//7;//1; //7;//1;//3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*重新恢复DMA指针*/
  void MYDMA_Enable(DMA_Channel_TypeDef*DMA_CHx)
	{ 
		DMA_Cmd(DMA_CHx, DISABLE );  //关闭USART1 TX DMA1 所指示的通道      
		DMA_SetCurrDataCounter(DMA_CHx,DMA_Rec_Len);//DMA通道的DMA缓存的大小
		DMA_Cmd(DMA_CHx, ENABLE);  //使能USART1 TX DMA1 所指示的通道 
	} 


void UART4_IRQHandler(void)                	//串口1中断服务程序
{
	u8 Res;
	u8 Sync;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;//
	if(USART_GetITStatus(UART4, USART_IT_IDLE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		Res =USART_ReceiveData(UART4);	//读取接收到的数据
		Uart4_Rec_Cnt = DMA_Rec_Len-DMA_GetCurrDataCounter(DMA2_Channel3);	//算出接本帧数据长度
		//printf("Uart4_Rec_Cnt=%d\n",Uart4_Rec_Cnt);
		Uart4_Over=1;
		//printf("Uart4_Over=%d\n",Uart4_Over);
		USART_ClearITPendingBit(UART4, USART_IT_IDLE); 
		MYDMA_Enable(DMA2_Channel3);                   //恢复DMA指针，等待下一次的接收
		if((DMA_Rece_Buf[0]==0xAA&&DMA_Rece_Buf[1]==0x0E)&&(DMA_Rece_Buf[2]==0x0D)&&(DMA_Rece_Buf[4]==0x00&&DMA_Rece_Buf[5]==0x00) \
		&&DMA_Rece_Buf[6]==0xAA&DMA_Rece_Buf[7]==0x0D&DMA_Rece_Buf[8]==0x00&DMA_Rece_Buf[9]==0x00&DMA_Rece_Buf[10]==0x00&DMA_Rece_Buf[11]==0x00)  //初始化同步完成
		{
		 Sync=xSemaphoreGiveFromISR(BinarySemaphore_camera_Sync, &xHigherPriorityTaskWoken);		
		}
		if((DMA_Rece_Buf[0]==0xAA&&DMA_Rece_Buf[1]==0x0A)&&(DMA_Rece_Buf[2]==0x05))//拍照成功
		{		
		 xSemaphoreGiveFromISR(BinarySemaphore_camera_data_response, &xHigherPriorityTaskWoken);	
		}
  } 
}


#if 0
void UART4_Init(u32 bound){
   //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //使能GPIOC时钟
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//使能USART1时钟
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
	
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //使能GPIOC时钟
	//USART_DeInit(UART4);
 
	//串口1对应引脚复用映射
//	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); //GPIOA9复用为USART1
//	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4); //GPIOA10复用为USART1
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;					// PC10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;					// PC11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//GPIO_Mode_IPU;// GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
  //USART1端口配置
 

   //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  USART_Init(UART4, &USART_InitStructure); //初始化串口1
	
  USART_Cmd(UART4, ENABLE);  //使能串口4 
	USART_ClearFlag(UART4,USART_FLAG_TC);
	/*打开空闲中断*/
	USART_ITConfig(UART4,USART_IT_IDLE,ENABLE);
	/*打开接收中断*/
	USART_ITConfig(UART4,USART_IT_RXNE,ENABLE);
	
	
	//USART_ClearFlag(USART1, USART_FLAG_TC);
	
#if 1
	//USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=15;//7;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

#endif	
}
#endif

