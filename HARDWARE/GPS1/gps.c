
#include "gps.h"
#include "string.h"
#include "print.h"

const u32 BAUD_id[9]={4800,9600,19200,38400,57600,115200,230400,460800,921600};

// 返回值:	0~0XFE,代表逗号所在位置的偏移.
//			0XFF,代表不存在第cx个逗号							  
u8 NMEA_Comma_Pos(u8 *buf,u8 cx)
{
	u8 *p=buf;
	while(cx)
	{
		if(*buf=='*'||*buf<' '||*buf>'z')return 0XFF;	// 遇到'*'或者非法字符,则不存在第cx个逗号
		if(*buf==',')cx--;
		buf++;
	}
	return buf-p;
}

// m^n函数
// 返回值:m^n次方.
u32 NMEA_Pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)result*=m;
	return result;
}

// str转换为数字,以','或者'*'结束
// buf:数字存储区
// dx:小数点位数,返回给调用函数
// 返回值:转换后的数值
int NMEA_Str2num(u8 *buf,u8*dx)
{
	u8 *p=buf;
	u32 ires=0,fres=0;
	u8 ilen=0,flen=0,i;
	u8 mask=0;
	int res;
	while(1)							// 得到整数和小数的长度
	{
		if(*p=='-'){mask|=0X02;p++;}	// 是负数
		if(*p==','||(*p=='*'))break;	// 遇到结束了
		if(*p=='.'){mask|=0X01;p++;}	// 遇到小数点了
		else if(*p>'9'||(*p<'0'))		// 有非法字符
		{
			ilen = 0;
			flen = 0;
			break;
		}
		if(mask&0X01)flen++;
		else ilen++;
		p++;
	}
	if(mask&0X02)buf++;		// 去掉负号
	for(i=0; i<ilen; i++)	// 得到整数部分数据
	{
		ires += NMEA_Pow(10,ilen-1-i)*(buf[i]-'0');
	}
	if(flen > 5)flen = 5;	// 最多取5位小数
	*dx = flen;	 			// 小数点位数
	for(i=0; i<flen; i++)	// 得到小数部分数据
	{
		fres += NMEA_Pow(10, flen-1-i)*(buf[ilen+1+i]-'0');
	}
	res = ires*NMEA_Pow(10, flen) + fres;
	if(mask&0X02)res = - res;
	return res;
}	  							 

// 分析GNRMC信息
// gpsx:nmea信息结构体
// buf:接收到的GPS数据缓冲区首地址
int NMEA_GNRMC_Analysis(nmea_msg *gpsx, u8 *buf)
{
	u8 *p1, dx, posx;
	u32 temp;
	float rs;
	
	p1 = (u8*)strstr((const char *)buf, "$GNRMC");
	if(p1 == NULL)
	{
		p1 = (u8*)strstr((const char *)buf, "$GPRMC");
		if(p1 == NULL)return -1;
	}
	
	posx = NMEA_Comma_Pos(p1, 1);							// 时分秒
	if(posx != 0XFF)
	{
		temp = NMEA_Str2num(p1+posx,&dx)/NMEA_Pow(10,dx);
		gpsx->utc.hour = temp/10000;
		gpsx->utc.min = (temp/100)%100;
		gpsx->utc.sec = temp%100;
	}
	else return -1;
	
	// 增加经纬度是否有效判断
	posx = NMEA_Comma_Pos(p1, 2);							// V无效定位
	if(posx != 0XFF)
	{
		if(*(p1+posx) == 'V')
		{
			Printf("无效经纬度");
			return -1;
		}
		else Printf("已获取经纬度");
	}
	
	posx = NMEA_Comma_Pos(p1, 3);							// 纬度
	if(posx != 0XFF)
	{
		temp = NMEA_Str2num(p1+posx, &dx);
		gpsx->latitude = temp/NMEA_Pow(10, dx+2);
		rs = temp%NMEA_Pow(10, dx+2);
		gpsx->latitude = gpsx->latitude*NMEA_Pow(10, 5)+(rs*NMEA_Pow(10, 5-dx))/60;
	}
	else return -1;
	
	posx = NMEA_Comma_Pos(p1, 4);							// 南纬/北纬 
	if(posx != 0XFF)gpsx->nshemi = *(p1+posx);
	else return -1;
	
 	posx = NMEA_Comma_Pos(p1, 5);							// 经度
	if(posx != 0XFF)
	{
		temp = NMEA_Str2num(p1+posx, &dx);
		gpsx->longitude = temp/NMEA_Pow(10, dx+2);
		rs = temp%NMEA_Pow(10, dx+2);
		gpsx->longitude = gpsx->longitude*NMEA_Pow(10, 5)+(rs*NMEA_Pow(10, 5-dx))/60;
	}
	else return -1;
	
	posx = NMEA_Comma_Pos(p1, 6);							//东经/西经
	if(posx != 0XFF)gpsx->ewhemi = *(p1+posx);
	else return -1;
	
	posx = NMEA_Comma_Pos(p1, 9);							//日月年
	if(posx != 0XFF)
	{
		temp = NMEA_Str2num(p1+posx, &dx);
		gpsx->utc.date = temp/10000;
		gpsx->utc.month = (temp/100)%100;
		gpsx->utc.year = 2000+temp%100;
	}
	else return -1;
	
	return 0;
}
