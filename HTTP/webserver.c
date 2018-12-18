// Copyright (c)2011 by Aaron, All Rights Reserved.
/*----------------------------------------------------------------------------+
|  File Name:  httpd.c, v1.0                                                  |
|  Author:     aaron.xu1982@gmail.com                                         |
|  Date:       2011.10.06                                                     |
+-----------------------------------------------------------------------------+
|  Description: 基于lwIP的http网页复位进程。                                   |
|               监听80端口，如果接收到http请求则从文件系统读取相应的文件并返     |
|               回，如果文件不存在的返回404.htm。只接受'GET'请求。              |
|                                                                             |
+-----------------------------------------------------------------------------+
|  Release Notes:                                                             |
|                                                                             |
|  Logs:                                                                      |
|  WHO       WHEN         WHAT                                                |
|  ---       ----------   --------------------------------------------------  |
|  Aaron     2011.10.06   born                                                |
|  Aaron     2011.12.20   增加跟服务器通信                                     |
|                                                                             |
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| Include files                                                               |
+----------------------------------------------------------------------------*/
// Standard includes.
#include <stdio.h>
#include <string.h>

//#include "stm32f10x_lib.h"

// Scheduler includes.
//#include "FreeRTOS.h"
//#include "task.h"

// misc includes
//#include "config.h"
//#include "rtc.h"
//#include "tcpclient.h"
//#include "sntp.h"

// lwIP includes.
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/inet.h"

#include "webserver.h"
#include "fattester.h"

//#include "netif/loopif.h"

// FATFS includes.
#include "ff.h"
#include "exfuns.h"
//#include "OSFF.h"

//#include "Print.h"

/*----------------------------------------------------------------------------+
| Type Definition & Macro                                                     |
+----------------------------------------------------------------------------*/

const unsigned char *SerialNumber = (unsigned char *)(0x1FFFF7E8);

const char BuildDate[]      = __DATE__;
const char BuildTime[]      = __TIME__;
const char ReleaseVersion[] = "1.0.1";

#define HTTP_DEBUG1(x)		PrintS(x)
#define HTTP_DEBUG2(x, y)	Printf(x, y)

// 定义发送数据帧的最大长度.
#define webMAX_PAGE_SIZE	496

// Maximum number of arguments supported by this HTTP Server.
#define MAX_HTTP_ARGS       21

// The port on which we listen.
#define webHTTP_PORT		80

#define webSHORT_DELAY		10

// 定义解释CGI文件时不同的变量对应的意思
#define VAR_SERIAL_NUMBER   (0x11)
#define VAR_FW_VERSION		(0x12)
#define VAR_FW_DATE			(0x13)
#define VAR_FW_TIME			(0x14)

#define VAR_RUN_TIME		(0x21)
#define VAR_POWER_TIME		(0x22)

#define VAR_MAC_ADDR		(0x31)
#define VAR_DEFAULT_MAC		(0x32)
#define VAR_HOST_NAME		(0x33)
#define VAR_DHCP_EN			(0x34)
#define VAR_IP_ADDR			(0x35)
#define VAR_SUBNET_MASK		(0x36)
#define VAR_GATWAY_ADDR		(0x37)
#define VAR_DNS_SERVER		(0x38)
#define VAR_DNS_SERVER_BAK	(0x39)

#define VAR_SERVER_ADDR		(0x41)
#define VAR_SERVER_PORT		(0x42)
#define VAR_SEND_OFFSET		(0x43)
#define VAR_SNTP_UPDATE		(0x44)

#define VAR_VOICE_RATE		(0x51)
#define VAR_MODEM_SPEED		(0x52)

// Supported HTTP Commands
typedef enum _HTTP_COMMAND
{
    HTTP_GET,
    HTTP_POST,
    HTTP_NOT_SUPPORTED,
    HTTP_INVALID_COMMAND
} HTTP_COMMAND;

// File type definitions
typedef enum _HTTP_FILE_TYPE
{
	HTTP_TXT = 0u,		// File is a text document
	HTTP_HTM,			// File is HTML ( .htm)
	HTTP_HTML,			// File is HTML ( .html)
	HTTP_CGI,			// File is HTML ( .cgi)
	HTTP_XML,			// File is XML ( .xml)
	HTTP_CSS,			// File is stylesheet ( .css)
	HTTP_XBM,			// File is X-Windows bitmap (b/w) ( .xbm)
	HTTP_BMP,			// File is Microsoft Windows bitmap ( .bmp)
	HTTP_GIF,			// File is GIF image ( .gif)
	HTTP_PNG,			// File is PNG image ( .png)
	HTTP_JPG,			// File is JPG image ( .jpg)
	HTTP_WAV,			// File is audio ( .wav)
	HTTP_MP3,			// File is audio ( .mp3)
	HTTP_MMID,			// File is MIDI music data ( .mid)
	HTTP_AVI,			// File is Microsoft video ( .avi)
	HTTP_JAVA,			// File is java ( .java)
	HTTP_RTF,			// File is Microsoft Rich Text Format ( .rtf)
	HTTP_PDF,			// File is Adobe Acrobat PDF ( .pdf)
	HTTP_DOC,			// File is Microsoft Word ( .doc)
	HTTP_PPT,			// File is Microsoft PowerPoint ( .ppt)
	HTTP_XLS,			// File is Microsoft Excel ( .xls)
	HTTP_ZIP,			// File is DOS/PC - Pkzipped archive ( .zip)
	HTTP_EXE,			// File is PC executable ( .exe)
	HTTP_UNKNOWN		// File type is unknown
} HTTP_FILE_TYPE;

// Response html header
typedef enum
{
	HTTP_HDR_OK = 0,
	HTTP_HDR_OK_NO_CACHE,
	HTTP_HDR_302,
	HTTP_HDR_401,
	HTTP_HDR_403,
	HTTP_HDR_404,
	HTTP_HDR_414,
	HTTP_HDR_500,
	HTTP_HDR_501,
} HTTP_RESP_HEADER;

// Each dynamic variable within a CGI file should be preceeded with this character.
#define HTTP_VAR_CHAR			'%'
#define HTTP_DYNAMIC_FILE_TYPE  (HTTP_CGI)

#define HTTP_NET_WRITE(a, b, c, d)		\
	err = netconn_write(a, b, c, d);	\
	if (err != ERR_OK)					\
		HTTP_DEBUG2("\nE: %d", err);

