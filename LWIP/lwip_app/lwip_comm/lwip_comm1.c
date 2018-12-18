#include "lwip_comm.h" 
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/init.h"
#include "ethernetif.h" 
#include "lwip/timers.h"
#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/tcpip.h" 
#include "delay.h"
#include "usart.h"  
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "webserver.h"
#include "minIni.h"
#include "message.h"

#define DHCP_THREAD_PRIORITY		tskIDLE_PRIORITY + 2 
#define DHCP_THREAD_STACK_SIZE		128
void dhcp_thread(void *pv);

#define TCPIP_THREAD_PRIORITY		tskIDLE_PRIORITY + 3
#define TCPIP_THREAD_STACK_SIZE		512
TaskHandle_t TcpipThread_Handler;
void tcpip_thread(void *pv);

#define WEB_THREAD_PRIORITY			1
#define WEB_THREAD_STACK_SIZE		320
TaskHandle_t WebThread_Handler;

__lwip_dev lwipdev;						//lwip控制结构体 
struct netif lwip_netif;				//定义一个全局的网络接口

extern u32 memp_get_memorysize(void);	//在memp.c里面定义
extern u8_t *memp_memory;				//在memp.c里面定义.
extern u8_t *ram_heap;					//在mem.c里面定义.
extern xQueueHandle MainTaskQueue;

u32 TCPTimer=0;			//TCP查询计时器
u32 ARPTimer=0;			//ARP查询计时器
u32 lwip_localtime;		//lwip本地时间计数器,单位:ms

u8 linkState;

#if LWIP_DHCP
u32 DHCPfineTimer=0;	//DHCP精细处理计时器
u32 DHCPcoarseTimer=0;	//DHCP粗糙处理计时器
#endif

// lwip中mem和memp的内存申请
// 返回值:	0,成功;
//			其他,失败
u8 lwip_comm_mem_malloc(void)
{
	u32 mempsize;
	u32 ramheapsize; 
	mempsize=memp_get_memorysize();
//	memp_memory=mymalloc(SRAMIN,mempsize);
	memp_memory = (u8_t *)pvPortMalloc(mempsize);
	
	ramheapsize=LWIP_MEM_ALIGN_SIZE(MEM_SIZE)+2*LWIP_MEM_ALIGN_SIZE(4*3)+MEM_ALIGNMENT;
//	ram_heap=mymalloc(SRAMIN,ramheapsize);
	ram_heap = (u8_t *)pvPortMalloc(ramheapsize);
	
	if(!memp_memory||!ram_heap)
	{
//		lwip_comm_mem_free();
		return 1;
	}
	return 0;	
}
//lwip中mem和memp内存释放
void lwip_comm_mem_free(void)
{ 	
//	myfree(SRAMIN,memp_memory);
//	myfree(SRAMIN,ram_heap);
}
//lwip 默认IP设置
//lwipx:lwip控制结构体指针
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
	char inifile[] = "1:cfg.ini";
	//默认远端IP为:192.168.1.100
	lwipx->mqttip[0]=ini_getl("mqtt",	"ip0",	192,	inifile);
	lwipx->mqttip[1]=ini_getl("mqtt",	"ip1",	168,	inifile);
	lwipx->mqttip[2]=ini_getl("mqtt",	"ip2",	1,		inifile);
	lwipx->mqttip[3]=ini_getl("mqtt",	"ip3",	100,	inifile);
	lwipx->mqttport =ini_getl("mqtt",	"port",	1883,	inifile);
	//MAC地址设置(高三字节固定为:s/2*2.s.i,低三字节用STM32唯一ID)
	lwipx->mac[0]=dm9000cfg.mac_addr[0];
	lwipx->mac[1]=dm9000cfg.mac_addr[1];
	lwipx->mac[2]=dm9000cfg.mac_addr[2];
	lwipx->mac[3]=dm9000cfg.mac_addr[3];
	lwipx->mac[4]=dm9000cfg.mac_addr[4];
	lwipx->mac[5]=dm9000cfg.mac_addr[5]; 
	//默认本地IP为:192.168.1.55
	lwipx->ip[0]=ini_getl("local",	"ip0",	192,	inifile);	
	lwipx->ip[1]=ini_getl("local",	"ip1",	168,	inifile);
	lwipx->ip[2]=ini_getl("local",	"ip2",	1,		inifile);
	lwipx->ip[3]=ini_getl("local",	"ip3",	55,		inifile);
	//默认子网掩码:255.255.255.0
	lwipx->netmask[0]=ini_getl("local",	"mask0",	255,	inifile);	
	lwipx->netmask[1]=ini_getl("local",	"mask1",	255,	inifile);
	lwipx->netmask[2]=ini_getl("local",	"mask2",	255,	inifile);
	lwipx->netmask[3]=ini_getl("local",	"mask3",	0,		inifile);
	//默认网关:192.168.1.1
	lwipx->gateway[0]=ini_getl("local",	"gw0",	192,	inifile);	
	lwipx->gateway[1]=ini_getl("local",	"gw1",	168,	inifile);
	lwipx->gateway[2]=ini_getl("local",	"gw2",	1,		inifile);
	lwipx->gateway[3]=ini_getl("local",	"gw3",	1,		inifile);	
	lwipx->dhcpstatus=0;//没有DHCP
}


