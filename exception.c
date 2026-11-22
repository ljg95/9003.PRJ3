#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}

volatile int Pressing = 0;      // 유효한 누름이 진행 중인지

void EXTI15_10_IRQHandler(void){
	EXTI->PR = 0x1 << 13;
	NVIC_ClearPendingIRQ(40);

	// 토글하지 말고 핀을 직접 읽어서 상태 결정 falling 1 rising 0
	int Key_Pressed =  Macro_Check_Bit_Clear(GPIOC->IDR, 13);
	if (Key_Pressed) {                 // falling (누름)
		TIM2_Stopwatch_Start(2000);    // falling마다 항상 다시 시작
		Pressing = 1;
		return;
	}

	if (!Pressing) return;             // 누름 없이 들어온 rising은 무시
	Pressing = 0;

	if (TIM2_Check_Timeout() == 0) return;                // 20ms 미만이면 채터링이므로 무시
	// EXTI rising, 숏클릭 부분
	int s = m_status();
	m_invert(s);
	LED_Display(s == 2 ? 0 : 1);
}

void USART2_IRQHandler(void)
{
	if (Macro_Check_Bit_Set(USART2->SR, 5)) 
	{
		char ch = (char)USART2->DR;
		Motor_UART_Command(ch);
	}
}

void TIM2_IRQHandler(void)
{
	Macro_Clear_Bit(TIM2->SR, 0);         // UIF 지우기
	NVIC_ClearPendingIRQ(28);
	// 아직 누르고 있는 유효한 누름이면 → 롱클릭
	if (Pressing && Macro_Check_Bit_Clear(GPIOC->IDR, 13)) {
		Pressing = 0;             // 뗄 때 rising은 무시되게
		// 2초 경과 → 모터 on/off
		if (m_status() == 1 || m_status() == 2) { m_stop(); Buzzer_Play_Song(2); LED_Off();}
		else            { m_speed('1'); m_cw(); TIM1_Out_Stop(); TIM4_Repeat_Interrupt_Enable(0, 0); LED_On(); LED_Background_Off(); }
	}

}


void TIM4_IRQHandler(void)
{
	Macro_Clear_Bit(TIM4->SR, 0);
	NVIC_ClearPendingIRQ(30);
	
	Buzzer_BGM_1TICK();
}

