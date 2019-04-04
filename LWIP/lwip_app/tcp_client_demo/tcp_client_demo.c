
#include "tcp_client_demo.h" 
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "stdio.h"
#include "string.h" 

#include "FreeRTOS.h"
#include "semphr.h"
#include "netif/etharp.h"
#include "MQTTGPRSClient.h"
#include "minIni.h"
#include "camera.h"

#include "lwip_comm.h" 
#include "message.h"
#include "rtc.h"
#include <cstdlib>
#include <cstdio>
#include "lwip/api.h"
#include "rtc.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK 战舰开发板 V3
//TCP Client 测试代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/3/15
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//*******************************************************************************
//修改信息
//无
////////////////////////////////////////////////////////////////////////////////// 	   

char  timestamp_str[20];

u8  Msg_Buf[512];	   //DMA接收串口数据缓冲区
 
//TCP Client接收数据缓冲区
u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	
//TCP服务器发送数据内容
//const u8 *tcp_client_sendbuf="WarShip STM32F103 TCP Client send data\r\n";//
//const u8 *tcp_client_sendbuf=DMA_Rece_Buf;
//TCP Client 测试全局状态标记变量
//bit7:0,没有数据要发送;1,有数据要发送
//bit6:0,没有收到数据;1,收到数据了.
//bit5:0,没有连接上服务器;1,连接上服务器了.
//bit4~0:保留

u8 tcp_client_init;	//
u8 tcp_client_flag;	//
u8 tcp_client_test_flag=0;
u8 timestamp_flag;//时间戳 

u8 tcp_close_flag;	//
 xQueueHandle timestamp_Queue;
 
extern  TaskHandle_t USBTask_Handler;

//设置远端IP地址
void tcp_client_set_remoteip(void)
{
	u8 *tbuf;
	u16 xoff;
	u8 key;	
	tbuf=(u8_t *)pvPortMalloc(100);
	if(tbuf==NULL)return;
	//前三个IP保持和DHCP得到的IP一致	
	lwipdev.remoteip[0]=lwipdev.ip[0];
	lwipdev.remoteip[1]=lwipdev.ip[1];
	lwipdev.remoteip[2]=lwipdev.ip[2]; 	
	//lwipdev.remoteip[3]=ini_getl("server",	"ip3",	142,inifile);	
	sprintf((char*)tbuf,"Remote IP:%d.%d.%d.",lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2]);//远端IP
	xoff=strlen((char*)tbuf)*8+30; 
	while(1)
	{
		 break;
	}
}

//TCP Client 测试
void tcp_client_test(void)
{
 	struct tcp_pcb *tcppcb;  	//定义一个TCP服务器控制块
	struct ip_addr rmtipaddr;  	//远端ip地址
  char inifile[] = "1:cfg.ini";  
	MSG msg;
	u8 *tbuf;
 	u8 key;
	u8 res=0;		
	u8 t=0; 
	u8 connflag=0;		//连接标记	
	int data_len;
	static int Count_packet=0;	
	tcp_client_set_remoteip();//先选择IP
	tbuf=(u8_t *)pvPortMalloc(200);
	if(tbuf==NULL)return ;		//内存申请失败了,直接退出
	
	//lwipdev.remoteip[3]=ini_getl("server",	"ip3",	142,inifile);	
	//printf("lwipdev.remoteip[3]=%d\n",lwipdev.remoteip[3]);
	
	sprintf((char*)tbuf,"Local IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//服务器IP
	sprintf((char*)tbuf,"Remote IP:%d.%d.%d.%d",lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],142);//远端IP
  //TCP_CLIENT_PORT=ini_getl("PORT",	"port0",	8090,	inifile);	
	sprintf((char*)tbuf,"Remotewo Port:%d",TCP_CLIENT_PORT);//客户端端口号
	tcppcb=tcp_new();	//创建一个新的pcb
	if(tcppcb)			//创建成功
	{
		IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],142); 
		tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);  //连接到目的地址的指定端口上,当连接成功后回调tcp_client_connected()函数
 	}else res=1;
	while(res==0)
	{
		if(key==4)break;
     if(tcp_close_flag&(1<<7))
		{
		 tcp_close_flag&=~(1<<7);
//     tcp_client_connection_close(tcppcb,0);//关闭TCP Client连接 				
     tcp_close(tcppcb);
     vTaskResume(cameraTask_Handler);//恢复camera任务	
     break;			
		}				
		if(tcp_client_flag&1<<6)//是否收到数据?		
		{			
			printf("recv data *\n");	
			tcp_client_flag&=~(1<<6);//标记数据已经被处理了.
		}		
		if(tcp_client_flag&1<<5)//是否连接上?
		{
			if(connflag==0)
			{ 								
				connflag=1;//标记连接了
			} 
		}else if(connflag)
		{
			connflag=0;	//标记连接断开了
		} 
		delay_ms(2);
		t++;
		if(t==200)
		{
			if(connflag==0&&(tcp_client_flag&1<<5)==0)//未连接上,则尝试重连
			{ 
				tcp_client_connection_close(tcppcb,0);//关闭连接
				tcppcb=tcp_new();	//创建一个新的pcb
				if(tcppcb)			//创建成功
				{ 
					tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);//连接到目的地址的指定端口上,当连接成功后回调tcp_client_connected()函数
				}
			}
			t=0;		
		}		
	}
	//tcp_client_connection_close(tcppcb,0);//关闭TCP Client连接
} 