void Enc_LinkCallbackFunc(struct netif *netif)
{
	MSG msg;

	msg.Msg = MSG_LINKSTATE_CHANGE;
	msg.wParam = NETIF_FLAG_LINK_UP;
	msg.lParam = (u32)netif;
	xQueueSend(MainTaskQueue, &msg, 0);
}

void Enc_StatusCallbackFunc(struct netif *netif)
{
	MSG msg;

	msg.Msg = MSG_LINKSTATE_CHANGE;
	msg.wParam = NETIF_FLAG_UP;
	msg.lParam = (u32)netif;
	xQueueSend(MainTaskQueue, &msg, 0);
}

static void EthernetConfigureInterface(void * param)
{
	struct netif *Netif_Init_Flag;
	struct ip_addr ipaddr; 
	struct ip_addr netmask;
	struct ip_addr gw;
	
	lwip_comm_mem_malloc();	
	DM_bSem = xSemaphoreCreateBinary();
	DM_xSem = xSemaphoreCreateMutex();
	DM9000_Init();
	lwip_comm_default_ip_set(&lwipdev);
	
	IP4_ADDR(&ipaddr,lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	IP4_ADDR(&netmask,lwipdev.netmask[0],lwipdev.netmask[1] ,lwipdev.netmask[2],lwipdev.netmask[3]);
	IP4_ADDR(&gw,lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
	
	Netif_Init_Flag=netif_add(&lwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&ethernet_input);
	if(Netif_Init_Flag != NULL)
	{
		/* init netif link callback function */
		netif_set_link_callback(&lwip_netif, Enc_LinkCallbackFunc);
		netif_set_status_callback(&lwip_netif, Enc_StatusCallbackFunc);
		
		netif_set_default(&lwip_netif);
		netif_set_up(&lwip_netif);
	}
}

void lwIPInit(void *pvParameters)
{
	// once TCP stack has been initalized, set hw and IP parameters, initialize MAC too.
	tcpip_init(EthernetConfigureInterface, pvParameters);
}

//LWIP初始化(LWIP启动的时候使用)
//返回值:0,成功
//      1,内存错误
//      2,DM9000初始化失败
//      3,网卡添加失败.
u8 lwip_comm_init(void)
{
//	u32 cpu_sr;
//	u8 err;
	struct netif *Netif_Init_Flag;
	struct ip_addr ipaddr; 
	struct ip_addr netmask;
	struct ip_addr gw;
	if(lwip_comm_mem_malloc())		return 1;
	
	DM_bSem = xSemaphoreCreateBinary();
	DM_xSem = xSemaphoreCreateMutex();
	if(DM9000_Init())return 2;
	tcpip_init(NULL,NULL);
	lwip_comm_default_ip_set(&lwipdev);

#if LWIP_DHCP		//使用动态IP
	ipaddr.addr = 0;
	netmask.addr = 0;
	gw.addr = 0;
#else				//使用静态IP
	IP4_ADDR(&ipaddr,lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	IP4_ADDR(&netmask,lwipdev.netmask[0],lwipdev.netmask[1] ,lwipdev.netmask[2],lwipdev.netmask[3]);
	IP4_ADDR(&gw,lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
#endif
	Netif_Init_Flag=netif_add(&lwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&ethernet_input);
	if(Netif_Init_Flag != NULL)
	{
		/* init netif link callback function */
	//	netif_set_link_callback(&lwip_netif, Enc_LinkCallbackFunc);
	//	netif_set_status_callback(&lwip_netif, Enc_StatusCallbackFunc);
		
		netif_set_default(&lwip_netif);
		netif_set_up(&lwip_netif);
	}
	taskENTER_CRITICAL();

/*	xTaskCreate((TaskFunction_t )tcpip_thread,				//任务函数
                (const char*    )"TCPIP",					//任务名称
                (uint16_t       )TCPIP_THREAD_STACK_SIZE,	//任务堆栈大小
                (void*          )NULL,						//传递给任务函数的参数
                (UBaseType_t    )TCPIP_THREAD_PRIORITY,		//任务优先级
                (TaskHandle_t*  )&TcpipThread_Handler);		//任务句柄
	
	xTaskCreate((TaskFunction_t )web_thread,				//任务函数
                (const char*    )"WEB",						//任务名称
                (uint16_t       )WEB_THREAD_STACK_SIZE,		//任务堆栈大小
                (void*          )NULL,						//传递给任务函数的参数
                (UBaseType_t    )WEB_THREAD_PRIORITY,		//任务优先级
                (TaskHandle_t*  )&WebThread_Handler);		//任务句柄*/

	sys_thread_new("TCPIP",	tcpip_thread,	NULL,	TCPIP_THREAD_STACK_SIZE,	TCPIP_THREAD_PRIORITY);
//	sys_thread_new("WEB",	web_thread,		NULL,	WEB_THREAD_STACK_SIZE,		WEB_THREAD_PRIORITY);
	
	taskENTER_CRITICAL();
#if	LWIP_DHCP
		lwip_comm_dhcp_creat();		//创建DHCP任务
#endif	
	return 0;//操作OK.
}
//如果使能了DHCP


//M9000数据接收处理任务
void tcpip_thread(void *pv)
{
	ethernetif_input(&lwip_netif);
}

//如果使能了DHCP
#if LWIP_DHCP

//创建DHCP任务
void lwip_comm_dhcp_creat(void)
{
	taskENTER_CRITICAL();//rtos进入临界区
	xTaskCreate((TaskFunction_t )lwip_dhcp_task,     	
                (const char*    )"lwip_dhcp_task",   	
                (uint16_t       )LWIP_DHCP_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )LWIP_DHCP_TASK_PRIO,	
                (TaskHandle_t*  )&LWIP_DHCP_INPUTTask_Handler); 
	taskENTER_CRITICAL();//rtos退出临界区
}
//删除DHCP任务
void lwip_comm_dhcp_delete(void)
{
	dhcp_stop(&lwip_netif); 		//关闭DHCP
	//OSTaskDel(LWIP_DHCP_TASK_PRIO);	//删除DHCP任务
	vTaskDelete(LWIP_DHCP_INPUTTask_Handler);//删除DHCP任务
}
//DHCP处理任务
void lwip_dhcp_task(void *pdata)
{
	u32 ip=0,netmask=0,gw=0;
	dhcp_start(&lwip_netif);//开启DHCP 
	lwipdev.dhcpstatus=0;	//正在DHCP
	printf("正在查找DHCP服务器,请稍等...........\r\n");   
	while(1)
	{ 
		printf("正在获取地址...\r\n");
		ip=lwip_netif.ip_addr.addr;		//读取新IP地址
		netmask=lwip_netif.netmask.addr;//读取子网掩码
		gw=lwip_netif.gw.addr;			//读取默认网关 
		if(ip!=0)   					//当正确读取到IP地址的时候
		{
			lwipdev.dhcpstatus=2;	//DHCP成功
 			printf("网卡en的MAC地址为:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
			//解析出通过DHCP获取到的IP地址
			lwipdev.ip[3]=(uint8_t)(ip>>24); 
			lwipdev.ip[2]=(uint8_t)(ip>>16);
			lwipdev.ip[1]=(uint8_t)(ip>>8);
			lwipdev.ip[0]=(uint8_t)(ip);
			printf("通过DHCP获取到IP地址..............%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			//解析通过DHCP获取到的子网掩码地址
			lwipdev.netmask[3]=(uint8_t)(netmask>>24);
			lwipdev.netmask[2]=(uint8_t)(netmask>>16);
			lwipdev.netmask[1]=(uint8_t)(netmask>>8);
			lwipdev.netmask[0]=(uint8_t)(netmask);
			printf("通过DHCP获取到子网掩码............%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			//解析出通过DHCP获取到的默认网关
			lwipdev.gateway[3]=(uint8_t)(gw>>24);
			lwipdev.gateway[2]=(uint8_t)(gw>>16);
			lwipdev.gateway[1]=(uint8_t)(gw>>8);
			lwipdev.gateway[0]=(uint8_t)(gw);
			printf("通过DHCP获取到的默认网关..........%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //通过DHCP服务获取IP地址失败,且超过最大尝试次数
		{  
			lwipdev.dhcpstatus=0XFF;//DHCP失败.
			//使用静态IP地址
			IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			printf("DHCP服务超时,使用静态IP地址!\r\n");
			printf("网卡en的MAC地址为:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
			printf("静态IP地址........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			printf("子网掩码..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			printf("默认网关..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}  
		delay_ms(250); //延时250ms
	}
	lwip_comm_dhcp_delete();//删除DHCP任务 
}
#endif


//如果使能了DHCP
#if  LWIP_DHCP

//DHCP处理任务
void lwip_dhcp_process_handle(void)
{
	u32 ip=0,netmask=0,gw=0;
	switch(lwipdev.dhcpstatus)
	{
		case 0: 	//开启DHCP
			dhcp_start(&lwip_netif);
			lwipdev.dhcpstatus = 1;		//等待通过DHCP获取到的地址
			printf("正在查找DHCP服务器,请稍等...........\r\n");  
			break;
		case 1:		//等待获取到IP地址
		{
			ip=lwip_netif.ip_addr.addr;		//读取新IP地址
			netmask=lwip_netif.netmask.addr;//读取子网掩码
			gw=lwip_netif.gw.addr;			//读取默认网关 
			
			if(ip!=0)			//正确获取到IP地址的时候
			{
				lwipdev.dhcpstatus=2;	//DHCP成功
				printf("网卡en的MAC地址为:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
				//解析出通过DHCP获取到的IP地址
				lwipdev.ip[3]=(uint8_t)(ip>>24); 
				lwipdev.ip[2]=(uint8_t)(ip>>16);
				lwipdev.ip[1]=(uint8_t)(ip>>8);
				lwipdev.ip[0]=(uint8_t)(ip);
				printf("通过DHCP获取到IP地址..............%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
				//解析通过DHCP获取到的子网掩码地址
				lwipdev.netmask[3]=(uint8_t)(netmask>>24);
				lwipdev.netmask[2]=(uint8_t)(netmask>>16);
				lwipdev.netmask[1]=(uint8_t)(netmask>>8);
				lwipdev.netmask[0]=(uint8_t)(netmask);
				printf("通过DHCP获取到子网掩码............%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
				//解析出通过DHCP获取到的默认网关
				lwipdev.gateway[3]=(uint8_t)(gw>>24);
				lwipdev.gateway[2]=(uint8_t)(gw>>16);
				lwipdev.gateway[1]=(uint8_t)(gw>>8);
				lwipdev.gateway[0]=(uint8_t)(gw);
				printf("通过DHCP获取到的默认网关..........%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			}else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //通过DHCP服务获取IP地址失败,且超过最大尝试次数
			{
				lwipdev.dhcpstatus=0XFF;//DHCP超时失败.
				//使用静态IP地址
				IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
				IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
				IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
				printf("DHCP服务超时,使用静态IP地址!\r\n");
				printf("网卡en的MAC地址为:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
				printf("静态IP地址........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
				printf("子网掩码..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
				printf("默认网关..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			}
		}
		break;
		default : break;
	}
}
#endif 
