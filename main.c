#include "device_driver.h"
#include <stdio.h>
#include <string.h>

static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}


#if 1
void Main(void)
{
	
	Sys_Init(115200);
    m_init();
    Key_ISR_Enable(1);
    
    for(;;){
        
    }

    // printf("Motor Test!!\n");

    // int flag=0;
	// m_init();
	// //모터 먼저 돌리고 pwm만 바꿔보자
    // //1. 기본 순방향 동작.기본 duty는 50
	// m_cw (); //이때 모터 속도 기본값이 뭔지 확인
    // TIM2_Delay(10000);

    // //2. 스위치 짧게. (순방향-> 역방향)
    // printf("motor invert1\n");
    // flag=m_status();
    // printf("%d", flag);
    // m_invert(flag);
    // TIM2_Delay(10000);

    // //3. 모터 정지
    // printf("motor stop\n");
    // m_stop();
    // TIM2_Delay(10000);

    // //4.모터 정지인 상태에서 스위치 짧게. (동작 X)
    // printf("motor invert2\n");
    // flag=m_status();
    // printf("%d", flag);
    // m_invert(flag);
    // TIM2_Delay(10000);

    // //5. 짧은 스위치
    // flag=m_status();
    // printf("%d", flag);
    // printf("rcc 돌리기\n");
    // m_ccw();//일단 돌려놓고
    // TIM2_Delay(3000);
    // printf("motor invert3\n");
    // flag=m_status();
    // printf("%d", flag);
    // m_invert(flag);
    // TIM2_Delay(10000);

    // //6. 모터 방향은 유지. 속도제어 1
	// printf("Speeed 1\n");
    // m_speed(1);
	// TIM2_Delay(10000);

    // //7. 속도 증가
    // printf("Speeed 3\n");
    // m_speed(3);
    // TIM2_Delay(10000);

    // //8. 속도 증가
	// printf("Speeed 5\n");
    // m_speed(5);
	// TIM2_Delay(10000);

    // //9.방향 전환 (정크? 발생 유무 확인)
    // printf("motor invert3\n");
    // flag=m_status();
    // printf("%d", flag);
    // m_invert(flag);
    // TIM2_Delay(10000);

	// //10.모터 정지
	// printf("motor stop\n");
    // m_stop();
    // TIM2_Delay(10000);


    return ;
}



#endif


#if 0

extern volatile int TIM4_Expired;

#define NUM_OF_DATA 40

unsigned int src1_dat[NUM_OF_DATA];
unsigned int dst1_dat[NUM_OF_DATA];

void Main(void)
{
	unsigned int i=0;

	Sys_Init(115200);
	printf("Memory Copy Test\n");

	TIM4_Repeat_Interrupt_Enable(1, 1000);

	for(i=0;i<NUM_OF_DATA;i++)
	{
		src1_dat[i] = i + 8;
		dst1_dat[i] = 0;
	}

	for(i=0;i<NUM_OF_DATA;i++)
	{
		dst1_dat[i] = src1_dat[i];
	}

	for(i=0;i<NUM_OF_DATA;i++)
	{
		printf("dst1_dat[%d] = %d\n",i,dst1_dat[i]);
		dst1_dat[i] = 0;
	}

	for(;;)
	{
		if(TIM4_Expired)
		{
			static unsigned int led = 0;
			LED_Display(led ^= 0x1);
			TIM4_Expired = 0;
		}
	}
}

#endif

#if 0

extern volatile int TIM4_Expired;
extern volatile int DMA2_STREAM0_DONE;

#define NUM_OF_DATA     40

unsigned int src1_dat[NUM_OF_DATA];
unsigned int dst1_dat[NUM_OF_DATA];

void Main(void)
{
    unsigned int i = 0;

    Sys_Init(115200);
    printf("DMA Software Trigger Test(STM32F411)\n");

    TIM4_Repeat_Interrupt_Enable(1, 1000);

    for(i = 0; i < NUM_OF_DATA; i++)
    {
        src1_dat[i] = i + 8;
        dst1_dat[i] = 0;
    }

    DMA2_Stream0_M2M_Start(src1_dat, dst1_dat, NUM_OF_DATA);

    for(;;)
    {
        if(DMA2_STREAM0_DONE)
        {
            printf("\nDMA2-Stream0 Complete!\n");

            for(i = 0; i < NUM_OF_DATA; i++)
            {
                printf("@dst1_dat[%d] = %d\n", i, dst1_dat[i]);
                dst1_dat[i] = 0;
            }

            DMA2_STREAM0_DONE = 0;
        }

        if(TIM4_Expired)
        {
            static unsigned int led = 0;
            LED_Display(led ^= 0x1);
            TIM4_Expired = 0;
        }
    }
}

