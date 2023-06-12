/* LiteOS 头文件 */
#include "los_sys.h"
#include "los_task.ph"
#include "los_sem.h"

/* 板级外设头文件 */
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

 /* 定义任务句柄 */
UINT32 Pend_Task_Handle;
UINT32 Post_Task_Handle;     //上报任务
UINT32 Collect_Task_Handle;  //采集任务
UINT32 Show_Task_Handle;     //显示任务

/* 定义二值信号量的句柄 */
UINT32 BinarySem_Handle;

/* 函数声明 */
static UINT32 AppTaskCreate(void);
static UINT32 Creat_Pend_Task(void);
static UINT32 Creat_Post_Task(void);
static UINT32 Creat_Collect_Task(void);
static UINT32 Creat_Show_Task(void);

static void Show_Task(void);
static void Collect_Task(void);
static void Post_Task(void);

static void BSP_Init(void);


#define deviceNum 2	// 阈值的选项只能在0-1之间变化,当前只接入温湿度传感器
#define valueLen  5	// 数据长度  5个字符，例如：025.0

#define		Humiture		1	//温湿度
unsigned char Read_Humiture_CMD[8] = { 0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B };// 读取地址01的温湿度数据
// unsigned char Read_Humiture_CMD[8] = { 0x02,0x03,0x00,0x00,0x00,0x02,0xC4,0x38 };  // 读取地址02的温湿度数据
unsigned int Tem_value = 0, Hum_value = 0;
int Tem_value_int = 0;
char Temp_value_str[7];  //温度字符串
char Hum_value_str[6];   //湿度字符串
int lockCount = 0;  //锁定计数器
extern int open_flag;
int mainMenuCount = 0; //菜单数目

u8 keyTemp;  //临时保存按键值
int devicePointer = 0; //设定阈值用于标记当前选中的设备，默认选中第一个设备（温度）
int weishuPointer = 0; //设定阈值用于标记当前选中的位数，默认选中第一个位数
unsigned char Flag_R_LED_ONOFF = 1;
extern unsigned int Count_timer;  // 计数器，时间戳精度100ms
unsigned int i;
static unsigned int currentTimeFlag;
static unsigned int collectTimeFlag;
static unsigned int uploadTimeFlag;
char Send_data_buf[150];
unsigned char NB_Send_buf[355];
u8 U5RXBUFF[200] = { 0 };
extern unsigned char SN[4]; //卡号

// NRF24L01
u8 MASTER_ADDRESS[5] = { 0x34,0x43,0x10,0x10,0x01 }; // 主机地址
u8 MASTER_ADDRESS_STRING[11] = { 0 }; // 主机地址字符串
u8 RX_ADDRESS[RX_ADR_WIDTH] = { 0x34,0x43,0x10,0x10,0x01 }; //本设备地址
u8 TX_ADDRESS[5] = { 0 };  //目标地址 
int TX_ADDRESS_INDEX = 0;  // 目标地址索引
int TX_ADDRESS_LIST_LEN = 0;  // 目标地址列表长度
u8 TX_ADDRESS_LIST[20][10] = { 0 };
u8 DEVICE_TYPE[6] = { 0 };
u8 RX_ADDRESS_STRING[11] = { 0 };
void HexToStr(u8 hex[], u8* str, int len, int isCover); // 将十六进制数组转换为字符串
void GetNextTXAddress(void); // 获取下一个目标地址
void AddTXAddressToList(u8* address); // 添加目标地址到列表
void AddMeunItem(char* item); // 添加菜单项
void AddMeunChineseItem(char* item); // 添加菜单项

/** maxValue数组
 * 下标1：选中要修改值 0-温度  1-湿度  2-光照  3-PM2.5  4-PM10
 * 下标2：0 存放设备名称  1 存放阈值大小
 * 下标3：名称或者阈值，因为有些设备名称比较长，所以这里用了12个字节
 * 例如温度阈值：maxValue[0][0][]="温度阈值"
 *            maxValue[0][1][]="025.0"
*/
char maxValue[deviceNum][2][12];
float maxTemp = 0;  //温度阈值float类型，用于存放字符串转换后的数值
float maxHum = 0;  //湿度阈值float类型，用于存放字符串转换后的数值
extern bool Flag_Need_Init_Nbiot;
extern bool Flag_device_error;
extern bool is_send_breath;

