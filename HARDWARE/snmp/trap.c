#include "trap.h"
#include "delay.h"
#include "sensor.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "snmp.h"
#include "tcpip.h"
#include "minIni.h"

#include "dm9000.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "camera.h"
#include "adc.h"
#include "rtc.h"


#define NUM_PRIVATE_TRAP_LIST    5
extern struct snmp_msg_trap trap_msg;


struct trap_list trap_list_bank[NUM_PRIVATE_TRAP_LIST];
void vSendTrapCallback2( void * parameters );
struct trap_list * getNextFreePrivateTrapList();
void freePrivateTrapList(struct trap_list * list);


void trap_task(void * pvParameters)
{
	portTickType xLastWakeTime;
	static unsigned char trap_flag=1;
	u8_t snmpauthentraps_set= 2;
	static ip_addr_t trap_addr;
	//char inifile[] = "1:cfg.ini";
	char msg[200];
	
	
	
	struct snmp_obj_id objid = {10, {1, 3, 6, 1, 4,1,26381,1,1,1}};
	//static unsigned char msg[]  = "Alexandre_Malo-mpbc_ca";
//	static u8 msg[]  = "220,1.2,25.5,95,0,0,1,1,1,1*";
//	static u8 msg2[] = "salut simon, c'est mal";
	static u8 msglen;//= sizeof(msg);//22;

	struct snmp_varbind *vb;
	struct trap_list *vb_list;

	(void) pvParameters;
	
		
	

	/* SNMP trap destination cmd option */
//	trap_addr.addr=((u32_t)((ini_getl("snmp",	"ip3",	136,	inifile)) & 0xff) << 24) | \
//											 ((u32_t)((ini_getl("snmp",	"ip2",	1,	inifile)) & 0xff) << 16) | \
//											 ((u32_t)((ini_getl("snmp",	"ip1",	168,	inifile)) & 0xff) << 8)  | \
//												(u32_t)((ini_getl("snmp",	"ip0",	192,	inifile)) & 0xff);
	
		trap_addr.addr=((u32_t)((236) & 0xff) << 24) | \
											 ((u32_t)((0) & 0xff) << 16) | \
											 ((u32_t)((168) & 0xff) << 8)  | \
												(u32_t)((192) & 0xff);
	snmp_trap_dst_ip_set(0,&trap_addr);
	snmp_trap_dst_enable(0,trap_flag);
	snmp_set_snmpenableauthentraps(&snmpauthentraps_set);
	

  
	for ( ;; )
	{		 
    xLastWakeTime = xTaskGetTickCount();		
		vb_list = getNextFreePrivateTrapList();
		vb_list->ent_oid = &objid;
		vb_list->ent_oid=&objid;
		vb_list->spc_trap = 12;	
		sprintf(msg, "%.2f,%.2f,%.1f,%.1f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*", VOL, CUR, TEM, HUM, !WATER_STAT,DOOR_STAT,!SYS12_STAT,!BAK12_STAT,!UPS_STAT,
			!AC24_STAT,AC1_STAT,AC2_STAT,AC3_STAT,fan_STAT, alarm_STAT,light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT);
		//msglen=sizeof(msg);
		msglen=strlen(msg);
		
		vb = snmp_varbind_alloc(&objid, 4, msglen);

    
		if (vb != NULL)
		{
			 memcpy (vb->value, &msg, msglen);         
			 snmp_varbind_tail_add(&vb_list->vb_root,vb);
		}
		
//		vb = snmp_varbind_alloc(&objid, 4, msglen);
		
//		if (vb != NULL)
//		{
//			 memcpy (vb->value, &msg2, msglen);         
//			 snmp_varbind_tail_add(&vb_list->vb_root,vb);
//		} 
		
		tcpip_callback(vSendTrapCallback2, vb_list);
		//RTC_Get();//更新时间	
		
		
		//printf("tcpip_callback  over\n");
		// Wait for the next cycle.
		vTaskDelayUntil( &xLastWakeTime, 4000);
	}   
}

/*
 * 函数名：struct trap_list * getNextFreePrivateTrapList()
 * 描述  ：发送还没发送的trap  并标记已发送
 * 输入  ：无
 * 输出  ：无	
 */
struct trap_list * getNextFreePrivateTrapList()
{
	u8_t index;
	struct trap_list * result = NULL;   
	for(index = 0; index < NUM_PRIVATE_TRAP_LIST; index++)
	{
		if(!trap_list_bank[index].in_use)
		{
			 trap_list_bank[index].in_use = 1;
			 result = &trap_list_bank[index];         
			 break;
		}
	}   
	return result;  
}

/*
 * 函数名：void vSendTrapCallback2( void * parameters )
 * 描述  ：trap上报回调函数
 * 输入  ：无
 * 输出  ：无	
 */
void vSendTrapCallback2( void * parameters )
{
	struct trap_list * param;
	if( parameters != NULL )
	{
		param = (struct trap_list *) parameters;
		
		trap_msg.outvb = param->vb_root;
		
		snmp_send_trap(SNMP_GENTRAP_ENTERPRISESPC,
									 param->ent_oid,
									 param->spc_trap);
									 
		freePrivateTrapList(param);
	} 
}

/*
 * 函数名：void freePrivateTrapList(struct trap_list * list)
 * 描述  ：trap上报完释放内存
 * 输入  ：无
 * 输出  ：无	
 */
void freePrivateTrapList(struct trap_list * list)
{
	snmp_varbind_list_free(&list->vb_root);
	list->ent_oid = NULL;
	list->spc_trap = 0;
	list->in_use = 0;
}




















