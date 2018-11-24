
#include "sys.h"
#include "delay.h"
#include "key.h"
#include "usart.h" 
#include "malloc.h"
#include "w25qxx.h"
#include "mass_mal.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "memory.h"
#include "usb_bot.h"
#include "exfuns.h"
#include "ff.h"
#include "iap.h"
#include "stmflash.h"
#include "stdio.h"
#include <string.h>
#include "MD5.h"

char buff_md5[20];
void update_read_flash_test(void);

extern u8 Max_Lun;				//支持的磁盘个数,0表示1个,1表示2个.

 int main(void)
 {
	u8 res = 0;
	u8 key = 0;
	unsigned char digest_test[16]={0};
  char *md5_boot_txt="1:file_md5_boot.txt";		 
	delay_init();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	KEY_Init();
	uart_init(115200);
	W25QXX_Init();
	my_mem_init(SRAMIN);			//初始化内部内存池
	exfuns_init();						//为fatfs相关变量申请内存
	res = f_mount(fs[1],"1:",1);	//挂载FLASH.
	if(res == 0X0D)res = f_mkfs("1:",0,4096);//格式化FLASH
	delay_ms(1800);
	
	
	
	

#if 1
	f_open(file, "1:RESET", FA_READ|FA_OPEN_ALWAYS);
	f_close (file);
	key = KEY_Scan(0);
    update_read_flash_test();//read update Flag
	  printf("buff_md5=%s\n",buff_md5);
	  if(strcmp((char*)buff_md5,"md5updateflag")==0)
	  {					
			mf_unlink((u8*)md5_boot_txt);
	    printf("update  post.bin  app loading....\n");
      if(f_open(file, "1:post.bin", FA_READ|FA_OPEN_EXISTING) == 0)			
	    {
			 iap_write_appbin(FLASH_APP_ADDR,file);
			 iap_load_app(FLASH_APP_ADDR);	
		  }			 
	  }
		else
		{
		 if(f_open(file, "1:SmartCabinet.bin", FA_READ|FA_OPEN_EXISTING) == 0)
		 {
		 printf("original SmartCabinet.bin  app loading....\n");	
		 iap_write_appbin(FLASH_APP_ADDR,file);
		 printf("iap_write_appbin over....\n");	
		 iap_load_app(FLASH_APP_ADDR); 
		 }			
		}
	
	
#endif
	
	USB_Port_Set(0); 	//USB先断开
	delay_ms(700);
	USB_Port_Set(1);	//USB再次连接 
	Data_Buffer = mymalloc(SRAMIN, BULK_MAX_PACKET_SIZE*2*4*4);	//为USB数据缓存区申请内存
	Bulk_Data_Buff = mymalloc(SRAMIN, BULK_MAX_PACKET_SIZE);	//申请内存
 	//USB配置
 	USB_Interrupts_Config();    
 	Set_USBClock();
 	USB_Init();	    
	delay_ms(1800);	   	    
	while(1)
	{
	};
}
 

/*
 * 函数名：void post_read_flash_test()
 * 描述  读更新标志
 * 输入  ：无
 * 输出  ：无	  
 */
void update_read_flash_test(void)
{
   FIL *file4;//	  
	 u8 res,res2,res3,i,n4;
	 {	
		res = f_open(file, "1:file_md5_boot.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
		printf("file->fsize=%ld\n",file->fsize);
		if(res!=FR_OK)
		{ 
		 printf("open file_md5_boot.txt error!\r\n");
		}				
		res3= f_read(file, buff_md5, file->fsize, &br);
		f_close(file);				
	}				
}
