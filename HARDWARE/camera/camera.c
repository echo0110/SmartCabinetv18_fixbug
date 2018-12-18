#include "camera.h"
#include "MQTTPacket.h"
#include "StackTrace.h"
#include "stm32f10x_it.h" 

#include <string.h>
#include "mqtt.h"
#include "usart.h"
#include "tcp.h"
#include "MQTTPacket.h"
#include "MQTTGPRSClient.h"
#include "dma.h"
#include "mqtt.h"
#include "task.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "udp_demo.h" 
#include "netif/etharp.h"
#include "httpd.h"
#include "mqtt.h"
#include "exfuns.h"
#include "ff.h"
#include "usart.h"
#include "tcp_client_demo.h" 
#include "message.h"
#include "delay.h"
#include "rtc.h"
#include "fattester.h"
#include "sensor.h"
#include "usart.h"
#include "print.h"
#include "tcp_server_demo.h"
#include "semphr.h"
#include "iwdg.h"


#include "diskio.h"
#include "message.h"
#include "ff.h"
#include "mass_mal.h"
#include "diskio.h"	
#include "exfuns.h"
#include "usb_prop.h"

#include "hw_config.h"
#include "memory.h"
#include "usb_init.h"




//void* BinarySemaphore_mac;	//二值信号量句柄  数据接收信号量
//void* BinarySemaphore_mac_config;
void* BinarySemaphore_camera;//摄像头抓拍信号量
void* BinarySemaphore_camera_Sync;
void* BinarySemaphore_camera_response;
void* BinarySemaphore_response;
void* BinarySemaphore_camera_data_response;
void* BinarySemaphore_photo_command;
void* test_trap_signal;
void* test_tcp_signal;
void* timestamp_signal;//时间戳信号量

void* ping_signal;

void* close_tcp_server;
void* open_tcp_server;

void* cameraTask_Handler;
void* WDG_Sem;//看门狗检测

void* tcp_command;
void* mqtt_command;

/* 互斥信号量  for sd write*/
SemaphoreHandle_t sd_Sem = NULL;

TaskHandle_t ServerTask_Handler;

xQueueHandle USB_Queue;

extern xQueueHandle MainTaskQueue;
extern TaskHandle_t TickTask_Handler;
extern TaskHandle_t MainTask_Handler;
extern TaskHandle_t SDTask_Handler;

extern TaskHandle_t PingTask_Handler;


u8 camera_Sync_command[6]={0xAA,0x0D,0x00,0x00,0x00,0x00}; //摄像头同步命令
u8 camera_response_command[6]={0xAA,0x0E,0x0D,0x00,0x00,0x00}; //摄像头同步命令

u8 photo_command_array[6]={0xAA,0x04,0x05,0x00,0x00,0x00}; //摄像头拍照指令

u8 get_camera_data_command[6]={0xAA,0x0E,0x00,0x00,0x00,0x00}; //取图像数据命令

u8 end_command[6]={0xAA,0x0E,0x00,0x00,0xF0,0xF0}; //结束命令

xQueueHandle MsgQueue;  

xQueueHandle Msg_response; 

extern TaskHandle_t USBTask_Handler;
extern TaskHandle_t MqttTask_Handler;

//char  temp_str[17];
char  temp_str[50];

/*
 * 函数名：void camera_task(void *pvParameters)
 * 描述  ：摄像头命令
 * 输入  ：无
 * 输出  ：无	
 */

