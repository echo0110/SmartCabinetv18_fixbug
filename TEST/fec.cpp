
/**
  ******************************************************************************
  * @file    bsp_usart1.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   usart应用bsp
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 iSO-MINI STM32 开发板 
  * 论坛    :http://www.chuxue123.com
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */
#include "stdio.h"
#include "stm32f10x.h" 

//#include  "delay.h"
//#include "iostream"

#include "fec.h"

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

#include "trap.h"


#include "lwip_comm.h"




void vSendTrapCallback2( void * parameters );
struct trap_list * getNextFreePrivateTrapList();
void changed_trap(struct snmp_obj_id **objid,int VOL);
void trap_task(void *pvParameters);

#if 0
class Mytrap
{
public:
    void changed_trap(struct snmp_obj_id objid[],float VOL)
		{
			char msg[5];
			static u8 msglen;
			struct trap_list *vb_list;
			struct snmp_varbind *vb;
			vb_list->ent_oid =objid;
			sprintf(msg, "%.2f", VOL);
			if (vb!= NULL)
			{
			 msglen=strlen(msg);
			 memcpy (vb->value, &msg, msglen);         
			 snmp_varbind_tail_add(&vb_list->vb_root,vb);
			} 			
			tcpip_callback(vSendTrapCallback2, vb_list);  
		}
//    void changed_trap(struct snmp_obj_id **objid,int VOL)
//		{
//			char msg[5];
//			static u8 msglen;
//			struct trap_list *vb_list;
//			struct snmp_varbind *vb;
//			vb_list->ent_oid =*objid;
//			sprintf(msg, "%d", VOL);
//			if (vb!= NULL)
//			{
//			 msglen=strlen(msg);
//			 memcpy (vb->value, &msg, msglen);         
//			 snmp_varbind_tail_add(&vb_list->vb_root,vb);
//			} 					
//			tcpip_callback(vSendTrapCallback2, vb_list);  
//		}
};



void trap_task(void * pvParameters)
{
	portTickType xLastWakeTime;
	static unsigned char trap_flag=1;
	u8_t snmpauthentraps_set= 2;
	static ip_addr_t trap_addr;
	char msg_vol[5],msg_cur[5];
	
	
  //struct snmp_obj_id objid = {10, {1,3,6,1,4,1,1,1,1}};
	
	struct snmp_obj_id objid_vol = {10, {1,3,6,1,4,1,1,1,1}};	
	struct snmp_obj_id objid_cur = {10, {1,3,6,1,4,1,1,1,2}};

  struct snmp_obj_id objid[10];	
	
//  struct snmp_obj_id objid_tem = {10, {1,3,6,1,4,1,1,1,3}};	
//	struct snmp_obj_id objid_hum = {10, {1,3,6,1,4,1,1,1,4}};
//	
//	struct snmp_obj_id objid_water = {10, {1,3,6,1,4,1,1,1,5}};	
//	struct snmp_obj_id objid_door = {10, {1,3,6,1,4,1,1,1,6}};
	
	static u8 msglen;//= sizeof(msg);//22;

	struct snmp_varbind *vb,*vb_vol,*vb_cur;
	struct trap_list *vb_list;

	(void) pvParameters;
	
		
	
	trap_addr.addr=((u32_t)((lwipdev.snmpip[3]) & 0xff) << 24) | \
											 ((u32_t)((lwipdev.snmpip[2]) & 0xff) << 16) | \
											 ((u32_t)((lwipdev.snmpip[1]) & 0xff) << 8)  | \
												(u32_t)((lwipdev.snmpip[0]) & 0xff);
	snmp_trap_dst_ip_set(0,&trap_addr);
	snmp_trap_dst_enable(0,trap_flag);
	snmp_set_snmpenableauthentraps(&snmpauthentraps_set);
	

  
	for ( ;; )
	{		 
    xLastWakeTime = xTaskGetTickCount();		
		vb_list = getNextFreePrivateTrapList();
		
		objid[0]=objid_vol;
	  objid[1]=objid_cur;
		vb_list->spc_trap = 12;
		int a=12;
		
		Mytrap trap_report;//trap上报
		if(sys12_stat_changed==1)
		{	
      sys12_stat_changed=0;			
		 //trap_report.changed_trap(objid[0],VOL); 
		 trap_report.changed_trap(objid,VOL);
			//trap_report.add(a,VOL);
		}	
		vTaskDelayUntil(&xLastWakeTime,4000);
	}   
}

#endif








	
