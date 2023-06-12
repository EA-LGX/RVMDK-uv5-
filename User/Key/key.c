#include "key.h"
#include "stm32f10x.h"
#include "delay.h"
#include "gpio.h"
#include "lcd.h"
#include <string.h>

//初始化按键
void key_init() {
	GPIO_InitTypeDef GPIO_InitStruture;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//打开PD时钟
	GPIO_InitStruture.GPIO_Mode = GPIO_Mode_IPU;//上拉输入
	GPIO_InitStruture.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_11 | GPIO_Pin_12; //设置端口
	GPIO_Init(GPIOA, &GPIO_InitStruture);//初始化POID
}

void key_scan_task(int key_vaule) {

}

//按键处理函数
//返回按键值
//mode:0,不支持连续按;1,支持连续按;
u8 KEY_Scan(u8 mode) {
	static u8 flag = 1;//之前没有按键按下
	//static u8 key_up=1;//按键按松开标志
	if (mode)flag = 1;  //支持连按		  
	if (flag == 1 && (key_1 == 0 || key_2 == 0 || key_3 == 0 || key_4 == 0 || key_5 == 0)) {
		delay_ms(10);//去抖动 
		flag = 0;
		if (key_1 == 0)return key1;
		else if (key_2 == 0)return key2;
		else if (key_3 == 0)return key3;
		else if (key_4 == 0)return key4;
		else if (key_5 == 0)return key5;
	}
	else if (key_1 == 1 && key_2 == 1 && key_3 == 1 && key_4 == 1 && key_5 == 1)flag = 1;
	return 0;// 无按键按下
}