//lwIP TCP连接建立后调用回调函数
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	struct tcp_client_struct *es=NULL;  
	if(err==ERR_OK)   
	{
		es=(struct tcp_client_struct*)mem_malloc(sizeof(struct tcp_client_struct));  //申请内存
		if(es) //内存申请成功
		{
 			es->state=ES_TCPCLIENT_CONNECTED;//状态为连接成功
			es->pcb=tpcb;  
			es->p=NULL; 
			tcp_arg(tpcb,es);        			//使用es更新tpcb的callback_arg
			tcp_recv(tpcb,tcp_client_recv);  	//初始化LwIP的tcp_recv回调功能   
			tcp_err(tpcb,tcp_client_error); 	//初始化tcp_err()回调函数
			tcp_sent(tpcb,tcp_client_sent);		//初始化LwIP的tcp_sent回调功能
		  tcp_poll(tpcb,tcp_client_poll,1); 	//初始化LwIP的tcp_poll回调功能 			
 			tcp_client_flag|=1<<5; 				//标记连接到服务器了
			err=ERR_OK;
		}else
		{ 
			tcp_client_connection_close(tpcb,es);//关闭连接
			err=ERR_MEM;	//返回内存分配错误
		}
	}else
	{
		tcp_client_connection_close(tpcb,0);//关闭连接
	}
	return err;
}
    
