#ifndef __GPIO_H
#define __GPIO_H

#include "stm32f10x.h"
#include "usart.h"

#define R_LED_ON       {GPIO_ResetBits(GPIOC,GPIO_Pin_13);}//��
#define R_LED_OFF			{GPIO_SetBits(GPIOC,GPIO_Pin_13);}//��

#define G_LED_ON       {GPIO_ResetBits(GPIOC,GPIO_Pin_14);}//��
#define G_LED_OFF			{GPIO_SetBits(GPIOC,GPIO_Pin_14);}//��

#define Y_LED_ON       {GPIO_ResetBits(GPIOC,GPIO_Pin_15);}//��
#define Y_LED_OFF			{GPIO_SetBits(GPIOC,GPIO_Pin_15);}//��	

#define R_LED PCout(13)
#define G_LED PCout(14)
#define Y_LED PCout(15)

#define NB_PWR_ON()								GPIO_SetBits(GPIOA,GPIO_Pin_4)	//NBģ���ϵ�
#define NB_PWR_OFF()								GPIO_ResetBits(GPIOA,GPIO_Pin_4)	 //NBģ��ϵ�

#define Sensor_PWR_ON()								GPIO_SetBits(GPIOB,GPIO_Pin_5)	//�������ϵ�
#define Sensor_PWR_OFF()								GPIO_ResetBits(GPIOB,GPIO_Pin_5)	 //�������ϵ�

void GPIO_Configuration(void);
void Init_Nbiot(void);    //��ʼ��NBģ��

#endif /* __GPIO_H */