/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

#if LWIP_DHCP
		extern struct dhcp **pSys_dhcp;
#endif
/*----------------------------------------------------------------------------+
| Internal Variables                                                          |
+----------------------------------------------------------------------------*/
// Module variable used as buffer to build the dynamic debug.htm page.
static char s_HttpBuffer[webMAX_PAGE_SIZE];

// Module variable that store the hits count of the debug.htm page.
static u32 s_nPageHits = 0;

// HTTP Command Strings
static const char HTTP_GET_STRING[]  = "GET ";
#define HTTP_GET_STRING_LEN			(sizeof(HTTP_GET_STRING)-1)
static const char HTTP_POST_STRING[] = "POST ";
#define HTTP_POST_STRING_LEN		(sizeof(HTTP_POST_STRING)-1)

// defines the directory which contains the http sever files
static const char HTTP_WEBPAGE_DIR[] = "1:/webpages";
#define HTTP_WEBPAGE_DIR_LEN		(sizeof(HTTP_WEBPAGE_DIR)-1)

// Default HTML file.
static const char HTTP_DEFAULT_HTM[] = "index.htm";
#define HTTP_DEFAULT_HTM_LEN		(sizeof(HTTP_DEFAULT_HTM)-1)

// Default 404 file.
static const char HTTP_DEFAULT_404[] = "404.htm";
#define HTTP_DEFAULT_404_LEN		(sizeof(HTTP_DEFAULT_404)-1)

static const char HTTP_HEADER_END_STR[] = "\r\n\r\n";
#define HTTP_HEADER_END_STR_LEN		(sizeof(HTTP_HEADER_END_STRING)-1)

// Format of the dynamic page.
static const char debugHTML_START[] =
"<html>\
<head>\
</head>\
<BODY onLoad=\"window.setTimeout(&quot;location.href='debug.htm'&quot;,2000)\" bgcolor=\"#FFFFFF\" text=\"#2477E6\">\
\r\nPage Hits = ";

static const char debugHTML_END[] =
"\r\n</pre>\
\r\n</font></BODY>\
</html>";

// File type extensions corresponding to HTTP_FILE_TYPE
static const char * const httpFileExtensions[HTTP_UNKNOWN+1] =
{
	"txt",				// HTTP_TXT
	"htm",				// HTTP_HTM
	"html",				// HTTP_HTML
	"cgi",				// HTTP_CGI
	"xml",				// HTTP_XML
	"css",				// HTTP_CSS
	"xbm",				// HTTP_XBM
	"bmp",				// HTTP_BMP
	"gif",				// HTTP_GIF
	"png",				// HTTP_PNG
	"jpg",				// HTTP_JPG
	"wav",				// HTTP_WAV
	"mp3",				// HTTP_MP3
	"mid",				// HTTP_MMID
	"avi",				// HTTP_AVI
	"cla",				// HTTP_JAVA
	"rtf",				// HTTP_RTF
	"pdf",				// HTTP_PDF
	"doc",				// HTTP_DOC
	"ppt",				// HTTP_PPT
	"xls",				// HTTP_XLS
	"zip",				// HTTP_ZIP
	"exe",				// HTTP_EXE
	"\0\0\0"			// HTTP_UNKNOWN
};

// Content-type strings corresponding to HTTP_FILE_TYPE
static const char * const httpContentTypes[HTTP_UNKNOWN+1] =
{
	"text/plain",				// HTTP_TXT
	"text/html",				// HTTP_HTM
	"text/html",				// HTTP_HTML
	"text/html",				// HTTP_CGI
	"text/xml",					// HTTP_XML
	"text/css",					// HTTP_CSS
	"image/x-xbitmap",			// HTTP_XBM
	"image/x-ms-bmp",			// HTTP_BMP
	"image/gif",				// HTTP_GIF
	"image/png",				// HTTP_PNG
	"image/jpeg",				// HTTP_JPG
	"audio/x-wave",				// HTTP_WAV
	"audio/mpeg",				// HTTP_MP3
	"x-music/x-midi",			// HTTP_MMID
	"video/x-msvideo",			// HTTP_AVI
	"application/java-vm",		// HTTP_JAVA
	"application/rtf",			// HTTP_RTF
	"application/pdf",			// HTTP_PDF
	"application/vnd.ms-word",	// HTTP_DOC
	"application/vnd.ms-powerpoint",	// HTTP_PPT
	"application/vnd.ms-excel",	// HTTP_XLS
	"application/zip",			// HTTP_ZIP
	"application/octet-stream",	// HTTP_EXE
	""	// HTTP_UNKNOWN
};

// Initial response strings (Corresponding to HTTP_STATUS)
static const char * const HttpHeaderString[] =
{
	"HTTP/1.1 200 OK\r\nContent-type: ",
	"HTTP/1.1 200 OK\r\nDate: Mon, 27 Jul 2009 12:53:00 GMT\r\nExpires: Mon, 27 Jul 2009 12:54:00 GMT\r\nCache-control: private\r\nContent-type: ",
	"HTTP/1.1 302 Found\r\nConnection: close\r\nLocation: ",
	"HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Protected\"\r\nConnection: close\r\n\r\n401 Unauthorized: Password required\r\n",
	"HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\n403 Forbidden: SSL Required - use HTTPS\r\n",
	"HTTP/1.1 404 Not found\r\nConnection: close\r\n\r\n404: File not found\r\n",
	"HTTP/1.1 414 Request-URI Too Long\r\nConnection: close\r\n\r\n414 Request-URI Too Long: Buffer overflow detected\r\n",
	"HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n500 Internal Server Error: Expected data not present\r\n",
	"HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\n501 Not Implemented: Only GET and POST supported\r\n",
};

/*----------------------------------------------------------------------------+
| Function Prototype                                                          |
+----------------------------------------------------------------------------*/
void ParseHTMLRequest(struct netconn *pxNetCon);
void HttpExecCmd(char** argv, u8 argc);
int  HttpGetVar(char VarID, char *RxBuf);
char Hexatob(short hex);
char* HttpUrlDecode(char* cData);
HTTP_COMMAND HttpCmdParse(char *string, char** argv, char* argc, char* type);
FIL* Http_fOpen(char* filename);	// Find and open a file
void Http_fClose(FIL* handle);

