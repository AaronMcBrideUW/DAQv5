///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DMA 
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "SYS.h"
#include "sam.h"
#include "inttypes.h"
#include "initializer_list"
#include "memory.h"

#define IRQ_MAX_PRIORITY 4 /// NOTE: THIS SHOULD BE REPLACED...

#define _super_(_super_, _this_) explicit _this_(_super_ *super):super(super) {}; \
                                const _super_ *super

enum DMA_ERROR {
  DMACH_ERROR_NONE,
  DMACH_ERROR_UNKNOWN,
  DMACH_ERROR_CRC,
  DMACH_ERROR_DESC,
  DMACH_ERROR_TRANSFER
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: MISC
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @typedef DMA callback typedefs
typedef void (*errorCBType)(int, DMA_ERROR);
typedef void (*transferCBType)(int);

/// @internal DMA storage variables
DmacDescriptor wbDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16)));
DmacDescriptor baseDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16))); 
errorCBType errorCB = nullptr;
transferCBType transferCB = nullptr;

/// @defgroup DMA hardware properties 
#define DMA_IRQ_COUNT 5
#define DMA_PRILVL_COUNT 4

#define DMA_IRQPRI_MAX 5 
#define DMA_QOS_MAX 3

/// @defgroup DMA misc config
const int IRQ_PRILVL = 1;
const int PRILVL_SERVICE_QUAL[DMA_PRILVL_COUNT] = { 2 };
const bool PRILVL_RR_MODE[DMA_PRILVL_COUNT] = { false };
const bool PRILVL_ENABLED[DMA_PRILVL_COUNT] = { true };

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: INTERFACE MODULES
///////////////////////////////////////////////////////////////////////////////////////////////////

struct sys{

  bool reset();

  struct init {
    // TO DO
  };

  struct enabled{ 
    inline operator bool();
    bool operator = (const bool value);
  }enabled;

  struct errorCallback{
    inline operator errorCBType(); /// MAY HAVE TO ADD BOOL OP HERE...
    inline bool operator = (const errorCBType errorCallback);
  }errorCallback;

  struct transferCallback{
    inline operator transferCBType();
    inline bool operator = (const transferCBType transferCallback);
  }transferCallback;

};

struct activeChannel {

  struct remainingBytes{
    inline operator int();
  }remainingBytes;

  struct index{
    inline operator int();
  }index;

  struct busy{
    inline operator bool();
  }busy;

}activeChannel;

enum CHANNEL_STATE {
  STATE_DISABLED,
  STATE_IDLE,
  STATE_ACTIVE,
  STATE_SUSPENDED
};

struct channel {
  channel(const int &index):index(index) {};
  const int &index;

  bool trigger();

  struct enabled {
    inline operator bool();
    bool operator = (const bool value);
    _super_(channel, enabled);
  }enabled{this};

  struct triggerSrc {

  }triggerSrc;

  struct triggerAction {

  }triggerAction;

  struct suspended {

  }suspended;

  struct baseDescriptor {

  }baseDescriptor;

  struct writebackDescriptor {

  }writebackDescriptor;

  struct error {

  }error;

  struct config {
    explicit config(const int index):index(index) {};
    const int &index;

    struct transferThreshold{

      _super_(config, transferThreshold);
    }transferThreshold{this};

    struct burstLength{

      _super_(config, burstLength);
    }burstlength{this};

    struct runInStandby {

    }runInStandby;

  }config{this};

}channel;




