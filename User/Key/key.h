#ifndef _KEY_H
#define _KEY_H

#include "sys.h"
#include <string.h>

#define key_1 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)
#define key_2 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7)
#define key_3 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8)
#define key_4 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)
#define key_5 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)

#define key1 1	//����1
#define key2 2	//����2
#define key3 3	//����3
#define key4 4	//����4
#define key5 5	//����5

void key_init();
u8 KEY_Scan(u8);  	//����ɨ�躯��	
void Key_Show(void);//��������ʾ�˵�

#endif
