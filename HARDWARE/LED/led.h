#ifndef __LED_H
#define __LED_H	 
#include "sys.h"

//#define LED_MCU PGout(7)	// PG7
#define LED_MCU PAout(1)	// PA1
#define LED_FAN PCout(7)	// PC7	

void LED_Init(void);

#endif
