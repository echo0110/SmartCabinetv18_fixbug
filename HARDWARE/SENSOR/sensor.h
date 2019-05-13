#ifndef __SENSOR_H
#define __SENSOR_H	 
#include "sys.h"
#include "led.h"




// PB5	允许外设使用系统12V
#define EQU_SYS12V_EN()		{PBout(5) = 1;}
#define EQU_SYS12V_DIS()	{PBout(5) = 0;}
// PB6	允许外设使用UPS12V
#define EQU_UPS12V_EN()		{PBout(6) = 1;}
#define EQU_UPS12V_DIS()	{PBout(6) = 0;}

#define LED1()	{PAout(1) = 0;}







// PG2	系统电源  
#define SYS12_SENSOR		PGin(12)
#define SYS12_ON	0
#define SYS12_OFF	1
// PD3	备用电源
#define BAK12_SENSOR		PDin(3)
#define BAK12_ON	0
#define BAK12_OFF	1

// PD6	UPS状态检
#define UPS12_SENSOR		PDin(6)
#define UPS12_ON	0
#define UPS12_OFF	1

//	PB9	特殊电源检测
#define AC24_SENSOR		PBin(9)
#define AC24_ON		0
#define AC24_OFF	1




// PF6	AC1-220V输出控制  L1
#define AC1_SENSOR()		PFin(6)
#define OUT_AC1_220V_ON()		{PFout(6) = 0;}
#define OUT_AC1_220V_OFF()	{PFout(6) = 1;}

// PF7	AC2-220V输出控制  L2
#define AC2_SENSOR()		PFin(7)
#define OUT_AC2_220V_ON()		{PFout(7) = 0;}
#define OUT_AC2_220V_OFF()	{PFout(7) = 1;}

// PF9	AC3-220V输出控制  L3
#define AC3_SENSOR()		PFin(9)
#define OUT_AC3_220V_ON()		{PFout(9) = 0;}
#define OUT_AC3_220V_OFF()	{PFout(9) = 1;}

// PE6	风扇控制
#define out_fan_SENSOR()		PFin(6)
#define out_fan_ON()		{PEout(6) = 1;}
#define out_fan_OFF()	  {PEout(6) = 0;}

// PE5   警报
#define alarm_SENSOR()		PEin(5)
#define alarm_ON()		{PEout(5) = 0;}
#define alarm_OFF()	  {PEout(5) = 1;}

// PF8   照明
#define light_SENSOR()		PFin(8)
#define light_ON()		{PFout(8) = 1;}
#define light_OFF()	  {PFout(8) = 0;}

// PF10   加热
#define heat_SENSOR()		PFin(10)
#define heat_ON()		{PFout(10) = 0;}
#define heat_OFF()	  {PFout(10) = 1;}


// PE4   输出 DC1
#define DC1_SENSOR()		PEin(4)
#define DC1_ON()		{PEout(4) = 1;}
#define DC1_OFF()	  {PEout(4) = 0;}

// PE3   输出 DC2
#define DC2_SENSOR()		PEin(3)
#define DC2_ON()		{PEout(3) = 1;}
#define DC2_OFF()	  {PEout(3) = 0;}

// PE2   输出 DC3
#define DC3_SENSOR()		PEin(2)
#define DC3_ON()		{PEout(2) = 1;}
#define DC3_OFF()	  {PEout(2) = 0;}

// PB6  特殊电源输出控制 DC4
#define DC4_SENSOR()		PBin(6)
#define DC4_ON()		{PBout(6) = 1;}
#define DC4_OFF()	  {PBout(6) = 0;}

// PC12	温湿度传感器
#define TEM_HUM_SENSOR		PCin(12)
#define AM2301_Write_1()	{PCout(12) = 1;}
#define AM2301_Write_0()	{PCout(12) = 0;}

// PB7	水浸状态 0-浸水 1-正常
#define WATER_SENSOR		PBin(7)
#define WATER_IN	0
#define WATER_OUT	1

// PC7	门锁状态 0-开 1-关   测试使用  正式使用要改引脚
//#define DOOR_SENSOR		PCin(7)

// PD2	门锁状态 0-开 1-关
#define DOOR_SENSOR		PDin(2)
#define DOOR_OPEN		1
#define DOOR_CLOSE	0



// PG7	
//#define NET_STAT		PGin(6)	



extern float TEM, HUM;
extern u8 DOOR_STAT, WATER_STAT, SYS12_STAT, BAK12_STAT, BAT12_STAT,UPS_STAT,AC24_STAT;
extern u8 AC1_STAT, AC2_STAT, AC3_STAT, fan_STAT, alarm_STAT, light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT,out_special_STAT;
extern u8 TEM_STAT,VOL_STAT;
extern u8 NET_STAT;

extern u8 IS_EQU_SYS12V, IS_EQU_UPS12V;
extern u8 stat_changed;

extern u8 tem_stat_changed;
extern u8 vol_stat_changed;
extern u8 sys12_stat_changed;
extern u8 bak12_stat_changed,ups_stat_changed,ac24_stat_changed;

extern u8 snmp_tem_changed,snmp_vol_changed;

extern int SD_STAT;

void SENSOR_Init(void);
void STAT_CHECK(void);

void sd_out_config(void);

void sd_ipd_config(void);

extern void GET_AM2301_Data(void);
extern void DOOR_SENSOR_CHECK(void); //门开关接口扫描函数
extern void LIGHT_SENSOR_CHECK(void);

extern void SD_SENSOR_CHECK(void); //SD卡轮询检测

void SD_state_machine_CHECK(void);//SD卡轮询检测

extern void sd_detect_status(void);

extern u8 test_Select(void);
extern void Init_Iwdg(void);

extern void USB_Reset(void);

typedef struct _EP_BUF_DSCR { 
uint32_t ADDR_TX;    
uint32_t COUNT_TX;    
uint32_t ADDR_RX;   
uint32_t COUNT_RX;
} EP_BUF_DSCR;

#endif
