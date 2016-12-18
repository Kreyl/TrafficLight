/*
 * ir.cpp
 *
 *  Created on: 04.07.2013
 *      Author: kreyl
 */

#include "ir.h"
#include "uart.h"

//#define DAC_CONST   0

void ir_t::Init() {
    // ==== GPIO ====
    // Once the DAC channel is enabled, the corresponding GPIO pin is automatically
    // connected to the DAC converter. In order to avoid parasitic consumption,
    // the GPIO pin should be configured in analog.
    PinSetupAnalog(LED_IR);
    // ==== DAC ====
    rccEnableAPB1(RCC_APB1ENR_DACEN, FALSE);
    // TIM7 TRGO evt as trigger, trigger enable, buffer disable
    DAC->CR = DAC_CR_EN1 | DAC_CR_DMAEN1 | (0b010 << 3) | DAC_CR_TEN1 | DAC_CR_BOFF1;
    // ZeroArr
    for(uint8_t i=0; i<CARRIER_PERIOD_CNT; i++) ZeroArr[i] = 0;
    // ==== DMA ====
    dmaStreamAllocate     (DAC_DMA, IRQ_PRIO_HIGH, NULL, NULL);
    dmaStreamSetPeripheral(DAC_DMA, &DAC->DHR12R1);
    dmaStreamSetMode      (DAC_DMA, IRLED_DMA_MODE);
    dmaStreamSetMemory0(DAC_DMA, ZeroArr);
    dmaStreamSetTransactionSize(DAC_DMA, CARRIER_PERIOD_CNT);
    // ==== Sampling timer ====
    SamplingTmr.Init();
    SamplingTmr.SetUpdateFrequencyChangingTopValue(SAMPLING_FREQ_HZ);
    SamplingTmr.SelectMasterMode(mmUpdate);
    SamplingTmr.Enable();

    // ==== Chunk timer ====
//    PinSetupOut(GPIOB, 7, omPushPull);  // DEBUG
    ChunkTmr.Init();
    ChunkTmr.SetupPrescaler(1000000);   // Freq = 1MHz => Period = 1us
    ChunkTmr.SetTopValue(999);          // Initial, do not care
    ChunkTmr.EnableIrqOnUpdate();
    nvicEnableVector(TMR_DAC_CHUNK_IRQ, IRQ_PRIO_HIGH);
}

void ir_t::TransmitWord(uint16_t wData, uint16_t DACValue) {
//    if(Busy) return;
    Busy = true;
    // ==== Fill buffer depending on data ====
    PChunk = TxBuf;
    *PChunk++ = {1, 2400}; // }
    *PChunk++ = {0, 600};  // } Header
    // Data
    for(uint8_t i=0; i<IR_BIT_CNT; i++) {
        if(wData & 0x8000) *PChunk++ = {1, 1200};  // 1
        else               *PChunk++ = {1, 600};   // 0
        *PChunk++ = {0, 600};                      // space
        wData <<= 1;
    }
//    for(uint8_t i=0; i<CHUNK_CNT; i++) Uart.Printf("%u %u\r", TxBuf[i].On, TxBuf[i].Duration);
//    Uart.Printf("\r");
    // ==== Start transmission ====
    PChunk = TxBuf;
    // Start chunk timer
    ChunkTmr.SetCounter(0);
    ChunkTmr.SetTopValue(PChunk->Duration);
    ChunkTmr.Enable();
//    Uart.Printf("TMr %u\r",
    // Fill carrier array
    CarrierArr[0] = DACValue;
    CarrierArr[1] = 0;
    IDacCarrierEnable();    // Start DAC-based carrier
}

void ir_t::IChunkTmrHandler() {
    ChunkTmr.ClearIrqPendingBit();
//    Uart.PrintfI("Tmr Irq\r");
    uint32_t LenSent = PChunk - TxBuf;
    // Check if last chunk
    if(LenSent >= CHUNK_CNT-1) {
        Busy = false;
        ChunkTmr.Disable();
        IDacCarrierDisable(); // Stop Dac
//        App.SignalEvt()
    }
    else {
        PChunk++;
        ChunkTmr.SetTopValue(PChunk->Duration);
        if(PChunk->On) {
            //PinSet(GPIOB, 7);
            IDacCarrierEnable();
        }
        else {
            //PinClear(GPIOB, 7);
            IDacCarrierDisable();
        }
    }
}

// ============================= Interrupts ====================================
extern "C" {
CH_IRQ_HANDLER(TMR_DAC_CHUNK_IRQ_HANDLER) {
    CH_IRQ_PROLOGUE();
    chSysLockFromISR();
    ir.IChunkTmrHandler();
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}
} // extern c

