///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS CONTROLLER (HEADER)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "inttypes.h"
#include "sam.h"
#include "initializer_list"

#include "SYS.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum DMA_CB_REASON {

};

enum DMA_ERROR {
  DMA_ERROR_NONE,
  DMA_ERROR_PARAM
};

struct {
  bool roundRobinMode = false;
  unsigned int serviceQuality = 2;
  unsigned int irqPriority = 1;
  void (*errorCallback)(unsigned int, DMA_ERROR) = nullptr;
  void (*transferCallback)(unsigned int) = nullptr;
}dma_config;

typedef struct ChannelDescriptor {
  unsigned int minBeats = 1;
  unsigned int burstLength = 1;
  unsigned int triggerSrc = 0;
  unsigned int triggerAction = 3;
};

DMA_ERROR dma_update_config();

DMA_ERROR dma_init();

DMA_ERROR dma_exit();

DMA_ERROR dma_enable_channel(unsigned int channelNum, unsigned int channelPriority,
  bool runInStandby);

DMA_ERROR dma_disable_channel(unsigned int channelNum);

DMA_ERROR dma_reset_channel(unsigned int channelNum);

DMA_ERROR dma_set_trigger_source(unsigned int channelNum, unsigned int triggerSource);

DMA_ERROR dma_set_trigger_action(unsigned int channelNum, unsigned int triggerAction);

DMA_ERROR dma_set_burst_length(unsigned int channelNum, unsigned int );

DMA_ERROR dma_set_transfer_descriptor();

DMA_ERROR dma_link_transfer_descriptors(std::initializer_list<DmacDescriptor>);