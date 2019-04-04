#include "dm9000.h"
#include "netif/ethernetif.h" 
#include "delay.h"
#include "usart.h"
#include "lwip_comm.h"
#include "netif/ethernetif.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "sensor.h"
#include "camera.h"
//#include "queue.h"
#include "usart.h"
#include "print.h"

#include "iwdg.h"
#include "STM32F10x_IWDG.h"
//#include "portmacro.h"
#include "FreeRTOS.h"
#include "event_groups.h"

struct dm9000_config dm9000cfg;	// DM9000配置结构体
void *DM_bSem;			// 二值信号量句柄  数据接收信号量
void *DM_xSem;			// 互斥信号量
void *tcp_udp_Sem=NULL;//互斥信号量

u16 link_status;

u8 DM9000_Init(void)
{
	u32 temp;
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef ReadWriteTiming;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE|\
						   RCC_APB2Periph_GPIOF|RCC_APB2Periph_GPIOG,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOG,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|\
								  GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOD,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|\
								  GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOE,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOF,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOG,&GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG,GPIO_PinSource6);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line6;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	EXTI_ClearITPendingBit(EXTI_Line6);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_SYS_INTERRUPT_PRIORITY + 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	ReadWriteTiming.FSMC_AddressSetupTime = 0;
	ReadWriteTiming.FSMC_AddressHoldTime = 0;
	ReadWriteTiming.FSMC_DataSetupTime = 3;
	ReadWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
	ReadWriteTiming.FSMC_CLKDivision = 0x00;
	ReadWriteTiming.FSMC_DataLatency = 0x00;
	ReadWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;
	
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM2;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &ReadWriteTiming;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &ReadWriteTiming;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM2,ENABLE);

	temp=*(vu32*)(0x1FFFF7E8);
	dm9000cfg.mode=DM9000_AUTO;	
 	dm9000cfg.queue_packet_len=0;
	dm9000cfg.imr_all = IMR_PAR|IMR_PRI; 
	// SSI+ID,mac第一位应为偶数
	dm9000cfg.mac_addr[0]='s'/2*2;
	dm9000cfg.mac_addr[1]='s';
	dm9000cfg.mac_addr[2]='i';
	dm9000cfg.mac_addr[3]=(temp>>16)&0XFF;	// 低三字节用STM32的唯一ID
	dm9000cfg.mac_addr[4]=(temp>>8)&0XFFF;
	dm9000cfg.mac_addr[5]=temp&0XFF;

	dm9000cfg.multicase_addr[0]=0Xff;
	dm9000cfg.multicase_addr[1]=0Xff;
	dm9000cfg.multicase_addr[2]=0Xff;
	dm9000cfg.multicase_addr[3]=0Xff;
	dm9000cfg.multicase_addr[4]=0Xff;
	dm9000cfg.multicase_addr[5]=0Xff;
	dm9000cfg.multicase_addr[6]=0Xff;
	dm9000cfg.multicase_addr[7]=0Xff; 
	
	DM9000_Reset();
	delay_ms(100);
	temp=DM9000_Get_DeiviceID();
	if(temp!=DM9000_ID) return 1;
	DM9000_Set_PHYMode(dm9000cfg.mode);
	
	DM9000_WriteReg(DM9000_NCR,0X00);
	DM9000_WriteReg(DM9000_TCR,0X00);
	DM9000_WriteReg(DM9000_BPTR,0X3F);	
	DM9000_WriteReg(DM9000_FCTR,0X38);
	DM9000_WriteReg(DM9000_FCR,0X00);
	DM9000_WriteReg(DM9000_SMCR,0X00);
	DM9000_WriteReg(DM9000_NSR,NSR_WAKEST|NSR_TX2END|NSR_TX1END);
	DM9000_WriteReg(DM9000_ISR,0X0F);
	DM9000_WriteReg(DM9000_TCR2,0X80);

	DM9000_Set_MACAddress(dm9000cfg.mac_addr);
	DM9000_Set_Multicast(dm9000cfg.multicase_addr);
	DM9000_WriteReg(DM9000_RCR,RCR_DIS_LONG|RCR_DIS_CRC|RCR_RXEN);
	DM9000_WriteReg(DM9000_IMR,IMR_PAR); 
	temp=DM9000_Get_SpeedAndDuplex();

	DM9000_WriteReg(DM9000_IMR,dm9000cfg.imr_all);
	return 0;		
}

