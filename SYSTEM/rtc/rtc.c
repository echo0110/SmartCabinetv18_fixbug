
/*--------------------------------------------------------------------------+
| Include files                                                             |
+--------------------------------------------------------------------------*/
#include <stdio.h>

#include "FreeRTOS.h"

#include "print.h"
#include "rtc.h"
#include "semphr.h"

const u8 mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};

_calendar_obj calendar;//时钟结构体 

/*--------------------------------------------------------------------------+
| Type Definition & Macro                                                   |
+--------------------------------------------------------------------------*/

#define SECONDS_PER_YEAR	(365*24*60*60)ul
#define SECONDS_PER_DAY		(24*60*60)ul
#define DAYS_PER_YEAR		(365)
#define HOURS_PER_DAY		(24)
#define MINUTES_PER_HOUR	(60)
#define SECONDS_PER_MINUTE	(60)

#define RTCClockOutput_Enable	0
#define RTCInterrupt_Enable		1

/*--------------------------------------------------------------------------+
| Global Variables                                                          |
+--------------------------------------------------------------------------*/
Rtc_Time time;
extern xSemaphoreHandle RTC_Sem;

/* 定义从复位之后的系统运行的时间，以秒为单位，32位数可以计数136年 */
u32 SysRunTimer;
/* 定义从上电之后的系统运行时间，软件复位不会清空该计数器 */
 u32 SysPowerTimer;

/*--------------------------------------------------------------------------+
| Internal Variables                                                        |
+--------------------------------------------------------------------------*/
const struct t_Month MonthTable[] =
{
    {0,  "",    0},				// Invalid month
    {31, "Jan", 1},				// January
    {28, "Feb", 4},				// February (note leap years are handled by code)
    {31, "Mar", 4},				// March
    {30, "Apr", 7},				// April
    {31, "May", 2},				// May
    {30, "Jun", 5},				// June
    {31, "Jul", 7},				// July
    {31, "Aug", 3},				// August
    {30, "Sep", 6},				// September
    {31, "Oct", 1},				// October
    {30, "Nov", 4},				// November
    {31, "Dec", 6}				// December
};

/*--------------------------------------------------------------------------+
| Function Prototype                                                        |
+--------------------------------------------------------------------------*/
u32 _mktime(Rtc_Time *tm);
int _localtime(u32 timereg, Rtc_Time *tm);
 void _WriteTime(u16 year, u8 month, u8 date, u8 hour, u8 minute, u8 second);

/*--------------------------------------------------------------------------+
| System Initialization Routines                                            |
+--------------------------------------------------------------------------*/
/****************************************************************************
* Function Name  : RTC_Configuration
* Description    : Configures the RTC.
* Input          : None
* Output         : None
* Return         : None
****************************************************************************/
void RTC_Configuration(void)
{
	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	
	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);
	
	/* Reset Backup Domain */
	BKP_DeInit();
	
	/* Enable LSE */
	RCC_LSEConfig(RCC_LSE_ON);
	/* Wait till LSE is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) ;
	
	/* Select LSE as RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	
	/* Enable RTC Clock */
	RCC_RTCCLKCmd(ENABLE);
	
	/* Wait for RTC registers synchronization */
	RTC_WaitForSynchro();
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	
	/* Enable the RTC Second */
	RTC_ITConfig(RTC_IT_SEC, ENABLE);
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Set RTC prescaler: set RTC period to 1sec */
	RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Change the current time */
//	_WriteTime(2018, 2, 11, 8, 30, 0);// 从2018年2月11日上午8点30分0秒开始计数
	_WriteTime(1982, 6, 25, 11, 0, 0);// 
}

void RTC_Init(void)
{
#if RTCInterrupt_Enable
	NVIC_InitTypeDef NVIC_InitStructure;
#endif // RTCInterrupt_Enable

	/* Check if the Power On Reset flag is set */
	if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
	{
		// 上电复位 Power On Reset
//		PrintS("\n\nPower On Reset");
		/* 上电后，初始化SysPowerTimer计数器的值 */
		SysPowerTimer = 0;
	}
	/* Check if the Pin Reset flag is set */
	else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
	{
		// 外部RST管脚复位
//		PrintS("\n\nExternal Reset");
	}
	/* Check if the system has resumed from IWDG reset */
	else if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		// IWDG看门狗复位
//		PrintS("\n\nIWDG Reset");
	}
	else
	{
	}

	SysRunTimer = 0;

	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
//		PrintS("\nRTC not yet configured,");
		RTC_Configuration();
