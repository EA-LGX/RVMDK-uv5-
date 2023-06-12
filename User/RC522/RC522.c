#include "sys.h"
#include "RC522.h"
#include "delay.h"
#include "string.h"
#include "usart.h"

#define EN 1
#define CN 0
unsigned char SelectedSnr[4];
unsigned char SN[4]; //卡号

void print_info(unsigned char* p, int cnt) {
	int i;
	for (i = 0;i < cnt;i++) {
		printf("0x%X ", p[i]);
	}
	printf("\r\n");
}

/**********************************************
RCC522工作流程
RCC522初始化
寻卡
if(寻卡 == yes) 防冲撞
选卡
验证卡片
读卡片/写卡片
结束卡片操作

u8   req_code; //寻卡模式
u8 *pTagType;  //卡类型。


参数说明: req_code[IN]:寻卡方式
				0x52 = 寻感应区内所有符合14443A标准的卡
				0x26 = 寻未进入休眠状态的卡

					pTagType[OUT]：卡片类型代码
				0x4400 = Mifare_UltraLight
				0x0400 = Mifare_One(S50)
				0x0200 = Mifare_One(S70)
				0x0800 = Mifare_Pro(X)
				0x4403 = Mifare_DESFire
*/

unsigned char KEY[14][6] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
	{0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
	{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
	{0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD},
	{0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A},
	{0x71, 0x4C, 0x5C, 0x88, 0x6E, 0x97},
	{0x58, 0x7E, 0xE5, 0xF9, 0x35, 0x0F},
	{0xA0, 0x47, 0x8C, 0xC3, 0x90, 0x91},
	{0x53, 0x3C, 0xB6, 0xC7, 0x23, 0xf6},
	{0x8F, 0xD0, 0xA4, 0xF2, 0x56, 0xE9},
	{0x66, 0x55, 0x44, 0x11, 0x22, 0x33},
	{0x66, 0x55, 0x44, 0x33, 0x22, 0x11}
};

int read_card() {
	unsigned char CT[2];//卡类型
	u8 status = 1;
	status = RC522_PcdRequest(PICC_REQIDL, CT);//(寻卡模式，卡类型),成功返回0
	if (status == MI_OK)//寻卡成功
	{
		status = MI_ERR;
		status = RC522_PcdAnticoll(SN);  //防冲撞，成功返回0，SN是读到卡号的地址
		printf("卡类型:");
		print_info(CT, 2);//打印类型
		printf("卡号:");
		print_info(SN, 4);//打印卡号
	}
	if (status == MI_OK) {
		status = MI_ERR;
		status = RC522_PcdSelect(SN);	//选定卡片，SN：卡的序列号
	}
	return status;
}


/*
功能： 修改卡密码
参数：
u8   auth_mode ：验证密码的类型。（分为PICC_AUTHENT1A和PICC_AUTHENT1B）
u8   addr      ：密码存放的地址。（密码放在每个扇区的第3个块。第一个扇区比较特殊。）
				 一般填：3、7、11.....规律
u8 *Src_Key    ：原密码
u8 *New_Key    ：新密码
u8 *pSnr       ：卡号

密码格式：
u8 KEY[6]={0xff,0xff,0xff,0xff,0xff,0xff}; //验证A或者B密码的格式
u8 MIMA_1[16]={88,88,88,88,88,88,0xff,0x07,0x80,0x29,88,88,88,88,88,88}; //修改新密码的格式(A密码 控制位  B密码 )
*/
int card_passworld(u8 auth_mode, u8 addr, u8* Src_Key, u8* New_Key, u8* pSnr) {
	int status;
	/*1. 寻卡*/
	status = read_card();
	/*2. 验证卡密码*/
	if (status == MI_OK) {
		status = RC522_PcdAuthState(auth_mode, addr, Src_Key, pSnr);   //验证卡片密码 形参参数：验证方式，块地址，密码，卡许列号
	}
	/*3. 写数据到卡*/
	if (status == MI_OK) {
		status = RC522_PcdWrite(addr, New_Key); //写数据到第addr块，New_Key写入的数据值。
	}
	return status;
}


/*
功能：写数据到指定块
参数：
u8   addr      ：数据存放的地址。每个扇区的0、1、2块是存放数据。3是存放密码。
				一般填：0、1、2 、4、5、6、8、9、10
数据一般格式：u8 SJ[16]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255}; //写入的金额；

*/
u8 write_card_data(u8* data) {
	u8 KEY[6] = { 0xff,0xff,0xff,0xff,0xff,0xff }; //卡密码-初始密码--白卡的出厂密码--

	int status = MI_ERR;

	/*1. 寻卡*/
	status = read_card();

	/*2. 验证卡密码*/
	if (status == MI_OK) {
		status = RC522_PcdAuthState(PICC_AUTHENT1A, 3, KEY, SN);   //验证卡片密码 形参参数：验证方式，块地址，密码，卡许列号
	}

	/*3. 写数据到卡*/
	if (status == MI_OK) {
		status = RC522_PcdWrite(2, data); //写数据到第addr块，data入的数据值。
	}
	if (status == MI_OK) {
		printf("write data\r\n");
		print_info(data, 16);
	}
	return status;
}


/*
功能：读数据到指定块
参数：
u8   auth_mode ：验证密码的类型。（分为PICC_AUTHENT1A和PICC_AUTHENT1B）
u8   addr      ：数据存放的地址。每个扇区的0、1、2块是存放数据。3是存放密码。
			   一般填：0、1、2 、4、5、6、8、9、10
u8 *Src_Key    ：原密码
u8 *data       ：读出的数据
u8 *pSnr       ：卡号

数据一般格式：u8 SJ[16]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255}; //写入的金额；

*/
u8 read_card_data() {
	u8 KEY[6] = { 0xff,0xff,0xff,0xff,0xff,0xff }; //卡密码-初始密码--白卡的出厂密码--
	char status;
	u8 data[16];
	/*1. 寻卡*/
	status = read_card();
	/*2. 验证卡密码*/
	if (status == MI_OK) {
		status = RC522_PcdAuthState(PICC_AUTHENT1A, 3, KEY, SN);   //验证卡片密码 形参参数：验证方式，块地址，密码，卡许列号
	}
	//printf("RC522_PcdAuthState=%s",status);
	//USART1TxChar(status);
	 /*3. 读出数据*/
	if (status == MI_OK) {
		status = RC522_PcdRead(2, data); //从第addr块读出数据值。
	}
	if (status == MI_OK) {
		printf("read data\r\n");
		print_info(data, 16);
	}
	return status;
}

/*
函数功能：移植接口--SPI时序读写一个字节
函数参数：data:要写入的数据
返 回 值：读到的数据
*/
u8 RC522_SPI_ReadWriteOneByte(u8 tx_data) {
	u8 rx_data = 0;
	u8 i;
	for (i = 0;i < 8;i++) {
		RC522_SCLK = 0;
		if (tx_data & 0x80) { RC522_OUTPUT = 1; }
		else { RC522_OUTPUT = 0; }
		tx_data <<= 1;
		RC522_SCLK = 1;
		rx_data <<= 1;
		if (RC522_INPUT)rx_data |= 0x01;
	}
	return rx_data;
}

/*
函数功能：初始化RC522的IO口
*/

/*
RC522射频模块外部的接口:
*1--SDA <----->PC6--片选脚
*2--SCK <----->PC7--时钟线
*3--MOSI<----->PC8--输出
*4--MISO<----->PB6--输入
*5--悬空
*6--GND <----->GND
*7--RST <----->PC9--复位脚
*8--VCC <----->VCC
*/

void RC522_IO_Init(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);        //使能PC端口时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);        //使能PB端口时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     //IO口速度为50MHz
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9);  //拉高

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;      //上拉输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      //上拉输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}


