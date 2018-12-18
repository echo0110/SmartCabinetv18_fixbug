#ifndef __OLED_H
#define __OLED_H	 
#include "sys.h"
//OLED显示相关定义
//修改日期：2018/01/22
//接线顺序
//CLK------PA5
//MOSI-----PA7
//D/C------PB0
//CS1------PA6
//FSO------PA4
//CS2------PB1

/* the macro definition to trigger the led on or off 
 * 1 - off
 - 0 - on
 */
#define ON  0
#define OFF 1
#define maxPages	6

//带参宏，可以像内联函数一样使用
#define lcd_cs1(a)	if (a)	\
					GPIO_SetBits(GPIOA,GPIO_Pin_6);\
					else		\
					GPIO_ResetBits(GPIOA,GPIO_Pin_6)

#define lcd_rs(a)	if (a)	\
					GPIO_SetBits(GPIOB,GPIO_Pin_0);\
					else		\
					GPIO_ResetBits(GPIOB,GPIO_Pin_0)


#define lcd_sid(a)	if (a)	\
					GPIO_SetBits(GPIOA,GPIO_Pin_7);\
					else		\
					GPIO_ResetBits(GPIOA,GPIO_Pin_7)

#define lcd_sclk(a)	if (a)	\
					GPIO_SetBits(GPIOA,GPIO_Pin_5);\
					else		\
					GPIO_ResetBits(GPIOA,GPIO_Pin_5)
#define Rom_CS(a)	if (a)	\
					GPIO_SetBits(GPIOB,GPIO_Pin_1);\
					else		\
					GPIO_ResetBits(GPIOB,GPIO_Pin_1)

#define Rom_OUT(a)	if (a)	\
					GPIO_SetBits(GPIOA,GPIO_Pin_4);\
					else		\
					GPIO_ResetBits(GPIOA,GPIO_Pin_4)

#define ROM_OUT    GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4)
					
extern u8 const logo[];
extern u8 const GSMGET[];
extern u8 const GSMLOSS[];
extern u8 const LANGET[];
extern u8 const LANLOSS[];
extern u8 const BAT[];
extern u8 const BAT1[];
extern u8 const BAT2[];
					
					
void OLED_Init(void);
void clear_screen(void);
void display_GB2312_string(u8 y, u8 x, u8 *text);
void display_128x64(const u8 *dp);
void display_string_5x7(u8 y, u8 x, u8 *text);
void transfer_command_lcd(int data1);
void display_graphic_16x16(u16 page, u16 column, const u8 *dp);
void show_oled(u8 page);

#endif
