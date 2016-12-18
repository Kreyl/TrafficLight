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

// EEAddresses
#define EE_ADDR_DEVICE_ID       0

App_t App;
ir_t ir;
Eeprom_t EE;
static uint8_t ISetID(int32_t NewID);
static void ReadIDfromEE();

TmrKL_t TmrIrTx(MS2ST(IR_TX_PERIOD_MS), EVT_TIME_TO_IRTX, tktPeriodic);

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
    Uart.Init(115200, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN);
    ReadIDfromEE();
    Uart.Printf("\r%S %S; ID=%u\r", APP_NAME , BUILD_TIME, App.ID);
    Clk.PrintFreqs();

    LedRed.Init();
    LedYellow.Init();
    LedStraight.Init();
    LedLeft.Init();
    LedRight.Init();

    ir.Init();
    TmrIrTx.InitAndStart();

    if(Rslt != OK) {
        LedRed.StartOrRestart(lsqFailureClk);
        chThdSleepMilliseconds(2700);
    }
    else if(Radio.Init() != OK) {
        LedRed.StartOrRestart(lsqFailureCC);
        chThdSleepMilliseconds(2700);
    }
    else {
        // Show that all is ok
        LedRed.StartOrRestart(lsqStart);
        chThdSleepMilliseconds(180);
        LedYellow.StartOrRestart(lsqStart);
        chThdSleepMilliseconds(180);
        LedLeft.StartOrRestart(lsqStart);
        chThdSleepMilliseconds(180);
        LedStraight.StartOrRestart(lsqStart);
        chThdSleepMilliseconds(180);
        LedRight.StartOrRestart(lsqStart);
    }

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
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

#if UART_RX_ENABLED
        if(Evt & EVT_UART_NEW_CMD) {
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

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", App.ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        uint8_t r = ISetID(dw32);
        PShell->Ack(r);
    }

    else PShell->Ack(CMD_UNKNOWN);
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    App.ID = EE.Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(App.ID < ID_MIN or App.ID > ID_MAX) {
        Uart.Printf("\rUsing default ID\r");
        App.ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return FAILURE;
    uint8_t rslt = EE.Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == OK) {
        App.ID = NewID;
        Uart.Printf("New ID: %u\r", App.ID);
        return OK;
    }
    else {
        Uart.Printf("EE error: %u\r", rslt);
        return FAILURE;
    }
}
#endif
