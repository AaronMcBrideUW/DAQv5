/*

///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS (CPP FILE)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <DMA.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA VARIABLES & DEFS
///////////////////////////////////////////////////////////////////////////////////////////////////

DmacDescriptor wbDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16)));
DmacDescriptor baseDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16))); 

#define OOB_(_val_, _min_, _max_) (_val_ < _min_ || _val_ > _max_)

#define DMA_SQ_MAX 3
#define DMA_IRQPRI_MAX 5 
#define DMA_IRQ_COUNT 5
#define DMA_PRILVL_COUNT 4
#define DMA_THRESH_MAX 8
#define DMA_BURSTLEN_MAX 16
#define DMA_TRIGACT_MAX 3
#define DMA_TRIGACT_RESERVED 1

#define TRD_MAX_BEATSIZE 4
#define TRD_MAX_BLOCKACT 3

#define DMA_CRC_CH_NUM 31

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dma {
  static inline bool dma_desc_valid(DmacDescriptor *desc) {
    return !(!desc 
      || !desc->BTCNT.bit.BTCNT 
      || !desc->DESCADDR.bit.DESCADDR
      || !desc->SRCADDR.bit.SRCADDR 
      || !desc->BTCTRL.bit.VALID);
  }

  void DMAC_MAIN_HANDLER(void) {
    unsigned int sourceNum = DMAC->INTPEND.bit.ID;
    if (sys_config.errorCallback) {
      if (DMAC->Channel[sourceNum].CHINTFLAG.bit.TERR) {
        sys_config.errorCallback(sourceNum, channel_get_error(sourceNum));
      }
    }
    if (sys_config.transferCallback) {
      if (DMAC->Channel[sourceNum].CHINTFLAG.bit.TCMPL) {
        DMAC->Channel[sourceNum].CHINTFLAG.bit.TCMPL = 1;
        sys_config.transferCallback(sourceNum);
      }
    }
  }
  void DMAC_0_Handler(void) __attribute__((alias("DMAC_MAIN_HANDLER")));
  void DMAC_1_Handler(void) __attribute__((alias("DMAC_MAIN_HANDLER")));
  void DMAC_2_Handler(void) __attribute__((alias("DMAC_MAIN_HANDLER")));
  void DMAC_3_Handler(void) __attribute__((alias("DMAC_MAIN_HANDLER")));
  void DMAC_4_Handler(void) __attribute__((alias("DMAC_MAIN_HANDLER")));

  bool sys_init() {
    if (DMAC->CTRL.bit.DMAENABLE)
      return false;

    for (int i = 0; i < DMA_IRQ_COUNT; i++) {
      NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
    }
    DMAC->BASEADDR.reg = (uintptr_t)baseDescArray;
    DMAC->WRBADDR.reg = (uintptr_t)wbDescArray;
    sys_update_config();

    DMAC->CTRL.reg = 
        DMAC_CTRL_LVLEN_Msk
      | DMAC_CTRL_DMAENABLE;
    return true;
  }

  bool sys_exit() {
    DMAC->CTRL.bit.DMAENABLE = 0;
    DMAC->CTRL.bit.SWRST = 1;

    memset(baseDescArray, 0, sizeof(baseDescArray));
    memset(wbDescArray, 0, sizeof(wbDescArray));

    for (int i = 0; i < DMA_IRQ_COUNT; i++) {
      NVIC_DisableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
    }
    return true;
  }

  bool sys_update_config() {
    static bool errorCBNull = !sys_config.errorCallback;
    static bool transCBNull = !sys_config.transferCallback;
    if (sys_config.serviceQuality > DMA_SQ_MAX
      ||sys_config.irqPriority > DMA_IRQPRI_MAX)
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
      NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), sys_config.irqPriority);
    }
    if (DMAC->PRICTRL0.bit.RRLVLEN0 != (uint8_t)sys_config.roundRobinMode) {
      changePriorityCtrl((uint8_t)sys_config.roundRobinMode, 
        DMAC_PRICTRL0_RRLVLEN0_Pos, (DMAC_PRICTRL0_LVLPRI1_Pos - DMAC_PRICTRL0_LVLPRI0_Pos));
    }
    if (DMAC->PRICTRL0.bit.QOS0 != (uint8_t)sys_config.serviceQuality) {
      changePriorityCtrl((uint8_t)sys_config.serviceQuality, 
        DMAC_PRICTRL0_QOS0_Pos, (DMAC_PRICTRL0_QOS1_Pos - DMAC_PRICTRL0_QOS0_Pos));
    }
    if (errorCBNull != !sys_config.errorCallback) {
      errorCBNull = !sys_config.errorCallback;
      changeChannelIRQ(DMAC_CHINTFLAG_TERR_Pos, !errorCBNull);
    }
    if (transCBNull != !sys_config.transferCallback) {
      transCBNull = !sys_config.transferCallback;
      changeChannelIRQ(DMAC_CHINTFLAG_TCMPL_Pos, !transCBNull);
    }
    return true;
  }

  unsigned int sys_get_active_channel() {
    return DMAC->ACTIVE.bit.ID; 
  }

  unsigned int sys_get_active_blockCount() {
    return DMAC->ACTIVE.bit.BTCNT;
  }

  bool channel_enable(unsigned int channelNum) {
    if (channelNum > DMAC_CH_NUM)
      return false;
    DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE = 1;  
    return true;
  }

  bool channel_disable(unsigned int channelNum) {
    if (channelNum > DMAC_CH_NUM)
      return false;
    DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE = 0;
    return true;
  }

  bool channel_reset(unsigned int channelNum) {
    if (channelNum > DMAC_CH_NUM)
      return false;
    DMAC->Channel[channelNum].CHCTRLA.bit.ENABLE = 0;
    DMAC->Channel[channelNum].CHCTRLA.bit.SWRST = 1;
    if (sys_config.errorCallback) {
      DMAC->Channel[channelNum].CHINTENSET.bit.TERR = 1;
    } else {
      DMAC->Channel[channelNum].CHINTENCLR.bit.TERR = 1;
    }
    if (sys_config.transferCallback) {
      DMAC->Channel[channelNum].CHINTENSET.bit.TCMPL = 1;
    } else {
      DMAC->Channel[channelNum].CHINTENCLR.bit.TCMPL = 1;
    }
    memset(&baseDescArray[channelNum], 0, sizeof(DmacDescriptor));
    memset(&wbDescArray[channelNum], 0, sizeof(DmacDescriptor));
    return true;
  }

  bool channel_set_chd(unsigned int channelNum,
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

  bool channel_set_base_trd(unsigned int channelNum, DmacDescriptor *baseDesc) {
    if (channelNum > DMAC_CH_NUM || !dma_desc_valid(baseDesc)); 
      return false;
    memcpy(&baseDescArray[channelNum], baseDesc, sizeof(DmacDescriptor));
    memset(&wbDescArray[channelNum], 0, sizeof(DmacDescriptor));
    return true;
  }

  bool channel_set_trigger(unsigned int channelNum, unsigned int triggerSrc,
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

  bool channel_trigger(unsigned int channelNum) {
    if (channelNum > DMAC_CH_NUM)
      return false;
    DMAC->SWTRIGCTRL.reg |= (1 << channelNum);
    return true;
  }

  bool channel_set_cmd(unsigned int channelNum, int cmd) {
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

  DMACH_ERROR channel_get_error(unsigned int channelNum) {
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

  DMACH_STATE channel_get_state(unsigned int channelNum) {
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

  DmacDescriptor *channel_get_wbd(unsigned int channelNum) {
    if (channelNum > DMAC_CH_NUM) 
      return nullptr;
    return &wbDescArray[channelNum];
  }

  DmacDescriptor *channel_get_base_trd(unsigned int channelNum) {
    if (channelNum > DMAC_CH_NUM)
      return nullptr;
    return &baseDescArray[channelNum];
  }

  bool crc_set_config(unsigned int channelNum, unsigned int crcType, 
    unsigned int crcSize) {

    if (channelNum > DMA_CRC_CH_NUM 
      || (crcType != 16 && crcType != 32) 
      || crcSize > 32)
      return false;

    DMAC->CRCCTRL.reg = (decltype(DMAC->CRCCTRL.reg)) 
        DMAC_CRCCTRL_CRCMODE_CRCGEN
      | DMAC_CRCCTRL_CRCSRC(channelNum < 0 ? DMAC_CRCCTRL_CRCSRC_IO_Val : channelNum + 2)
      | (crcType << DMAC_CRCCTRL_CRCPOLY_Pos)
      | (crcSize << DMAC_CRCCTRL_CRCBEATSIZE_Pos);
    return true;
  }

  inline decltype(DMAC->CRCCHKSUM.reg) crc_get_chksum() {
    return DMAC->CRCCHKSUM.reg; 
  }

  DmacDescriptor *trd_link_group(std::initializer_list<DmacDescriptor*> descList) {
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

    DmacDescriptor *targetTRD = nullptr;
    if (!removeIndex) {
      targetTRD = baseDesc;
      baseDesc = (DmacDescriptor*)targetTRD->DESCADDR.bit.DESCADDR;
    } else {
      DmacDescriptor *pastTRD = baseDesc;
      for (int i = 0; i <= removeIndex; i++) {
        if (!pastTRD->DESCADDR.bit.DESCADDR)
          return nullptr;
        pastTRD = (DmacDescriptor*)pastTRD->DESCADDR.bit.DESCADDR;
      }
      targetTRD = (DmacDescriptor*)pastTRD->DESCADDR.bit.DESCADDR;
      pastTRD->DESCADDR.bit.DESCADDR = targetTRD->DESCADDR.bit.DESCADDR;
    }
    targetTRD->DESCADDR.bit.DESCADDR = 0;
    return targetTRD;
  }

  bool trd_set_loop(DmacDescriptor *baseDesc, bool looped) {
    if (!baseDesc)
      return false;

    uintptr_t baseAddr = (uintptr_t)baseDesc;
    DmacDescriptor *curr = baseDesc;
    if (looped) {
      while(curr->DESCADDR.bit.DESCADDR) {
        if (curr->DESCADDR.bit.DESCADDR == baseAddr)
          return true;
        curr = (DmacDescriptor*)curr->DESCADDR.bit.DESCADDR;
      }
      curr->DESCADDR.bit.DESCADDR = baseAddr;
    } else {
      while(curr->DESCADDR.bit.DESCADDR != baseAddr) {
        if (curr->DESCADDR.bit.DESCADDR)
          return true;
        curr = (DmacDescriptor*)curr->DESCADDR.bit.DESCADDR;
      }
      curr->DESCADDR.bit.DESCADDR = 0;
    }
    return true;
  }

  bool trd_factory(DmacDescriptor *desc, void *src, void *dest, 
    unsigned int incrSrc, unsigned int incrDest, unsigned int beatCount, 
      unsigned int beatSize, unsigned int transferAction) {

    if (!desc || !src || !dest || (incrSrc > 1 && incrDest > 1)
      || beatSize > TRD_MAX_BEATSIZE || transferAction > TRD_MAX_BLOCKACT) 
        return false;  

    uint32_t srcMod = beatCount * (beatSize + 1) * (incrSrc ? pow(2, incrSrc) : 1);
    uint32_t destMod = beatCount * (beatSize + 1) * (incrDest ? pow(2, incrDest) : 1);

    desc->SRCADDR.bit.SRCADDR = (uintptr_t)src + srcMod;
    desc->DESCADDR.bit.DESCADDR = (uintptr_t)dest + destMod;
    desc->BTCNT.bit.BTCNT = (decltype(desc->BTCNT.bit.BTCNT))beatCount; 

    desc->BTCTRL.reg = 
        (DMAC_BTCTRL_STEPSIZE((uint8_t)log2(incrSrc > incrDest ? incrSrc : incrDest)))
      | ((uint8_t)(incrSrc > incrDest) << DMAC_BTCTRL_STEPSEL_Pos)
      | ((uint8_t)(incrSrc > 0) << DMAC_BTCTRL_SRCINC_Pos)
      | ((uint8_t)(incrDest > 0) << DMAC_BTCTRL_DSTINC_Pos)
      | (DMAC_BTCTRL_BEATSIZE((uint8_t)log2(beatSize)))
      | (DMAC_BTCTRL_BLOCKACT((uint8_t)transferAction))
      | DMAC_BTCTRL_VALID;
    return dma_desc_valid(desc);
  }

}

*/