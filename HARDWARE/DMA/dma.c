#include "dma.h"

void UART_DMA_Config(DMA_Channel_TypeDef *DMA_CHx, u32 cpar, u32 cmar, u32 cdir)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1|RCC_AHBPeriph_DMA2, ENABLE);
	DMA_DeInit(DMA_CHx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = cpar;
	DMA_InitStructure.DMA_MemoryBaseAddr = cmar;
	DMA_InitStructure.DMA_DIR = cdir;//DMA_DIR_PeripheralDST;				// 数据传输方向
	DMA_InitStructure.DMA_BufferSize = 0;									// UART_DMA_Enable()函数设置长度
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		// 外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					// 内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	// 外设数据宽度Byte
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			// 内存数据宽度Byte
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							// 非循环模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;					// CHx拥有中优先级
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							// 非内存到内存传输
	DMA_Init(DMA_CHx, &DMA_InitStructure);
}

void UART_DMA_Enable(DMA_Channel_TypeDef *DMA_CHx, u16 len)
{
	DMA_Cmd(DMA_CHx, DISABLE );
	DMA_SetCurrDataCounter(DMA_CHx,len);
	DMA_Cmd(DMA_CHx, ENABLE);
}
