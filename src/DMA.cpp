///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS (CPP FILE)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <DMA.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA MISC
///////////////////////////////////////////////////////////////////////////////////////////////////

DmacDescriptor wbDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16)));
DmacDescriptor baseDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16))); 

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

#define OOB_(_val_, _min_, _max_) (_val_ < _min_ || _val_ > _max_)

#define DMA_SQ_MAX 3
#define DMA_IRQPRI_MAX 5 // Find a way to get this from board

#define DMA_IRQ_COUNT 5
#define DMA_PRILVL_COUNT 4

#define DMA_THRESH_MAX 8
#define DMA_BURSTLEN_MAX 16
#define DMA_TRIGACT_MAX 3
#define DMA_TRIGACT_RESERVED 1

static inline bool dma_desc_valid(DmacDescriptor *desc) {
  return !(!desc 
    || !desc->BTCNT.bit.BTCNT 
    || !desc->DESCADDR.bit.DESCADDR
    || !desc->SRCADDR.bit.SRCADDR 
    || !desc->BTCTRL.bit.VALID);
}

bool dmactrl_init() {
  if (DMAC->CTRL.bit.DMAENABLE)
    return false;

  for (int i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }
  DMAC->BASEADDR.reg = (uintptr_t)baseDescArray;
  DMAC->WRBADDR.reg = (uintptr_t)wbDescArray;
  dmactrl_update_config();

  DMAC->CTRL.reg = 
      DMAC_CTRL_LVLEN_Msk
    | DMAC_CTRL_DMAENABLE;
  return true;
}

bool dma_exit() {
  DMAC->CTRL.bit.DMAENABLE = 0;
  DMAC->CTRL.bit.SWRST = 1;

  memset(baseDescArray, 0, sizeof(baseDescArray));
  memset(wbDescArray, 0, sizeof(wbDescArray));

  for (int i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_DisableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }
  return true;
}

bool dmactrl_update_config() {
  static bool errorCBNull = !dmactrl_config.errorCallback;
  static bool transCBNull = !dmactrl_config.transferCallback;
  if (dmactrl_config.serviceQuality > DMA_SQ_MAX
    ||dmactrl_config.irqPriority > DMA_IRQPRI_MAX)
    return false;

  auto changePriorityCtrl = [](uint8_t val, unsigned int startPos, 
    unsigned int step) -> void {
    for (int i = 0; i < DMA_PRILVL_COUNT; i++) {
      DMAC->PRICTRL0.reg &= ~(1 << startPos + i * step);
    }
    if (val) {
      for (int i = 0; i < DMA_PRILVL_COUNT; i++) {
        DMAC->PRICTRL0.reg |= (val << startPos + i * step);
      }
    }
  };
  auto changeChannelIRQ = [](uint8_t pos, bool setPos) {
    if (setPos) {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENSET.reg 
          = (decltype(DMAC->Channel[i].CHINTENSET.reg))(1 << pos);
      }
    } else {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENCLR.reg 
          = (decltype(DMAC->Channel[i].CHINTENSET.reg))(1 << pos);;
      }      
    }
  };
  for (int i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), dmactrl_config.irqPriority);
  }
  if (DMAC->PRICTRL0.bit.RRLVLEN0 != (uint8_t)dmactrl_config.roundRobinMode) {
    changePriorityCtrl((uint8_t)dmactrl_config.roundRobinMode, 
      DMAC_PRICTRL0_RRLVLEN0_Pos, (DMAC_PRICTRL0_LVLPRI1_Pos - DMAC_PRICTRL0_LVLPRI0_Pos));
  }
  if (DMAC->PRICTRL0.bit.QOS0 != (uint8_t)dmactrl_config.serviceQuality) {
    changePriorityCtrl((uint8_t)dmactrl_config.serviceQuality, 
      DMAC_PRICTRL0_QOS0_Pos, (DMAC_PRICTRL0_QOS1_Pos - DMAC_PRICTRL0_QOS0_Pos));
  }
  if (errorCBNull != !dmactrl_config.errorCallback) {
    errorCBNull = !dmactrl_config.errorCallback;
    changeChannelIRQ(DMAC_CHINTFLAG_TERR_Pos, !errorCBNull);
  }
  if (transCBNull != !dmactrl_config.transferCallback) {
    transCBNull = !dmactrl_config.transferCallback;
    changeChannelIRQ(DMAC_CHINTFLAG_TCMPL_Pos, !transCBNull);
  }
  return true;
}

