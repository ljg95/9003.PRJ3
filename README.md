# STM32F411 DC 모터 제어 및 부저·LED 연출 펌웨어

STM32F411RE(Cortex-M4) 보드에서 DC 모터의 **회전 방향(정회전/역회전/정지)** 과 **속도(PWM duty)** 를 **UART 명령** 과 **사용자 버튼(PC13)** 으로 제어하고, 모터 동작 상태에 따라 **부저 사운드(배경음악 BGM / 정지 축하음)** 및 **외부 LED(PB3~PB6) 패턴 연출**을 수행하는 베어메탈 펌웨어입니다.
모든 동작은 타이머 및 외부/통신 인터럽트로 처리되고, `Main()` 의 메인 루프는 비어 있습니다.

## 동작 흐름 (`main.c`)

```c
void Main(void)
{
    Sys_Init(115200);               // FPU, 클럭, UART2, LED(PA5, PB3~PB6) 초기화
    m_init();                       // 모터 GPIO(PA0, PA1) + PWM(TIM3) 초기화
    Key_ISR_Enable(1);              // 버튼(PC13) EXTI 인터럽트 활성화
    Uart2_RX_Interrupt_Enable(1);   // UART2 수신 인터럽트 활성화
    TIM1_Out_Init();                // 부저 PWM 출력(TIM1_CH1, PA8) 초기화
    Buzzer_Play_Song(1);            // 대기 상태 배경음악(BGM) 재생 및 TIM4(125ms) 시작

    for(;;){ }                      // 이후 모든 처리는 ISR에서 수행
}
```

| 단계 | 함수 | 내용 |
|---|---|---|
| 1 | `Sys_Init(115200)` | FPU(CP10/CP11) 활성화 → `Clock_Init()`(SYSCLK 96MHz) → `Uart2_Init(115200)` → `stdout` 버퍼링 해제(`printf` 즉시 출력) → `LED_Init()`(PA5 및 PB3~PB6 출력 설정) |
| 2 | `m_init()` | PA0/PA1을 출력으로 설정하고 모터 정지, TIM3 CH3(PB0) PWM 1kHz·duty 50%로 시작 |
| 3 | `Key_ISR_Enable(1)` | PC13을 EXTI13으로 연결, rising/falling 양쪽 엣지 인터럽트(IRQ 40) 활성화 |
| 4 | `Uart2_RX_Interrupt_Enable(1)` | USART2 RXNE 인터럽트(IRQ 38) 활성화 |
| 5 | `TIM1_Out_Init()` | PA8을 TIM1_CH1(AF1) PWM 출력으로 설정하고 MOE(Main Output Enable) 활성화 |
| 6 | `Buzzer_Play_Song(1)` | 배경음악(`song1`, 무한 반복) 첫 음을 출력하고 TIM4 125ms 주기 인터럽트(IRQ 30) 시작 |
| 7 | `for(;;)` | 대기. 실제 제어는 `USART2_IRQHandler`, `EXTI15_10_IRQHandler`, `TIM2_IRQHandler`, `TIM4_IRQHandler`에서 처리 |

## 핀 배치

| 핀 | 용도 |
|---|---|
| PA0, PA1 | L293 방향 입력 1A(2번 핀), 2A(7번 핀) — Push-Pull 출력 |
| PB0 | TIM3_CH3 PWM 출력 → L293 1,2EN(1번 핀), 모터 속도 제어 (AF2) |
| PA8 | TIM1_CH1 PWM 출력 → 부저(Buzzer) 음계 주파수 출력 (AF1) |
| PB3 ~ PB6 | 부저/상태 연동 외부 LED 4개 — Push-Pull 출력 (BGM 교차 점멸 / 축하음 전체 점등 / 모터 구동 시 소등) |
| PA2 / PA3 | USART2 TX / RX (115200 bps, 8N1) — ST-LINK 가상 COM 포트 |
| PA5 | 보드 내장 상태 LED (모터 정회전 시 ON, 역회전/정지 시 OFF) |
| PC13 | 사용자 버튼 (누르면 Low) |

## 타이머 할당 현황

| 타이머 | 용도 | 설정 |
|---|---|---|
| `TIM1` (CH1, PA8) | 부저 음계 주파수(PWM) 생성 | 8MHz 카운터 클럭 기준 음계 주파수별 `ARR` 가변, Duty 50% |
| `TIM2` | 버튼 롱클릭 스톱워치(2초) & 모터 정지/방향 전환 딜레이(500ms) | 20µs 틱 (50kHz), 인터럽트(IRQ 28) 및 폴링 딜레이 |
| `TIM3` (CH3, PB0) | DC 모터 속도 제어용 PWM 생성 | 1kHz 고정 주파수, 속도 단계(1~9)에 따라 Duty 50~100% 가변 |
| `TIM4` | 부저 악보 재생 및 LED 연출용 주기 타이머 | 16분음표 길이인 **125ms 주기** 반복 인터럽트(IRQ 30) |

