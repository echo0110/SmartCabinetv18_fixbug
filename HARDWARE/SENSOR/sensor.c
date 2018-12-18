#include "stm32f10x.h"
#include "sys.h" 
#include "delay.h"
#include "usart.h"
#include "sensor.h"

#include "FreeRTOS.h"
#include "task.h"
#include "camera.h"
#include "semphr.h"

#include "mmc_sd.h"
#include "diskio.h"
#include "message.h"
#include "ff.h"
#include "mass_mal.h"
#include "diskio.h"	
#include "exfuns.h"
#include "usb_prop.h"
#include "oled.h"
#include "mmc_sd.h"	
#include "spi1.h"
#include "stm32f10x_iwdg.h"
#include "tcp_client_demo.h" 
#include "hw_config.h"
#include "memory.h"
#include "usb_init.h"
#include "adc.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "STM32F10x_IWDG.h"
#include "iwdg.h"
#include "httpd.h"
#include <string.h>
#include <stdio.h>
#include "usb_regs.h"
#include "mqtt.h"

#define	USB_EP_NUM	4

#define EP_BUF_ADDR (sizeof(EP_BUF_DSCR)*USB_EP_NUM)  

//EP_BUF_DSCR *EP0_DSCR = (EP_BUF_DSCR *) 0x40006000;

/*USB??????????????,????*/  
EP_BUF_DSCR * pBUF_DSCR = (EP_BUF_DSCR *) 0x40006000; 

float TEM = 0, HUM = 0;
u8 DOOR_STAT=1, WATER_STAT, SYS12_STAT, EQU12_STAT, EQU24_STAT, BAT12_STAT,UPS_STAT,special_power_STAT;
u8 AC1_STAT, AC2_STAT, AC3_STAT,fan_STAT, alarm_STAT, light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT,out_special_STAT;
u8 IS_EQU_SYS12V, IS_EQU_UPS12V;
u8 stat_changed = 0;
u32 check_stat_times;

int SD_STAT;


void USB_CTR_Handler(void);
extern xQueueHandle MainTaskQueue;
extern xQueueHandle USBTaskQueue;
extern TaskHandle_t USBTask_Handler;



// 传感器及IO控制初始化
void SENSOR_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB| RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |RCC_APB2Periph_GPIOE| RCC_APB2Periph_GPIOF, ENABLE);
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO,ENABLE);
	
	// PA4	SD卡CS引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_IPD;  //下拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  //
	GPIO_Init(GPIOA,&GPIO_InitStructure);  	
	// PC7	外部中断引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_IPU;  //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  //
	GPIO_Init(GPIOC,&GPIO_InitStructure);  
	
	// PA8	远程开门
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		// 推挽
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// PB7	水浸
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		// 推挽
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// PC12	温湿度
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;	// 开漏输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	
	// PG12	主电源 电源1 直流12V
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;		// 上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	// PD3	电源2
	// PD6	UPS状态检测
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;		// 上拉输入
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	// PB9	特殊电源状态检测
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;		// 上拉输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	

	
	// PB6	特殊电源V4
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	// 推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	// PB7	水浸
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;		// 下拉输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// PE2	V3
	// PE3	V2
	// PE4	V1
	// PE5	风扇
	// PE6	交流L1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6;	    		 //LED1-->PE.5 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOE, &GPIO_InitStructure);	  				 //推挽输出 ，IO口速度为50MHz
	GPIO_SetBits(GPIOE,GPIO_Pin_2);//V3  默认开	
	GPIO_SetBits(GPIOE,GPIO_Pin_3);//V2  默认开	
	GPIO_SetBits(GPIOE,GPIO_Pin_4);//V1  默认开	
	GPIO_SetBits(GPIOE,GPIO_Pin_5);//关风扇
	GPIO_ResetBits(GPIOE,GPIO_Pin_6);//L1  默认开	
	
	// PF6	报警器
	// PF7	交流L2
	// PF8	照明
	// PF9	交流L3
	// PF10	加热控制
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	// 推挽输出
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOF,GPIO_Pin_7);						 //L2  默认开					 
	GPIO_ResetBits(GPIOF,GPIO_Pin_9);						 //L3  默认开	
  GPIO_SetBits(GPIOF,GPIO_Pin_8);						 //F8  照明默认关
	
	
	//测试引脚
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
 // PA12  D+引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
			
}

