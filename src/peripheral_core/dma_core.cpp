///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS INTERFACE - CPP
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <peripheral_core/dma_core.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: SYSTEM 
///////////////////////////////////////////////////////////////////////////////////////////////////

bool sys::reset() {
  DMAC->CTRL.bit.DMAENABLE = 0;
  DMAC->CRCCTRL.reg &= ~DMAC_CRCCTRL_MASK;
  DMAC->CTRL.bit.SWRST = 1;
  while(DMAC->CTRL.bit.SWRST);

  memset(wbDescArray, 0, sizeof(wbDescArray));
  memset(baseDescArray, 0, sizeof(baseDescArray));
  return true;
}

sys::enabled::operator bool() {
  return (bool)DMAC->CTRL.bit.DMAENABLE;
}

bool sys::enabled::operator = (const bool value) {
  if (value == (bool)DMAC->CTRL.bit.DMAENABLE) {
    return true;
  }
  if (value) {
    for (int i = 0; i < DMA_IRQ_COUNT; i++) {
      NVIC_ClearPendingIRQ((IRQn_Type)(DMAC_0_IRQn + i));
      NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), IRQ_PRILVL);
      NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
    }
    for (int i = 0; i < DMA_PRILVL_COUNT; i++) {
      DMAC->CTRL.reg |= 
        ((uint8_t)PRILVL_ENABLED[i] << (DMAC_CTRL_LVLEN0_Pos + i));    
      DMAC->PRICTRL0.reg |= 
        ((uint8_t)PRILVL_RR_MODE[i] << (DMAC_PRICTRL0_RRLVLEN0_Pos + i * 8))
      | ((uint8_t)PRILVL_SERVICE_QUAL[i] << (DMAC_PRICTRL0_QOS0_Pos + i * 8));
    }
    DMAC->BASEADDR.reg = (uint32_t)baseDescArray;
    DMAC->WRBADDR.reg = (uint32_t)wbDescArray;
    DMAC->CTRL.bit.DMAENABLE = 1;
  }
  return true;
}

sys::errorCallback::operator errorCBType() {
  return errorCB;
}

bool sys::errorCallback::operator = (const errorCBType errorCallback) {
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
  return true;
}

sys::transferCallback::operator transferCBType() {
  return transferCB;
}

bool sys::transferCallback::operator = (const transferCBType transferCallback) {
  transferCB = transferCallback; 
  if (transferCallback) {
    for (int i = 0; i < DMAC_CH_NUM; i++) {
      DMAC->Channel[i].CHINTENSET.bit.TCMPL = 1;
    }
  } else {
    for (int i = 0; i < DMAC_CH_NUM; i++) {
      DMAC->Channel[i].CHINTENCLR.bit.TCMPL = 1;
      DMAC->Channel[i].CHINTFLAG.bit.TCMPL = 1;
    }
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA ACTIVE CHANNEL 
///////////////////////////////////////////////////////////////////////////////////////////////////

activeChannel::remainingBytes::operator int() {
  uint32_t count = DMAC->ACTIVE.bit.BTCNT;
  if (DMAC->ACTIVE.bit.ABUSY || !count) {
    return 0;
  } else 
  return count / wbDescArray[DMAC->ACTIVE.bit.ID].BTCTRL.bit.BEATSIZE;
}

activeChannel::remainingBytes::operator int() {
  return DMAC->ACTIVE.bit.ID; 
}

activeChannel::busy::operator bool() {
  return DMAC->ACTIVE.bit.ABUSY; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA CHANNEL 
///////////////////////////////////////////////////////////////////////////////////////////////////