/*----------------------------------------------------------------------------+
| System Initialization Routines                                              |
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Main Routine                                                                |
+----------------------------------------------------------------------------*/
/*
 * Web server task function.
 */
void web_thread(void *pv)
{
	struct netconn *pHTTPListener;
	struct netconn *pNewConnection;

	// Create a new tcp connection handle.
	pHTTPListener = netconn_new(NETCONN_TCP);
	netconn_bind(pHTTPListener, NULL, webHTTP_PORT);	// bind to port 80
	netconn_listen(pHTTPListener);

	while (1)
	{
		// Wait for a first connection.
		netconn_accept(pHTTPListener, &pNewConnection);

		if (pNewConnection != NULL)
		{
#ifdef HTTP_DEBUG
			HTTP_DEBUG1("\n$http request begin: ");
#endif
//			ParseHTMLRequest(pNewConnection);

			netconn_close(pNewConnection);
			while (netconn_delete(pNewConnection) != ERR_OK)	// 观察pKeyGetConn是否会变成NULL值
			{
#ifdef HTTP_DEBUG
				HTTP_DEBUG1("\n$netconn_delete wait");
#endif
				vTaskDelay(webSHORT_DELAY);
			}
#ifdef HTTP_DEBUG
			HTTP_DEBUG1("\n$request end");
#endif
		}
	}
}

/*----------------------------------------------------------------------------+
| General Subroutines                                                         |
+----------------------------------------------------------------------------*/
#if 0	// 如果不使用密码登录，则下面的Base64编解码函数可以不编译
/* Base64 编码表 */
static const char Base64_EncTbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/*
 * 对给定的字符串进行Base64加密编码并输出加密后的密文
 * psrc: 待加密的明文字符串
 * len : 待加密的明文字符串的长度
 * psec: 经Base64转换后的密文
 * 返回: 转换后的Base64编码的长度
 * 备注: 每次从缓冲区中读取3个字节，并转换为4个字节的密文，不足4个字节用'='补齐。
 *       要给存放Base64编码的缓冲区预留足够的长度，因为本函数不会检查缓冲区的大小
 */
u32 Base64_Encode(char *psrc, u32 len, char *psec)
{
	u32 i;
	u32 nLength = 0;
	char c1, c2, c3;

	/* 先转换3的整数倍个字节 */
	for (i=3; i<len; i+=3)
	{
		/* 读入3个字节 */
		c1 = *psrc ++;
		c2 = *psrc ++;
		c3 = *psrc ++;

		/* 将3个字节的原始数据转换为4个字节的Base64编码 */
		*psec++ = Base64_EncTbl[c1 >> 2];
		*psec++ = Base64_EncTbl[((c1 << 4) | (c2 >> 4)) & 0x3F];
		*psec++ = Base64_EncTbl[((c2 << 2) | (c3 >> 6)) & 0x3F];
		*psec++ = Base64_EncTbl[c3 & 0x3F];
		nLength += 4;
		/* 根据RFC822，Base64编码每行后面加上回车换行'\r\n' */
		if ((i % 57) == 0)
		{
			*psec++ = '\r';
			*psec++ = '\n';
			nLength += 2;
		}
	}
	i -= 3;
	/* 转换剩余的不是3的整数倍的字节 */
	if ((len - i) == 1)
	{
		/* 剩余一个字节 */
		c1 = *psrc;
		*psec++ = Base64_EncTbl[c1 >> 2];
		*psec++ = Base64_EncTbl[(c1 << 4) & 0x3F];
		*psec++ = '=';
		*psec++ = '=';
		nLength += 4;
	}
	else if ((len - i) == 2)
	{
		c1 = *psrc ++;
		c2 = *psrc;
		*psec++ = Base64_EncTbl[c1 >> 2];
		*psec++ = Base64_EncTbl[((c1 << 4) | (c2 >> 4)) & 0x3F];
		*psec++ = Base64_EncTbl[(c2 << 2) & 0x3F];
		*psec++ = '=';
		nLength += 4;
	}

	return nLength;
}

/* Base64 解码表 */
static const char Base64_DecTbl[] =
{
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,	 0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,	// '+' '/'
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,	// '0'-'9'
	 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 'A'-'Z'
	15,	16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
	 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,	// 'a'-'z'
	 41,42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0
};
/*
 * 对经过Base64加密的字符串进行解码并输出
 * psrc : 已加密的Base64字符串
 * len  : 已加密的Base64字符串的长度
 * pdata: 解码后的明文字符串
 * 返回 : 解码后的明文字符串的长度
 * 备注 : 每次从缓冲区中读取4个字节，并转换为3个字节的明文。
 *        要给存放明文编码的缓冲区预留足够的长度，因为本函数不会检查缓冲区的大小
 */
u32 Base64_Decode(char *psrc, u32 len, char *pdata)
{
	u32 i = 0;
	u32 nLength = 0;
	char c1, c2, c3, c4;
	
	while (i < len)
	{
		if ((*psrc == '\r') || (*psrc == '\n'))
		{
			psrc ++;
			i ++;
			continue;
		}
		/* 读入4个字节 */
		c1 = *psrc ++;
		c2 = *psrc ++;
		c3 = *psrc ++;
		c4 = *psrc ++;
		c1 = Base64_DecTbl[c1];
		c2 = Base64_DecTbl[c2];
		
		/* 将4个字节的Base64编码转换为3个字节的原始数据 */
		*pdata++ = (c1 << 2) | (c2 >> 4);	// ((c1 << 2) & 0xFC) | ((c2 >> 4) & 0x03)
		nLength ++;
		if (c3 != '=')
		{
			c3 = Base64_DecTbl[c3];
			*pdata++ = (c2 << 4) | (c3 >> 2);	// ((c2 << 4) & 0xF0) | ((c3 >> 2) & 0x0F)
			nLength ++;

			if (c4 != '=')
			{
				c4 = Base64_DecTbl[c4];
				*pdata++ = (c3 << 6) | (c4);
				nLength ++;
			}
		}

		i += 4;
	}
	return nLength;
}
#endif
/*
 * Parse the incoming HTML request and send file.
 *
 * pxNetCon : The netconn to use to send and receive data.
 */
