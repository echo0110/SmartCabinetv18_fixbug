#ifndef __camera_H
#define __camera_H	
#include "sys.h"

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"


extern void* cameraTask_Handler;

//extern void* BinarySemaphore_mac;	//mac二值信号量句柄  数据接收信号量	
//extern void* BinarySemaphore_mac_config;//mac配置信号量 
extern void* BinarySemaphore_camera;//摄像头抓拍信号量
extern void* BinarySemaphore_response;
extern void* cameraTask_Handler;

extern void* BinarySemaphore_camera_Sync;
extern void* BinarySemaphore_camera_response;
extern void* BinarySemaphore_camera_data_response;
extern void* BinarySemaphore_photo_command;
extern void* test_trap_signal;
extern void* test_tcp_signal;
extern void* timestamp_signal;
extern void* ping_signal;

extern void* close_tcp_server;
extern void* open_tcp_server;
extern void* WDG_Sem;//看门狗检测

extern void UART_Send(u8 *str,u8 len3);
extern u8 Camera_init(void);
extern void camera_write_flash_test(void);
extern u8 Camera_check(void);

extern xQueueHandle MsgQueue;  
extern xQueueHandle Msg_response; 

extern void* tcp_command;
extern void camera_write_SD_test(void);
//extern void write_sd_test(void);
void write_sd_test(char j);

extern void read_sd_test(void);
extern void image_rename(void);
extern TaskHandle_t ServerTask_Handler;

extern char* time_to_timestamp(void);
//extern char  temp_str[17];

extern char  temp_str[50];

extern void time_to_sd_path(char *buff);
extern void EXTI_PC7_Init(void);

extern SemaphoreHandle_t sd_Sem;

extern void* mqtt_command;

extern xQueueHandle USB_Queue;

void USB_Close(void);//关SD卡轮询检测任务
void USB_Open(void);//开


 
#endif 