/*
功能描述:选卡读取卡存储器容量
输入参数:serNum 传入卡序列号
返 回 值:成功返回卡容量
*/
u8 RC522_MFRC522_SelectTag(u8* serNum) //读取卡存储器容量
{
	u8 i;
	u8 status;
	u8 size;
	u8 recvBits;
	u8 buffer[9];

	buffer[0] = PICC_ANTICOLL1;	  //防撞码1     
	buffer[1] = 0x70;
	buffer[6] = 0x00;
	for (i = 0;i < 4;i++) {
		buffer[i + 2] = *(serNum + i);	//buffer[2]-buffer[5]为卡序列号
		buffer[6] ^= *(serNum + i);	  //卡校验码
	}

	RC522_CalulateCRC(buffer, 7, &buffer[7]);	//buffer[7]-buffer[8]为RCR校验码
	RC522_ClearBitMask(Status2Reg, 0x08);
	status = RC522_PcdComMF522(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);

	if ((status == MI_OK) && (recvBits == 0x18))
		size = buffer[0];
	else
		size = 0;

	return size;
}


/*
延时函数，纳秒级
*/
void RC522_Delay(u32 ns) {
	u32 i;
	for (i = 0;i < ns;i++) {
		__nop();
		__nop();
		__nop();
	}
}