u8 Read_SensorData(void)
{
	u8 i, count, dat, Rev_Bit;
	
	for(i=0; i<8; i++)
	{
		count = 1;
		while(TEM_HUM_SENSOR&&(count++))delay_us(1);	// 等待信号高电平结束
		count = 1;
		while(!TEM_HUM_SENSOR&&(count++))delay_us(1);	// 信号低电平时间48-55us
		delay_us(50);									// 信号高电平时间"0"22-30us,"1"68-75us
		if(TEM_HUM_SENSOR)Rev_Bit = 1;
		else Rev_Bit = 0;
		dat <<= 1;
		dat |= Rev_Bit;
	}
	return (dat);
}

void GET_AM2301_Data(void)
{
	u8 count, i, Sensor_Check = 0;
	u8 Sensor_Data[4];
	
	AM2301_Write_0();delay_us(1000);	// 主机起始信号拉低时间800-2000us
	AM2301_Write_1();delay_us(30);		// 主机释放总线时间20-200us
	if(!TEM_HUM_SENSOR)					// 收到应答信号说明通信成功
	{
		count = 1;
		while(!TEM_HUM_SENSOR&&(count++))delay_us(1);	// 响应低电平时间75-85us
		count = 1;
		while(TEM_HUM_SENSOR&&(count++))delay_us(1);	// 响应高电平时间75-85us
		
		// 数据接收
		for(i=0; i<4; i++)
		{
			Sensor_Data[i] = Read_SensorData();
			Sensor_Check += Sensor_Data[i];
		}
		if(Sensor_Check == Read_SensorData())
		{
			HUM = Sensor_Data[0]*256 + Sensor_Data[1];
			HUM = HUM/10;
			TEM = Sensor_Data[2]*256 + Sensor_Data[3];
			if(TEM >= 0x8000)TEM = 0-TEM/10;
			else TEM = TEM/10;			
			if(TEM > 42)out_fan_ON();
			if(TEM < 40)out_fan_OFF();
			
			if(TEM > 2)heat_OFF();//
			if(TEM < 0)heat_ON();
		}
	}
}

void STAT_CHECK(void)
{
	if(WATER_STAT != WATER_SENSOR)  //水浸状态改变推送
	{
		delay_ms(10);
		if(WATER_STAT != WATER_SENSOR)
		{
			WATER_STAT = WATER_SENSOR;
			stat_changed = 1;
		}
	}
	
	if(SYS12_STAT != SYS12_SENSOR) //电源1 直流12V
	{
		delay_ms(10);
		if(SYS12_STAT != SYS12_SENSOR)
		{
			SYS12_STAT = SYS12_SENSOR;
			stat_changed = 1;
		}
	}
	
	if(EQU12_STAT != EQU12_SENSOR) //电源2 
	{
		delay_ms(10);
		if(EQU12_STAT != EQU12_SENSOR)
		{
			EQU12_STAT = EQU12_SENSOR;
			stat_changed = 1;
		}
	}
	
	if(UPS_STAT != UPS_SENSOR)  //UPS_STA
	{
		delay_ms(10);
		if(UPS_STAT != UPS_SENSOR)
		{
			UPS_STAT = UPS_SENSOR;
			stat_changed = 1;
		}
	}	
	if(special_power_STAT != special_power_SENSOR)  //特殊电源
	{
		delay_ms(10);
		if(special_power_STAT != special_power_SENSOR)
		{
			special_power_STAT = special_power_SENSOR;
			stat_changed = 1;
		}
	}		
	if(AC1_STAT != AC1_SENSOR())  //交流L1
	{
		delay_ms(10);
		if(AC1_STAT != AC1_SENSOR())
		{
			AC1_STAT = AC1_SENSOR();
			stat_changed = 1;
		}
	}
	if(AC2_STAT != AC2_SENSOR())  //交流L2
	{
		delay_ms(10);
		if(AC2_STAT != AC2_SENSOR())
		{
			AC2_STAT = AC2_SENSOR();
			stat_changed = 1;
		}
	}
	if(AC3_STAT != AC3_SENSOR())  //交流L3
	{
		delay_ms(10);
		if(AC3_STAT != AC3_SENSOR())
		{
			AC3_STAT = AC3_SENSOR();
			stat_changed = 1;
		}
	}
	if(DC1_STAT != DC1_SENSOR())  //直流DC1
	{
		delay_ms(10);
		if(DC1_STAT != DC1_SENSOR())
		{
			DC1_STAT = DC1_SENSOR();
			stat_changed = 1;
		}
	}
	if(DC2_STAT != DC2_SENSOR())  //直流DC2
	{
		delay_ms(10);
		if(DC2_STAT  !=DC2_SENSOR())
		{
			DC2_STAT = DC2_SENSOR();
			stat_changed = 1;
		}
	}
	
	if(DC3_STAT != DC3_SENSOR())  //直流DC3
	{
		delay_ms(10);
		if(DC3_STAT != DC3_SENSOR())
		{
			DC3_STAT = DC3_SENSOR();
			stat_changed = 1;
		}
	}
	if(DC4_STAT != DC4_SENSOR())  //直流DC4
	{
		delay_ms(10);
		if(DC4_STAT != DC4_SENSOR())
		{
			DC4_STAT = DC4_SENSOR();
			stat_changed = 1;
		}
	}
	
	vTaskSuspendAll();
	GET_AM2301_Data();
	xTaskResumeAll();	
}

