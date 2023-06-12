/* LiteOS ͷ�ļ� */
#include "los_sys.h"
#include "los_task.ph"
#include "los_sem.h"

/* �弶����ͷ�ļ� */
#include "stm32f10x.h"
#include "delay.h"
#include "sys.h"
#include "lcd.h"
#include "spi.h"
#include "usart.h"
#include "timer.h"
#include "gpio.h"
#include "key.h"
#include "tools.h"
#include "24l01.h"
#include "rc522.h"
#include <string.h>
#include <stdlib.h>

 /* ���������� */
UINT32 Pend_Task_Handle;
UINT32 Post_Task_Handle;     //�ϱ�����
UINT32 Collect_Task_Handle;  //�ɼ�����
UINT32 Show_Task_Handle;     //��ʾ����

/* �����ֵ�ź����ľ�� */
UINT32 BinarySem_Handle;

/* �������� */
static UINT32 AppTaskCreate(void);
static UINT32 Creat_Pend_Task(void);
static UINT32 Creat_Post_Task(void);
static UINT32 Creat_Collect_Task(void);
static UINT32 Creat_Show_Task(void);

static void Show_Task(void);
static void Collect_Task(void);
static void Post_Task(void);

static void BSP_Init(void);


#define deviceNum 2	// ��ֵ��ѡ��ֻ����0-1֮��仯,��ǰֻ������ʪ�ȴ�����
#define valueLen  5	// ���ݳ���  5���ַ������磺025.0

#define		Humiture		1	//��ʪ��
unsigned char Read_Humiture_CMD[8] = { 0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B };// ��ȡ��ַ01����ʪ������
// unsigned char Read_Humiture_CMD[8] = { 0x02,0x03,0x00,0x00,0x00,0x02,0xC4,0x38 };  // ��ȡ��ַ02����ʪ������
unsigned int Tem_value = 0, Hum_value = 0;
int Tem_value_int = 0;
char Temp_value_str[7];  //�¶��ַ���
char Hum_value_str[6];   //ʪ���ַ���
int lockCount = 0;  //����������
extern int open_flag;
int mainMenuCount = 0; //�˵���Ŀ

u8 keyTemp;  //��ʱ���水��ֵ
int devicePointer = 0; //�趨��ֵ���ڱ�ǵ�ǰѡ�е��豸��Ĭ��ѡ�е�һ���豸���¶ȣ�
int weishuPointer = 0; //�趨��ֵ���ڱ�ǵ�ǰѡ�е�λ����Ĭ��ѡ�е�һ��λ��
unsigned char Flag_R_LED_ONOFF = 1;
extern unsigned int Count_timer;  // ��������ʱ�������100ms
unsigned int i;
static unsigned int currentTimeFlag;
static unsigned int collectTimeFlag;
static unsigned int uploadTimeFlag;
char Send_data_buf[150];
unsigned char NB_Send_buf[355];
u8 U5RXBUFF[200] = { 0 };
extern unsigned char SN[4]; //����

// NRF24L01
u8 MASTER_ADDRESS[5] = { 0x34,0x43,0x10,0x10,0x01 }; // ������ַ
u8 MASTER_ADDRESS_STRING[11] = { 0 }; // ������ַ�ַ���
u8 RX_ADDRESS[RX_ADR_WIDTH] = { 0x34,0x43,0x10,0x10,0x01 }; //���豸��ַ
u8 TX_ADDRESS[5] = { 0 };  //Ŀ���ַ 
int TX_ADDRESS_INDEX = 0;  // Ŀ���ַ����
int TX_ADDRESS_LIST_LEN = 0;  // Ŀ���ַ�б���
u8 TX_ADDRESS_LIST[20][10] = { 0 };
u8 DEVICE_TYPE[6] = { 0 };
u8 RX_ADDRESS_STRING[11] = { 0 };
void HexToStr(u8 hex[], u8* str, int len, int isCover); // ��ʮ����������ת��Ϊ�ַ���
void GetNextTXAddress(void); // ��ȡ��һ��Ŀ���ַ
void AddTXAddressToList(u8* address); // ���Ŀ���ַ���б�
void AddMeunItem(char* item); // ��Ӳ˵���
void AddMeunChineseItem(char* item); // ��Ӳ˵���

/** maxValue����
 * �±�1��ѡ��Ҫ�޸�ֵ 0-�¶�  1-ʪ��  2-����  3-PM2.5  4-PM10
 * �±�2��0 ����豸����  1 �����ֵ��С
 * �±�3�����ƻ�����ֵ����Ϊ��Щ�豸���ƱȽϳ���������������12���ֽ�
 * �����¶���ֵ��maxValue[0][0][]="�¶���ֵ"
 *            maxValue[0][1][]="025.0"
*/
char maxValue[deviceNum][2][12];
float maxTemp = 0;  //�¶���ֵfloat���ͣ����ڴ���ַ���ת�������ֵ
float maxHum = 0;  //ʪ����ֵfloat���ͣ����ڴ���ַ���ת�������ֵ
extern bool Flag_Need_Init_Nbiot;
extern bool Flag_device_error;
extern bool is_send_breath;