void ParseHTMLRequest(struct netconn *pxNetCon)
{
	char *argv[MAX_HTTP_ARGS];
	char argc;
	char FileType;
	char *pRxString;
	char strPageHits[10];
	struct netbuf *pRxBuffer;
	HTTP_COMMAND httpCommand;
	unsigned short usLength;
	FIL* fp = NULL;
	u32 i;

	// We expect to immediately get data.
	netconn_recv(pxNetCon, &pRxBuffer);

	if (pRxBuffer != NULL)
	{
		// Get the received data
		netbuf_data(pRxBuffer, (void *)&pRxString, &usLength);

		// Parse the http command string
		httpCommand = HttpCmdParse(pRxString, argv, &argc, &FileType);
	
		// Is this a GET?  We don't handle anything else.
		if (httpCommand == HTTP_GET)
		{
			// If there are any arguments, this must be a remote command. Execute it and
			// then send the file. The file name may be modified by command handler.
			if (argc > 1u)
			{
				// handle this remote command.
				HttpExecCmd(argv, argc);
			}

			// Is the page requested debug.html ?
			if ((strcmp(argv[0], "/debug.htm") == 0) ||
				(strcmp(argv[0], "/debug.html") == 0))
			{
				// Update the hit count.
				++s_nPageHits;
				sprintf(strPageHits, "%d", (int)s_nPageHits);
				
				// Write out the HTTP OK header.
				strcpy(s_HttpBuffer, HttpHeaderString[HTTP_HDR_OK]);
				strcat(s_HttpBuffer, httpContentTypes[HTTP_HTML]);
				strcat(s_HttpBuffer, HTTP_HEADER_END_STR);

				// Generate the dynamic page... First the page header.
				strcat(s_HttpBuffer, debugHTML_START);
				
				// ... Then the hit count...
				strcat(s_HttpBuffer, strPageHits);
				strcat(s_HttpBuffer, "<p><pre>Task            State    Priority   Stack    #<br>************************************************<br>");
				
				// ... Then the list of tasks and their status...
				vTaskList((char *) s_HttpBuffer + strlen(s_HttpBuffer));

				// ... Finally the page footer.
				strcat(s_HttpBuffer, debugHTML_END);
			}
			else if (strcmp(argv[0], "/0") == 0)
			{
				// is a special control code, should parse the parameter
				if (strcmp(argv[1], "0") == 0)
				{
					if (strcmp(argv[2], "LED1") == 0)
					{
						//
					}
					else if (strcmp(argv[2], "LED2") == 0)
					{
					}
				}
				else if (strcmp(argv[1], "1") == 0)
				{
				}
				s_HttpBuffer[0] = '\0';
			}
			else if (strcmp(argv[0], "/1") == 0)
			{
				// is a special control code, should parse the parameter
				if ((argc >= 3)
				 && (strcmp(argv[1], "3") == 0))
				{
//					PrintS("\n");
//					PrintS(argv[2]);
				}
				s_HttpBuffer[0] = '\0';
			}
			else
			{
				// find and open the required file
				fp = Http_fOpen(argv[0]);
				if (fp == NULL)
				{
					// 404 ERROR
					strcpy(s_HttpBuffer, HttpHeaderString[HTTP_HDR_404]);
				}
			}

			if (fp == NULL)
			{
				// Write out the dynamically generated page.
				u32_t nSize = strlen(s_HttpBuffer);
				char *pData = s_HttpBuffer;
//				err_t  err;
#ifdef HTTP_DEBUG
				HTTP_DEBUG2("\nS: %d", nSize);
#endif
				netconn_write(pxNetCon, pData, nSize, NETCONN_NOCOPY);
			}
			else	// (fp != NULL)
			{
				UINT nSize;
//				err_t err;

				// CGI file
				if (FileType == HTTP_CGI)
				{
					char FileBuf[40];	// 所以通过CGI解析的字符串的长度(即HttpGetVar()返回的长度)不能超过40个字节
					char *pHttpBuf = s_HttpBuffer;
					char temp;
					u32  len;
					short hex;

					// Write out the HTTP OK & NOCACHE header.
					strcpy(s_HttpBuffer, HttpHeaderString[HTTP_HDR_OK_NO_CACHE]);
					strcat(s_HttpBuffer, httpContentTypes[FileType]);
					strcat(s_HttpBuffer, HTTP_HEADER_END_STR);
					nSize = strlen(s_HttpBuffer);
					netconn_write(pxNetCon, s_HttpBuffer, nSize, NETCONN_COPY);

					// Parse the CGI file content
					while (1)
					{
						f_read(fp, FileBuf, 1, &nSize);
						if (nSize < 1) break;
						if (FileBuf[0] == HTTP_VAR_CHAR)
						{
							f_read(fp, FileBuf, 2, &nSize);
							hex = (short)FileBuf[0] << 8;
							hex |= FileBuf[1];
							temp = Hexatob(hex);
							len = HttpGetVar(temp, FileBuf);
						}
						else
						{
							len = 1;
						}
						for (i=0; i<len; i++)
						{
							// put the data into Http send buffer
							*pHttpBuf++ = FileBuf[i];
							if (pHttpBuf >= s_HttpBuffer + webMAX_PAGE_SIZE)
							{
#ifdef HTTP_DEBUG
//								HTTP_DEBUG2("\nS: %d", webMAX_PAGE_SIZE);
#endif
								netconn_write(pxNetCon, s_HttpBuffer, webMAX_PAGE_SIZE, NETCONN_COPY);
								pHttpBuf = s_HttpBuffer;
							}
						}
					}
					// Send the parsed html buffer
					if (pHttpBuf != s_HttpBuffer)
					{
						nSize = (int)(pHttpBuf - s_HttpBuffer);
#ifdef HTTP_DEBUG
						HTTP_DEBUG2("\nS: %d", nSize);
#endif
						netconn_write(pxNetCon, s_HttpBuffer, nSize, NETCONN_COPY);
					}
				}
				// Static html page file
				else
				{
					// Write out the HTTP OK header.
					strcpy(s_HttpBuffer, HttpHeaderString[HTTP_HDR_OK]);
					strcat(s_HttpBuffer, httpContentTypes[FileType]);
					strcat(s_HttpBuffer, HTTP_HEADER_END_STR);
					nSize = strlen(s_HttpBuffer);
					netconn_write(pxNetCon, s_HttpBuffer, nSize, NETCONN_COPY);
	
					// Write out the file contants
					while (1)
					{
						f_read(fp, s_HttpBuffer, webMAX_PAGE_SIZE, &nSize);
						//
						//HTTP_DEBUG2("\nS: %d", nSize);
						netconn_write(pxNetCon, s_HttpBuffer, nSize, NETCONN_COPY);
						//
						if (nSize < webMAX_PAGE_SIZE) break;
					}
				}
				// Close file
				Http_fClose(fp);
				fp = NULL;
			}
		}
		else if (httpCommand == HTTP_POST)
		{
			// do nothing
		}

		netbuf_delete(pRxBuffer);
	}
}

