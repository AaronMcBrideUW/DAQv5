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

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: MISC
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @typedef DMA callback typedefs
typedef void (*errorCBType)(int, CHANNEL_ERROR);
typedef void (*transferCBType)(int);

/// @internal DMA storage variables
DmacDescriptor wbDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16)));
DmacDescriptor baseDescArray[DMAC_CH_NUM] __attribute__ ((aligned (16))); 
errorCBType errorCB = nullptr;
transferCBType transferCB = nullptr;

/// @defgroup DMA Enums
enum CHANNEL_STATE;
enum CHANNEL_ERROR;
enum TRIGGER_SOURCE;
enum TRIGGER_ACTION;

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

  // More to do -> init() method??

  struct enabled{ 
    inline operator bool();
    void operator = (const bool);
  }enabled;

  struct errorCallback{
    inline operator errorCBType(); /// MAY HAVE TO ADD BOOL OP HERE...
    inline void operator = (const errorCBType);
  }errorCallback;

  struct transferCallback{
    inline operator transferCBType();
    inline void operator = (const transferCBType);
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


struct channel {
  channel(const int &index):index(index) {};
  const int &index;

  bool trigger();

  struct state {
    bool operator = (const CHANNEL_STATE);
    inline operator CHANNEL_STATE();
    inline operator int();
    inline bool operator == (const CHANNEL_STATE);
    _super_(channel, state);
  }state{this};

  struct enabled {
    inline operator bool();
    inline void operator = (const bool);
    _super_(channel, enabled);
  }enabled{this};

  struct triggerSource {
    inline operator TRIGGER_SOURCE();
    void operator = (const TRIGGER_SOURCE);
    _super_(channel, triggerSource);
  }triggerSrc{this};

  struct triggerAction {
    inline operator TRIGGER_ACTION();
    void operator = (const TRIGGER_ACTION);
    _super_(channel, triggerAction);
  }triggerAction{this};


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

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: ENUMS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum CHANNEL_ERROR {
  ERROR_NONE,
  ERROR_UNKNOWN,
  ERROR_CRC,
  ERROR_DESC,
  ERROR_TRANSFER
};

enum CHANNEL_STATE {
  STATE_DISABLED,
  STATE_IDLE,
  STATE_SUSPENDED,
  STATE_PENDING,
  STATE_ACTIVE,
};

enum TRIGGER_ACTION : uint8_t {
  TRANSFER_BLOCK = 0,
  TRANSFER_BURST = 2,
  TRANSFER_ALL   = 3
};

enum TRIGGER_SOURCE : uint8_t {
  TRIGGER_SOFTWARE          = 0,
  TRIGGER_RTC_TIMESTAMP     = 1,
  TRIGGER_DSU_DCC0          = 2,
  TRIGGER_DSU_DCC1          = 3,
  TRIGGER_SERCOM0_RX        = 4,
  TRIGGER_SERCOM0_TX        = 5,
  TRIGGER_SERCOM1_RX        = 6,
  TRIGGER_SERCOM1_TX        = 7,
  TRIGGER_SERCOM2_RX        = 8,
  TRIGGER_SERCOM2_TX        = 9,
  TRIGGER_SERCOM3_RX        = 10,
  TRIGGER_SERCOM3_TX        = 11,
  TRIGGER_SERCOM4_RX        = 12,
  TRIGGER_SERCOM4_TX        = 13,
  TRIGGER_SERCOM5_RX        = 14,
  TRIGGER_SERCOM5_TX        = 15,
  TRIGGER_SERCOM6_RX        = 16,
  TRIGGER_SERCOM6_TX        = 17,
  TRIGGER_SERCOM7_RX        = 18,
  TRIGGER_SERCOM7_TX        = 19,
  TRIGGER_CAN0_DEBUG_REQ    = 20,
  TRIGGER_CAN1_DEBUG_REQ    = 21,
  TRIGGER_TCC0_OOB          = 22,
  TRIGGER_TCC0_COMPARE_0    = 23,
  TRIGGER_TCC0_COMPARE_1    = 24,
  TRIGGER_TCC0_COMPARE_2    = 25,
  TRIGGER_TCC0_COMPARE_3    = 26,
  TRIGGER_TCC0_COMPARE_4    = 27,
  TRIGGER_TCC0_COMPARE_5    = 28,
  TRIGGER_TCC1_OOB          = 29,
  TRIGGER_TCC1_COMPARE_0    = 30,
  TRIGGER_TCC1_COMPARE_1    = 31,
  TRIGGER_TCC1_COMPARE_2    = 32,
  TRIGGER_TCC1_COMPARE_3    = 33,
  TRIGGER_TCC2_OOB          = 34,
  TRIGGER_TCC2_COMPARE_0    = 35,
  TRIGGER_TCC2_COMPARE_1    = 36,
  TRIGGER_TCC2_COMPARE_2    = 37,
  TRIGGER_TCC3_OOB          = 38,
  TRIGGER_TCC3_COMPARE_0    = 39,
  TRIGGER_TCC3_COMPARE_1    = 40,
  TRIGGER_TCC4_OOB          = 41,
  TRIGGER_TCC4_COMPARE_0    = 42,
  TRIGGER_TCC4_COMPARE_1    = 43,
  TRIGGER_TC0_OOB           = 44,
  TRIGGER_TC0_COMPARE_0     = 45,
  TRIGGER_TC0_COMPARE_1     = 46,
  TRIGGER_TC1_OOB           = 47,
  TRIGGER_TC1_COMPARE_0     = 48,
  TRIGGER_TC1_COMPARE_1     = 49,
  TRIGGER_TC2_OOB           = 50,
  TRIGGER_TC2_COMPARE_0     = 51,
  TRIGGER_TC2_COMPARE_1     = 52,
  TRIGGER_TC3_OOB           = 53,
  TRIGGER_TC3_COMPARE_0     = 54,
  TRIGGER_TC3_COMPARE_1     = 55,
  TRIGGER_TC4_OOB           = 56,
  TRIGGER_TC4_COMPARE_0     = 57,
  TRIGGER_TC4_COMPARE_1     = 58,
  TRIGGER_TC5_OOB           = 59,
  TRIGGER_TC5_COMPARE_0     = 60,
  TRIGGER_TC5_COMPARE_1     = 61,
  TRIGGER_TC6_OOB           = 62,
  TRIGGER_TC6_COMPARE_0     = 63,
  TRIGGER_TC6_COMPARE_1     = 64,
  TRIGGER_TC7_OOB           = 65,
  TRIGGER_TC7_COMPARE_0     = 66,
  TRIGGER_TC7_COMPARE_1     = 67,
  TRIGGER_ADC0_RESRDY       = 68,
  TRIGGER_ADC0_SEQ          = 69,
  TRIGGER_ADC1_RESRDY       = 70,
  TRIGGER_ADC1_SEQ          = 71,
  TRIGGER_DAC_EMPTY0        = 72,
  TRIGGER_DAC_EMPTY1        = 73,
  TRIGGER_DAC_RESULT_READY0 = 74,
  TRIGGER_DAC_RESULT_READY1 = 75,
  TRIGGER_I2S_RX0           = 76,
  TRIGGER_I2S_RX1           = 77,
  TRIGGER_I2S_TX0           = 78,
  TRIGGER_I2S_TX1           = 79,
  TRIGGER_PCC_RX            = 80,
  TRIGGER_AES_WRITE         = 81,
  TRIGGER_AES_READ          = 82,
  TRIGGER_QSPI_RX           = 83,
  TRIGGER_QSPI_TX           = 84
};




