#include "./tools.h"

bool Flag_Need_Init_Nbiot = 1;
bool Flag_device_error = 0;

char* my_strcat(char* str1, char* str2) {
    char* pt = str1;
    while (*str1 != '\0') str1++;
    while (*str2 != '\0') *str1++ = *str2++;
    *str1 = '\0';
    return pt;
}

void StringToHexstring(char* string, char* hexstring) {
    int b;
    int length;
    char str2[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    char s1;
    char s2;
    int i, j = 0;
    length = strlen(string);
    for (i = 0, j = 0; (i < length) && (j < 2 * length); i++, j++) {
        b = 0x0f & (string[i] >> 4);
        s1 = str2[b];
        hexstring[j] = s1;
        b = 0x0f & string[i];
        s2 = str2[b];
        j++;
        hexstring[j] = s2;
    }
}

void Wait_OK(void) {
    while (!Flag_usart1_receive_OK);
    Flag_usart1_receive_OK = 0;
    CLR_Buf1();
}

void copy_str(char* des, char* src, unsigned char len) {
    unsigned char i;
    for (i = 0;i < len;i++) {
        *(des + i) = *(src + i);
    }
}

bool Wait_Str_x_100ms(char* str1, char* str2, unsigned char time_x_100ms) {
    bool Flag_receive_right = 0;
    Count_time_wait_ok = time_x_100ms;
    while (!Flag_receive_right && Count_time_wait_ok) {
        if (Flag_Usart1_Receive
            && (!Count_Timer3_value_USART1_receive_timeout)) {
            Flag_Usart1_Receive = 0;
            if (Query(USART1_RX_BUF, str1, USART1_REC_LEN)) {
                Flag_receive_right = 1;
                USART2TxStr(USART1_RX_BUF);
                USART2TxStr("Correct return!\r\n");
            }
            else if (Query(USART1_RX_BUF, str2, USART1_REC_LEN)) {
                Flag_receive_right = 1;
                USART2TxStr(USART1_RX_BUF);
                USART2TxStr("Correct return!\r\n");
            }
        }
    }
    if (!Count_time_wait_ok) {
        USART2TxStr("Flag_device_error = 1 \r\n");
        Flag_device_error = 1;
    }
    else
        Flag_device_error = 0;
    USART2TxStr("Count_time_wait_ok = ");
    USART2TxChar(Count_time_wait_ok / 100 + '0');
    USART2TxChar(Count_time_wait_ok % 100 / 10 + '0');
    USART2TxChar(Count_time_wait_ok % 10 + '0');
    USART2TxStr("\r\n");
    delay_ms(100);
    if (Flag_device_error)
        return 1;
    else
        return 0;
}

bool Wait_Str1_Str2_x_100ms(char uartx, char and_or, char str_NO, char* str1, char* str2, unsigned char time_x_100ms) {
    bool Flag_receive_right = 0;
    bool* add_Flag_Usart_x_Receive;
    unsigned char* Count_Timer3_value_USART_x_receive_timeout;
    char* USARTx_RX_BUF;
    unsigned int LEN = 0;
    switch (uartx) {
    case 1: add_Flag_Usart_x_Receive = &Flag_Usart1_Receive;
        Count_Timer3_value_USART_x_receive_timeout = &Count_Timer3_value_USART1_receive_timeout;
        USARTx_RX_BUF = &USART1_RX_BUF[0];
        LEN = USART_REC_LEN;
        break;
    case 2: add_Flag_Usart_x_Receive = &Flag_Usart2_Receive;
        Count_Timer3_value_USART_x_receive_timeout = &Count_Timer3_value_USART2_receive_timeout;
        USARTx_RX_BUF = (char*)&USART2_RX_BUF[0];
        LEN = USART_REC_LEN;
        break;
    case 3: add_Flag_Usart_x_Receive = &Flag_Usart3_Receive;
        Count_Timer3_value_USART_x_receive_timeout = &Count_Timer3_value_USART3_receive_timeout;
        USARTx_RX_BUF = (char*)&USART3_RX_BUF[0];
        LEN = USART_REC_LEN;
        break;

    }
    Count_time_wait_ok = time_x_100ms;
    while (!Flag_receive_right && Count_time_wait_ok) {
        if (*add_Flag_Usart_x_Receive
            && (!*Count_Timer3_value_USART_x_receive_timeout)) {
            *add_Flag_Usart_x_Receive = 0;
            if (and_or == 2) {
                if (Query(USARTx_RX_BUF, str1, LEN) && Query(USARTx_RX_BUF, str2, LEN)) {
                    Flag_receive_right = 1;
                }
            }
            else if (and_or == 1) {
                if (Query(USARTx_RX_BUF, str1, LEN)) {
                    Flag_receive_right = 1;
                }
                else {
                    if (str_NO == 2) {
                        if (Query(USARTx_RX_BUF, str2, LEN)) {
                            Flag_receive_right = 1;
                        }
                    }
                }
            }
        }
    }
    if (!Count_time_wait_ok) {
        Flag_device_error = 1;
        R_LED_ON;
    }
    else {
        R_LED_OFF;
        Flag_device_error = 0;
    }
    delay_ms(100);
    if (Flag_device_error)
        return 1;
    else
        return 0;
}

bool Wait_OK_x_100ms(unsigned char time_x_100ms) {
    Count_time_wait_ok = time_x_100ms;
    while (!Flag_usart1_receive_OK && Count_time_wait_ok) {

    }
    if (!Count_time_wait_ok) {
        Flag_device_error = 1;
    }
    else
        Flag_device_error = 0;
    Flag_usart1_receive_OK = 0;

    delay_ms(100);

    if (Flag_device_error)
        return 1;
    else
        return 0;
}

bool Wait_ready_x_100ms(unsigned char time_x_100ms) {
    bool Flag_usart1_receive_ready = 0;
    Count_time_wait_ok = time_x_100ms;
    while (!Flag_usart1_receive_ready && Count_time_wait_ok) {
        if (Query(USART1_RX_BUF, "ready", USART1_REC_LEN)) {
            Flag_usart1_receive_ready = 1;
            USART2TxStr("wifi moudule reboot successfully\r\n");
        }
    }
    if (!Count_time_wait_ok) {
        Flag_device_error = 1;
        USART2TxStr("Flag_device_error = 1\r\n");
    }
    delay_ms(100);
    if (Flag_device_error)
        return 1;
    else
        return 0;
}

unsigned char Query(char* src, char* des, unsigned int LEN) {
    unsigned int y = 0, len = 0, n = 0;
    unsigned char Result = 0;
    char* i;
    i = des;
    for (; *i != '\0';i++, len++) {}
    for (y = 0; y < LEN - len;y++) {
        for (n = 0;n < len;n++) {
            if (*(src + y + n) == *(des + n)) {
                Result = 1;
            }
            else {
                Result = 0;
                break;
            }
        }
        if (n == len) {
            return Result;
        }
    }
    return Result;
}

/**
 * 1.字符串拼接，数据长度计算并转为16进制。
 * 2.新添参数 service_id 服务id
 * */
void DealUpData(char* from, uint8_t* to, unsigned int service_id) {
    // 假设 服务ID=21,数据="025.0030.0"，下面是拼接过程
    int i = 0;
    char tmp[6] = { 0 };
    strcat((char*)to, "19,0,1,");  //AT+QLWDATASEND=19,0,1,

    sprintf(tmp, "%d", 5 + strlen(from));
    strcat((char*)to, tmp);//AT+QLWDATASEND=19,0,1,15

    tmp[0] = 0;
    strcat((char*)to, ",02");  //AT+QLWDATASEND=19,0,1,15,02

    sprintf(tmp, "%04x", service_id);
    strcat((char*)to, tmp); //AT+QLWDATASEND=19,0,1,15,020015

    tmp[0] = 0;
    sprintf(tmp, "%04x", strlen(from));
    strcat((char*)to, tmp); //AT+QLWDATASEND=19,0,1,15,020015000a

    tmp[0] = 0;
    for (i = 0;i < strlen(from);i++) {    // %02x  
        sprintf(tmp, "%02x", from[i]);  // tmp="31"  "%02x"  1
        strcat((char*)to, tmp);
        tmp[0] = 0;
    }//AT+QLWDATASEND=19,0,1,15,020015000a3032352e303033302e30

    strcat((char*)to, ",0x0100\r\n");
}

void Array_CLR(char* src, char len) {
    unsigned int y = 0;
    for (y = 0;y < len;y++) {
        src[y] = '\0';
    }
}