void mainMenu(void);  // ���˵�����
void setMaxValue(void);  // 1.��ֵ���ý���
void collectSensorData(void); // 3.�ɼ����������ݽ���
void controllLED(void);  // 4.����LED�ƽ���
void nRF24L01(void);  // 5.nRF24L01����
void rc522();  // 6.RFID����


int main(void) {
	GPIO_Configuration(); 		//GPIO��ʼ��
	delay_init(); 			 //��ʱ��ʼ��
	Lcd_Init();  			 //LCD��Ļ��ʼ��
	LCD_Clear(BLACK); 		 //����
	Usart1_Init(115200); 	 //����1��ʼ��
	Usart2_Init(115200); 	 //����2��ʼ��
	Usart3_Init(9600);  	 //����3��ʼ��
	Uart5_Init(115200);				//����5��ʼ��
	NVIC_Configuration(); 	 //�жϳ�ʼ��  	
	TIM3_Int_Init(999, 7199);  //��ʱ��3��ʼ����100ms�ж�һ��
	Sensor_PWR_ON();  		//�������ϵ�

	BACK_COLOR = BLACK;   //���ñ���ɫ
	POINT_COLOR = GREEN;  //����������ɫ
	// ��ʼ����ֵ����
	strcat(&maxValue[0][0][0], "�¶���ֵ");
	strcat(&maxValue[0][1][0], "025.0");
	strcat(&maxValue[1][0][0], "ʪ����ֵ");
	strcat(&maxValue[1][1][0], "030.0");

	LCD_DrawRectangle(0, 0, 320, 240);  //�����ο�
	key_init();  //������ʼ��
	LCD_ShowString(320 - 8 * 14, 220, "202009517111");
	LCD_Show_Chinese16x16(320 - 16 * 4, 200, "½����");
	while (1) {
		mainMenu();
	}
}

