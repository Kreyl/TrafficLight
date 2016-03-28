/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "main.h"
#include "board.h"
#include "led.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "Sequences.h"
#include "ir.h"

App_t App;
ir_t ir;
TmrKL_t TmrIrTx;

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

    ir.Init();
    TmrIrTx.InitAndStart(chThdGetSelfX(), MS2ST(IR_TX_PERIOD_MS), EVT_TIME_TO_IRTX, tktPeriodic);

    if(Rslt != OK) {
        LedRed.StartSequence(lsqFailureClk);
        chThdSleepMilliseconds(2700);
    }
    else if(Radio.Init() != OK) {
        LedRed.StartSequence(lsqFailureCC);
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

#if 1 // ==== Radio cmd ====
        if(Evt & EVT_RADIO_NEW_CMD) {
            CmdQ.Get(&State);
            LedRed.SetBrightness(State.Brightness[0]);
            LedYellow.SetBrightness(State.Brightness[1]);
            LedStraight.SetBrightness(State.Brightness[2]);
            LedLeft.SetBrightness(State.Brightness[3]);
            LedRight.SetBrightness(State.Brightness[4]);
        }
#endif

#if 1 // ==== IR TX ====
        if(Evt & EVT_TIME_TO_IRTX) {
            if(State.IRPwr != 0) {
                uint16_t Data = State.IRData;
                Data <<= 8;
                uint16_t Pwr = State.IRPwr;
                Pwr = Pwr * 10 + 1000;
                ir.TransmitWord(Data, Pwr);
            }
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