//		PrintS(" configured!");
		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	}
	else
	{
//		PrintS("\nNo need to configure RTC");
		
		/* Enable the RTC Second */
		RTC_ITConfig(RTC_IT_SEC, ENABLE);
	
	    /* Wait for RTC registers synchronization */
	    RTC_WaitForSynchro();
	}

	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	/* Disable the Tamper Pin */
	BKP_TamperPinCmd(DISABLE); /* To output RTCCLK/64 on Tamper pin, the tamper
								 functionality must be disabled */

#if RTCClockOutput_Enable
	/* Enable RTC Clock Output on Tamper Pin */
	BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);
#endif // RTCClockOutput_Enable

#if RTCInterrupt_Enable
	/* Enable the RTC Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_SYS_INTERRUPT_PRIORITY + 5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif // RTCInterrupt_Enable

	/* Clear reset flags */
	RCC_ClearFlag();
}

/*--------------------------------------------------------------------------+
| General Subroutines                                                       |
+--------------------------------------------------------------------------*/

/*
从RTC计数器中读取当前时间，并转换为年、月、日格式的时间
*/
u32 RTC_GetTime(Rtc_Time *tm)
{
//	xSemaphoreTake(RTC_Sem, portMAX_DELAY);
	u32 counter = RTC_GetCounter();
	if (tm == NULL)
	{
		_localtime(counter, &time);
	}
	else
	{
		_localtime(counter, tm);
	}
//	xSemaphoreGive(RTC_Sem);
	return counter;
}

// 返回FAT文件系统所认识的日期时间格式
// 为防止中断改变时间的值，不使用系统的time变量而是使用从计数器中读取的值，该值不会被别的进程所改变
u32 RTC_GetFatTime(void)
{
	u32 rt;
	u32 counter;
	Rtc_Time tm;

	counter = RTC_GetCounter();
	_localtime(counter, &tm);
	
	rt = ((tm.Year - 1980) << 25)		// 31 - 25
		| ((u32)tm.Month << 21)			// 24 - 21
		| ((u32)tm.DayOfMonth << 16)	// 20 - 16
		| ((u32)tm.Hour << 11)			// 15 - 11
		| ((u32)tm.Minute << 5)			// 10 -  5
		| ((u32)tm.Second >> 1);		//  4 -  0，最大到32，所以需要将秒数右移一位
	return rt;
}

// 一次性写入日期和时间，不检查参数，内部使用
void _WriteTime(u16 year, u8 month, u8 date, u8 hour, u8 minute, u8 second)
{
	u32 counter;

	time.Year   = year;
	time.Month  = month;
	time.DayOfMonth = date;
	time.DayOfWeek  = RTC_GetDayOfWeek(year, month, date);
	time.Hour   = hour;
	time.Minute = minute;
	time.Second = second;
	counter = _mktime(&time);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	/* Change the current time */
	RTC_SetCounter(counter);
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
}

//void RTC_SetTime(Rtc_Time *tm)
int RTC_SetTime(u8 hour, u8 minute, u8 second)
{
	u32 counter;
	Rtc_Time tm;

	if ((hour >= HOURS_PER_DAY) || (minute >= MINUTES_PER_HOUR) || (second >= SECONDS_PER_MINUTE))
	{
		return TIME_SET_WRONG_TIME;
	}
	counter = RTC_GetCounter();
	_localtime(counter, &tm);
	tm.Hour   = hour;
	tm.Minute = minute;
	tm.Second = second;
	counter = _mktime(&tm);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	/* Change the current time */
	RTC_SetCounter(counter);
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	
	return TIME_SET_OK;
}

// 设置系统当前日期 (年－月－日)
// 返回：成功：TIME_SET_OK，
//       错误：错误代码
int RTC_SetDate(u16 year, u8 month, u8 date)
{
	u32 counter;
	Rtc_Time tm;

	// 参数检查
	if ((year  > 2099)
	 || (month > 12)
	 || (date  > 31))
	{
		return TIME_SET_WRONG_TIME;
	}
	if (date > MonthTable[month].Days)
	{
		if (month == 2)	// 如果是闰年的二月, 则又多出一天
		{
			if (IsLeapYear(year))
			{
				if (date > 29)	// 闰年的二月为29天
				{
					return TIME_SET_WRONG_TIME;
				}
			}
			else
			{
				return TIME_SET_WRONG_TIME;
			}
		}
		else
		{
			return TIME_SET_WRONG_TIME;
		}
	}

	counter = RTC_GetCounter();
	_localtime(counter, &tm);
	tm.Year  = year;
	tm.Month = month;
	tm.DayOfMonth = date;
	counter = _mktime(&tm);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	/* Change the current time */
	RTC_SetCounter(counter);
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	return TIME_SET_OK;
}

//
// 判断是否是闰年, 闰年2月为29天, 这一年为366天
// year: 公元年份
u8 IsLeapYear(u16 year)
{
	if ((!(year % 4) && (year % 100)) || !(year % 400))
	{
		return 1;
	}
	return 0;
}