// ���˵�
void mainMenu() {
	static u8 arrow = 0; //��ͷλ�ã���ǰѡ�еĲ˵���
	TIM_Cmd(TIM3, DISABLE);  //ʧ��TIM3	 DISABLE
	LCD_Clear(BLACK); //���� 

	LCD_DrawRectangle(0, 0, 320, 240);	//�����ο�	
	LCD_ShowString(320 - 14 * 8, 220, "202009517111");
	LCD_Show_Chinese16x16(320 - 16 * 4, 200, "½����");
	mainMenuCount = 0;
	AddMeunChineseItem("��ֵ�趨");
	AddMeunItem("NB-init");
	AddMeunChineseItem("��ʼ�ɼ�");
	AddMeunChineseItem("ת���ģ��");
	AddMeunItem("NRF24L01");
	AddMeunItem("RFID");
	AddMeunChineseItem("��ֵ�趨");
	AddMeunChineseItem("��ֵ�趨");
	AddMeunChineseItem("��ֵ�趨");
	AddMeunChineseItem("��ֵ�趨");

	LCD_ShowString(30 - 16, 30 + 16 * 2 * arrow, ">");

	// while (1) {
	// 	for (int i = 0; i < 100; i++) {
	// 		LCD_LoadBar(210, 10, 100, 16, i);  //���ؽ�����
	// 		// ����ת�ַ�����ʾ��LCD��
	// 		char str[4] = { 0 };
	// 		sprintf(str, "%3d", i);
	// 		LCD_ShowString(180, 10, str);
	// 		delay_ms(50);
	// 	}
	// 	LCD_Fill(210, 10, 310, 20, BLACK);  //LCD��ʾ��ɫ����
	// }

	while (1) {
		while (!(keyTemp = KEY_Scan(1))); //�������ȴ��û�����

		if (keyTemp == key1) {
			arrow = 0;
			break;
		}
		else if (keyTemp == key2) {  	//ģʽѡ��,��ͷ�ƶ�
			if (arrow < 5) {
				LCD_Fill(30 - 16, 30 + 16 * 2 * (arrow % 6), 30 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD��ʾ��ɫ����
				LCD_ShowString(30 - 16, 30 + 16 * 2 * (arrow % 6 + 1), ">");
			}
			else if (arrow == 5) {
				LCD_Fill(30 - 16, 30 + 16 * 2 * (arrow % 6), 30 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD��ʾ��ɫ����
				LCD_ShowString(170 - 16, 30, ">");
			}
			else if (arrow == mainMenuCount - 1) {
				LCD_Fill(170 - 16, 30 + 16 * 2 * (arrow % 6), 170 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD��ʾ��ɫ����
				LCD_ShowString(30 - 16, 30, ">");
			}
			else if (arrow > 5) {
				LCD_Fill(170 - 16, 30 + 16 * 2 * (arrow % 6), 170 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD��ʾ��ɫ����
				LCD_ShowString(170 - 16, 30 + 16 * 2 * (arrow % 6 + 1), ">");
			}
			arrow = (arrow + 1) % mainMenuCount;  //��ͷ��ԶС�ڲ˵���
			delay_ms(300);
		}
		else if (keyTemp == key3) {
			// �����κβ���
		}
		else if (keyTemp == key4) {
			// �����κβ���
		}
		else if (keyTemp == key5) {  //ȷ��
			// ���ݵ�ǰ��ͷλ�ã����벻ͬ�Ľ���
			if (arrow == 0) {
				setMaxValue();  //��ֵ���ý���
				break;
			}
			else if (arrow == 1) {	// ��ʼ��NBģ��
				TIM_Cmd(TIM3, ENABLE);
				NB_PWR_ON();  			 //NBģ���ϵ�
				BACK_COLOR = BLACK;   //���ñ���ɫ
				POINT_COLOR = GREEN;  //����������ɫ
				LCD_Clear(BLACK); //����
				LCD_ShowString(10, 220, "NB-Moudle Init...");
				Flag_Need_Init_Nbiot = 0;
				Flag_device_error = 0;
				Init_Nbiot();  		//��ʼ��NBģ��
				TIM_Cmd(TIM3, DISABLE);
				break;
			}
			else if (arrow == 2) {
				collectSensorData();  //�ɼ����ݽ���
				break;
			}
			else if (arrow == 3) {
				controllLED();  //����LED����
				break;
			}
			else if (arrow == 4) {// NRF24L01����
				nRF24L01();
				break;
			}
			else if (arrow == 5) {// RC522����
				LCD_Clear(BLACK); //����
				if (Flag_Need_Init_Nbiot != 0) {
					LCD_ShowString(120, 100, "NB");
					LCD_Show_Chinese16x16(60 + 16, 100, "ģ�黹û�г�ʼ��");
					delay_ms(5000);
					break;
				}
				rc522();
				break;
			}
			else if (arrow == 6) {
			}
		}
	}
}

// 1.��ֵ���ý���
void setMaxValue() {
	LCD_Clear(BLACK); //���� 
	POINT_COLOR = RED;		//�ʻ�Ϊ��ɫ	
	LCD_Show_Chinese16x16(280 - 16 * 3, 40, "����˵��");
	POINT_COLOR = GREEN;		//�ʻ�Ϊ��ɫ	
	LCD_ShowString(280 - 16 * 3, 40 + 16 * 1, "HOME");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 1, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 2, "�豸ѡ��");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 2, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 3, "λ��ѡ��");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 3, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 4, "��");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 4, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 5, "��");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 5, "*");
	LCD_DrawRectangle(0, 0, 320, 240);	//�����ο�	
	LCD_ShowString(320 - 14 * 8, 220, "202009517111");
	LCD_Show_Chinese16x16(320 - 16 * 4, 200, "½����");
	LCD_Show_Chinese16x16(40 + 16 * 2, 40, &maxValue[0][0][0]);  // Ĭ����ʾ0���豸����
	LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40, &maxValue[0][1][0]); // Ĭ����ʾ0���豸��ֵ
	LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 1, "-"); // Ĭ�ϼ�ͷָ��ĵ�һλ��
	while (1) {
		while (!(keyTemp = KEY_Scan(0))); //�������ȴ��������²�������ֵ����keyTemp
		if (keyTemp == key1) { //�˳���ֵ���ý���
			Y_LED_ON;
			USART1TxStr("��ֵ�趨���,�ϱ���ֵ:");
			Send_data_buf[0] = 0;  // ��һ�»�������
			strcat(Send_data_buf, &maxValue[0][1][0]);  // ����maxValue[0][1]="025.0"
			strcat(Send_data_buf, &maxValue[1][1][0]);  // ����maxValue[1][1]="050.0"
			USART1TxStr(Send_data_buf); // ��ӡƴ�Ӻ����ֵ 025.0050.0
			strcpy((char*)Send_data_buf, "AT+QLWDATASEND=");
			DealUpData(Send_data_buf, &NB_Send_buf[15], 21);  // 21Ϊ�����ƶ���ķ���ID
			USART2TxStr((char*)Send_data_buf);  // ����ֵ�ϱ���������
			LCD_ShowString(8 * 2, 16 * 12, "send....");
			G_LED_ON;
			Y_LED_OFF;
			R_LED_OFF;
			LCD_ShowString(8 * 2, 16 * 12, "Send OK!");
			CLR_Buf2();
			LCD_Clear(BLACK); //����
			break;
		}
		else if (keyTemp == key2) {  // �л��豸
			// �������㣬ʹ��devicePointer��ֵʼ��Ϊ0~1���ﵽѭ���л���Ч��
			devicePointer = (devicePointer + 1) % deviceNum;
			LCD_Show_Chinese16x16(40 + 16 * 2, 40 + 16 * 0, &maxValue[devicePointer][0][0]);  // ��ʾ�豸����
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 0, &maxValue[devicePointer][1][0]); // ��ʾ�豸��ֵ
			LCD_Fill(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, 40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8 + 8, 40 + 16 * 2, BLACK);  //LCD��ʾ��ɫ����
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 1, "-"); // ��ͷָ�ص�1λ
			weishuPointer = 0;// ��ͷָ�ص�1λ
		}
		else if (keyTemp == key3) { //λ���л�
			LCD_Fill(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, 40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8 + 8, 40 + 16 * 2, BLACK);  //LCD��ʾ��ɫ����
			weishuPointer = (weishuPointer + 1) % valueLen;
			if (maxValue[devicePointer][1][weishuPointer] == '.') { //����С����������
				LCD_Fill(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, 40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8 + 8, 40 + 16 * 2, BLACK);  //LCD��ʾ��ɫ����
				weishuPointer = (weishuPointer + 1) % valueLen;
			}
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, "-"); // ��ͷָ���λ��
		}
		else if (keyTemp == key4) {  //��
			maxValue[devicePointer][1][weishuPointer]++;
			if (maxValue[devicePointer][1][weishuPointer] > '9') //����9���0
				maxValue[devicePointer][1][weishuPointer] = '0';
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 0, &maxValue[devicePointer][1][0]); // ��ʾ�豸��ֵ
		}
		else if (keyTemp == key5) {  //��
			maxValue[devicePointer][1][weishuPointer]--;
			if (maxValue[devicePointer][1][weishuPointer] < '0') //С��0���9
				maxValue[devicePointer][1][weishuPointer] = '9';
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 0, &maxValue[devicePointer][1][0]); // ��ʾ�豸��ֵ
		}
	}
}

