///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "sam.h"
#include "math.h"
#include "Utilities.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> GCLK
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  bool runInStandby = false;
  bool improveDutyCycle = false;
  bool enableOutput = false;
}gclk_config;


bool set_gclk(int gclkNum, bool enabled, uint8_t source, int freq);

bool link_gclk(int gclkNum, bool linkEnabled, int channelNum);

int get_gclk_freq();


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> OSCULP32K 
///////////////////////////////////////////////////////////////////////////////////////////////////

#define OSCULP_EN1K_FREQ 1024;
#define OSCULP_EN32K_FREQ 32746;

bool set_osculp(bool enabled);

int get_osculp_freq();


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> XOSC32K 
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Specifies xosc configuration
struct {
  bool highSpeedMode = false;
  bool onDemand = true;
  bool runInStandby = false;
  bool outputSignal = false;
  bool enableCFD = true;
}xosc32k_config;

/// @brief Available xosc frequencies
enum XOSC32K_FREQ {
  XOSC_FREQ_NULL,
  XOSC32K_FREQ_1KHZ,
  XOSC32K_FREQ_32KHZ
};

bool set_xosc32k(bool enabled, XOSC32K_FREQ freq = XOSC_FREQ_NULL, int startupTime = 0, 
  bool waitForLock = false);

int get_xosc32k_freq();


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DFLL
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  bool onDemand = true;
  bool runInStandby = false;
  bool quickLockEnabled = false;
  bool chillCycle = true;
  bool usbRecoveryMode = true;
  bool stabalizeFreq = true;
  uint8_t maxFineAdj = 1;
  uint8_t maxCoarseAdj = 1;
  uint8_t coarseAdj = 0;
  uint8_t fineAdj = 0;
}dfll_config;

bool set_dfll(bool enabled, int freq = 0, bool closedLoopMode = false, 
  bool waitForLock = true);

int get_dfll_freq();

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DPLL
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  bool onDemand = true;
  bool runInStandby = false;
  bool dcoFilterEnabled = false;
  uint8_t dcoFilterSel = 0;
  bool lockTimeout = true;
  bool wakeUpFast = false;
  uint8_t integralFilterSel = 0;
  bool ceilFreq = false;
  unsigned int maxFreqOffset = 10000;
}dpll_config[OSCCTRL_DPLLS_NUM];

enum DPLL_SRC : uint8_t {
  DPLL_SRC_NULL = 2,
  DPLL_SRC_GCLK0 = 0,
  DPLL_SRC_XOSC32K = 1
};


bool set_dpll(int dpllNum, bool enabled, DPLL_SRC source = DPLL_SRC_NULL, int freq = 0, 
  bool waitForLock = true);

int get_dpll_freq(int dpllNum);