// Copyright (c)2011 by Aaron, All Rights Reserved.
/*--------------------------------------------------------------------------+
|  File Name:  Printf.c, v1.0.0												|
|  Author:     aaron.xu1982@gmail.com                                       |
|  Date:       2006年02月28日												|
+---------------------------------------------------------------------------+
|  Description: 串口格式化输出函数											|
|																			|
+---------------------------------------------------------------------------+
|  Release Notes:                                                           |
|                                                                           |
|  Logs:                                                                    |
|  WHO       WHEN         WHAT                                              |
|  ---       ----------   --------------------------------------------------|
|  Aaron     2011/09/28   born                                              |
|                                                                           |
+--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------+
| Include files                                                             |
+--------------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "usart.h"
#include "print.h"

/*--------------------------------------------------------------------------+
| Type Definition & Macro                                                   |
+--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------+
| Global Variables                                                          |
+--------------------------------------------------------------------------*/
const char HexTab[] = "0123456789ABCDEF";		// ASCII-hex table
extern xSemaphoreHandle USART1_Sem;

/*--------------------------------------------------------------------------+
| Internal Variables                                                        |
+--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------+
| Function Prototype                                                        |
+--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------+
| General Subroutines                                                       |
+--------------------------------------------------------------------------*/
void PrintC(char c)
{
	SerialPutChar(c);
}

// 从串口显示字符串，
void PrintS(const char *s)
{
	xSemaphoreTake(USART1_Sem, portMAX_DELAY);

	SerialPutString((const u16 *)s, 0);

	xSemaphoreGive(USART1_Sem);
}

// 从串口显示格式化字符串，相比库函数printf()，增加了互斥量的操作，同时减小了对堆栈的消耗
void Printf(const char *format, ...)
{
#	define MAX_TBUF	128							// 注意: 只允许接收该大小的字符串.
	char    tbuf[MAX_TBUF];
	va_list v_list;
	char    *ptr;


	va_start(v_list, format);					// Initialize variable arguments.
	vsnprintf(tbuf, MAX_TBUF, format, v_list);
	va_end(v_list);

	xSemaphoreTake(USART1_Sem, portMAX_DELAY);
	ptr= tbuf;
	while(*ptr != '\0')
	{
		SerialPutChar(*ptr++);
	}
	xSemaphoreGive(USART1_Sem);

#	undef MAX_TBUF
}

/* 打印8bit的整数 */
void Print8(u8 n)
{
	SerialPutChar(HexTab[(n & 0xF0)>>4]);
	SerialPutChar(HexTab[(n & 0x0F)]);
}

/* 打印16bit的整数 */
void Print16(u16 n)
{
	SerialPutChar(HexTab[(n & 0xF000)>>12]);
	SerialPutChar(HexTab[(n & 0x0F00)>>8]);
	SerialPutChar(HexTab[(n & 0x00F0)>>4]);
	SerialPutChar(HexTab[(n & 0x000F)]);
}

/* 打印32bit的整数 */
void Print32(u32 n)
{
	SerialPutChar(HexTab[(n & 0xF0000000)>>28]);
	SerialPutChar(HexTab[(n & 0x0F000000)>>24]);
	SerialPutChar(HexTab[(n & 0x00F00000)>>20]);
	SerialPutChar(HexTab[(n & 0x000F0000)>>16]);
	SerialPutChar(HexTab[(n & 0x0000F000)>>12]);
	SerialPutChar(HexTab[(n & 0x00000F00)>>8]);
	SerialPutChar(HexTab[(n & 0x000000F0)>>4]);
	SerialPutChar(HexTab[(n & 0x0000000F)]);
}

