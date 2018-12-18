#include "iwdg.h"
#include "STM32F10x_IWDG.h"
//#include "portmacro.h"
#include "FreeRTOS.h"
#include "event_groups.h"

#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "message.h"
#include "camera.h"
#include "delay.h"

static u8 Flag_camera=10;
static int count;
static u8 Flag_wdg=1;

/* 定义事件组 */
EventGroupHandle_t xCreatedEventGroup;

extern TaskHandle_t USBTask_Handler;
extern TaskHandle_t MqttTask_Handler;



//喂独立看门狗
void IWDG_Feed(void)
{   
 	IWDG_ReloadCounter();	 					   
}

/**
  * @brief  看门狗任务  
  * @param  argument
  * @retval None
  */
void vTaskWDG(void * argument)
{
	static TickType_t	xTicksToWait = 100 / portTICK_PERIOD_MS*50; /* 最大延迟5s */
	EventBits_t			uxBits;
	
/*
	0:Running
	1:Ready 就绪
	2:block 阻塞
*/	
	if(xCreatedEventGroup == NULL)
	{
	 printf("xCreatedEventGroup failed\n");		/* 没有创建成功失败机制 */
	 return;
	}
  
	while(1)  //检测MQTT连接服务器
	{
//	 /* 等待所有的任务发来事件 */
	uxBits = xEventGroupWaitBits(xCreatedEventGroup, /* 事件标志组句柄 */
										 TASK_BIT_MQTT,		/* 等待WDG_BIT_TASK_ALL被设置 */
										 pdTRUE,				 /* 退出前TASK_BIT_ALL被清除，这里是TASK_BIT_ALL都被设置才表示“退出”*/
										 pdTRUE,				 /* 设置为pdTRUE表示等待TASK_BIT_ALL都被设置*/
										 xTicksToWait);			 /* 等待延迟时间 */
	 printf("uxBits_MQTT=%04x\n",uxBits);	
	 if(((uxBits&0xff) & (1<<5)))
	 {
	  break;
	 }
	 IWDG_Feed();
	}
	
  while(1)
	{
		/* 等待所有的任务发来事件 */
		uxBits = xEventGroupWaitBits(xCreatedEventGroup, /* 事件标志组句柄 */
							         TASK_BIT_ALL,		/* 等待WDG_BIT_TASK_ALL被设置 */
							         pdTRUE,				 /* 退出前TASK_BIT_ALL被清除，这里是TASK_BIT_ALL都被设置才表示“退出”*/
							         pdTRUE,				 /* 设置为pdTRUE表示等待TASK_BIT_ALL都被设置*/
							         xTicksToWait);			 /* 等待延迟时间 */
		printf("uxBits=%04x\n",uxBits);		
		//printf("camera=%d\n",eTaskGetState(cameraTask_Handler));
	 
	  xEventGroupClearBits(xCreatedEventGroup, TASK_BIT_Clear);
		
   //if((uxBits & (1<<9))==0)  //开始检测  camera  
	 if(GPIO_ReadOutputDataBit(GPIOG,GPIO_Pin_15)==0) //
	 {
		uxBits|=(1<<0);//camera
    uxBits|=(1<<1);//usb
		uxBits|=(1<<2);//sd
		uxBits|=(1<<5);//mqtt
    printf("uxBits_begin_camera=%04x\n",uxBits);				 
		count++;
		if(count>10)
		{
		 Flag_wdg=0;//停止喂狗
		}
    if(Flag_wdg==1)
		{
		  if(((uxBits&0xff)&TASK_BIT_ALL) == 0xff)
			{
				IWDG_Feed();
				printf("camera begin\n");
			}				
		}						 			 
	 }
   //if((uxBits & (1<<9)))  //停止检测 camera  
	 if(GPIO_ReadOutputDataBit(GPIOG,GPIO_Pin_15)==1)
	 {
		uxBits|=(1<<0);//camera
    uxBits|=(1<<1);//usb
    count=0;	
    printf("uxBits_stop_camera=%04x\n",uxBits);		 
		if(((uxBits&0xff)&TASK_BIT_ALL)==0xff)
		{
			IWDG_Feed();
			printf("camera and usb block\n");
		}	 
	 } 
 }
}

		