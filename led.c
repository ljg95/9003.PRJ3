#include "device_driver.h"

volatile int l;

void LED_Init(void)
{
	/* 아래 코드 수정 금지 : Port-A Clock Enable */
	Macro_Set_Bit(RCC->AHB1ENR, 0);
	Macro_Set_Bit(RCC->AHB1ENR, 1);

	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10);
	Macro_Write_Block(GPIOB->MODER, 0xFF, 0x55, 6);
	Macro_Clear_Bit(GPIOA->OTYPER, 5);
	Macro_Clear_Area(GPIOB->OTYPER, 0xf, 3);
	Macro_Clear_Bit(GPIOA->ODR, 5); 
	Macro_Set_Area(GPIOB->ODR, 0xA, 3);

}

void LED_On(void)
{
	Macro_Set_Bit(GPIOA->ODR, 5); 
}

void LED_Off(void)
{
	Macro_Clear_Bit(GPIOA->ODR, 5); 
}

void LED_Display(int on)
{
	Macro_Write_Block(GPIOA->ODR, 0x1, on & 0x1, 5);
}

void LED_Mode(int mode)
{
	if (mode)
	{
		LED_Cong();
	}
	else 
	{
		LED_Background();
	}
}

void LED_Background(void)
{
	if (l) { Macro_Clear_Area(GPIOB->ODR, 0x5, 3); Macro_Set_Area(GPIOB->ODR, 0xA, 3); l ^= 1;}
	else { Macro_Clear_Area(GPIOB->ODR, 0xA, 3); Macro_Set_Area(GPIOB->ODR, 0x5, 3); l^= 1;}
}

void LED_Background_Off(void)
{
	Macro_Clear_Area(GPIOB->ODR, 0xff, 3);
}

void LED_Cong(void)
{
	Macro_Set_Area(GPIOB->ODR, 0xf, 3);
}