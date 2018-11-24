#ifndef __STMFLASH_H__
#define __STMFLASH_H__

#include "stm32f10x.h"

#define  STM32_FLASH_SIZE   512
#define  STM32_FLASH_BASE   0X08000000

#if STM32_FLASH_SIZE < 256
#define STM_SECTOR_SIZE 1024            //×Ö½Ú
#else 
#define STM_SECTOR_SIZE	2048
#endif		 



u16   FlashReadHalfWord(u32 addr);
void  FlashRead(u32 addr,u16 *pbuffer,u16 num);
void  FlashWriteNoCheck(u32 addr,u16 *pbuffer,u16 num);
void  FlashWrite(u32 addr,u16 *pbuffer,u16 num);  



#endif