/*
 * Function:    void HttpExecCmd(BYTE** argv, BYTE argc)
 *
 * Input:       argv  - List of arguments
 *              argc  - Argument count.
 *
 * Overview:    This function is a "callback" from HTTPServer task. Whenever a
 *              remote node performs interactive task on page that was served,
 *              HTTPServer calls this functions with action arguments info.
 *              Main application should interpret this argument and act accordingly.
 *
 *              Following is the format of argv:
 *              If HTTP action was : thank.htm?name=Joe&age=25
 *                      argv[0] => thank.htm
 *                      argv[1] => name
 *                      argv[2] => Joe
 *                      argv[3] => age
 *                      argv[4] => 25
 *
 *              Use argv[0] as a command identifier and rests of the items as
 *              command arguments.
 */
void HttpExecCmd(char **argv, u8 argc)
{
    u8  var;
	u8  var2;
    u8  CurrentArg;
    u16 TmpWord;
	u32 tmp;
//	bool bNeedReboot = FALSE;
	struct in_addr ip;

	// Loop through all variables that we've been given
	CurrentArg = 1;
	while (CurrentArg < argc)
	{
		/* 网络参数设置页 */
		if (strcmp(argv[CurrentArg], "mac") == 0)
		{
			/* 设置本机的MAC地址 */
			CurrentArg ++;
			if (CurrentArg >= argc) break;

			if (strlen(argv[CurrentArg]) != 17)	// MAC地址的长度为17个字节
				continue;
			for (tmp=2; tmp<17; tmp+=3)	// 检查字节之间的'-'分隔符
			{
				if (argv[CurrentArg][tmp] != '-')
					break;
			}
			if (tmp < 17) continue;		// 如果'-'的位置不正确，则忽略当前的MAC地址设置

//			u8 mac[6];
			for (tmp=0; tmp<6; tmp++)
			{
				var  = argv[CurrentArg][tmp*3];
				var2 = argv[CurrentArg][tmp*3+1];
				/* 将两个MAC地址的字符转换为一个ASCII码 */
				if ((var >= 'a') && (var <= 'f'))
				{
					var -= 'a' - 0x0A;
				}
				else if ((var >= 'A') && (var <= 'F'))
				{
					var -= 'A' - 0x0A;
				}
				else if ((var >= '0') && (var <= '9'))
				{
					var -= '0';
				}
				else
				{
					break;				// 如果不是16进制字符，则退出循环
				}
				if ((var2 >= 'a') && (var2 <= 'f'))
				{
					var2 -= 'a' - 0x0A;
				}
				else if ((var2 >= 'A') && (var2 <= 'F'))
				{
					var2 -= 'A' - 0x0A;
				}
				else if ((var2 >= '0') && (var2 <= '9'))
				{
					var2 -= '0';
				}
				else
				{
					break;
				}

//				mac[tmp] = (var << 4) | var2;
			}
			if (tmp == 6)
			{
//				memcpy(conf.emac_addr, mac, 6);
			}
		}
		else if (strcmp(argv[CurrentArg], "hostName") == 0)
		{
			/* 设置网络唯一主机名称 */
			CurrentArg ++;
			if (CurrentArg >= argc) break;

//			FormatNetBIOSName(argv[CurrentArg]);
//			strncpy(conf.NetBIOSName, argv[CurrentArg], 16);
		}
		else if (strcmp(argv[CurrentArg], "dhcpEn") == 0)
		{
			/* 设置是否启用DHCP获取IP地址 */
			CurrentArg ++;
			if (CurrentArg >= argc) break;

			if (argv[CurrentArg][0] == '0')
			{
//				conf.DHCP_en = 0;
			}
			else
			{
//				conf.DHCP_en = 1;
			}
		}
		else if (strcmp(argv[CurrentArg], "ip") == 0)
		{
			/* 设置本机IP地址 */
			if (inet_aton(argv[CurrentArg+1], &ip))
			{
				CurrentArg ++;
//				conf.LocalIp = htonl(ip.s_addr);
			}
		}
		else if (strcmp(argv[CurrentArg], "mask") == 0)
		{
			/* 设置子网掩码 */
			if (inet_aton(argv[CurrentArg+1], &ip))
			{
				CurrentArg ++;
//				conf.SubnetMask = htonl(ip.s_addr);
			}
		}
		else if (strcmp(argv[CurrentArg], "gateway") == 0)
		{
			/* 设置网关地址 */
			if (inet_aton(argv[CurrentArg+1], &ip))
			{
				CurrentArg ++;
//				conf.GetwayIp = htonl(ip.s_addr);
			}
		}
		else if (strcmp(argv[CurrentArg], "dnsserver") == 0)
		{
#if 1	// LWIP_DNS
			/* 设置DNS服务器地址 */
			if (inet_aton(argv[CurrentArg+1], &ip))
			{
				CurrentArg ++;
//				conf.DnsAddr = htonl(ip.s_addr);
			}
#endif	// LWIP_DNS
		}
		else if (strcmp(argv[CurrentArg], "dnsserver2") == 0)
		{
#if 1	// LWIP_DNS
			/* 设置备用DNS服务器地址 */
			if (inet_aton(argv[CurrentArg+1], &ip))
			{
				CurrentArg ++;
//				conf.DnsAddr2 = htonl(ip.s_addr);
			}
#endif	// LWIP_DNS
		}
		else if (strcmp(argv[CurrentArg], "reboot") == 0)
		{
			/* 设置完毕，需要重启以更新网络配置 */
			CurrentArg ++;
			if (CurrentArg >= argc) break;

			if (argv[CurrentArg][0] == '1')
			{
//				bNeedReboot = TRUE;
			}
		}
		
		/* 服务器参数设置页 */
		else if (strcmp(argv[CurrentArg], "ServerIp") == 0)
		{
			/* 设置服务器IP地址 */
			if (inet_aton(argv[CurrentArg+1], &ip))
			{
				CurrentArg ++;
//				conf.ServerIp = htonl(ip.s_addr);
			}
		}
		else if (strcmp(argv[CurrentArg], "ServerPort") == 0)
		{
			/* 设置服务器端口号 */
			TmpWord = atoi(argv[CurrentArg+1]);
			if (TmpWord >= 1024)
			{
				CurrentArg ++;
//				if(TmpWord == 8293)conf.Signature = 0x1702A;
//				else if(TmpWord == 4444)
//				{
//					extern void SystemReset(void);
//					SystemReset();
//				}
//				else conf.ServerPort = TmpWord;
			}
		}
		else if (strcmp(argv[CurrentArg], "Timeout") == 0)
		{
			/* 设置服务器端口号 */
			TmpWord = atoi(argv[CurrentArg+1]);
			if (TmpWord >= 3)
			{
				CurrentArg ++;
//				conf.tcpSendTimeout = TmpWord*1000;
			}
		}
		else if (strcmp(argv[CurrentArg], "Sntp") == 0)
		{
			/* 设置服务器端口号 */
			TmpWord = atoi(argv[CurrentArg+1]);
			if (TmpWord >= 3)
			{
				CurrentArg ++;
//				conf.sntpUpdateDelay = TmpWord*1000;
			}
		}
		else
		{
		}
		// Advance to the next variable (if present)
		CurrentArg++;	
	}
	
	// Save any changes to non-volatile memory
//	config_save();

	// if we need reboot?
//	if (bNeedReboot)
	{
//		extern void SystemReset(void);
//		SystemReset();
	}
}

