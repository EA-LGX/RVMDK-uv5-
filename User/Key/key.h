#ifndef _KEY_H
#define _KEY_H

#include "sys.h"
#include <string.h>

#define key_1 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)
#define key_2 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7)
#define key_3 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8)
#define key_4 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)
#define key_5 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)

#define key1 1	//按键1
#define key2 2	//按键2
#define key3 3	//按键3
#define key4 4	//按键4
#define key5 5	//按键5

void key_init();
u8 KEY_Scan(u8);  	//按键扫描函数	
void Key_Show(void);//键盘与显示菜单

#endif
