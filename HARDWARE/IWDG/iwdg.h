#ifndef __WDG_H
#define __WDG_H
#include "sys.h"
#include "FreeRTOS.h"
#include "event_groups.h" 


#define TASK_BIT_Camera (1 <<0)
#define TASK_BIT_USB	  (1 << 1)
#define TASK_BIT_SD     (1 << 2)

#define TASK_BIT_LWIP	  (1 << 3)
#define TASK_BIT_TCP	  (1 << 4)
#define TASK_BIT_MQTT	  (1 << 5)
#define TASK_BIT_TICK	  (1 << 6)
#define TASK_BIT_MAIN	  (1 << 7)


//#define TASK_BIT_begin  (1 <<8)
//#define TASK_BIT_end    (1 <<9)

#define TASK_BIT_camera_flag  (1 <<9)
#define TASK_BIT_sd_flag      (1 <<8)

#define TASK_BIT_timestamp   (1 <<10)  
#define TASK_BIT_tcp_client  (1 <<11)

#define USB_ClOSE_BIT         (1 <<12)




//#define TASK_BIT_ALL (TASK_BIT_LWIP | TASK_BIT_USB | TASK_BIT_Camera | TASK_BIT_TCP|TASK_BIT_MQTT| \
//                      TASK_BIT_TICK | TASK_BIT_MAIN | TASK_BIT_SD|TASK_BIT_camera_flag|TASK_BIT_sd_flag)

#define TASK_BIT_ALL (TASK_BIT_LWIP | TASK_BIT_USB | TASK_BIT_Camera | TASK_BIT_TCP|TASK_BIT_MQTT| \
                      TASK_BIT_TICK | TASK_BIT_MAIN | TASK_BIT_SD)
											
											
#define TASK_BIT_Clear (TASK_BIT_LWIP | TASK_BIT_USB | TASK_BIT_Camera | TASK_BIT_TCP|TASK_BIT_MQTT| \
                      TASK_BIT_TICK | TASK_BIT_MAIN | TASK_BIT_SD)

//#define TASK_BIT_TEST (TASK_BIT_LWIP | TASK_BIT_TCP|TASK_BIT_MQTT| \
//                      TASK_BIT_TICK | TASK_BIT_MAIN | TASK_BIT_SD|TASK_BIT_begin|TASK_BIT_end)

//#define TASK_BIT_ALL (TASK_BIT_SD )






void IWDG_Init(u8 prer,u16 rlr);
void IWDG_Feed(void);

/* 定义事件组 */
extern EventGroupHandle_t xCreatedEventGroup;

 
#endif
