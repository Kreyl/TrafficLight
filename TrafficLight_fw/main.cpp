#include "board.h"
#include "led.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "Sequences.h"
#include "ir.h"
#include "MsgQ.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

#define EE_ADDR_DEVICE_ID       0
#define IR_TX_PERIOD_MS         50
#define ID_MIN                  1
#define ID_MAX                  15
#define ID_DEFAULT              ID_MIN

ir_t ir;
int32_t ID;
static uint8_t ISetID(int32_t NewID);
void ReadIDfromEE();

static TmrKL_t TmrIrTx {MS2ST(IR_TX_PERIOD_MS), evtIdTimeToTxIR, tktPeriodic};

LedSmooth_t LedRed(LED_RED);
LedSmooth_t LedYellow(LED_YELLOW);
LedSmooth_t LedStraight(LED_STRAIGHT);
LedSmooth_t LedLeft(LED_LEFT);
LedSmooth_t LedRight(LED_RIGHT);

State_t State;
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    uint8_t Rslt = Clk.SwitchToHSE();
    Clk.UpdateFreqValues();

    // Init OS
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
    Uart.EnableRx();
    ReadIDfromEE();
    Printf("\r%S %S; ID=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), ID);
    Clk.PrintFreqs();

    LedRed.Init();
    LedYellow.Init();
    LedStraight.Init();
    LedLeft.Init();
    LedRight.Init();

//    ir.Init();
    TmrIrTx.StartOrRestart();

    if(Rslt != retvOk) {
        LedRed.StartOrRestart(lsqFailureClk);
        chThdSleepMilliseconds(2700);
    }
    else if(Radio.Init() != retvOk) {
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
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            case evtIdRadioCmd:
                LedRed.SetBrightness(State.Brightness[0]);
                LedYellow.SetBrightness(State.Brightness[1]);
                LedStraight.SetBrightness(State.Brightness[2]);
                LedLeft.SetBrightness(State.Brightness[3]);
                LedRight.SetBrightness(State.Brightness[4]);
                break;

        case evtIdTimeToTxIR:
//            if(State.IRPwr != 0) {
//                uint16_t Data = State.IRData;
//                Data <<= 8;
//                uint16_t Pwr = State.IRPwr;
//                Pwr = Pwr * 10 + 1000;
//                ir.TransmitWord(Data, Pwr);
//            }
            break;
        } // switch
    } // while true
}

#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ack(retvOk);

    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNext<int32_t>(&ID) != retvOk) { PShell->Ack(retvCmdError); return; }
        uint8_t r = ISetID(ID);
        PShell->Ack(r);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    ID = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) {
        Printf("\rUsing default ID\r");
        ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return retvFail;
    uint8_t rslt = EE::Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == retvOk) {
        ID = NewID;
        Printf("New ID: %u\r", ID);
        return retvOk;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retvFail;
    }
}
#endif