u16 DM9000_ReadReg(u16 reg)
{
	DM9000->REG=reg;
	return DM9000->DATA; 
}

void DM9000_WriteReg(u16 reg,u16 data)
{
	DM9000->REG=reg;
	DM9000->DATA=data;
}

u16 DM9000_PHY_ReadReg(u16 reg)
{
	u16 temp;
	DM9000_WriteReg(DM9000_EPAR,DM9000_PHY|reg);
	DM9000_WriteReg(DM9000_EPCR,0X0C);
	delay_ms(10);
	DM9000_WriteReg(DM9000_EPCR,0X00);
	temp=(DM9000_ReadReg(DM9000_EPDRH)<<8)|(DM9000_ReadReg(DM9000_EPDRL));
	return temp;
}

void DM9000_PHY_WriteReg(u16 reg,u16 data)
{
	DM9000_WriteReg(DM9000_EPAR,DM9000_PHY|reg);
	DM9000_WriteReg(DM9000_EPDRL,(data&0xff));
	DM9000_WriteReg(DM9000_EPDRH,((data>>8)&0xff));
	DM9000_WriteReg(DM9000_EPCR,0X0A);
	delay_ms(50);
	DM9000_WriteReg(DM9000_EPCR,0X00);	
}

u32 DM9000_Get_DeiviceID(void)
{
	u32 value;
	value =DM9000_ReadReg(DM9000_VIDL);
	value|=DM9000_ReadReg(DM9000_VIDH) << 8;
	value|=DM9000_ReadReg(DM9000_PIDL) << 16;
	value|=DM9000_ReadReg(DM9000_PIDH) << 24;
	return value;
}

u8 DM9000_Get_SpeedAndDuplex(void)
{
	u8 temp;
	u8 i=0;	
	if(dm9000cfg.mode==DM9000_AUTO)
	{
		while(!(DM9000_PHY_ReadReg(0X01)&0X0020))
		{
			delay_ms(100);					
			i++;
			if(i>100)return 0XFF;
		}	
	}
	else
	{
		while(!(DM9000_ReadReg(DM9000_NSR)&0X40))
		{
			delay_ms(100);					
			i++;
			if(i>100)return 0XFF;
		}
	}
	temp =((DM9000_ReadReg(DM9000_NSR)>>6)&0X02);
	temp|=((DM9000_ReadReg(DM9000_NCR)>>3)&0X01);
	return temp;
}

void DM9000_Set_PHYMode(u8 mode)
{
	u16 BMCR_Value,ANAR_Value;	
	switch(mode)
	{
		case DM9000_10MHD:		//10M半双工
			BMCR_Value=0X0000;
			ANAR_Value=0X21;
			break;
		case DM9000_10MFD:		//10M全双工
			BMCR_Value=0X0100;
			ANAR_Value=0X41;
			break;
		case DM9000_100MHD:		//100M半双工
			BMCR_Value=0X2000;
			ANAR_Value=0X81;
			break;
		case DM9000_100MFD:		//100M全双工
			BMCR_Value=0X2100;
			ANAR_Value=0X101;
			break;
		case DM9000_AUTO:		//自动协商模式
			BMCR_Value=0X1000;
			ANAR_Value=0X01E1;
			break;		
	}
	DM9000_PHY_WriteReg(DM9000_PHY_BMCR,BMCR_Value);
	DM9000_PHY_WriteReg(DM9000_PHY_ANAR,ANAR_Value);
 	DM9000_WriteReg(DM9000_GPR,0X00);
}

