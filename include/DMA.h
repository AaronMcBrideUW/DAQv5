///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS CONTROLLER (HEADER)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "sam.h"
#include "inttypes.h"
#include "initializer_list"

#include "SYS.h"

typedef struct ChannelDescriptor {
  unsigned int transferThreshold = 1;
  unsigned int burstLength = 0;
  unsigned int priorityLvl = 2;
  bool runInStandby = true;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum DMA_CB_REASON {

};

enum DMACH_STATE {
  DMACH_STATE_UNKNOWN,
  DMACH_STATE_DISABLED,
  DMACH_STATE_IDLE,
  DMACH_STATE_BUSY,
  DMACH_STATE_PEND,
};

enum DMACH_ERROR {
  DMACH_ERROR_NONE,
  DMACH_ERROR_UNKNOWN,
  DMACH_ERROR_CRC,
  DMACH_ERROR_DESC,
  DMACH_ERROR_TRANSFER
};

struct {
  bool roundRobinMode = false;
  unsigned int serviceQuality = 2;
  unsigned int irqPriority = 1;
  void (*errorCallback)(unsigned int, DMACH_ERROR) = nullptr;
  void (*transferCallback)(unsigned int) = nullptr;
}dmactrl_config;


bool dmactrl_init(); // FP Complete

bool dmactrl_exit(); // FP Complete

bool dmactrl_update_config(); // FP Complete



bool dmach_enable(unsigned int channelNum); // FP Complete 

bool dmach_disable(unsigned int channelNum); // FP Complete 

bool dmach_reset(unsigned int channelNum); // FP Complete



bool dmach_set_channel_desc(unsigned int channelNum, ChannelDescriptor &desc); // FP Complete

bool dmach_set_transfer_desc(unsigned int channelNum, DmacDescriptor *desc); // FP Complete

bool dmach_set_trigger(unsigned int channelNum, unsigned int triggerSrc,
   unsigned int triggerAction); // FP Complete

bool dmach_trigger(unsigned int channelNum); // FP Complete  

bool dmach_set_cmd(unsigned int channelNum, int cmd); // FP Complete



DMACH_ERROR dmach_get_error(unsigned int channelNum); // FP Complete

DMACH_STATE dmach_get_state(unsigned int channelNum); // FP complete



DmacDescriptor *dmaUtil_link_tdesc(std::initializer_list<DmacDescriptor*> descList,
  bool loopList);

bool dmaUtil_insert_tdesc(DmacDescriptor *base, DmacDescriptor *toInsert,
  unsigned int insertIndex);

  
//DMA_ERROR dma_set_descriptor(std::initializer_list<DmacDescriptor*> transferDescs);