## 회로도 (L293 모터 드라이버)

L293의 1·2번 채널(H-브리지 좌측)만 사용해 모터 1개를 양방향으로 구동합니다.

```
      NUCLEO-F411RE                            L293 (16-pin DIP)
   +----------------+                    +---------------------------+
   |            3V3 |------------------->| 16  VCC1  (logic 3.3V)    |
   |                |      5V ---------->|  8  VCC2  (motor 5V)      |
   |                |                    |                           |
   |            PB0 |--- TIM3_CH3 PWM -->|  1  1,2EN                 |
   |            PA0 |------------------->|  2  1A                    |
   |            PA1 |------------------->|  7  2A                    |
   |                |                    |                           |
   |                |      +-------+     |                           |
   |                |      |       +-----|  3  1Y                    |
   |                |      | MOTOR |     |                           |
   |                |      |       +-----|  6  2Y                    |
   |                |      +-------+     |                           |
   |                |                    |                           |
   |            GND |------------------->|  4, 5  GND                |
   +----------------+                    |                           |
                                         |  9 ~ 15  NC (unused)      |
                                         +---------------------------+
```

| L293 핀 | 이름 | 연결 |
|---|---|---|
| 1 | 1,2EN | PB0 (TIM3_CH3 PWM, 1kHz) |
| 2 | 1A | PA0 |
| 3 | 1Y | 모터 단자 A |
| 4, 5 | GND (방열) | GND |
| 6 | 2Y | 모터 단자 B |
| 7 | 2A | PA1 |
| 8 | VCC2 | 5V (모터 전원, 로직 전원과 분리) |
| 9 ~ 15 | 3,4EN, 3A, 3Y, GND, GND, 4Y, 4A | 미사용 (미연결) |
| 16 | VCC1 | 3.3V (로직 전원) |

- 보드 GND와 L293 GND, 5V 모터 전원의 GND는 반드시 공통으로 연결합니다.
- L293**D**는 플라이백 다이오드가 내장되어 있습니다. L293(D 없음)을 쓰는 경우 1Y·2Y 각각에서 VCC2 방향과 GND 방향으로 외부 다이오드 4개가 필요합니다.

### 방향/속도 제어 진리표

| PA0 (1A) | PA1 (2A) | 1Y | 2Y | `m_status()` | 코드 상 상태 |
|---|---|---|---|---|---|
| 0 | 0 | L | L | 0 | 정지 (`m_stop`) |
| 1 | 0 | H | L | 1 | 역회전 CCW (`m_ccw`) |
| 0 | 1 | L | H | 2 | 정회전 CW (`m_cw`) |

EN(PB0)에 PWM이 들어가므로, EN이 High인 구간에만 출력이 구동되고 Low 구간에는 출력이 Hi-Z가 됩니다.
따라서 duty(50~100%)가 클수록 모터 평균 전압이 높아져 빨라집니다.

### 모터 상태 (`m_status()` = `GPIOA->ODR[1:0]`)

| 값 | PA1 | PA0 | 상태 |
|---|---|---|---|
| 0 | 0 | 0 | 정지 (`m_stop`) |
| 1 | 0 | 1 | 역회전 CCW (`m_ccw`) |
| 2 | 1 | 0 | 정회전 CW (`m_cw`) |

## 부저 사운드 및 LED(PB3~PB6) 연동 동작

부저는 `TIM1_CH1`(`PA8`)으로 음계 주파수를 출력하며, `TIM4`가 **125ms(16분음표 1틱)** 마다 인터럽트(`TIM4_IRQHandler` → `Buzzer_BGM_1TICK()`)를 발생시켜 음표 길이를 카운트하고 외부 LED(`PB3`~`PB6`)를 제어합니다.

| 상태 | 트리거 조건 | 부저 동작 (`PA8`) | 외부 LED 동작 (`PB3`~`PB6`) | 내장 LED (`PA5`) |
|---|---|---|---|---|
| **대기 상태 (BGM)** | 부팅 직후 또는 축하음 재생 완료 후 (`Buzzer_Play_Song(1)`) | 배경음악(`song1`, 학교종) 무한 반복 재생 | `LED_Background()`: 125ms 틱마다 홀수/짝수 비트(`0xA` ↔ `0x5`) 교차 점멸 | OFF |
| **모터 구동 중** | 버튼 롱클릭으로 모터 시동 시 | 부저 출력 정지 (`TIM1_Out_Stop`) 및 `TIM4` 인터럽트 비활성화 | `LED_Background_Off()`: 외부 LED 전체 소등 | CW: ON / CCW: OFF |
| **모터 정지 (축하 사운드)** | 회전 중 버튼 롱클릭 또는 UART `S`/`s` 명령으로 정지 시 (`Buzzer_Play_Song(2)`) | 축하 사운드(`song2`) 1회 재생 후 자동으로 대기 상태 BGM(`song1`)으로 복귀 | `LED_Cong()`: 축하 사운드가 재생되는 동안 4개 LED(`0xF`) 전체 점등 | OFF |