// 3.�ɼ����ݽ���
void collectSensorData() {
	char tmp[6] = { 0 };
	int n = 3; //����ʧ���ط�����
	TIM_Cmd(TIM3, ENABLE); // ������ʱ��3
	LCD_Clear(BLACK); //����
	LCD_Show_Chinese16x16(120, 60, "��ʪ��");
	LCD_Show_Chinese16x16(20 + 16 * 4, 100, "��ǰ        ��ֵ");
	LCD_Show_Chinese16x16(20, 120, "�¶ȣ�      ��          ��");
	LCD_Show_Chinese16x16(20, 160, "ʪ�ȣ�      ��          ��");
	maxTemp = atof(&maxValue[0][1][0]);
	maxHum = atof(&maxValue[1][1][0]);
	tmp[0] = 0;
	POINT_COLOR = RED;		//�ʻ�Ϊ��ɫ	
	sprintf(tmp, "%.1f", maxTemp);
	LCD_ShowString(20 + 16 * 3 + 12 * 8, 120, &tmp[0]);
	tmp[0] = 0;
	sprintf(tmp, "%.1f", maxHum);
	LCD_ShowString(20 + 16 * 3 + 12 * 8, 160, &tmp[0]);
	POINT_COLOR = GREEN;		//�ʻ�Ϊ��ɫ	
	uploadTimeFlag = Count_timer;  // ��ǵ�ǰʱ��,�ɼ�ʱ���־
	collectTimeFlag = Count_timer;  // ��ǵ�ǰʱ�䣬�ϴ�ʱ���־
	while ((keyTemp = KEY_Scan(0)) == 0) {// ��������˳����ݲɼ�����
		//	4s�ɼ�һ�����ݵ����ϴ�
		if (Count_timer - collectTimeFlag > 40) { // 4S��ʱ��  ��ǰʱ�����ȥ��ǵ�ʱ�����ȡʱ���
			//��ʼ�ɼ����ݣ�����LED
			Y_LED_ON;
			LCD_Fill(20 + 8 * 19 + 8 * 5, 10, 20 + 8 * 19 + 8 * 7, 10 + 16, YELLOW);
			USART3TxData_hex(Read_Humiture_CMD, 8);
			if ((Flag_Usart3_Receive) && (!Count_Timer3_value_USART3_receive_timeout)) {
				Flag_Usart3_Receive = 0;
				Tem_value = USART3_RX_BUF[3];
				Tem_value <<= 8;
				Tem_value |= USART3_RX_BUF[4];
				Hum_value = USART3_RX_BUF[5];
				Hum_value <<= 8;
				Hum_value |= USART3_RX_BUF[6];
				CLR_Buf3();
				Temp_value_str[0] = 't';
				Tem_value_int = Tem_value;
				if (Tem_value_int > 0) {
					Temp_value_str[1] = '+';
					Temp_value_str[2] = (char)(Tem_value / 100 + '0');
					Temp_value_str[3] = (char)(Tem_value % 100 / 10 + '0');
					Temp_value_str[4] = '.';
					Temp_value_str[5] = (char)(Tem_value % 10 + '0');
				}
				if (Tem_value_int < 0) {
					Tem_value_int = (~Tem_value_int) + 1;
					Temp_value_str[1] = '-';
					Temp_value_str[2] = (char)(Tem_value / 100 + '0');
					Temp_value_str[3] = (char)(Tem_value % 100 / 10 + '0');
					Temp_value_str[4] = '.';
					Temp_value_str[5] = (char)(Tem_value % 10 + '0');
				}
				Temp_value_str[6] = 0;
				Hum_value_str[0] = 'h';
				Hum_value_str[1] = (char)(Hum_value / 100 + '0');
				Hum_value_str[2] = (char)(Hum_value % 100 / 10 + '0');
				Hum_value_str[3] = '.';
				Hum_value_str[4] = (char)(Hum_value % 10 + '0');
				Hum_value_str[5] = 0;

				POINT_COLOR = RED;  //�ʻ�Ϊ��ɫ
				//�ж�ʪ���Ƿ������ֵ
				if (atof(&Temp_value_str[2]) > maxTemp) {//atof()�������ַ���ת��Ϊ������
					LCD_ShowString(20 + 8 * 27, 120, "Warning");
				}
				else {
					LCD_Fill(20 + 8 * 27, 120, 20 + 8 * 35, 120 + 16, BLACK);
				}
				// �ж��¶��Ƿ������ֵ
				if (atof(&Hum_value_str[1]) > maxHum) {
					LCD_ShowString(20 + 8 * 27, 160, "Warning");
				}
				else {
					LCD_Fill(20 + 8 * 27, 160, 20 + 16 * 16, 160 + 16, BLACK);
				}

				POINT_COLOR = YELLOW;  //�ʻ�Ϊ��ɫ
				LCD_ShowString(20 + 16 * 3, 120, &Temp_value_str[1]);
				LCD_ShowString(20 + 16 * 3, 160, &Hum_value_str[1]);
			}
			else {
				// POINT_COLOR = YELLOW;
				// LCD_ShowString(20 + 16 * 3, 120, "-----");
				// LCD_ShowString(20 + 16 * 3, 160, "-----");
				// LCD_ShowString(20 + 64 + 16 * 3, 120, "-----");
				// LCD_ShowString(20 + 64 + 16 * 3, 160, "-----");
				Temp_value_str[1] = 0;
				Hum_value_str[1] = 0;
			}

			// �ɼ���ϣ�Ϩ��LED�ƣ����±�ǵ�ǰʱ��
			Y_LED_OFF;
			LCD_Fill(20 + 8 * 19 + 8 * 5, 10, 20 + 8 * 19 + 8 * 7, 10 + 16, BLACK);
			collectTimeFlag = Count_timer;
		}
		else {
			tmp[0] = 0;
			sprintf(tmp, "%2d", (40 - (Count_timer - collectTimeFlag)) / 10);
			POINT_COLOR = GREEN;		//�ʻ�Ϊ��ɫ	
			LCD_ShowString(20, 10, "Collecting data in ");
			POINT_COLOR = YELLOW;		//�ʻ�Ϊ��ɫ	
			LCD_ShowString(20 + 8 * 20, 10, &tmp[0]);
			POINT_COLOR = GREEN;		//�ʻ�Ϊ��ɫ	
			LCD_ShowString(20 + 8 * 20 + 8 * 3, 10, "s");
		}

		//	20s�����ϱ�
		if (Count_timer - uploadTimeFlag > 200) { // 20S��ʱ��  ��ǰʱ�����ȥ��ǵ�ʱ�����ȡʱ���
			// ��ʼ�ϱ����ݣ�����LED
			G_LED_ON;
			LCD_Fill(20 + 8 * 19 + 8 * 5, 30, 20 + 8 * 19 + 8 * 7, 30 + 16, GREEN);
			USART1TxStr("NBIOT module is sending data:");
			Send_data_buf[0] = 0;
			strcat(Send_data_buf, &Temp_value_str[1]);  // ���±�1��ʼ����ȥ��t+25.0�е�t,ֻҪ+25.0
			strcat(Send_data_buf, &Hum_value_str[1]);  // ���±�1��ʼ����ȥ��h50.0�е�h,ֻҪ50.0
			USART1TxStr(Send_data_buf);   // ƴ�Ӻ����ݲ��� = +25.050.5
			strcpy((char*)NB_Send_buf, "AT+QLWDATASEND=");
			DealUpData(Send_data_buf, &NB_Send_buf[15], 20);  //20Ϊ�����Ʒ���ID
			n = 3;
			while (n--) {
				USART2TxStr((char*)NB_Send_buf);
				LCD_ShowString(8 * 2, 16 * 12, "send....");
				if (!Wait_Str1_Str2_x_100ms(2, 1, 1, "SEND OK", "", 100)) {
					n = 0;
					LCD_ShowString(8 * 2, 16 * 12, "Send OK!");
					break;
				}
				else {
					R_LED_ON;
					LCD_ShowString(8 * 2, 16 * 12, "Send Fail!");
				}
			}
			// �ϱ���ϣ�Ϩ��LED�ƣ����±�ǵ�ǰʱ��
			G_LED_OFF;
			R_LED_OFF;
			LCD_Fill(20 + 8 * 19 + 8 * 5, 30, 20 + 8 * 19 + 8 * 7, 30 + 16, BLACK);
			CLR_Buf2();
			uploadTimeFlag = Count_timer; //���±�ǵ�ǰʱ��
		}
		else {
			tmp[0] = 0;
			sprintf(tmp, "%3d", (200 - (Count_timer - uploadTimeFlag)) / 10);
			POINT_COLOR = GREEN;		//�ʻ�Ϊ��ɫ
			LCD_ShowString(20, 30, "Uploading  data in ");
			POINT_COLOR = YELLOW;		//�ʻ�Ϊ��ɫ	
			LCD_ShowString(20 + 8 * 19, 30, &tmp[0]);
			POINT_COLOR = GREEN;		//�ʻ�Ϊ��ɫ
			LCD_ShowString(20 + 8 * 20 + 8 * 3, 30, "s");
		}
	}
}

