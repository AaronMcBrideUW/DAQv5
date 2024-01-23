///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Board.h>
#include "sam.h"

#define CLK_SOURCE_COUNT (OSCCTRL_XOSCS_NUM + OSCCTRL_DPLLS_NUM \ 
  + OSCCTRL_DFLLS_NUM + 2)

template<typename T> struct Setting;
class System_;
struct CLK_SRC;
enum SYS_PERIPHERAL;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYSTEM CONFIG
///////////////////////////////////////////////////////////////////////////////////////////////////

enum SYS_PERIPHERAL : uint8_t { // Num must correspond with GCLK-> ch num
  PERIPH_NONE,
  PERIPH_SERCOM0 = 3,
  PERIPH_SERCOM1 = 4, 
  PERIPH_SERCOM2,
  PERIPH_SERCOM3,
  PERIPH_SERCOM4 //...
};

enum PIN_CONFIG : uint8_t {
  INPUT_PIN,
  OUTPUT_PIN
};

struct SystemConfig { 
  struct {                 
    struct {                    
      bool enabled = true;
      bool runInStandby = false;
      bool exportSignal = false;
      uint8_t startupTimeConfig = 2;
      bool alwaysOn = true;
    }XOSC32K;

    struct {
      bool enabled[OSCCTRL_XOSCS_NUM] = { false, false };
      uint32_t freq[OSCCTRL_XOSCS_NUM] = {8000000, 8000000};
      uint8_t startupTimeSel = 4;
      bool autoLoopCtrl = true;
      bool lowBufferGain = false;
      bool onDemand = false;
      bool runInStandby = false;
    }XOSC;

    struct {
      bool enabled = true;
      uint32_t freq = 48000000;
      bool runInStandby = false;
      bool onDemand = true;
      uint8_t maxFineAdjStep = 10;
      uint8_t maxCoarseAdjStep = 0;
      uint8_t fineAdjStep = 0;
      uint8_t coarseAdjStep = 0;
      bool waitForLock = false;
      bool quickLockEnabled = true;
      bool chillCycle = false;
      bool stabalizeFreq = true; 
      bool closedLoopMode = true;
    }DFLL;

    struct {
      bool enabled[OSCCTRL_DPLLS_NUM] = {true, true};
      uint32_t freq[OSCCTRL_DPLLS_NUM] = {120000000, 120000000};
      bool onDemand = false;
      bool runInStandby = false;
      uint8_t xoscDivFactor = 0;
      bool enableDCOFilter = false;
      uint8_t dcoFilterSelection = 0;   // See data sheet
      bool integralFilterSelection = 0; // See data sheetd
      uint8_t lockTimeoutSelection = 0;
    }DPLL;

    struct {
      bool enabled[OSCCTRL_XOSCS_NUM] = {false, false};
      uint32_t freq[OSCCTRL_XOSCS_NUM] = { };
      bool runInStandby = false;
      bool onDemand = true;
    }XOSC;

    struct {
      uint16_t minFreqDiff = 10;
      bool runInStandby = false;
      bool improveDutyCycle = false;
    }GEN;

    struct {
      bool failiureDetection = true;
      uint8_t irqPriority = 2;
      uint8_t cpuSrc = GCLK_SOURCE_DPLL0;
      uint8_t cpuDivSelection = 1;
    }MISC;
  }CLK;

  struct {
    struct {

    }SEEPROM;
  }NVM;
};

// Default startup
const extern SystemConfig &defaultStartup;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYSTEM CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class System_ {

  public:

    struct CLK_ {

      int8_t allocGCLK(uint32_t freq, uint16_t maxFreqOffset, SYS_PERIPHERAL peripheral);

      bool freeGCLK(uint8_t gclkNum);

      protected:
        friend System_;
        const System_ *super;
        explicit CLK_(System_ *super) : super(super) {} 

        bool init = false;
        uint32_t sources[CLK_SOURCE_COUNT] = { 0 };
        bool agclk[GCLK_NUM] = {false};
        int8_t periphAlloc[GCLK_GEN_NUM] = {-1};

        bool initialize();

    }CLK{this};

    struct PIN_ {
      
      bool configurePin(SYS_PERIPHERAL peripheral, PIN_CONFIG config, bool enablePullup);


      private:
        friend System_;
        const System_ *super;
        explicit PIN_(System_ *sys) : super(sys);
    }PIN{this};

  private:
    friend CLK_;
    SystemConfig *config;

    System_() {}
};

extern System_ &SYS;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYSTEM SETTING STRUCT
///////////////////////////////////////////////////////////////////////////////////////////////////

