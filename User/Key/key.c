#include "key.h"
#include "stm32f10x.h"
#include "delay.h"
#include "gpio.h"
#include "lcd.h"
#include <string.h>

//��ʼ������
void key_init() {
	GPIO_InitTypeDef GPIO_InitStruture;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//��PDʱ��
	GPIO_InitStruture.GPIO_Mode = GPIO_Mode_IPU;//��������
	GPIO_InitStruture.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_11 | GPIO_Pin_12; //���ö˿�
	GPIO_Init(GPIOA, &GPIO_InitStruture);//��ʼ��POID
}

void key_scan_task(int key_vaule) {

}

//����������
//���ذ���ֵ
//mode:0,��֧��������;1,֧��������;
u8 KEY_Scan(u8 mode) {
	static u8 flag = 1;//֮ǰû�а�������
	//static u8 key_up=1;//�������ɿ���־
	if (mode)flag = 1;  //֧������		  
	if (flag == 1 && (key_1 == 0 || key_2 == 0 || key_3 == 0 || key_4 == 0 || key_5 == 0)) {
		delay_ms(10);//ȥ���� 
		flag = 0;
		if (key_1 == 0)return key1;
		else if (key_2 == 0)return key2;
		else if (key_3 == 0)return key3;
		else if (key_4 == 0)return key4;
		else if (key_5 == 0)return key5;
	}
	else if (key_1 == 1 && key_2 == 1 && key_3 == 1 && key_4 == 1 && key_5 == 1)flag = 1;
	return 0;// �ް�������
}
