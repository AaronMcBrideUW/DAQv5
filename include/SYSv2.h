
#pragma once
#include "sam.h"
#include "math.h"
#include "Utilities.h"

typedef void (*oscCB)(OSC_ID, OSC_STATUS);

enum OSC_ID : uint8_t {

};

enum OSC_STATUS : uint8_t {

};


struct {
  bool highSpeedMode = false;
  bool onDemand = true;
  bool runInStandby = false;
  bool output1khz = true;
  bool output32khz = true;
  bool outputSignal = false;
  bool enableCFD = true;
}xosc32k_config;

void set_osculp(bool enabled);

void set_xosc32k(bool enabled = true, int startupTime = 0, bool waitForLock = false);


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

bool set_dfll(bool enabled, uint32_t freq, bool closedCycleMode, bool waitForLock);

