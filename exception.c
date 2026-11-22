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

void TIM2_IRQHandler(void)
{
	Macro_Clear_Bit(TIM2->SR, 0);         // UIF 지우기
	NVIC_ClearPendingIRQ(28);
	// 아직 누르고 있는 유효한 누름이면 → 롱클릭
	if (Pressing && Macro_Check_Bit_Clear(GPIOC->IDR, 13)) {
		Pressing = 0;             // 뗄 때 rising은 무시되게
		// 2초 경과 → 모터 on/off
		if (m_status() == 1 || m_status() == 2) { m_stop(); LED_Off(); }
		else            { m_cw();   LED_On();  }
	}

}

volatile int TIM4_Expired = 0;

void TIM4_IRQHandler(void)
{
	Macro_Clear_Bit(TIM4->SR, 0);
	NVIC_ClearPendingIRQ(30);
	TIM4_Expired = 1;
}


volatile int DMA2_STREAM0_DONE = 0;

void DMA2_Stream0_IRQHandler(void)
{
	DMA2->LIFCR = 0x3F << 0;
	NVIC_ClearPendingIRQ(56);
    DMA2_STREAM0_DONE = 1;
}

volatile int DMA1_STREAM6_DONE = 0;

void DMA1_Stream6_IRQHandler(void)
{
	DMA1->HIFCR = 0x3F << 16;
	NVIC_ClearPendingIRQ(17);
    DMA1_STREAM6_DONE = 1;
}