void DM9000_Set_MACAddress(u8 *macaddr)
{
	u8 i;
	for(i=0;i<6;i++)
	{
		DM9000_WriteReg(DM9000_PAR+i,macaddr[i]);
	}
}

void DM9000_Set_Multicast(u8 *multicastaddr)
{
	u8 i;
	for(i=0;i<8;i++)
	{
		DM9000_WriteReg(DM9000_MAR+i,multicastaddr[i]);
	}
}

void DM9000_Reset(void)
{
	DM9000_RST = 0;
	delay_ms(10);
	DM9000_RST = 1;
	delay_ms(100);
 	DM9000_WriteReg(DM9000_GPCR,0x01);
	DM9000_WriteReg(DM9000_GPR,0);
 	DM9000_WriteReg(DM9000_NCR,(0x02|NCR_RST));
	do 
	{
		delay_ms(25); 	
	}while(DM9000_ReadReg(DM9000_NCR)&1);
	DM9000_WriteReg(DM9000_NCR,0);
	DM9000_WriteReg(DM9000_NCR,(0x02|NCR_RST));
	do 
	{
		delay_ms(25);	
	}while (DM9000_ReadReg(DM9000_NCR)&1);
} 

void DM9000_SendPacket(struct pbuf *p)
{
	struct pbuf *q;
	u16 pbuf_index = 0;
	u8 word[2], word_index = 0;
	
	xSemaphoreTake(DM_xSem, portMAX_DELAY);
	
	DM9000_WriteReg(DM9000_IMR, IMR_PAR);
	DM9000->REG = DM9000_MWCMD;
	q = p;
	
 	while(q)
	{
		if (pbuf_index < q->len)
		{
			word[word_index++] = ((u8_t*)q->payload)[pbuf_index++];
			if (word_index == 2)
			{
				DM9000->DATA = ((u16)word[1]<<8)|word[0];
				word_index = 0;
			}
		}
		else
		{
			q = q->next;
			pbuf_index = 0;
		}
	}

	if(word_index == 1)DM9000->DATA = word[0];

	DM9000_WriteReg(DM9000_TXPLL, p->tot_len&0XFF);
	DM9000_WriteReg(DM9000_TXPLH, (p->tot_len>>8)&0XFF);
	DM9000_WriteReg(DM9000_TCR, 0X01);
	while((DM9000_ReadReg(DM9000_ISR)&0X02)==0);
	DM9000_WriteReg(DM9000_ISR, 0X02);
 	DM9000_WriteReg(DM9000_IMR, dm9000cfg.imr_all);
	
	xSemaphoreGive(DM_xSem);
}

struct pbuf *DM9000_Receive_Packet(void)
{
	struct pbuf* p = NULL;
	struct pbuf* q;
    u32 rxbyte;
	vu16 rx_status, rx_length;
    u16* data;
	u16 dummy; 
	int len;