unsigned int dmactrl_get_busy_channel() {
  return DMAC->ACTIVE.bit.ID; 
}

unsigned int dmactrl_get_active_blockCount() {
  return DMAC->ACTIVE.bit.BTCNT;
}

bool dmach_enable(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM)
    return false;
  DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE = 1;  
  return true;
}

bool dmach_disable(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM)
    return false;
  DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE = 0;
  return true;
}

bool dmach_reset(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM)
    return false;
  DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE = 0;
  DMAC->Channel[channelNum].CHCTRLA.bit.SWRST = 1;
  if (dmactrl_config.errorCallback) {
    DMAC->Channel[channelNum].CHINTENSET.bit.TERR = 1;
  } else {
    DMAC->Channel[channelNum].CHINTENCLR.bit.TERR = 1;
  }
  if (dmactrl_config.transferCallback) {
    DMAC->Channel[channelNum].CHINTENSET.bit.TCMPL = 1;
  } else {
    DMAC->Channel[channelNum].CHINTENCLR.bit.TCMPL = 1;
  }
  memset(&baseDescArray[channelNum], 0, sizeof(DmacDescriptor));
  memset(&wbDescArray[channelNum], 0, sizeof(DmacDescriptor));
  return true;
}

bool dmach_set_chd(unsigned int channelNum,
  ChannelDescriptor &desc) {
  #define CTRL DMAC->Channel[channelNum].CHCTRLA.bit

  if (channelNum > DMAC_CH_NUM
    ||OOB_((uint8_t)desc.transferThreshold, 1, DMA_THRESH_MAX)
    ||OOB_((uint8_t)desc.burstLength, 1, DMA_BURSTLEN_MAX)
    ||OOB_((uint8_t)desc.priorityLvl, 1, DMA_PRILVL_COUNT))
    return false;

  CTRL.THRESHOLD = log2(desc.transferThreshold);
  CTRL.BURSTLEN = desc.burstLength - 1;
  CTRL.RUNSTDBY = (uint8_t)desc.runInStandby;
  DMAC->Channel[channelNum].CHPRILVL.bit.PRILVL = desc.priorityLvl;
  return true;
}

bool dmach_set_base_trd(unsigned int channelNum, DmacDescriptor *baseDesc) {
  if (channelNum > DMAC_CH_NUM || !dma_desc_valid(baseDesc)); 
    return false;
  memcpy(&baseDescArray[channelNum], baseDesc, sizeof(DmacDescriptor));
  memset(&wbDescArray[channelNum], 0, sizeof(DmacDescriptor));
  return true;
}

bool dmach_set_trigger(unsigned int channelNum, unsigned int triggerSrc,
  unsigned int triggerAction) {
  #define CTRL DMAC->Channel[channelNum].CHCTRLA.bit
  
  if (channelNum > DMAC_CH_NUM 
    ||triggerSrc >= DMAC_TRIG_NUM
    ||triggerAction > DMA_TRIGACT_MAX 
    ||triggerAction == DMA_TRIGACT_RESERVED) 
    return false;

  CTRL.TRIGSRC = (uint8_t)triggerSrc;   
  CTRL.TRIGACT = (uint8_t)triggerAction; 
  return true;
}

bool dmach_trigger(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM)
    return false;
  DMAC->SWTRIGCTRL.reg |= (1 << channelNum);
  return true;
}

bool dmach_set_cmd(unsigned int channelNum, int cmd) {
  if (channelNum > DMAC_CH_NUM)
    return false;    
  if (cmd == -1) {
    DMAC->Channel[channelNum].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;
  } else if (cmd == 1) {
    DMAC->Channel[channelNum].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_RESUME_Val;
  } else if (!cmd) {
    DMAC->Channel[channelNum].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
  } else {
    return false;
  }
  return true;
}

