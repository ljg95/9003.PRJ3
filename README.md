# STM32F411 DC 모터 제어 펌웨어

STM32F411RE(Cortex-M4) 보드에서 DC 모터의 **회전 방향(정회전/역회전/정지)** 과 **속도(PWM duty)** 를
**UART 명령** 과 **사용자 버튼(PC13)** 으로 제어하는 베어메탈 펌웨어입니다.
모든 동작은 인터럽트로 처리되고, `Main()` 의 메인 루프는 비어 있습니다.

## 동작 흐름 (`main.c`)

```c
void Main(void)
{
    Sys_Init(115200);               // FPU, 클럭, UART2, LED 초기화
    m_init();                       // 모터 GPIO + PWM(TIM3) 초기화
    Key_ISR_Enable(1);              // 버튼(PC13) EXTI 인터럽트 활성화
    Uart2_RX_Interrupt_Enable(1);   // UART2 수신 인터럽트 활성화

    for(;;){ }                      // 이후 모든 처리는 ISR에서 수행
}
```

| 단계 | 함수 | 내용 |
|---|---|---|
| 1 | `Sys_Init(115200)` | FPU(CP10/CP11) 활성화 → `Clock_Init()`(SYSCLK 96MHz) → `Uart2_Init(115200)` → `stdout` 버퍼링 해제(`printf` 즉시 출력) → `LED_Init()` |
| 2 | `m_init()` | PA0/PA1을 출력으로 설정하고 모터 정지, TIM3 CH3(PB0) PWM 1kHz·duty 50%로 시작 |
| 3 | `Key_ISR_Enable(1)` | PC13을 EXTI13으로 연결, rising/falling 양쪽 엣지 인터럽트(IRQ 40) 활성화 |
| 4 | `Uart2_RX_Interrupt_Enable(1)` | USART2 RXNE 인터럽트(IRQ 38) 활성화 |
| 5 | `for(;;)` | 대기. 실제 제어는 `USART2_IRQHandler`, `EXTI15_10_IRQHandler`, `TIM2_IRQHandler`에서 처리 |

## 핀 배치

| 핀 | 용도 |
|---|---|
| PA0, PA1 | L293 방향 입력 1A(2번 핀), 2A(7번 핀) — Push-Pull 출력 |
| PB0 | TIM3_CH3 PWM 출력 → L293 1,2EN(1번 핀), 모터 속도 제어 (AF2) |
| PA2 / PA3 | USART2 TX / RX (115200 bps, 8N1) — ST-LINK 가상 COM 포트 |
| PA5 | 상태 LED (모터 정회전 시 ON) |
| PC13 | 사용자 버튼 (누르면 Low) |

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
   |            GND |---+--------------->|  4, 5, 12, 13  GND        |
   +----------------+   +--------------->|  9  3,4EN  (unused)       |
                                         | 10, 11, 14, 15  NC        |
                                         +---------------------------+
```

| L293 핀 | 이름 | 연결 |
|---|---|---|
| 1 | 1,2EN | PB0 (TIM3_CH3 PWM, 1kHz) |
| 2 | 1A | PA0 |
| 3 | 1Y | 모터 단자 A |
| 4, 5, 12, 13 | GND (방열) | GND |
| 6 | 2Y | 모터 단자 B |
| 7 | 2A | PA1 |
| 8 | VCC2 | 5V (모터 전원, 로직 전원과 분리) |
| 9 | 3,4EN | GND (3·4번 채널 미사용) |
| 10, 11, 14, 15 | 3A, 3Y, 4Y, 4A | 미연결 |
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

## 사용법

### UART 명령 (115200 bps)

터미널에서 한 글자를 보내면 `Motor_UART_Command()`가 처리합니다(대소문자 구분 없음).

| 입력 | 동작 | 출력 메시지 |
|---|---|---|
| `F` / `f` | 정지 → 500ms 대기 → 정회전(CW), LED ON | `Motor: Forward (CW)` |
| `R` / `r` | 정지 → 500ms 대기 → 역회전(CCW), LED OFF | `Motor: Reverse (CCW)` |
| `S` / `s` | 정지, LED OFF | `Motor: Stop` |
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
| **롱클릭** (2초 이상 누름) | 회전 중이면 정지 + LED OFF / 정지 상태면 속도 1단계로 정회전 시작 + LED ON |
| **숏클릭** (20ms ~ 2초) | 회전 방향 반전 (CW ↔ CCW, 정지 → 500ms 대기 → 반대 방향) |
| 20ms 미만 | 채터링으로 간주하여 무시 |

버튼을 누르면(falling) `TIM2_Stopwatch_Start(2000)`으로 2초 타이머가 시작됩니다.
- 2초가 지나도 계속 누르고 있으면 `TIM2_IRQHandler`에서 롱클릭으로 처리합니다.
- 2초 전에 떼면(rising) `EXTI15_10_IRQHandler`에서 경과 시간을 확인해 숏클릭으로 처리합니다.

## 빌드 및 다운로드

- 툴체인: Arm GNU Toolchain 15.2.rel1 (`arm-none-eabi`)
- 링커 스크립트: `rom_0x08000000.lds` (Flash 0x08000000 시작)
- 시작 코드: `crt0.s` → `Main()` 호출

```powershell
make
```

```powershell
make run
```

`make`는 `rom_0x08000000.elf` / `.bin`과 역어셈블 덤프(`__dump.txt`, `__dump_all.txt`)를 생성하고,
`make run`은 STM32CubeProgrammer CLI로 SWD를 통해 보드에 기록합니다.

> `Makefile`은 `.gitignore`에 포함되어 저장소에는 없습니다. 로컬 툴체인 경로(`TOOL_DIR`)에 맞게 직접 준비해야 합니다.

## 파일 구성

| 파일 | 설명 |
|---|---|
| `main.c` | 시스템 초기화 및 진입점 `Main()` |
| `motor.c` | 모터 방향/정지/반전/속도 제어 (`m_*` 함수) |
| `uart.c` | UART1/2 드라이버, USART2 수신 ISR, `Motor_UART_Command()` |
| `exception.c` | 버튼(EXTI13), TIM2, TIM4, DMA 인터럽트 핸들러 |
| `timer.c` | TIM2(스톱워치/딜레이), TIM3(PWM), TIM4(딜레이/반복) |
| `key.c` | 버튼 폴링 및 EXTI 설정 |
| `led.c` | PA5 LED 제어 |
| `runtime.c` | newlib 시스템콜 스텁 (`printf` → UART) |
| `device_driver.h` | 전체 드라이버 함수 선언 |
| `option.h`, `macro.h` | 클럭/메모리 상수, 레지스터 비트 조작 매크로 |
