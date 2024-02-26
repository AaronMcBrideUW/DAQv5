        void replaceTask();

#pragma once
#include <peripheral_core/core_util.h>
#include <type_traits>
#include <sam.h>
#include <string.h>
#include <initializer_list>


#define DMA_IRQ_COUNT 5
#define DMA_PRILVL_COUNT 4
#define DMA_IRQPRI_MAX 5 
#define DMA_QOS_MAX 3

#define _setDesc_(_arrSize_, _isSrc_) return setDesc_((void*)ptr, \ 
sizeof(T), std::is_volatile<T>::value, _arrSize_, _isSrc_);

namespace core {

  class channelTask;

  struct dma {

    typedef void (*transferCallbackType)(int channelIndex);
    typedef void (*errorCallbackType)(int channelIndex, enum ERROR_TYPE);  

    enum CHANNEL_STATE : int;
    enum CHANNEL_ERROR : int;
    enum PERIPHERAL_LINK : int;
    enum TRANSFER_MODE : int;
    enum CRC_MODE : int;
    enum CRC_STATUS : int;

    struct configGroup;
    struct ctrlGroup;
    struct activeChannelGroup;
    struct crcGroup;
    struct channelGroup;
    class taskDescriptor;

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

      static inline bool busy(); // DONE

    }activeChannel;


    struct crcGroup {

      static inline bool inputChannel(const int&);
      static inline int inputChannel();

      static inline bool mode(const CRC_MODE&);
      static inline CRC_MODE mode();

      static inline CRC_STATUS status();

      /// @brief Set as address of task to input/output data
      ///        to the CRC module.
      static struct {}CRC_INPUT;
      static struct {}CRC_OUTPUT;

    }crc;


    struct channelGroup {

      const int index;

      inline bool init(const bool&); // DONE
      inline bool init() const;

      inline bool state(const CHANNEL_STATE&); // DONE
      inline CHANNEL_STATE state() const;
      
      inline bool linkedPeripheral(const PERIPHERAL_LINK&); // DONE
      inline PERIPHERAL_LINK linkedPeripheral() const;

      inline bool transferMode(const TRANSFER_MODE&); // DONE
      inline TRANSFER_MODE transferMode() const;

      inline CHANNEL_ERROR error() const; // DONE

      inline dma::taskDescriptor writebackDescriptor(); // DONE

      bool setTasks(std::initializer_list<taskDescriptor&>); // DONE
      bool addTask(const int&, dma::taskDescriptor&);
      dma::taskDescriptor &removeTask(const int&);
      dma::taskDescriptor &getTask(const int&);

      int taskCount();  // DONE
      bool clearTasks();

    }channel[DMAC_CH_NUM]{{.index = init_seq(channel, 0, 1)}};


    class taskDescriptor {
      taskDescriptor();
      taskDescriptor(DmacDescriptor*);
      taskDescriptor(const taskDescriptor&); // DONE

      taskDescriptor operator = (const taskDescriptor&); // DONE
      explicit operator DmacDescriptor*();
      operator bool() const;

      template<typename T>
      bool source(const T *ptr) { _setDesc_(1, true); } // DONE
      template<typename T, size_t N>
      bool source(const T (*ptr)[N]) { _setDesc_(N, true); }
      inline void *source() const;

      template<typename T>
      bool destination(const T *ptr) { _setDesc_(1, false); } // DONE
      template<typename T, size_t N>
      bool destination(const T (*)[N]) { _setDesc_(N, false); }
      inline void *destination() const;

      inline bool enabled(const bool&); // DONE
      inline bool enabled() const;

      bool length(const int&); // DONE
      int length() const;

      inline bool suspendChannel(const bool&); // DONE
      inline bool suspendChannel() const;

      inline dma::taskDescriptor &linkedTask(); // DONE
      inline int assignedChannel();
       
      inline void reset();

      ~taskDescriptor(); // DONE ///////// NEED TO ADD BETTER HANDLEING HERE...

