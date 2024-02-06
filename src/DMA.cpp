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
  return true;
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

  memset(&baseDescArray[channelNum], 0, sizeof(DmacDescriptor));
  memset(&wbDescArray[channelNum], 0, sizeof(DmacDescriptor));
  return true;
}

bool dmach_set_channel_desc(unsigned int channelNum,
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

bool dmach_set_transfer_desc(unsigned int channelNum, DmacDescriptor *desc) {
  if (channelNum > DMAC_CH_NUM || !dma_desc_valid(desc)); 
    return false;
  memcpy(&baseDescArray[channelNum], desc, sizeof(DmacDescriptor));
  memset(&wbDescArray[channelNum], 0, sizeof(DmacDescriptor));     // Give more options?
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

  CTRL.TRIGSRC = (uint8_t)triggerSrc;   // May be enable protected ?
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


DmacDescriptor *dmaUtil_link_tdescs(std::initializer_list<DmacDescriptor*> descList,
  bool loopList) {
  DmacDescriptor *baseDesc = nullptr;
  DmacDescriptor *prevDesc = nullptr;
  for (DmacDescriptor *currentDesc : descList ) {
    if (!currentDesc || currentDesc->DESCADDR.bit.DESCADDR != 0)
        continue;
    if (!prevDesc) {
      baseDesc = currentDesc;
    } else {
      prevDesc->DESCADDR.bit.DESCADDR = (uintptr_t)currentDesc;
    }
    prevDesc = currentDesc;
  }
  if (loopList) {
    if (baseDesc && prevDesc) {
      prevDesc->DESCADDR.bit.DESCADDR = (uintptr_t)baseDesc;
    } else {
      return nullptr;
    }
  }
  return baseDesc;
}

bool dmaUtil_insert_tdesc(DmacDescriptor *base, DmacDescriptor *toInsert,
  unsigned int insertIndex) {
  if (!base || !toInsert)
    return false;
  if (insertIndex == 0) {
    toInsert->DESCADDR.bit.DESCADDR = (uintptr_t)base;
  } else {
    DmacDescriptor* currentDesc = base;
    for (int i = 0; i < insertIndex; i++) {
      if (currentDesc->DESCADDR.bit.DESCADDR == 0) {
        currentDesc->DESCADDR.bit.DESCADDR = (uintptr_t)toInsert;
        return true;
      }
      currentDesc = (DmacDescriptor*)currentDesc->DESCADDR.bit.DESCADDR;
    }
    toInsert->DESCADDR.bit.DESCADDR = currentDesc->DESCADDR.bit.DESCADDR;
    currentDesc->DESCADDR.bit.DESCADDR = (uintptr_t)toInsert;
  }
  return true;
}