/*
 * 函数名：void DOOR_SENSOR_CHECK(void)
 * 描述  ：门开关定时检测
 * 输入  ：无
 * 输出  ：无	
 */
void DOOR_SENSOR_CHECK(void) //门开关接口扫描函数
{
	if(DOOR_STAT != DOOR_SENSOR)
	{
		delay_ms(10);
		if(DOOR_STAT != DOOR_SENSOR)
		{
			DOOR_STAT = DOOR_SENSOR;					
			stat_changed = 1;
			if(DOOR_SENSOR==1)//开门
			{					
				light_ON();
				xSemaphoreGive(BinarySemaphore_photo_command);		
			}
			else
			{
			 light_OFF();
			}						
		}
	}
}

/*
 * 函数名：void LIGHT_SENSOR_CHECK(void)
 * 描述  ：门开关定时检测
 * 输入  ：无
 * 输出  ：无	
 */
void LIGHT_SENSOR_CHECK(void)
{
	if(DOOR_SENSOR==1)//如果门是开着的
	{					
		light_ON();
	}
	if(DOOR_SENSOR==0)//如果门是关着的
	{					
		light_OFF();
	}
}
     


void sd_out_config(void)
{	
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
  // PA4	SD卡CS引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;  //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  //
	GPIO_Init(GPIOA,&GPIO_InitStructure);  
}


void sd_ipd_config(void)
{	
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
  // PA4	SD卡CS引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_IPD;  //下拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  //
	GPIO_Init(GPIOA,&GPIO_InitStructure);  
}





/*
 * 函数名：main 任务函数 
 * 描述  ：
 * 输入  ：无
 * 输出  ：无	
 */
void tcp_task(void *pvParameters)
{ 	
	for(;;)
	{	
	 if(timestamp_flag&(1<<7))
   {     	 
		tcp_client_test();		 
	 }		 	 	 
	 if(tcp_client_flag&(1<<7))  //发送时间戳
	 {		  
	  tcp_client_test();  
	 }
	 if(tcp_close_flag&(1<<7))  //关闭TCP连接
	 {
	  tcp_client_test(); 
	 }
   vTaskDelay(10);
	}
}


/*
 * 函数名：void sd_task(void *pvParameters)
 * 描述  ：Sd卡检测任务
 * 输入  ：无
 * 输出  ：无	
 */
void sd_task(void *pvParameters)
{	
	for(;;)
	{	
	 SD_SENSOR_CHECK();		 		 	 	  		
	 vTaskDelay(500);     	
	}
}
/*
 * 函数名：void ping_task(void *pvParameters)
 * 描述  ：ping包检测
 * 输入  ：无
 * 输出  ：无	
 */
