
#include "usb_lib.h" 
#include "mass_mal.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usart.h"
#include "hw_config.h"
#include "FreeRTOS.h"

void USB_HP_CAN1_TX_IRQHandler(void)
{
	CTR_HP();
}

void USB_LP_CAN1_RX0_IRQHandler(void) 
{
	USB_Istr();
} 

void USB_HwInit(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	// Reset USB Engine
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB, ENABLE);
	// Init USB Clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);////////////
	// Release reset USB Engine
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB, DISABLE);
	// Enable USB 48MHz clock
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);///////////////

	// GPIO assign to the USB engine
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

	// Disconnect device
	USB_Cable_Config(DISABLE);

	// USB interrupt connect to NVIC
	NVIC_InitStructure.NVIC_IRQChannel = USB_HP_CAN1_TX_IRQn;//USB_HP_CAN_TX_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_INTR_HIGH_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USB_LP_Irq_Enable();
}

/*******************************************************************************
* Function Name  : USB_LP_Irq_En
* Description    : 开启USB LP中断
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_LP_Irq_Enable(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;//USB_LP_CAN_RX0_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_INTR_LOW_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
* Function Name  : USB_LP_Irq_En
* Description    : 关闭USB LP中断
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_LP_Irq_Disable(void)
{
	/* Disable the Selected IRQ Channels -------------------------------------*/
	NVIC->ICER[USB_LP_CAN1_RX0_IRQn >> 0x05] =	(u32)0x01 << (USB_LP_CAN1_RX0_IRQn & (u8)0x1F);
}

/*******************************************************************************
* Function Name  : Enter_LowPowerMode
* Description    : Power-off system clocks and power while entering suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Enter_LowPowerMode(void)
{
	/* Set the device state to suspend */
	bDeviceState = SUSPENDED;
}

/*******************************************************************************
* Function Name  : Leave_LowPowerMode
* Description    : Restores system clocks and power while exiting suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Leave_LowPowerMode(void)
{
	DEVICE_INFO *pInfo = &Device_Info;

	/* Set the device state to the correct state */
	if (pInfo->Current_Configuration != 0)
	{
		/* Device configured */
		bDeviceState = CONFIGURED;
	}
	else
	{
		bDeviceState = ATTACHED;
	}
}

/*******************************************************************************
* Function Name  : USB_Cable_Config
* Description    : Software Connection/Disconnection of USB Cable.
* Input          : None.
* Return         : Status
*******************************************************************************/
void USB_Cable_Config(FunctionalState NewState)
{
	if (NewState != DISABLE)
	{
	//	GPIO_SetBits(USB_CONNECT_PORT, USB_CONNECT_PIN);
	}
	else
	{
	//	GPIO_ResetBits(USB_CONNECT_PORT, USB_CONNECT_PIN);
	}
} 

void USB_Port_Set(u8 enable)
{
	RCC->APB2ENR|=1<<2;
	if(enable)_SetCNTR(_GetCNTR()&(~(1<<1)));
	else
	{	  
		_SetCNTR(_GetCNTR()|(1<<1));
		GPIOA->CRH&=0XFFF00FFF;
		GPIOA->CRH|=0X00033000;
		PAout(12)=0;	    		  
	}
}

/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Get_SerialNum(void)
{
	u32 Device_Serial0, Device_Serial1, Device_Serial2;
	Device_Serial0 = *(u32*)(0x1FFFF7E8);
	Device_Serial1 = *(u32*)(0x1FFFF7EC);
	Device_Serial2 = *(u32*)(0x1FFFF7F0);
	Device_Serial0 += Device_Serial2;
	if (Device_Serial0 != 0)
	{
		MASS_StringSerial[2] = (u8)(Device_Serial0 & 0x000000FF);
		MASS_StringSerial[4] = (u8)((Device_Serial0 & 0x0000FF00) >> 8);
		MASS_StringSerial[6] = (u8)((Device_Serial0 & 0x00FF0000) >> 16);
		MASS_StringSerial[8] = (u8)((Device_Serial0 & 0xFF000000) >> 24);

		MASS_StringSerial[10] = (u8)(Device_Serial1 & 0x000000FF);
		MASS_StringSerial[12] = (u8)((Device_Serial1 & 0x0000FF00) >> 8);
		MASS_StringSerial[14] = (u8)((Device_Serial1 & 0x00FF0000) >> 16);
		MASS_StringSerial[16] = (u8)((Device_Serial1 & 0xFF000000) >> 24);

		MASS_StringSerial[18] = (u8)(Device_Serial2 & 0x000000FF);
		MASS_StringSerial[20] = (u8)((Device_Serial2 & 0x0000FF00) >> 8);
		MASS_StringSerial[22] = (u8)((Device_Serial2 & 0x00FF0000) >> 16);
		MASS_StringSerial[24] = (u8)((Device_Serial2 & 0xFF000000) >> 24);
	}
}