#endif

#if 0

extern volatile int TIM4_Expired;

const char * str[] = {
    "[1] The DMA controller performs direct memory transfer by sharing the system bus with the Cortex™-M3 core.\n",
	"[2] The DMA request may stop the CPU access to the system bus for some bus cycles,\n",
	"[3] when the CPU and DMA are targeting the same destination (memory or peripheral).\n",
	"[4] The bus matrix implements round-robin scheduling, thus ensuring at least half of the system bus bandwidth (both to memory and peripheral) for the CPU.\n",
	"[5] After an event, the peripheral sends a request signal to the DMA Controller.\n",
	"[6] The DMA controller serves the request depending on the channel priorities.\n",
	"[7] As soon as the DMA Controller accesses the peripheral, an Acknowledge is sent to the peripheral by the DMA Controller.\n",
	"[8] The peripheral releases its request as soon as it gets the Acknowledge from the DMA Controller.\n",
	"[9] Once the request is deasserted by the peripheral, the DMA Controller release the Acknowledge.\n" };

void Main(void)
{
    unsigned int str_num = 0;

    Sys_Init(115200);
    printf("USART String TX Test\n\n");
    TIM4_Repeat_Interrupt_Enable(1, 1000);

    for (;;)
    {
		if(str_num < (sizeof(str)/sizeof(str[0])))
		{
			printf("%s", str[str_num]);
			str_num++;
		}

        if(TIM4_Expired)
        {
            static unsigned int led = 0;
            LED_Display(led ^= 0x1);
            TIM4_Expired = 0;
        }        
    }
}

#endif

#if 0

extern volatile int TIM4_Expired;
extern volatile int DMA1_STREAM6_DONE;

const char * str[] = {
    "[1] The DMA controller performs direct memory transfer by sharing the system bus with the Cortex™-M3 core.\n\r",
	"[2] The DMA request may stop the CPU access to the system bus for some bus cycles,\n\r",
	"[3] when the CPU and DMA are targeting the same destination (memory or peripheral).\n\r",
	"[4] The bus matrix implements round-robin scheduling, thus ensuring at least half of the system bus bandwidth (both to memory and peripheral) for the CPU.\n",
	"[5] After an event, the peripheral sends a request signal to the DMA Controller.\n\r",
	"[6] The DMA controller serves the request depending on the channel priorities.\n\r",
	"[7] As soon as the DMA Controller accesses the peripheral, an Acknowledge is sent to the peripheral by the DMA Controller.\n",
	"[8] The peripheral releases its request as soon as it gets the Acknowledge from the DMA Controller.\n",
	"[9] Once the request is deasserted by the peripheral, the DMA Controller release the Acknowledge.\n" };

static void Uart2_Wait_for_TX_Complete(void)
{
    while(!Macro_Check_Bit_Set(USART2->SR, 6));
}

void Main(void)
{
    unsigned int str_num = 0;

    Sys_Init(115200);
    
    TIM4_Repeat_Interrupt_Enable(1, 1000);

    printf("DMA(M2P) Test - H/W (USART) Trigger\n\n");    
    Uart2_Wait_for_TX_Complete();

    for (;;)
    {
        if (DMA1_STREAM6_DONE || (str_num == 0))
        {
            if (str_num < (sizeof(str) / sizeof(str[0])))
            {
                Macro_Set_Bit(USART2->CR3, 7);
                DMA1_Stream6_USART2_TX_Satrt((void * )str[str_num], strlen(str[str_num]));
                str_num++;
                DMA1_STREAM6_DONE = 0;                
            }

            else
            {
                Macro_Clear_Bit(DMA1_Stream6->CR, 0);	
                Macro_Clear_Bit(USART2->CR3, 7);
            }
        }

        if(TIM4_Expired)
        {
            static unsigned int led = 0;
            LED_Display(led ^= 0x1);
            TIM4_Expired = 0;
        }        
    }
}

#endif