/*
函数功能：RC522芯片初始化
*/
void RC522_Init(void) {
	RC522_IO_Init();	//RC522初始化
	RC522_PcdReset();  			//复位RC522 
	RC522_PcdAntennaOff();	//关闭天线
	delay_ms(2);  		  //延时2毫秒
	RC522_PcdAntennaOn();		//开启天线
	M500PcdConfigISOType('A'); //设置RC632的工作方式
}


/*
函数功能：复位RC522
*/
void RC522_Reset(void) {
	RC522_PcdReset();				//复位RC522
	RC522_PcdAntennaOff();	//关闭天线
	delay_ms(2);  		  //延时2毫秒
	RC522_PcdAntennaOn();		//开启天线  	
}


/*
功    能: 寻卡
参数说明: req_code[IN]:寻卡方式
				0x52   = 寻感应区内所有符合14443A标准的卡
				0x26   = 寻未进入休眠状态的卡
					pTagType[OUT]:卡片类型代码
				0x4400 = Mifare_UltraLight
				0x0400 = Mifare_One(S50)
				0x0200 = Mifare_One(S70)
				0x0800 = Mifare_Pro(X)
				0x4403 = Mifare_DESFire
返 回 值: 成功返回MI_OK
*/
char RC522_PcdRequest(u8 req_code, u8* pTagType) {
	char status;
	u8 unLen;
	u8 ucComMF522Buf[MAXRLEN];  	   // MAXRLEN  18

	RC522_ClearBitMask(Status2Reg, 0x08);	//清RC522寄存器位,/接收数据命令
	RC522_WriteRawRC(BitFramingReg, 0x07); //写RC632寄存器
	RC522_SetBitMask(TxControlReg, 0x03);  //置RC522寄存器位

	ucComMF522Buf[0] = req_code; 	    //寻卡方式

	status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen); //通过RC522和ISO14443卡通讯

	if ((status == MI_OK) && (unLen == 0x10)) {
		*pTagType = ucComMF522Buf[0];
		*(pTagType + 1) = ucComMF522Buf[1];
	}
	else {
		status = MI_ERR;
	}
	return status;
}


