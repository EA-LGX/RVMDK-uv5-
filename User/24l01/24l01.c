#include "24l01.h"
#include "delay.h"
#include "usart.h"
#include "gpio.h"
#include "lcd.h"
#include "RC522.h"

// PC6 Ƭѡ PC7 SCK PC8 MOSI PC9 ��λ  PB6 MISO  CE PA0  IRQ PA1
//��ʼ��24L01��IO��
void NRF24L01_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOC, ENABLE);

	// PC6  CSN     PC7  SCK     PC8  MOSI    PC9  RST
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9);  //����

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;  // MISO:PB6
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;      //��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;   // CE:PA0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   //��������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //IRQ:PA1

	GPIO_SetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1);

	NRF24L01_CE = 0; 	//ʹ��24L01
	NRF24L01_CSN = 1;	//SPIƬѡȡ��	 	
}


//���24L01�Ƿ����
//����ֵ:0���ɹ�;1��ʧ��	
u8 NRF24L01_Check(void) {
	u8 buf[5] = { 0XA5,0XA5,0XA5,0XA5,0XA5 };
	u8 i;
	//SPI1_SetSpeed(SPI_BaudRatePrescaler_8); //spi�ٶ�Ϊ9Mhz��24L01�����SPIʱ��Ϊ10Mhz��   	 
	NRF24L01_Write_Buf(NRF_WRITE_REG + TX_ADDR, buf, 5);//д��5���ֽڵĵ�ַ.	
	NRF24L01_Read_Buf(TX_ADDR, buf, 5); //����д��ĵ�ַ  
	for (i = 0;i < 5;i++)if (buf[i] != 0XA5)break;
	if (i != 5)return 1;//���24L01����	
	return 0;		 //��⵽24L01
}

//SPIд�Ĵ���
//reg:ָ���Ĵ�����ַ
//value:д���ֵ
u8 NRF24L01_Write_Reg(u8 reg, u8 value) {
	u8 status;
	NRF24L01_CSN = 0;                 //ʹ��SPI����
	status = RC522_SPI_ReadWriteOneByte(reg);//���ͼĴ����� 
	RC522_SPI_ReadWriteOneByte(value);      //д��Ĵ�����ֵ
	NRF24L01_CSN = 1;                 //��ֹSPI����	   
	return(status);       			//����״ֵ̬
}

//��ȡSPI�Ĵ���ֵ
//reg:Ҫ���ļĴ���
u8 NRF24L01_Read_Reg(u8 reg) {
	u8 reg_val;
	NRF24L01_CSN = 0;          //ʹ��SPI����		
	RC522_SPI_ReadWriteOneByte(reg);   //���ͼĴ�����
	reg_val = RC522_SPI_ReadWriteOneByte(0XFF);//��ȡ�Ĵ�������
	NRF24L01_CSN = 1;          //��ֹSPI����		    
	return(reg_val);           //����״ֵ̬
}


//��ָ��λ�ö���ָ�����ȵ�����
//reg:�Ĵ���(λ��)
//*pBuf:����ָ��
//len:���ݳ���
//����ֵ,�˴ζ�����״̬�Ĵ���ֵ 
u8 NRF24L01_Read_Buf(u8 reg, u8* pBuf, u8 len) {
	u8 status, u8_ctr;
	NRF24L01_CSN = 0;           //ʹ��SPI����
	status = RC522_SPI_ReadWriteOneByte(reg);//���ͼĴ���ֵ(λ��),����ȡ״ֵ̬   	   
	for (u8_ctr = 0;u8_ctr < len;u8_ctr++)pBuf[u8_ctr] = RC522_SPI_ReadWriteOneByte(0XFF);//��������
	NRF24L01_CSN = 1;       //�ر�SPI����
	return status;        //���ض�����״ֵ̬
}

//��ָ��λ��дָ�����ȵ�����
//reg:�Ĵ���(λ��)
//*pBuf:����ָ��
//len:���ݳ���
//����ֵ,�˴ζ�����״̬�Ĵ���ֵ
u8 NRF24L01_Write_Buf(u8 reg, u8* pBuf, u8 len) {
	u8 status, u8_ctr;
	NRF24L01_CSN = 0;          //ʹ��SPI����
	status = RC522_SPI_ReadWriteOneByte(reg);//���ͼĴ���ֵ(λ��),����ȡ״ֵ̬
	for (u8_ctr = 0; u8_ctr < len; u8_ctr++)RC522_SPI_ReadWriteOneByte(*pBuf++); //д������	 
	NRF24L01_CSN = 1;       //�ر�SPI����
	return status;          //���ض�����״ֵ̬
}

