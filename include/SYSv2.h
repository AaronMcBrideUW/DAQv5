
#pragma once
#include "sam.h"
#include "math.h"
#include "Utilities.h"

typedef void (*oscCB)(OSC_ID, OSC_STATUS);

enum OSC_ID : unsigned int {

};

enum OSC_STATUS : unsigned int {

};


void set_osculp(bool enabled);

int get_osculp_freq();

struct {
  bool highSpeedMode = false;
  bool onDemand = true;
  bool runInStandby = false;
  bool outputSignal = false;
  bool enableCFD = true;
}xosc32k_config;

enum XOSC32K_OUTPUT {
  XOSC32K_OUTPUT_1KHZ,
  XOSC32K_OUTPUT_32KHZ
};

void set_xosc32k(bool enabled = true, XOSC32K_OUTPUT outputSel, unsigned int startupTime = 0, bool waitForLock = false);

int get_xosc32k_freq();

struct {
  bool onDemand = true;
  bool runInStandby = false;
  bool quickLockEnabled = false;
  bool chillCycle = true;
  bool usbRecoveryMode = true;
  bool stabalizeFreq = true;
  bool ceilFreq = false;
  uint8_t maxFineAdj = 1;
  uint8_t maxCoarseAdj = 1;
  uint8_t coarseAdj = 0;
  uint8_t fineAdj = 0;
}dfll_config;

bool set_dfll(bool enabled, unsigned int freq, bool closedLoopMode, bool waitForLock);

struct {
  bool onDemand = true;
  bool runInStandby = false;
  uint8_t dcoFilterSel = 0;
  bool lockTimeout = true;
  bool wakeUpFast = false;
  uint8_t integralFilterSel = 0;
  bool ceilFreq;
}dpll_config[OSCCTRL_DPLLS_NUM];

enum DPLL_REF : uint8_t {
  DPLL_REF_GCLK0 = 0,
  DPLL_REF_XOSC32K = 1,
  DPLL_REF_XOSC0 = 2,
  DPLL_REF_XOSC1 = 3
};

bool set_dpll(unsigned int dpllNum, bool enabled, DPLL_REF reference, 
  unsigned int freq);