void mainMenu(void);  // 主菜单界面
void setMaxValue(void);  // 1.阈值设置界面
void collectSensorData(void); // 3.采集传感器数据界面
void controllLED(void);  // 4.控制LED灯界面
void nRF24L01(void);  // 5.nRF24L01界面
void rc522();  // 6.RFID界面


int main(void) {
	GPIO_Configuration(); 		//GPIO初始化
	delay_init(); 			 //延时初始化
	Lcd_Init();  			 //LCD屏幕初始化
	LCD_Clear(BLACK); 		 //清屏
	Usart1_Init(115200); 	 //串口1初始化
	Usart2_Init(115200); 	 //串口2初始化
	Usart3_Init(9600);  	 //串口3初始化
	Uart5_Init(115200);				//串口5初始化
	NVIC_Configuration(); 	 //中断初始化  	
	TIM3_Int_Init(999, 7199);  //定时器3初始化，100ms中断一次
	Sensor_PWR_ON();  		//传感器上电

	BACK_COLOR = BLACK;   //设置背景色
	POINT_COLOR = GREEN;  //设置字体颜色
	// 初始化阈值数组
	strcat(&maxValue[0][0][0], "温度阈值");
	strcat(&maxValue[0][1][0], "025.0");
	strcat(&maxValue[1][0][0], "湿度阈值");
	strcat(&maxValue[1][1][0], "030.0");

	LCD_DrawRectangle(0, 0, 320, 240);  //画矩形框
	key_init();  //按键初始化
	LCD_ShowString(320 - 8 * 14, 220, "202009517111");
	LCD_Show_Chinese16x16(320 - 16 * 4, 200, "陆广兴");
	while (1) {
		mainMenu();
	}
}

