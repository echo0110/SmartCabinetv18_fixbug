/*------------------------------------------------------------------------------+
|  File Name:  Shell.c, v1.1.1													|
|  Author:     aaron.xu1982@gmail.com                                         	|
|  Date:       2011.09.25                                                     	|
+-------------------------------------------------------------------------------+
|  Description: 超级终端命令行驱动程序，支持命令的回退功能。					|
|                                                                             	|
+-------------------------------------------------------------------------------+
|  Release Notes:                                                             	|
|                                                                             	|
|  Logs:                                                                      	|
|  WHO       WHEN         WHAT                                                	|
|  ---       ----------   ------------------------------------------------------|
|  Aaron     2008.05.04   born                                                	|
|  Aaron     2011.06.22   修改为通用的命令行模式                              	|
|  Aaron     2011.09.25   移植到STM32单片机和FreeRTOS操作系统环境下           	|
|                                                                             	|
+------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------+
| Include files                                                               	|
+------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


/* Library includes. */
//#include "stm32f10x_lib.h"

//#include "SD_SPI.h"
//#include "SD_SDIO.h"
//#include "spi_flash.h"

//#include "BKP.h"
#include "rtc.h"
#include "USART.h"
#include "print.h"
#include "Shell.h"
//#include "config.h"
#include "ff.h"
#include "fattester.h"

/*----------------------------------------------------------------------------+
| Type Definition & Macro                                                     |
+----------------------------------------------------------------------------*/
// 缓冲区定义
#define MAX_CMD_LEN				64		// 命令行允许输入字符的最大长度
#define MAX_CMD_BUF_BACKS		8		// 命令行状态支持回退的最高次数
#define MAX_CMD_BUF_LEN			128		// 存放回退命令的缓冲区的长度
#define MAX_CMD_ARGS			8		// 命令行最多能带8个参数

// 和超级终端相关的宏定义
#define KEY_BACKSPACE	0x08
#define KEY_TABLE		0x09			// Table键
#define KEY_UP_ARROW	"\x1B\x5B\x41"
#define KEY_DOWN_ARROW	"\x1B\x5B\x42"
#define KEY_RIGHT_ARROW	"\x1B\x5B\x43"
#define KEY_LEFT_ARROW	"\x1B\x5B\x44"
#define KEY_HOME		"\x1B\x5B\x48"
#define KEY_END			"\x1B\x5B\x4B"

typedef int (*cmdproc)(int argc, char *argv[]);
typedef struct {
	char *cmd;
	char *hlp;
	cmdproc proc;
}t_CMD;

/*----------------------------------------------------------------------------+
| Function Prototype                                                          |
+----------------------------------------------------------------------------*/

int CMD_Serial(int argc, char *argv[]);

/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/
//extern u32 BeepTimer;		// 启动蜂鸣器的时间，以系统心跳周期为单位，在定时器中断中进行判断和处理

const u8 *StrDayOfWeek[] = {
	"日",		//"Sun", 
	"一",		//"Mon", 
	"二",		//"Tues", 
	"三",		//"Wed", 
	"四",		//"Thu", 
	"五",		//"Fri", 
	"六",		//"Sat",  
	"日",		//"Sun"
};

const t_CMD CMD_INNER[] = {
	{"?",		"help", 								CMD_Help	},
 	{"help",	"Show this list",						CMD_Help	},
	{"date", 	"Show or set current date: YY-MM-DD",	CMD_GetDate	},
	{"time", 	"Show or set current time: HH.MM.SS",	CMD_GetTime	},
	{"clock", 	"Show system running clock",			CMD_SysClk	},
	{"serial",	"Show the device serial number",		CMD_Serial	},
	{"dir", 	"Show Directory content",				CMD_Dir		},
	{"type",	"display a file content",				CMD_Type	},
	{"fdump", 	"dump a file content",					CMD_FDump	},
//	{"test",	"get leds status",						CMD_Test	},
	{"cls",		"clean screen",							CMD_Cls		},
	{NULL, NULL, NULL}
};

/*----------------------------------------------------------------------------+
| Internal Variables                                                          |
+----------------------------------------------------------------------------*/
u32 BeepTimer = 0;

