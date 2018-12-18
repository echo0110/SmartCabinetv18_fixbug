#include <string.h>
#include "md5.h" 
#include "ff.h"			/* Declarations of FatFs API */
#include "diskio.h"		/* Declarations of disk I/O functions */
#include "exfuns.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "dm9000.h"
#include "stdio.h"
#include <stdlib.h>
#include "httpd.h"
#include "lwip_comm.h" 

char temp2_md5[50];
//char temp_md5[200];

char temp_md5[1600];
char digest_temp2[50];

char sub_bin_size[20];//get  bin size 
char *test_md5_bin="1:post.bin";

FIL *file_md5_boot;

void* IAPTask_Handler;//





extern TaskHandle_t tcpTask_Handler;




unsigned char PADDING[]={0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  

void MD5Init(MD5_CTX *context)  
{  
    context->count[0] = 0;  
    context->count[1] = 0;  
    context->state[0] = 0x67452301;  
    context->state[1] = 0xEFCDAB89;  
    context->state[2] = 0x98BADCFE;  
    context->state[3] = 0x10325476;  
}  
void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen)  
{  
    unsigned int i = 0,index = 0,partlen = 0;  
    index = (context->count[0] >> 3) & 0x3F;  
    partlen = 64 - index;  
    context->count[0] += inputlen << 3;  
    if(context->count[0] < (inputlen << 3))  
        context->count[1]++;  
    context->count[1] += inputlen >> 29;  

    if(inputlen >= partlen)  
    {  
        memcpy(&context->buffer[index],input,partlen);  
        MD5Transform(context->state,context->buffer);  
        for(i = partlen;i+64 <= inputlen;i+=64)  
            MD5Transform(context->state,&input[i]);  
        index = 0;          
    }    
    else  
    {  
        i = 0;  
    }  
    memcpy(&context->buffer[index],&input[i],inputlen-i);  
}  
void MD5Final(MD5_CTX *context,unsigned char digest[16])  
{  
    unsigned int index = 0,padlen = 0;  
    unsigned char bits[8];  
    index = (context->count[0] >> 3) & 0x3F;  
    padlen = (index < 56)?(56-index):(120-index);  
    MD5Encode(bits,context->count,8);  
    MD5Update(context,PADDING,padlen);  
    MD5Update(context,bits,8);  
    MD5Encode(digest,context->state,16);  
}  
void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len)  
{  
    unsigned int i = 0,j = 0;  
    while(j < len)  
    {  
        output[j] = input[i] & 0xFF;    
        output[j+1] = (input[i] >> 8) & 0xFF;  
        output[j+2] = (input[i] >> 16) & 0xFF;  
        output[j+3] = (input[i] >> 24) & 0xFF;  
        i++;  
        j+=4;  
    }  
}  
void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len)  
{  
    unsigned int i = 0,j = 0;  
    while(j < len)  
    {  
        output[i] = (input[j]) |  
            (input[j+1] << 8) |  
            (input[j+2] << 16) |  
            (input[j+3] << 24);  
        i++;  
        j+=4;   
    }  
}  
void MD5Transform(unsigned int state[4],unsigned char block[64])  
{  
    unsigned int a = state[0];  
    unsigned int b = state[1];  
    unsigned int c = state[2];  
    unsigned int d = state[3];  
    unsigned int x[64];  
    MD5Decode(x,block,64);  
    FF(a, b, c, d, x[ 0], 7, 0xd76aa478);   
    FF(d, a, b, c, x[ 1], 12, 0xe8c7b756);   
    FF(c, d, a, b, x[ 2], 17, 0x242070db);   
    FF(b, c, d, a, x[ 3], 22, 0xc1bdceee);   
    FF(a, b, c, d, x[ 4], 7, 0xf57c0faf);   
    FF(d, a, b, c, x[ 5], 12, 0x4787c62a);   
    FF(c, d, a, b, x[ 6], 17, 0xa8304613);   
    FF(b, c, d, a, x[ 7], 22, 0xfd469501);   
    FF(a, b, c, d, x[ 8], 7, 0x698098d8);   
    FF(d, a, b, c, x[ 9], 12, 0x8b44f7af);   
    FF(c, d, a, b, x[10], 17, 0xffff5bb1);   
    FF(b, c, d, a, x[11], 22, 0x895cd7be);   
    FF(a, b, c, d, x[12], 7, 0x6b901122);   
    FF(d, a, b, c, x[13], 12, 0xfd987193);   
    FF(c, d, a, b, x[14], 17, 0xa679438e);   
    FF(b, c, d, a, x[15], 22, 0x49b40821);   


    GG(a, b, c, d, x[ 1], 5, 0xf61e2562);   
    GG(d, a, b, c, x[ 6], 9, 0xc040b340);   
    GG(c, d, a, b, x[11], 14, 0x265e5a51);   
    GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);   
    GG(a, b, c, d, x[ 5], 5, 0xd62f105d);   
    GG(d, a, b, c, x[10], 9,  0x2441453);   
    GG(c, d, a, b, x[15], 14, 0xd8a1e681);   
    GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);   
    GG(a, b, c, d, x[ 9], 5, 0x21e1cde6);   
    GG(d, a, b, c, x[14], 9, 0xc33707d6);   
    GG(c, d, a, b, x[ 3], 14, 0xf4d50d87);   
    GG(b, c, d, a, x[ 8], 20, 0x455a14ed);   
    GG(a, b, c, d, x[13], 5, 0xa9e3e905);   
    GG(d, a, b, c, x[ 2], 9, 0xfcefa3f8);   
    GG(c, d, a, b, x[ 7], 14, 0x676f02d9);   
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);   


    HH(a, b, c, d, x[ 5], 4, 0xfffa3942);   
    HH(d, a, b, c, x[ 8], 11, 0x8771f681);   
    HH(c, d, a, b, x[11], 16, 0x6d9d6122);   
    HH(b, c, d, a, x[14], 23, 0xfde5380c);   
    HH(a, b, c, d, x[ 1], 4, 0xa4beea44);   
    HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9);   
    HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60);   
    HH(b, c, d, a, x[10], 23, 0xbebfbc70);   
    HH(a, b, c, d, x[13], 4, 0x289b7ec6);   
    HH(d, a, b, c, x[ 0], 11, 0xeaa127fa);   
    HH(c, d, a, b, x[ 3], 16, 0xd4ef3085);   
    HH(b, c, d, a, x[ 6], 23,  0x4881d05);   
    HH(a, b, c, d, x[ 9], 4, 0xd9d4d039);   
    HH(d, a, b, c, x[12], 11, 0xe6db99e5);   
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8);   
    HH(b, c, d, a, x[ 2], 23, 0xc4ac5665);   


    II(a, b, c, d, x[ 0], 6, 0xf4292244);   
    II(d, a, b, c, x[ 7], 10, 0x432aff97);   
    II(c, d, a, b, x[14], 15, 0xab9423a7);   
    II(b, c, d, a, x[ 5], 21, 0xfc93a039);   
    II(a, b, c, d, x[12], 6, 0x655b59c3);   
    II(d, a, b, c, x[ 3], 10, 0x8f0ccc92);   
    II(c, d, a, b, x[10], 15, 0xffeff47d);   
    II(b, c, d, a, x[ 1], 21, 0x85845dd1);   
    II(a, b, c, d, x[ 8], 6, 0x6fa87e4f);   
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0);   
    II(c, d, a, b, x[ 6], 15, 0xa3014314);   
    II(b, c, d, a, x[13], 21, 0x4e0811a1);   
    II(a, b, c, d, x[ 4], 6, 0xf7537e82);   
    II(d, a, b, c, x[11], 10, 0xbd3af235);   
    II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb);   
    II(b, c, d, a, x[ 9], 21, 0xeb86d391);   
    state[0] += a;  
    state[1] += b;  
    state[2] += c;  
    state[3] += d;  
}


