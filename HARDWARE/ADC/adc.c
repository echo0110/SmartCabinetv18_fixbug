#include "adc.h"
#include "delay.h"
#include "sensor.h"
#include "minini.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "camera.h"

float VOL, CUR;
extern TaskHandle_t USBTask_Handler;

void  Adc_Init(void)
{ 	
	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC |RCC_APB2Periph_ADC1, ENABLE );

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);									// 设置ADC分频因子6 72M/6=12,ADC最大时间不能超过14M

	// PC1-电压、PA0电流           
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;						// 模拟输入引脚  电压
	GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;						// 模拟输入引脚  电流
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
		
	

	ADC_DeInit(ADC1);  // 复位ADC1 

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;					// ADC工作模式:ADC1和ADC2工作在独立模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;						// 模数转换工作在单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;					// 模数转换工作在单次转换模式
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	// 转换由软件而不是外部触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;				// ADC数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;								// 顺序进行规则转换的ADC通道的数目
	ADC_Init(ADC1, &ADC_InitStructure);									// 根据ADC_InitStruct中指定的参数初始化外设ADCx的寄存器   

  
	ADC_Cmd(ADC1, ENABLE);						// 使能指定的ADC1
	
	ADC_ResetCalibration(ADC1);					// 使能复位校准  

	while(ADC_GetResetCalibrationStatus(ADC1));	// 等待复位校准结束
	
	ADC_StartCalibration(ADC1);					//开启AD校准
 
	while(ADC_GetCalibrationStatus(ADC1));		//等待校准结束
 
//	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//使能指定的ADC1的软件转换启动功能

}

//获得ADC值，ch:通道值 0~3
u16 Get_Adc(u8 ch)
{
  	// 设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	// ADC1,ADC通道,采样时间为239.5周期	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);								// 使能指定的ADC1的软件转换启动功能	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));						// 等待转换结束

	return ADC_GetConversionValue(ADC1);								// 返回最近一次ADC1规则组的转换结果
}

u16 Get_Adc_Average(u8 ch,u8 times)
{ 
	u32 temp_val=0;
	u16 temp,i;
	for(i=0;i<1000;i++)
	{
//		temp_val+=Get_Adc(ch);
		if(Get_Adc(ch)>temp_val) temp_val=Get_Adc(ch);
		//delay_ms(1);
	}
	return temp_val;
} 	 

void Get_Vol(void)
{
	u16 adcx;	
	u16 temp;
	//adcx = Get_Adc_Average(ADC_Channel_11, 20); //PC1
	temp= Get_Adc_Average(ADC_Channel_11, 20); //PC1
	VOL = (float)temp*3.3/4096*180;
	if((VOL>600)||(VOL<80))VOL = 0;
}

void Get_Cur(void)
{
	u16 adcx;
	
	adcx = Get_Adc_Average(ADC_Channel_0, 20);//PA0
	//CUR = ((float)adcx*3.3/4096-0.02)*1000;
	CUR = ((float)adcx*3.3/4096)*3.75;
	if((CUR<0) || (VOL==0))CUR = 0;
}
/*
 * 函数名：void output_control_default(void)
 * 描述  ：默认输出控制
 * 输入  ：无
 * 输出  ：无	
 */
void output_control_default(void)
{
	char inifile[] = "1:cfg.ini";
	
	if((ini_getl("ctr",	"L1",	0,	inifile))) 
	{		
		OUT_AC1_220V_ON();
		AC1_STAT=1;
	}			
	if(!(ini_getl("ctr","L1",	0,	inifile)))
	{		
	  OUT_AC1_220V_OFF();
		AC1_STAT=0;
	}    
  if((ini_getl("ctr",	"L1",	0,	inifile))==2) {};		

	if((ini_getl("ctr",	"L2",	0,	inifile))) 
	{	 
		OUT_AC2_220V_ON();
		AC2_STAT=1;
	}		
	if(!(ini_getl("ctr","L2",	0,	inifile)))
	{	  
		OUT_AC2_220V_OFF();
    AC2_STAT=0;		
	}    
	if((ini_getl("ctr",	"L2",	0,	inifile))==2) {};	

	if((ini_getl("ctr",	"L3",	0,	inifile))) 
	{	 
		OUT_AC3_220V_ON();
		AC3_STAT=1;
	}			
	if(!(ini_getl("ctr","L3",	0,	inifile))) 
	{	
	 OUT_AC3_220V_OFF();
	 AC3_STAT=0;
	}			
	if((ini_getl("ctr",	"L3",	0,	inifile))==2) {};	

	if((ini_getl("ctr",	"V1",	0,	inifile)))
	{	
	 DC1_ON();
	 DC1_STAT=1;
	}    	
	if(!(ini_getl("ctr","V1",	0,	inifile))) 
	{
		DC1_STAT=0;
	  DC1_OFF();
	}		
  if((ini_getl("ctr",	"V1",	0,	inifile))==2) {};			

	if((ini_getl("ctr",	"V2",	0,	inifile)))
	{
		DC2_STAT=1;
		DC2_ON();	
	} 	
	if(!(ini_getl("ctr","V2",	0,	inifile)))
	{
	  DC2_STAT=0;
	  DC2_OFF();
	}
	if((ini_getl("ctr",	"V2",	0,	inifile))==2) {};	

	if((ini_getl("ctr",	"V3",	0,	inifile)))
	{
	 DC3_STAT=1;
	 DC3_ON();	
	}   
	if(!(ini_getl("ctr","V3",	0,	inifile)))
	{
	 DC3_STAT=0;
	 DC3_OFF();		
	}   
  if((ini_getl("ctr",	"V3",	0,	inifile))==2) {};	

		
	//DC4	
  if((ini_getl("ctr",	"V4",	0,	inifile)))
	{
	 DC4_STAT=1;
	 DC4_ON();	
	}   
	if(!(ini_getl("ctr","V4",	0,	inifile)))
	{
	 DC4_STAT=0;
	 DC4_OFF();		
	}   
  if((ini_getl("ctr",	"V4",	0,	inifile))==2) {};			

//	if((ini_getl("ctr","light",	0,	inifile)))  light_OFF();
//	if(!(ini_getl("ctr","light",0,	inifile)))light_ON();	
//	if((ini_getl("ctr",	"light",2,	inifile))==2) {};	

//	if(ini_getl("ctr",	"fan",	2,	inifile))   out_fan_OFF();
//	if(!(ini_getl("ctr","fan",	2,	inifile)))  out_fan_ON();
//	if((ini_getl("ctr",	"fan",	2,	inifile))==2) {};	
}

/*
 * 函数名：void read_Flooding(void)
 * 描述  ：水浸控制
 * 输入  ：无
 * 输出  ：无	
 */
void read_Flooding(void)
{
	if(GPIO_ReadOutputDataBit(GPIOB,GPIO_Pin_7)==0) //水浸
	{
	 GPIO_SetBits(GPIOE,GPIO_Pin_6);//关L1
   GPIO_SetBits(GPIOF,GPIO_Pin_7);//关L2					 
	 GPIO_SetBits(GPIOF,GPIO_Pin_9);//关L3 
	}
}

























