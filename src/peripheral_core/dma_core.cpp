/*
///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS INTERFACE - CPP
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <peripheral_core/dma_core.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: SYSTEM 
///////////////////////////////////////////////////////////////////////////////////////////////////

//// DMA ENABLED
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
//// DMA ERROR CALLBACK
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
sys::errorCallback::operator bool() {
  return errorCB != nullptr;
}
//// DMA TRANSFER CALLBACK
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
sys::transferCallback::operator bool() {
  return transferCB != nullptr;
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

//// TRANSFER DESCRIPTOR
transferDescriptor channel::descriptorList::operator [] (const int &index) {
  DmacDescriptor *desc = &baseDescArray[super->index];
  for (int i = 0; i < index; i++) {
    if (!desc->DESCADDR.bit.DESCADDR) { 
      return transferDescriptor(desc);
    }
    desc = (DmacDescriptor*)desc->DESCADDR.bit.DESCADDR;
  }
  return transferDescriptor(desc);
}
void channel::descriptorList::operator = (std::initializer_list<transferDescriptor&> descList) {




}


//// CHANNEL STATE
bool channel::state::operator = (const CHANNEL_STATE &value) {
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
      setSuspend(0);
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
channel::state::operator CHANNEL_STATE() {
  if (!DMAC->Channel[super->index].CHCTRLA.bit.ENABLE) {
    return STATE_DISABLED;
  } else if (DMAC->Channel[super->index].CHINTFLAG.bit.SUSP) {
    return STATE_SUSPENDED;
  } else if (DMAC->Channel[super->index].CHSTATUS.bit.BUSY) {
    return STATE_ACTIVE;
  } else if (DMAC->Channel[super->index].CHSTATUS.bit.PEND) {
    return STATE_PENDING;
  }
  return STATE_IDLE;    
}
//// CHANNEL ERROR
bool channel::error::operator == (const CHANNEL_ERROR &err) {
  return (CHANNEL_ERROR)(*this) == err;
}
channel::error::operator CHANNEL_ERROR() {
  if (DMAC->Channel[super->index].CHINTFLAG.bit.TERR) {
    if (DMAC->Channel[super->index].CHSTATUS.bit.FERR) {
      return ERROR_DESC;
    }
    return ERROR_TRANSFER;
  } else if (DMAC->Channel[super->index].CHSTATUS.bit.CRCERR) {
    return ERROR_CRC;
  } 
  return ERROR_NONE;
}
channel::error::operator bool() {
  return (*this) != ERROR_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA CHANNEL CONFIG
///////////////////////////////////////////////////////////////////////////////////////////////////

//// TRIGGER SOURCE 
void channelConfig::triggerSource::operator = (const TRIGGER_SOURCE &value) {
  DMAC->Channel[super->index].CHCTRLA.bit.TRIGSRC = (uint8_t)value;
}
channelConfig::triggerSource::operator TRIGGER_SOURCE() {
  return static_cast<TRIGGER_SOURCE>
    (DMAC->Channel[super->index].CHCTRLA.bit.TRIGSRC);
}

//// TRIGGER ACTION 
void channelConfig::triggerAction::operator = (const TRIGGER_ACTION &value) {
  DMAC->Channel[super->index].CHCTRLA.bit.TRIGACT = (uint8_t)value;
}
channelConfig::triggerAction::operator TRIGGER_ACTION() {
  return static_cast<TRIGGER_ACTION>
    (DMAC->Channel[super->index].CHCTRLA.bit.TRIGACT);
}
//// BURST THRESHOLD 
bool channelConfig::burstThreshold::operator = (const int &value) {
  static const int THRESH_REF[] = {1, 2, 4, 8};
  for (int i = 0; i < sizeof(THRESH_REF) / sizeof(THRESH_REF[0]); i++) {
    if (value == THRESH_REF[i]) {
      DMAC->Channel[super->index].CHCTRLA.bit.THRESHOLD = i;
      DMAC->Channel[super->index].CHCTRLA.bit.BURSTLEN = value - 1;
      return true;
    }    
  }
  return false;
}
channelConfig::burstThreshold::operator int() {
  return (1 << DMAC->Channel[super->index].CHCTRLA.bit.THRESHOLD, 2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA DESCRIPTOR LIST
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DMA_MAX_DESC 256




///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA TRANSFER DESCRIPTOR
///////////////////////////////////////////////////////////////////////////////////////////////////

static const int INCR_REF[] = {1, 2, 4, 8, 16, 32, 64, 128};
static const int DATASIZE_REF[] = {8, 16, 32};

//// CONSTRUCTORS
transferDescriptor::transferDescriptor() {
  descriptor = new DmacDescriptor;
}
transferDescriptor::transferDescriptor(DmacDescriptor *desc) {
  descriptor = desc;
}
transferDescriptor::transferDescriptor(const transferDescriptor &other) {
  (*this) = other;
}
//// ASSIGNMENT (COPY) OPERATOR
void transferDescriptor::operator = (const transferDescriptor &other) {
  if (&other == this) {
    return;
  }
  uintptr_t nextAddr = descriptor->DESCADDR.bit.DESCADDR;
  memcpy(descriptor, &other.descriptor, sizeof(DmacDescriptor));
  descriptor->DESCADDR.bit.DESCADDR = nextAddr;
}

//// TRANSFER SOURCE
bool transferDescriptor::addr::operator = (const void *ptr) {
  uintptr_t address = (uintptr_t)ptr;
  if (ptr && address != DMAC_SRCADDR_RESETVALUE) {
    return false;
  }
  if (type == 0) {
    super->descriptor->SRCADDR.bit.SRCADDR = address;
  } else {
    super->descriptor->DSTADDR.bit.DSTADDR = address;
  }
  return true; 
}
transferDescriptor::addr::operator void*() const {
  uintptr_t address = (uintptr_t)(*this);
  return (address == DMAC_SRCADDR_RESETVALUE ? nullptr : (void*)address);
}
transferDescriptor::addr::operator uintptr_t() const {
  return (type ? super->descriptor->SRCADDR.bit.SRCADDR
    : super->descriptor->DSTADDR.bit.DSTADDR);
}
//// INCREMENT LOCATION
bool transferDescriptor::increment::operator = (const int &value) {
  int stepsel = type ? DMAC_BTCTRL_STEPSEL_DST_Val 
    : DMAC_BTCTRL_STEPSEL_SRC_Val;
  decltype(super->descriptor->BTCTRL.reg) regMask = type 
    ? DMAC_BTCTRL_SRCINC : DMAC_BTCTRL_DSTINC;
  if (!value) {
    super->descriptor->BTCTRL.reg &= ~regMask;
    return true;
  }
  for (int i = 0; i < sizeof(INCR_REF) / sizeof(INCR_REF[0]); i++) {
    if (value == INCR_REF[i]) {
      if (value == 1) {
        super->descriptor->BTCTRL.reg |= regMask;
        if (super->descriptor->BTCTRL.bit.STEPSEL == stepsel) {
          super->descriptor->BTCTRL.bit.STEPSIZE = 0;
        }
      } else {
        super->descriptor->BTCTRL.bit.STEPSEL = stepsel;
        super->descriptor->BTCTRL.bit.STEPSIZE = i;
      }
      return true;
    }
  }
  return false;
}
transferDescriptor::increment::operator int() const {
  int stepsel = type ? DMAC_BTCTRL_STEPSEL_DST_Val 
    : DMAC_BTCTRL_STEPSEL_SRC_Val;
  decltype(super->descriptor->BTCTRL.reg) regMask = type 
    ? DMAC_BTCTRL_SRCINC : DMAC_BTCTRL_DSTINC;
  if ((super->descriptor->BTCTRL.reg & regMask) == 0) {
    return 0;
  }
  if (super->descriptor->BTCTRL.bit.STEPSEL == stepsel) {
    return INCR_REF[super->descriptor->BTCTRL.bit.STEPSIZE];
  }
  return 1;
}
//// DATA SIZE
bool transferDescriptor::dataSize::operator = (const int &value) {
  for (int i = 0; i < sizeof(DATASIZE_REF) / sizeof(DATASIZE_REF[0]); i++) {
    if (value == DATASIZE_REF[i]) {
      super->descriptor->BTCTRL.bit.BEATSIZE = i;
      return true;
    }
  }
  return false;
}
transferDescriptor::dataSize::operator int() const {
  return DATASIZE_REF[super->descriptor->BTCTRL.bit.BEATSIZE];
}
//// TRANSFER LENGTH
bool transferDescriptor::length::operator = (const int &value) {
  if (value < 0 || value > UINT16_MAX) {
    return false;
  }
  super->descriptor->BTCNT.bit.BTCNT = value;
  return true;
}
transferDescriptor::length::operator int() const {
  return super->descriptor->BTCNT.bit.BTCNT;
}
//// SUSPEND ON COMPLETE
void transferDescriptor::suspOnCompl::operator = (const bool &value) {
  super->descriptor->BTCTRL.bit.BLOCKACT = value 
    ? DMAC_BTCTRL_BLOCKACT_SUSPEND_Val : DMAC_BTCTRL_BLOCKACT_NOACT_Val;
}
transferDescriptor::suspOnCompl::operator bool() const {
  return super->descriptor->BTCTRL.bit.BLOCKACT == DMAC_BTCTRL_BLOCKACT_SUSPEND_Val;
}
//// DESCRIPTOR VALID
bool transferDescriptor::valid::operator = (const bool &value) {
  if (value 
    && super->descriptor->SRCADDR.bit.SRCADDR != DMAC_SRCADDR_RESETVALUE
    && super->descriptor->DESCADDR.bit.DESCADDR != DMAC_SRCADDR_RESETVALUE
    && super->descriptor->BTCNT.bit.BTCNT != DMAC_BTCNT_RESETVALUE) {
    return false;
  }
  super->descriptor->BTCTRL.bit.VALID = (uint8_t)value;
  return true;
}
transferDescriptor::valid::operator bool() const {
  return super->descriptor->BTCTRL.bit.VALID;
}
//// RESET
void transferDescriptor::reset() {
  uintptr_t nextAddr = descriptor->DESCADDR.bit.DESCADDR;
  memset(descriptor, 0, sizeof(DmacDescriptor));
  descriptor->DESCADDR.bit.DESCADDR = nextAddr;
}

// transferDescriptor::~transferDescriptor() {
//   if (descriptor == &baseDescArray[index]) {
//     if (descriptor->DESCADDR.bit.DESCADDR) {
//       DmacDescriptor *next = (DmacDescriptor*)descriptor->DESCADDR.bit.DESCADDR;
//       memcpy(&baseDescArray[index], next, sizeof(DmacDescriptor));
//       if (prevDescriptor) {
//         if (prevDescriptor == next) {
//           baseDescArray[index].DESCADDR.bit.DESCADDR 
//             = (uintptr_t)(&baseDescArray[index]);
//         } else {
//           prevDescriptor->DESCADDR.bit.DESCADDR 
//             = (uintptr_t)(&baseDescArray[index]);
//         }
//       }
//       delete next;
//     } else {
//       memset(&baseDescArray[index], 0, sizeof(DmacDescriptor));        
//     }   
//   } else {
//     if (prevDescriptor) {
//       prevDescriptor->DESCADDR.bit.DESCADDR = descriptor->DESCADDR.bit.DESCADDR;
//     }
//     delete descriptor;
//   }
// }

*/