## 사용법

### UART 명령 (115200 bps)

터미널에서 한 글자를 보내면 `USART2_IRQHandler`에서 `Motor_UART_Command()`를 호출해 처리합니다(대소문자 구분 없음).

| 입력 | 동작 | 출력 메시지 |
|---|---|---|
| `F` / `f` | 정지(`m_stop`) → 추가 500ms 대기 → 정회전(CW), 내장 LED(PA5) ON | `Motor: Forward (CW)` |
| `R` / `r` | 정지(`m_stop`) → 추가 500ms 대기 → 역회전(CCW), 내장 LED(PA5) OFF | `Motor: Reverse (CCW)` |
| `S` / `s` | 정지(`m_stop`), 내장 LED(PA5) OFF, **축하 사운드(`song2`) 1회 재생 후 BGM 복귀** | `Motor: Stop` |
| `1` ~ `9` | 속도 단계 변경 (PWM duty) | `Motor: Speed level N` |

> **주의:** `F`/`R`/`S`는 모터가 **회전 중일 때만** 동작합니다(`m_status() != 0`).
> 정지 상태에서 모터를 처음 켜려면 버튼 롱클릭을 사용해야 합니다.

속도 단계별 duty (`m_speed`):

| 단계 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| Duty(%) | 50 | 56 | 62 | 68 | 74 | 80 | 86 | 92 | 100 |

### 버튼 (PC13)

| 조작 | 동작 |
|---|---|
| **롱클릭** (2초 이상 누름) | • **정지 상태일 때**: 속도 1단계로 정회전(CW) 시작 + 내장 LED(PA5) ON + **부저 BGM 정지 및 외부 LED(PB3~PB6) 소등**<br>• **회전 중일 때**: 모터 정지 + 내장 LED(PA5) OFF + **축하 사운드(`song2`) 재생(외부 LED 전체 점등) 후 BGM 및 교차 점멸로 자동 복귀** |
| **숏클릭** (20ms ~ 2초) | 회전 방향 반전 (CW ↔ CCW, 정지 후 반대 방향 구동 및 내장 LED 토글) |
| 20ms 미만 | 채터링으로 간주하여 무시 |

버튼을 누르면(falling) `TIM2_Stopwatch_Start(2000)`으로 2초 타이머가 시작됩니다.
- 2초가 지나도 계속 누르고 있으면 `TIM2_IRQHandler`에서 롱클릭으로 처리합니다.
- 2초 전에 떼면(rising) `EXTI15_10_IRQHandler`에서 경과 시간을 확인해 숏클릭으로 처리합니다.

## 빌드 및 다운로드

- 툴체인: Arm GNU Toolchain (`arm-none-eabi`)
- 링커 스크립트: `rom_0x08000000.lds` (Flash 0x08000000 시작)
- 시작 코드: `crt0.s` → `Main()` 호출

```bash
make
```

```bash
make run
```

`make`는 `rom_0x08000000.elf` / `.bin`과 역어셈블 덤프(`__dump.txt`, `__dump_all.txt`)를 생성하고,
`make run`은 STM32CubeProgrammer CLI로 SWD를 통해 보드에 기록합니다.

> `Makefile`은 `.gitignore`에 포함되어 저장소에는 없습니다. 로컬 툴체인 경로(`TOOL_DIR`)에 맞게 직접 준비해야 합니다.

## 파일 구성

| 파일 | 설명 |
|---|---|
| `main.c` | 시스템 초기화, 부저 BGM 시작 및 진입점 `Main()` |
| `buzzer.c` | 악보 데이터(`song1` BGM, `song2` 축하음), `Buzzer_Play_Song()`, 125ms 틱 핸들러 `Buzzer_BGM_1TICK()` |
| `led.c` | 내장 LED(`PA5`) 및 외부 연출용 LED(`PB3`~`PB6`) 제어 (`LED_Background`, `LED_Cong`, `LED_Background_Off`) |
| `motor.c` | 모터 방향/정지/반전/속도 제어 (`m_*` 함수) |
| `uart.c` | UART2 초기화, 인터럽트 설정 및 `Motor_UART_Command()` |
| `exception.c` | 버튼(`EXTI15_10_IRQHandler`), `USART2_IRQHandler`, `TIM2_IRQHandler`, `TIM4_IRQHandler` 인터럽트 핸들러 |
| `timer.c` | `TIM1`(부저 PWM), `TIM2`(스톱워치/딜레이), `TIM3`(모터 PWM), `TIM4`(125ms 주기 인터럽트) 드라이버 |
| `key.c` | 버튼 폴링 및 EXTI13 설정 |
| `runtime.c` | newlib 시스템콜 스텁 (`printf` → UART2) |
| `device_driver.h` | 전체 드라이버 함수 선언 |
| `option.h`, `macro.h` | 클럭/메모리 상수, 레지스터 비트 조작 매크로 |