//lwIP tcp_recv()函数的回调函数
err_t tcp_client_recv(void *arg,struct tcp_pcb *tpcb,struct pbuf *p,err_t err)
{ 
	u32 data_len = 0;
	struct pbuf *q;
	struct tcp_client_struct *es;
	err_t ret_err; 
	LWIP_ASSERT("arg != NULL",arg != NULL);
	es=(struct tcp_client_struct *)arg; 
	if(p==NULL)//如果从服务器接收到空的数据帧就关闭连接
	{
		es->state=ES_TCPCLIENT_CLOSING;//需要关闭TCP 连接了 
 		es->p=p; 
		ret_err=ERR_OK;
	}else if(err!= ERR_OK)//当接收到一个非空的数据帧,但是err!=ERR_OK
	{ 
		if(p)pbuf_free(p);//释放接收pbuf
		ret_err=err;
	}else if(es->state==ES_TCPCLIENT_CONNECTED)	//当处于连接状态时
	{
		if(p!=NULL)//当处于连接状态并且接收到的数据不为空时
		{
			memset(tcp_client_recvbuf,0,TCP_CLIENT_RX_BUFSIZE);  //数据接收缓冲区清零
			for(q=p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
			{
				//判断要拷贝到TCP_CLIENT_RX_BUFSIZE中的数据是否大于TCP_CLIENT_RX_BUFSIZE的剩余空间，如果大于
				//的话就只拷贝TCP_CLIENT_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
				if(q->len > (TCP_CLIENT_RX_BUFSIZE-data_len)) memcpy(tcp_client_recvbuf+data_len,q->payload,(TCP_CLIENT_RX_BUFSIZE-data_len));//拷贝数据
				else memcpy(tcp_client_recvbuf+data_len,q->payload,q->len);
				data_len += q->len;  	
				if(data_len > TCP_CLIENT_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
			}
			tcp_client_flag|=1<<6;		//标记接收到数据了
 			tcp_recved(tpcb,p->tot_len);//用于获取接收数据,通知LWIP可以获取更多数据
			pbuf_free(p);  	//释放内存
			ret_err=ERR_OK;
		}
	}else  //接收到数据但是连接已经关闭,
	{ 
		tcp_recved(tpcb,p->tot_len);//用于获取接收数据,通知LWIP可以获取更多数据
		es->p=NULL;
		pbuf_free(p); //释放内存
		ret_err=ERR_OK;
	}
	return ret_err;
}
//lwIP tcp_err函数的回调函数
void tcp_client_error(void *arg,err_t err)
{  
	//这里我们不做任何处理
} 

//lwIP tcp_poll的回调函数
err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb)
{
	err_t ret_err;
	int data_len;
	static int count_tcp;
	static int recv_meg;
	struct tcp_client_struct *es;	
	MSG msg;
	es=(struct tcp_client_struct*)arg;
	if(es!=NULL)  //连接处于空闲可以发送数据
	{
		if(timestamp_flag&(1<<7))  //发送时间戳
		{
			static u32 RtcCounter = 0;
			u32 timestamp_buf[10];
			char  string[25];
			char  str[25];
			char topic[20];
			timestamp_buf[0]='@';
			timestamp_buf[1]='@';
		  timestamp_buf[2]=GPS_mktime(&calendar);//RTC_GetTime(NULL);	
      timestamp_buf[3]='*';	
      timestamp_buf[4]='*';	
			sprintf(topic, "%02X%02X%02X", STM32ID2, STM32ID1, STM32ID0);//机箱ID
			snprintf(str,11, "%d",  timestamp_buf[2]);
			memset(timestamp_str,0,20);
			snprintf(timestamp_str,18, "%d:%d.%s",0,timestamp_buf[2],"jpeg");//路径+时间戳
			memset(string,0,25);
      snprintf(string,22, "%c%c%s,%s%c%c", timestamp_buf[0],timestamp_buf[1],str,topic,timestamp_buf[3],timestamp_buf[4]);					
			es->p=pbuf_alloc(PBUF_TRANSPORT, 21,PBUF_POOL);	//申请内存 
			pbuf_take(es->p,string,21);	//将tcp_client_sentbuf[]中的数据拷贝到es->p_tx中
			tcp_client_senddata(tpcb,es);//将tcp_client_sentbuf[]里面复制给pbuf的数据发送出去
			timestamp_flag&=~(1<<7);	//清除数据发送标志			 
			if(es->p)pbuf_free(es->p);	//释放内存
			
			write_sd_test(0);//创建以时间戳命名的图片名		
			vTaskResume(cameraTask_Handler);	
		} 
		if(tcp_client_flag&(1<<7))	//判断是否有数据要发送
		{			
		  data_len=(DMA_Rece_Buf[3]<<8)|(DMA_Rece_Buf[2]&0x00ff);
			//write_sd_test(10);//图片写入SD卡	
			es->p=pbuf_alloc(PBUF_TRANSPORT, data_len,PBUF_POOL);	//申请内存 
			pbuf_take(es->p,(char*)&DMA_Rece_Buf[4],data_len);	//将tcp_client_sentbuf[]中的数据拷贝到es->p_tx中
			tcp_client_senddata(tpcb,es);//将tcp_client_sentbuf[]里面复制给pbuf的数据发送出去
 
			tcp_client_flag&=~(1<<7);	//清除数据发送标志
			Uart4_Over=0;			
			
			write_sd_test(10);//图片写入SD卡  放到这里  TCP可以传  但是写不进到SD卡里	
      vTaskResume(cameraTask_Handler);			
			printf("TCP  transport over\n");
			if(es->p)pbuf_free(es->p);	//释放内存			
		}      	  
		
		else if(es->state==ES_TCPCLIENT_CLOSING)
		{ 
 			tcp_client_connection_close(tpcb,es);//关闭TCP连接
		} 
		ret_err=ERR_OK;
	}else
	{ 
		tcp_abort(tpcb);//终止连接,删除pcb控制块
		ret_err=ERR_ABRT;
	}
	return ret_err;
} 
//lwIP tcp_sent的回调函数(当从远端主机接收到ACK信号后发送数据)
err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct tcp_client_struct *es;
	LWIP_UNUSED_ARG(len);
	es=(struct tcp_client_struct*)arg;
	if(es->p)tcp_client_senddata(tpcb,es);//发送数据
	return ERR_OK;
}
//此函数用来发送数据
void tcp_client_senddata(struct tcp_pcb *tpcb, struct tcp_client_struct * es)
{
	struct pbuf *ptr; 
 	err_t wr_err=ERR_OK;
	while((wr_err==ERR_OK)&&es->p&&(es->p->len<=tcp_sndbuf(tpcb)))
	{
		ptr=es->p;
		wr_err=tcp_write(tpcb,ptr->payload,ptr->len,1); //将要发送的数据加入到发送缓冲队列中
		if(wr_err==ERR_OK)
		{  
			es->p=ptr->next;			//指向下一个pbuf
			if(es->p)pbuf_ref(es->p);	//pbuf的ref加一
			pbuf_free(ptr);				//释放ptr 
		}else if(wr_err==ERR_MEM)es->p=ptr;
		tcp_output(tpcb);		//将发送缓冲队列中的数据立即发送出去
	} 	
} 
//关闭与服务器的连接
void tcp_client_connection_close(struct tcp_pcb *tpcb, struct tcp_client_struct * es)
{
	//移除回调
	tcp_abort(tpcb);//终止连接,删除pcb控制块
	tcp_arg(tpcb,NULL);  
	tcp_recv(tpcb,NULL);
	tcp_sent(tpcb,NULL);
	tcp_err(tpcb,NULL);
	tcp_poll(tpcb,NULL,0);  
	if(es)mem_free(es); 
	tcp_client_flag&=~(1<<5);//标记连接断开了
		 
//  vTaskResume(USBTask_Handler);	
//  vTaskResume(TickTask_Handler);
//  vTaskResume(MainTask_Handler);
	
	vTaskResume(cameraTask_Handler);//恢复camera任务
}