      private:
        friend channelGroup;
        bool setDesc_(const void*, const int&, const bool&, 
          const int&, const bool&);
        bool alloc;
        int srcAlign, destAlign, assignedCh;
        DmacDescriptor *desc;
        taskDescriptor *linked;
    };
    

  }dma;

  enum dma::CRC_STATUS {
    CRC_DISABLED,
    CRC_IDLE,
    CRC_BUSY,
    CRC_ERROR
  };

  enum dma::CRC_MODE {
    CRC_TYPE_16,
    CRC_TYPE_32,
  };

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

  enum dma::TRANSFER_MODE { 
    MODE_TRANSFER_1VALUE  = 1,
    MODE_TRANSFER_2VALUE  = 2,
    MODE_TRANSFER_4VALUE  = 4,
    MODE_TRANSFER_8VALUE  = 8,
    MODE_TRANSFER_12VALUE = 12,
    MODE_TRANSFER_16VALUE = 16, 
    MODE_TRANSFER_TASK    = 17,  
    MODE_TRANSFER_ALL     = 18,   
  };

  enum dma::PERIPHERAL_LINK {
    LINK_NONE              = 0,
    LINK_RTC_TIMESTAMP     = 1,
    LINK_DSU_DCC0          = 2,
    LINK_DSU_DCC1          = 3,
    LINK_SERCOM0_RX        = 4,
    LINK_SERCOM0_TX        = 5,
    LINK_SERCOM1_RX        = 6,
    LINK_SERCOM1_TX        = 7,
    LINK_SERCOM2_RX        = 8,
    LINK_SERCOM2_TX        = 9,
    LINK_SERCOM3_RX        = 10,
    LINK_SERCOM3_TX        = 11,
    LINK_SERCOM4_RX        = 12,
    LINK_SERCOM4_TX        = 13,
    LINK_SERCOM5_RX        = 14,
    LINK_SERCOM5_TX        = 15,
    LINK_SERCOM6_RX        = 16,
    LINK_SERCOM6_TX        = 17,
    LINK_SERCOM7_RX        = 18,
    LINK_SERCOM7_TX        = 19,
    LINK_CAN0_DEBUG_REQ    = 20,
    LINK_CAN1_DEBUG_REQ    = 21,
    LINK_TCC0_OOB          = 22,
    LINK_TCC0_COMPARE_0    = 23,
    LINK_TCC0_COMPARE_1    = 24,
    LINK_TCC0_COMPARE_2    = 25,
    LINK_TCC0_COMPARE_3    = 26,
    LINK_TCC0_COMPARE_4    = 27,
    LINK_TCC0_COMPARE_5    = 28,
    LINK_TCC1_OOB          = 29,
    LINK_TCC1_COMPARE_0    = 30,
    LINK_TCC1_COMPARE_1    = 31,
    LINK_TCC1_COMPARE_2    = 32,
    LINK_TCC1_COMPARE_3    = 33,
    LINK_TCC2_OOB          = 34,
    LINK_TCC2_COMPARE_0    = 35,
    LINK_TCC2_COMPARE_1    = 36,
    LINK_TCC2_COMPARE_2    = 37,
    LINK_TCC3_OOB          = 38,
    LINK_TCC3_COMPARE_0    = 39,
    LINK_TCC3_COMPARE_1    = 40,
    LINK_TCC4_OOB          = 41,
    LINK_TCC4_COMPARE_0    = 42,
    LINK_TCC4_COMPARE_1    = 43,
    LINK_TC0_OOB           = 44,
    LINK_TC0_COMPARE_0     = 45,
    LINK_TC0_COMPARE_1     = 46,
    LINK_TC1_OOB           = 47,
    LINK_TC1_COMPARE_0     = 48,
    LINK_TC1_COMPARE_1     = 49,
    LINK_TC2_OOB           = 50,
    LINK_TC2_COMPARE_0     = 51,
    LINK_TC2_COMPARE_1     = 52,
    LINK_TC3_OOB           = 53,
    LINK_TC3_COMPARE_0     = 54,
    LINK_TC3_COMPARE_1     = 55,
    LINK_TC4_OOB           = 56,
    LINK_TC4_COMPARE_0     = 57,
    LINK_TC4_COMPARE_1     = 58,
    LINK_TC5_OOB           = 59,
    LINK_TC5_COMPARE_0     = 60,
    LINK_TC5_COMPARE_1     = 61,
    LINK_TC6_OOB           = 62,
    LINK_TC6_COMPARE_0     = 63,
    LINK_TC6_COMPARE_1     = 64,
    LINK_TC7_OOB           = 65,
    LINK_TC7_COMPARE_0     = 66,
    LINK_TC7_COMPARE_1     = 67,
    LINK_ADC0_RESRDY       = 68,
    LINK_ADC0_SEQ          = 69,
    LINK_ADC1_RESRDY       = 70,
    LINK_ADC1_SEQ          = 71,
    LINK_DAC_EMPTY0        = 72,
    LINK_DAC_EMPTY1        = 73,
    LINK_DAC_RESULT_READY0 = 74,
    LINK_DAC_RESULT_READY1 = 75,
    LINK_I2S_RX0           = 76,
    LINK_I2S_RX1           = 77,
    LINK_I2S_TX0           = 78,
    LINK_I2S_TX1           = 79,
    LINK_PCC_RX            = 80,
    LINK_AES_WRITE         = 81,
    LINK_AES_READ          = 82,
    LINK_QSPI_RX           = 83,
    LINK_QSPI_TX           = 84
  };
}










///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: CHANNEL MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////

/*
typedef struct channelTask { 
  friend channel; 
  channelTask();
  channelTask(const channelTask&);
  channelTask(DmacDescriptor*);

  inline void operator = (const channelTask&);

  struct addr {
    inline bool operator = (const void*);
    inline operator void*() const;
    inline operator uintptr_t() const;
    _multi_(channelTask, addr);
  }src{this, 0}, dest{this, 1};

  struct increment {
    bool operator = (const int&);
    operator int() const;
    _multi_(channelTask, increment);
  }incrementSrc{this, 0}, incrementDest{this, 1};

  template<int T>
  void value() {

  }
  
  struct dataSize {
    inline bool operator = (const int&);
    inline operator int() const;
    _super_(channelTask, dataSize);
  }dataSize{this};

  struct length {
    inline bool operator = (const int&);
    inline operator int() const;
    _super_(channelTask, length);
  }length{this};

  struct suspOnCompl {
    inline void operator = (const bool&);
    inline operator bool() const;
    _super_(channelTask, suspOnCompl);
  }suspOnCompl{this};

  struct valid {
    inline bool operator = (const bool&);
    inline operator bool() const;
    _super_(channelTask, valid);
  }valid{this};

  inline void reset();

  private:
    DmacDescriptor *descriptor;
};

*/