void camera_task(void *pvParameters)
{
	u8 ret1,res;
	int  camera_data_len,last_camera_data_len;
	int  count_packet;
	static int count_camera;
	u16 Count=0;
	static int count_tcp;
	Task_MSG msg;
  
	while(1)
	{	
	 printf("camera task\n");
	 UART_Send(camera_Sync_command,6);	
	 memset(DMA_Rece_Buf,0,512);	 	
	 if(xSemaphoreTake(BinarySemaphore_photo_command, portMAX_DELAY)==pdTRUE)
	 {
		 static u8 i;	 
		 printf("the door have been opend\n");			
		 
		 vTaskSuspend(SDTask_Handler);		 
		 vTaskSuspend(MqttTask_Handler);		 
		 //vTaskSuspend(USBTask_Handler);
		 vTaskSuspend(MainTask_Handler);
		 
		 vTaskSuspend(PingTask_Handler);
		 
		 portDISABLE_INTERRUPTS(); //关中断
		     		 
		 USB_Port_Set(0);
		 
     res=Camera_init(); //初始化成功	
		 for(i=0;i<3;i++)
		 {		 
			UART_Send(photo_command_array,6);	//发送拍照命令		     	
			while(1)
			{
			 if(xSemaphoreTake(BinarySemaphore_camera_data_response, portMAX_DELAY)) //拍照成功
			 {	            				
				 Uart4_Over=0;		
				 camera_data_len=DMA_Rece_Buf[5]*65536+DMA_Rece_Buf[4]*256+DMA_Rece_Buf[3];//得到数据长度
				 printf("camera_data_len=%d\n",camera_data_len);
				 last_camera_data_len=camera_data_len%512;
				 printf("[*]last_camera_data_len=%d\n",last_camera_data_len);
				
				 if(last_camera_data_len>0)
				 count_packet=camera_data_len/512+1;
				 else  count_packet=camera_data_len/512;				 
				 break;						 
			 }				
			} 		
			UART_Send(get_camera_data_command,6);	//取图像数据命令
      printf("count_packet=%d\n",count_packet);				 
			while(count_packet)   //地址是不是有问题  每次发送命令的地址
			{      			
			 if(Uart4_Over==1)
			 {
				  Uart4_Over=0;
					if(Camera_check())
					{
						int data_len;										
						if(Count==0)
						{	
							timestamp_flag=timestamp_flag|(1<<7);//先发时间戳
							vTaskSuspend(cameraTask_Handler);						
						}			
						data_len=(DMA_Rece_Buf[3]<<8)|(DMA_Rece_Buf[2]&0x00ff);
						printf("data_len=%d\n",data_len);
						tcp_client_flag=tcp_client_flag|(1<<7);//发送图像数据					
						vTaskSuspend(cameraTask_Handler);						
					  count_packet--;	         	
					}
					else
					{
						Count=Count-1;
            if(Count>50)
						{
						 break;
						}							
					}					 				 
				 Count++;
				 printf("Count=%u\n",Count);
				 get_camera_data_command[4]=Count&0xff;//0x01;//j;//低位
				 get_camera_data_command[5]=(Count>>8)&0xff;//0x00;//j>>8;//高位	
         if(count_packet>0)	
				 {
				  UART_Send(get_camera_data_command,6);	
				 }
         else
				 {
				  UART_Send(end_command,6);	
					get_camera_data_command[4]=0x00;
					get_camera_data_command[5]=0x00;
				 }					 				
			 }					 
			}			
     Count=0;	  			
		 tcp_close_flag=tcp_close_flag|(1<<7);
			
		 vTaskSuspend(cameraTask_Handler);//等关闭完TCP连接再恢复 camera下面执行		
		 Uart4_Over=0;		
     memset(DMA_Rece_Buf,0,512);
		}
		 
		
	  portENABLE_INTERRUPTS ();//开中断
		
    vTaskResume(SDTask_Handler);  
    vTaskResume(MqttTask_Handler);		 
		vTaskResume(MainTask_Handler);
		vTaskResume(PingTask_Handler);
		
		 USB_Port_Set(1);
		 USB_Init();		
  }		
 }
}


/*
 * 函数名：u8 Camera_init(void)
 * 描述  ：
 * 输入  ：无
 * 输出  ：无	
 */
u8 Camera_init(void)
{
	static int camera_count=0;
	static int count=0;	
	while(1)
	{
		Uart4_Over=0;	
    UART_Send(camera_Sync_command,6);		
		if(xSemaphoreTake(BinarySemaphore_camera_Sync, portMAX_DELAY)) 
		{
		 memset(DMA_Rece_Buf,0,50);
		 printf("Sync sucess\r\n");
		 return 1;//同步成功
		}	
    else
	  {
	   UART_Send(camera_Sync_command,6);
		 camera_count++;
	  }	
		if(camera_count>50)
	  {
		 printf("Sync erro\n");
	   return 0;//同步失败
	  }
	} 
}
/*
 * 函数名：u8 Camera_check(void)
 * 描述  ：图像数据校验
 * 输入  ：无
 * 输出  ：无	
 */
u8 Camera_check(void)
{
	int i;
	u8 sum=0; 
	for(i=0;i<Uart4_Rec_Cnt-2;i++)
	{
	sum=DMA_Rece_Buf[i]+sum;
	}
	if(sum==DMA_Rece_Buf[Uart4_Rec_Cnt-2])
	{
	return 1;//校验正确
	}
	else
	{
	return 0;//校验错误
	}  
}

