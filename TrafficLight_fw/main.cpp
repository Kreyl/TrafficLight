/*
 * main.cpp
 *
 *  Created on: 20 ����. 2014 �.
 *      Author: g.kruglov
 */

#include "main.h"
#include "board.h"
#include "led.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "Sequences.h"

App_t App;

LedSmooth_t LedRed(LED_RED);
LedSmooth_t LedYellow(LED_YELLOW);
LedSmooth_t LedStraight(LED_STRAIGHT);
LedSmooth_t LedLeft(LED_LEFT);
LedSmooth_t LedRight(LED_RIGHT);

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    uint8_t Rslt = Clk.SwitchToHSE();
    Clk.UpdateFreqValues();

    // Init OS
    halInit();
    chSysInit();
    App.InitThread();

    // ==== Init hardware ====
    App.ID = App.GetDipSwitch();
    Uart.Init(115200, UART_GPIO, UART_TX_PIN);//, UART_GPIO, UART_RX_PIN);
    Uart.Printf("\r%S %S; ID=%u\r", APP_NAME , BUILD_TIME, App.ID);
    Clk.PrintFreqs();

    LedRed.Init();
    LedYellow.Init();
    LedStraight.Init();
    LedLeft.Init();
    LedRight.Init();

    if(Rslt != OK) {
        LedRed.StartSequence(lsqFailure);
        chThdSleepMilliseconds(2700);
    }
    else if(Radio.Init() != OK) {
        LedYellow.StartSequence(lsqFailure);
        chThdSleepMilliseconds(2700);
    }
    else {
        // Show that all is ok
        LedRed.StartSequence(lsqStart);
        chThdSleepMilliseconds(180);
        LedYellow.StartSequence(lsqStart);
        chThdSleepMilliseconds(180);
        LedLeft.StartSequence(lsqStart);
        chThdSleepMilliseconds(180);
        LedStraight.StartSequence(lsqStart);
        chThdSleepMilliseconds(180);
        LedRight.StartSequence(lsqStart);
    }

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
#if UART_RX_ENABLED
        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

    } // while true
}

#if UART_RX_ENABLED // ======================= Command processing ============================
void App_t::OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(OK);
    }

    else PShell->Ack(CMD_UNKNOWN);
}
#endif

const PortPin_t DipPins[6] = {DIPSW_PIN1, DIPSW_PIN2, DIPSW_PIN3, DIPSW_PIN4, DIPSW_PIN5, DIPSW_PIN6};
uint8_t App_t::GetDipSwitch() {
    uint8_t Rslt = 0;
    for(int i=5; i>=0; i--) {
        PinSetupIn(DipPins[i], pudPullUp);
        __NOP(); __NOP(); __NOP(); __NOP();   // Let it to stabilize
        Rslt <<= 1;
        if(PinIsClear(DipPins[i])) Rslt |= 1;
        PinSetupAnalog(DipPins[i]);
    }
    return Rslt;
}
