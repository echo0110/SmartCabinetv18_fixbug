/*******************************************************************************
** 文件名: 		mian.c
** 版本：  		1.0
** 工作环境: 	RealView MDK-ARM 4.14
** 作者: 		wuguoyana
** 生成日期: 	2011-04-29
** 功能:		LED测试程序
				程序烧写到板子上，LED2-LED5会依次点亮，闪烁
** 相关文件:	stm32f10x.h
** 修改日志：	2011-04-29 创建文档
*******************************************************************************/
/* 包含头文件 *****************************************************************/
#include "stm32f10x.h"
/* 类型声明 ------------------------------------------------------------------*/
/* 宏定义 --------------------------------------------------------------------*/
#define LED2   GPIO_Pin_6
#define LED3   GPIO_Pin_7
#define LED4   GPIO_Pin_8
#define LED5   GPIO_Pin_9
/* 变量 ----------------------------------------------------------------------*/
GPIO_InitTypeDef GPIO_InitStructure;
ErrorStatus HSEStartUpStatus;
/* 函数声明 ------------------------------------------------------------------*/
void Delay(__IO uint32_t nCount);
void LED_Configuration(void);
void RCC_HSI_Configuration(void);
/* 函数功能 ------------------------------------------------------------------*/

/*******************************************************************************
  * @函数名称	main
  * @函数说明   主函数，硬件初始化，实现LED1-LED4闪烁
  * @输入参数   无
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
int main(void)
{
    LED_Configuration();

    //设置中断向量表的位置在 0x3000
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x3000);

    while (1)
    {
        //下面通过3种方式来操作GPIO电平实现LED闪烁
        GPIO_ResetBits(GPIOB,LED2);//点亮LED1
        Delay(100);
        GPIO_SetBits(GPIOB,LED2);  //熄灭LED1
        Delay(100);
        GPIO_ResetBits(GPIOB,LED3);//点亮LED2
        Delay(100);
        GPIO_SetBits(GPIOB,LED3);  //熄灭LED2
        Delay(100);

        GPIO_WriteBit(GPIOB,LED4,Bit_RESET); //点亮LED3
        Delay(100);
        GPIO_WriteBit(GPIOB,LED4,Bit_SET);   //熄灭LED3
        Delay(100);
        //点亮LED4
        GPIO_WriteBit(GPIOB, LED5, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOB, LED5)));
        Delay(100);
        //熄灭LED4
        GPIO_WriteBit(GPIOB, LED5, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOB, LED5)));
        Delay(100);

    }
}

/*******************************************************************************
  * @函数名称	LED_Configuration
  * @函数说明   配置使用LED
  * @输入参数   无
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
void LED_Configuration(void)
{
    //使能LED所在GPIO的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);
    //初始化LED的GPIO
    GPIO_InitStructure.GPIO_Pin = LED2 | LED3 | LED4 | LED5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,LED2 | LED3 | LED4 | LED5);  //熄灭LED2-5
}

/*******************************************************************************
  * @函数名称	Delay
  * @函数说明   插入一段延时时间
  * @输入参数   nCount: 指定延时时间长度
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
void Delay(__IO uint32_t nCount)
{
    int i,j;
    for (i=0; i<nCount; i++)
        for (j=0; j<10000; j++)
            ;
}

#ifdef  USE_FULL_ASSERT
/*******************************************************************************
  * @函数名称	assert_failed
  * @函数说明   报告在检查参数发生错误时的源文件名和错误行数
  * @输入参数   file: 源文件名
  				line: 错误所在行数
  * @返回参数   无
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* 用户可以增加自己的代码用于报告错误的文件名和所在行数,
       例如：printf("错误参数值: 文件名 %s 在 %d行\r\n", file, line) */

    /* 无限循环 */
    while (1)
    {
    }
}
#endif

/***************************END OF FILE****************************************/
