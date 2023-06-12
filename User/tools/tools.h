/**
 * 工具类文件，放置一些常用或不好归类的函数
 */
#ifndef _TOOLS_H
#define _TOOLS_H

#include "gpio.h"
#include "usart.h"
#include "timer.h"
#include "delay.h"
#include "key.h"
#include "lcd.h"
#include "sys.h"
#include "spi.h"

unsigned char Query(char* src, char* des, unsigned int LEN);
bool Wait_Str_x_100ms(char* str1, char* str2, unsigned char time_x_100ms);
bool Wait_Str1_Str2_x_100ms(char uartx, char and_or, char str_NO, char* str1, char* str2, unsigned char time_x_100ms);
bool Wait_OK_x_100ms(unsigned char time_x_100ms);

void Wait_OK(void);
void copy_str(char* des, char* src, unsigned char len);
void DealUpData(char* from, uint8_t* to, unsigned int service_id);
void Array_CLR(char* src, char len);

#endif // __TOOLS_H__