/*
 * Function:    void HttpGetVar(char VarID, char *RxBuf)
 *
 * Input:       VarID - Variable Identifier
 *              RxBuf - Buffer for value storage.
 *
 * Output:      Variable length in RxBuf.
 *
 * Overview:    Whenever a variable substitution is required on any html pages,
 *              ParseHTMLRequest calls this function 8-bit variable identifier.
 */
int HttpGetVar(char VarID, char *RxBuf)
{
	int i;
	char buf[8];
	char *pstr;
//	u8   temp;
	struct in_addr ip;
	int rt = 0;
	const char HexTab[] = "0123456789ABCDEF";;

	switch (VarID)
	{
	case VAR_SERIAL_NUMBER:				/* 本机序列号，等于STM32单片机的全球唯一序列号 */
		for (i=0; i<12; i++)
		{
			*RxBuf++ = HexTab[SerialNumber[i] >> 4];
			*RxBuf++ = HexTab[SerialNumber[i]&0x0F];
			if (i != 11) *RxBuf++ = ' ';
		}
		rt = 35;
		break;
	case VAR_FW_VERSION:				/* 软件版本 */
		rt = strlen(ReleaseVersion);
		strncpy(RxBuf, ReleaseVersion, rt);
		break;
	case VAR_FW_DATE:					/* 软件编译日期 */
		rt = strlen(BuildDate);
		strncpy(RxBuf, BuildDate, rt);
		break;
	case VAR_FW_TIME:
		rt = strlen(BuildTime);
		strncpy(RxBuf, BuildTime, rt);
		break;
	case VAR_RUN_TIME:					/* 系统从复位到现在所经过的时间 */
//		RTC_GetTimeString(SysRunTimer, RxBuf);
		rt = strlen(RxBuf);
		break;
	case VAR_POWER_TIME:				/* 系统从上电到现在所经过的时间 */
//		RTC_GetTimeString(SysPowerTimer, RxBuf);
		rt = strlen(RxBuf);
		break;

	case VAR_MAC_ADDR:					/* 当前的本机MAC地址 */
//		pstr = (char *)conf.emac_addr;
		for (i=0; i<6; i++)
		{
			*RxBuf++ = HexTab[pstr[i] >> 4];
			*RxBuf++ = HexTab[pstr[i]&0x0F];
			if (i != 5) *RxBuf++ = '-';
		}
		rt = 17;
		break;
	case VAR_DEFAULT_MAC:				/* 默认的MAC地址 */
//		config_GetInitMac((u8 *)buf);
		for (i=0; i<6; i++)
		{
			*RxBuf++ = HexTab[buf[i] >> 4];
			*RxBuf++ = HexTab[buf[i]&0x0F];
			if (i != 5) *RxBuf++ = '-';
		}
		rt = 17;
		break;
	case VAR_HOST_NAME:					/* 当前的网络名称 */
//		rt = strlen(conf.NetBIOSName);
//		strncpy(RxBuf, conf.NetBIOSName, rt);
		break;
	case VAR_DHCP_EN:					/* 是否启用DHCP自动获取IP地址 */
//		if (conf.DHCP_en == 0)
			*RxBuf = '0';
//		else
//			*RxBuf = '1';
		rt = 1;
		break;
	case VAR_IP_ADDR:					/* 本机IP地址 */
//		ip.s_addr = sys_ip_addr->addr;
		pstr = inet_ntoa(ip);
		rt = strlen(pstr);
		strncpy(RxBuf, pstr, rt);
		break;
	case VAR_SUBNET_MASK:				/* 子网掩码 */
//		ip.s_addr = sys_netmask->addr;
		pstr = inet_ntoa(ip);
		rt = strlen(pstr);
		strncpy(RxBuf, pstr, rt);
		break;
	case VAR_GATWAY_ADDR:				/* 网关地址 */
//		ip.s_addr = sys_gw->addr;
		pstr = inet_ntoa(ip);
		rt = strlen(pstr);
		strncpy(RxBuf, pstr, rt);
		break;
	case VAR_DNS_SERVER:				/* DNS域名服务器地址 */
#if LWIP_DNS
#if LWIP_DHCP
		if (conf.DHCP_en != 0)
		{
			ip.s_addr = (*pSys_dhcp)->offered_dns_addr[0].addr;
		}
		else
#endif // LWIP_DHCP
		{
			ip.s_addr = htonl(conf.DnsAddr);
		}
		pstr = inet_ntoa(ip);
		rt = strlen(pstr);
		strncpy(RxBuf, pstr, rt);
#else
		rt = 0;
#endif
		break;
	case VAR_DNS_SERVER_BAK:
#if LWIP_DNS
#if LWIP_DHCP
		if (conf.DHCP_en != 0)
		{
			ip.s_addr = (*pSys_dhcp)->offered_dns_addr[1].addr;
		}
		else
#endif // LWIP_DHCP
		{
			ip.s_addr = htonl(conf.DnsAddr2);
		}
		pstr = inet_ntoa(ip);
		rt = strlen(pstr);
		strncpy(RxBuf, pstr, rt);
#else
		rt = 0;
#endif
		break;

	case VAR_SERVER_ADDR:				/* 服务器的IP地址 */
//		ip.s_addr = htonl(conf.ServerIp);
		pstr = inet_ntoa(ip);
		rt = strlen(pstr);
		strncpy(RxBuf, pstr, rt);
		break;
	case VAR_SERVER_PORT:				/* 服务器的端口号 */
//		sprintf(buf, "%d", conf.ServerPort);
		rt = strlen(buf);
		strncpy(RxBuf, buf, rt);
		break;
	case VAR_SEND_OFFSET:
//		sprintf(buf, "%d", conf.tcpSendTimeout/1000);
		rt = strlen(buf);
		strncpy(RxBuf, buf, rt);
		break;
	case VAR_SNTP_UPDATE:
//		sprintf(buf, "%d", conf.sntpUpdateDelay/1000);
		rt = strlen(buf);
		strncpy(RxBuf, buf, rt);
		break;
		
	case VAR_VOICE_RATE:				/* 语音码率编号 */
//		sprintf(buf, "%d", conf.VoiceRate);
//		rt = strlen(buf);
//		strncpy(RxBuf, buf, rt);
		break;
	case VAR_MODEM_SPEED:				/* Modem速率编号编号 */
//		sprintf(buf, "%d", conf.ModemRate);
//		rt = strlen(buf);
//		strncpy(RxBuf, buf, rt);
		break;

//	case VAR_VOICE_RATE_CNT:			/* 可供选择的语音码率的个数 */
//		sprintf(RxBuf, "%d", AMBE_VoiceRateMax);
//		rt = strlen(RxBuf);
//		break;
//	case VAR_VOICE_RATE_1:				/* 第1个可共选择的语音码率 */
//	case VAR_VOICE_RATE_2:
//	case VAR_VOICE_RATE_3:
//	case VAR_VOICE_RATE_4:
//	case VAR_VOICE_RATE_5:
//	case VAR_VOICE_RATE_6:
//	case VAR_VOICE_RATE_7:
//	case VAR_VOICE_RATE_8:
//	case VAR_VOICE_RATE_9:
//	case VAR_VOICE_RATE_10:
//	case VAR_VOICE_RATE_11:
//		temp = VarID - VAR_VOICE_RATE_1;
//		strcpy(RxBuf, AMBE_VoiceRateArray[temp].name);
//		rt = strlen(RxBuf);
//		break;

	default:
		rt = 0;
		break;
	}
	return rt;
}

