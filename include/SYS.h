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
struct CLK_SOURCE_DESCRIPTOR;


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYSTEM CONFIG
///////////////////////////////////////////////////////////////////////////////////////////////////


struct SystemConfig {
  struct {
    struct {
      bool runInStandby = false;
      bool exportSignal = false;
    }XOSC32K;
    struct {
      bool runInStandby = false;
      bool onDemand = true;
      uint8_t maxFineAdjStep = 10;
      uint8_t maxCoarseAdjStep = 0;
      uint8_t fineAdjStep = 0;
      uint8_t coarseAdjStep = 0;
      bool chillCycle = false;
      bool stabalizeFreq = true; 
      bool closedLoopMode = true;
    }DFLL;
    struct {
      bool onDemand = true;
      bool runInStandby = false;
      uint8_t xoscDivFactor = 0;
      bool enableDCOFilter = false;
      uint8_t dcoFilterSelection = 0;   // See data sheet
      bool integralFilterSelection = 0; // See data sheetd
      uint8_t lockTimeoutSelection = 0;
    }DPLL;
    struct {
      bool runInStandby = false;
      bool improveDutyCycle = false;
    }GEN;
    struct {
      uint8_t irqPriority = 2;
    }MISC;
    /*
    CLK_SOURCE_DESCRIPTOR SRC_CONFIG[8] = { 
      {true, OSCULP32K, 32000},
      {false, XOSC32K, 0},
      {false, XOSC0, 0},
      {false, XOSC1, 0},
      {false, DFLL, 0},
      {false, DPLL0, 0},
      {false, DPLL1, 0} 
    };
    */
  }CLK;
};
extern SystemConfig &defaultStartup;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYSTEM CLASS
///////////////////////////////////////////////////////////////////////////////////////////////////

class System_ {

  public:

    SystemConfig *config;

    struct CLK_ {

      int8_t allocGCLK(const uint32_t &targetFreq);

      bool freeGCLK(uint8_t gclkNum);

      protected:
        friend System_;
        const System_ *super;
        explicit CLK_(System_ *super) : super(super) {} 

        bool init = false;
        bool agclk[GCLK_NUM] = { 0 };
        
        void initialize();

        bool setXOSC32K(bool enabled, bool highSpeed);

        bool setDFLL(bool enabled, uint16_t multiplier);

        bool setDPLL(uint8_t dpllNum, bool enabled, DPLL_REFERENCE ref,  uint8_t muliplyNumerator, 
          uint8_t multiplyDenominator);

        void setSource(SOURCE_ID id, uint32_t freq);

    }CLK{this};

  private:
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


