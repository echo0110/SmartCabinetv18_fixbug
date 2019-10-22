#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mqtt.h"
#include "MQTTPacket.h"
#include "usart.h"
#include "delay.h"
#include "gsm.h"
#include "dm9000.h"
#include "MQTTGPRSClient.h"
#include "dma.h"
#include "print.h"
#include "gps.h"
#include "sensor.h"
#include "adc.h"
#include "lwip_comm.h"
#include "minIni.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "camera.h"
#include "iwdg.h"
#include "httpd.h"

#include <math.h>
#include "lwip/netif.h"
#include "dns.h"
#include "lwip/api.h"

#include "lwip/inet.h"

#include "lwip/ip_addr.h"

#include "tcp_client_demo.h" 

#include "transport.h"

#include "stm32f10x.h"
#include "lwip_mqtt.h"
#include "mqtt.h"

#define MAX_PUB_TIME 60//10

#define Change_percent  10//10


#define Change_CUR  10//10

#define maxElementsOfFile	512

/* 互斥信号量  for lwip_mqtt&&mqtt*/
SemaphoreHandle_t lwip_Sem = NULL;
/* 互斥信号量  for lwip_mqtt&&mqtt*/
SemaphoreHandle_t lwip_Sem2 = NULL;


u8 Lwip_no_mqtt_msg_exchange = 1;
//u32 no_mqtt_msg_times;
u16 Lwip_publishSpaces = MAX_PUB_TIME*10;
u16 Lwip_pingSpaces;


int tcpclient_sock;


extern TaskHandle_t MqttTask_Handler;
int Lwip_MQTTSubscribe(char* subtopic, enum QoS pos);
int Lwip_MQTTMsgPublish(char* subtopic, enum QoS qos, u8 retained, u8* msg, int msg_len);

int Lwip_MQTTClientInit(void)
{
	int len = 0;
	u8 buf[200];
	int buflen = sizeof(buf);
	u8 sessionPresent, connack_rc;
	char clientid[20];
		
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
	sprintf(clientid, "SSI.SC%02X%02X%02X\n", STM32ID2, STM32ID1, STM32ID0);
	
	// 客户端ID--必须唯一
	connectData.clientID.cstring = clientid;	// SSI+MAC
	// 保活间隔
	connectData.keepAliveInterval = KEEPLIVE_TIME;
	// 清除会话
	
	// 串行化连接消息
	len = MQTTSerialize_connect(buf, buflen, &connectData);
	// 发送数据
	if(transport_sendPacketBuffer(buf, len)< 0)return -2;
	
	if(MQTTPacket_read(buf, buflen, transport_getdata) != CONNACK)return -4;
	
	// 拆解连接回应包
	if(MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 || connack_rc != 0)return -5;

	if(sessionPresent == 1)return 1;	// 不需要重新订阅--服务器已经记住了客户端的状态
	else return 0;						// 需要重新订阅
}







