
#include "SYSv2.h"

#define DFLL_BASE_FREQ 48000000


void set_osculp(bool enabled) {
  if (enabled) {
    OSC32KCTRL->OSCULP32K.reg |=
        OSC32KCTRL_OSCULP32K_EN1K
      | OSC32KCTRL_OSCULP32K_EN32K;
  
  } else {
    OSC32KCTRL->OSCULP32K.reg = 0;
  }
}


void set_xosc32k(bool enabled = true, int startupTime = 0, 
  bool waitForLock = false) {
  
  // Enable/Disable xosc32k
  if (enabled && !OSC32KCTRL->XOSC32K.bit.ENABLE) { 
    OSC32KCTRL->XOSC32K.reg |=
        OSC32KCTRL_XOSC32K_ENABLE
      | OSC32KCTRL_XOSC32K_STARTUP(CLAMP<uint8_t>(log2(startupTime), 0, 6));
  }
  // xosc32k config
  OSC32KCTRL->XOSC32K.reg =
      ((uint8_t)xosc32k_config.highSpeedMode << OSC32KCTRL_XOSC32K_CGM_Pos)
    | ((uint8_t)xosc32k_config.onDemand << OSC32KCTRL_XOSC32K_ONDEMAND_Pos)
    | ((uint8_t)xosc32k_config.runInStandby << OSC32KCTRL_XOSC32K_RUNSTDBY_Pos)
    | ((uint8_t)xosc32k_config.output32khz << OSC32KCTRL_XOSC32K_EN32K_Pos)
    | ((uint8_t)xosc32k_config.output1khz << OSC32KCTRL_XOSC32K_EN1K_Pos)
    | ((uint8_t)xosc32k_config.outputSignal << OSC32KCTRL_XOSC32K_XTALEN_Pos)
    | (OSC32KCTRL->XOSC32K.bit.ENABLE << OSC32KCTRL_XOSC32K_ENABLE_Pos);

  // xosc32k failiure ctrl
  OSC32KCTRL->CFDCTRL.reg |= 
      ((uint8_t)xosc32k_config.enableCFD << OSC32KCTRL_CFDCTRL_CFDEN_Pos)
    | OSC32KCTRL_CFDCTRL_SWBACK;

  // Wait for lock
  while(waitForLock && !OSC32KCTRL->STATUS.bit.XOSC32KRDY);
}


bool set_dfll(bool enabled, uint32_t freq, bool closedCycleMode, bool waitForLock) {
  if (freq < DFLL_BASE_FREQ) return false;

  // Enable/Disable dfll
  if (enabled) {

    uint16_t mulVal = dfll_config.ceilFreq ? 

    OSCCTRL->DFLLCTRLA.bit.ENABLE = 1;


    if (closedCycleMode) {



    } else {


    }

  } else {
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 0;
  }

  // dfll config
  OSCCTRL->DFLLCTRLA.reg = 
      ((uint8_t)dfll_config.onDemand << OSCCTRL_DFLLCTRLA_ONDEMAND_Pos)
    | ((uint8_t)dfll_config.runInStandby << OSCCTRL_DFLLCTRLA_RUNSTDBY_Pos)
    | (OSCCTRL->DFLLCTRLA.bit.ENABLE << OSCCTRL_DFLLCTRLA_ENABLE_Pos);

  OSCCTRL->DFLLCTRLB.reg = 
      (!(uint8_t)dfll_config.quickLockEnabled << OSCCTRL_DFLLCTRLB_QLDIS_Pos)
    | (!(uint8_t)dfll_config.chillCycle << OSCCTRL_DFLLCTRLB_CCDIS_Pos)
    | ((uint8_t)dfll_config.usbRecoveryMode << OSCCTRL_DFLLCTRLB_USBCRM_Pos)
    | (!(uint8_t)dfll_config.stabalizeFreq << OSCCTRL_DFLLCTRLB_STABLE_Pos)
    | (OSCCTRL->DFLLCTRLB.bit.MODE << OSCCTRL_DFLLCTRLB_MODE_Pos);
  while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

  OSCCTRL->DFLLMUL.reg = 
      OSCCTRL_DFLLMUL_FSTEP(dfll_config.maxFineAdj)
    | OSCCTRL_DFLLMUL_CSTEP(dfll_config.maxCoarseAdj)
    | OSCCTRL_DFLLMUL_MUL(OSCCTRL->DFLLMUL.bit.MUL);
  while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

  // Wait for lock
  while(waitForLock && !OSCCTRL->STATUS.bit.DFLLRDY);
}