/*
功    能: 防冲撞
参数说明: pSnr[OUT]:卡片序列号，4字节
返    回: 成功返回MI_OK
*/
char RC522_PcdAnticoll(u8* pSnr) {
	char status;
	u8 i, snr_check = 0;
	u8 unLen;
	u8 ucComMF522Buf[MAXRLEN];

	RC522_ClearBitMask(Status2Reg, 0x08);  //清RC522寄存器位 
	RC522_WriteRawRC(BitFramingReg, 0x00); //写
	RC522_ClearBitMask(CollReg, 0x80);     //清

	ucComMF522Buf[0] = PICC_ANTICOLL1;   //PICC_ANTICOLL1 = 0x93
	ucComMF522Buf[1] = 0x20;

	status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen); //0x0c,通过RC522和ISO14443卡通讯
	//PCD_TRANSCEIVE =发送并接收数据
	//2：写入卡里的数据字节长度
	//ucComMF522Buf:存放数据的地址
	//unLen：从卡里读出的数据长度
	if (status == MI_OK) {
		for (i = 0;i < 4;i++) {
			*(pSnr + i) = ucComMF522Buf[i];  //把读到的卡号赋值给pSnr
			snr_check ^= ucComMF522Buf[i];
		}
		if (snr_check != ucComMF522Buf[i]) {
			status = MI_ERR;
		}
	}
	RC522_SetBitMask(CollReg, 0x80);
	return status;
}


/*
功    能：选定卡片
参数说明：pSnr[IN]:卡片序列号，4字节
返    回：成功返回MI_OK
*/
char RC522_PcdSelect(u8* pSnr) {
	char status;
	u8 i;
	u8 unLen;
	u8 ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = PICC_ANTICOLL1;
	ucComMF522Buf[1] = 0x70;
	ucComMF522Buf[6] = 0;

	for (i = 0;i < 4;i++) {
		ucComMF522Buf[i + 2] = *(pSnr + i);
		ucComMF522Buf[6] ^= *(pSnr + i);
	}

	RC522_CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]); //用MF522计算CRC16函数，校验数据
	RC522_ClearBitMask(Status2Reg, 0x08);	                //清RC522寄存器位
	status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);
	if ((status == MI_OK) && (unLen == 0x18))status = MI_OK;
	else status = MI_ERR;

	return status;
}


/*
功    能：验证卡片密码
参数说明：auth_mode[IN]: 密码验证模式
				 0x60 = 验证A密钥
				 0x61 = 验证B密钥
		  addr[IN]：块地址
		  pKey[IN]：扇区密码
		  pSnr[IN]：卡片序列号，4字节
返    回：成功返回MI_OK
*/
char RC522_PcdAuthState(u8 auth_mode, u8 addr, u8* pKey, u8* pSnr) {
	char status;
	u8 unLen;
	u8 ucComMF522Buf[MAXRLEN];  //MAXRLEN  18(数组的大小)

	//验证模式+块地址+扇区密码+卡序列号   
	ucComMF522Buf[0] = auth_mode;
	ucComMF522Buf[1] = addr;
	memcpy(&ucComMF522Buf[2], pKey, 6); //拷贝，复制
	memcpy(&ucComMF522Buf[8], pSnr, 4);

	status = RC522_PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);
	if ((status != MI_OK) || (!(RC522_ReadRawRC(Status2Reg) & 0x08)))status = MI_ERR;
	return status;
}


/*
功    能：读取M1卡一块数据
参数说明：
					addr：块地址
		  p   ：读出的块数据，16字节
返    回：成功返回MI_OK
*/
char RC522_PcdRead(u8 addr, u8* p) {
	char status;
	u8 unLen;
	u8 i, ucComMF522Buf[MAXRLEN]; //18

	ucComMF522Buf[0] = PICC_READ;
	ucComMF522Buf[1] = addr;
	RC522_CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
	status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);//通过RC522和ISO14443卡通讯
	if ((status == MI_OK && (unLen == 0x90))) {
		for (i = 0;i < 16;i++) {
			*(p + i) = ucComMF522Buf[i];
		}
	}
	else {
		status = MI_ERR;
	}
	return status;
}