volatile int CMD_RxLength;				// 超级终端下当前这一行的字符长度
volatile int CMD_RxCurrentPos;			// 超级终端下当前光标的位置
char CMD_RxPreByte;						// 通过超级终端输入控制符如左、右箭头，一次按键发送3个字符，前面2个字符固定为0x1B、0x5B，如果发现这两个字符连续出现则表示下一个字符为控制符
char CMD_RxBuffer[MAX_CMD_LEN];			// 接收数据缓冲区

#if MAX_CMD_BUF_BACKS > 0
	struct t_CMD_Line CMD_Line[MAX_CMD_BUF_BACKS];	// 之前的命令的缓冲区的长度和缓冲区指针
	int  CMD_LineNum;					// 之前接收到的命令行的数目，防止刚开机的时候回退命令过头
	int  CMD_CurrentLine;				// 当前所在的命令行，用于超级终端下按上、下箭头时回到上次发送的命令
	char CMD_Buffer[MAX_CMD_BUF_LEN];	// 命令行缓冲区，多次接收的命令行都放在里面，通过指针定位
	int  CMD_BufferLength;				// 缓冲区中命令的总长度，为防止缓冲区溢出
#endif // MAX_CMD_BUF_BACKS

/*----------------------------------------------------------------------------+
| System Initialization Routines                                              |
+----------------------------------------------------------------------------*/
// 初始化命令行驱动程序
void Init_Shell(void)
{
	int i;

	// 初始化和串口有关的全局变量
	CMD_RxPreByte = 0;
	CMD_RxLength = 0;
	CMD_RxCurrentPos = 0;
	for (i=0; i<sizeof(CMD_RxBuffer); i++)
	{
		CMD_RxBuffer[i] = 0x00;
	}

#if MAX_CMD_BUF_BACKS > 0
	CMD_LineNum = 0;					// 刚开机的时候不能回退
	CMD_CurrentLine = 0;
	CMD_BufferLength = 0;
	for (i=0; i<MAX_CMD_BUF_BACKS; i++)
	{
		CMD_Line[i].nLength = 0;
		CMD_Line[i].Buffer = CMD_Buffer;
	}
#endif // MAX_CMD_BUF_BACKS
}

/*----------------------------------------------------------------------------+
| General Subroutines                                                         |
+----------------------------------------------------------------------------*/
int str2int(char *str)
{
	int i = 0;

	while (*str != '\0')
	{
		if ((*str < '0') || (*str > '9'))
		{
			break;
		}
		i *= 10;
		i += *str - '0';
		str ++;
	}
	return i;
}

void str2lower(char *str)
{
	while (*str != '\0')
	{
		if ((*str >= 'A') && (*str <= 'Z'))
		{
			*str += 'a' - 'A';
		}
		str ++;
	}
}

int ParseCmd(char *cmdline, int cmd_len)
{
	int i;
	int argc = 0;					// 命令行参数的个数
	int state = 0;					// 当前命令行字符是否空格
	int command_num = -1;			// 对应匹配的命令序号
	char *argv[MAX_CMD_ARGS];
	char *pBuf;

	// Parse Args -------------------------------------------------------------
	cmdline[cmd_len] = '\0';
	pBuf = cmdline;
	// 判断如果是以AT开头，则表示这是一条AT指令，发送给Modem并等待Modem返回
	if (((cmdline[0] == 'a') || (cmdline[0] == 'A'))
	 && ((cmdline[1] == 't') || (cmdline[1] == 'T')))
	{
//		if (ModemState == ModemState_Cmd)
//		{
			// 发送命令给Modem
//			USART2_PutString(cmdline);
			// 发送行结束符0x0D
//			USART2_PutChar(0x0D);
//		}
		return 0;
	}
	// find out all words in the command line
	while (*pBuf != '\0')
	{
		if (state == 0)					// 前一个字符是空格或tab
		{
			if ((*pBuf != ' ')
			 && (*pBuf != '\t'))
			{
				argv[argc] = pBuf;		// 将argv[]指向当前字符
				argc ++;
				state = 1;
			}
		}
		else							// 前一个字符不是空格，则判断当前字符是否空格
		{
			if ((*pBuf == ' ')
			 || (*pBuf == '\t'))
			{
				*pBuf = '\0';
				state = 0;
			}
		}
		pBuf ++;
		if (argc >= MAX_CMD_ARGS) break;//
	}
	if(argc == 0)
		return 0;
	// Get Matched Command ----------------------------------------------------
	for (i=0; CMD_INNER[i].cmd!=NULL; i++)
	{
		str2lower(argv[0]);
		if (strcmp(argv[0], CMD_INNER[i].cmd) == 0)	// 匹配
		{
			command_num = i;
			break;
		}
	}
	if (command_num < 0)
	{
		PrintS("\nBad command: ");		PrintS(argv[0]);
		PrintS("\nplease type 'help' or '?' for a command list");
		return -1;
	}
		
	if(CMD_INNER[command_num].proc != NULL)
	{
		return CMD_INNER[command_num].proc(argc, argv);
	}
	return 0;			
}

