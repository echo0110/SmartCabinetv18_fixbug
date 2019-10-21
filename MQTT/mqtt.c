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

#define MAX_PUB_TIME 60//10

#define Change_percent  10//10


#define Change_CUR  10//10

u8 no_mqtt_msg_exchange = 1;
//u32 no_mqtt_msg_times;
u16 publishSpaces = MAX_PUB_TIME*10;
u16 pingSpaces;

char CHECK_MAC[MAX_MAC][18];
char CHECK_IP[MAX_IP][18];
char MAC_STAT[MAX_MAC][18]={{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"}};
char IP_STAT[MAX_IP][18]={{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"},{"0"}};
char INI_IP_STAT[3][18]={{"0"},{"0"},{"0"}};

Ping_ip Ping_ip_array[10];

u8 MAC_COUNT[MAX_MAC];
u8 buf_count = 0;



extern TaskHandle_t MqttTask_Handler;

int MQTTClientInit(void)
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
//	connectData.cleansession = 1;
	// 用户名
//	connectData.username.cstring = NULL;
	// 用户密码
//	connectData.password.cstring = NULL;
	// 创建MQTT客户端连接参数
//	connectData.willFlag = 0;
	// MQTT版本
//	connectData.MQTTVersion = 4;
	
	// 串行化连接消息
	len = MQTTSerialize_connect(buf, buflen, &connectData);
	// 发送数据
	if(sendMQTTData(buf, len, 200) < 0)return -2;
	
	if(MQTTPacket_read(buf, buflen, getMQTTData) != CONNACK)return -4;
	
	// 拆解连接回应包
	if(MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 || connack_rc != 0)return -5;

	if(sessionPresent == 1)return 1;	// 不需要重新订阅--服务器已经记住了客户端的状态
	else return 0;						// 需要重新订阅
}

void GetCheckIP(void)
{
	u8 i, j,k;
	char delim[] = " .";
	char *token;
	char inifile[] = "1:cfg.ini";
	char key[5];
	char buf[10][18];
	char ip_buf[4][4];
	
	//struct _Ping_ip Ping_ip_arrry[10];
	for(i=0; i<10; i++)
	{
		sprintf(key, "ip%x", i);
		//memset(CHECK_IP[i],0,18);
		ini_gets("IP", key, "0", CHECK_IP[i], 18, inifile);// 172.28.0.10's MAC: 6c-ae-8b-1e-5b-52
	}			
  Split_IP();
}

#if 0
void GetCheckMAC(void)
{
	u8 i;
	char inifile[] = "1:cfg.ini";
	char section[5];
	
	for(i=0; i<10; i++)
	{
		sprintf(section,"mac%x",i);
		sprintf(CHECK_MAC[i],"%02x-%02x-%02x-%02x-%02x-%02x",	(u8)ini_getl(section,"mac0",0,inifile),(u8)ini_getl(section,"mac1",0,inifile),
																(u8)ini_getl(section,"mac2",0,inifile),(u8)ini_getl(section,"mac3",0,inifile),
																(u8)ini_getl(section,"mac4",0,inifile),(u8)ini_getl(section,"mac5",0,inifile));
	}
	// 默认172.28.0.10的MAC：6c-ae-8b-1e-5b-52
}
#endif

void MqttTask(void *pvParameters)
{
	int res, times, pings = 0;
	u32 xLastExecutionTime;
	int type;
	u8 buf[200];
	int buflen = sizeof(buf);
	int sessionPresent = 0;
	
	struct ip_addr addr;
	err_t err;
	
	char topic[MSG_TOPIC_LEN];
	char msg[200];
	char host_name[16];
	char host_port[6];
	//char hostname[]="117.64.249.208";//"www.cncqs.cn"; 
  //char hostname[]="www.cncqs.cn";   
  char hostname[]="47.97.184.119"; 
	u8 mqtt_ip[16]={0};
	Printf("...... MQTT Connecting Server......\r\n");
	
//	sprintf(host_name, "%d.%d.%d.%d", lwipdev.mqttip[0], lwipdev.mqttip[1], lwipdev.mqttip[2], lwipdev.mqttip[3]);
//	sprintf(host_port, "%d", lwipdev.mqttport);

	GetCheckIP();
	
	if((err = netconn_gethostbyname((char*)(hostname), &(addr))) == ERR_OK) 
	{	
    inet_ntoa(addr.addr);		
		lwipdev.mqttip[0]=addr.addr;
    lwipdev.mqttip[1]=addr.addr>>8; 
    lwipdev.mqttip[2]=addr.addr>>16;
    lwipdev.mqttip[3]=addr.addr>>24;
	 //printf("netconn_gethostbyname(%s)==%s\n", (char*)(hostname),inet_ntoa(addr.addr));
	}	
	else 
	{
		printf("netconn_gethostbyname(%s)==%i\n", (char*)(hostname), (int)(err));
	}
	
	sprintf(host_name, "%d.%d.%d.%d", lwipdev.mqttip[0], lwipdev.mqttip[1], lwipdev.mqttip[2], lwipdev.mqttip[3]);
	sprintf(host_port, "%d", lwipdev.mqttport);
	
MQTT_START:
	while(1)
	{
		gsm_dect();
		vTaskDelay(2000/portTICK_RATE_MS);
		res = SIM800C_CONNECT_SERVER((u8*)host_name, (u8*)host_port);
		if(res == 0)
		{
			SIM_CON_OK = 1;			
			
			Printf("MQTT连接服务器成功!!!\r\n");
			
			break;
		}
		else 
		{
			SIM_CON_OK = 0;
			Printf("MQTT连接服务器失败,错误代码:%d\r\n", res);
//			gsm_reset();
		}
		vTaskDelay(3000/portTICK_RATE_MS);
		/* 发送事件标志，表示任务正常运行 */
	 	
	}
	sessionPresent = MQTTClientInit();
	if(sessionPresent < 0)
	{
		Printf("MQTT sessionPresent:%d\r\n", sessionPresent);
		gsm_reset();
		goto MQTT_START;
	}
	if(sessionPresent != 1)
	{
		// 订阅消息
		
		sprintf(topic, "Camera/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe MQTTIP:%d\r\n", res);
		
		sprintf(topic, "SSMAC/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res = MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe SSMAC:%d\r\n", res);
		sprintf(topic, "LOCALIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe LOCALIP:%d\r\n", res);
//		sprintf(topic, "AISMAC/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
//		res += MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe AISMAC:%d\r\n", res);
		sprintf(topic, "MQTTIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe MQTTIP:%d\r\n", res);
		
		sprintf(topic, "SERV/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe SERV:%d\r\n", res);
		
		sprintf(topic, "SNMPIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe SNMPIP:%d\r\n", res);
		sprintf(topic, "EQUCTRL/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
		res += MQTTSubscribe(topic, QOS1);Printf("MQTTSubscribe EQUCTRL:%d\r\n", res);
		if(res != 0)goto MQTT_START;
	}
	
	xLastExecutionTime = xTaskGetTickCount();
		
	while(1)
	{ 
		
		u8 latitude_temp,longitude_temp;
		u16 TEM_temp,VOL_temp,CUR_temp;
		vTaskDelayUntil(&xLastExecutionTime, 100);//100ms,configTICK_RATE_HZ / SYS_TICK_RATE_HZ
		
		publishSpaces++;pingSpaces++;
    
    //Printf("publish  before\r\n");		
		
		/* 发送事件标志，表示任务正常运行 */
	 // xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_MQTT);		
		
		//每隔MAX_PUB_TIME秒发布一次消息   正常情况下一分钟一次,发生变化比较明显时,直接立马推送 温度和电压  
		if((publishSpaces>MAX_PUB_TIME*10) || (!no_mqtt_msg_exchange))
		{
			publishSpaces = 0;
			no_mqtt_msg_exchange = 1;
			//pingSpaces = 0;
			
			sprintf(topic, "ASMAC/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
			sprintf(msg, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s*", IP_STAT[0], IP_STAT[1], IP_STAT[2], IP_STAT[3], IP_STAT[4], IP_STAT[5], IP_STAT[6], IP_STAT[7], IP_STAT[8], IP_STAT[9]);
			MQTTMsgPublish(topic, QOS0, 0, (u8*)msg, strlen(msg));
			vTaskDelay(100/portTICK_RATE_MS);

			sprintf(topic, "GPSSTAT/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
			sprintf(msg, "%c,%d.%d,%c,%d.%d*", gpsxSC.nshemi, gpsxSC.latitude/100000, gpsxSC.latitude%100000, gpsxSC.ewhemi, gpsxSC.longitude/100000, gpsxSC.longitude%100000);
			MQTTMsgPublish(topic, QOS0, 0, (u8*)msg, strlen(msg));
			vTaskDelay(100/portTICK_RATE_MS);

			
			sprintf(topic, "EQUSTAT/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
//			sprintf(msg, "%.2f,%.2f,%.1f,%.1f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*", VOL, CUR, TEM, HUM, !WATER_STAT,DOOR_STAT,!SYS12_STAT,!BAK12_STAT,!UPS_STAT,
//			!AC24_STAT,AC1_STAT,AC2_STAT,AC3_STAT,fan_STAT, alarm_STAT,light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT);
			
			sprintf(msg, "%.2f,%.2f,%.1f,%.1f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d*", VOL, CUR, TEM, HUM, !WATER_STAT,DOOR_STAT,!SYS12_STAT,!BAK12_STAT,!UPS_STAT,
			!AC24_STAT,AC1_STAT,AC2_STAT,AC3_STAT,fan_STAT, alarm_STAT,light_STAT,heat_STAT,DC1_STAT,DC2_STAT,DC3_STAT,DC4_STAT);//NET_STAT
			
			MQTTMsgPublish(topic, QOS0, 0, (u8*)msg, strlen(msg));
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
		if(USART2_RX_STA & 0X8000)
		{
			type = MQTTPacket_read(buf, buflen, getMQTTData);
			if(type != -1)
			{
				pingSpaces = 0;
				// 处理消息
				mqtt_pktype_ctl(type,buf,buflen);
				pingSpaces = 0;
				USART2_RX_STA = 0;
				reset();
				UART_DMA_Enable(DMA1_Channel6, USART2_MAX_LEN);	// 恢复接收
			}
		}
		
		if(pingSpaces > KEEPLIVE_TIME/2*10)
		{
			times = 5;
			res = 1;
			pingSpaces=0;
			while((times--)&&(res!=0))res = my_mqtt_send_pingreq();
		//	res = my_mqtt_send_pingreq();
		//	Printf("ping times: %d\r\n", 10-times);
			
			if(res != 0)
			{
				Printf("MQTT my_mqtt_send_pingreq() = %d\r\n", res);
				pings = 0;
				gsm_reset();
				goto MQTT_START;
			}
			else
			{
				pings++;
				Printf("my mqtt send pingreq: %d\r\n", pings);
			}				
		}
	}
	
	
}

int MQTTSubscribe(char* subtopic, enum QoS pos)
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
	if(sendMQTTData(buf, len, 200) < 0)return -2;
	if(MQTTPacket_read(buf, buflen, getMQTTData) != SUBACK)return -4;
	if(MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen) != 1)return -5;
	
	//检测返回数据的正确性
	if((granted_qos == 0x80)||(submsgid != (PacketID-1)))return -6;

    //订阅成功
	return 0;
}
u16 GetNextPackID(void)
{
	static u16 pubpacketid = 0;
	return pubpacketid++;
}

int WaitForPacket(u8 packettype, u16 waittime, u8 times)
{
	int type = -1;
	u8 n = 0;
	u8 buf[200];
	int buflen = sizeof(buf);
	
	USART2_RX_STA = 0;
	reset();
	
	do
	{
		UART_DMA_Enable(DMA1_Channel6, USART2_MAX_LEN);	// 恢复接收
		while(--waittime)
		{
			vTaskDelay(10);
			if(USART2_RX_STA & 0X8000)break;
		}
		if(USART2_RX_STA & 0X8000)type = MQTTPacket_read(buf, buflen, getMQTTData);
		
		USART2_RX_STA = 0;
		reset();
		
		if(type != -1)mqtt_pktype_ctl(type, buf, buflen);
		n++;
	}while((type != packettype)&&(n < times));
	if(type == packettype)return 0;
	else return -1;	
}





int MQTTMsgPublish(char* subtopic, enum QoS qos, u8 retained, u8* msg, int msg_len)
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
		packid = GetNextPackID();
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
		sendMQTTData(buf, len, 0);
		return 0;
	case QOS1:
		sendMQTTData(buf, len, 100);
		if(WaitForPacket(PUBACK, 100, 5) < 0)return -3;
		return 1;
	case QOS2:
		sendMQTTData(buf, len, 100);
		// 等待PUBREC
		if(WaitForPacket(PUBREC, 100, 5) < 0)return -3;
		// 发送PUBREL
		len = MQTTSerialize_pubrel(buf, buflen,0, packetidbk);
		if(sendMQTTData(buf, len, 100) < 0)return -2;
		// 等待PUBCOMP
		if(WaitForPacket(PUBREC,100, 5) < 0)return -7;
		return 2;
	default:
		return -8;
	}
}

void deliverMessage(MQTTString *TopicName, MQTTMessage *msg, MQTT_USER_MSG *mqtt_user_msg)
{
	// 消息质量
	mqtt_user_msg->msgqos = msg->qos;
	// 保存消息
	memcpy(mqtt_user_msg->msg, msg->payload, msg->payloadlen);
	mqtt_user_msg->msg[msg->payloadlen] = 0;
	// 保存消息长度
	mqtt_user_msg->msglenth = msg->payloadlen;
	// 消息主题
	memcpy((char *)mqtt_user_msg->topic,TopicName->lenstring.data,TopicName->lenstring.len);
	mqtt_user_msg->topic[TopicName->lenstring.len] = 0;
	// 消息ID
	mqtt_user_msg->packetid = msg->id;
	// 标明消息合法
	mqtt_user_msg->valid = 1;		
}

#if 0
void LOCALIP(MQTT_USER_MSG *msg)
{
	char delim[] = " .,*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[12][4];
	char key[6];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; token!=NULL; token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	for(i=0; i<4; i++)
	{
		sprintf(key, "ip%x", i);
		ini_putl("local", key, atoi(buf[i]), inifile);
		sprintf(key, "mask%x", i);
		ini_putl("local", key, atoi(buf[4+i]), inifile);
		sprintf(key, "gw%x", i);
		ini_putl("local", key, atoi(buf[8+i]), inifile);
	}
}

void MQTTIP(MQTT_USER_MSG *msg)
{
	char delim[] = " .,*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[5][6];
	char key[4];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; token!=NULL; token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	for(i=0; i<4; i++)
	{
		sprintf(key, "ip%x", i);
		ini_putl("mqtt", key, atoi(buf[i]), inifile);
	}
	ini_putl("mqtt", "port", atoi(buf[4]), inifile);
}

void SNMPIP(MQTT_USER_MSG *msg)
{
	char delim[] = " .,*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[5][6];
	char key[4];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; token!=NULL; token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	for(i=0; i<4; i++)
	{
		sprintf(key, "ip%x", i);
		ini_putl("snmp", key, atoi(buf[i]), inifile);
	}
	ini_putl("snmp", "port", atoi(buf[4]), inifile);
}

void ISMAC(MQTT_USER_MSG *msg)
{
	char delim[] = " .,*";
	char macdelim[] = ":-";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[10][18];
	char macbuf[10][6];
	char section[5];
	char key[5];
	u8 i, j;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; token!=NULL; token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	for(i=0; i<10; i++)
	{
		MAC_COUNT[i] = 0;
		strcpy(CHECK_MAC[i], buf[i]);
	}
	
	for(i=0; i<10; i++)
	{
		if(strcmp(buf[i],"0") == 0)
			for(j=0; j<6; j++)
				macbuf[i][j] = 0;
		else
		{
			for(token=strtok(buf[i],macdelim),j=0; token!=NULL; token=strtok(NULL, macdelim),j++)
				macbuf[i][j] = strtol(token, NULL, 16);
		}
	}
	
	for(i=0; i<10; i++)
	{
		sprintf(section, "mac%x", i);
		for(j=0; j<6; j++)
		{
			sprintf(key, "mac%x", j);
			ini_putl(section, key, macbuf[i][j], inifile);
		}
	}
}
#endif

void LOCALIP(MQTT_USER_MSG *msg)
{
	char delim[] = ",*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[3][16];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; (token!=NULL)&&(i<3); token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	ini_puts("local", "ip", buf[0], inifile);
	ini_puts("local", "mask", buf[1], inifile);
	ini_puts("local", "gw", buf[2], inifile);
}

void MQTTIP(MQTT_USER_MSG *msg)
{
	char delim[] = ",*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[2][16];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; (token!=NULL)&&(i<2); token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	ini_puts("mqtt", "ip", buf[0], inifile);
	ini_putl("mqtt", "port", atoi(buf[1]), inifile);
}

void SNMPIP(MQTT_USER_MSG *msg)
{
	char delim[] = ",*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[2][16];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; (token!=NULL)&&(i<2); token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	ini_puts("snmp", "ip", buf[0], inifile);
	ini_putl("snmp", "port", atoi(buf[1]), inifile);
}

void ISIP(MQTT_USER_MSG *msg)
{
	//char delim[] = " .,*";
	char delim[] = ",*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[10][18];	
	char key[5];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; token!=NULL; token=strtok(NULL, delim),i++)
	{
	 memset(buf[i],0,18);
	 memcpy(buf[i], token,strlen(token));
	}
	
	
	for(i=0; i<10; i++)
	{
		//strcpy(CHECK_IP[i], buf[i]);
		memcpy(CHECK_IP[i], buf[i],strlen(buf[i]));
		sprintf(key, "ip%x", i);
		
		ini_puts("ip", key, buf[i], inifile);
	}
	GetCheckIP();
	//Split_IP();  
	printf("puts over....\n");
}

void EQUCTRL(MQTT_USER_MSG *msg)
{
	char inifile[] = "1:cfg.ini";
	//AC1
	if(msg->msg[0] == '1') 
  {	
	 OUT_AC1_220V_ON();
    AC1_STAT=1;		
	 ini_puts("ctr", "L1", "1", inifile);
	}
	if(msg->msg[0] == '0')
	{		
	 OUT_AC1_220V_OFF();
	 AC1_STAT=0;
	 ini_puts("ctr", "L1", "0", inifile);
	}		
	if(msg->msg[0] == '2')	{}
	
	//AC2
	if(msg->msg[2] == '1') 
	{
		OUT_AC2_220V_ON(); 
		AC2_STAT=1;
		ini_puts("ctr", "L2", "1", inifile);
	}		
	if(msg->msg[2] == '0')
	{		
		OUT_AC2_220V_OFF();
		AC2_STAT=0;
		ini_puts("ctr", "L2", "0", inifile); //AC2		
	}		
	if(msg->msg[2] == '2')	{}
		
	//AC3	
	if(msg->msg[4] == '1')
	{
		OUT_AC3_220V_ON();
		AC3_STAT=1;
		ini_puts("ctr", "L3", "1", inifile);
	}	
	if(msg->msg[4] == '0')
	{
		OUT_AC3_220V_OFF();
		AC3_STAT=0;
		ini_puts("ctr", "L3", "0", inifile);
	}		
	if(msg->msg[4] == '2')	{}
		
	if(msg->msg[6] == '1')
	{
	 fan_STAT=1;
	 out_fan_ON();//
	}		
	if(msg->msg[6] == '0') 
	{	 
	 out_fan_OFF();
	 fan_STAT=0;
	}		
	if(msg->msg[6] == '2')	{}
						
	if(msg->msg[8] == '1')
	{
	 alarm_ON();
	 alarm_STAT=1;
	}		
	if(msg->msg[8] == '0')
	{
	 alarm_OFF();
	 alarm_STAT=0;
	}		
	if(msg->msg[8] == '2')  {}
		
	if(msg->msg[10] == '1') 
	{
	 open_the_door();
	 light_ON();
	 light_STAT=1;
	}	
	if(msg->msg[10] == '0')
	{
	 light_OFF();
	 light_STAT=0;
	}		
	if(msg->msg[10] == '2') {}
	
	if(msg->msg[12] == '1')
	{
	 heat_ON();
	 heat_STAT=1;	
	}			
	if(msg->msg[12] == '0')
	{
	 heat_OFF();
	 heat_STAT=0;
	}		
	if(msg->msg[12] == '2') {}	
	
  //DC1		
	if(msg->msg[14] == '1') 
	{
		DC1_ON();
		DC1_STAT=1;
		ini_puts("ctr", "V1", "1", inifile);		
	}	
	if(msg->msg[14] == '0')
	{
		DC1_OFF();
		DC1_STAT=0;
		ini_puts("ctr", "V1", "0", inifile);		
	}		
	if(msg->msg[14] == '2') {}
		
		
		
	  //DC2	
	if(msg->msg[16] == '1')
	{
		DC2_ON();	
		DC2_STAT=1;
		ini_puts("ctr", "V2", "1", inifile);
	}		
	if(msg->msg[16] == '0')
	{
	 DC2_OFF();
	 DC2_STAT=0;
	 ini_puts("ctr", "V2", "0", inifile);
	}		
	if(msg->msg[16] == '2') {}
	
	//DC3
	if(msg->msg[18] == '1')
	{
		DC3_ON();
		DC3_STAT=1;
		ini_puts("ctr", "V3", "1", inifile);		
	}		
	if(msg->msg[18] == '0') 
	{
		DC3_OFF();
		DC3_STAT=0;
		ini_puts("ctr", "V3", "0", inifile);		
	}
	if(msg->msg[18] == '2') {}
	
  //DC4		
	if(msg->msg[20] == '1') 
	{
		DC4_ON();
		DC4_STAT=1;
		ini_puts("ctr", "V4", "1", inifile);			
	}	
	if(msg->msg[20] == '0') 
	{
		DC4_OFF();
		DC4_STAT=0;
		ini_puts("ctr", "V4", "0", inifile);		
	}
	if(msg->msg[20] == '2') {}
	
  //远程开门		
	if(msg->msg[22] == '1') 
	{
	 open_the_door();
	 DOOR_STAT=1;		
	}
	if(msg->msg[22] == '0') {};
	if(msg->msg[22] == '2') {}
	//if(msg->msg[12] == '1')OPEN_DOOR();
}


void SERVCTRL(MQTT_USER_MSG *msg)
{
  char delim[] = ",*";
	char *token;
	char inifile[] = "1:cfg.ini";
	char buf[2][16];
	u8 i;
	
	for(token=strtok((char*)(msg->msg),delim),i=0; (token!=NULL)&&(i<2); token=strtok(NULL, delim),i++)
		strcpy(buf[i], token);
	
	ini_puts("serv", "ip", buf[0], inifile);
	ini_putl("serv", "port", atoi(buf[1]), inifile);
}

void Camera(MQTT_USER_MSG *msg)
{
	if(msg->msg[0]=='1')
	{
	 printf("recv  camera control cammand\n");		
	 //xSemaphoreGive(BinarySemaphore_photo_command);
	}			
}

// 处理数据
void UserMsgCtl(MQTT_USER_MSG *msg)
{
	char topic[MSG_TOPIC_LEN];
	Printf("rcv a msg: %s\r\n", msg->topic);
	
	sprintf(topic, "Camera/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
	if(strcmp((char*)(msg->topic),topic) == 0)Camera(msg);
	
	sprintf(topic, "LOCALIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
	if(strcmp((char*)(msg->topic),topic) == 0)LOCALIP(msg);
	
	sprintf(topic, "MQTTIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
	if(strcmp((char*)(msg->topic),topic) == 0)MQTTIP(msg);
	
	sprintf(topic, "SNMPIP/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
	if(strcmp((char*)(msg->topic),topic) == 0)SNMPIP(msg);
	
	sprintf(topic, "SSMAC/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
	if(strcmp((char*)(msg->topic),topic) == 0)ISIP(msg);
	
	sprintf(topic, "EQUCTRL/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
	if(strcmp((char*)(msg->topic),topic) == 0)EQUCTRL(msg);
	
	sprintf(topic, "SERV/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
	if(strcmp((char*)(msg->topic),topic) == 0)SERVCTRL(msg);
	
//	sprintf(topic, "camera/%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);
//	if(strcmp((char*)(msg->topic),topic) == 0)Camera(msg);
	
	
	switch(msg->msgqos)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	default:
		break;
	}
//	printf("MQTT>>消息主题：%s\n",msg->topic);	
//	printf("MQTT>>消息类容：%s\n",msg->msg);	
//	printf("MQTT>>消息长度：%d\n",msg->msglenth);	 
	
	// 处理完后销毁数据
	msg->valid  = 0;
}

void mqtt_pktype_ctl(u8 packtype, u8 *buf, u32 buflen)
{
	int rc, len;
	MQTTMessage msg;
	MQTT_USER_MSG mqtt_user_msg;
	MQTTString receivedTopic;

	switch(packtype)
	{
	case PUBLISH:
		// 拆析PUBLISH消息
		if(MQTTDeserialize_publish(&msg.dup,(int*)&msg.qos, &msg.retained, &msg.id, &receivedTopic,(u8 **)&msg.payload, &msg.payloadlen, buf, buflen) != 1)return;
		//接受消息
		deliverMessage(&receivedTopic, &msg, &mqtt_user_msg);
		if(msg.qos == QOS0)
		{
			UserMsgCtl(&mqtt_user_msg);
			return;
		}
		if(msg.qos == QOS1)
		{
			len = MQTTSerialize_puback(buf, buflen, mqtt_user_msg.packetid);
			sendMQTTData(buf, len, 200);
			UserMsgCtl(&mqtt_user_msg);
			return;												
		}
		if(msg.qos == QOS2)
		{
			len = MQTTSerialize_ack(buf, buflen, PUBREC, 0, mqtt_user_msg.packetid);			                
			sendMQTTData(buf, len, 200);
		}		
		break;
	case PUBREL:				           
		rc = MQTTDeserialize_ack(&msg.type,&msg.dup, &msg.id, buf,buflen);
		if((rc != 1)||(msg.type != PUBREL)||(msg.id != mqtt_user_msg.packetid))return ;
		if(mqtt_user_msg.valid == 1)UserMsgCtl(&mqtt_user_msg);
		len = MQTTSerialize_pubcomp(buf, buflen, msg.id);	                   	
		sendMQTTData(buf, len, 200);										
		break;
	case   PUBACK:	// 等级1客户端推送数据后，服务器返回
		break;
	case   PUBREC:	// 等级2客户端推送数据后，服务器返回
		break;
	case   PUBCOMP:	// 等级2客户端推送PUBREL后，服务器返回
		break;
	default:
		break;
	}
}

int my_mqtt_send_pingreq(void)
{
	int len = 0;
	u8 buf[200];
	int buflen = sizeof(buf);
	
	len = MQTTSerialize_pingreq(buf, buflen);
	if(sendMQTTData(buf, len, 400) < 0)return 2;
	if(MQTTPacket_read(buf, buflen, getMQTTData) != PINGRESP)return 3;

	return 0;
}

/*
 * 函数名：void open_the_door()
 * 描述  ：远程开门
 * 输入  ：无
 * 输出  ：无	
 */
void open_the_door(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_8);//开门
	delay_ms(200);
	GPIO_ResetBits(GPIOA,GPIO_Pin_8);//开门
	delay_ms(500);
}


#if 1

/*
 * 函数名：void SD_state_machine_CHECK(void) //SD卡轮询检测
 * 描述  ：状态机检测
 * 输入  ：无
 * 输出  ：无	
 */
void read_ini_config(void)
{
	char inifile[] = "1:cfg.ini";
	//char buf[20][16];
	u8 i;
	
	ini_gets("local", "ip", "192.168.0.0",INI_IP_STAT[0], 16, inifile);
	
	ini_gets("local", "mask", "0.0.0.0", INI_IP_STAT[1], 16, inifile);

	ini_gets("local", "gw", "0.0.0.1", INI_IP_STAT[2], 16, inifile);

//	
//	ini_gets("mqtt", "ip", "192.168.0.31", buf[3], 16, inifile);
	

}


#endif 