// 4.����LED����
void controllLED() {
	TIM_Cmd(TIM3, ENABLE); // ������ʱ��3
	LCD_Clear(BLACK); //���� 
	POINT_COLOR = RED;		//�ʻ�Ϊ��ɫ	
	LCD_Show_Chinese16x16(280 - 16 * 3, 40, "����˵��");
	POINT_COLOR = GREEN;		//�ʻ�Ϊ��ɫ	
	LCD_ShowString(16 * 3, 40 + 16 * 1, "HOME");
	LCD_ShowString(16 * 3 - 8, 40 + 16 * 1, "*");
	LCD_ShowString(16 * 3, 40 + 16 * 2, "YellowLED");
	LCD_ShowString(16 * 3 - 8, 40 + 16 * 2, "*");
	LCD_ShowString(16 * 3, 40 + 16 * 3, "Turn Left");
	LCD_ShowString(16 * 3 - 8, 40 + 16 * 3, "*");
	LCD_ShowString(16 * 3, 40 + 16 * 4, "Turn Right");
	LCD_ShowString(16 * 3 - 8, 40 + 16 * 4, "*");
	LCD_ShowString(16 * 3, 40 + 16 * 5, "Back");
	LCD_ShowString(16 * 3 - 8, 40 + 16 * 5, "*");
	R_LED_OFF;G_LED_OFF;Y_LED_OFF;  //�ȹص����е�LED
	while (1) {

		while (!(keyTemp = KEY_Scan(0))); //�������ȴ��������²�������ֵ����keyTemp

		if (keyTemp == key1) {  //����Key1�˳�ѭ�������˳�����LED����
			break;
		}

		else if (keyTemp == key2) {  //����Key2��ɫLED�����ɿ�Ϩ��
			Y_LED_ON;
			while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == 0); //�������ȴ������ɿ�
			Y_LED_OFF;
		}

		// ����һ�������ӳٵķ�����ȱ���ǵ��û����µ�˲�䣬�п��ܸպô����ӳٵ�ʱ����ڣ����°�����Ч
		//        ������ʱ��ʱ��Խ����������˲���豸������ʱ״̬�ĸ���Խ��
		else if (keyTemp == key3) { //����Key3��ɫLED��˸
			do {
				G_LED_ON;
				delay_ms(100);
				G_LED_OFF;
				delay_ms(100);
			} while (KEY_Scan(0) != key3);  //�ٴΰ���key3�ر�
		}

		// �����������ö�ʱ���ķ���������ʱ�����Խ������һ����������
		else if (keyTemp == key4) { //����Key4��ɫLED��˸ 
			R_LED_OFF;
			currentTimeFlag = Count_timer;  // ��ǵ�ǰʱ���
			while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 0); //�������ȴ��û��ɿ�����
			do {
				if (Count_timer - currentTimeFlag >= 2) {  // 200ms
					PCout(13) = !PCout(13);  // ��ת��ɫLED
					currentTimeFlag = Count_timer;  // ���±�ǵ�ǰʱ���
				}
			} while (KEY_Scan(0) != key4);
			R_LED_OFF;
		}

		else if (keyTemp == key5) { //����key5
			// �����κβ���
		}
	}
	TIM_Cmd(TIM3, DISABLE); // �رն�ʱ��3
}