/*
功    能：写数据到M1卡指定块
参数说明：addr：块地址
		  p   ：向块写入的数据，16字节
返    回：成功返回MI_OK
*/
char RC522_PcdWrite(u8 addr, u8* p) {
	char status;
	u8 unLen;
	u8 i, ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = PICC_WRITE;// 0xA0 //写块
	ucComMF522Buf[1] = addr;      //块地址
	RC522_CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

	status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

	if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {
		status = MI_ERR;
	}

	if (status == MI_OK) {
		for (i = 0;i < 16;i++)//向FIFO写16Byte数据 
		{
			ucComMF522Buf[i] = *(p + i);
		}
		RC522_CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);
		status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);
		if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {
			status = MI_ERR;
		}
	}
	return status;
}


/*
功    能：命令卡片进入休眠状态
返    回：成功返回MI_OK
*/
char RC522_PcdHalt(void) {
	u8 status;
	u8 unLen;
	u8 ucComMF522Buf[MAXRLEN]; //MAXRLEN==18
	status = status;
	ucComMF522Buf[0] = PICC_HALT;
	ucComMF522Buf[1] = 0;
	RC522_CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
	status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
	return MI_OK;
}


/*
功    能：用MF522计算CRC16函数
参    数：
				*pIn ：要读数CRC的数据
				len：-数据长度
				*pOut：计算的CRC结果
*/
void RC522_CalulateCRC(u8* pIn, u8 len, u8* pOut) {
	u8 i, n;
	RC522_ClearBitMask(DivIrqReg, 0x04);  //CRCIrq = 0  
	RC522_WriteRawRC(CommandReg, PCD_IDLE);
	RC522_SetBitMask(FIFOLevelReg, 0x80); //清FIFO指针

	//向FIFO中写入数据  
	for (i = 0;i < len;i++) {
		RC522_WriteRawRC(FIFODataReg, *(pIn + i));  //开始RCR计算
	}

	RC522_WriteRawRC(CommandReg, PCD_CALCCRC);   //等待CRC计算完成 
	i = 0xFF;
	do {
		n = RC522_ReadRawRC(DivIrqReg);
		i--;
	} while ((i != 0) && !(n & 0x04));//CRCIrq = 1

	//读取CRC计算结果 
	pOut[0] = RC522_ReadRawRC(CRCResultRegL);
	pOut[1] = RC522_ReadRawRC(CRCResultRegM);
}


/*
功    能：复位RC522
返    回：成功返回MI_OK
*/
char RC522_PcdReset(void) {
	RC522_RST = 1;   //PF1写1
	RC522_Delay(10);
	RC522_RST = 0;   //PF1清0
	RC522_Delay(10);
	RC522_RST = 1;	 //PF1写1
	RC522_Delay(10);
	RC522_WriteRawRC(CommandReg, PCD_RESETPHASE);  //写RC632寄存器，复位
	RC522_WriteRawRC(CommandReg, PCD_RESETPHASE);	//写RC632寄存器，复位
	RC522_Delay(10);

	RC522_WriteRawRC(ModeReg, 0x3D);             //和Mifare卡通讯，CRC初始值0x6363
	RC522_WriteRawRC(TReloadRegL, 30);           //写RC632寄存器   
	RC522_WriteRawRC(TReloadRegH, 0);
	RC522_WriteRawRC(TModeReg, 0x8D);
	RC522_WriteRawRC(TPrescalerReg, 0x3E);

	RC522_WriteRawRC(TxAutoReg, 0x40);//必须要
	return MI_OK;
}


