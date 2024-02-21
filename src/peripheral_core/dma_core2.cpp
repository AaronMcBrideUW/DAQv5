
#include "peripheral_core/dma_core2.h"

/// @internal DMA storage variables
static volatile __attribute__ ((section(".hsram"))) __attribute__ ((aligned (16))) 
  DmacDescriptor wbDescArray[DMAC_CH_NUM];

static volatile __attribute__ ((section(".hsram"))) __attribute__ ((aligned (16))) 
  DmacDescriptor baseDescArray[DMAC_CH_NUM]; 



static core::dma::errorCallbackType errorCB = nullptr;
static core::dma::transferCallbackType transferCB = nullptr;


namespace core {

  /***********************************************************************************************/
  /// SECTION: DMA CONTROL 

  bool dma::ctrlGroup::init(const bool &value) {
    DMAC->CTRL.bit.DMAENABLE = 0;
    while(DMAC->CTRL.bit.DMAENABLE);

    for (int i = 0; i < DMA_IRQ_COUNT; i++) {
      NVIC_DisableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
      NVIC_ClearPendingIRQ((IRQn_Type)(DMAC_0_IRQn + i));
    }
    if (value) {
      for (int i = 0; i < DMA_IRQ_COUNT; i++) {
        NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), dma::configGroup::irqPriority);
        NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
      }
    }
    DMAC->CRCCTRL.reg &= ~DMAC_CRCCTRL_MASK;
    DMAC->CTRL.bit.SWRST = 1;
    while(DMAC->CTRL.bit.SWRST);

    memset((void*)wbDescArray, 0, sizeof(wbDescArray));
    memset((void*)baseDescArray, 0, sizeof(baseDescArray));
    DMAC->BASEADDR.reg = (uint32_t)baseDescArray;
    DMAC->WRBADDR.reg = (uint32_t)wbDescArray;

    if (value) {
      for (int i = 0; i < DMA_PRILVL_COUNT; i++) {
        DMAC->CTRL.reg |= 
          (dma::configGroup::prilvl_enabled[i] << (DMAC_CTRL_LVLEN0_Pos + i));    
        DMAC->PRICTRL0.reg |= 
          (dma::configGroup::prilvl_rr_mode[i] << (DMAC_PRICTRL0_RRLVLEN0_Pos + i * 8))
        | (dma::configGroup::prilvl_service_qual[i] << (DMAC_PRICTRL0_QOS0_Pos + i * 8));
      }
    }
    return true;
  }
  bool dma::ctrlGroup::init() {
    return DMAC->BASEADDR.bit.BASEADDR && DMAC->WRBADDR.bit.WRBADDR;
  }

  bool dma::ctrlGroup::enabled(const bool &value) {
    if (!dma::ctrlGroup::init()) {
      return false;
    }
    DMAC->CTRL.bit.DMAENABLE = value;
    while(DMAC->CTRL.bit.DMAENABLE != value);
    return true;
  } 
  bool dma::ctrlGroup::enabled() {
    return DMAC->CTRL.bit.DMAENABLE;
  }

  bool dma::ctrlGroup::errorCallback(errorCallbackType &errorCallback) {
    errorCB = errorCallback;
    if (errorCallback) {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENSET.bit.TERR = 1;
      }
    } else {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTFLAG.bit.TERR = 1;
        DMAC->Channel[i].CHINTENCLR.bit.TERR = 1;
      }
    }
  }
  bool dma::ctrlGroup::errorCallback() {
    return errorCB;
  }

  bool dma::ctrlGroup::transferCallback(transferCallbackType &transferCallback) {
    transferCB = transferCallback; 
    if (transferCallback && !transferCB) {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENSET.bit.TCMPL = 1;
      }
    } else if (!transferCallback) {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENCLR.bit.TCMPL = 1;
        DMAC->Channel[i].CHINTFLAG.bit.TCMPL = 1;
      }
    }
    return true;
  }
  bool dma::ctrlGroup::transferCallback() {
    return transferCB;
  }

  /***********************************************************************************************/
  /// SECTION: ACTIVE CHANNEL 

  int dma::activeChannelGroup::remainingBytes() {
    return DMAC->ACTIVE.bit.BTCNT * DMAC->ACTIVE.bit.ABUSY
      * wbDescArray[DMAC->ACTIVE.bit.ID].BTCTRL.bit.BEATSIZE;
  }

  int dma::activeChannelGroup::index() {
    return DMAC->ACTIVE.bit.ID; 
  }

  bool dma::activeChannelGroup::isBusy() {
    return DMAC->ACTIVE.bit.ABUSY; 
  }

  /***********************************************************************************************/
  /// SECTION: CHANNEL

  bool dma::channelGroup::init(const bool &value) {
    DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
    while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
    DMAC->Channel[index].CHCTRLA.bit.SWRST = 1;
    while(DMAC->Channel[index].CHCTRLA.bit.SWRST);

    memset((void*)&baseDescArray[index], 0, sizeof(DmacDescriptor));
    memset((void*)&wbDescArray[index], 0, sizeof(DmacDescriptor));
    if (value) {
      DMAC->Channel[index].CHCTRLA.bit.RUNSTDBY 
        = configGroup::chRunStandby[index];
      DMAC->Channel[index].CHPRILVL.bit.PRILVL
        = configGroup::chPrilvl[index];
      DMAC->Channel[index].CHINTENSET.reg |=
          ((int)errorCB << DMAC_CHINTENSET_TERR_Pos)
        | ((int)transferCB << DMAC_CHINTENSET_TCMPL_Pos);
    }
    return true;
  } 
  bool dma::channelGroup::init() {
    return memcmp((void*)&baseDescArray[index], (void*)&wbDescArray[index], 
      sizeof(DmacDescriptor)) || DMAC->Channel[index].CHCTRLA.reg 
      != DMAC_CHCTRLA_RESETVALUE;
  }

  bool dma::channelGroup::state(const CHANNEL_STATE &value) {    
    if (value == this->state()) {
      return true;
    }
    switch(value) {
      case STATE_DISABLED: { // DONE
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
        while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
        DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
        DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
        return true;
      }
      case STATE_IDLE: { // DONE
        if (DMAC->Channel[index].CHSTATUS.reg & (DMAC_CHSTATUS_BUSY 
          | DMAC_CHSTATUS_PEND)) {
          DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
          while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
        }
        DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
        DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
        return true;
      }
      case STATE_SUSPENDED: { // DONE
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
        DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
        DMAC->Channel[index].CHCTRLB.bit.CMD == DMAC_CHCTRLB_CMD_SUSPEND_Val;
        while(!DMAC->Channel[index].CHINTFLAG.bit.SUSP);
        return true;
      }
      case STATE_ACTIVE: { // DONE
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
        DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
        DMAC->Channel[index].CHINTFLAG.reg = DMAC_CHINTFLAG_RESETVALUE;
        DMAC->SWTRIGCTRL.reg |= (1 << index);
        return true;
      }
      default: {
        return false;
      }
    }
  }
  dma::CHANNEL_STATE dma::channelGroup::state() {
    if (!DMAC->Channel[index].CHCTRLA.bit.ENABLE) {
      return STATE_DISABLED;
    } else if (DMAC->Channel[index].CHINTFLAG.bit.SUSP) {
      return STATE_SUSPENDED;
    } else if (DMAC->Channel[index].CHSTATUS.bit.BUSY 
      || DMAC->Channel[index].CHSTATUS.bit.PEND) {
      return STATE_ACTIVE;
    }
    return STATE_IDLE;    
  }


  dma::CHANNEL_ERROR dma::channelGroup::error() {
    if (DMAC->Channel[index].CHINTFLAG.bit.TERR) {
      if (DMAC->Channel[index].CHSTATUS.bit.FERR) {
        return ERROR_DESC;
      }
      return ERROR_TRANSFER;
    } else if (DMAC->Channel[index].CHSTATUS.bit.CRCERR) {
      return ERROR_CRC;
    } 
    return ERROR_NONE;
  }

  bool dma::channelGroup::triggerSource(const TRIGGER_SOURCE &value) {
    DMAC->Channel[index].CHCTRLA.bit.TRIGSRC = value;
    return DMAC->Channel[index].CHCTRLA.bit.TRIGSRC == value;
  }
  dma::TRIGGER_SOURCE dma::channelGroup::triggerSource() {
    return static_cast<TRIGGER_SOURCE>(DMAC->Channel[index]
      .CHCTRLA.bit.TRIGSRC);
  }

  bool dma::channelGroup::transferType(const TRANSFER_TYPE &value) {
    DMAC->Channel[index].CHCTRLA.bit.TRIGACT = value;
    return DMAC->Channel[index].CHCTRLA.bit.TRIGACT == value;
  }
  dma::TRANSFER_TYPE dma::channelGroup::transferType() {
    return static_cast<TRANSFER_TYPE>(DMAC->Channel[index]
      .CHCTRLA.bit.TRIGACT);
  }

  bool dma::channelGroup::burstThreshold(const int &value) {
    if (value == 16) {
      DMAC->Channel[index].CHCTRLA.bit.BURSTLEN 
        = DMAC_CHCTRLA_BURSTLEN_16BEAT_Val;
      DMAC->Channel[index].CHCTRLA.bit.THRESHOLD 
        = DMAC_CHCTRLA_THRESHOLD_8BEATS_Val;
      return DMAC->Channel[index].CHCTRLA.bit.BURSTLEN 
        == DMAC_CHCTRLA_BURSTLEN_16BEAT_Val;
    }
    static const int THRESH_REF[] = {1, 2, 4, 8};
    for (int i = 0; i < sizeof(THRESH_REF) / sizeof(THRESH_REF[0]); i++) {
      if (value == THRESH_REF[i]) {
        DMAC->Channel[index].CHCTRLA.bit.THRESHOLD = i;
        DMAC->Channel[index].CHCTRLA.bit.BURSTLEN = value - 1;
        return DMAC->Channel[index].CHCTRLA.bit.BURSTLEN == value - 1;
      }    
    }
    return false;
  }

  int dma::channelGroup::burstThreshold() {
    return DMAC->Channel[index].CHCTRLA.bit.BURSTLEN + 1 
      - DMAC_CHCTRLA_BURSTLEN_SINGLE_Val;
  }

  /***********************************************************************************************/
  /// SECTION: DESCRIPTOR

  bool dma::transferDescriptor::source(const void *ptr) {
    uintptr_t address = (uintptr_t)ptr;
    if (!ptr || address == DMAC_SRCADDR_RESETVALUE) {
      return false;
    }
    sourceAddr = address;
    if (desc.BTCTRL.bit.STEPSEL == 0) {
      desc.SRCADDR.bit.SRCADDR = sourceAddr + desc.BTCNT.bit.BTCNT 
      * (desc.BTCTRL.bit.BEATSIZE + 1) * (1 << desc.BTCTRL.bit.STEPSIZE);
    } else {
      desc.SRCADDR.bit.SRCADDR = sourceAddr + desc.BTCNT.bit.BTCNT 
      * (desc.BTCTRL.bit.BEATSIZE + 1);
    }
    return true; 
  }
  bool dma::transferDescriptor::source(const volatile void *ptr) {
    uintptr_t address = (uintptr_t)ptr;
    if (!ptr || address != DMAC_SRCADDR_RESETVALUE) {
      return false;
    }
    sourceAddr = address;
    desc.SRCADDR.bit.SRCADDR = sourceAddr;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
    return true;
  }
  void *dma::transferDescriptor::source() {
    return (void*)sourceAddr;
  }


  bool dma::transferDescriptor::destination(const void *ptr) {
    uintptr_t address = (uintptr_t)ptr;
    if (!ptr || address = DMAC_SRCADDR_RESETVALUE) {
      return false;
    }
    destAddr = address;
    if (desc.BTCTRL.bit.STEPSEL == 0) {
      desc.DSTADDR.bit.DSTADDR = destAddr + desc.BTCNT.bit.BTCNT 
      * (desc.BTCTRL.bit.BEATSIZE + 1) * (1 << desc.BTCTRL.bit.STEPSIZE);
    } else {
      desc.DSTADDR.bit.DSTADDR = destAddr + desc.BTCNT.bit.BTCNT 
      * (desc.BTCTRL.bit.BEATSIZE + 1);
    }
    return true;
  }
  bool dma::transferDescriptor::destination(const volatile void *ptr) {
    uintptr_t address = (uintptr_t)ptr;
    if (!ptr || address = DMAC_SRCADDR_RESETVALUE) {
      return false;
    }
    destAddr = address;
    desc.DSTADDR.bit.DSTADDR = address;
    return true;
  }
  void *dma::transferDescriptor::destination() {
    return (void*)destAddr;
  }







}