// 主菜单
void mainMenu() {
	static u8 arrow = 0; //箭头位置，当前选中的菜单项
	TIM_Cmd(TIM3, DISABLE);  //失能TIM3	 DISABLE
	LCD_Clear(BLACK); //清屏 

	LCD_DrawRectangle(0, 0, 320, 240);	//画矩形框	
	LCD_ShowString(320 - 14 * 8, 220, "202009517111");
	LCD_Show_Chinese16x16(320 - 16 * 4, 200, "陆广兴");
	mainMenuCount = 0;
	AddMeunChineseItem("阈值设定");
	AddMeunItem("NB-init");
	AddMeunChineseItem("开始采集");
	AddMeunChineseItem("转向灯模拟");
	AddMeunItem("NRF24L01");
	AddMeunItem("RFID");
	AddMeunChineseItem("阈值设定");
	AddMeunChineseItem("阈值设定");
	AddMeunChineseItem("阈值设定");
	AddMeunChineseItem("阈值设定");

	LCD_ShowString(30 - 16, 30 + 16 * 2 * arrow, ">");

	// while (1) {
	// 	for (int i = 0; i < 100; i++) {
	// 		LCD_LoadBar(210, 10, 100, 16, i);  //加载进度条
	// 		// 数字转字符并显示在LCD上
	// 		char str[4] = { 0 };
	// 		sprintf(str, "%3d", i);
	// 		LCD_ShowString(180, 10, str);
	// 		delay_ms(50);
	// 	}
	// 	LCD_Fill(210, 10, 310, 20, BLACK);  //LCD显示黑色方块
	// }

	while (1) {
		while (!(keyTemp = KEY_Scan(1))); //阻塞，等待用户按键

		if (keyTemp == key1) {
			arrow = 0;
			break;
		}
		else if (keyTemp == key2) {  	//模式选择,箭头移动
			if (arrow < 5) {
				LCD_Fill(30 - 16, 30 + 16 * 2 * (arrow % 6), 30 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD显示黑色方块
				LCD_ShowString(30 - 16, 30 + 16 * 2 * (arrow % 6 + 1), ">");
			}
			else if (arrow == 5) {
				LCD_Fill(30 - 16, 30 + 16 * 2 * (arrow % 6), 30 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD显示黑色方块
				LCD_ShowString(170 - 16, 30, ">");
			}
			else if (arrow == mainMenuCount - 1) {
				LCD_Fill(170 - 16, 30 + 16 * 2 * (arrow % 6), 170 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD显示黑色方块
				LCD_ShowString(30 - 16, 30, ">");
			}
			else if (arrow > 5) {
				LCD_Fill(170 - 16, 30 + 16 * 2 * (arrow % 6), 170 - 8, 30 + 16 * 2 * (arrow % 6) + 16, BLACK);  //LCD显示黑色方块
				LCD_ShowString(170 - 16, 30 + 16 * 2 * (arrow % 6 + 1), ">");
			}
			arrow = (arrow + 1) % mainMenuCount;  //箭头永远小于菜单数
			delay_ms(300);
		}
		else if (keyTemp == key3) {
			// 不做任何操作
		}
		else if (keyTemp == key4) {
			// 不做任何操作
		}
		else if (keyTemp == key5) {  //确定
			// 根据当前箭头位置，进入不同的界面
			if (arrow == 0) {
				setMaxValue();  //阈值设置界面
				break;
			}
			else if (arrow == 1) {	// 初始化NB模块
				TIM_Cmd(TIM3, ENABLE);
				NB_PWR_ON();  			 //NB模块上电
				BACK_COLOR = BLACK;   //设置背景色
				POINT_COLOR = GREEN;  //设置字体颜色
				LCD_Clear(BLACK); //清屏
				LCD_ShowString(10, 220, "NB-Moudle Init...");
				Flag_Need_Init_Nbiot = 0;
				Flag_device_error = 0;
				Init_Nbiot();  		//初始化NB模块
				TIM_Cmd(TIM3, DISABLE);
				break;
			}
			else if (arrow == 2) {
				collectSensorData();  //采集数据界面
				break;
			}
			else if (arrow == 3) {
				controllLED();  //控制LED界面
				break;
			}
			else if (arrow == 4) {// NRF24L01界面
				nRF24L01();
				break;
			}
			else if (arrow == 5) {// RC522界面
				LCD_Clear(BLACK); //清屏
				if (Flag_Need_Init_Nbiot != 0) {
					LCD_ShowString(120, 100, "NB");
					LCD_Show_Chinese16x16(60 + 16, 100, "模块还没有初始化");
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

// 1.阈值设置界面
void setMaxValue() {
	LCD_Clear(BLACK); //清屏 
	POINT_COLOR = RED;		//笔画为红色	
	LCD_Show_Chinese16x16(280 - 16 * 3, 40, "操作说明");
	POINT_COLOR = GREEN;		//笔画为绿色	
	LCD_ShowString(280 - 16 * 3, 40 + 16 * 1, "HOME");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 1, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 2, "设备选择");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 2, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 3, "位数选择");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 3, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 4, "加");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 4, "*");
	LCD_Show_Chinese16x16(280 - 16 * 3, 40 + 16 * 5, "减");
	LCD_ShowString(280 - 16 * 3 - 8, 40 + 16 * 5, "*");
	LCD_DrawRectangle(0, 0, 320, 240);	//画矩形框	
	LCD_ShowString(320 - 14 * 8, 220, "202009517111");
	LCD_Show_Chinese16x16(320 - 16 * 4, 200, "陆广兴");
	LCD_Show_Chinese16x16(40 + 16 * 2, 40, &maxValue[0][0][0]);  // 默认显示0号设备名称
	LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40, &maxValue[0][1][0]); // 默认显示0号设备阈值
	LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 1, "-"); // 默认箭头指向的第一位数
	while (1) {
		while (!(keyTemp = KEY_Scan(0))); //阻塞，等待按键按下并将按键值赋给keyTemp
		if (keyTemp == key1) { //退出阈值设置界面
			Y_LED_ON;
			USART1TxStr("阈值设定完毕,上报阈值:");
			Send_data_buf[0] = 0;  // 清一下缓存数组
			strcat(Send_data_buf, &maxValue[0][1][0]);  // 例：maxValue[0][1]="025.0"
			strcat(Send_data_buf, &maxValue[1][1][0]);  // 例：maxValue[1][1]="050.0"
			USART1TxStr(Send_data_buf); // 打印拼接后的阈值 025.0050.0
			strcpy((char*)Send_data_buf, "AT+QLWDATASEND=");
			DealUpData(Send_data_buf, &NB_Send_buf[15], 21);  // 21为电信云定义的服务ID
			USART2TxStr((char*)Send_data_buf);  // 将阈值上报到服务器
			LCD_ShowString(8 * 2, 16 * 12, "send....");
			G_LED_ON;
			Y_LED_OFF;
			R_LED_OFF;
			LCD_ShowString(8 * 2, 16 * 12, "Send OK!");
			CLR_Buf2();
			LCD_Clear(BLACK); //清屏
			break;
		}
		else if (keyTemp == key2) {  // 切换设备
			// 求余运算，使得devicePointer的值始终为0~1，达到循环切换的效果
			devicePointer = (devicePointer + 1) % deviceNum;
			LCD_Show_Chinese16x16(40 + 16 * 2, 40 + 16 * 0, &maxValue[devicePointer][0][0]);  // 显示设备名称
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 0, &maxValue[devicePointer][1][0]); // 显示设备阈值
			LCD_Fill(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, 40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8 + 8, 40 + 16 * 2, BLACK);  //LCD显示黑色方块
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 1, "-"); // 箭头指回第1位
			weishuPointer = 0;// 箭头指回第1位
		}
		else if (keyTemp == key3) { //位数切换
			LCD_Fill(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, 40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8 + 8, 40 + 16 * 2, BLACK);  //LCD显示黑色方块
			weishuPointer = (weishuPointer + 1) % valueLen;
			if (maxValue[devicePointer][1][weishuPointer] == '.') { //遇到小数点则跳过
				LCD_Fill(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, 40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8 + 8, 40 + 16 * 2, BLACK);  //LCD显示黑色方块
				weishuPointer = (weishuPointer + 1) % valueLen;
			}
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10 + weishuPointer * 8, 40 + 16 * 1, "-"); // 箭头指向的位数
		}
		else if (keyTemp == key4) {  //加
			maxValue[devicePointer][1][weishuPointer]++;
			if (maxValue[devicePointer][1][weishuPointer] > '9') //超过9则归0
				maxValue[devicePointer][1][weishuPointer] = '0';
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 0, &maxValue[devicePointer][1][0]); // 显示设备阈值
		}
		else if (keyTemp == key5) {  //减
			maxValue[devicePointer][1][weishuPointer]--;
			if (maxValue[devicePointer][1][weishuPointer] < '0') //小于0则归9
				maxValue[devicePointer][1][weishuPointer] = '9';
			LCD_ShowString(40 + 16 * 2 + 4 * 16 + 10, 40 + 16 * 0, &maxValue[devicePointer][1][0]); // 显示设备阈值
		}
	}
}

