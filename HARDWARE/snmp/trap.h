#ifndef __trap_H
#define __trap_H


	
#include "sys.h"

#include "stm32f10x.h"
#include "snmp_msg.h"


#ifdef __cplusplus  
extern "C" {  
#endif  
  
  
#ifdef __cplusplus  
}  
#endif 


//extern "C" void trap_task(void * pvParameters);

struct trap_list
{
   struct snmp_varbind_root vb_root;
   struct snmp_obj_id *ent_oid;
   s32 spc_trap;
   u8 in_use; //表示发送了trap  
};
//extern struct trap_list;

//extern void changed_trap(struct snmp_obj_id *objid,T & data);

#endif 
