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

#define DMA_IRQ_COUNT 5
#define DMA_PRILVL_COUNT 4

static DMA_ERROR dma_get_errors_(int channelNum) {

  if (channelNum <= 0) {

  } else if (channelNum <= DMAC_CH_NUM){
    return DMA_ERROR_NONE;
  } else {
    return DMA_ERROR_PARAM;
  }
}

static inline bool dma_desc_valid(DmacDescriptor &desc) {
  return !(!desc.BTCNT.bit.BTCNT || !desc.DESCADDR.bit.DESCADDR
    || !desc.SRCADDR.bit.SRCADDR);
}

DMA_ERROR dmactrl_update_config() {
  auto changePriorityCtrl = [](uint8_t val, unsigned int startPos, unsigned int step) 
    -> void {
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
    changePriorityCtrl((uint8_t)dmactrl_config.roundRobinMode, DMAC_PRICTRL0_RRLVLEN0_Pos,
      (DMAC_PRICTRL0_LVLPRI1_Pos - DMAC_PRICTRL0_LVLPRI0_Pos));
  }
  if (DMAC->PRICTRL0.bit.QOS0 != (uint8_t)dmactrl_config.serviceQuality) {
    changePriorityCtrl((uint8_t)dmactrl_config.serviceQuality, DMAC_PRICTRL0_QOS0_Pos,
      (DMAC_PRICTRL0_QOS1_Pos - DMAC_PRICTRL0_QOS0_Pos));
  }
}

DMA_ERROR dmactrl_init(unsigned int channelNumber) {
  if (channelNumber > DMAC_CH_NUM)
    return DMA_ERROR_PARAM;

  dmactrl_exit();

  for (int i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }
  DMAC->BASEADDR.reg = (uintptr_t)baseDescArray;
  DMAC->WRBADDR.reg = (uintptr_t)wbDescArray;
  dmactrl_update_config();

  DMAC->CTRL.reg = 
      DMAC_CTRL_LVLEN_Msk
    | DMAC_CTRL_DMAENABLE;
  return dma_get_errors_(-1);
}

DMA_ERROR dma_exit() {
  DMAC->CTRL.bit.SWRST = 1;
  for (int i = 0; i < DMA_IRQ_COUNT; i++) {
    NVIC_DisableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
  }
  return dma_get_errors_(-1);
}

DMA_ERROR dmactrl_enable_channel(unsigned int channelNum, unsigned int priorityLvl, 
  bool runInStandby) {
  if (channelNum > DMAC_CH_NUM)
    return DMA_ERROR_PARAM;

  DMAC->Channel[channelNum].CHCTRLA.reg |=
      DMAC_CHCTRLA_ENABLE
    | ((uint8_t)runInStandby << DMAC_CHCTRLA_RUNSTDBY_Pos);
  DMAC->Channel[channelNum].CHPRILVL.bit.PRILVL = (uint8_t)priorityLvl;

  return dma_get_errors_(channelNum);
}

#define DMA_MAX_BURSTLEN 7

DMA_ERROR dmach_update_config(unsigned int channelNum) {
  if (channelNum > DMAC_CH_NUM) 
    return DMA_ERROR_PARAM;
  DMAC->Channel[channelNum].CHCTRLA.reg = (decltype(DMAC->Channel[channelNum].CHCTRLA.reg))
      (((uint8_t)dmach_config[channelNum].transferThreshold << DMAC_CHCTRLA_THRESHOLD_Pos)
    | ((uint8_t)dmach_config[channelNum].burstLength << DMAC_CHCTRLA_BURSTLEN_Pos)
    | ((uint8_t)dmach_config[channelNum].triggerAction << DMAC_CHCTRLA_TRIGACT_Pos)
    | ((uint8_t)dmach_config[channelNum].triggerSource << DMAC_CHCTRLA_TRIGSRC_Pos));
  return DMA_ERROR_NONE;
}

DMA_ERROR dmach_set_descriptor(unsigned int channelNum, DmacDescriptor *baseDescriptor) {
  if (channelNum < DMAC_CH_NUM || !baseDescriptor) 
    return DMA_ERROR_PARAM;
  memcpy(&baseDescArray[channelNum], baseDescriptor, sizeof(DmacDescriptor));
  return DMA_ERROR_NONE;
}





/*
DMA_ERROR dma_set_transfer_descriptor(std::initializer_list<DmacDescriptor*> descList) {
  DmacDescriptor *prev = nullptr;

  for (DmacDescriptor  *desc : descList ) {
    if (desc == nullptr)
      break;

    if (dma_desc_valid(&desc)) {

    }
    
  }

}
*/



