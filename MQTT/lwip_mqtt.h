#ifndef lwip_mqtt_H_
#define lwip_mqtt_H_
#include "stm32f10x.h"
#include "mqtt.h"


#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

//enum QoS { QOS0, QOS1, QOS2 };
//#define MSG_MAX_LEN		200
//#define MSG_TOPIC_LEN	20
//#define	KEEPLIVE_TIME	100//120
//#define	KEEPLIVE_TIME_test	100
//#define MAX_MAC			10
//#define MAX_IP		10
//#define MAC_KEEP_TIME	120

//extern char CHECK_IP[MAX_IP][18];


#define Lwip_MAX_PUB_TIME 50//10



// 数据交换结构体
//typedef struct __MQTTMessage
//{
//    u32 qos;
//    u8 retained;
//    u8 dup;
//    u16 id;
//	u8 type;
//    void *payload;
//    s32 payloadlen;
//}MQTTMessage;

//// 用户接收消息结构体
//typedef struct __MQTT_MSG
//{
//	u8 msgqos;
//	u8 msg[MSG_MAX_LEN];
//	u32 msglenth;
//	u8 topic[MSG_TOPIC_LEN];
//	u16 packetid;
//	u8 valid;
//}MQTT_USER_MSG;

//// 需要ping的server ip结构体
//typedef struct _Ping_ip 
//{
//	u8 ip[4];       //设置server ip地址
//}Ping_ip;


void Lwip_MqttTask(void *pvParameters);
int Lwip_MQTTClientInit(void);
int Lwip_MQTTSubscribe(char* subtopic, enum QoS pos);
int Lwip_MQTTMsgPublish(char* subtopic, enum QoS qos, u8 retained, u8* msg, int msg_len);
void mqtt_pktype_ctl(u8 packtype, u8 *buf, u32 buflen);
int Lwip_my_mqtt_send_pingreq(void);

extern u8 Lwip_no_mqtt_msg_exchange;

extern SemaphoreHandle_t lwip_Sem;
extern SemaphoreHandle_t lwip_Sem2;

#endif /* MQTT_H_ */
