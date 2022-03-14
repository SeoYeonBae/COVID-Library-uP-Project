/**
 * uP2 final project
 * 20175098
 * 배서연
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"

#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"

int i;
uint32_t ui32SysClock, timer;
volatile bool isAInUse = false, isBInUse = false, isCInUse = false, isDInUse = false, isTimerOn = false;
char ReceivedSeatData, sendData;
int seatATime, seatBTime, seatCTime, seatDTime;

void GPIO_Init(){

    ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 25000000);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

    IntMasterEnable();
}

void UART_Init(){

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    UARTConfigSetExpClk(UART0_BASE, ui32SysClock, 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                UART_CONFIG_PAR_NONE));
}

void Timer_Init(){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));

    GPIODirModeSet(GPIO_PORTJ_BASE,GPIO_PIN_0,GPIO_DIR_MODE_IN);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    TimerLoadSet(TIMER0_BASE,TIMER_A,ui32SysClock);

    IntEnable(INT_TIMER0A);
    IntEnable(INT_GPIOJ);

    TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);

    GPIOIntEnable(GPIO_PORTJ_BASE,GPIO_PIN_0);
    GPIOIntClear(GPIO_PORTJ_BASE,GPIO_PIN_0);

    IntEnable(INT_TIMER0A);
    IntEnable(INT_GPIOJ);
}

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while(ui32Count--)      // Loop while there are more characters to send.
    {
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);          // Write the next character to the UART.
    }
}

void Int_Timer0(){  // TIMER Interrupt 사용자의 좌석 시간 관리
    if(isTimerOn){
        if(isAInUse){
            seatATime--;
            if(seatATime == 0){
                UARTSend((uint8_t *)" SeatA Time out\n\r", 19);
                isAInUse = false;
                seatNotAvaliable('A');
            }
        }
        if(isBInUse){
            seatBTime--;
            if(seatBTime == 0){
                UARTSend((uint8_t *)" SeatB Time Out\n\r", 19);
                isBInUse = false;
                seatNotAvaliable('B');
            }
        }
        if(isCInUse){
            seatCTime--;
            if(seatCTime == 0){
                UARTSend((uint8_t *)" SeatC Time Out\n\r", 19);
                isCInUse = false;
                seatNotAvaliable('C');
            }
        }
        if(isDInUse){
            seatDTime--;
            if(seatDTime == 0){
                UARTSend((uint8_t *)" SeatD Time Out\n\r", 19);
                isDInUse = false;
                seatNotAvaliable('D');
            }
        }
    }
    TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
}

void Int_GPIOJ(){  // GPIO Port J Interrupt  버튼이 눌렸을 때 시간 부여 및 사용자에게 알림

    if(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)==0){
        if(ReceivedSeatData == 'A'){
            seatATime = 20;
            UARTSend((uint8_t *)" SeatA Time Set\n\r", 19);
        }else if(ReceivedSeatData == 'B'){
            UARTSend((uint8_t *)" SeatB Time Set\n\r", 19);
            seatBTime = 20;
        }else if(ReceivedSeatData == 'C'){
            UARTSend((uint8_t *)" SeatC Time Set\n\r", 19);
            seatCTime = 20;
        }else if(ReceivedSeatData == 'D'){
            UARTSend((uint8_t *)" SeatD Time Set\n\r", 19);
            seatDTime = 20;
        }
        isTimerOn = true;
    }
    UARTSend((uint8_t *)" Enter Seat: ", 13);
    for(i = 0; i<300000;i++);
    GPIOIntClear(GPIO_PORTJ_BASE,GPIO_PIN_0);
}

void seatAvailable(char seat){  // 사용 가능한 좌석 불 켜주는 함수

    if(seat == 'A'){
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_1,GPIO_PIN_1);
    }else if(seat == 'B'){
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_0,GPIO_PIN_0);
    }else if(seat == 'C'){
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_PIN_4);
    }else{
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_0,GPIO_PIN_0);
    }
    UARTSend((uint8_t *)"\n\r Press SW1 \n\r", 19);
}

void seatNotAvaliable(char seat){   // 사용 불가능한 좌석 알려 주고 불 끄는 함수
    if(seat == 'A'){
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_1,GPIO_PIN_1);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_1,0X0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_1,GPIO_PIN_1);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_1,0X0);
    }else if(seat == 'B'){
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_0,GPIO_PIN_0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_0,0X0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_0,GPIO_PIN_0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_0,0X0);
    }else if(seat == 'C'){
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_PIN_4);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_4,0X0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_PIN_4);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_4,0X0);
    }else{
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_0,GPIO_PIN_0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_0,0X0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_0,GPIO_PIN_0);
        for(i=0;i<500000;i++);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_0,0X0);
    }
    UARTSend((uint8_t *)"\n\r Enter Seat: ", 17);
}

int main(void)
{
    GPIO_Init();
    UART_Init();
    Timer_Init();

    isAInUse = false;
    isBInUse = false;
    isCInUse = false;
    isDInUse = false;

    UARTSend((uint8_t *)" Enter Seat: ", 13);

    while(1){

        if(UARTCharsAvail(UART0_BASE)){

            ReceivedSeatData = UARTCharGetNonBlocking(UART0_BASE);

            if(ReceivedSeatData == 'A'){
                if(!isBInUse){
                    isAInUse = true;
                    seatAvailable('A');
                }else{
                    seatNotAvaliable('A');
                }
            }else if(ReceivedSeatData == 'B'){
                if(!isAInUse && !isCInUse){
                    isBInUse = true;
                    seatAvailable('B');
                }else{
                    seatNotAvaliable('B');
                }
            }else if(ReceivedSeatData == 'C'){
                if(!isBInUse && !isDInUse){
                    isCInUse = true;
                    seatAvailable('C');
                }else{
                    seatNotAvaliable('C');
                }
            }else if(ReceivedSeatData == 'D'){
                if(!isCInUse){
                    isDInUse = true;
                    seatAvailable('D');
                }else{
                    seatNotAvaliable('D');
                }
            }
        }
    }
}
