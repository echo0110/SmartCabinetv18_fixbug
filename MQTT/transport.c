#include "transport.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "string.h"
#include "stm32f10x.h"
#include "core_cm3.h"
#include "system_stm32f10x.h"
#include "stdint.h"
#include "print.h"

static int mysock;

//extern struct sockaddr_in; 
	

/************************************************************************
** 函数名称: transport_sendPacketBuffer									
** 函数功能: 以TCP方式发送数据
** 入口参数: unsigned char* buf：数据缓冲区
**           int buflen：数据长度
** 出口参数: <0发送数据失败							
************************************************************************/

s32 transport_sendPacketBuffer( u8* buf, s32 buflen)
{
	s32 rc;
	rc = lwip_write(mysock, buf, buflen);
	return rc;
}

/************************************************************************
** 函数名称: transport_getdata									
** 函数功能: 以阻塞的方式接收TCP数据
** 入口参数: unsigned char* buf：数据缓冲区
**           int count：数据长度
** 出口参数: <=0接收数据失败									
************************************************************************/
s32 transport_getdata(u8* buf, s32 count)
{
	s32 rc;
	//这个函数在这里不阻塞
  rc = lwip_recv(mysock, buf, count, 0);
	return rc;
}


/************************************************************************
** 函数名称: transport_open									
** 函数功能: 打开一个接口，并且和服务器 建立连接
** 入口参数: char* servip:服务器域名
**           int   port:端口号
** 出口参数: <0打开连接失败										
************************************************************************/
s32 transport_open(s8* servip, s32 port)
{
	
	s32 ret;
	//s32 opt;
	
	struct sockaddr_in addr;
	s32 *sock = &mysock;
	
	//初始换服务器信息
	memset(&addr,0,sizeof(addr));
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	//填写服务器端口号
	addr.sin_port = PP_HTONS(port);
	//填写服务器IP地址
	addr.sin_addr.s_addr = inet_addr((const char*)servip);
	
	//创建SOCK
	*sock = lwip_socket(AF_INET,SOCK_STREAM,0);
	 if(*sock < 0)
   Printf("[ERROR] Create socket failed\n");
	//连接服务器 
	ret = lwip_connect(*sock,(struct sockaddr*)&addr,sizeof(addr));
	if(ret != 0)
	{
		 //关闭链接
		 close(*sock);
		 //连接失败
		 return -1;
	}
	//连接成功,设置超时时间1000ms
	//opt = 1000;
	//setsockopt(*sock,SOL_SOCKET,SO_RCVTIMEO,&opt,sizeof(int));
	
	//返回套接字
	return *sock;
}


/************************************************************************
** 函数名称: transport_close									
** 函数功能: 关闭套接字
** 入口参数: unsigned char* buf：数据缓冲区
**           int buflen：数据长度
** 出口参数: <0发送数据失败							
************************************************************************/
int transport_close(void)
{
	int rc;
	rc = lwip_close(mysock);
	return rc;
}