/*
函数功能：设置RC632的工作方式
*/
char M500PcdConfigISOType(u8 type) {
	if (type == 'A')                        //ISO14443_A
	{
		RC522_ClearBitMask(Status2Reg, 0x08);     //清RC522寄存器位
		RC522_WriteRawRC(ModeReg, 0x3D);          //3F//CRC初始值0x6363
		RC522_WriteRawRC(RxSelReg, 0x86);         //84
		RC522_WriteRawRC(RFCfgReg, 0x7F);         //4F  //调整卡的感应距离//RxGain = 48dB调节卡感应距离  
		RC522_WriteRawRC(TReloadRegL, 30);        //tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
		RC522_WriteRawRC(TReloadRegH, 0);
		RC522_WriteRawRC(TModeReg, 0x8D);
		RC522_WriteRawRC(TPrescalerReg, 0x3E);
		RC522_Delay(1000);
		RC522_PcdAntennaOn();		//开启天线 
	}
	else return 1;       //失败，返回1
	return MI_OK;				//成功返回0
}


/*
功    能：读RC632寄存器
参数说明：Address[IN]:寄存器地址
返    回：读出的值
*/
u8 RC522_ReadRawRC(u8 Address) {
	u8 ucAddr;
	u8 ucResult = 0;
	RC522_CS = 0;						//片选选中RC522
	ucAddr = ((Address << 1) & 0x7E) | 0x80;
	RC522_SPI_ReadWriteOneByte(ucAddr);		  //发送命令
	ucResult = RC522_SPI_ReadWriteOneByte(0); //读取RC522返回的数据
	RC522_CS = 1;						   //释放片选线(PF0)
	return ucResult;         //返回读到的数据
}


/*
功    能：写RC632寄存器
参数说明：Address[IN]:寄存器地址
		  value[IN] :写入的值
*/
void RC522_WriteRawRC(u8 Address, u8 value) {
	u8 ucAddr;
	RC522_CS = 0; //PF0写 0 (SDA)(SPI1片选线，低电平有效)
	ucAddr = ((Address << 1) & 0x7E);
	RC522_SPI_ReadWriteOneByte(ucAddr); //SPI1发送一个字节
	RC522_SPI_ReadWriteOneByte(value);  //SPI1发送一个字节
	RC522_CS = 1;										      //PF1写1(SDA)(SPI1片选线)
}


/*
功    能：置RC522寄存器位
参数说明：reg[IN]:寄存器地址
		  mask[IN]:置位值
*/
void RC522_SetBitMask(u8 reg, u8 mask) {
	char tmp = 0x0;
	tmp = RC522_ReadRawRC(reg);					//读RC632寄存器
	RC522_WriteRawRC(reg, tmp | mask);   //写RC632寄存器
}


/*
功    能：清RC522寄存器位
参数说明：reg[IN]:寄存器地址
		 mask[IN]:清位值
*/
void RC522_ClearBitMask(u8 reg, u8 mask) {
	char tmp = 0x0;
	tmp = RC522_ReadRawRC(reg);        //读RC632寄存器
	RC522_WriteRawRC(reg, tmp & ~mask); // clear bit mask
}


