
#include "iap.h"
#include "sys.h"
#include "stmflash.h"

iapfun  jump2app;

//跳转到应用程序段
//appxaddr:用户代码起始地址.
void iap_load_app(u32 appxaddr)
{
	if(((*(vu32*)appxaddr)&0x2FFE0000)==0x20000000)	//检查栈顶地址是否合法.
	{
		jump2app = (iapfun)*(vu32*)(appxaddr+4);		//用户代码区第二个字为程序开始地址(复位地址)		
		MSR_MSP(*(vu32*)appxaddr);					    //初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
		jump2app();									    //跳转到APP.
	}
}


//appxaddr:应用程序起始地址
//appbuf:应用程序code
//appsize:应用程序大小（字节）
void iap_write_appbin(u32 appxaddr, FIL *fp)
{
	u32 APP_Sector = 0;
	u16 APP_Byte = 0;
	u16 ReadNum;
	u32 i = 0;
	u16 j = 0;
	u8 ReadAppBuffer[512];
	u16 ChangeBuffer[256];
	
	APP_Sector = fp->fsize/512;
	APP_Byte = fp->fsize%512;
	for(i=0; i<APP_Sector; i++)
	{
		f_read(fp, ReadAppBuffer, 512, (UINT *)&ReadNum);
		for(j=0; j<256; j++)ChangeBuffer[j] = (ReadAppBuffer[j*2+1]<<8) + ReadAppBuffer[j*2];
		FlashWrite(appxaddr+i*512, ChangeBuffer, 256);	 
	}
	if(APP_Byte != 0)
	{
		f_read(fp, ReadAppBuffer, APP_Byte, (UINT *)&ReadNum);
		for(j=0; j<(APP_Byte/2); j++)ChangeBuffer[j] = (ReadAppBuffer[j*2+1]<<8) + ReadAppBuffer[j*2];
		FlashWrite(appxaddr+i*512, ChangeBuffer, APP_Byte/2);
	}
}
