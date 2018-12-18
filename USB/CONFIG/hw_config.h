#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H
#include "platform_config.h"
#include "usb_type.h" 
 
#define BULK_MAX_PACKET_SIZE  0x00000040	//包大小,最大64字节.

#define USB_INTR_HIGH_PRIORITY	(configLIBRARY_SYS_INTERRUPT_PRIORITY+3)
#define USB_INTR_LOW_PRIORITY	(configLIBRARY_SYS_INTERRUPT_PRIORITY+3)

//USB通用代码函数声明
void USB_HwInit(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Cable_Config (FunctionalState NewState);
void Get_SerialNum(void);
void USB_LP_Irq_Enable(void);
void USB_LP_Irq_Disable(void);
void USB_Port_Set(u8 enable);
#endif  
