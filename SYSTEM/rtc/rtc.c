
/*--------------------------------------------------------------------------+
| Include files                                                             |
+--------------------------------------------------------------------------*/
#include <stdio.h>

#include "FreeRTOS.h"

#include "print.h"
#include "rtc.h"
#include "semphr.h"

const u8 mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};

_calendar_obj calendar;//Ê±ÖÓ½á¹¹Ìå 

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

/* ¶¨Òå´Ó¸´Î»Ö®ºóµÄÏµÍ³ÔËĞĞµÄÊ±¼ä£¬ÒÔÃëÎªµ¥Î»£¬32Î»Êı¿ÉÒÔ¼ÆÊı136Äê */
u32 SysRunTimer;
/* ¶¨Òå´ÓÉÏµçÖ®ºóµÄÏµÍ³ÔËĞĞÊ±¼ä£¬Èí¼ş¸´Î»²»»áÇå¿Õ¸Ã¼ÆÊıÆ÷ */
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
//	_WriteTime(2018, 2, 11, 8, 30, 0);// ´Ó2018Äê2ÔÂ11ÈÕÉÏÎç8µã30·Ö0Ãë¿ªÊ¼¼ÆÊı
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
		// ÉÏµç¸´Î» Power On Reset
//		PrintS("\n\nPower On Reset");
		/* ÉÏµçºó£¬³õÊ¼»¯SysPowerTimer¼ÆÊıÆ÷µÄÖµ */
		SysPowerTimer = 0;
	}
	/* Check if the Pin Reset flag is set */
	else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
	{
		// Íâ²¿RST¹Ü½Å¸´Î»
//		PrintS("\n\nExternal Reset");
	}
	/* Check if the system has resumed from IWDG reset */
	else if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		// IWDG¿´ÃÅ¹·¸´Î»
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
´ÓRTC¼ÆÊıÆ÷ÖĞ¶ÁÈ¡µ±Ç°Ê±¼ä£¬²¢×ª»»ÎªÄê¡¢ÔÂ¡¢ÈÕ¸ñÊ½µÄÊ±¼ä
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

// ·µ»ØFATÎÄ¼şÏµÍ³ËùÈÏÊ¶µÄÈÕÆÚÊ±¼ä¸ñÊ½
// Îª·ÀÖ¹ÖĞ¶Ï¸Ä±äÊ±¼äµÄÖµ£¬²»Ê¹ÓÃÏµÍ³µÄtime±äÁ¿¶øÊÇÊ¹ÓÃ´Ó¼ÆÊıÆ÷ÖĞ¶ÁÈ¡µÄÖµ£¬¸ÃÖµ²»»á±»±ğµÄ½ø³ÌËù¸Ä±ä
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
		| ((u32)tm.Second >> 1);		//  4 -  0£¬×î´óµ½32£¬ËùÒÔĞèÒª½«ÃëÊıÓÒÒÆÒ»Î»
	return rt;
}

// Ò»´ÎĞÔĞ´ÈëÈÕÆÚºÍÊ±¼ä£¬²»¼ì²é²ÎÊı£¬ÄÚ²¿Ê¹ÓÃ
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