// 5.nRF24L01����
void nRF24L01() {
	NRF24L01_Init(); // ��ʼ��NRF24L01
	LCD_Clear(BLACK); //����
	TIM_Cmd(TIM3, ENABLE); // �رն�ʱ��3
	printf("NRF24L01 ����ģʽ\r\n");
	printf("������ַ:");
	HexToStr(MASTER_ADDRESS, MASTER_ADDRESS_STRING, 5, 1);
	printf("%s", MASTER_ADDRESS_STRING);
	while (1) { // �������ģʽ
		char rx_buf[33] = { 0 };
		NRF24L01_RX_Mode(MASTER_ADDRESS);
		if (NRF24L01_RxPacket(rx_buf) == 0) { //һ�����յ���Ϣ,����ʾ����.
			rx_buf[32] = 0;//�����ַ���������
			printf("\r\n�յ���Ϣ:");
			printf("%s", rx_buf);
			AddTXAddressToList(rx_buf);
		}
	}
	int breakFlag = 0;
	while (1 || breakFlag == 0) {
		for (int i = 0; i < 3; i++) {
			u8 TX_ADDRESS_STRING[11] = { 0 };
			u8 tx_buf[33] = { 0 };
			u8 rx_buf[33] = { 0 };
			u8 tmp_buff[3] = { 0 };
			LCD_Clear(BLACK); //����
			LCD_ShowString(1, 1, "H:");
			LCD_ShowString(1, 8 * 3, RX_ADDRESS_STRING);
			GetNextTXAddress(); // ��ȡ��һ��Ŀ���ַ
			HexToStr(TX_ADDRESS, TX_ADDRESS_STRING, 5, 1);  //��Ŀ���ַת��Ϊ�ַ���
			NRF24L01_TX_Mode(TX_ADDRESS, RX_ADDRESS);
			printf("\r\nSend To ");
			printf("%s", TX_ADDRESS_STRING);
			LCD_ShowString(2, 1, "To:");
			LCD_ShowString(2, 8 * 4, TX_ADDRESS_STRING);
			strcat(tx_buf, RX_ADDRESS_STRING);
			if (NRF24L01_TxPacket(tx_buf) == TX_OK) {
				printf("\r\nSended DATA:");
				printf("%s", tx_buf);
				LCD_ShowString(3, 1, "S:");
				LCD_ShowString(3, 8 * 3, tx_buf);
				tx_buf[32] = 0;//���������	
			}
			tx_buf[0] = 0;

			currentTimeFlag = Count_timer; // ��ǵ�ǰʱ��,���ؽ���ģʽ
			R_LED = 1;
			NRF24L01_RX_Mode(RX_ADDRESS);

			while (1) {   // �ȴ��ظ� 5s // ����key1�˳�
				if (NRF24L01_RxPacket(rx_buf) == 0) { //һ�����յ���Ϣ,����ʾ����.
					rx_buf[32] = 0;//�����ַ���������
					printf("\r\n�յ���Ϣ:");
					printf("%s", rx_buf);
					LCD_ShowString(3, 1, "R:");
					LCD_ShowString(3, 8 * 3, rx_buf);
					rx_buf[0] = 0; // ��ս��ջ�����
				}

				if (Count_timer - currentTimeFlag > 50) {
					R_LED = 0;
					break;// 5s�󽫲��ٵȴ����˳�����ģʽ����������һ���豸
				}

				if (KEY_Scan(0) == key1) {
					breakFlag = 1;
					break;
				}
			};
			if (breakFlag == 1) {
				break;
			}
		}
		if (breakFlag == 1) {
			break;
		}
	}
	TIM_Cmd(TIM3, DISABLE); // �رն�ʱ��3
}