int CMD_Help(int argc, char *argv[])
{
	int i;	
	
	for(i=0; CMD_INNER[i].cmd != NULL; i++)
	{
		if(CMD_INNER[i].hlp!=NULL)
		{
			Printf("\n%-9s  ---  %s", CMD_INNER[i].cmd, CMD_INNER[i].hlp);
		}
	}
	
	return i;
}

int CMD_GetDate(int argc, char *argv[])
{
	BYTE temp;
	struct _date_t Date;

	Rtc_ReadTime();
	Printf("\nDate: %04d-%02d-%02d %s", RtcTime.Year, RtcTime.Month, RtcTime.DayOfMonth, StrDayOfWeek[RtcTime.DayOfWeek]);
	if(argc<2)
		return 0;

	argv[1][2] = '\0';
	Date.Year  = 2000 + str2int(&argv[1][0]);
	argv[1][5] = '\0';
	Date.Month = str2int(&argv[1][3]);
	argv[1][8] = '\0';
	Date.Date  = str2int(&argv[1][6]);

	if ((Date.Year > 2099) || (Date.Month > 12) || (Date.Date > 31))
	{
		PrintS("\ninput date error");
		return 0;
	}
	if ((temp = RTC_SetDate(Date.Year, Date.Month, Date.Date)) != TIME_SET_OK)
	{
		Printf("\nerror occured when set data: %02X", temp);
		return 0;
	}

	Printf("\nSet date: %04d-%02d-%02d", Date.Year, Date.Month, Date.Date);
	return 1;
}

int CMD_GetTime(int argc, char *argv[])
{
	BYTE temp;
	struct _time_t Time;

	Rtc_ReadTime();
	Printf("\nTime: %02d:%02d:%02d", RtcTime.Hour, RtcTime.Minute, RtcTime.Second);
	if(argc<2)
		return 0;
		
	argv[1][2] = '\0';
	Time.Hour   = str2int(&argv[1][0]);
	argv[1][5] = '\0';
	Time.Minute = str2int(&argv[1][3]);
	argv[1][8] = '\0';
	Time.Second = str2int(&argv[1][6]);

	if ((Time.Hour > 23) || (Time.Minute > 59) || (Time.Second > 59))
	{
		PrintS("\ninput time error");
		return 0;
	}
	if ((temp = RTC_SetTime(Time.Hour, Time.Minute, Time.Second)) != TIME_SET_OK)
	{
		Printf("\nerror occured when set time: %02X", temp);
		return 0;
	}

	Printf("\nSet time: %02d:%02d:%02d OK!", Time.Hour, Time.Minute, Time.Second);
	return 1;
}

int CMD_SysClk(int argc, char *argv[])
{
	u32 apbclock = 0x00;
	RCC_ClocksTypeDef RCC_ClocksStatus;

	RCC_GetClocksFreq(&RCC_ClocksStatus);
	apbclock = RCC_ClocksStatus.PCLK2_Frequency;
	Printf("\nSystem is running @ %dHz", apbclock);
	return apbclock;
}

void GetSerialNum(u8 *pBuf)
{
	u32 *pDest;
	volatile u32 *pSerial;

	pDest = (u32 *)(pBuf);
	pSerial = (volatile u32 *)(0x1FFFF7E8);	//0x1FFFF7E8

	*pDest++ = *pSerial++;
	*pDest++ = *pSerial++;
	*pDest++ = *pSerial++;
}

int CMD_Serial(int argc, char *argv[])
{
	u8 buf[12];

	GetSerialNum(buf);
	PrintS("\nSERIAL: ");
	PrintHex(buf, 12, 0, 0);
	return 0;
}

