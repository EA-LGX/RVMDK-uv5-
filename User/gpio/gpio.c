#include "gpio.h"

void GPIO_Configuration(void) {	//GPIO初始化
	GPIO_InitTypeDef GPIO_InitStructure;
	/*初始化结构体*/
	GPIO_StructInit(&GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//使能PA端口时钟 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//使能PC端口时钟 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);//使能PC端口时钟 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//普通推挽输出 
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA.0

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//普通推挽输出 
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA.1

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//普通推挽输出 
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA.4，控制NB模块开关

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//普通推挽输出 
	GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化PB.5，控制传感器开关

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//普通推挽输出 
	GPIO_Init(GPIOC, &GPIO_InitStructure);      //初始化PC.13，控制红色R_LED

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//普通推挽输出 
	GPIO_Init(GPIOC, &GPIO_InitStructure);      //初始化PC.14，控制绿色G_LED

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//普通推挽输出 
	GPIO_Init(GPIOC, &GPIO_InitStructure);      //初始化PC.15，控制黄色Y_LED

	R_LED_OFF;
	G_LED_OFF;
	Y_LED_OFF;
}

void Init_Nbiot(void) {
	unsigned int i, j, n = 3;
	char n_str;
	char temp_str[30];
	for (i = 0;i < 30;i++)
		temp_str[i] = 0;
	Y_LED_ON;
	USART1TxStr("The data return by checking NB moudule...\r\n");
	LCD_ShowString(8 * 2, 16 * 2, "restarting...");
	delay_ms(1000);
	USART2TxStr("AT+QRST=1\r\n");
	delay_ms(500);
	Wait_Str1_Str2_x_100ms(2, 1, 1, "Leaving the BROM", "", 60);
	CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	while (n--) {
		USART1TxStr("Sending the AT Command...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 3, &n_str);
		LCD_ShowString(8 * 2, 16 * 3, "AT...");
		USART2TxStr("AT\r\n");
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "OK", "", 50))
			n = 0;
	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("Checking the device's PDP address...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 4, &n_str);
		LCD_ShowString(8 * 2, 16 * 4, "AT+CGPADDR=1");
		USART2TxStr("AT+CGPADDR=1\r\n");
		if (!Wait_Str1_Str2_x_100ms(2, 2, 2, "+CGPADDR:", "OK", 50))
			n = 0;
	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("Config the CTwing's address and port...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 5, &n_str);
		LCD_ShowString(8 * 2, 16 * 5, "AT+QLWSERV=\"221.229.214.202\",5683");
		USART2TxStr("AT+QLWSERV=\"221.229.214.202\",5683\r\n");
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "OK", "", 50))
			n = 0;
	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("Checking the device's IMEI...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 6, &n_str);
		LCD_ShowString(8 * 2, 16 * 6, "AT+CGSN=1");
		USART2TxStr("AT+CGSN=1\r\n");
		if (!Wait_Str1_Str2_x_100ms(2, 2, 2, "+CGSN:", "OK", 50)) {
			for (i = 0;i < USART_REC_LEN;i++) {
				if ((USART2_RX_BUF[i] == '+')
					&& (USART2_RX_BUF[i + 1] == 'C')
					&& (USART2_RX_BUF[i + 2] == 'G')
					&& (USART2_RX_BUF[i + 3] == 'S')
					&& (USART2_RX_BUF[i + 4] == 'N')
					&& (USART2_RX_BUF[i + 5] == ':')) {
					strcpy(temp_str, "AT+QLWCONF=\"");
					for (j = 0;j < 15;j++, i++)
						temp_str[j + 12] = USART2_RX_BUF[i + 7];
					strcpy(&temp_str[27], "\"\r\n");
					i = USART_REC_LEN;
				}
			}
			n = 0;
		}
	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("config the CTwing's param...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 7, &n_str);
		LCD_ShowString(8 * 2, 16 * 7, temp_str);
		USART2TxStr(temp_str);
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "OK", "", 50))
			n = 0;
		for (i = 0;i < 30;i++)
			temp_str[i] = 0;
	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("Add the LwM2M object...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 8, &n_str);
		LCD_ShowString(8 * 2, 16 * 8, "AT+QLWADDOBJ=19,0,1,\"0\"");
		USART2TxStr("AT+QLWADDOBJ=19,0,1,\"0\"\r\n");
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "OK", "", 50))
			n = 0;
		delay_ms(1000);delay_ms(1000);delay_ms(1000);
	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("Enable the RF and Send a request about register...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 9, &n_str);
		LCD_ShowString(8 * 2, 16 * 9, "AT+QLWOPEN=0");
		USART2TxStr("AT+QLWOPEN=0\r\n");
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "CONNECT OK", "", 100)) {
			USART1TxStr("Server Connected!\r\n");
		}
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "+QLWOBSERVE:", "", 70))
			n = 0;

	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("Set the data' format...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 10, &n_str);
		LCD_ShowString(8 * 2, 16 * 10, "AT+QLWCFG=\"dataformat\",1,1");
		USART2TxStr("AT+QLWCFG=\"dataformat\",1,1\r\n");
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "OK", "", 50))
			n = 0;

	}CLR_Buf2();

	Y_LED_OFF;
	delay_ms(1000);
	Y_LED_ON;
	n = 3;
	while (n--) {
		USART1TxStr("Send handshake frame...\r\n");
		n_str = (3 - n) + '0';
		LCD_ShowString(8 * 1 - 3, 16 * 11, &n_str);
		LCD_ShowString(8 * 2, 16 * 11, "AT+QLWDATASEND=19,0,1,14,02......");
		USART2TxStr("AT+QLWDATASEND=19,0,1,14,02000100070007636f6e6e656374,0x0100\r\n");
		//	Wait_Str1_Str2_x_100ms(5,1,1,"OK","",20);
		if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "SEND OK", "", 100)) {
			n = 0;
			G_LED_ON;
			Y_LED_OFF;
			R_LED_OFF;
			LCD_ShowString(8 * 2, 16 * 12, "send ok!");
		}
		else {
			Y_LED_OFF;
			G_LED_OFF;
			Y_LED_ON;
			LCD_ShowString(8 * 2, 16 * 12, "fail   !");
		}
	}CLR_Buf2();
}