/*
 * 函数名：void  md5_test(void)
 * 描述  ：MD5测试函数  计算MD5值
 * 输入  ：无
 * 输出  ：无	
 */
void md5_test(unsigned char digest[])
{
	u8 res,i,j=0;
	u16 count,k,m,n;
	u16 bin_count,bin_remain;
	u8 ret = 0;
	u8 Flag_test;
	char buf_test[3][520];
	
	char digest_temp[50];
	MD5_CTX context;
	MD5Init(&context);	
	
	if(((atoi(sub_bin_size))%512)==0)
	bin_count=(atoi(sub_bin_size))>>9;	
	
	if(((atoi(sub_bin_size))%512)!=0)
	{
	 Flag_test=1;
	 bin_count=((atoi(sub_bin_size))>>9)+1;
	 bin_remain=(atoi(sub_bin_size))%512;	
	}
		
	/* 计算文件MD5 */

  for(count=0;count<bin_count;count++)
  {			
	 test_read(file_iap,buf_test[0],count);
	 if(Flag_test==1)  //不能被整除
	 {
		 if(count==(bin_count-1))
		 {
	    MD5Update(&context,(u8*)buf_test[0], bin_remain);
		 }
		 else  
		 {
		 MD5Update(&context,(u8*)buf_test[0], 512);
		 }		
	 }
	 else //能被整除
	 {
	  MD5Update(&context,(u8*)buf_test[0], 512);	 
	 }
   memset(buf_test,0,520);		
	} 
	f_close(file_iap);	
	vPortFree(file_iap);
	

	MD5Final(&context, digest);		
	/* 打印MD5值 */
	for(i=0;i<16;i++)
	{		
	 sprintf(&digest_temp[i+j],"%02x",digest[i]);			
	 j++;			
	}
	snprintf(digest_temp2,33,"%s",digest_temp);
	printf("digest_temp2=%s\n",digest_temp2);
}



