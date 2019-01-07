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

#include "lwip_comm.h"


#define NUM_PRIVATE_TRAP_LIST    5
extern struct snmp_msg_trap trap_msg;


struct trap_list trap_list_bank[NUM_PRIVATE_TRAP_LIST];
extern void vSendTrapCallback2( void * parameters );
struct trap_list * getNextFreePrivateTrapList();
extern void freePrivateTrapList(struct trap_list * list);





void trap_task(void * pvParameters)
{
	portTickType xLastWakeTime;
	static unsigned char trap_flag=1;
	u8_t snmpauthentraps_set= 2;
	static ip_addr_t trap_addr;
	char msg_vol[5],msg_cur[5];
	
	
  //struct snmp_obj_id objid = {10, {1,3,6,1,4,1,1,1,2}};
	
  //struct snmp_obj_id objid[22];	
	struct snmp_obj_id objid_vol = {10, {1,3,6,1,4,1,1,1,1}};	
	struct snmp_obj_id objid_cur = {10, {1,3,6,1,4,1,1,1,2}};
	struct snmp_obj_id objid_tem = {10, {1,3,6,1,4,1,1,1,3}};	
	struct snmp_obj_id objid_hum = {10, {1,3,6,1,4,1,1,1,4}};
	struct snmp_obj_id objid_water = {10, {1,3,6,1,4,1,1,1,5}};	
	struct snmp_obj_id objid_door = {10, {1,3,6,1,4,1,1,1,6}};	
	struct snmp_obj_id objid_sys12= {10, {1,3,6,1,4,1,1,1,7}};
	struct snmp_obj_id objid_bak12 = {10, {1,3,6,1,4,1,1,1,8}};	
	struct snmp_obj_id objid_ups= {10, {1,3,6,1,4,1,1,1,9}};
	struct snmp_obj_id objid_ac24 = {10, {1,3,6,1,4,1,1,1,10}};	
	struct snmp_obj_id objid_ac1= {10, {1,3,6,1,4,1,1,1,11}};
	struct snmp_obj_id objid_ac2 = {10, {1,3,6,1,4,1,1,1,12}};	
	struct snmp_obj_id objid_ac3= {10, {1,3,6,1,4,1,1,1,13}};	
//	
	struct snmp_obj_id objid_fan= {10, {1,3,6,1,4,1,1,1,14}};
	struct snmp_obj_id objid_alarm = {10, {1,3,6,1,4,1,1,1,15}};	
	struct snmp_obj_id objid_light= {10, {1,3,6,1,4,1,1,1,16}};	
	  
	struct snmp_obj_id objid_heat= {10, {1,3,6,1,4,1,1,1,17}};
	struct snmp_obj_id objid_dc1= {10, {1,3,6,1,4,1,1,1,18}};
	struct snmp_obj_id objid_dc2 = {10, {1,3,6,1,4,1,1,1,19}};	
	struct snmp_obj_id objid_dc3= {10, {1,3,6,1,4,1,1,1,20}};
  struct snmp_obj_id objid_dc4= {10, {1,3,6,1,4,1,1,1,21}};


//	struct snmp_varbind *vb;
//	struct trap_list *vb_list;

	(void) pvParameters;
	
		
	
//	trap_addr.addr=((u32_t)((lwipdev.snmpip[3]) & 0xff) << 24) | \
//											 ((u32_t)((lwipdev.snmpip[2]) & 0xff) << 16) | \
//											 ((u32_t)((lwipdev.snmpip[1]) & 0xff) << 8)  | \
//												(u32_t)((lwipdev.snmpip[0]) & 0xff);

	trap_addr.addr=((u32_t)((210) & 0xff) << 24) | \
											 ((u32_t)((0) & 0xff) << 16) | \
											 ((u32_t)((168) & 0xff) << 8)  | \
												(u32_t)((192) & 0xff);
	snmp_trap_dst_ip_set(0,&trap_addr);
	snmp_trap_dst_enable(0,trap_flag);
	snmp_set_snmpenableauthentraps(&snmpauthentraps_set);
	

  
	for ( ;; )
	{		 
    xLastWakeTime = xTaskGetTickCount();		
//		vb_list = getNextFreePrivateTrapList();
//		
//		vb_list->spc_trap = 12;
		
    //changed_trap(&objid_tem,SYS12_STAT);
		
    if(sys12_stat_changed) 	
	  {	   
		 changed_trap(&objid_sys12,SYS12_STAT);
		 sys12_stat_changed=0;
	  }
//		if(bak12_stat_changed)
//		{
//		 bak12_stat_changed=0;
//		 changed_trap(&objid_bak12,BAK12_STAT);		
//		}			
//    if(ups_stat_changed)
//		{
//		 ups_stat_changed=0;
//		 changed_trap(&objid_ups,UPS_STAT);	
//		}		
//    if(ac24_stat_changed)
//		{
//		 ac24_stat_changed=0;
//		 changed_trap(&objid_ac24,AC24_STAT);	
//		}				
//				
	
		// Wait for the next cycle.
		vTaskDelayUntil( &xLastWakeTime, 1000);
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







/*
 * 函数名：void changed_trap(struct snmp_obj_id *objid,float VOL)
 * 描述  ：指定节点  上报对应的  状态量
 * 输入  ：无
 * 输出  ：无	
 */
void changed_trap(struct snmp_obj_id *objid,u8 data)
{
	char msg[5];
	static u8 msglen;
	struct trap_list *vb_list;
	struct snmp_varbind *vb;
	//vb_list->spc_trap = 12;
	
	sprintf(msg, "%d", data);
	vb_list = getNextFreePrivateTrapList();
	
	vb_list->ent_oid =objid;
	vb_list->spc_trap = 12;
	msglen=strlen(msg);
	vb = snmp_varbind_alloc(objid, 4, msglen);
	if (vb!= NULL)
	{
	 memcpy (vb->value, &msg, msglen);         
	 snmp_varbind_tail_add(&vb_list->vb_root,vb);
	} 					
	tcpip_callback(vSendTrapCallback2, vb_list);  
}









