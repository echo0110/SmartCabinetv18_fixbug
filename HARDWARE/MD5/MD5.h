#ifndef MD5_H  
#define MD5_H 

#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ff.h"			/* Declarations of FatFs API */
//#include "diskio.h"		/* Declarations of disk I/O functions */
//#include "exfuns.h"

//extern u8 *data_test[200];


typedef struct  
{  
    unsigned int count[2];  
    unsigned int state[4];  
    unsigned char buffer[64];     
}MD5_CTX;  


#define F(x,y,z) ((x & y) | (~x & z))  
#define G(x,y,z) ((x & z) | (y & ~z))  
#define H(x,y,z) (x^y^z)  
#define I(x,y,z) (y ^ (x | ~z))  
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))  
#define FF(a,b,c,d,x,s,ac) \
{ \
    a += F(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}  
#define GG(a,b,c,d,x,s,ac) \
{ \
    a += G(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}  
#define HH(a,b,c,d,x,s,ac) \
{ \
    a += H(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}  
#define II(a,b,c,d,x,s,ac) \
{ \
    a += I(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}                                              
void MD5Init(MD5_CTX *context);  
void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen);  
void MD5Final(MD5_CTX *context,unsigned char digest[16]);  
void MD5Transform(unsigned int state[4],unsigned char block[64]);  
void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len);  
void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len); 

extern void md5_test(unsigned char digest[]);
extern int  strindex(char s[], char t[]);
extern void md5_json(char* http_post_payload,u16 http_len);
extern char temp2_md5[50];
//extern char temp_md5[200];
extern char temp_md5[1600];
extern char sub_bin_size[20];//get  bin size 

extern void substring( char *s, char ch1, char ch2, char *substr );
extern void md5_write_to_flash(void);
extern void file_truncate(int len2);

extern void* IAPTask_Handler;//

extern void test_read(FIL *file_md5,char *data,u16 i);

extern void md5_test_bug(unsigned char digest[]);


extern void StringCopy(char *dst,const char *src);//×Ö·û´®¸´ÖÆ²âÊÔ

extern char *my_memcpy(void *dest, void *src, int num);

extern FIL *file_md5_boot;

#endif

