// 3.采集数据界面
void collectSensorData() {
	char tmp[6] = { 0 };
	int n = 3; //发送失败重发次数
	TIM_Cmd(TIM3, ENABLE); // 开启定时器3
	LCD_Clear(BLACK); //清屏
	LCD_Show_Chinese16x16(120, 60, "温湿度");
	LCD_Show_Chinese16x16(20 + 16 * 4, 100, "当前        阈值");
	LCD_Show_Chinese16x16(20, 120, "温度：      ℃          ℃");
	LCD_Show_Chinese16x16(20, 160, "湿度：      ％          ％");
	maxTemp = atof(&maxValue[0][1][0]);
	maxHum = atof(&maxValue[1][1][0]);
	tmp[0] = 0;
	POINT_COLOR = RED;		//笔画为红色	
	sprintf(tmp, "%.1f", maxTemp);
	LCD_ShowString(20 + 16 * 3 + 12 * 8, 120, &tmp[0]);
	tmp[0] = 0;
	sprintf(tmp, "%.1f", maxHum);
	LCD_ShowString(20 + 16 * 3 + 12 * 8, 160, &tmp[0]);
	POINT_COLOR = GREEN;		//笔画为绿色	
	uploadTimeFlag = Count_timer;  // 标记当前时间,采集时间标志
	collectTimeFlag = Count_timer;  // 标记当前时间，上传时间标志
	while ((keyTemp = KEY_Scan(0)) == 0) {// 按任意键退出数据采集界面
		//	4s采集一次数据但不上传
		if (Count_timer - collectTimeFlag > 40) { // 4S定时器  当前时间戳减去标记的时间戳获取时间差
			//开始采集数据，点亮LED
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

				POINT_COLOR = RED;  //笔画为红色
				//判断湿度是否大于阈值
				if (atof(&Temp_value_str[2]) > maxTemp) {//atof()函数将字符串转换为浮点数
					LCD_ShowString(20 + 8 * 27, 120, "Warning");
				}
				else {
					LCD_Fill(20 + 8 * 27, 120, 20 + 8 * 35, 120 + 16, BLACK);
				}
				// 判断温度是否大于阈值
				if (atof(&Hum_value_str[1]) > maxHum) {
					LCD_ShowString(20 + 8 * 27, 160, "Warning");
				}
				else {
					LCD_Fill(20 + 8 * 27, 160, 20 + 16 * 16, 160 + 16, BLACK);
				}

				POINT_COLOR = YELLOW;  //笔画为黄色
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

			// 采集完毕，熄灭LED灯，重新标记当前时间
			Y_LED_OFF;
			LCD_Fill(20 + 8 * 19 + 8 * 5, 10, 20 + 8 * 19 + 8 * 7, 10 + 16, BLACK);
			collectTimeFlag = Count_timer;
		}
		else {
			tmp[0] = 0;
			sprintf(tmp, "%2d", (40 - (Count_timer - collectTimeFlag)) / 10);
			POINT_COLOR = GREEN;		//笔画为绿色	
			LCD_ShowString(20, 10, "Collecting data in ");
			POINT_COLOR = YELLOW;		//笔画为红色	
			LCD_ShowString(20 + 8 * 20, 10, &tmp[0]);
			POINT_COLOR = GREEN;		//笔画为绿色	
			LCD_ShowString(20 + 8 * 20 + 8 * 3, 10, "s");
		}

		//	20s数据上报
		if (Count_timer - uploadTimeFlag > 200) { // 20S定时器  当前时间戳减去标记的时间戳获取时间差
			// 开始上报数据，点亮LED
			G_LED_ON;
			LCD_Fill(20 + 8 * 19 + 8 * 5, 30, 20 + 8 * 19 + 8 * 7, 30 + 16, GREEN);
			USART1TxStr("NBIOT module is sending data:");
			Send_data_buf[0] = 0;
			strcat(Send_data_buf, &Temp_value_str[1]);  // 从下标1开始，即去掉t+25.0中的t,只要+25.0
			strcat(Send_data_buf, &Hum_value_str[1]);  // 从下标1开始，即去掉h50.0中的h,只要50.0
			USART1TxStr(Send_data_buf);   // 拼接后数据部分 = +25.050.5
			strcpy((char*)NB_Send_buf, "AT+QLWDATASEND=");
			DealUpData(Send_data_buf, &NB_Send_buf[15], 20);  //20为电信云服务ID
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
			// 上报完毕，熄灭LED灯，重新标记当前时间
			G_LED_OFF;
			R_LED_OFF;
			LCD_Fill(20 + 8 * 19 + 8 * 5, 30, 20 + 8 * 19 + 8 * 7, 30 + 16, BLACK);
			CLR_Buf2();
			uploadTimeFlag = Count_timer; //重新标记当前时间
		}
		else {
			tmp[0] = 0;
			sprintf(tmp, "%3d", (200 - (Count_timer - uploadTimeFlag)) / 10);
			POINT_COLOR = GREEN;		//笔画为绿色
			LCD_ShowString(20, 30, "Uploading  data in ");
			POINT_COLOR = YELLOW;		//笔画为红色	
			LCD_ShowString(20 + 8 * 19, 30, &tmp[0]);
			POINT_COLOR = GREEN;		//笔画为绿色
			LCD_ShowString(20 + 8 * 20 + 8 * 3, 30, "s");
		}
	}
}

