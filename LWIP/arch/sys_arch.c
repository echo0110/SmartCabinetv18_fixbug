/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
/* lwIP includes. */
#define SYS_ARCH_GLOBALS
 
/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/lwip_sys.h"
#include "lwip/mem.h"
#include "delay.h"
#include "arch/sys_arch.h"

const uint32_t NullMessage;

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	if(size > MAX_QUEUE_ENTRIES)size = MAX_QUEUE_ENTRIES;
 	mbox->xQueue = xQueueCreate(size, sizeof(void *));

	if(mbox->xQueue != NULL)return ERR_OK;
	else return ERR_MEM;
} 

void sys_mbox_free(sys_mbox_t *mbox)
{
	vQueueDelete(mbox->xQueue);
	mbox->xQueue=NULL;
}

void sys_mbox_post(sys_mbox_t *mbox,void* msg)
{	 
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(msg==NULL)msg = (void*)&NullMessage;
	if((SCB_ICSR_REG&0xFF) == 0)  //线程执行
	{
		while(xQueueSendToBack(mbox->xQueue, &msg, portMAX_DELAY) != pdPASS);//portMAX_DELAY,死等直到发送成功
	}
	else
	{
		while(xQueueSendToBackFromISR(mbox->xQueue, &msg, &xHigherPriorityTaskWoken) != pdPASS);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
//	while (xQueueSendToBack(mbox->xQueue, &msg, portMAX_DELAY) != pdPASS);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{ 
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(msg==NULL)msg = (void*)&NullMessage;
	if((SCB_ICSR_REG&0xFF) == 0)
	{
		if(xQueueSendToBack(mbox->xQueue, &msg, 0)!= pdPASS)
			return ERR_MEM;
	}
	else
	{
		if(xQueueSendToBackFromISR(mbox->xQueue, &msg, &xHigherPriorityTaskWoken)!= pdPASS)
			return ERR_MEM;
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
//	if(xQueueSendToBack(mbox->xQueue, &msg, 0)!= pdPASS)return ERR_MEM;
	return ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{ 
	u32_t rtos_timeout,timeout_new;
	BaseType_t temp;
	
	temp=xQueueReceive(mbox->xQueue, msg, 0);
	if((temp == pdPASS)&&(*msg != NULL))
	{	
		if(*msg == (void*)&NullMessage) *msg = NULL;
		return 0;
	}
	
	if(timeout!=0)
	{
		rtos_timeout=(timeout*configTICK_RATE_HZ)/1000; //转换为节拍数,因为freertos延时使用的是节拍数,而LWIP是用ms
		if(rtos_timeout<1)
		{
			rtos_timeout=1;//至少1个节拍
		}
		else if(rtos_timeout >= portMAX_DELAY)
		{
			rtos_timeout = portMAX_DELAY - 1;
		}
	}else rtos_timeout = 0; 
	timeout =xTaskGetTickCount(); //获取系统时间 
	if(rtos_timeout != 0)
	  temp = xQueueReceive(mbox->xQueue, msg, rtos_timeout); //请求消息队列,等待时限为rtos_timeout
	else
		temp = xQueueReceive(mbox->xQueue, msg, portMAX_DELAY);//为0则无限等

	if(temp == errQUEUE_EMPTY)
	{
		timeout=SYS_ARCH_TIMEOUT;  //请求超时
		*msg = NULL;
	}
	else
	{
		if(*msg!=NULL)
	  {	
		  if(*msg == (void*)&NullMessage) 
				*msg = NULL;   	
	  }    
		timeout_new=xTaskGetTickCount();
		if (timeout_new>timeout) timeout_new = timeout_new - timeout;//算出请求消息或使用的时间
		else timeout_new = 0xffffffff - timeout + timeout_new; 
		timeout=timeout_new*1000/configTICK_RATE_HZ + 1; 
	}
	return timeout;
	
/*	void *dummyptr;
	portTickType StartTime, EndTime, Elapsed;

	StartTime = xTaskGetTickCount();

	if (msg == NULL)msg = &dummyptr;

	if (timeout != 0)
	{
		if (xQueueReceive(mbox->xQueue, &(*msg), timeout / portTICK_RATE_MS) == pdTRUE)
		{
			EndTime = xTaskGetTickCount();
			Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

			return (Elapsed);
		}
		else // timed out blocking for message
		{
			*msg = NULL;

			return SYS_ARCH_TIMEOUT;
		}
	}
	else // block forever for a message.
	{
		while (xQueueReceive(mbox->xQueue, &(*msg), portMAX_DELAY) != pdTRUE); // time is arbitrary
		EndTime = xTaskGetTickCount();
		Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

		return (Elapsed); // return time blocked TODO test
	}*/
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	BaseType_t temp;
	
	temp=xQueueReceive(mbox->xQueue, msg, 0);
	if((temp == pdPASS)&&(*msg != NULL))
	{	
		if(*msg == (void*)&NullMessage) *msg = NULL;
		return 0;
	}
	else
	{
		return SYS_MBOX_EMPTY;
	}
/*	void *dummyptr;

	if (msg == NULL)msg = &dummyptr;
	if (pdTRUE == xQueueReceive(mbox, &(*msg), 0))return ERR_OK;
	else return SYS_MBOX_EMPTY;*/
}

//检查一个消息邮箱是否有效
//*mbox:消息邮箱
//返回值:1,有效.
//      0,无效
int sys_mbox_valid(sys_mbox_t *mbox)
{  
	if(mbox->xQueue != NULL)
	{
	   return 1;
	}		
	return 0;
} 

//设置一个消息邮箱为无效
//*mbox:消息邮箱
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	mbox->xQueue=NULL;
} 

//创建一个信号量
//*sem:创建的信号量
//count:信号量值
//返回值:ERR_OK,创建OK
// 	     ERR_MEM,创建失败
err_t sys_sem_new(sys_sem_t* sem, u8_t count)
{   
	*sem=xSemaphoreCreateCounting(0xFF, count);
	if(*sem==NULL)return ERR_MEM; 
	LWIP_ASSERT("OSSemCreate ",*sem != NULL );
	return ERR_OK;
} 

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{ 
	u32_t rtos_timeout, timeout_new;
	BaseType_t temp;
  if(xSemaphoreTake(*sem, 0) == pdPASS)
	{
		return 0;
	}		
	if(	timeout!=0) 
	{
		rtos_timeout = (timeout * configTICK_RATE_HZ) / 1000;//转换为节拍数,因为UCOS延时使用的是节拍数,而LWIP是用ms
		if(rtos_timeout < 1)
		rtos_timeout = 1;
	}else rtos_timeout = 0; 
	timeout = xTaskGetTickCount(); 
  if(rtos_timeout	!= 0)
	{
		temp = xSemaphoreTake(*sem, rtos_timeout);
	}
	else
	{
		temp = xSemaphoreTake(*sem, portMAX_DELAY);
	}
 	if(temp != pdPASS)
	{
		timeout=SYS_ARCH_TIMEOUT;//请求超时
	}		
	else
	{     
 		timeout_new = xTaskGetTickCount(); 
		if (timeout_new>=timeout) timeout_new = timeout_new - timeout;
		else timeout_new = 0xffffffff - timeout + timeout_new;
 		timeout = (timeout_new*1000/configTICK_RATE_HZ + 1);//算出请求消息或使用的时间(ms)
	}
	return timeout;
/*	portTickType StartTime, EndTime, Elapsed;

	StartTime = xTaskGetTickCount();

	if (timeout != 0)
	{
		if (xSemaphoreTake(*sem, timeout / portTICK_RATE_MS) == pdTRUE)
		{
			EndTime = xTaskGetTickCount();
			Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

			return (Elapsed); // return time blocked TODO test
		}
		else
		{
			return SYS_ARCH_TIMEOUT;
		}
	}
	else // must block without a timeout
	{
		while (xSemaphoreTake(*sem, portMAX_DELAY) != pdTRUE);
		EndTime = xTaskGetTickCount();
		Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

		return (Elapsed); // return time blocked

	}*/
}

void sys_sem_signal(sys_sem_t *sem)
{
	while(xSemaphoreGive(*sem) != pdTRUE);
}

void sys_sem_free(sys_sem_t *sem)
{
	vSemaphoreDelete(*sem); 
	*sem = NULL;
}

//查询一个信号量的状态,无效或有效
//sem:信号量指针
//返回值:1,有效.
//      0,无效
int sys_sem_valid(sys_sem_t *sem)
{
	if(*sem != NULL)
    return 1;
  else
    return 0;		
} 

//设置一个信号量无效
//sem:信号量指针
void sys_sem_set_invalid(sys_sem_t *sem)
{
	*sem=NULL;
} 

//arch初始化
void sys_init(void)
{ 
    //这里,我们在该函数,不做任何事情
} 


/*TaskHandle_t LWIP_ThreadHandler;
//创建一个新进程
//name:进程名称
//thred:进程任务函数
//arg:进程任务函数的参数
//stacksize:进程任务的堆栈大小
//prio:进程任务的优先级
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	if(strcmp(name,TCPIP_THREAD_NAME)==0)//创建TCP IP内核任务
	{
	taskENTER_CRITICAL();  //进入临界区 
	xTaskCreate((TaskFunction_t)thread,
						(const char*  )name,
						(uint16_t     )stacksize,
						(void*        )NULL,
						(UBaseType_t  )prio,
						(TaskHandle_t*)&LWIP_ThreadHandler);//创建TCP IP内核任务 
	taskEXIT_CRITICAL();  //退出临界区
	}
	return 0;
} 
*/
sys_thread_t sys_thread_new(char *name, void (* thread)(void *arg), void *arg, int stacksize, int prio)
{
xTaskHandle CreatedTask;
int result;

 //  if (s_nextthread < SYS_THREAD_MAX)
   {
      result = xTaskCreate(thread, name, stacksize, arg, prio, &CreatedTask);

	   // For each task created, store the task handle (pid) in the timers array.
	   // This scheme doesn't allow for threads to be deleted
//	   s_timeoutlist[s_nextthread++].pid = CreatedTask;

	   if(result == pdPASS)
	   {
		   return CreatedTask;
	   }
	   else
	   {
		   return NULL;
	   }
   }
//   else
   {
//      return NULL;
   }
}

//lwip延时函数
//ms:要延时的ms数
//void sys_msleep(u32_t ms)
//{
//	delay_ms(ms);
//}

//获取系统时间,LWIP1.4.1增加的函数
//返回值:当前系统时间(单位:毫秒)
u32_t sys_now(void)
{
	u32_t lwip_time;
	lwip_time=(xTaskGetTickCount()*1000/configTICK_RATE_HZ+1);//将节拍数转换为LWIP的时间MS
	return lwip_time; 		//返回lwip_time;
}

/********************************--ADD--************************************/
/*用在cc.h的SYS_ARCH_PROTECT(lev)*/
uint32_t Enter_Critical(void)
{
	if(SCB_ICSR_REG&0xFF)//在中断里
	{
		return taskENTER_CRITICAL_FROM_ISR();
	}
	else  //在线程
	{
		taskENTER_CRITICAL();
		return 0;
	}
}
/*用在cc.YS_ARCH_UNPROTECT(lev)*/
void Exit_Critical(uint32_t lev)
{
	if(SCB_ICSR_REG&0xFF)//在中断里
	{
		taskEXIT_CRITICAL_FROM_ISR(lev);
	}
	else  //在线程
	{
		taskEXIT_CRITICAL();
	}
}