void ping_task(void *pvParameters)
{
	u8 pingFailedTimes[10];//++;
	static u8 counts,Ping_frequency;
	portTickType xLastExecutionTime;
	xLastExecutionTime = xTaskGetTickCount();	
	while(1)
	{		
		printf("Ping_ip_array[0].ip[0]==%d\n",Ping_ip_array[0].ip[0]);
		printf("Ping_ip_array[0].ip[1]==%d\n",Ping_ip_array[0].ip[1]);
		printf("Ping_ip_array[0].ip[2]==%d\n",Ping_ip_array[0].ip[2]);
		printf("Ping_ip_array[0].ip[3]==%d\n",Ping_ip_array[0].ip[3]);
		//GetCheckIP();
		for(Ping_frequency=0;Ping_frequency<4;Ping_frequency++)
		{
		 for(counts=0;counts<MAX_IP;counts++)
		 {
			if(pingCmdSend(counts))
			// pingCmdSend(counts);
			{
				if(xSemaphoreTake(ping_signal, 400)==pdTRUE)
				{
				 memcpy(IP_STAT[counts],CHECK_IP[counts],strlen(CHECK_IP[counts]));
				 printf("%d:Ping sucess\n",counts);		 		
				}
				else
				{
				 printf("ping Timeout\n");
				 pingFailedTimes[counts]++;
				 if(pingFailedTimes[counts]==4)
				 {
				 printf("%d:ping Timeout  ErrCode 4!\n",counts);			
				 strcpy(IP_STAT[counts], "0");
				 pingFailedTimes[counts]=0;
				 }						
				}
		  }
			else
			{
			 printf("ping Timeout\n");
			 pingFailedTimes[counts]++;
			 if(pingFailedTimes[counts]==4)
			 {
				printf("%d:ping Timeout  ErrCode 4!\n",counts);			
				strcpy(IP_STAT[counts], "0");
				pingFailedTimes[counts]=0;
			 }		
			}
      vTaskDelay(10);	 			
		 }
		}
    vTaskDelay(2000);	 
	}
}


/*
 * 函数名：void SD_SENSOR_CHECK(void)
 * 描述  ：Sd卡检测函数
 * 输入  ：无
 * 输出  ：无	
 */
void SD_SENSOR_CHECK(void) //SD卡轮询检测
{	
	u16 temp=0;
	temp=SD_Initialize();
	//printf("temp=%d\n",temp);
	if(SD_STAT!= temp)
	{
		delay_ms(10);
		temp=SD_Initialize();
		if(SD_STAT != temp)
		{
			SD_STAT = temp;					
			stat_changed = 1;
			if(temp==0)//SD卡在位
			{	
       Max_Lun=1;
       SD_Initialize();				
			 if(f_mount(fs[0],"0:",1) == 0X0D) //挂载SD	
			 f_mkfs("0:",0,4096);
			 Mass_Block_Size[1] =512;						
			 Mass_Block_Count[1]=0xE6D000;
			 PAout(12)=0;
			 GPIO_ResetBits(GPIOA,GPIO_Pin_12);		                                            
			 delay_xms(65);   
			 GPIO_SetBits(GPIOA,GPIO_Pin_12);	
			 delay_xms(65);
	     USB_Init();
			}
			else 
			{
			 Max_Lun=0;
			 f_mount(NULL, "0:", 0);        // 卸载SD卡
			 Mass_Block_Size[1] =0;							
			 Mass_Block_Count[1]=0;	
			}						
		}
	}
}


/*
 * 函数名：void SD_state_machine_CHECK(void) //SD卡轮询检测
 * 描述  ：状态机检测
 * 输入  ：无
 * 输出  ：无	
 */
