//
// --------------------------------------------------------------------------
// Shell.h
// --------------------------------------------------------------------------

#ifndef __COMMAND_H__
#define __COMMAND_H__

/*----------------------------------------------------------------------------+
| Include files                                                               |
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| Type Definition & Macro                                                     |
+----------------------------------------------------------------------------*/
#define OS_TICKS_PER_SEC	configTICK_RATE_HZ


struct t_CMD_Line
{
	int nLength;						// 当前接收到命令行的长度
	char *Buffer;						// 指向存放当前接收到的命令行的缓冲区
};

/*----------------------------------------------------------------------------+
| Constant Definition                                                         |
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Function Prototype                                                          |
+----------------------------------------------------------------------------*/
void Init_Shell(void);
void CMD_RxByte(char aData);
void CMD_RxFrame(void);

int str2int(char *str);

// ---------------------------------------------------------------------------
int CMD_Help(int argc, char *argv[]);
int CMD_SysClk(int argc, char *argv[]);
int CMD_GetDate(int argc, char *argv[]);
int CMD_GetTime(int argc, char *argv[]);
int CMD_ChgBaudRate(int argc, char *argv[]);
int CMD_SetIpAddr(int argc, char *argv[]);
int CMD_CD(int argc, char *argv[]);
int CMD_Dir(int argc, char *argv[]);

int CMD_Test(int argc, char *argv[]);
int CMD_Type(int argc, char *argv[]);
int CMD_FDump(int argc, char *argv[]);

//int CMD_Test(int argc, char *argv[]);

int CMD_Cls(int argc, char *argv[]);

/*----------------------------------------------------------------------------+
| End of header file                                                          |
+----------------------------------------------------------------------------*/
#endif // __COMMAND_H__
/*------------------------ Nothing Below This Line --------------------------*/