//����NRF24L01����һ������
//txbuf:�����������׵�ַ
//����ֵ:�������״��
u8 NRF24L01_TxPacket(u8* txbuf) {
	u8 sta;
	//SPI1_SetSpeed(SPI_BaudRatePrescaler_8);//spi�ٶ�Ϊ9Mhz��24L01�����SPIʱ��Ϊ10Mhz��   
	NRF24L01_CE = 0;
	NRF24L01_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);//д���ݵ�TX BUF  32���ֽ�
	NRF24L01_CE = 1;//��������	   
	while (NRF24L01_IRQ != 0);//�ȴ��������
	sta = NRF24L01_Read_Reg(STATUS);  //��ȡ״̬�Ĵ�����ֵ	   
	NRF24L01_Write_Reg(NRF_WRITE_REG + STATUS, sta); //���TX_DS��MAX_RT�жϱ�־
	if (sta & MAX_TX)//�ﵽ����ط�����
	{
		NRF24L01_Write_Reg(FLUSH_TX, 0xff);//���TX FIFO�Ĵ��� 
		return MAX_TX;
	}
	if (sta & TX_OK)//�������
	{
		return TX_OK;
	}
	return 0xff;//����ԭ����ʧ��
}


//����NRF24L01����һ������
//txbuf:�����������׵�ַ
//����ֵ:0��������ɣ��������������
u8 NRF24L01_RxPacket(u8* rxbuf) {
	u8 sta;
	//SPI1_SetSpeed(SPI_BaudRatePrescaler_8); //spi�ٶ�Ϊ9Mhz��24L01�����SPIʱ��Ϊ10Mhz��   
	sta = NRF24L01_Read_Reg(STATUS);  //��ȡ״̬�Ĵ�����ֵ    	 
	NRF24L01_Write_Reg(NRF_WRITE_REG + STATUS, sta); //���TX_DS��MAX_RT�жϱ�־
	if (sta & RX_OK)//���յ�����
	{
		NRF24L01_Read_Buf(RD_RX_PLOAD, rxbuf, RX_PLOAD_WIDTH);//��ȡ����
		NRF24L01_Write_Reg(FLUSH_RX, 0xff);//���RX FIFO�Ĵ��� 
		return 0;
	}
	return 1;//û�յ��κ�����
}


//�ú�����ʼ��NRF24L01��RXģʽ
//����RX��ַ,дRX���ݿ��,ѡ��RFƵ��,�����ʺ�LNA HCURR
//��CE��ߺ�,������RXģʽ,�����Խ���������		   
void NRF24L01_RX_Mode(u8* RX_ADDRESS) {
	for (int i = 0; i < 5; i++) {
		R_LED = !R_LED;
		delay_ms(100);
	}
	NRF24L01_Init();    	//��ʼ��NRF24L01 
	while (NRF24L01_Check()) {//���NRF24L01�Ƿ���λ.	
		printf("NRF24L01 Error");
		LCD_ShowString(1, 1, "NRF24L01 Error");
	}
	NRF24L01_CE = 0;
	NRF24L01_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH);//дRX�ڵ��ַ

	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);    	//ʹ��ͨ��0���Զ�Ӧ��    
	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01);	//ʹ��ͨ��0�Ľ��յ�ַ  	 
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_CH, 40);	    	//����RFͨ��Ƶ��		  
	NRF24L01_Write_Reg(NRF_WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH);//ѡ��ͨ��0����Ч���ݿ�� 	    
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x0f);	//����TX�������,0db����,2Mbps,���������濪��   
	NRF24L01_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0f);		//���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ 
	NRF24L01_CE = 1; //CEΪ��,�������ģʽ 
}

//�ú�����ʼ��NRF24L01��TXģʽ  // _TX_ADDRESS:���͵�ַ 
//����TX��ַ,дTX���ݿ��,����RX�Զ�Ӧ��ĵ�ַ,���TX��������,ѡ��RFƵ��,�����ʺ�LNA HCURR
//PWR_UP,CRCʹ��
//��CE��ߺ�,������RXģʽ,�����Խ���������		   
//CEΪ�ߴ���10us,����������.	 
void NRF24L01_TX_Mode(u8* TX_ADDRESS, u8* RX_ADDRESS) {
	NRF24L01_Init();   //��ʼ��NRF24L01 
	while (NRF24L01_Check()) {	//���NRF24L01�Ƿ���λ.	
		printf("NRF24L01 Error");
		LCD_ShowString(1, 1, "NRF24L01 Error");
	}
	NRF24L01_CE = 0;
	NRF24L01_Write_Buf(NRF_WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);//дTX�ڵ��ַ 
	NRF24L01_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK	  

	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);  //ʹ��ͨ��0���Զ�Ӧ��    
	NRF24L01_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01); //ʹ��ͨ��0�Ľ��յ�ַ  
	NRF24L01_Write_Reg(NRF_WRITE_REG + SETUP_RETR, 0x1a);//�����Զ��ط����ʱ��:500us + 86us;����Զ��ط�����:10��
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_CH, 40);       //����RFͨ��Ϊ40
	NRF24L01_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x0f);  //����TX�������,0db����,2Mbps,���������濪��   
	NRF24L01_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0e);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�
	NRF24L01_CE = 1;//CEΪ��,10us����������
}