/*
 * 函数名：void md5_json(char* http_post_payload,char http_len)
 * 描述  从接收文件 尾部取出 MD5值
 * 输入  ：无
 * 输出  ：无	  找34个数据 然后snprintf提取 前面32个数据
 */
void md5_json(char* http_post_payload,u16 http_len)
{
  int len,j=0,i;
	u8 digest_test[16]={0};
	char temp3_md5[50];
	printf("temp_md5=%s\n",temp_md5);
  md5_test(digest_test);
	if(strcmp((char*)temp_md5,(char*)digest_temp2)==0)//MD5 verification is correct
	{ 			
	 printf("hardware reset\n");
	 md5_write_to_flash();
	 NVIC_SystemReset();//
	}
	else
	{		
		printf("MD5 verification is wrong\n");
		NVIC_SystemReset();//
	}
}


/*
 * 函数名void substring( char *s, char ch1, char ch2, char *substr )
 * 描述  ：找两个字符之间的字符串
 * 输入  ：无
 * 输出  ：无	
 */
void substring( char *s, char ch1, char ch2, char *substr )
{
    while( *s && *s++!=ch1 ) ;
    while( *s && *s!=ch2 ) *substr++=*s++ ;
    *substr='\0';
}


/*
 * 函数名：void md5_write_to_flash(char* buff)
 * 描述  nd5写进flash   boot读取使用
 * 输入  ：无
 * 输出  ：无	  
 */
void md5_write_to_flash(void)
{
	u8 res,i,n4;		
	char buff[20]="md5updateflag";
	res=f_lseek(file_md5_boot, file_md5_boot->fsize);			
	n4= f_write(file_md5_boot, buff,strlen(buff),&br);
	vPortFree(file_md5_boot);
	f_close(file_md5_boot);		
}


/*
 * 函数名u8 f_truncate(void)
 * 描述  ：截断文件
 * 输入  ：无
 * 输出  ：无	
 */
void file_truncate(int len2)
{ 
  u8 res,i,n4,res6;			 
	res6=f_lseek(file_iap, len2);//将文件在当前位置截断			
	n4= f_truncate(file_iap);
}





/*
 * 函数名：void test_read(FIL *file_md5,u8 *data,u16 i)
 * 描述  ：
 * 输入  ：无
 * 输出  ：无	
 */ 
void test_read(FIL *file_iap,char buff[],u16 i)
{
	u8 ret,res;
	u16 bin_count,bin_remain;
	bin_count=(atoi(sub_bin_size))/512;	  //要再判断下能不能被512整除  现在做的是不能被整除的情况
	bin_remain=(atoi(sub_bin_size))%512;	
	if(i==bin_count)
	{
		f_read(file_iap, buff, bin_remain, &br);
	}
	else
	{
		res=f_lseek(file_iap, 512*i);
		{
		 ret=f_read(file_iap, buff, 512, &br); //res=f_lseek(file_iap, file_iap->fsize);
		}	
	}	
}



void StringCopy(char *dst,const char *src)//
{
	while (*src!='\0')
	{
		*(dst++)=*(src++);
	}
}



char *my_memcpy(void *dest, void *src, int num)
{
	char *pdest;
	char *psrc;
	if((dest == NULL) ||(src == NULL))
	{
		printf("parameter error!\n");
		return 0;
	}
	
	pdest = dest;
	psrc = src;
	while(num --)
	{		
		*pdest++=*psrc++;		
	}
	return dest;
}


















































