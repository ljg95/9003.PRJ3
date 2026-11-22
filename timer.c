#include "device_driver.h"

#define TIM2_TICK         	(20) 					// usec
#define TIM2_FREQ 	  		(1000000./TIM2_TICK)	// Hz
#define TIME2_PLS_OF_1ms  	(1000./TIM2_TICK)
#define TIM2_MAX	  		(0xffffffffu)

#define TIM3_FREQ 	  			(8000000.) 	      		// Hz
#define TIM3_TICK	  			(1000000./TIM3_FREQ)	// usec
#define TIME3_PLS_OF_1ms  		(1000./TIM3_TICK)

#define TIM4_TICK	  		(20) 					// usec
#define TIM4_FREQ 	  		(1000000./TIM4_TICK) 	// Hz
#define TIME4_PLS_OF_1ms  	(1000./TIM4_TICK)
#define TIM4_MAX	  		(0xffffu)

#define TIM1_FREQ 	  			(8000000.) 	      		// Hz
#define TIM1_TICK	  			(1000000./TIM1_FREQ)	// usec
#define TIME1_PLS_OF_1ms  		(1000./TIM1_TICK)


void TIM2_Stopwatch_Start(int time) // ms
{
	Macro_Set_Bit(RCC->APB1ENR, 0);

	TIM2->CR1 = (1<<4)|(1<<3)|(1<<2);                 // ← 수정: URS 추가 URS = 1로 하면 "카운터가 진짜로 끝났을 때만 알려줘"
	TIM2->PSC = (unsigned int)(TIMXCLK/TIM4_FREQ + 0.5)-1;
	TIM2->ARR = TIME4_PLS_OF_1ms * time - 1;

	Macro_Set_Bit(TIM2->EGR,0);

	Macro_Clear_Bit(TIM2->SR, 0);         // UIF 지우기

	NVIC_ClearPendingIRQ(28);
	Macro_Set_Bit(TIM2->DIER, 0);         // UIE: 업데이트 인터럽트 허용
	NVIC_EnableIRQ(28);

	Macro_Set_Bit(TIM2->CR1, 0);
}

// 0: 20ms 안 지남 / 1: 20ms 지남 / 2: 2초 지남(타이머 종료)
int TIM2_Check_Timeout(void)
{
	// 타이머가 멈췄으면 2초 경과
	if (Macro_Check_Bit_Set(TIM2->SR, 0)) {
		Macro_Clear_Bit(TIM2->SR, 0);
		return 2;
	}

	// 경과 시간(usec) >= 20ms
	if ((TIM2->ARR - TIM2->CNT) * TIM4_TICK >= (TIM4_TICK * 1000)) return 1;

	return 0;
}

unsigned int TIM2_Ms_To_Tick(void)
{
	return (unsigned int)(TIME4_PLS_OF_1ms * TIM4_TICK);
}


void TIM2_Delay(int time)
{
	Macro_Set_Bit(RCC->APB1ENR, 0);

	TIM2->CR1 = (1<<4)|(1<<3);
	TIM2->PSC = (unsigned int)(TIMXCLK/TIM2_FREQ + 0.5)-1;
	TIM2->ARR = TIME2_PLS_OF_1ms * time;

	Macro_Set_Bit(TIM2->EGR,0);
	Macro_Clear_Bit(TIM2->SR, 0);
	Macro_Set_Bit(TIM2->CR1, 0);

	while(Macro_Check_Bit_Clear(TIM2->SR, 0));
	Macro_Clear_Bit(TIM2->CR1, 0);
}

void TIM4_Repeat(int time)
{
	Macro_Set_Bit(RCC->APB1ENR, 2);

	TIM4->CR1 = (1<<4)|(0<<3);
	TIM4->PSC = (unsigned int)(TIMXCLK/TIM4_FREQ + 0.5)-1;
	TIM4->ARR = TIME4_PLS_OF_1ms * time - 1;

	Macro_Set_Bit(TIM4->EGR,0);
	Macro_Clear_Bit(TIM4->SR, 0);
	Macro_Set_Bit(TIM4->CR1, 0);
}