DMACH_ERROR dmach_get_error(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM)
    return DMACH_ERROR_UNKNOWN;

  if (DMAC->Channel[channelNum].CHINTFLAG.bit.TERR) {
    DMAC->Channel[channelNum].CHINTFLAG.bit.TERR = 1;

    if (DMAC->Channel[channelNum].CHSTATUS.bit.FERR) {
      return DMACH_ERROR_DESC;
    }
    return DMACH_ERROR_TRANSFER;
  } else if (DMAC->Channel[channelNum].CHSTATUS.bit.CRCERR) {
    return DMACH_ERROR_CRC;
  } 
  return DMACH_ERROR_NONE;
}

DMACH_STATE dmach_get_state(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM) 
    return DMACH_STATE_UNKNOWN;

  if (!DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE) {
    return DMACH_STATE_DISABLED;
  } else if (DMAC->Channel[channelNum].CHSTATUS.bit.BUSY) {
    return DMACH_STATE_BUSY;
  } else if (DMAC->Channel[channelNum].CHSTATUS.bit.PEND) {
    return DMACH_STATE_PEND;
  } else if (DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE) {
    return DMACH_STATE_IDLE;
  }
  return DMACH_STATE_UNKNOWN;
}

DmacDescriptor *dmach_get_wbd(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM) 
    return nullptr;
  return &wbDescArray[channelNum];
}

DmacDescriptor *dmach_get_trd(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM)
    return nullptr;
  return &baseDescArray[channelNum];
}


DmacDescriptor *trd_list_group(std::initializer_list<DmacDescriptor*> descList) {
  DmacDescriptor *baseDesc = nullptr;
  DmacDescriptor *prevDesc = nullptr;
  
  for (DmacDescriptor *currentTRD : descList) {
    if (!currentTRD)
      continue;
    if (!prevDesc) {
      baseDesc = currentTRD;
    } else {
      prevDesc->DESCADDR.bit.DESCADDR = (uintptr_t)currentTRD;
    }
    prevDesc = currentTRD;
  }
  return baseDesc;
}

bool trd_unlink_group(DmacDescriptor *baseDesc) {
  if (!baseDesc)
    return false;
  DmacDescriptor *currentTRD = baseDesc;

  while(currentTRD->DESCADDR.bit.DESCADDR != 0) {
    DmacDescriptor *tempNext = (DmacDescriptor*)currentTRD->DESCADDR.bit.DESCADDR;
    currentTRD->DESCADDR.bit.DESCADDR = 0;
    currentTRD = tempNext;
  }
  return true;
}

bool trd_insert(DmacDescriptor *baseDesc, DmacDescriptor *insertDesc, 
  unsigned int insertIndex) {
  if (!baseDesc || !insertDesc)
    return false;

  DmacDescriptor *currentTRD = baseDesc;
  if (!insertIndex) {
    currentTRD->DESCADDR.bit.DESCADDR = (uintptr_t)baseDesc;
    baseDesc = insertDesc;
  } else {
    for (int i = 0; i < insertIndex; i++) {
      if (!currentTRD->DESCADDR.bit.DESCADDR) { 
        currentTRD->DESCADDR.bit.DESCADDR = (uintptr_t)insertDesc;
        break;
      }
      currentTRD = (DmacDescriptor*)currentTRD->DESCADDR.bit.DESCADDR;                                                                                                                                                                                                                                                                                                                       
    }
    uintptr_t nextTemp = currentTRD->DESCADDR.bit.DESCADDR;
    currentTRD->DESCADDR.bit.DESCADDR = (uintptr_t)insertDesc;
    insertDesc->DESCADDR.bit.DESCADDR = nextTemp;
  }
  return true;
}

DmacDescriptor *trd_remove(DmacDescriptor *baseDesc, unsigned int removeIndex) {
  if (!baseDesc)
    return nullptr;

  DmacDescriptor *currentTRD = baseDesc;
  for (int i = 0; i < removeIndex; i++) {
    
    currentTRD = (DmacDescriptor*)currentTRD->DESCADDR.bit.DESCADDR;
  }

}