void rc522() {
	TIM_Cmd(TIM3, ENABLE);
	RC522_Init(); //RC522��ʼ��
	LCD_Clear(BLACK); //����
	is_send_breath = 1;
	LCD_DrawRectangle(0, 0, 320, 240);	//�����ο�	
	// �ڵڶ�����ʾ����ˢ����
	POINT_COLOR = YELLOW;
	LCD_Show_Chinese16x16(140, 10, "ˢ������������");
	POINT_COLOR = GREEN;
	LCD_ShowString(6, 5, "K1>");
	LCD_Show_Chinese16x16(6 + 8 * 3, 5, "����");

	LCD_ShowString(6, 190, "K5>");
	LCD_Show_Chinese16x16(6 + 8 * 3, 190, "����");

	/// ����ɨ��
	while (1) {
		while (!(keyTemp = KEY_Scan(0))) {//�������û�����֮ǰ��һֱѰ��
			if (open_flag == 1) {
				DOOR_OPEN = 1;// ����
				open_flag = 0;
			}
			if (open_flag == 2) {
				DOOR_OPEN = 0;//����
				open_flag = 0;
			}
			if (read_card_data() == MI_OK) {
				lockCount = 0;
				u8 card_id[20] = { 0 };
				for (int i = 0;i < 4;i++) {
					sprintf(&card_id[i * 2], "%02X", SN[i]);
				}
				UART5TxData(card_id);
				LCD_Show_Chinese16x16(120, 60, "����");
				LCD_ShowString(120 + 16 * 2, 60, ":");
				LCD_ShowString(120 + 16 * 2 + 8, 60, card_id);
				NB_Send_buf[0] = 0;

				// �������ϴ���������ƽ̨
				strcpy((char*)NB_Send_buf, "AT+QLWDATASEND=");
				DealUpData(card_id, &NB_Send_buf[15], 22);
				USART2TxStr((char*)NB_Send_buf);
				while (1) {
					currentTimeFlag = Count_timer;
					while (Count_timer < currentTimeFlag + 100) {
						LCD_LoadBar(120, 100, 100, 20, Count_timer - currentTimeFlag);
						if (Count_timer - currentTimeFlag > 100) {//����10s���߿���
							open_flag = 0;
							LCD_Fill(120, 100, 220, 120, BLACK);
							break;
						}
						if (open_flag == 1 || open_flag == 2) {
							LCD_Fill(120, 100, 220, 120, BLACK);
							break;
						}
					}
					break;
				}
				if (open_flag == 1) {
					DOOR_OPEN = 1; //����
					LCD_Show_Chinese16x16(120, 40, "��ӭ��");
					LCD_ShowString(120 + 16 * 3, 40, ":");
					LCD_Show_Chinese16x16(120 + 16 * 4, 40, "½����");
					POINT_COLOR = MAGENTA;
					G_LED = 1;
					LCD_Show_Chinese16x16(140, 100, "�ѿ���");//��ʾ�ѿ���
					while (keyTemp = KEY_Scan(0) != key5); //�ȴ��û�����
					DOOR_OPEN = 0;//����
					LCD_Show_Chinese16x16(140, 100, "�ѹ���");//��ʾ�ѹ���
					POINT_COLOR = GREEN;

					delay_ms(1000);
					G_LED = 0;
				}
				if (open_flag == 2) {
					POINT_COLOR = RED;
					LCD_Show_Chinese16x16(140, 100, "ע��");//��ʾ�ѿ���
					POINT_COLOR = GREEN;
					lockCount++;
					if (lockCount == 5) {
						delay_ms(60000);
					}
					delay_ms(1000);
				}
				LCD_Fill(120, 40, 310, 120, BLACK);
				open_flag = 0;
			}
		}

		if (keyTemp == key1) {  //����Key1�˳�ѭ�������˳�RC522ģ��
			break;
		}

		else if (keyTemp == key2) {  //����Key2��ɫLED�����ɿ�Ϩ��

		}

		// ����һ�������ӳٵķ�����ȱ���ǵ��û����µ�˲�䣬�п��ܸպô����ӳٵ�ʱ����ڣ����°�����Ч
		//        ������ʱ��ʱ��Խ����������˲���豸������ʱ״̬�ĸ���Խ��
		else if (keyTemp == key3) { //����Key3��ɫLED��˸
			do {
				G_LED_ON;
				delay_ms(100);
				G_LED_OFF;
				delay_ms(100);
			} while (KEY_Scan(0) != key3);  //�ٴΰ���key3�ر�
		}

		// �����������ö�ʱ���ķ���������ʱ�����Խ������һ����������
		else if (keyTemp == key4) { //����Key4��ɫLED��˸ 
			R_LED_OFF;
			currentTimeFlag = Count_timer;  // ��ǵ�ǰʱ���
			while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 0); //�������ȴ��û��ɿ�����
			do {
				if (Count_timer - currentTimeFlag >= 2) {  // 200ms
					PCout(13) = !PCout(13);  // ��ת��ɫLED
					currentTimeFlag = Count_timer;  // ���±�ǵ�ǰʱ���
				}
			} while (KEY_Scan(0) != key4);
			R_LED_OFF;
		}

		else if (keyTemp == key5) { //����key5
			// �����κβ���
		}
	}
	is_send_breath = 0;
	TIM_Cmd(TIM3, DISABLE);  //ʧ��TIM3	 DISABLE
}

