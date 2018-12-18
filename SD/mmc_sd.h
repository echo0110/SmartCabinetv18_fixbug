#ifndef _MMC_SD_H_
#define _MMC_SD_H_		 
#include "stm32f10x.h"






								  	 
// SD卡类型定义  
#define SD_TYPE_ERR     0X00
#define SD_TYPE_MMC     0X01
#define SD_TYPE_V1      0X02
#define SD_TYPE_V2      0X04
#define SD_TYPE_V2HC    0X06	   
// SD卡指令表  	   
#define CMD0    0       //卡复位
#define CMD1    1
#define CMD8    8       //命令8 ，SEND_IF_COND
#define CMD9    9       //命令9 ，读CSD数据
#define CMD10   10      //命令10，读CID数据
#define CMD12   12      //命令12，停止数据传输
#define CMD16   16      //命令16，设置SectorSize 应返回0x00
#define CMD17   17      //命令17，读sector
#define CMD18   18      //命令18，读Multi sector
#define CMD23   23      //命令23，设置多sector写入前预先擦除N个block
#define CMD24   24      //命令24，写sector
#define CMD25   25      //命令25，写Multi sector
#define CMD41   41      //命令41，应返回0x00
#define CMD55   55      //命令55，应返回0x01
#define CMD58   58      //命令58，读OCR信息
#define CMD59   59      //命令59，使能/禁止CRC，应返回0x00



#define CMD13   13      //命令13，应返回0x01
//数据写入回应字意义
#define MSD_DATA_OK                0x05
#define MSD_DATA_CRC_ERROR         0x0B
#define MSD_DATA_WRITE_ERROR       0x0D
#define MSD_DATA_OTHER_ERROR       0xFF
//SD卡回应标记字
#define MSD_RESPONSE_NO_ERROR      0x00
#define MSD_IN_IDLE_STATE          0x01
#define MSD_ERASE_RESET            0x02
#define MSD_ILLEGAL_COMMAND        0x04
#define MSD_COM_CRC_ERROR          0x08
#define MSD_ERASE_SEQUENCE_ERROR   0x10
#define MSD_ADDRESS_ERROR          0x20
#define MSD_PARAMETER_ERROR        0x40
#define MSD_RESPONSE_FAILURE       0xFF
 							   						 	 
//这部分应根据具体的连线来修改!
//SD卡片选引脚					    	  
#define SD_CS_SET GPIO_SetBits(GPIOA,GPIO_Pin_4)
#define SD_CS_CLR GPIO_ResetBits(GPIOA,GPIO_Pin_4)

extern u8  SD_Type;//SD卡的类型

extern u8 sd_init;//判断初始化有没有SD卡
//函数申明区 
u8 SD_SPI_ReadWriteByte(u8 data);
void SD_SPI_SpeedLow(void);
void SD_SPI_SpeedHigh(void);
u8 SD_WaitReady(void);							//等待SD卡准备
extern u8 SD_WaitReady_init(void);//给初始化的时候使用
u8 SD_GetResponse(u8 Response);					//获得相应
u8 SD_Initialize(void);							//初始化
u8 SD_ReadDisk(u8*buf,u32 sector,u8 cnt);		//读块
u8 SD_WriteDisk(u8*buf,u32 sector,u8 cnt);		//写块
u32 SD_GetSectorCount(void);   					//读扇区数
u8 SD_GetCID(u8 *cid_data);                     //读SD卡CID
u8 SD_GetCSD(u8 *csd_data);                     //读SD卡CSD

extern u8 SD_Initialize(void);

extern int SD_Rdy_Status;				// SD卡的状态

extern u8 SD_SendCmd(u8 cmd, u32 arg, u8 crc);

extern void* spi_receive_command;;
extern void* spi_send_command;

extern void SPINivcConfiguration(void);

extern u8 SD_test_Init(void);

extern u8 sd_flag;

extern u8 test_Select(void);

 
#endif




