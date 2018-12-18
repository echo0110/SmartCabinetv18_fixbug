#ifndef MQTT_H_
#define MQTT_H_
#include "stm32f10x.h"
#include "mqtt.h"

enum QoS { QOS0, QOS1, QOS2 };
#define MSG_MAX_LEN		200
#define MSG_TOPIC_LEN	20
#define	KEEPLIVE_TIME	120
#define MAX_MAC			10
#define MAX_IP		10
#define MAC_KEEP_TIME	120

//extern char CHECK_IP[MAX_IP][18];
extern char CHECK_MAC[MAX_MAC][18];
extern char CHECK_IP[MAX_IP][18];
extern char MAC_STAT[MAX_MAC][18];
extern u8 MAC_COUNT[MAX_MAC];
extern char IP_STAT[MAX_IP][18];

extern u8 buf_count;
extern u8 no_mqtt_msg_exchange;



// 数据交换结构体
typedef struct __MQTTMessage
{
    u32 qos;
    u8 retained;
    u8 dup;
    u16 id;
	u8 type;
    void *payload;
    s32 payloadlen;
}MQTTMessage;

// 用户接收消息结构体
typedef struct __MQTT_MSG
{
	u8 msgqos;
	u8 msg[MSG_MAX_LEN];
	u32 msglenth;
	u8 topic[MSG_TOPIC_LEN];
	u16 packetid;
	u8 valid;
}MQTT_USER_MSG;

// 需要ping的server ip结构体
typedef struct _Ping_ip 
{
	u8 ip[4];       //MQTT设置ip地址
}Ping_ip;

extern Ping_ip Ping_ip_array[10];

void MqttTask(void *pvParameters);
int MQTTClientInit(void);
int MQTTSubscribe(char* subtopic, enum QoS pos);
int MQTTMsgPublish(char* subtopic, enum QoS qos, u8 retained, u8* msg, int msg_len);
void mqtt_pktype_ctl(u8 packtype, u8 *buf, u32 buflen);
int my_mqtt_send_pingreq(void);
extern void open_the_door(void);

extern void GetCheckIP(void);

#endif /* MQTT_H_ */