// ÉèÖÃÏµÍ³µ±Ç°ÈÕÆÚ (Äê£­ÔÂ£­ÈÕ)
// ·µ»Ø£º³É¹¦£ºTIME_SET_OK£¬
//       ´íÎó£º´íÎó´úÂë
int RTC_SetDate(u16 year, u8 month, u8 date)
{
	u32 counter;
	Rtc_Time tm;

	// ²ÎÊı¼ì²é
	if ((year  > 2099)
	 || (month > 12)
	 || (date  > 31))
	{
		return TIME_SET_WRONG_TIME;
	}
	if (date > MonthTable[month].Days)
	{
		if (month == 2)	// Èç¹ûÊÇÈòÄêµÄ¶şÔÂ, ÔòÓÖ¶à³öÒ»Ìì
		{
			if (IsLeapYear(year))
			{
				if (date > 29)	// ÈòÄêµÄ¶şÔÂÎª29Ìì
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
// ÅĞ¶ÏÊÇ·ñÊÇÈòÄê, ÈòÄê2ÔÂÎª29Ìì, ÕâÒ»ÄêÎª366Ìì
// year: ¹«ÔªÄê·İ
u8 IsLeapYear(u16 year)
{
	if ((!(year % 4) && (year % 100)) || !(year % 400))
	{
		return 1;
	}
	return 0;
}


//
// ¼ÆËãµ±Ç°ÎªĞÇÆÚ¼¸, ÒÔ2001Äê1ÔÂ1ÈÕÎªĞÇÆÚÒ»Îª»ù×¼
// ·µ»Ø¼ÆËãµÃµ½µÄĞÇÆÚÊı
u8 RTC_GetDayOfWeek(u16 year, u8 month, u8 date)
{
	u16 temp;

	// Ã¿Äê¶à³ö1Ìì, ÈòÄêÔò¶à³ö2Ìì
	if (year >= 2001)					// Èç¹ûÊÇ2001ÄêÒÔºó£¬Ôò¿ÉÒÔÖ±½Ó¼ÆËã
	{
		temp  = (year - 2001);
		// ÔÙ¼ÓÉÏÃ¿¸öÈòÄê¶îÍâ¶à³öµÄ1Ìì
		temp += (year - 2000)/4;
		temp -= (year - 2000)/100;
		temp += (year - 2000)/400;

		// ¼õÈ¥ÒòÎªÈòÄê¶ø¶àËãµÄÒ»Ìì
		if (month <= 2)
		{
			if (IsLeapYear(year))
			{
				temp --;
			}
		}
	}
	else								// Èç¹ûÊÇ2001ÄêÒÔÇ°£¬ÔòÏÈ¼ÆËãºÍ2001ÄêÏà²îµÄÌìÊı£¬ÔÙ²¹»ØÀ´
	{
		temp = year - 2001;				// ´ËÊ±tempÎª¸ºÊı£¬
		// ÔÙ¼õÈ¥Ã¿¸öÈòÄê¶îÍâÉÙµôµÄ1Ìì
		temp -= (2000 - year)/4;
		temp += (2000 - year)/100;
		temp -= (2000 - year)/400;

		// ¼õÈ¥ÒòÎªÈòÄê¶ø¶àËãµÄ1Ìì
		temp --;
		// µ«ÊÇÈòÄêµÄ¶şÔÂ·İÖ®ºóµÄÔÂ·İÃ»ÓĞ¶àËã£¬ËùÒÔÒª¼Ó»Ø¸Õ²Å¼õÈ¥µÄ1Ìì
		if (month > 2)
		{
			if (IsLeapYear(year))
			{
				temp ++;
			}
		}
	}
	// ¼ÓÉÏÃ¿¸öÔÂµÄ1ºÅ¶ÔÓ¦µÄĞÇÆÚÊı
	temp += MonthTable[month].Val;
	temp += (date - 1);

	// ½«¸ºÊı±äÎªÕıÊı
	while (temp > 0x7FFF)
	{
		temp += 7;						// Ã¿´Î¼Ó7£¬Ö±µ½Òç³ö±äÎªÕıÊı
	}

	// ÕâÑùµÄÊıÔÙºÍ7ÇóÓà¾ÍµÃµ½ÁËĞÇÆÚÊı
	temp %= 7;
	if (temp == 0)
		temp = 7;
	return temp;
}

// ½«STM32¼Ä´æÆ÷¸ñÊ½µÄÊ±¼äĞÅÏ¢×ª»¯Îª¿É¶ÁµÄÊ±¼äĞÅÏ¢
int _localtime(u32 timereg, Rtc_Time *tm)
{
	u32 temp;
//	u8 bLeap = 0;
	
	tm->Second = timereg % SECONDS_PER_MINUTE;	// Ãë
	timereg   /= SECONDS_PER_MINUTE;			// ·ÖÖÓÊı
	tm->Minute = timereg % MINUTES_PER_HOUR;	// ·Ö
	timereg   /= MINUTES_PER_HOUR;				// Ğ¡Ê±Êı
	tm->Hour   = timereg % HOURS_PER_DAY;		// Ê±
	timereg   /= HOURS_PER_DAY;					// ¾àÀë1970Äê01ÔÂ01ÈÕµÄÌìÊı

	/* ÒÔÏÂ¸ù¾İÌìÊıÀ´¼ÆËãµ±Ç°ÈÕÆÚ, ´Ó1968ÄêÕâÒ»ÈòÄê¿ªÊ¼ËãÆğ */
	temp = 1968;
	timereg += DAYS_PER_YEAR*2 + 1;
	while (timereg >= (DAYS_PER_YEAR*4 + 1))	// Ã¿¹ı4Äê£¬¼ÆËãÒ»´ÎÈòÄê
	{
		timereg -= (DAYS_PER_YEAR*4 + 1);
		temp += 4;
	}
	while (timereg > DAYS_PER_YEAR)
	{
		timereg -= DAYS_PER_YEAR;
		temp += 1;
	}
	tm->Year = temp;					// Äê
	temp = 1;
	/* Èç¹ûÊÇÈòÄêµÄ1ÔÂºÍ2ÔÂ·İ£¬Ôò */
	if (IsLeapYear(tm->Year) &&
		(timereg <= (MonthTable[1].Days + MonthTable[2].Days)))
	{
		timereg += 1;					// ÓÉÓÚÈòÄê£¬Ç°Ãæ¶à¼õÁË1£¬ÏÖÔÚ¼Ó»ØÀ´
		/* ¿¼ÂÇµ½ÈòÄêµÄ¶şÔÂ·İÊÇ29Ìì£¬ĞèÒªÌØÊâ´¦Àí */
		/* ÔÚÕâÀï£¬ÒòÎªÖªµÀÊÇ¶şÔÂ·İÖ®Ç°£¬ËùÒÔÖ»¼õÈ¥µÚÒ»¸öÔÂµÄÌìÊı */
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

// ½«ÎÒÃÇÊìÏ¤µÄÄê¡¢ÔÂ¡¢ÈÕ¸ñÊ½µÄÊ±¼ä×ª»»³ÉSTM32µÄÓ²¼ş¼ÆÊıÆ÷µÄÖµ
u32 _mktime(Rtc_Time *tm)
{
	u32 temp;
	int i;

	temp = tm->Year - 1970;				// Ïà¶ÔÓÚ1970ÄêËù¾­ÀúµÄÄê·İ
	temp *= DAYS_PER_YEAR;				// ³ËÒÔÃ¿ÄêµÄÌìÊı
	temp += (tm->Year - 1968)/4;		// ÔÙ¼ÓÉÏÃ¿¸öÈòÄê¶îÍâ¶à³öµÄ1Ìì£¬ÓÉÓÚÖ»ĞèÒª¿¼ÂÇ
										// 1970-2099ÄêÖ®¼ä£¬¶ø2000ÓÖÊÇÈòÄê£¬ËùÒÔ²»¿¼ÂÇ
										// 100µÄÕıÊıÄê²»ÊÇÈòÄê¶ø400µÄÕûÊıÄêÊÇÈòÄêµÄÎÊÌâ
	for (i=1; i<tm->Month; i++)		// ¼ÓÉÏ¸ÃÔÂ·İÔÚ¸ÃÄêÖĞµÄÌìÊı
	{
		temp += (u32)(MonthTable[i].Days);
	}
	if (tm->Month <= 2)					// Èç¹ûÊÇÈòÄê£¬¶øÔÂ·İĞ¡ÓÚ3ÔÂ·İ£¬ÔòĞèÒª¼õÈ¥Ö®Ç°¶à¼ÓµÄ1
	{
		if (IsLeapYear(tm->Year))
		{
			temp -= 1;
		}
	}
	temp += (tm->DayOfMonth - 1);		// ¼ÓÉÏµ±ÔÂµÄÌìÊı£¬µÃµ½¾àÀë1970Äê01ÔÂ01ÈÕËù¹ıÈ¥µÄÌìÊı
	temp *= HOURS_PER_DAY;				// ³ËÒÔÃ¿ÌìµÄĞ¡Ê±Êı£¬
	temp += tm->Hour;					// ¼ÓÉÏĞ¡Ê±Êı
	temp *= MINUTES_PER_HOUR;			// ³ËÒÔÃ¿Ğ¡Ê±60·Ö
	temp += tm->Minute;					// ¼ÓÉÏ·ÖÖÓÊı
	temp *= SECONDS_PER_MINUTE;			// ³ËÒÔÃ¿·ÖÖÓ60Ãë
	temp += tm->Second;					// ¼ÓÉÏÃëÊı£¬µÃµ½µ±Ç°Ê±¼äÀë1970Äê01ÔÂ01ÈÕ00Ê±00·Ö00ÃëËù¹ıÈ¥µÄÃëÊı
										// Õâ¾ÍÊÇÎÒÃÇ½«ÒªĞ´Èëµ½STM322µÄÓ²¼şÊ±ÖÓ¼ÆÊıÆ÷ÀïÃæµÄÖµ

	return temp;
}

// ½«ÎÒÃÇÊìÏ¤µÄÄê¡¢ÔÂ¡¢ÈÕ¸ñÊ½µÄÊ±¼ä×ª»»³ÉSTM32µÄÓ²¼ş¼ÆÊıÆ÷µÄÖµ GPSÊ±¼ä×ª»¯³ÉÊ±¼ä´Á
u32 GPS_mktime(_calendar_obj *tm)
{
	u32 temp;
	int i;

	temp = tm->w_year - 1970;				// Ïà¶ÔÓÚ1970ÄêËù¾­ÀúµÄÄê·İ
	temp *= DAYS_PER_YEAR;				// ³ËÒÔÃ¿ÄêµÄÌìÊı
	temp += (tm->w_year - 1968)/4;		// ÔÙ¼ÓÉÏÃ¿¸öÈòÄê¶îÍâ¶à³öµÄ1Ìì£¬ÓÉÓÚÖ»ĞèÒª¿¼ÂÇ
										// 1970-2099ÄêÖ¼ä£¬¶ø2000ÓÖÊÇÈòÄê£¬ËùÒÔ²»¿¼ÂÇ
										// 100µÄÕıÊıÄê²»ÊÇÈòÄê¶ø400µÄÕûÊıÄêÊÇÈòÄêµÄÎÊÌâ
	for (i=1; i<tm->w_month; i++)		// ¼ÓÉÏ¸ÃÔÂ·İÔÚ¸ÃÄêÖĞµÄÌìÊı
	{
		temp += (u32)(MonthTable[i].Days);
	}
	if (tm->w_month <= 2)					// Èç¹ûÊÇÈòÄê£¬¶øÔÂ·İĞ¡ÓÚ3ÔÂ·İ£¬ÔòĞèÒª¼õÈ¥ÖÇ°¶à¼ÓµÄ1
	{
		if (IsLeapYear(tm->w_year))
		{
			temp -= 1;
		}
	}
	temp += (tm->w_date - 1);		// ¼ÓÉÏµ±ÔÂµÄÌìÊı£¬µÃµ½¾àÀë1970Äê01ÔÂ01ÈÕËù¹ıÈ¥µÄÌìÊı
	temp *= HOURS_PER_DAY;				// ³ËÒÔÃ¿ÌìµÄĞ¡Ê±Êı£¬
	temp += tm->hour;					// ¼ÓÉÏĞ¡Ê±Êı
	temp *= MINUTES_PER_HOUR;			// ³ËÒÔÃ¿Ğ¡Ê±60·Ö
	temp += tm->min;					// ¼ÓÉÏ·ÖÖÓÊı
	temp *= SECONDS_PER_MINUTE;			// ³ËÒÔÃ¿·ÖÖÓ60Ãë
	temp += tm->sec;					// ¼ÓÉÏÃëÊı£¬µÃµ½µ±Ç°Ê±¼äÀë1970Äê01ÔÂ01ÈÕ00Ê±00·Ö00ÃëËù¹ıÈ¥µÄÃëÊı
										// Õâ¾ÍÊÇÎÒÃÇ½«ÒªĞ´Èëµ½STM322µÄÓ²¼şÊ±ÖÓ¼ÆÊıÆ÷ÀïÃæµÄÖµ

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

// ½«Ãë¼ÆÊıÆ÷µÄÖµ×ª»»Îª°üº¬ÌìÊı¡¢Ğ¡Ê±¡¢·ÖÖÓºÍÃëµÄ×Ö·û´®£¬ÓÃÓÚÏÔÊ¾
int RTC_GetTimeString(u32 sec, char *pbuf)
{
	u8 second, minute, hour;
	u16 days;
	
	second = sec % SECONDS_PER_MINUTE;	// Ãë
	sec   /= SECONDS_PER_MINUTE;		// ·ÖÖÓÊı
	minute = sec % MINUTES_PER_HOUR;	// ·Ö
	sec   /= MINUTES_PER_HOUR;			// Ğ¡Ê±Êı
	hour   = sec % HOURS_PER_DAY;		// Ê±
	days   = sec / HOURS_PER_DAY;		// ÌìÊı

	sprintf(pbuf, "%d Ìì %02d:%02d:%02d ", days, hour, minute, second);

	return 0;
}




/*
 * º¯ÊıÃû£ºµÃµ½µ±Ç°µÄÊ±¼ä
 * ÃèÊö  £º
 * ÊäÈë  £ºÎŞ
 * Êä³ö  £ºÎŞ	
 */
u8 RTC_Get(void)
{
	static u16 daycnt=0;
	u32 timecount=0; 
	u32 temp=0;
	u16 temp1=0;	  
    timecount=RTC_GetCounter();	 
 	temp=timecount/86400;   //µÃµ½ÌìÊı(ÃëÖÓÊı¶ÔÓ¦µÄ)
	if(daycnt!=temp)//³¬¹ıÒ»ÌìÁË
	{	  
		daycnt=temp;
		temp1=1970;	//´Ó1970Äê¿ªÊ¼
		while(temp>=365)
		{				 
			if(Is_Leap_Year(temp1))//ÊÇÈòÄê
			{
				if(temp>=366)temp-=366;//ÈòÄêµÄÃëÖÓÊı
				else {temp1++;break;}  
			}
			else temp-=365;	  //Æ½Äê 
			temp1++;  
		}   
		calendar.w_year=temp1;//µÃµ½Äê·İ
		temp1=0;
		while(temp>=28)//³¬¹ıÁËÒ»¸öÔÂ
		{
			if(Is_Leap_Year(calendar.w_year)&&temp1==1)//µ±ÄêÊÇ²»ÊÇÈòÄê/2ÔÂ·İ
			{
				if(temp>=29)temp-=29;//ÈòÄêµÄÃëÖÓÊı
				else break; 
			}
			else 
			{
				if(temp>=mon_table[temp1])temp-=mon_table[temp1];//Æ½Äê
				else break;
			}
			temp1++;  
		}
		calendar.w_month=temp1+1;	//µÃµ½ÔÂ·İ
		calendar.w_date=temp+1;  	//µÃµ½ÈÕÆÚ 
	}
	temp=timecount%86400;     		//µÃµ½ÃëÖÓÊı   	   
	calendar.hour=temp/3600+8;     	//Ğ¡Ê±
	calendar.min=(temp%3600)/60; 	//·ÖÖÓ	
	calendar.sec=(temp%3600)%60; 	//ÃëÖÓ
	//calendar.week=RTC_Get_Week(calendar.w_year,calendar.w_month,calendar.w_date);//»ñÈ¡ĞÇÆÚ   
	return 0;
}	 

//ÅĞ¶ÏÊÇ·ñÊÇÈòÄêº¯Êı
//ÔÂ·İ   1  2  3  4  5  6  7  8  9  10 11 12
//ÈòÄê   31 29 31 30 31 30 31 31 30 31 30 31
//·ÇÈòÄê 31 28 31 30 31 30 31 31 30 31 30 31
//ÊäÈë:Äê·İ
//Êä³ö:¸ÃÄê·İÊÇ²»ÊÇÈòÄê.1,ÊÇ.0,²»ÊÇ
u8 Is_Leap_Year(u16 year)
{			  
	if(year%4==0) //±ØĞëÄÜ±»4Õû³ı
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1;//Èç¹ûÒÔ00½áÎ²,»¹ÒªÄÜ±»400Õû³ı 	   
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
	/* ÀÛ¼ÓÏµÍ³ÔËĞĞÊ±¼ä¼ÆÊıÆ÷ */
	SYS_TIMER_INC();
}

/*--------------------------------------------------------------------------+
| End of source file                                                        |
+--------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line ------------------------*/