/*
 * 函数名：void UART_Send(u8 *str,u8 len3)
 * 描述  ：发送同步摄像头命令
 * 输入  ：无
 * 输出  ：无	
 */
void UART_Send(u8 *str,u8 len3)
{
	 u8 i;
	 //while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);	
   for(i=0;i<len3;i++)
	{
  	 USART_SendData(UART4, *(str+i));
	   while(USART_GetFlagStatus(UART4,USART_FLAG_TC)==RESET);  //while这个时间等的很久数据还没发出去  已经是下一个中断了
	}
}


/*
 * 函数名：u8 camera_write_sd_test(void)
 * 描述  ：图像数据写入SD卡
 * 输入  ：无
 * 输出  ：无	
 */
void write_sd_test(char j)
{ 
	u8 res,i,n4=10;
	FIL	*file_camera;
	char test_buf[17];
	char test_read_buf[10];
	u32 timestamp_buf[10];
	char  str[17];
	char calendar_buf[50];
	int sd_data_len;
	u8 str_len;
	sd_data_len=(DMA_Rece_Buf[3]<<8)|(DMA_Rece_Buf[2]&0x00ff);	
	if(j==0)
	{		
	 time_to_sd_path(calendar_buf);
	 str_len=strlen(calendar_buf);
   memcpy(temp_str,calendar_buf,str_len);			
	}
  if(j==10)
	{	
		file_camera=(FIL*)pvPortMalloc(1000);//申请内存
		{	
			printf("temp_str=%s\n",temp_str);
			res = f_open(file_camera,temp_str,FA_OPEN_ALWAYS|FA_WRITE|FA_READ);		
			if(res!=FR_OK)
			{ 
			 printf("open test3 error!\r\n");
			}
			res=f_lseek(file_camera, file_camera->fsize);			
			n4=f_write(file_camera, &DMA_Rece_Buf[4],sd_data_len,&br);//
			vPortFree(file_camera);
			f_close(file_camera);	
			memset(DMA_Rece_Buf,0,512);		
		}	
	}	
}
/*
 * 函数名：image_rename(void)
 * 描述  ：图片重命名
 * 输入  ：无
 * 输出  ：无	
 */
void image_rename(void)
{
	u32 timestamp_buf[10];
	char  new_image[40];	
	timestamp_buf[0]=10;//RTC_GetTime(NULL);	
	snprintf(new_image,18, "%d:%d.%s",0,timestamp_buf[0],"jpeg");
	f_rename("0:camera80.jpeg",new_image);			
}

/*
 * 函数名：char* time_to_timestamp(void)
 * 描述  ：本地时间生成时间戳函数
 * 输入  ：无
 * 输出  ：无	
 */
char* time_to_timestamp(void)
{
	static u32 RtcCounter = 0;
	static u32 RtcCounter2 = 0;
	Rtc_Time RTC_DateStruct;
	struct _date_t  RTC_date_t;
	char timestamp_buf[20];
	char test_buf[100];
	char *timestamp;
	timestamp =timestamp_buf;
	RtcCounter=RTC_GetTime(&RTC_DateStruct);
  printf("RtcCounter=%d",RtcCounter);
  RtcCounter2=_mktime(&RTC_DateStruct);
	return timestamp;
}


/*
 * 函数名：void time_to_sd_path(void)
 * 描述  ：时间转化为 sd图片路径
 * 输入  ：无
 * 输出  ：无	
 */
void time_to_sd_path(char *buff)
{
	char test_buf[100];
	RTC_Get();//更新时间	
	sprintf(buff, "%d:%d_%d_%d_%d_%d_%d.%s",0,calendar.w_year,calendar.w_month,calendar.w_date,
	calendar.hour,calendar.min,calendar.sec,"jpeg");	
}
/*
 * 函数名：USB_Close(void)
 * 描述  ：
 * 输入  ：无
 * 输出  ：无	
 */
void USB_Close(void)
{
	xEventGroupSetBits(xCreatedEventGroup,USB_ClOSE_BIT);   	
}

/*
 * 函数名：USB_Open(void)
 * 描述  ：
 * 输入  ：无
 * 输出  ：无	
 */
void USB_Open(void)
{
	xEventGroupClearBits(xCreatedEventGroup, USB_ClOSE_BIT); 	
}

