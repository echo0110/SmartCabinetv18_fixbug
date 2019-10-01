#ifndef __TRANSPORT_H
#define __TRANSPORT_H


#include "stm32f10x.h"
#include "cc.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip_addr.h"
#include "inet.h"


/************************************************************************
** 函数名称: transport_sendPacketBuffer									
** 函数功能: 以TCP方式发送数据
** 入口参数: unsigned char* buf：数据缓冲区
**           int buflen：数据长度
** 出口参数: <0发送数据失败							
************************************************************************/
s32 transport_sendPacketBuffer( u8* buf, s32 buflen);

/************************************************************************
** 函数名称: transport_getdata									
** 函数功能: 以阻塞的方式接收TCP数据
** 入口参数: unsigned char* buf：数据缓冲区
**           int count：数据长度
** 出口参数: <=0接收数据失败									
************************************************************************/
s32 transport_getdata(u8* buf, s32 count);

/************************************************************************
** 函数名称: transport_open									
** 函数功能: 打开一个接口，并且和服务器 建立连接
** 入口参数: char* servip:服务器域名
**           int   port:端口号
** 出口参数: <0打开连接失败										
************************************************************************/
s32 transport_open(s8* servip, s32 port);

/************************************************************************
** 函数名称: transport_close									
** 函数功能: 关闭套接字
** 入口参数: unsigned char* buf：数据缓冲区
**           int buflen：数据长度
** 出口参数: <0发送数据失败							
************************************************************************/
s32 transport_close(void);

struct sockaddr_in {
  u8_t sin_len;
  u8_t sin_family;
  u16_t sin_port;
  struct in_addr sin_addr;
  char sin_zero[8];
};

#endif