/*
功    能：通过RC522和ISO14443卡通讯
参数说明：Command[IN]:RC522命令字
		  pIn [IN]:通过RC522发送到卡片的数据
		  InLenByte[IN]:发送数据的字节长度
		  pOut [OUT]:接收到的卡片返回数据
		  *pOutLenBit[OUT]:返回数据的位长度
*/
char RC522_PcdComMF522(u8 Command, u8* pIn, u8 InLenByte, u8* pOut, u8* pOutLenBit) {
	char status = MI_ERR;
	u8 irqEn = 0x00;
	u8 waitFor = 0x00;
	u8 lastBits;
	u8 n;
	u16 i;

	switch (Command) {
	case PCD_AUTHENT:    //验证密钥
		irqEn = 0x12;
		waitFor = 0x10;
		break;
	case PCD_TRANSCEIVE: //发送并接收数据
		irqEn = 0x77;
		waitFor = 0x30;
		break;
	default:
		break;
	}
	RC522_WriteRawRC(ComIEnReg, irqEn | 0x80);
	RC522_ClearBitMask(ComIrqReg, 0x80);			//清所有中断位
	RC522_WriteRawRC(CommandReg, PCD_IDLE);
	RC522_SetBitMask(FIFOLevelReg, 0x80);	 	//清FIFO缓存

	for (i = 0;i < InLenByte;i++) {
		RC522_WriteRawRC(FIFODataReg, pIn[i]);
	}

	RC522_WriteRawRC(CommandReg, Command);
	if (Command == PCD_TRANSCEIVE) {
		RC522_SetBitMask(BitFramingReg, 0x80);	 //开始传送
	}

	//有问题，下面的循环
//i = 600;//根据时钟频率调整，操作M1卡最大等待时间25ms
	i = 2000;
	do {
		n = RC522_ReadRawRC(ComIrqReg);
		i--;
	} while ((i != 0) && !(n & 0x01) && !(n & waitFor));

	RC522_ClearBitMask(BitFramingReg, 0x80);
	if (i != 0) {
		if (!(RC522_ReadRawRC(ErrorReg) & 0x1B)) {
			status = MI_OK;
			if (n & irqEn & 0x01) {
				status = MI_NOTAGERR;
			}
			if (Command == PCD_TRANSCEIVE) {
				n = RC522_ReadRawRC(FIFOLevelReg);
				lastBits = RC522_ReadRawRC(ControlReg) & 0x07;
				if (lastBits) {
					*pOutLenBit = (n - 1) * 8 + lastBits;
				}
				else {
					*pOutLenBit = n * 8;
				}

				if (n == 0)n = 1;
				if (n > MAXRLEN)n = MAXRLEN;
				for (i = 0; i < n; i++) {
					pOut[i] = RC522_ReadRawRC(FIFODataReg);
				}
			}
		}
		else {
			status = MI_ERR;
		}
	}
	RC522_SetBitMask(ControlReg, 0x80);// stop timer now
	RC522_WriteRawRC(CommandReg, PCD_IDLE);
	return status;
}


/*
函数功能：开启天线
参    数：每次启动或关闭天险发射之间应至少有1ms的间隔
*/
void RC522_PcdAntennaOn(void) {
	u8 i;
	i = RC522_ReadRawRC(TxControlReg);
	if (!(i & 0x03)) {
		RC522_SetBitMask(TxControlReg, 0x03);
	}
}


/*
函数功能：关闭天线
参    数：每次启动或关闭天险发射之间应至少有1ms的间隔
*/
void RC522_PcdAntennaOff(void) {
	RC522_ClearBitMask(TxControlReg, 0x03); //清RC522寄存器位
}




//				
//					notarize = 1 读卡
//					


u8  DefaultKey[6] = { 0xff,0xff,0xff,0xff,0xff,0xff }; //默认密码 	