// 4.控制LED界面
void controllLED() {
	TIM_Cmd(TIM3, ENABLE); // 开启定时器3
	LCD_Clear(BLACK); //清屏 
	POINT_COLOR = RED;		//笔画为红色	
	LCD_Show_Chinese16x16(280 - 16 * 3, 40, "操作说明");
	POINT_COLOR = GREEN;		//笔画为绿色	
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
	R_LED_OFF;G_LED_OFF;Y_LED_OFF;  //先关掉所有的LED
	while (1) {

		while (!(keyTemp = KEY_Scan(0))); //阻塞，等待按键按下并将按键值赋给keyTemp

		if (keyTemp == key1) {  //按下Key1退出循环，即退出控制LED界面
			break;
		}

		else if (keyTemp == key2) {  //按下Key2黄色LED亮，松开熄灭
			Y_LED_ON;
			while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == 0); //阻塞，等待按键松开
			Y_LED_OFF;
		}

		// 方法一：采用延迟的方法。缺点是当用户按下的瞬间，有可能刚好处于延迟的时间段内，导致按键无效
		//        而且延时的时间越长，当按下瞬间设备处于延时状态的概率越大
		else if (keyTemp == key3) { //按下Key3绿色LED闪烁
			do {
				G_LED_ON;
				delay_ms(100);
				G_LED_OFF;
				delay_ms(100);
			} while (KEY_Scan(0) != key3);  //再次按下key3关闭
		}

		// 方法二：采用定时器的方法，计算时间差。可以解决方法一遇到的问题
		else if (keyTemp == key4) { //按下Key4红色LED闪烁 
			R_LED_OFF;
			currentTimeFlag = Count_timer;  // 标记当前时间戳
			while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 0); //阻塞，等待用户松开按键
			do {
				if (Count_timer - currentTimeFlag >= 2) {  // 200ms
					PCout(13) = !PCout(13);  // 翻转红色LED
					currentTimeFlag = Count_timer;  // 重新标记当前时间戳
				}
			} while (KEY_Scan(0) != key4);
			R_LED_OFF;
		}

		else if (keyTemp == key5) { //按下key5
			// 不做任何操作
		}
	}
	TIM_Cmd(TIM3, DISABLE); // 关闭定时器3
}