int CMD_Dir(int argc, char *argv[])
{
#if (_USE_LFN == 0)
	BYTE rt;
	u32 j, k, lTemp;
	DIR Dir;					/* Directory object */
	FILEINFO Finfo;
	int IsRoot = 0;


	j = 0;
	k = 0;
	lTemp = 0;
	if (argc < 2)
	{
		rt = f_opendir(&Dir, "\\");
		IsRoot = 1;
	}
	else
	{
		rt = f_opendir(&Dir, argv[1]);
	}
	if (rt == FR_OK)
	while (1)
	{
		PrintC('\n');
		rt = f_readdir(&Dir, &Finfo);
		if ((rt != FR_OK) || (Finfo.fname[0] == '\0')) break;
		Printf("%04d-%02d-%02d  ", (Finfo.fdate>>9)+1980, (Finfo.fdate>>5)&0x0F, (Finfo.fdate & 0x1F));
		Printf("%02d:%02d    ", (Finfo.ftime>>11), (Finfo.ftime>>5)&0x3F);
		if (Finfo.fattrib & ATTR_DIR) 	// 目录
		{
			k ++;
			PrintS("<DIR>          ");
		}
		else							// 文件
		{
			j ++;
			lTemp += Finfo.fsize;
			Printf("%14d ", Finfo.fsize);
		}
		{
		PrintS((const char *)Finfo.fname);
		}
	}

	Printf("\n%16d 个文件%12d 字节", j, lTemp);
	Printf("\n%16d 个目录", k);
	if (IsRoot)							// 如果是根目录，则计算磁盘的可用空间
	{
		FATFS *fs;
		rt = f_getfreeclust("\\", &lTemp, &fs);	// get free clusts
		lTemp *= fs->csize;		// get free sectors
		lTemp *= SECT_SIZE(fs);		// get free bytes
		Printf("%12d 可用字节", lTemp);
	}
#else
	BYTE rt;
	u32 j, k, lTemp;
	DIR Dir;					/* Directory object */
	char lfn[_MAX_LFN+1];
	FILINFO Finfo;
	int IsRoot = 0;


	j = 0;
	k = 0;
	lTemp = 0;
	if (argc < 2)
	{
		rt = f_opendir(&Dir, "\\");
		IsRoot = 1;
	}
	else
	{
		rt = f_opendir(&Dir, argv[1]);
	}
	if (rt == FR_OK)
	while (1)
	{
		PrintC('\n');
		Finfo.lfname = lfn;
		Finfo.lfsize = sizeof(lfn);
		rt = f_readdir(&Dir, &Finfo);
		if ((rt != FR_OK) || (Finfo.fname[0] == '\0')) break;
		Printf("%04d-%02d-%02d  ", (Finfo.fdate>>9)+1980, (Finfo.fdate>>5)&0x0F, (Finfo.fdate & 0x1F));
		Printf("%02d:%02d    ", (Finfo.ftime>>11), (Finfo.ftime>>5)&0x3F);
//		if (Finfo.fattrib & ATTR_DIR) 	// 目录
		if (Finfo.fattrib & 0x10)
		{
			k ++;
			PrintS("<DIR>          ");
		}
		else							// 文件
		{
			j ++;
			lTemp += Finfo.fsize;
			Printf("%14d ", Finfo.fsize);
		}
		if (Finfo.lfname[0])
		{
			PrintS((const char *)Finfo.lfname);
		}
		else
		{
			PrintS((const char *)Finfo.fname);
		}
	}
	if (rt == FR_OK)
	{
		Printf("\n%16d 个文件%12d 字节", j, lTemp);
		Printf("\n%16d 个目录", k);
		if (IsRoot)							// 如果是根目录，则计算磁盘的可用空间
		{
			FATFS *fs;
//			rt = f_getfreeclust(0, &lTemp, &fs);	// get free clusts
			lTemp *= fs->csize;			// get free sectors
//			lTemp *= SECT_SIZE(fs);		// get free bytes
			Printf("%12d 可用字节", lTemp);
		}
	}
	else
	{
		PrintS("\nDisk Read Error!");
	}
#endif
	return 0;
}