void Lwip_MqttTask(void *pvParameters)
{
	int res, times, pings = 0;
	u32 xLastExecutionTime;
	int type;
	u8 buf[200];
	
	int buflen = sizeof(buf);
	int sessionPresent = 0;
	
	struct ip_addr addr;
	err_t err;
//	char *recv_data;
	int bytes_received;
	char topic[MSG_TOPIC_LEN];
	char msg[200];
	char host_name[16];
  char hostname[]="192.168.0.155"; 
	int host_port=1883;
	u8 mqtt_ip[16]={0};
	Printf("LWIP..... MQTT Connecting Server......\r\n");
	
//	GetCheckIP();	
MQTT_START:
	while(1)
	{
		xSemaphoreTake(lwip_Sem, portMAX_DELAY );
		transport_close();
		memset(host_name,strlen(host_name),0);
		vTaskDelay(2000/portTICK_RATE_MS);
		sprintf(host_name, "%d.%d.%d.%d",192,168,0,155);
		res=transport_open((s8*)"192.168.0.155",(s32)host_port);		
		if(res != -1)
		{
			SIM_CON_OK = 1;						
			Printf("Lwip_MQTT连接服务器成功!!!\r\n");		
			break;
		}	
		else 
		{
			SIM_CON_OK = 0;
			Printf("Lwip_MQTT连接服务器失败,错误代码:%d\r\n", res);
//			gsm_reset();
		}
		
		vTaskDelay(3000/portTICK_RATE_MS);
		/* 发送事件标志，表示任务正常运行 */
	 	
	}
	sessionPresent = Lwip_MQTTClientInit();
	if(sessionPresent < 0)
	{
		Printf("Lwip_MQTT sessionPresent:%d\r\n", sessionPresent);
		gsm_reset();
		goto MQTT_START;
	}
	if(sessionPresent != 1)
	{
		// 订阅消息
		
		sprintf(topic, "Camera/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		
		res += Lwip_MQTTSubscribe(topic, QOS1);Printf("Lwip_MQTTSubscribe MQTTIP:%d\r\n", res);
		
		sprintf(topic, "SSMAC/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res = Lwip_MQTTSubscribe(topic, QOS1);Printf("Lwip_MQTTSubscribe SSMAC:%d\r\n", res);
		sprintf(topic, "LOCALIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += Lwip_MQTTSubscribe(topic, QOS1);Printf("Lwip_MQTTSubscribe LOCALIP:%d\r\n", res);

		sprintf(topic, "MQTTIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += Lwip_MQTTSubscribe(topic, QOS1);Printf("Lwip_MQTTSubscribe MQTTIP:%d\r\n", res);
		
//		sprintf(topic, "SERV/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
//		res += Lwip_MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe SERV:%d\r\n", res);
//		
		sprintf(topic, "SNMPIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += Lwip_MQTTSubscribe(topic, QOS1);Printf("Lwip_MQTTSubscribe SNMPIP:%d\r\n", res);
		sprintf(topic, "EQUCTRL/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += Lwip_MQTTSubscribe(topic, QOS1);Printf("Lwip_MQTTSubscribe EQUCTRL:%d\r\n", res);
		if(res != 0)goto MQTT_START;
	}
	xSemaphoreGive(lwip_Sem);
	xLastExecutionTime = xTaskGetTickCount();
		
	while(1)
	{ 
		
		u8 latitude_temp,longitude_temp;
		u16 TEM_temp,VOL_temp,CUR_temp;
	  
		vTaskDelayUntil(&xLastExecutionTime, 100);//100ms,configTICK_RATE_HZ / SYS_TICK_RATE_HZ
		Lwip_publishSpaces++;Lwip_pingSpaces++;
    
    //Printf("publish  before\r\n");		
		
		/* 发送事件标志，表示任务正常运行 */
	 // xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_MQTT);		
		
		
		//每隔MAX_PUB_TIME秒发布一次消息   正常情况下一分钟一次,发生变化比较明显时,直接立马推送 温度和电压  
		if((Lwip_publishSpaces>MAX_PUB_TIME*10) || (!no_mqtt_msg_exchange))
		{
			Lwip_publishSpaces = 0;
			no_mqtt_msg_exchange = 1;
			//pingSpaces = 0;
//			xSemaphoreTake(lwip_Sem, portMAX_DELAY );
			sprintf(topic, "ASMAC/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
			sprintf(msg, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s*", IP_STAT[0], IP_STAT[1], IP_STAT[2], IP_STAT[3], IP_STAT[4], IP_STAT[5], IP_STAT[6], IP_STAT[7], IP_STAT[8], IP_STAT[9]);
			Lwip_MQTTMsgPublish(topic, QOS0, 0, (u8*)msg, strlen(msg));
			vTaskDelay(100/portTICK_RATE_MS);

			sprintf(topic, "GPSSTAT/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
			sprintf(msg, "%c,%d.%d,%c,%d.%d*", gpsxSC.nshemi, gpsxSC.latitude/100000, gpsxSC.latitude%100000, gpsxSC.ewhemi, gpsxSC.longitude/100000, gpsxSC.longitude%100000);
			Lwip_MQTTMsgPublish(topic, QOS0, 0, (u8*)msg, strlen(msg));
			vTaskDelay(100/portTICK_RATE_MS);

			
			sprintf(topic, "EQUSTAT/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
//			sprintf(msg, "%.2f,%.2f,%.1f,%.1f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*", VOL, CUR, TEM, HUM, !WATER_STAT,DOOR_STAT,!SYS12_STAT,!BAK12_STAT,!UPS_STAT,
//			!AC24_STAT,AC1_STAT,AC2_STAT,AC3_STAT,fan_STAT, alarm_STAT,light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT);
			
			sprintf(msg, "%.2f,%.2f,%.1f,%.1f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*", VOL, CUR, TEM, HUM, !WATER_STAT,DOOR_STAT,!SYS12_STAT,!BAK12_STAT,!UPS_STAT,
			!AC24_STAT,AC1_STAT,AC2_STAT,AC3_STAT,fan_STAT, alarm_STAT,light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT);//NET_STAT
			
			Lwip_MQTTMsgPublish(topic, QOS0, 0, (u8*)msg, strlen(msg));
//			xSemaphoreGive(lwip_Sem);
			vTaskDelay(100/portTICK_RATE_MS);
		}
		//Printf("publish  after\r\n");	
		//温度或电压 
    if(tem_stat_changed==1||vol_stat_changed==1)
		{
			tem_stat_changed=0;
			vol_stat_changed=0;
			sprintf(topic, "EQUSTAT/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
			sprintf(msg, "%.2f,%.2f,%.1f,%.1f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*", VOL, CUR, TEM, HUM, !WATER_STAT,DOOR_STAT,!SYS12_STAT,!BAK12_STAT,!UPS_STAT,
			!AC24_STAT,AC1_STAT,AC2_STAT,AC3_STAT,fan_STAT, alarm_STAT,light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT);
		}

    //Printf("publish  after *****\r\n");			
//		if(USART2_RX_STA & 0X8000)
//		{
//			type = MQTTPacket_read(buf, buflen, getMQTTData);
//			if(type != -1)
//			{
//				Lwip_pingSpaces = 0;
//				// 处理消息
//				mqtt_pktype_ctl(type,buf,buflen);
//				Lwip_pingSpaces = 0;
//				USART2_RX_STA = 0;
//				reset();
//				UART_DMA_Enable(DMA1_Channel6, USART2_MAX_LEN);	// 恢复接收
//			}
//		}
	
//		if(Lwip_pingSpaces > KEEPLIVE_TIME/2*10)
//		{
//			times = 5;
//			res = 1;
//			Lwip_pingSpaces=0;
//			while((times--)&&(res!=0))res = my_mqtt_send_pingreq();
//		//	res = my_mqtt_send_pingreq();
//		//	Printf("ping times: %d\r\n", 10-times);
//			
//			if(res != 0)
//			{
//				Printf("MQTT my_mqtt_send_pingreq() = %d\r\n", res);
//				pings = 0;
//				gsm_reset();
//				goto MQTT_START;
//			}
//			else
//			{
//				pings++;
//				Printf("my mqtt send pingreq: %d\r\n", pings);
//			}				
//		}
	}		
}

int Lwip_MQTTSubscribe(char* subtopic, enum QoS pos)
{
	int len = 0;
	u8 buf[200];
	int buflen = sizeof(buf);
	int req_qos = 0;
	u16 submsgid;
	int subcount;
	int granted_qos;
	u32 PacketID = 0;
	
	MQTTString topicString = MQTTString_initializer;
	
	// 复制主题
    topicString.cstring = subtopic;
	// 订阅质量
	req_qos = pos;

	len = MQTTSerialize_subscribe(buf, buflen, 0, PacketID++, 1, &topicString, &req_qos);
	
	if(transport_sendPacketBuffer(buf, len)< 0)return -2;
	
	if(MQTTPacket_read(buf, buflen, transport_getdata) != SUBACK)return -4;
	
	if(MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen) != 1)return -5;
	
	//检测返回数据的正确性
	if((granted_qos == 0x80)||(submsgid != (PacketID-1)))return -6;

    //订阅成功
	return 0;
}

u16 Lwip_GetNextPackID(void)
{
	static u16 pubpacketid = 0;
	return pubpacketid++;
}



int Lwip_MQTTMsgPublish(char* subtopic, enum QoS qos, u8 retained, u8* msg, int msg_len)
{
	int len = 0;
	u8 buf[200];
	int buflen = sizeof(buf);
	u16 packid = 0, packetidbk;
	
	MQTTString topicString = MQTTString_initializer;
	
	topicString.cstring = (char *)subtopic;
	
	//填充数据包ID
	if((qos == QOS1)||(qos == QOS2))
	{ 
		packid = Lwip_GetNextPackID();
	}
	else
	{
		qos = QOS0;
		retained = 0;
		packid = 0;
	}
	
	len = MQTTSerialize_publish(buf, buflen, 0, qos, retained, packid, topicString, msg, msg_len);
	
	switch(qos)
	{
	case QOS0:
		//sendMQTTData(buf, len, 0);
	  transport_sendPacketBuffer(buf,len);
		return 0;
	case QOS1:
		sendMQTTData(buf, len, 100);
//		if(WaitForPacket(PUBACK, 100, 5) < 0)return -3;
		return 1;
	case QOS2:
		sendMQTTData(buf, len, 100);
		// 等待PUBREC
//		if(WaitForPacket(PUBREC, 100, 5) < 0)return -3;
		// 发送PUBREL
		len = MQTTSerialize_pubrel(buf, buflen,0, packetidbk);
		if(sendMQTTData(buf, len, 100) < 0)return -2;
		// 等待PUBCOMP
//		if(WaitForPacket(PUBREC,100, 5) < 0)return -7;
		return 2;
	default:
		return -8;
	}
}





int lwip_my_mqtt_send_pingreq(void)
{
	int len = 0;
	u8 buf[200];
	int buflen = sizeof(buf);
	
	len = MQTTSerialize_pingreq(buf, buflen);
//	if(sendMQTTData(buf, len, 400) < 0)return 2;
	if(transport_sendPacketBuffer(buf, len)< 0)return 2;
//	if(MQTTPacket_read(buf, buflen, getMQTTData) != PINGRESP)return 3;
	if(MQTTPacket_read(buf, buflen, transport_getdata) != PINGRESP)return 3;

	return 0;
}