//
// 计算当前为星期几, 以2001年1月1日为星期一为基准
// 返回计算得到的星期数
u8 RTC_GetDayOfWeek(u16 year, u8 month, u8 date)
{
	u16 temp;

	// 每年多出1天, 闰年则多出2天
	if (year >= 2001)					// 如果是2001年以后，则可以直接计算
	{
		temp  = (year - 2001);
		// 再加上每个闰年额外多出的1天
		temp += (year - 2000)/4;
		temp -= (year - 2000)/100;
		temp += (year - 2000)/400;

		// 减去因为闰年而多算的一天
		if (month <= 2)
		{
			if (IsLeapYear(year))
			{
				temp --;
			}
		}
	}
	else								// 如果是2001年以前，则先计算和2001年相差的天数，再补回来
	{
		temp = year - 2001;				// 此时temp为负数，
		// 再减去每个闰年额外少掉的1天
		temp -= (2000 - year)/4;
		temp += (2000 - year)/100;
		temp -= (2000 - year)/400;

		// 减去因为闰年而多算的1天
		temp --;
		// 但是闰年的二月份之后的月份没有多算，所以要加回刚才减去的1天
		if (month > 2)
		{
			if (IsLeapYear(year))
			{
				temp ++;
			}
		}
	}
	// 加上每个月的1号对应的星期数
	temp += MonthTable[month].Val;
	temp += (date - 1);

	// 将负数变为正数
	while (temp > 0x7FFF)
	{
		temp += 7;						// 每次加7，直到溢出变为正数
	}

	// 这样的数再和7求余就得到了星期数
	temp %= 7;
	if (temp == 0)
		temp = 7;
	return temp;
}

// 将STM32寄存器格式的时间信息转化为可读的时间信息
int _localtime(u32 timereg, Rtc_Time *tm)
{
	u32 temp;
//	u8 bLeap = 0;
	
	tm->Second = timereg % SECONDS_PER_MINUTE;	// 秒
	timereg   /= SECONDS_PER_MINUTE;			// 分钟数
	tm->Minute = timereg % MINUTES_PER_HOUR;	// 分
	timereg   /= MINUTES_PER_HOUR;				// 小时数
	tm->Hour   = timereg % HOURS_PER_DAY;		// 时
	timereg   /= HOURS_PER_DAY;					// 距离1970年01月01日的天数

	/* 以下根据天数来计算当前日期, 从1968年这一闰年开始算起 */
	temp = 1968;
	timereg += DAYS_PER_YEAR*2 + 1;
	while (timereg >= (DAYS_PER_YEAR*4 + 1))	// 每过4年，计算一次闰年
	{
		timereg -= (DAYS_PER_YEAR*4 + 1);
		temp += 4;
	}
	while (timereg > DAYS_PER_YEAR)
	{
		timereg -= DAYS_PER_YEAR;
		temp += 1;
	}
	tm->Year = temp;					// 年
	temp = 1;
	/* 如果是闰年的1月和2月份，则 */
	if (IsLeapYear(tm->Year) &&
		(timereg <= (MonthTable[1].Days + MonthTable[2].Days)))
	{
		timereg += 1;					// 由于闰年，前面多减了1，现在加回来
		/* 考虑到闰年的二月份是29天，需要特殊处理 */
		/* 在这里，因为知道是二月份之前，所以只减去第一个月的天数 */
		if (timereg > MonthTable[1].Days)
		{
			timereg -= MonthTable[1].Days;
			temp += 1;
		}
//		bLeap = 1;
	}
	else
	{
		while (timereg > MonthTable[temp].Days)
		{
			timereg -= MonthTable[temp].Days;
			temp += 1;
		}
	}
	tm->Month  = temp;
	tm->DayOfMonth = timereg;
	tm->DayOfWeek  = RTC_GetDayOfWeek(tm->Year, tm->Month, tm->DayOfMonth);

	return 0;
}