int CMD_Type(int argc, char *argv[])
{
	UINT length;
	u8 temp;
	s8 BUF[33];

	if (argc < 2)
	{
		PrintS("\n显示文本文件的内容。");
		PrintS("\nTYPE [drive:][path][filename]\n");
	}
	else
	{
		FIL* fp;
    
		fp = FileOpen(argv[1], FA_READ);
		if (fp != NULL)
		{
			PrintC('\n');
//			f_seek(fp, 0, SEEK_SET);
			while (1)
			{
				temp = f_read(fp, BUF, 32, &length);
				if (temp != FR_OK) break;
				if (length == 0) break;
				BUF[length] = '\0';
				PrintS((const char *)BUF);
			}
			FileClose(fp);
		}
	}
	return 0;
}

int CMD_FDump(int argc, char *argv[])
{
	UINT length;
	UINT offset;
	FIL* fp;
	BYTE temp;
	u8 BUF[33];

	if (argc < 2)
	{
		PrintS("\n以HEX格式显示文件的内容。");
		PrintS("\nFDUMP [drive:][path][filename]\n");
	}
	else
	{
		offset = 0;
		fp = FileOpen(argv[1], FA_READ);
		if (fp != NULL)
		{
			PrintC('\n');
//			f_seek(fp, 0, SEEK_SET);
			while (1)
			{
				temp = f_read(fp, BUF, 32, &length);
				if (temp != FR_OK) break;
				if (length == 0) break;
				PrintHex(BUF, length, 1, offset);
				offset += length;
			}
			FileClose(fp);
		}
	}
	return 0;
}

int CMD_Cls(int argc, char *argv[])
{
  SerialPutChar('\r');
  SerialPutChar(0x0c);
  return 0;
}
/*----------------------------------------------------------------------------+
| General Subroutines                                                         |
+----------------------------------------------------------------------------*/
// 接收到一行命令的处理
void CMD_RxFrame(void)
{
	u32 i;
	char *pTemp;

#if MAX_CMD_BUF_BACKS > 0
	// 如果这次的命令和上次的不一样，则保存这一次的命令
	if (CMD_RxLength > 0)
	{
		if ((CMD_RxLength != CMD_Line[0].nLength) ||			// 如果两次的命令完全一样，则不保存
			(memcmp(CMD_RxBuffer, CMD_Line[0].Buffer, CMD_RxLength) != 0))
		{
			// 首先腾出缓冲区来，使得有足够空间可以存放这一次的命令
			while (CMD_LineNum > 0)
			{
				if ((CMD_RxLength + CMD_BufferLength) < sizeof(CMD_Buffer))
				{
					break;				// 缓冲区已经足够，则跳出循环
				}
				CMD_BufferLength -= CMD_Line[CMD_LineNum-1].nLength;
				CMD_LineNum --;
			}

			if ((CMD_RxLength + CMD_BufferLength) < sizeof(CMD_Buffer))
			{							// 如果有足够空间
				if (CMD_LineNum >= MAX_CMD_BUF_BACKS)	// 需要挪出最前面的一个命令
				{
					CMD_LineNum --;
					CMD_BufferLength -= CMD_Line[CMD_LineNum].nLength;
				}

				for (i=CMD_LineNum; i>0; i--)
				{
					CMD_Line[i].nLength = CMD_Line[i-1].nLength;
					CMD_Line[i].Buffer = CMD_Line[i-1].Buffer;
				}
				CMD_Line[0].nLength = CMD_RxLength;
				CMD_Line[0].Buffer = CMD_Line[1].Buffer + CMD_Line[1].nLength;
				if (CMD_Line[0].Buffer >= (CMD_Buffer + sizeof(CMD_Buffer)))
				{
					CMD_Line[0].Buffer -= sizeof(CMD_Buffer);
				}
				// 保存这次命令到缓冲区
				pTemp = CMD_Line[0].Buffer;
				for (i=0; i<CMD_RxLength; i++)
				{
					*pTemp = CMD_RxBuffer[i];
					pTemp ++;
					if (pTemp >= (CMD_Buffer + sizeof(CMD_Buffer)))
					{
						pTemp -= sizeof(CMD_Buffer);
					}
				}
				CMD_BufferLength += CMD_RxLength;

				if (CMD_LineNum < MAX_CMD_BUF_BACKS)
				{
					CMD_LineNum ++;
				}
			}
			else						// 如果空间不足，则什么也不做
			{
			}
		}
		CMD_CurrentLine = 0;
	}
#endif // MAX_CMD_BUF_BACKS

	// 命令行解释
	ParseCmd((char *)CMD_RxBuffer, CMD_RxLength);
	PrintS("\n\\>");

	// 清空接收缓冲区
	for (i=0; i<=CMD_RxLength; i++)
	{
		CMD_RxBuffer[i] = 0x00;
	}
	CMD_RxLength = 0;
	CMD_RxCurrentPos = 0;
}

