#include "timer.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	 
//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器3!
unsigned int Count_timer = 0;  // 无符号整型取值范围0~65535 即周期约为6553秒=109分钟
unsigned int Count_timer_100ms_x_Collect_Interval = 65530;//默认先不触发采集计时
unsigned char Count_timer_100ms_x_Send_Interval_after_collect = 201;//默认不触发发送计时

unsigned char Count_wait_ok = 0;
unsigned char Count_Send_Time_Interval = 0;
unsigned char count_time = 0;
unsigned char Count_time_wait_ok = 0;
bool Flag_allow_dsplay_time;
bool is_send_breath = 0;

void TIM3_Int_Init(u16 arr, u16 psc) {
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能

	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx					 
}

//定时器3中断服务程序
void TIM3_IRQHandler(void) {  //TIM3中断 100ms
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) { //检查TIM3更新中断发生与否
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx更新中断标志
		Count_timer++;  //计数器加100ms
		if (is_send_breath && Count_timer % 30 == 0) {
			USART2TxStr("AT+QLWDATASEND=19,0,0,16,020014000b6565656565656565656565,0x0000\r\n");
		}

		if (Count_Timer3_value_USART3_receive_timeout) {
			Count_Timer3_value_USART3_receive_timeout--;
		}
		if (Count_Timer3_value_USART2_receive_timeout)
			Count_Timer3_value_USART2_receive_timeout--;
		if (Count_Timer3_value_USART3_receive_timeout)
			Count_Timer3_value_USART3_receive_timeout--;
		if (Count_Send_Time_Interval)
			Count_Send_Time_Interval--;
		if (Count_Timer3_value_USART1_receive_timeout)
			Count_Timer3_value_USART1_receive_timeout--;
		if (Count_Timer3_value_USART2_receive_timeout)
			Count_Timer3_value_USART2_receive_timeout--;

		if (count_time)
			count_time--;
		if (Count_time_wait_ok)
			Count_time_wait_ok--;
		if (Count_timer_100ms_x_Collect_Interval < 65530) {
			Count_timer_100ms_x_Collect_Interval++;
			Flag_allow_dsplay_time = 1;
		}
		if (Count_timer_100ms_x_Send_Interval_after_collect < 200) {
			Count_timer_100ms_x_Send_Interval_after_collect++;
		}
	}
}