/*
 * Function:    char Hexatob(short hex)
 *
 * Summary:     Convert a hex date to a single hex byte.
 *
 * Input:       hex - The hex data with high byte and low byte
 *
 * Output:      A hex byte, if the input is wrong, it returns 0x20(blankspace)
 *
 * Overview:    convert the URL encoding string to single hex byte, The following
 *              conversions are made: 0x3231"21" to 0x21, 0x4138"A8" to 0xA8, ...
 */
char Hexatob(short hex)
{
	char a, b;

	a = (hex & 0xFF00) >> 8;
	b = (hex & 0x00FF);
	if ((a >= 'a') && (a <= 'f'))
	{
		a -= 'a'-0x0A;
	}
	else if ((a >= 'A') && (a <= 'F'))
	{
		a -= 'A'-0x0A;
	}
	else if ((a >= '0') && (a <= '9'))
	{
		a -= '0';
	}
	else
	{
		a = 2;
	}
	if ((b >= 'a') && (b <= 'f'))
	{
		b -= 'a'-0x0A;
	}
	else if ((b >= 'A') && (b <= 'F'))
	{
		b -= 'A'-0x0A;
	}
	else if ((b >= '0') && (b <= '9'))
	{
		b -= '0';
	}
	else
	{
		b = 0;
	}
	a <<= 4;
	a |= b;

	return a;
}

/*
 * Function:    char* HttpUrlDecode(char* cData)
 *
 * Summary:     Parses a string from URL encoding to plain-text.
 *
 * Input:       cData - The string which is to be decoded in place.
 *
 * Output:      A pointer to the last null terminator in data, which is also the
 *              first free byte for new data.
 *
 * Overview:    Parses a string from URL encoding to plain-text. The following
 *              conversions are made: '+' to ' ' and "%xx" to a single hex byte.
 *
 *              After completion, the data has been decoded and a null terminator
 *              signifies the end of a name or value.
 */
char* HttpUrlDecode(char* cData)
{
	char *pRead, *pWrite;
	char c;
	int wLen;
	short hex;
	
	// Determine length of input
	wLen = strlen(cData);
	
	// Read all characters in the string
	for (pRead = pWrite = cData; wLen != 0; )
	{
		c = *pRead++;
		wLen--;
		
		if (c == '=' || c == '&')
			*pWrite++ = '\0';
		else if (c == '+')
			*pWrite++ = ' ';
		else if (c == '%')
		{
			if (wLen < 2)
			{
				wLen = 0;
			}
			else
			{
				hex   = *pRead++;
				hex <<= 8;
				hex  |= *pRead++;
				wLen -= 2;
				*pWrite++ = Hexatob(hex);
			}
		}
		else
		{
			*pWrite++ = c;
		}
	}
	
	*pWrite = '\0';
	
	return pWrite;
}

/*
 * Function:    HTTP_COMMAND HttpCmdParse(BYTE *string, BYTE **argv, BYTE *argc, BYTE *type)
 *
 * Input:       string  - HTTP Command String
 *              argv    - List of string pointer to hold HTTP arguments.
 *              argc    - Pointer to hold total number of arguments
 *                        in this command string
 *              type    - Pointer to hold type of file received.
 *
 * Output:      HTTP FSM and connections are initialized
 *
 * Note:        This function parses URL that may or may not contain arguments.
 *              e.g. "GET /detail.htm?name=about&page=12 HTTP/1.1\r\nAccept: image/gif,..."
 *                  would be returned as below:
 *                      argv[0] => /detail.htm
 *                      argv[1] => name
 *                      argv[2] => about
 *                      argv[3] => page
 *                      argv[4] => 12
 *                      argc = 5
 *
 *              This parses does not "de-escape" URL string.
 */