	xSemaphoreTake(DM_xSem, portMAX_DELAY);
__error_retry:	
	DM9000_ReadReg(DM9000_MRCMDX);
	rxbyte = (u8)DM9000->DATA;
	if(rxbyte)
	{
		if(rxbyte > 1)
		{
//			printf("dm9000 rx: rx error, stop device\r\n");
			DM9000_WriteReg(DM9000_RCR, 0x00);
			DM9000_WriteReg(DM9000_ISR, 0x80);		 
			return (struct pbuf*)p;
		}
		DM9000->REG = DM9000_MRCMD;
		rx_status = DM9000->DATA;
        rx_length = DM9000->DATA;
        p = pbuf_alloc(PBUF_RAW, rx_length, PBUF_POOL);
		if(p != NULL)
        {
            for(q=p; q!=NULL; q=q->next)
            {
                data = (u16*)q->payload;
                len = q->len;
                while(len > 0)
                {
					*data = DM9000->DATA;
                    data ++;
                    len -= 2;
                }
            }
        }
		else
		{
//			printf("pbuf内存申请失败:%d\r\n",rx_length);
            data = &dummy;
			len = rx_length;
			while(len)
			{
				*data = DM9000->DATA;
				len -= 2;
			}
        }	

		if((rx_status&0XBF00) || (rx_length < 0X40) || (rx_length > DM9000_PKT_MAX))
		{
//			printf("rx_status:%#x\r\n",rx_status);
//			if (rx_status & 0x100)printf("rx fifo error\r\n");
//			if (rx_status & 0x200)printf("rx crc error\r\n");
//			if (rx_status & 0x8000)printf("rx length error\r\n");
            if (rx_length > DM9000_PKT_MAX)
			{
//				printf("rx length too big\r\n");
				DM9000_WriteReg(DM9000_NCR, NCR_RST);
				delay_ms(5);
			}
			if(p != NULL)pbuf_free((struct pbuf*)p);
			p = NULL;
			goto __error_retry;
		}
	}
	else
    {
        DM9000_WriteReg(DM9000_ISR, ISR_PTS);
        dm9000cfg.imr_all = IMR_PAR|IMR_PRI;
        DM9000_WriteReg(DM9000_IMR, dm9000cfg.imr_all);
    }
	
	xSemaphoreGive(DM_xSem);
	return (struct pbuf*)p; 
}

//中断处理函数
void DMA9000_ISRHandler(void)
{
	u16 int_status;
	//u16 link_status;
	
	u16 last_io; 
	portBASE_TYPE xSwitchRequired;
	BaseType_t xHigherPriorityTaskWoken;
	
	last_io = DM9000->REG;
	int_status = DM9000_ReadReg(DM9000_ISR);
  
  //link_status = DM9000_ReadReg(NSR_LINKST);	
 // printf("int_status=%d\n",int_status);	
	
	DM9000_WriteReg(DM9000_ISR, int_status);
	if(int_status & ISR_ROS)printf("dm9000.c：overflow \r\n");
    if(int_status & ISR_ROOS)printf("dm9000.c：overflow counter overflow \r\n");	
	if(int_status & ISR_PRS)		//接收中断
	{
			/* 发送事件标志，表示任务正常运行 */
	 //xEventGroupSetBitsFromISR(xCreatedEventGroup, TASK_BIT_LWIP,&xHigherPriorityTaskWoken);
		/* Give the semaphore in case the lwIP task needs waking. */
		xSemaphoreGiveFromISR(DM_bSem, &xSwitchRequired);
	}
	if(int_status&ISR_PTS)			//发送中断
	{ 
		// 发送完成中断，用户自行添加所需代码
	}
	DM9000->REG = last_io;
	
	/* Switch tasks if necessary. */
	portEND_SWITCHING_ISR(xSwitchRequired);
}

//外部中断线6的中断服务函数
void  EXTI9_5_IRQHandler(void)
{
	static BaseType_t xHigherPriorityTaskWoken;
	BaseType_t xSwitchRequired = pdFALSE;
	EXTI_ClearITPendingBit(EXTI_Line6); //清除中断线6挂起标志位
  
	if(EXTI_GetITStatus(EXTI_Line7) != RESET) 
	{
	 delay_xms(100);
	 if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)==1) //开门
	 {
    delay_xms(100);	 
		if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)==1)
		{
			xSemaphoreGiveFromISR(BinarySemaphore_photo_command,&xHigherPriorityTaskWoken);			
			light_ON();
			printf("EXTI_Line7  interrput\n");      
		}	  	
	 }
   else if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)==0)//关门
	 {
		 delay_xms(100);
		 if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)==0)
		 {
		 	light_OFF();			 
		 }		
	 }		   
	}
	EXTI_ClearITPendingBit(EXTI_Line7); //清除中断线7挂起标志位	
  portEND_SWITCHING_ISR(xSwitchRequired);	
	while(DM9000_INT == 0)
	{
		DMA9000_ISRHandler();
	}	
}