enum MINMAX_MODE : uint8_t {
  MINMAX_NONE,
  WRAP_VALUE,
  CLAMP_VALUE
};

enum VALIDSET_MODE : uint8_t {
  VALIDSET_NONE,
  CEIL_VALUE,
  FLOOR_VALUE,
  ROUND_VALUE
};

template<typename T> struct Setting {

  Setting(T min, T max, MINMAX_MODE mode, T defaultValue) 
    : settingType(1), defaultValue(defaultValue) {

    static_assert(min < max, "Min & max values are invalid");
    testType();

    value = defaultValue;
    this->minmax.min = min;
    this->minmax.max = max;
    this->minmax.mode = mode;
  }

  Setting(T *validSet, uint16_t validSetLength, VALIDSET_MODE mode, 
    T defaultValue) : settingType(2), defaultValue(defaultValue) {

    static_assert(validSet != nullptr && validSetLength > 0, "Valid set is invalid");
    testType();

    value = defaultValue;
    this->set.validSet = validSet;
    this->set.validSetLength = validSetLength;
    this->set.mode = mode;
  }

  Setting(T defaultValue) : settingType(0), defaultValue(defaultValue) {
    testType();
    value = defaultValue;
  }

  bool equals(const T &newValue) {

    if (settingType == 0) {
      value = newValue;
      return true;

    } else if (settingType == 1) {
      switch(minmax.mode) {

        case MINMAX_NONE: {
          if (newValue <= minmax.max && newValue >= minmax.min ) {
            value = newValue;
            return true;
          } else {
            return false;
          }
        }
        case CLAMP_VALUE: {
          if (newValue > minmax.max) {
            value = minmax.max;
          } else if (newValue < minmax.min) {
            value = minmax.min;
          } else {
            value = newValue;
          }
          return true;
        }
        case WRAP_VALUE: {
          if (newValue > minmax.max) {
            value = minmax.min + ((newValue - minmax.min) % (minmax.max - minmax.min));
          } else if (newValue < minmax.min) {
            value =  minmax.max - ((minmax.min - newValue) % (minmax.max - minmax.min));
          }
          return true;
        }
      }
    } else if (settingType == 2) {
      switch(set.mode) {

        case VALIDSET_NONE: {
          for (uint16_t i = 0; i < set.validSetLength; i++) {
            if (set.validSet[i] == newValue) {
              value = newValue;
              return true;
            }
          }
          return false;
        }
        case CEIL_VALUE: {
          for (uint16_t i = 0; i < set.validSetLength; i++) {
            if (set.validSet[i] > newValue) {
              value = set.validSet[i];
              return true;
            }
          }
          value = set.validSet[set.validSetLength - 1];
          return true;
        }
        case FLOOR_VALUE: {
          for (uint16_t i = set.validSetLength - 1; i >= 0; i--) {
            if (set.validSet[i] < newValue) {
              value = set.validSet[i];
              return true;
            }
          }
          value = set.validSet[0];
          return true;
        }
        case ROUND_VALUE: {
          if (newValue < set.validSet[0]) {
            value = set.validSet[0];
            
          } else if (newValue > set.validSet[set.validSetLength - 1]) {
            value = set.validSet[set.validSetLength - 1];  

          } else {
            for (int i = 0; i < set.validSetLength - 1; i++) {
              if (newValue > set.validSet[i] && newValue < set.validSet[i + 1]) {
                if (newValue <= (set.validSet[i + 1] - set.validSet[i])) {
                  value = set.validSet[i];
                } else {
                  value = set.validSet[i + 1];
                }
              }
            }
          }
          return true;
        }
      }
    }
    return false;
  }

  const T &get() { return value; }

  void setDefault() { value = defaultValue; }

  //// OPERATORS ///////////////////////////////////////////

  bool operator = (const T &newValue) { return set(newValue); }

  const T &operator * () const { return value; }

  bool operator == (const T &otherValue) const { return (value == otherValue); }

  private:  
    T value;
    const T defaultValue;
    const uint8_t settingType; // 0 = no restrict, 1 = min/max, 2 = set
    bool locked = false;

    union {
      struct {
        T *validSet = nullptr;
        uint16_t validSetLength = 0;
        VALIDSET_MODE mode = VALIDSET_NONE;
      }set;
      struct {
        T min = T();
        T max = T();
        MINMAX_MODE mode = MINMAX_NONE;
      }minmax;
    };

    void testType() {
      T t1 = T();
      T t2 = T();
      bool result = true;
      result = t1 < t2;
      result = t1 > t2;
      result = t1 <= t2;
      result = t1 >= t2;
      result = t1 % t2;
      result = t1 == t2;
    }
};


