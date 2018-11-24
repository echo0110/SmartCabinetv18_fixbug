
#include "stmflash.h"

u16 FlashBuffer[STM_SECTOR_SIZE / 2];   //最多是2K字节




//读取指定地址的16位数据
u16  FlashReadHalfWord(u32 addr)
{
	return *(vu16 *)addr;
}


void  FlashRead(u32 addr,u16 *pbuffer,u16 num)   	
{
	u16 i;
	for(i = 0;i < num;i++)
	{
		*pbuffer++ = FlashReadHalfWord(addr);//读取2个字节.
		addr += 2;                           //偏移2个字节.	
	}
}



void  FlashWriteNoCheck(u32 addr,u16 *pbuffer,u16 num)
{
	u16 i;
	for(i = 0;i < num;i++)
	{
		FLASH_ProgramHalfWord (addr,pbuffer[i]);
		addr += 2;	
	}
}



//带擦除的写
void  FlashWrite(u32 addr,u16 *pbuffer,u16 num)
{
	u16  num_sector;  //在第几页
	u16  offset_sector;  //在一页的偏移量
	u16  num_last;    //剩下的半字
	u16  i;
	num_sector =  (addr - STM32_FLASH_BASE) / STM_SECTOR_SIZE;	   //获取第几页
	offset_sector =  ((addr - STM32_FLASH_BASE) % STM_SECTOR_SIZE) / 2;  //获取偏移量，16个字节为单位
	num_last = 	STM_SECTOR_SIZE / 2 - offset_sector;                     //求出剩下的半字
	FLASH_Unlock();                              //解锁
	if(num <= num_last)  num_last = num;		 //可以一次写完
	for(;;)										 //开始不停的写
	{
		FlashRead(num_sector * STM_SECTOR_SIZE + STM32_FLASH_BASE,FlashBuffer,STM_SECTOR_SIZE / 2);  //读出一页
		for(i = 0;i < num_last;i++)
		{
			if(FlashBuffer[offset_sector + i] != 0xffff) break;	
		}
		if(i < num_last)						//需要擦除
		{
			FLASH_ErasePage (num_sector * STM_SECTOR_SIZE + STM32_FLASH_BASE);                      //擦除
			for(i = 0;i < num_last;i++)
			{
				FlashBuffer[offset_sector + i] = pbuffer[i];	               //把要写入的数据填入数组
			}
			FlashWriteNoCheck(num_sector * STM_SECTOR_SIZE + STM32_FLASH_BASE,FlashBuffer,STM_SECTOR_SIZE / 2);               //写整个扇区		
		}else								    //不需要擦除
		{
			FlashWriteNoCheck(addr,pbuffer,num_last);	//直接将要写入的数据写入flash
		}
		if(num_last == num)  break;                     //写完后就跳出
		else                                            //没有写完继续
		{
			num_sector += 1;    //页加一
			offset_sector = 0;  //偏移为0
			pbuffer += num_last;
			addr += num_last * 2;
			num -= num_last;    //调整要写入的数量
			if(num > (STM_SECTOR_SIZE / 2))  num_last = STM_SECTOR_SIZE / 2;
			else  num_last = num;   //调整num_last		
		}		
	}
	FLASH_Lock();             //上锁

}