void SD_state_machine_CHECK(void) //SD卡轮询检测
{
  u16 cur_state=0;
	u16 temp;
	temp=SD_Initialize();
	cur_state = SD_STAT;   
	switch(cur_state) //
	{            
	case 0: 
			if(temp==255) //
			{		
			 SD_STAT=255;  //
			 Max_Lun=0;
			 f_mount(NULL, "0", 0);        // 卸载SD卡
			 Mass_Block_Size[1] =0;							
			 Mass_Block_Count[1]=0;	
			} 
			else if(temp==170) //
			{   
			 SD_STAT=170;
			 Max_Lun=0;
			 f_mount(NULL, "0", 0);        // 卸载SD卡
			 Mass_Block_Size[1] =0;							
			 Mass_Block_Count[1]=0;	
			}    
      else if(temp==0) //
			{   
			 SD_STAT =0;
			 Max_Lun=1;
       SD_Initialize();				
			 if(f_mount(fs[0],"0:",1) == 0X0D) //挂载SD	
			 f_mkfs("0:",0,4096);
			 Mass_Block_Size[1] =512;						
			 Mass_Block_Count[1]=0xE6D000;
			}  			
	case 255: 
			if(temp==0) //
			{                
			 SD_STAT =0;
			 Max_Lun=1;
       SD_Initialize();				
			 if(f_mount(fs[0],"0:",1) == 0X0D) //挂载SD	
			 f_mkfs("0:",0,4096);
			 Mass_Block_Size[1] =512;						
			 Mass_Block_Count[1]=0xE6D000;
			}           
			else if(temp==170)
			{
			 SD_STAT=170;
			 Max_Lun=0;
			 f_mount(NULL, "0", 0);        // 卸载SD卡
			 Mass_Block_Size[1] =0;							
			 Mass_Block_Count[1]=0;	
			}
	case 170: 
			if(temp==170)  
			{          
			 SD_STAT=170;
			 Max_Lun=0;
			 f_mount(NULL, "0", 0);        // 卸载SD卡
			 Mass_Block_Size[1] =0;							
			 Mass_Block_Count[1]=0;	
			}
			else if(temp==255) //
			{   
			 SD_STAT=255;
			 Max_Lun=0;
			 f_mount(NULL, "0", 0);        // 卸载SD卡
			 Mass_Block_Size[1] =0;							
			 Mass_Block_Count[1]=0;	
			}  
			else if(temp==0) //
			{   
			 SD_STAT=0;
			 Max_Lun=1;
       SD_Initialize();				
			 if(f_mount(fs[0],"0:",1) == 0X0D) //挂载SD	
			 f_mkfs("0:",0,4096);
			 Mass_Block_Size[1] =512;						
			 Mass_Block_Count[1]=0xE6D000;
			} 
	}
}



void Init_Iwdg(void)
{
	/* IWDG timeout equal to 200 ms (the timeout may varies due to LSI frequency dispersion) */

	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* IWDG counter clock: 40KHz(LSI) / 32 = 1.25 KHz */
  //	IWDG_SetPrescaler(IWDG_Prescaler_32);
	
	/* IWDG counter clock: 40KHz(LSI) / 256 = 0.15625 KHz */
  	IWDG_SetPrescaler(IWDG_Prescaler_256);

	/* Set counter reload value to 1249: (2499+1)/1.25=2000ms */
	//IWDG_SetReload(2499);
	
	/* Set counter reload value to 124999: (1561+1)/0.15625=10000ms */
	IWDG_SetReload(1561);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();

	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	IWDG_Enable();
  
	/* Configures the IWDG clock mode when the MCU under Debug mode */
	DBGMCU_Config(DBGMCU_IWDG_STOP, ENABLE);
}


/*
 * 函数名：void test_detect(void)  
 * 描述  ：状态机检测
 * 输入  ：无
 * 输出  ：无	
 */
void test_detect(void) //SD卡轮询检测
{
//  u16 cur_state=0;
//	u16 temp;
//	temp=SD_Initialize();
//	cur_state = SD_STAT; 
//	if(temp==0)
//	{
//	 e0_event_function(SD_STAT);
//	}


}

//void e0_event_function(int * nxt_state)
//{   
//    int cur_state;   
//    cur_state = *nxt_state;   
//    switch(cur_state)
//    {       
//        case 0: //???1,?e0?????,s1???   
//        case 255: //??a0??;           
//        *nxt_state = 255;
//    }
//}




/*
 * 函数名：void UsbTask(void *pParameters)
 * 描述  ：USB任务
 * 输入  ：无
 * 输出  ：无	
 */
void UsbTask(void *pParameters)
{		
	USBTaskQueue = xQueueCreate((unsigned portBASE_TYPE)1, semSEMAPHORE_QUEUE_ITEM_LENGTH);
	xSemaphoreTake(USBTaskQueue,  0);

	Data_Buffer = pvPortMalloc(BULK_MAX_PACKET_SIZE*2*4*4);
	Bulk_Data_Buff = pvPortMalloc(BULK_MAX_PACKET_SIZE);
	
	vTaskDelay(1800/portTICK_RATE_MS);
	USB_Port_Set(0);
	vTaskDelay(700/portTICK_RATE_MS);
	USB_Port_Set(1);
	USB_HwInit();
	USB_Init();
  
	while(1)
	{		
		if(xSemaphoreTake(USBTaskQueue, portMAX_DELAY) == pdTRUE)
		{					
			USB_CTR_Handler();
		}
	}
}