void  notarize_type1() {
	u8 status;
	u8 section = 0;
	u8 block = 0;
	unsigned char block_value[16];
	u8 KEY[6] = { 0xff,0xff,0xff,0xff,0xff,0xff }; //卡密码-初始密码--白卡的出厂密码--
	//u8 KEY[6]={0x00,0x00,0x00,0x00,0x00,0x0}; //卡密码-初始密码--白卡的出厂密码--
	u8 i = 0;
	status = read_card();
	if (!status) {
		if (EN)printf("function NO1 Read the card ...\r\n");
		if (CN)printf("功能1 读取卡片 \r\n");
		//if(OLED)GUI_ShowString(48,0,"NO_1",16,1);
		for (section = 0; section < 16; section++) {
			//printf("section:%d , section1:%d \r\n ",section,(section*4+3));
			status = RC522_PcdAuthState(PICC_AUTHENT1A, (section * 4 + 3), KEY, SN);   //验证卡片密码 形参参数：验证方式，块地址，密码，卡许列号

			if (!status) {
				for (block = 0; block < 4; block++) {
					status = RC522_PcdRead(0, block_value);  // 读卡，读取1扇区0块数据到buf[0]-buf[16]
					//status = RC522_PcdRead(2,block_value);  // 读卡，读取1扇区0块数据到buf[0]-buf[16

					if (EN)printf("SQ:%d , QK:%d     ", section, block);
					if (EN)printf("passA	");
					for (i = 0; i < 6; i++) {
						if (EN)printf("%X ", block_value[i]);
					}
					if (EN)printf("		Value	");
					for (i = 6; i < 10; i++) {
						if (EN)printf("%X ", block_value[i]);
					}
					if (EN)printf("		passB	");
					for (i = 10; i < 16; i++) {
						if (EN)printf("%X ", block_value[i]);
					}
					if (EN)printf("\r\n");
				}
			}
			else {
				if (EN)printf("wrong password \r\n");
				if (CN)printf("密码错误 \r\n");
				//if(OLED)GUI_ShowString(0,16,"5",16,1);
			}
		}
		if (EN)printf("\r\n");
		if (EN)printf("\r\n");
		if (EN)printf("\r\n");
	}
	else {
		if (EN)printf("Looking for failure \r\n");
		if (CN)printf("卡片未找到 \r\n");
		//if(OLED)GUI_ShowString(0,16,"4",16,1);
	}
}


//				
//					notarize = 3 常规密码测试
//					

void  notarize_type3() {
	u8 status;
	u8 section = 0;
	u8 block_num = 0;
	u8 KEY_num = 0;
	unsigned char block_value[16];

	u8 i = 0;
	status = read_card();
	if (!status) {
		if (EN)printf("function NO3 Password test ...\r\n");
		if (CN)printf("功能3 常规密码测试 \r\n");
		//if(OLED)GUI_ShowString(48,0,"NO_3",16,1);
		for (section = 0; section < 16; section++) {
			for (KEY_num = 0; KEY_num < 13; KEY_num++) {
				status = read_card();
				if (!status) {
					//printf("section:%d  KEY:%d %d %d %d %d %d   ",section,KEY[KEY_num][0],KEY[KEY_num][1],KEY[KEY_num][2],KEY[KEY_num][3],KEY[KEY_num][4],KEY[KEY_num][5]);
					status = RC522_PcdAuthState(PICC_AUTHENT1A, (section * 4 + 3), KEY[KEY_num], SN);// 校验1扇区密码，密码位于每一扇区第3块
					if (!status) {
						status = 1;
						//for(block_num = 0; block_num< 4 ; block_num++){

						status = RC522_PcdRead((section * 4 + 3), block_value);  // 读卡，读取1扇区0块数据到buf[0]-buf[16] 
						if (EN)printf("SQ:%d , QK:%d  KEY:%X %X %X %X %X %X   ", section, 3, KEY[KEY_num][0], KEY[KEY_num][1], KEY[KEY_num][2], KEY[KEY_num][3], KEY[KEY_num][4], KEY[KEY_num][5]);
						if (EN)printf("passA	");
						for (i = 0; i < 6; i++) {
							if (EN)printf("%X ", block_value[i]);
						}
						if (EN)printf("		Value	");
						for (i = 6; i < 10; i++) {
							if (EN)printf("%X ", block_value[i]);
						}
						if (EN)printf("		passB	");
						for (i = 10; i < 16; i++) {
							if (EN)printf("%X ", block_value[i]);
						}
						if (EN)printf("\r\n");

						//}	



					}
					else {
						if (EN)printf("wrong password \r\n");
					}
				}
				status = 1;
			}
		}
		if (EN)printf("\r\n");
		if (EN)printf("\r\n");
		if (EN)printf("\r\n");

	}
	else {
		if (EN)printf("Looking for failure \r\n");
		if (CN)printf("卡片未找到 \r\n");
		//if(OLED)GUI_ShowString(0,16,"4",16,1);
	}
}
