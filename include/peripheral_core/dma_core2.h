
#pragma once
#include <peripheral_core/core_util.h>
#include <initializer_list>
#include <sam.h>
#include <string.h>


#define DMA_IRQ_COUNT 5
#define DMA_PRILVL_COUNT 4
#define DMA_IRQPRI_MAX 5 
#define DMA_QOS_MAX 3

namespace core {

  class transferDescriptor;

  struct dma {

    typedef void (*transferCallbackType)(int channelIndex);
    typedef void (*errorCallbackType)(int channelIndex, enum ERROR_TYPE);  

    enum CHANNEL_STATE : int;
    enum CHANNEL_ERROR : int;
    enum TRIGGER_SOURCE : int;
    enum TRANSFER_TYPE : int;

    struct configGroup {

      constexpr static int irqPriority = 1;
      constexpr static int prilvl_service_qual[DMA_PRILVL_COUNT] = {2};
      constexpr static bool prilvl_rr_mode[DMA_PRILVL_COUNT] = {false};
      constexpr static bool prilvl_enabled[DMA_PRILVL_COUNT] = {false};
      constexpr static bool chRunStandby[DMAC_CH_NUM] = {false};
      constexpr static int chPrilvl[DMAC_CH_NUM] = {2};
    
    }config;

    struct ctrlGroup {      

      static inline bool init(const bool&); // DONE
      static inline bool init();

      static inline bool enabled(const bool&); // DONE
      static inline bool enabled();

      static inline bool errorCallback(errorCallbackType&); // DONE
      static inline bool errorCallback();

      static inline bool transferCallback(transferCallbackType&); // DONE
      static inline bool transferCallback();

    }ctrl;


    struct activeChannelGroup {

      static inline int remainingBytes(); // DONE

      static inline int index(); // DONE

      static inline bool isBusy(); // DONE

    }activeChannel;


    struct channelGroup {
      const int index;

      inline bool init(const bool&); // DONE
      inline bool init();

      inline bool state(const CHANNEL_STATE&); // DONE
      inline CHANNEL_STATE state();

      inline bool descriptor(transferDescriptor);

      inline CHANNEL_ERROR error(); // DONE

      inline bool triggerSource(const TRIGGER_SOURCE&); // DONE
      inline TRIGGER_SOURCE triggerSource();

      inline bool transferType(const TRANSFER_TYPE&); // DONE
      inline TRANSFER_TYPE transferType();

      inline bool burstThreshold(const int&); // DONE
      inline int burstThreshold();

      struct lastTransfer {

        // TO DO...

      }lastTransfer;

    }channel[DMAC_CH_NUM]{{.index = init_seq(channel, 0, 1)}};


    typedef class transferDescriptor {
      friend dma::channelGroup;

      bool source(const void*);
      bool source(const volatile void*);
      void* source();

      bool destination(const void*);
      bool destination(const volatile void*);
      void* destination();

      bool sourceIncrement(const int&);
      int sourceIncrement();

      bool destIncrement(const int&);
      int destIncrement();

      bool transferCount(const int&);
      int transferCount();

      bool dataSize(const int&);
      int dataSize();

      bool suspendMode(const bool&);
      bool suspendMode();

      bool valid(const bool&);
      bool valid();

      protected:
        uintptr_t sourceAddr;
        uintptr_t destAddr;
        DmacDescriptor desc;
    };

  }dma;


  enum dma::CHANNEL_ERROR {
    ERROR_NONE,
    ERROR_CRC,
    ERROR_DESC,
    ERROR_TRANSFER
  };

  enum dma::CHANNEL_STATE {
    STATE_DISABLED,
    STATE_IDLE,
    STATE_SUSPENDED,
    STATE_ACTIVE
  };

  enum dma::TRANSFER_TYPE {
    ACTION_TRANSFER_BLOCK = 0,
    ACTION_TRANSFER_BURST = 2,
    ACTION_TRANSFER_ALL   = 3
  };

  enum dma::TRIGGER_SOURCE {
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
}











///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: CHANNEL MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////

/*
typedef struct transferDescriptor { 
  friend channel; 
  transferDescriptor();
  transferDescriptor(const transferDescriptor&);
  transferDescriptor(DmacDescriptor*);

  inline void operator = (const transferDescriptor&);

  struct addr {
    inline bool operator = (const void*);
    inline operator void*() const;
    inline operator uintptr_t() const;
    _multi_(transferDescriptor, addr);
  }src{this, 0}, dest{this, 1};

  struct increment {
    bool operator = (const int&);
    operator int() const;
    _multi_(transferDescriptor, increment);
  }incrementSrc{this, 0}, incrementDest{this, 1};

  template<int T>
  void value() {

  }
  
  struct dataSize {
    inline bool operator = (const int&);
    inline operator int() const;
    _super_(transferDescriptor, dataSize);
  }dataSize{this};

  struct length {
    inline bool operator = (const int&);
    inline operator int() const;
    _super_(transferDescriptor, length);
  }length{this};

  struct suspOnCompl {
    inline void operator = (const bool&);
    inline operator bool() const;
    _super_(transferDescriptor, suspOnCompl);
  }suspOnCompl{this};

  struct valid {
    inline bool operator = (const bool&);
    inline operator bool() const;
    _super_(transferDescriptor, valid);
  }valid{this};

  inline void reset();

  private:
    DmacDescriptor *descriptor;
};

*/