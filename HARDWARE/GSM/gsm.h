#ifndef __GSM_H__
#define __GSM_H__	 
#include "sys.h"

#define SIM_OK				0
#define SIM_COMMUNTION_ERR	1//0xff
#define SIM_CPIN_ERR		2//0xfe
#define SIM_CREG_FAIL		3//0xfd
#define SIM_MAKE_CALL_ERR	0Xfc
#define SIM_ATA_ERR			0xfb

#define SIM_CMGF_ERR		0xfa
#define SIM_CSCS_ERR		0xf9
#define SIM_CSCA_ERR		0xf8
#define SIM_CSMP_ERR		0Xf7
#define SIM_CMGS_ERR		0xf6
#define SIM_CMGS_SEND_FAIL	0xf5

#define SIM_CNMI_ERR		0xf4
#define SIM_PKEY PCout(9)	// gsm模块开关机引脚定义

extern u8 Flag_Rec_Message;	// 收到短信标示
extern u8 SIM_CON_OK;

extern u8 SIM800C_CSQ[3];
u8 gsm_dect(void);
void gsm_reset(void);
u8 SIM800C_CONNECT_SERVER(u8 *servip,u8 *port);
u8 SIM800C_GPRS_SEND_DATA(u8 *temp_data);
u8 sim800c_send_cmd(u8 *cmd,u8 *ack,u16 waittime);

#endif