// 接收到单个字节的处理
void CMD_RxByte(char aData)
{
	char  temp;
	char *pTemp;
	u32 i;

	if (CMD_RxPreByte == 0x01)
	{
		if (aData == 0x5B)				// 出现第二个控制符
		{
			CMD_RxPreByte = 0x02;
		}
		else
		{
			CMD_RxPreByte = 0x00;
		}
	}
	else if (CMD_RxPreByte == 0x02)		// 前面连续出现过2个控制符，则这一个字符表示具体是什么控制符
	{
		if (aData == 0x41)				// Up Arrow, 回到上一次的命令
		{
#if MAX_CMD_BUF_BACKS > 0
			if (CMD_CurrentLine < CMD_LineNum)
			{
				// 删除超级终端上的当前行，先回到开始处，然后发送空格到最后，然后在回到开始
				for (i=0; i<CMD_RxCurrentPos; i++)
				{
					PrintC(KEY_BACKSPACE);
				}
				for (i=0; i<CMD_RxLength; i++)
				{
					PrintC(' ');
				}
				for (i=0; i<CMD_RxLength; i++)
				{
					PrintC(KEY_BACKSPACE);
				}
				// 拷贝缓冲区
				CMD_RxLength = CMD_Line[CMD_CurrentLine].nLength;
				CMD_RxCurrentPos = CMD_RxLength;		// 光标指向命令行末尾
				pTemp = CMD_Line[CMD_CurrentLine].Buffer;
				for (i=0; i<CMD_RxLength; i++)
				{
					temp = *pTemp;
					pTemp ++;
					if (pTemp >= (CMD_Buffer + sizeof(CMD_Buffer)))
					{
						// 如果超出缓冲区范围，则回到缓冲区开始处
						pTemp = CMD_Buffer;
					}
					CMD_RxBuffer[i] = temp;
					// 更新超级终端的显示
					PrintC(temp);
				}
				CMD_CurrentLine ++;
			}
			else
			{
				BeepTimer = SYS_TICK_RATE_HZ / 10;	// 蜂鸣器响铃100ms
			}
#endif // MAX_CMD_BUF_BACKS
		}
		else if (aData == 0x42)			// Down Arrow, 回到下一次的命令
		{
#if MAX_CMD_BUF_BACKS > 0
			if (CMD_CurrentLine > 0)
			{
				// 删除超级终端上的当前行，先回到开始处，然后发送空格到最后，然后在回到开始
				for (i=0; i<CMD_RxCurrentPos; i++)
				{
					PrintC(KEY_BACKSPACE);
				}
				for (i=0; i<CMD_RxLength; i++)
				{
					PrintC(' ');
				}
				for (i=0; i<CMD_RxLength; i++)
				{
					PrintC(KEY_BACKSPACE);
				}
				CMD_CurrentLine --;
				if (CMD_CurrentLine > 0)
				{
					// 拷贝缓冲区
					CMD_RxLength = CMD_Line[CMD_CurrentLine-1].nLength;
					CMD_RxCurrentPos = CMD_RxLength;	// 光标指向命令行末尾

					pTemp = CMD_Line[CMD_CurrentLine-1].Buffer;
					for (i=0; i<CMD_RxLength; i++)
					{
						temp = *pTemp;
						pTemp ++;
						if (pTemp >= (CMD_Buffer + sizeof(CMD_Buffer)))
						{
							// 如果超出缓冲区范围，则回到缓冲区开始处
							pTemp = CMD_Buffer;
						}
						CMD_RxBuffer[i] = temp;
						// 更新超级终端的显示
						PrintC(temp);
					}
				}
				else							// 0xFF表示回到最后一次的命令，此时命令行为空
				{
					CMD_RxLength = 0;
					CMD_RxCurrentPos = 0;
					CMD_RxBuffer[0] = 0x00;
				}
			}
			else
			{
				BeepTimer = SYS_TICK_RATE_HZ / 10;	// 蜂鸣器响铃100ms
			}
#endif // MAX_CMD_BUF_BACKS
		}
		else if (aData == 0x43)			// Rigth Arrow,
		{
			if (CMD_RxCurrentPos < CMD_RxLength)
			{
				CMD_RxCurrentPos ++;
				PrintS(KEY_RIGHT_ARROW);
			}
		}
		else if (aData == 0x44)			// Left Arrow,
		{
			if (CMD_RxCurrentPos > 0)
			{
				CMD_RxCurrentPos --;
				PrintS(KEY_LEFT_ARROW);
			}
		}
		else if (aData == 0x48)			// Home，单独处理，因为在超级终端下'Home'表示回到整个屏幕的左上角
		{
			while (CMD_RxCurrentPos)
			{
				CMD_RxCurrentPos --;
				PrintS(KEY_LEFT_ARROW);
			}
		}
		else if (aData == 0x4B)			// End，单独处理，因为在超级终端下'End'会将这一行光标后面的字符都清除掉
		{
			while (CMD_RxCurrentPos < CMD_RxLength)
			{
				CMD_RxCurrentPos ++;
				PrintS(KEY_RIGHT_ARROW);
			}
		}
		CMD_RxPreByte = 0x00;
	}
	else
	{
		if (aData == KEY_BACKSPACE)		// 退格，清除前面一个字符并将后面的字符向前移
		{
			if ((CMD_RxLength > 0) && (CMD_RxCurrentPos > 0))
			{
				// 清除光标处的字符
				for (i=CMD_RxCurrentPos; i<CMD_RxLength; i++)
				{
					CMD_RxBuffer[i-1] = CMD_RxBuffer[i];
				}
				CMD_RxCurrentPos --;
				CMD_RxLength --;
				CMD_RxBuffer[CMD_RxLength] = 0x00;
				// 左移一个字符
				PrintC(KEY_BACKSPACE);
				// 更新超级终端上的显示
				for (i=CMD_RxCurrentPos; i<CMD_RxLength; i++)
				{
					PrintC(CMD_RxBuffer[i]);
				}
				// 将最后一个字符用空格代替
				PrintC(' ');
				PrintC(KEY_BACKSPACE);
				// 将光标移回到刚才的位置
				for (i=CMD_RxLength; i>CMD_RxCurrentPos; i--)
				{
					PrintC(KEY_BACKSPACE);
				}
			}
			else
			{
				BeepTimer = SYS_TICK_RATE_HZ / 10;	// 蜂鸣器响铃100ms
			}
		}
		else if (aData == KEY_TABLE)	// Table键
		{
			BeepTimer = SYS_TICK_RATE_HZ / 10;		// 蜂鸣器响铃100ms
		}
		else if (aData == 0x0A)			// 换行，不处理
		{
		}
		else if (aData == 0x0D)			// 回车
		{
			CMD_RxBuffer[CMD_RxLength] = 0x00;		// 结束标志
			CMD_RxFrame();				// 执行处理函数
		}
		else if (aData == 0x1B)			// 控制符第一个字符
		{
			CMD_RxPreByte = 0x01;
			return;
		}
		else if (aData >= 0x20)			// 可打印字符或汉字
		{
			if (CMD_RxLength < sizeof(CMD_RxBuffer))
			{
				for (i=CMD_RxLength; i>CMD_RxCurrentPos; i--)
				{
					CMD_RxBuffer[i] = CMD_RxBuffer[i-1];
				}
				CMD_RxBuffer[CMD_RxCurrentPos] = aData;
				CMD_RxLength ++;
				CMD_RxCurrentPos ++;
				// 更新超级终端上的字符显示
				for (i=CMD_RxCurrentPos; i<=CMD_RxLength; i++)
				{
					PrintC(CMD_RxBuffer[i-1]);
				}
				// 将光标移回到插入字符的位置
				for (i=CMD_RxLength; i>CMD_RxCurrentPos; i--)
				{
					PrintS(KEY_LEFT_ARROW);
				}
			}
		}
		CMD_RxPreByte = 0x00;
	}
}

/*----------------------------------------------------------------------------+
| Interrupt Service Routines                                                  |
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| End of source file                                                          |
+----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
