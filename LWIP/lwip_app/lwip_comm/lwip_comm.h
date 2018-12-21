#ifndef _LWIP_COMM_H
#define _LWIP_COMM_H 
#include "dm9000.h" 

#define LWIP_MAX_DHCP_TRIES		4   //DHCP服务器最大重试次数
#define LINK_UP		0
#define LINK_DOWN	1

//lwip控制结构体
typedef struct  
{
	u8 mac[6];      //MAC地址
	u8 mqttip[4];	//远端主机IP地址
	u8 remoteip[4];	//远端主机IP地址
	u16 mqttport;
	u8 snmpip[4];	//远端主机IP地址
	u16 snmpport; 
	u8 servip[4];	
	u16 servport; 
	u8 ip[4];       //本机IP地址
	u8 netmask[4]; 	//子网掩码
	u8 gateway[4]; 	//默认网关的IP地址
	
	vu8 dhcpstatus;	//dhcp状态 
					//0,未获取DHCP地址;
					//1,进入DHCP获取状态
					//2,成功获取DHCP地址
					//0XFF,获取失败.
}__lwip_dev;
extern __lwip_dev lwipdev;	//lwip控制结构体
extern struct netif lwip_netif;				//定义一个全局的网络接口
extern u8 linkState;
extern int  Flag;//判断数据接收标志
extern char COM_MAC_BUF[10][18];

void lwip_pkt_handle(void);
void lwip_periodic_handle(void);
void lwip_comm_dhcp_creat(void);//DHCP
	
void lwip_comm_default_ip_set(__lwip_dev *lwipx);
u8 lwip_comm_mem_malloc(void);
void lwip_comm_mem_free(void);
u8 lwip_comm_init(void);
void lwip_dhcp_process_handle(void);
void lwIPInit(void *pvParameters);

extern void lwip_periodic_handle(void);

//extern TaskHandle_t LWIPTask_Handler;//

#endif