/**
 * 16����ת�ַ���,����0x,�����ո�isCover�Ƿ񸲸� 1 ���� 0 ������
*/
void HexToStr(u8 hex[], u8* str, int len, int isCover) {
	if (isCover) {
		str[0] = 0;
	}
	for (int i = 0; i < len; i++) {
		char tmp[3] = { 0 };
		sprintf(tmp, "%02X", hex[i]);
		strcat(str, tmp);
	}
}

/**
 * ��ȡ��һ��Ŀ���ַ���豸���� ����TX_ADDRESS,DEVICE_TYPE
*/
void GetNextTXAddress(void) {
	for (int j = 0;j < 5;j++) {
		char tmp_buff[3] = { 0 };
		TX_ADDRESS[j] = TX_ADDRESS_LIST[TX_ADDRESS_INDEX][j];
	}
	for (int i = 5;i < 10;i++) {
		DEVICE_TYPE[i - 5] = TX_ADDRESS_LIST[TX_ADDRESS_INDEX][i];
	}
	// ��������
	TX_ADDRESS_INDEX++;
	TX_ADDRESS_INDEX = TX_ADDRESS_INDEX % TX_ADDRESS_LIST_LEN;
}

/**
 * ���һ��Ŀ���ַ���б�,�ַ�����ʽ��16���Ƶ�ַ,����֡��ʽ: ��ʪ�ȼ���豸:3443101001Humid
*/
void AddTXAddressToList(u8* address) {
	TX_ADDRESS_LIST_LEN++;
	//�豸��ַ
	for (int i = 0;i < 5;i++) {
		char tmp[3] = { 0 };
		tmp[0] = address[i * 2];
		tmp[1] = address[i * 2 + 1];
		TX_ADDRESS_LIST[TX_ADDRESS_LIST_LEN - 1][i] = strtol(tmp, NULL, 16);
	}
	//�豸����
	for (int i = 5;i < 10;i++) {
		TX_ADDRESS_LIST[TX_ADDRESS_LIST_LEN - 1][i] = address[i];
	}
}

void dataReceive(u8* data, u8 len) {
	// ����֡:+QLWDATARECV: 19,1,0,34,061F570035001B000B4C754775616E6778696E67000B313737353835343038333454
	UART5TxData("dataReceive: ");
	UART5TxData(data);
	// u8 temp[20] = { 0 };
	// u8 userNameHexStr[20] = { 0 };
	// for (int i = 0;i < 4;i++) {
	// 	temp[i] = data[i + 38];
	// }
	// UART5TxData("temp: ");
	// UART5TxData(temp);
	// int userNameLenth = 0;
	// userNameLenth = atoi(temp);
	// for (int i = 0;i < userNameLenth;i++) {
	// 	userNameHexStr[i] = data[i + 42];
	// }
	// UART5TxData("userNameHex: ");
	// UART5TxData(userNameHexStr);
	// u8 userName[20] = { 0 };
	// for (int i = 0;i < userNameLenth;i = i + 2) {
	// 	int tmp[3] = { 0 };
	// 	tmp[0] = userNameHexStr[i];
	// 	tmp[1] = userNameHexStr[i + 1];
	// }
	// UART5TxData("userName: ");
	// UART5TxData(userName);
}

void AddMeunItem(char* item) {
	if (mainMenuCount < 6) {  // ǰ6���˵��������
		LCD_Fill(30, 30 + 16 * ((mainMenuCount % 6) * 2), 30 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD��ʾ��ɫ����
		LCD_ShowString(30 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	else {
		LCD_Fill(170, 30 + 16 * ((mainMenuCount % 6) * 2), 170 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD��ʾ��ɫ����
		LCD_ShowString(170 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	mainMenuCount++;
}

void AddMeunChineseItem(char* item) {
	if (mainMenuCount < 6) {  // ǰ6���˵��������
		LCD_Fill(30, 30 + 16 * ((mainMenuCount % 6) * 2), 30 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD��ʾ��ɫ����
		LCD_Show_Chinese16x16(30 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	else {
		LCD_Fill(170, 30 + 16 * ((mainMenuCount % 6) * 2), 170 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD��ʾ��ɫ����
		LCD_Show_Chinese16x16(170 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	mainMenuCount++;
}