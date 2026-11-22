#include "device_driver.h"

void m_init (void) {
   Macro_Write_Block(GPIOA->MODER,0xf,0x5,0);//PA0, PA0 출력 모드로 설정
   Macro_Write_Block(GPIOA->OTYPER,0x3,0x0,0);//
   Macro_Write_Block(GPIOA->ODR,0x3,0x0,0);// 두 모터를 정지 상태
   TIM3_Out_Init();
   TIM3_Out_Freq_Generation(1000);// 상위 설계에선 8MHz로 되어 잇는데, test용 1000
   //기본 duty는 50%
}

void m_cw (void){
    Macro_Clear_Bit(GPIOA->ODR,0);//PA0
    Macro_Set_Bit(GPIOA->ODR,1);//PA1
}

void m_ccw(void){
    Macro_Set_Bit(GPIOA->ODR,0);
    Macro_Clear_Bit(GPIOA->ODR,1);
    
}

void m_stop(void){
    //m_speed(10);
    Macro_Clear_Area(GPIOA->ODR, 0x3, 0);   // PA0, PA1 동시에 0
}



void m_invert(int d){

    if(d==2) {
        m_stop();
        TIM4_Delay(500);
        m_ccw();
    } //이거 stop하고 delay 있어야 하나?
    else if (d==1) {
        m_stop();
        TIM4_Delay(500);
        m_cw();
    }
    
}

/// @brief Macro_Extract_Area(GPIOA->ODR,0x3,0) 현재 회전 정지 상태
/// @return  Macro_Extract_Area(GPIOA->ODR,0x3,0)
int m_status(){
    return Macro_Extract_Area(GPIOA->ODR,0x3,0);
}



/// @brief speed 문자 1~9까지 입력으로 펜의 속도 조절
/// @param speed 
void m_speed(unsigned char speed){
    const static unsigned char duty_arr[10] = {
        50, 56, 62, 68, 74, 80, 86, 92, 100, 0
    };

    // speed 문자형태 숫자를 int 형 숫자로 변경
    if (speed < 0x31 || speed > 0x39) return;

    int idx = speed - '0';
    TIM3_Out_PWM_Duty_Set(duty_arr[idx - 1]);
}