// PrintHex(): 以16进制形式输出一个缓冲区
// s:            要显示的数据的起始地址
// nLength:      要显示的数据的长度
// bShowAddress: 是否在最左边显示当前数据地址
// offset: 	     最左边数据地址的初始值，用于多次调用该函数显示一个大缓冲区中的数据
void PrintHex(u8 *s, u32 nLength, u8 bShowAddress, u32 offset)
{
	u32 i;
	u32 j;

	xSemaphoreTake(USART1_Sem, portMAX_DELAY);

	for (i=0; i<nLength; i++)
	{
		if (bShowAddress)
		{
			if (i == 0)
			{
				SerialPutChar(HexTab[(offset & 0xF000)>>12]);
				SerialPutChar(HexTab[(offset & 0x0F00)>>8]);
				SerialPutChar(HexTab[(offset & 0x00F0)>>4]);
				SerialPutChar(HexTab[(offset & 0x000F)]);
				SerialPutChar(':');
				SerialPutChar(' ');
			}
			else if ((i & 0x000F) == 0)		// 16的倍数
			{
#if 1			// 是否在一行二进制数据后面显示这一行二进制数据的ASCII字符
				SerialPutChar(' ');
				SerialPutChar(' ');
				SerialPutChar(' ');
				for (j=i-16; j<i; j++)
				{
					if ((s[j] >= 0x20) && (s[j] < 0x80))	// 可打印字符
						SerialPutChar(s[j]);
					else				// 不可打印的字符，则显示一个'.'
						SerialPutChar('.');	
				}
#endif
				SerialPutChar('\n');
				SerialPutChar(HexTab[((i+offset) & 0xF000)>>12]);
				SerialPutChar(HexTab[((i+offset) & 0x0F00)>>8]);
				SerialPutChar(HexTab[((i+offset) & 0x00F0)>>4]);
				SerialPutChar(HexTab[((i+offset) & 0x000F)]);
				SerialPutChar(':');
				SerialPutChar(' ');
			}
			else if ((i & 0x0007) == 0)		// 8的倍数
			{
				SerialPutChar(' ');
				SerialPutChar(' ');
			}
			else
			{
				SerialPutChar(' ');
			}
		}
		else
		{
			if (i == 0)
			{
			}
			else if ((i & 0x000F) == 0)		// 16的倍数
			{
				SerialPutChar('\n');
			}
			else if ((i & 0x0007) == 0)		// 8的倍数
			{
				SerialPutChar(' ');
				SerialPutChar(' ');
			}
			else
			{
				SerialPutChar(' ');
			}
		}
		SerialPutChar(HexTab[(s[i]>>4) & 0x0F]);
		SerialPutChar(HexTab[s[i] & 0x0F]);
	}	
#if 1
	if (bShowAddress)
	{
		if ((i & 0x0F) > 0)
		{
			if ((i & 0x0F) <= 8)
				SerialPutChar(' ');			// 多出每一行中间的一个空格的宽度
			for (j=(i&0x0F); j<16; j++)		// 将这一行的空格补齐
			{
				SerialPutChar(' ');
				SerialPutChar(' ');
				SerialPutChar(' ');
			}
			SerialPutChar(' ');
			SerialPutChar(' ');
			SerialPutChar(' ');
			for (j=(i & ~0x0F); j<i; j++)
			{
				if ((s[j] >= 0x20) && (s[j] < 0x80))	// 可打印字符
					SerialPutChar(s[j]);
				else						// 不可打印的字符，则显示一个'.'
					SerialPutChar('.');	
			}
		}
		else
		{
			SerialPutChar(' ');
			SerialPutChar(' ');
			SerialPutChar(' ');
			for (j=i-16; j<i; j++)
			{
				if ((s[j] >= 0x20) && (s[j] < 0x80))	// 可打印字符
					SerialPutChar(s[j]);
				else						// 不可打印的字符，则显示一个'.'
					SerialPutChar('.');	
			}
		}
	}
#endif
	SerialPutChar('\n');

	xSemaphoreGive(USART1_Sem);
}

/*--------------------------------------------------------------------------+
| End of source file                                                        |
+--------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line ------------------------*/