void TIM4_Repeat_Interrupt_Enable(int en, int time)
{
	if(en)
	{
		// TIM4 Clock On
		Macro_Set_Bit(RCC->APB1ENR, 2);
		TIM4->CR1 = (1<<4)|(0<<3);
		TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5)-1;
		TIM4->ARR = TIME4_PLS_OF_1ms * time;
		Macro_Set_Bit(TIM4->EGR,0);
		
		// TIM4 Pending Clear
		// NVIC Pending Clear
		Macro_Clear_Bit(TIM4->SR, 0);
		NVIC_ClearPendingIRQ(30);
		// TIM4 Interrupt Enable
		// NVIC Interrupt Enable
		Macro_Set_Bit(TIM4->DIER, 0);
		NVIC_EnableIRQ(30);
		// TIM4 Start
		Macro_Set_Bit(TIM4->CR1, 0);
	}

	else
	{
		NVIC_DisableIRQ(30);
		Macro_Clear_Bit(TIM4->CR1, 0);
		Macro_Clear_Bit(TIM4->DIER, 0);
	}
}

int TIM4_Check_Timeout(void)
{
	if(Macro_Check_Bit_Set(TIM4->SR, 0))
	{
		Macro_Clear_Bit(TIM4->SR, 0);
		return 1;
	}
	else
	{
		return 0;
	}
}

void TIM4_Stop(void)
{
	Macro_Clear_Bit(TIM4->CR1, 0);
}

void TIM4_Change_Value(int time)
{
	TIM4->ARR = TIME4_PLS_OF_1ms * time;
}


void TIM3_Out_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Set_Bit(RCC->APB1ENR, 1);

	Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 0);  	// PB0 => ALT
	Macro_Write_Block(GPIOB->AFR[0], 0xf, 0x2, 0); 	// PB0 => AF02

	Macro_Write_Block(TIM3->CCMR2,0xff, 0x68, 0);
	TIM3->CCER = (0<<9)|(1<<8);
}

void TIM3_Out_Freq_Generation(unsigned short freq)
{
	// Timer 주파수가 TIM3_FREQ가 되도록 PSC 설정
	TIM3->CCMR2=(1<<6)|(1<<5)|(1<<3);
	TIM3->CCER=(0<<9)|(1<<8);
	TIM3->PSC=(unsigned int)(SYSCLK/TIM3_FREQ+0.5)-1 ;
	// 요청한 주파수가 되도록 ARR 설정
	TIM3->ARR=(TIM3_FREQ/freq+0.5);
	// Duty Rate 50%가 되도록 CCR3 설정
	TIM3->CCR3=(TIM3->ARR)/2;
	// Manual Update(UG 발생)
	Macro_Set_Bit(TIM3->EGR,0);
	// Down Counter, Repeat Mode, Timer Start
	TIM3->CR1=(1<<4)|(0<<3)|(1<<0);

}

void TIM3_Out_PWM_Duty_Set(int duty)
{
    TIM3->CCR3 = ((TIM3->ARR+1)*duty)/100;
}

void TIM3_Out_Start(void)
{
	Macro_Set_Bit(TIM3->CR1, 0);
}


void TIM3_Out_Stop(void)
{
	Macro_Clear_Bit(TIM3->CR1, 0);
}



void TIM1_Out_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 0);
    Macro_Set_Bit(RCC->APB2ENR, 0);

    Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 16);
    Macro_Write_Block(GPIOA->AFR[1], 0xf, 0x1, 0);

    Macro_Write_Block(TIM1->CCMR1, 0xff, 0x68, 0);

    TIM1->CCER = (0<<1)|(1<<0);
    Macro_Set_Bit(TIM1->BDTR, 15);
}


void TIM1_Out_Freq_Generation(unsigned short freq)
{
    if (freq == 0) return;

    TIM1->CCMR1 = (1<<6)|(1<<5)|(1<<3);

    TIM1->CCER = (0<<1)|(1<<0);

    Macro_Set_Bit(TIM1->BDTR, 15);

    TIM1->PSC = (unsigned int)(SYSCLK/TIM1_FREQ+0.5)-1;

    TIM1->ARR = (unsigned int)(TIM1_FREQ/freq+0.5)-1;

    TIM1->CCR1 = (TIM1->ARR+1)/2;

    Macro_Set_Bit(TIM1->EGR, 0);

    TIM1->CNT = 0;

    TIM1->CR1 = (1<<4)|(0<<3)|(1<<0);
}

void TIM1_Out_Stop(void)
{
    Macro_Clear_Bit(TIM1->CR1, 0);
}