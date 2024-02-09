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

#define _nocpy_(_name_) private: _name_(){}

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

////////////// DMA DEFAULT CONFIGS /////////////

#define DMA_DEFAULT_PRILVL 


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  bool priority_lvl_enabled[DMA_PRILVL_COUNT] = { true };
  uint8_t irq_priority_lvl = 1;
  uint8_t service_quality = 0;
}dma_config;

struct { 
 
  struct { 

    /// @brief Enables/disables the dma system.
    /// @param bool -> Set true to reset & enable dma, or false 
    ///                reset & disable it.
    /// @return bool -> True if dma is enabled, false otherwise.
    struct { 
      inline operator bool() {
        return static_cast<bool>(DMAC->CTRL.bit.DMAENABLE);
      }
      inline bool operator = (const bool value) {
        DMAC->CTRL.bit.DMAENABLE = 0;
        DMAC->CRCCTRL.reg &= ~DMAC_CRCCTRL_MASK;
        DMAC->CTRL.bit.SWRST = 1;
        while(DMAC->CTRL.bit.SWRST);
        if (value) {
          for (int i = 0; i < DMA_IRQ_COUNT; i++) {
            NVIC_ClearPendingIRQ((IRQn_Type)(DMAC_0_IRQn + i));
            NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), 
              dma_config.irq_priority_lvl);
            NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
          }
          for (int i = 0; i < DMA_PRILVL_COUNT; i++) {
            DMAC->CTRL.reg |= (dma_config.priority_lvl_enabled[i] 
              << DMAC_CTRL_LVLEN0_Pos + i);            
          }
          DMAC->BASEADDR.reg = (uintptr_t)baseDescArray;
          DMAC->WRBADDR.reg = (uintptr_t)wbDescArray;
          // DMAC->PRICTRL0.bit
        }
        return true;
      }
    }enabled;



  }sys;

  struct {


  }channel;

  struct {

  }crc;

  struct {

  }utils;

  struct {
   
    struct {
      inline operator bool() { 
        return DMAC->ACTIVE.bit.ABUSY; 
      }
    }busy;

    struct {
      inline operator int() { 
        return DMAC->ACTIVE.bit.ID; 
      }
    }number;

    struct {
      operator int() {
        uint32_t count = DMAC->ACTIVE.bit.BTCNT;
        if (DMAC->ACTIVE.bit.ABUSY || !count) {
          return 0;
        } else 
        return count / wbDescArray[dma.active_channel.number].BTCTRL.bit.BEATSIZE;
      }
    }remaining_bytes;   

  }active_channel;

}dma;





