/*
 * ir.h
 *
 *  Created on: 04.07.2013
 *      Author: kreyl
 */

#pragma once

#include "hal.h"
#include "ch.h"
#include "kl_lib.h"
#include "board.h"

#define IR_CARRIER_HZ       56000
#define IR_BIT_CNT          12
#define MAX_PWR             4005     // Top DAC value

// Delays, uS
#define IR_TICK_US          600 // Protocol smallest time unit, us
/* Header = 4 * IR_TICK_US
 * Space  = 1 * IR_TICK_US
 * Zero   = 1 * IR_TICK_US
 * One    = 2 * IR_TICK_US
 */

// Timings
#define IR_HEADER_US        2400
#define IR_ZERO_US          600
#define IR_ONE_US           1200

struct IrChunk_t {
    uint8_t On;
    uint16_t Duration;
};
#define CHUNK_CNT   (1+1+(IR_BIT_CNT*2))    // Header + bit count

#define CARRIER_PERIOD_CNT  2
#define SAMPLING_FREQ_HZ    (CARRIER_PERIOD_CNT*IR_CARRIER_HZ)

#define IRLED_DMA_MODE  \
    DMA_PRIORITY_HIGH | \
    STM32_DMA_CR_MSIZE_HWORD | \
    STM32_DMA_CR_PSIZE_HWORD | \
    STM32_DMA_CR_MINC        | \
    STM32_DMA_CR_DIR_M2P     | \
    STM32_DMA_CR_CIRC

class ir_t {
private:
    IrChunk_t TxBuf[CHUNK_CNT], *PChunk; // Buffer of power values: header + all one's + 1 delay after
    Timer_t ChunkTmr{TMR_DAC_CHUNK};
    uint16_t CarrierArr[CARRIER_PERIOD_CNT], ZeroArr[CARRIER_PERIOD_CNT];
    Timer_t SamplingTmr{TMR_DAC_SMPL};
    inline void IDacCarrierDisable() {
        dmaStreamDisable(DAC_DMA);
        dmaStreamSetMemory0(DAC_DMA, ZeroArr);
        dmaStreamSetTransactionSize(DAC_DMA, CARRIER_PERIOD_CNT);
        dmaStreamSetMode(DAC_DMA, IRLED_DMA_MODE);
        dmaStreamEnable(DAC_DMA);
    }
    inline void IDacCarrierEnable() {
        dmaStreamDisable(DAC_DMA);
        dmaStreamSetMemory0(DAC_DMA, CarrierArr);
        dmaStreamSetTransactionSize(DAC_DMA, CARRIER_PERIOD_CNT);
        dmaStreamSetMode(DAC_DMA, IRLED_DMA_MODE);
        dmaStreamEnable(DAC_DMA);
    }
public:
    bool Busy;
    void Init();
    void TransmitWord(uint16_t wData, uint16_t DACValue);
    // Inner use
    void IChunkTmrHandler();
};

extern ir_t ir;
