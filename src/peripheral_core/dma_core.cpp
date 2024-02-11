///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS INTERFACE - CPP
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <peripheral_core/dma_core.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: SYSTEM 
///////////////////////////////////////////////////////////////////////////////////////////////////

sys::enabled::operator bool() {
  return (bool)DMAC->CTRL.bit.DMAENABLE;
}
void sys::enabled::operator = (const bool value) {
  if (value != (bool)DMAC->CTRL.bit.DMAENABLE) {
    DMAC->CTRL.bit.DMAENABLE = 0;
    DMAC->CRCCTRL.reg &= ~DMAC_CRCCTRL_MASK;
    DMAC->CTRL.bit.SWRST = 1;
    while(DMAC->CTRL.bit.SWRST);

    memset(wbDescArray, 0, sizeof(wbDescArray));
    memset(baseDescArray, 0, sizeof(baseDescArray));
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
  }
}

void sys::errorCallback::operator = (const errorCBType errorCallback) {
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
sys::errorCallback::operator errorCBType() {
  return errorCB;
}

void sys::transferCallback::operator = (const transferCBType transferCallback) {
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
}
sys::transferCallback::operator transferCBType() {
  return transferCB;
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

void channel::enabled::operator = (const bool value) {
  DMAC->Channel[super->index].CHCTRLA.bit.ENABLE = (uint8_t)value;
}
channel::enabled::operator bool() {
  return DMAC->Channel[super->index].CHCTRLA.bit.ENABLE;
}

bool channel::state::operator = (const CHANNEL_STATE value) {
  auto setSuspend = [&](int value) -> void {
    if (value > 0) {
      DMAC->Channel[super->index].CHCTRLB.bit.CMD == DMAC_CHCTRLB_CMD_SUSPEND_Val;
      while(DMAC->Channel[super->index].CHCTRLB.bit.CMD == DMAC_CHCTRLB_CMD_SUSPEND_Val);
    } else { 
      DMAC->Channel[super->index].CHINTFLAG.bit.SUSP = 1;   
      DMAC->Channel[super->index].CHCTRLB.bit.CMD 
        = value < 0 ? DMAC_CHCTRLB_CMD_RESUME_Val : DMAC_CHCTRLB_CMD_NOACT_Val;
    }
  };
  auto setEnabled = [&](bool value) -> void {
    if (value) {
      DMAC->Channel[super->index].CHCTRLA.bit.ENABLE = 1;
    } else {
      setSuspend(0); // May not be necessary...
      DMAC->Channel[super->index].CHCTRLA.bit.ENABLE = 0;
      while(DMAC->Channel[super->index].CHSTATUS.bit.BUSY);
    }
  };
  if (*this == value) {
    return true;
  } else if (value == STATE_PENDING) {
    return false;
  }
  switch(value) {
    case STATE_DISABLED: {
      setEnabled(false);
    }
    case STATE_IDLE: {
      setEnabled(false);
      setEnabled(true);
    }
    case STATE_SUSPENDED: {
      setEnabled(true);
      setSuspend(1);
    }
    case STATE_ACTIVE: {
      setEnabled(true);
      if (DMAC->Channel[super->index].CHINTFLAG.bit.SUSP) { // Ensure not suspended
        setSuspend(-1);
      } 
      if (!DMAC->Channel[super->index].CHSTATUS.bit.BUSY 
        && DMAC->Channel[super->index].CHSTATUS.bit.PEND) { // If not active (from resume) -> trigger
        DMAC->SWTRIGCTRL.reg |= (1 << super->index);
      }
    }
  }
  return true;
}

//                                                                      NEEDS TO BE REDONE!
channel::state::operator CHANNEL_STATE() {
  if (!DMAC->Channel[super->index].CHCTRLA.bit.ENABLE) {
    return STATE_DISABLED;
  } else if (DMAC->Channel[super->index].CHINTFLAG.bit.SUSP) {
    return STATE_SUSPENDED;
  } else if (DMAC->Channel[super->index].CHSTATUS.bit.BUSY
    || DMAC->Channel[super->index].CHSTATUS.bit.PEND) {
    //return STATE_ACTIVE;
  }
  return STATE_IDLE;    
}

void channel::triggerSource::operator = (const TRIGGER_SOURCE value) {
  DMAC->Channel[super->index].CHCTRLA.bit.TRIGSRC = (uint8_t)value;
}
channel::triggerSource::operator TRIGGER_SOURCE() {
  return static_cast<TRIGGER_SOURCE>
    (DMAC->Channel[super->index].CHCTRLA.bit.TRIGSRC);
}

void channel::triggerAction::operator = (const TRIGGER_ACTION value) {
  DMAC->Channel[super->index].CHCTRLA.bit.TRIGACT = (uint8_t)value;
}
channel::triggerAction::operator TRIGGER_ACTION() {
  return static_cast<TRIGGER_ACTION>
    (DMAC->Channel[super->index].CHCTRLA.bit.TRIGACT);
}





