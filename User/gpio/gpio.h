#ifndef __GPIO_H
#define __GPIO_H

#include "stm32f10x.h"
#include "usart.h"

#define R_LED_ON       {GPIO_ResetBits(GPIOC,GPIO_Pin_13);}//亮
#define R_LED_OFF			{GPIO_SetBits(GPIOC,GPIO_Pin_13);}//灭

#define G_LED_ON       {GPIO_ResetBits(GPIOC,GPIO_Pin_14);}//亮
#define G_LED_OFF			{GPIO_SetBits(GPIOC,GPIO_Pin_14);}//灭

#define Y_LED_ON       {GPIO_ResetBits(GPIOC,GPIO_Pin_15);}//亮
#define Y_LED_OFF			{GPIO_SetBits(GPIOC,GPIO_Pin_15);}//灭	

#define R_LED PCout(13)
#define G_LED PCout(14)
#define Y_LED PCout(15)

#define NB_PWR_ON()								GPIO_SetBits(GPIOA,GPIO_Pin_4)	//NB模块上电
#define NB_PWR_OFF()								GPIO_ResetBits(GPIOA,GPIO_Pin_4)	 //NB模块断电

#define Sensor_PWR_ON()								GPIO_SetBits(GPIOB,GPIO_Pin_5)	//传感器上电
#define Sensor_PWR_OFF()								GPIO_ResetBits(GPIOB,GPIO_Pin_5)	 //传感器断电

void GPIO_Configuration(void);
void Init_Nbiot(void);    //初始化NB模块

#endif /* __GPIO_H */
