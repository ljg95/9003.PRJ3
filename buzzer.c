#include "device_driver.h"
#include <stdio.h>

#define BASE  (500) //msec
#define TICK_MS (BASE / 4) // 16분음표(125ms)를 기본 인터럽트 주기로 설정

enum key {C1, C1_, D1, D1_, E1, F1, F1_, G1, G1_, A1, A1_, B1, C2, C2_, D2, D2_, E2, F2, F2_, G2, G2_, A2, A2_, B2};
// 각 음표가 몇 개의 틱(125ms) 동안 유지될지 비율로 정의 (N16=1, N8=2, N4=4, N2=8, N1=16)
enum note {N16 = 1, N8 = 2, N4 = 4, N2 = 8, N1 = 16};

const static unsigned short tone_value[] = {
    261, 277, 293, 311, 329, 349, 369, 391, 415, 440, 466, 493,
    523, 554, 587, 622, 659, 698, 739, 783, 830, 880, 932, 987
};

const static int song1[][2] = {
    {G1,N4}, {G1,N4}, {E1,N8}, {F1,N8}, {G1,N4}, {A1,N4}, {A1,N4}, {G1,N2},
    {G1,N4}, {C2,N4}, {E2,N4}, {D2,N8}, {C2,N8}, {D2,N2}
};

#define SONG_LEN (sizeof(song1) / sizeof(song1[0]))

const static int song2[][2] = {   //축하노래
    {C2, N8}, {E2, N8}, {G2, N8}, {C2, N4},
    {G1, N8}, {C2, N2}
};

#define SONG2_LEN (sizeof(song2) / sizeof(song2[0]))

// 인터럽트와 메인 루프가 공유하는 상태 변수 (반드시 volatile 선언)
volatile int song_idx = 0;      // 현재 연주 중인 음표 인덱스
volatile int tick_remain = 0;   // 현재 음표의 남은 시간(틱 수)

const static int (*cur_song)[2] = song1;
volatile int cur_song_len = SONG_LEN;
volatile int is_repeat = 1; // 1: 무한 반복(BGM), 0: 1회 재생 후 정지(효과음)

void Buzzer_Play_Song(int song_num)
{
    if (song_num == 1)
    {
        cur_song = song1;
        cur_song_len = SONG_LEN;
        is_repeat = 1;
    }
    else if (song_num == 2)
    {
        cur_song = song2;
        cur_song_len = SONG2_LEN;
        is_repeat = 0; // 축하음은 1번만 울리고 정지
    }

    song_idx = 0;
    TIM1_Out_Freq_Generation(tone_value[cur_song[0][0]]);
    tick_remain = cur_song[0][1];
    TIM4_Repeat_Interrupt_Enable(1, TICK_MS);
}

void Buzzer_BGM_1TICK(void)
{
    if(is_repeat)
    {
        LED_Background();
    }
    else
    {
        LED_Cong();
    }

    if (tick_remain > 0)
    {
        tick_remain--;
    }

    if (tick_remain == 0)
    {
        song_idx++;
        
        if (song_idx >= cur_song_len)
        {
            if (is_repeat)
            {
                song_idx = 0; // BGM은 무한 반복
            }
            else
            {
                // 축하음(1회 재생)이 끝나면 부저와 타이머 인터럽트 정지
                TIM1_Out_Stop();
                TIM4_Repeat_Interrupt_Enable(0, TICK_MS);
                Buzzer_Play_Song(1);
                return;
            }
        }

        TIM1_Out_Freq_Generation(tone_value[cur_song[song_idx][0]]);
        tick_remain = cur_song[song_idx][1];
    }
}