// 将我们熟悉的年、月、日格式的时间转换成STM32的硬件计数器的值
u32 _mktime(Rtc_Time *tm)
{
	u32 temp;
	int i;

	temp = tm->Year - 1970;				// 相对于1970年所经历的年份
	temp *= DAYS_PER_YEAR;				// 乘以每年的天数
	temp += (tm->Year - 1968)/4;		// 再加上每个闰年额外多出的1天，由于只需要考虑
										// 1970-2099年之间，而2000又是闰年，所以不考虑
										// 100的正数年不是闰年而400的整数年是闰年的问题
	for (i=1; i<tm->Month; i++)		// 加上该月份在该年中的天数
	{
		temp += (u32)(MonthTable[i].Days);
	}
	if (tm->Month <= 2)					// 如果是闰年，而月份小于3月份，则需要减去之前多加的1
	{
		if (IsLeapYear(tm->Year))
		{
			temp -= 1;
		}
	}
	temp += (tm->DayOfMonth - 1);		// 加上当月的天数，得到距离1970年01月01日所过去的天数
	temp *= HOURS_PER_DAY;				// 乘以每天的小时数，
	temp += tm->Hour;					// 加上小时数
	temp *= MINUTES_PER_HOUR;			// 乘以每小时60分
	temp += tm->Minute;					// 加上分钟数
	temp *= SECONDS_PER_MINUTE;			// 乘以每分钟60秒
	temp += tm->Second;					// 加上秒数，得到当前时间离1970年01月01日00时00分00秒所过去的秒数
										// 这就是我们将要写入到STM322的硬件时钟计数器里面的值

	return temp;
}

void RTC_FormatDateStr(char *pbuf)
{
	sprintf(pbuf, "%04d-%02d-%02d ", time.Year, time.Month, time.DayOfMonth);
}

void RTC_FormatTimeStr(char *pbuf)
{
	sprintf(pbuf, "%02d:%02d:%02d ", time.Hour, time.Minute, time.Second);
}

// 将秒计数器的值转换为包含天数、小时、分钟和秒的字符串，用于显示
int RTC_GetTimeString(u32 sec, char *pbuf)
{
	u8 second, minute, hour;
	u16 days;
	
	second = sec % SECONDS_PER_MINUTE;	// 秒
	sec   /= SECONDS_PER_MINUTE;		// 分钟数
	minute = sec % MINUTES_PER_HOUR;	// 分
	sec   /= MINUTES_PER_HOUR;			// 小时数
	hour   = sec % HOURS_PER_DAY;		// 时
	days   = sec / HOURS_PER_DAY;		// 天数

	sprintf(pbuf, "%d 天 %02d:%02d:%02d ", days, hour, minute, second);

	return 0;
}




/*
 * 函数名：得到当前的时间
 * 描述  ：
 * 输入  ：无
 * 输出  ：无	
 */
u8 RTC_Get(void)
{
	static u16 daycnt=0;
	u32 timecount=0; 
	u32 temp=0;
	u16 temp1=0;	  
    timecount=RTC_GetCounter();	 
 	temp=timecount/86400;   //得到天数(秒钟数对应的)
	if(daycnt!=temp)//超过一天了
	{	  
		daycnt=temp;
		temp1=1970;	//从1970年开始
		while(temp>=365)
		{				 
			if(Is_Leap_Year(temp1))//是闰年
			{
				if(temp>=366)temp-=366;//闰年的秒钟数
				else {temp1++;break;}  
			}
			else temp-=365;	  //平年 
			temp1++;  
		}   
		calendar.w_year=temp1;//得到年份
		temp1=0;
		while(temp>=28)//超过了一个月
		{
			if(Is_Leap_Year(calendar.w_year)&&temp1==1)//当年是不是闰年/2月份
			{
				if(temp>=29)temp-=29;//闰年的秒钟数
				else break; 
			}
			else 
			{
				if(temp>=mon_table[temp1])temp-=mon_table[temp1];//平年
				else break;
			}
			temp1++;  
		}
		calendar.w_month=temp1+1;	//得到月份
		calendar.w_date=temp+1;  	//得到日期 
	}
	temp=timecount%86400;     		//得到秒钟数   	   
	calendar.hour=temp/3600+8;     	//小时
	calendar.min=(temp%3600)/60; 	//分钟	
	calendar.sec=(temp%3600)%60; 	//秒钟
	//calendar.week=RTC_Get_Week(calendar.w_year,calendar.w_month,calendar.w_date);//获取星期   
	return 0;
}	 

//判断是否是闰年函数
//月份   1  2  3  4  5  6  7  8  9  10 11 12
//闰年   31 29 31 30 31 30 31 31 30 31 30 31
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
u8 Is_Leap_Year(u16 year)
{			  
	if(year%4==0) //必须能被4整除
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1;//如果以00结尾,还要能被400整除 	   
			else return 0;   
		}else return 1;   
	}else return 0;	
}	 			   

/*--------------------------------------------------------------------------+
| Interrupt Service Routines                                                |
+--------------------------------------------------------------------------*/
void RTC_IRQHandler(void)
{
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET)
	{
		/* Clear the RTC Second interrupt */
		RTC_ClearITPendingBit(RTC_IT_SEC);
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();
	}
	/* 累加系统运行时间计数器 */
	SYS_TIMER_INC();
}

/*--------------------------------------------------------------------------+
| End of source file                                                        |
+--------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line ------------------------*/
