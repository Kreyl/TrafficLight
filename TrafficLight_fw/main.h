/*
 * main.h
 *
 *  Created on: 15 ����. 2014 �.
 *      Author: g.kruglov
 */

#pragma once

#include "ch.h"
#include "kl_lib.h"
#include "uart.h"
#include "evt_mask.h"
#include "board.h"
#include "kl_buf.h"
#include "rlvl1_defins.h"

#define APP_NAME                "TrafficLight"
// ==== Constants and default values ====
#define ID_MIN                  1
#define ID_MAX                  36
#define ID_DEFAULT              ID_MIN

#define CMD_Q_LEN               27

#define IR_TX_PERIOD_MS         50

class App_t {
private:
    thread_t *PThread;
    State_t State;
public:
    uint8_t ID;
    CircBuf_t<State_t, CMD_Q_LEN> CmdQ;
    // Eternal methods
    void InitThread() { PThread = chThdGetSelfX(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    void OnCmd(Shell_t *PShell);
    // Inner use
    void ITask();
};

extern App_t App;
