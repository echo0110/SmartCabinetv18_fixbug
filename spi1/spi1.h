
#ifndef __SPI_H
#define __SPI_H
#include "stm32f10x.h"


// SPI总线速度设置  
 				  
void SPI1_Init(void);			 //初始化SPI口
void SPI1_SetSpeed(u8 SpeedSet); //设置SPI速度   
u8 SPI1_ReadWriteByte(u8 TxData);//SPI总线读写一个字节

extern u8  SPI1_ReadWriteByte_test(u8 TxData);
		 
#endif


/*----------------------德飞莱 技术论坛：www.doflye.net--------------------------*/
