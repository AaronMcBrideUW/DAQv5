///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS CONTROLLER (HEADER)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "sam.h"
#include "inttypes.h"
#include "initializer_list"

#include "SYS.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum DMA_CB_REASON {

};

enum DMA_ERROR {
  DMA_ERROR_NONE,
  DMA_ERROR_UNKNOWN,
  DMA_ERROR_PARAM,
  DMA_ERROR_DESCRIPTOR,
};

struct {
  bool roundRobinMode = false;
  unsigned int serviceQuality = 2;
  unsigned int irqPriority = 1;
  void (*errorCallback)(unsigned int, DMA_ERROR) = nullptr;
  void (*transferCallback)(unsigned int) = nullptr;
}dmactrl_config;

struct {
  bool enableDurringStandby = true;
  unsigned int transferThreshold = 1;
  unsigned int burstLength = 0;
  unsigned int priorityLvl = 2;
  unsigned int triggerSource = 3;
  unsigned int triggerAction = 3;
}dmach_config[DMAC_CH_NUM];

DMA_ERROR dmactrl_update_config(); // Done

DMA_ERROR dmactrl_init(); // Done

DMA_ERROR dmactrl_exit(); // Done

DMA_ERROR dmactrl_reset();

DMA_ERROR dmach_set_enabled(unsigned int channelNum, bool enabled); // Done 

DMA_ERROR dmach_reset(unsigned int channelNum); 

DMA_ERROR dmach_update_config(unsigned int channelNum); // Done 

DMA_ERROR dmach_set_descriptor(unsigned int channelNum, DmacDescriptor *baseDescriptor); // DONE

DMA_ERROR dmach_trigger(unsigned int channelNum); 

DMA_ERROR dmach_set_suspend(unsigned int channelNum, int suspendState);

//DMA_ERROR dma_set_descriptor(std::initializer_list<DmacDescriptor*> transferDescs);