HTTP_COMMAND HttpCmdParse(char *string, char **argv, char *argc, char *type)
{
	int i, lenA, lenB;
    BYTE c;
	char *pStr;
    char *ext;
    char *fileType;
    HTTP_COMMAND cmd;

    // Only "GET " / "POST " is supported
    if (strncmp(string, HTTP_GET_STRING, HTTP_GET_STRING_LEN) == 0)
    {
        string += HTTP_GET_STRING_LEN;
        cmd = HTTP_GET;
    }
    else if (strncmp(string, HTTP_POST_STRING, HTTP_POST_STRING_LEN) == 0)
    {
		string += HTTP_POST_STRING_LEN;
		cmd = HTTP_POST;
	}
    else
    {
        return HTTP_NOT_SUPPORTED;
    }

    // Skip white spaces.
    while (*string == ' ') string++;

	// Find the end of filename
	for (lenA=0, pStr=string; (*pStr!=' ') && (*pStr!='\0'); lenA++, pStr++) ;
	for (lenB=0, pStr=string; (*pStr!='?') && (lenB < lenA); lenB++, pStr++) ;

	// Get the filename
	i = 0;
	string[lenB] = '\0';

	// 串口输出Http请求字符串
#ifdef HTTP_DEBUG
	HTTP_DEBUG1(string);
#endif
	pStr = HttpUrlDecode(string);		//
	pStr -= 1;
	argv[i++] = string;

	if (*pStr == '/')					// no filename specified, default index.htm file
	{
		*type = HTTP_HTM;
	}
	else
	{
		// Find the extension in the filename
		for (ext=pStr; ext!=string; ext --)
		{
			if (*ext == '.')
				break;
		}
	
		// Compare the known extensions to determine Content-Type
		*type = HTTP_UNKNOWN;
		if (ext != string)
		{
			ext ++;
			for (c=HTTP_TXT; c<HTTP_UNKNOWN; c++)
			{
				fileType = (char *)httpFileExtensions[c];
				if (strcmp(ext, fileType) == 0)
				{
					*type = c;
					break;
				}
			}
		}
	}

	string += lenB + 1;					// points to the argv or the end
	if (lenB < lenA)
	{
		argv[i++] = string++;
		if (i < MAX_HTTP_ARGS)			// Do not accept any more arguments than we haved designed to.
		{
			c = *string;
			while ((c != ' ') &&  (c != '\0') && (c != '\r') && (c != '\n'))
			{
				if ((c == '?') || (c == '&') || (c == '='))
				{
					*string++ = '\0';
					HttpUrlDecode(argv[i-1]);
					argv[i++] = string;
					if (i >= MAX_HTTP_ARGS)		// Do not accept any more arguments than we haved designed to.
						break;
				}
				string ++;
				c = *string;
			}
			*string = '\0';
			HttpUrlDecode(argv[i-1]);
		}
	}
    *string = '\0';

    *argc = i;

    return cmd;
}

/*FIL* FileOpen(const char *path, BYTE mode)
{
	FIL* rt = NULL;
	FRESULT temp;

	// Allocate space for the FATFS.  Where the memory comes from depends on
	//the implementation of the port malloc function.
	rt = (FIL*)pvPortMalloc(sizeof(FATFS));
	if (rt != NULL)
	{
		temp = f_open(rt, path, mode);
		if (temp != FR_OK)
		{
//			PrintS("\nFileOpen: f_open() failed: ");	Print8(temp);
//			PrintS("\n");	PrintS(path);
			vPortFree((void *)rt);
			rt = NULL;
		}
	}
	else
	{
//		PrintS("\nFileOpen: Failed to malloc memory!");
	}

	return rt;
}*/

/* ------------------------------------------------------------------------------- */
// Find and open a file, if filename is not preset, it opens the default /index.htm file.
// if the specified file is not exist, it opens the /404.htm
FIL* Http_fOpen(char* filename)
{
	FIL* fp;
	char path[128];		// the maximal path name length is 127 bytes
	int len;

    // If string is empty, do not attempt to find it in FAT.
    if (*filename == '\0') return (FIL*)NULL;
	len = strlen(filename);

	// Check if filename length, not exceed the buffer size
	if ((len + HTTP_WEBPAGE_DIR_LEN) >= 128) goto FileNotFound;

	// Add the directory path before the actual file name
	strcpy(path, (char *)HTTP_WEBPAGE_DIR);
	strcpy(&path[HTTP_WEBPAGE_DIR_LEN], filename);
	if (filename[len-1] == '/')
	{
		// the total length of the pre path and sub path
		len = strlen(path);
		if ((len + HTTP_DEFAULT_HTM_LEN) >= 128) goto FileNotFound;
		strcpy(&path[len], HTTP_DEFAULT_HTM);
	}
//PrintS("\nO: ");
//PrintS((const char *)(&path[HTTP_WEBPAGE_DIR_LEN]));
	fp = FileOpen(path, FA_READ);
	if (fp != NULL)
	{
	//	FileSeek(fp, 0, SEEK_SET);
		f_lseek(fp, 0);
		return fp;
	}

FileNotFound:
//	PrintS(" - File not found!");
	// open the default 404 file, overwirte the previous path name
	path[HTTP_WEBPAGE_DIR_LEN] = '/';
	strcpy(&path[HTTP_WEBPAGE_DIR_LEN+1], (char *)HTTP_DEFAULT_404);
	fp = FileOpen(path, FA_READ);
	if (fp != NULL)
	{
	//	FileSeek(fp, 0, SEEK_SET);
		f_lseek(fp, 0);
	}
	return fp;
}

// Close a file by handle
void Http_fClose(FIL* handle)
{
	if (handle != NULL)
	{
		vPortFree((void *)handle);
	}
}



/*----------------------------------------------------------------------------+
| Interrupt Service Routines                                                  |
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| End of source file                                                          |
+----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
