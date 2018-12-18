#ifndef __MQTTGPRSClient_H
#define __MQTTGPRSClient_H	 
#include "sys.h"

extern vu16 read_buf_len;

void reset(void);
void sendMQTT(u8* sourse, int buflen,u8* des);
void getMQTT(u8* sourse, int count,u8* des);
#endif

 