// 5.nRF24L01界面
void nRF24L01() {
	NRF24L01_Init(); // 初始化NRF24L01
	LCD_Clear(BLACK); //清屏
	TIM_Cmd(TIM3, ENABLE); // 关闭定时器3
	printf("NRF24L01 主机模式\r\n");
	printf("本机地址:");
	HexToStr(MASTER_ADDRESS, MASTER_ADDRESS_STRING, 5, 1);
	printf("%s", MASTER_ADDRESS_STRING);
	while (1) { // 进入配对模式
		char rx_buf[33] = { 0 };
		NRF24L01_RX_Mode(MASTER_ADDRESS);
		if (NRF24L01_RxPacket(rx_buf) == 0) { //一旦接收到信息,则显示出来.
			rx_buf[32] = 0;//加入字符串结束符
			printf("\r\n收到消息:");
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
			LCD_Clear(BLACK); //清屏
			LCD_ShowString(1, 1, "H:");
			LCD_ShowString(1, 8 * 3, RX_ADDRESS_STRING);
			GetNextTXAddress(); // 获取下一个目标地址
			HexToStr(TX_ADDRESS, TX_ADDRESS_STRING, 5, 1);  //将目标地址转换为字符串
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
				tx_buf[32] = 0;//加入结束符	
			}
			tx_buf[0] = 0;

			currentTimeFlag = Count_timer; // 标记当前时间,换回接收模式
			R_LED = 1;
			NRF24L01_RX_Mode(RX_ADDRESS);

			while (1) {   // 等待回复 5s // 按下key1退出
				if (NRF24L01_RxPacket(rx_buf) == 0) { //一旦接收到信息,则显示出来.
					rx_buf[32] = 0;//加入字符串结束符
					printf("\r\n收到消息:");
					printf("%s", rx_buf);
					LCD_ShowString(3, 1, "R:");
					LCD_ShowString(3, 8 * 3, rx_buf);
					rx_buf[0] = 0; // 清空接收缓冲区
				}

				if (Count_timer - currentTimeFlag > 50) {
					R_LED = 0;
					break;// 5s后将不再等待，退出接收模式继续发送下一个设备
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
	TIM_Cmd(TIM3, DISABLE); // 关闭定时器3
}

void rc522() {
	TIM_Cmd(TIM3, ENABLE);
	RC522_Init(); //RC522初始化
	LCD_Clear(BLACK); //清屏
	is_send_breath = 1;
	LCD_DrawRectangle(0, 0, 320, 240);	//画矩形框	
	// 在第二行提示“请刷卡”
	POINT_COLOR = YELLOW;
	LCD_Show_Chinese16x16(140, 10, "刷卡或输入密码");
	POINT_COLOR = GREEN;
	LCD_ShowString(6, 5, "K1>");
	LCD_Show_Chinese16x16(6 + 8 * 3, 5, "返回");

	LCD_ShowString(6, 190, "K5>");
	LCD_Show_Chinese16x16(6 + 8 * 3, 190, "关门");

	/// 按键扫描
	while (1) {
		while (!(keyTemp = KEY_Scan(0))) {//阻塞，用户按下之前，一直寻卡
			if (open_flag == 1) {
				DOOR_OPEN = 1;// 开门
				open_flag = 0;
			}
			if (open_flag == 2) {
				DOOR_OPEN = 0;//关门
				open_flag = 0;
			}
			if (read_card_data() == MI_OK) {
				lockCount = 0;
				u8 card_id[20] = { 0 };
				for (int i = 0;i < 4;i++) {
					sprintf(&card_id[i * 2], "%02X", SN[i]);
				}
				UART5TxData(card_id);
				LCD_Show_Chinese16x16(120, 60, "卡号");
				LCD_ShowString(120 + 16 * 2, 60, ":");
				LCD_ShowString(120 + 16 * 2 + 8, 60, card_id);
				NB_Send_buf[0] = 0;

				// 将卡号上传至电信云平台
				strcpy((char*)NB_Send_buf, "AT+QLWDATASEND=");
				DealUpData(card_id, &NB_Send_buf[15], 22);
				USART2TxStr((char*)NB_Send_buf);
				while (1) {
					currentTimeFlag = Count_timer;
					while (Count_timer < currentTimeFlag + 100) {
						LCD_LoadBar(120, 100, 100, 20, Count_timer - currentTimeFlag);
						if (Count_timer - currentTimeFlag > 100) {//超过10s或者开门
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
					DOOR_OPEN = 1; //开门
					LCD_Show_Chinese16x16(120, 40, "欢迎您");
					LCD_ShowString(120 + 16 * 3, 40, ":");
					LCD_Show_Chinese16x16(120 + 16 * 4, 40, "陆广兴");
					POINT_COLOR = MAGENTA;
					G_LED = 1;
					LCD_Show_Chinese16x16(140, 100, "已开门");//提示已开门
					while (keyTemp = KEY_Scan(0) != key5); //等待用户关门
					DOOR_OPEN = 0;//关门
					LCD_Show_Chinese16x16(140, 100, "已关门");//提示已关门
					POINT_COLOR = GREEN;

					delay_ms(1000);
					G_LED = 0;
				}
				if (open_flag == 2) {
					POINT_COLOR = RED;
					LCD_Show_Chinese16x16(140, 100, "注册");//提示已开门
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

		if (keyTemp == key1) {  //按下Key1退出循环，即退出RC522模块
			break;
		}

		else if (keyTemp == key2) {  //按下Key2黄色LED亮，松开熄灭

		}

		// 方法一：采用延迟的方法。缺点是当用户按下的瞬间，有可能刚好处于延迟的时间段内，导致按键无效
		//        而且延时的时间越长，当按下瞬间设备处于延时状态的概率越大
		else if (keyTemp == key3) { //按下Key3绿色LED闪烁
			do {
				G_LED_ON;
				delay_ms(100);
				G_LED_OFF;
				delay_ms(100);
			} while (KEY_Scan(0) != key3);  //再次按下key3关闭
		}

		// 方法二：采用定时器的方法，计算时间差。可以解决方法一遇到的问题
		else if (keyTemp == key4) { //按下Key4红色LED闪烁 
			R_LED_OFF;
			currentTimeFlag = Count_timer;  // 标记当前时间戳
			while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 0); //阻塞，等待用户松开按键
			do {
				if (Count_timer - currentTimeFlag >= 2) {  // 200ms
					PCout(13) = !PCout(13);  // 翻转红色LED
					currentTimeFlag = Count_timer;  // 重新标记当前时间戳
				}
			} while (KEY_Scan(0) != key4);
			R_LED_OFF;
		}

		else if (keyTemp == key5) { //按下key5
			// 不做任何操作
		}
	}
	is_send_breath = 0;
	TIM_Cmd(TIM3, DISABLE);  //失能TIM3	 DISABLE
}

/**
 * 16进制转字符串,不带0x,不带空格，isCover是否覆盖 1 覆盖 0 不覆盖
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
 * 获取下一个目标地址和设备类型 更新TX_ADDRESS,DEVICE_TYPE
*/
void GetNextTXAddress(void) {
	for (int j = 0;j < 5;j++) {
		char tmp_buff[3] = { 0 };
		TX_ADDRESS[j] = TX_ADDRESS_LIST[TX_ADDRESS_INDEX][j];
	}
	for (int i = 5;i < 10;i++) {
		DEVICE_TYPE[i - 5] = TX_ADDRESS_LIST[TX_ADDRESS_INDEX][i];
	}
	// 更新索引
	TX_ADDRESS_INDEX++;
	TX_ADDRESS_INDEX = TX_ADDRESS_INDEX % TX_ADDRESS_LIST_LEN;
}

/**
 * 添加一个目标地址到列表,字符串格式的16进制地址,数据帧格式: 如湿度检测设备:3443101001Humid
*/
void AddTXAddressToList(u8* address) {
	TX_ADDRESS_LIST_LEN++;
	//设备地址
	for (int i = 0;i < 5;i++) {
		char tmp[3] = { 0 };
		tmp[0] = address[i * 2];
		tmp[1] = address[i * 2 + 1];
		TX_ADDRESS_LIST[TX_ADDRESS_LIST_LEN - 1][i] = strtol(tmp, NULL, 16);
	}
	//设备类型
	for (int i = 5;i < 10;i++) {
		TX_ADDRESS_LIST[TX_ADDRESS_LIST_LEN - 1][i] = address[i];
	}
}

void dataReceive(u8* data, u8 len) {
	// 数据帧:+QLWDATARECV: 19,1,0,34,061F570035001B000B4C754775616E6778696E67000B313737353835343038333454
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
	if (mainMenuCount < 6) {  // 前6个菜单放在左边
		LCD_Fill(30, 30 + 16 * ((mainMenuCount % 6) * 2), 30 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD显示黄色方块
		LCD_ShowString(30 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	else {
		LCD_Fill(170, 30 + 16 * ((mainMenuCount % 6) * 2), 170 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD显示黄色方块
		LCD_ShowString(170 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	mainMenuCount++;
}

void AddMeunChineseItem(char* item) {
	if (mainMenuCount < 6) {  // 前6个菜单放在左边
		LCD_Fill(30, 30 + 16 * ((mainMenuCount % 6) * 2), 30 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD显示黄色方块
		LCD_Show_Chinese16x16(30 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	else {
		LCD_Fill(170, 30 + 16 * ((mainMenuCount % 6) * 2), 170 + 16, 30 + 16 * ((mainMenuCount % 6) * 2 + 1), YELLOW);  //LCD显示黄色方块
		LCD_Show_Chinese16x16(170 + 20, 30 + 16 * ((mainMenuCount % 6) * 2), item);
	}
	mainMenuCount++;
}