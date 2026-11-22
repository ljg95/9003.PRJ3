#include "device_driver.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void Uart2_Init(int baud)
{
  double div;
  unsigned int mant;
  unsigned int frac;

  Macro_Set_Bit(RCC->AHB1ENR, 0);                   // PA2,3
  Macro_Set_Bit(RCC->APB1ENR, 17);                   // USART2 ON
  Macro_Write_Block(GPIOA->MODER, 0xf, 0xa, 4);     // PA2,3 => ALT
  Macro_Write_Block(GPIOA->AFR[0], 0xff, 0x77, 8);  // PA2,3 => AF07
  Macro_Write_Block(GPIOA->PUPDR, 0xf, 0x5, 4);     // PA2,3 => Pull-Up  

  volatile unsigned int t = GPIOA->LCKR & 0x7FFF;
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<2);                // Lock PA2, 3 Configuration
  GPIOA->LCKR = (0x0<<16)|t|(0x3<<2);
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<2);
  t = GPIOA->LCKR;

  div = PCLK1/(16. * baud);
  mant = (int)div;
  frac = (int)((div - mant) * 16. + 0.5);
  mant += frac >> 4;
  frac &= 0xf;

  USART2->BRR = (mant<<4)|(frac<<0);
  USART2->CR1 = (1<<13)|(0<<12)|(0<<10)|(1<<3)|(1<<2);
  USART2->CR2 = 0<<12;
  USART2->CR3 = 0;
}

void Uart2_RX_Interrupt_Enable(int en)
{
  if(en)
  {
    Macro_Set_Bit(USART2->CR1, 5);
    NVIC_ClearPendingIRQ(38);
    NVIC_EnableIRQ(38);
  }
  else
  {
    Macro_Clear_Bit(USART2->CR1, 5);
    NVIC_DisableIRQ(38);
  }
}

void Motor_UART_Command(char cmd)
{
	switch (cmd)
	{
		case 'f':
		case 'F':
    if(m_status()){
      m_stop();
      TIM2_Delay(500);
      m_cw();
      LED_On();
      printf("Motor: Forward (CW)\r\n");
    }break;

		case 'r':
		case 'R':
    if(m_status()){			
      m_stop();
      TIM2_Delay(500);
			m_ccw();
      LED_Off();
      printf("Motor: Reverse (CCW)\r\n");
    }break;

		case 's':
		case 'S':
    if(m_status()){
			m_stop();
      LED_Off();
			printf("Motor: Stop\r\n");
      Buzzer_Play_Song(2);
    }break;

		default:
			if (cmd >= '1' && cmd <= '9')
			{
				m_speed((unsigned char)cmd);
				printf("Motor: Speed level %c\r\n", cmd);
			}
			break;
	}

}
