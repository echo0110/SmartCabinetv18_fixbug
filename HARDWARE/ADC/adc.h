#ifndef __ADC_H
#define __ADC_H	
#include "sys.h"

extern float VOL, CUR;

void Adc_Init(void);
void Get_Vol(void);
void Get_Cur(void);
void output_control_default(void);
extern void read_Flooding(void);
 
#endif 
