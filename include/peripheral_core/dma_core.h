///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DMA 
///////////////////////////////////////////////////////////////////////////////////////////////////



#pragma once
#include "sam.h"
#include "inttypes.h"
#include "initializer_list"

#include "SYS.h"



///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA VARS & DEFS
///////////////////////////////////////////////////////////////////////////////////////////////////

DmacDescriptor wbDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16)));
DmacDescriptor baseDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16))); 

#define DMA_IRQPRI_MAX 5 
#define DMA_IRQ_COUNT 5
#define DMA_PRILVL_COUNT 4

enum DMACH_STATE {
  DMACH_STATE_UNKNOWN,
  DMACH_STATE_DISABLED,
  DMACH_STATE_IDLE,
  DMACH_STATE_BUSY,
  DMACH_STATE_PEND,
};

enum DMACH_ERROR {
  DMACH_ERROR_NONE,
  DMACH_ERROR_UNKNOWN,
  DMACH_ERROR_CRC,
  DMACH_ERROR_DESC,
  DMACH_ERROR_TRANSFER
};

typedef void (*callbackFunction)(void) dmaErrorCallback;

#define IRQ_MAX_PRIORITY 4 // FIND SOME WAY TO GET THIS ACTUALLY

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  struct {
    bool enabled = false;
    bool roundRobinMode = false;
    int serviceQuality = 2;
  }priorityLvl[DMA_PRILVL_COUNT];

  uint8_t irq_priority_lvl = 1;
}config;


typedef struct dma_ctrl_enabled { 
  inline operator bool() {
    return static_cast<bool>(DMAC->CTRL.bit.DMAENABLE);
  }
  inline bool operator = (const bool value) {
    DMAC->CTRL.bit.DMAENABLE = 0;
    DMAC->CRCCTRL.reg &= ~DMAC_CRCCTRL_MASK;
    DMAC->CTRL.bit.SWRST = 1;
    while(DMAC->CTRL.bit.SWRST);
    if (value) {
      if (config.priorityLvl->serviceQuality > 3
        ||config.irq_priority_lvl > IRQ_MAX_PRIORITY) {
        return false;
      }
      for (int i = 0; i < DMA_IRQ_COUNT; i++) {
        NVIC_ClearPendingIRQ((IRQn_Type)(DMAC_0_IRQn + i));
        NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), 
          config.irq_priority_lvl);
        NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
      }
      for (int i = 0; i < DMA_PRILVL_COUNT; i++) {
        DMAC->CTRL.reg |= 
          ((uint8_t)config.priorityLvl[i].enabled 
            << (DMAC_CTRL_LVLEN0_Pos + i));    
        DMAC->PRICTRL0.reg |= 
          ((uint8_t)config.priorityLvl[i].roundRobinMode 
            << (DMAC_PRICTRL0_RRLVLEN0_Pos + i * 8))
        | ((uint8_t)config.priorityLvl[i].serviceQuality 
            << (DMAC_PRICTRL0_QOS0_Pos + i * 8));
      }
      DMAC->BASEADDR.reg = (uint32_t)baseDescArray;
      DMAC->WRBADDR.reg = (uint32_t)wbDescArray;
      DMAC->CTRL.bit.DMAENABLE = 1;
    }
    return true;
  }
};

typedef struct dma_active_remainingBytes{
  operator int() {
    uint32_t count = DMAC->ACTIVE.bit.BTCNT;
    if (DMAC->ACTIVE.bit.ABUSY || !count) {
      return 0;
    } else 
    return count / wbDescArray[DMAC->ACTIVE.bit.ID].BTCTRL.bit.BEATSIZE;
  }
};

typedef struct dma_active_id{
  inline operator int() { 
    return DMAC->ACTIVE.bit.ID; 
  }
};

typedef struct dma_active_busy{
  inline operator bool() { 
    return DMAC->ACTIVE.bit.ABUSY; 
  }
};

/////////// DMAC CHANNEL TYPEDEFS ///////////

typedef struct {


  bool operator = (void (*dmaErrorCallback)(void)) {

  } 

};







