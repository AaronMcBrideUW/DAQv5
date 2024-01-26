
#include "SYSv2.h"

#define XOSC32K_SU_MAX 6
#define XOSC32K_EN1K_FREQ 1000
#define XOSC32K_EN32K_FREQ 32768
#define DFLL_BASE_FREQ 48000000

int getGclkFreq(unsigned int gclkNum) {
  if (gclkNum >= GCLK_NUM) return -1;

  switch(GCLK->GENCTRL[gclkNum].bit.SRC) {
    case 4: {
      return get_osculp_freq();
    }
    case 5: {
      return get_xosc32k_freq();
    }
  }
  /////// NEED TO COMPLETE!
}


void set_osculp(bool enabled) {
  if (enabled) {
    OSC32KCTRL->OSCULP32K.reg |=
        OSC32KCTRL_OSCULP32K_EN1K
      | OSC32KCTRL_OSCULP32K_EN32K;
  
  } else {
    OSC32KCTRL->OSCULP32K.reg = 0;
  }
}


int get_osculp_freq() {
  if (OSC32KCTRL->OSCULP32K.bit.EN1K) return XOSC32K_EN1K_FREQ;
  if (OSC32KCTRL->OSCULP32K.bit.EN32K) return XOSC32K_EN32K_FREQ;
  return 0;
}


void set_xosc32k(bool enabled = true, XOSC32K_OUTPUT outputSel, unsigned int startupTime = 0, 
  bool waitForLock = false) {
  
  // enable
  if (enabled) { 
    OSC32KCTRL->XOSC32K.reg |=
        OSC32KCTRL_XOSC32K_ENABLE
      | OSC32KCTRL_XOSC32K_STARTUP(CLAMP<uint32_t>(log2(startupTime), 0, XOSC32K_SU_MAX));
  
    // set config 
    OSC32KCTRL->XOSC32K.reg =
        (static_cast<uint8_t>(xosc32k_config.highSpeedMode) << OSC32KCTRL_XOSC32K_CGM_Pos)
      | (static_cast<uint8_t>(xosc32k_config.onDemand) << OSC32KCTRL_XOSC32K_ONDEMAND_Pos)
      | (static_cast<uint8_t>(xosc32k_config.runInStandby) << OSC32KCTRL_XOSC32K_RUNSTDBY_Pos)
      | ((outputSel == XOSC32K_OUTPUT_1KHZ ? 1 : 0) << OSC32KCTRL_XOSC32K_EN1K_Pos)
      | ((outputSel == XOSC32K_OUTPUT_32KHZ ? 1 : 0) << OSC32KCTRL_XOSC32K_EN32K_Pos)
      | (static_cast<uint8_t>(xosc32k_config.outputSignal) << OSC32KCTRL_XOSC32K_XTALEN_Pos)
      | (OSC32KCTRL->XOSC32K.bit.ENABLE << OSC32KCTRL_XOSC32K_ENABLE_Pos);

    // set failiure ctrl
    OSC32KCTRL->CFDCTRL.reg |= 
        ((uint8_t)xosc32k_config.enableCFD << OSC32KCTRL_CFDCTRL_CFDEN_Pos)
      | OSC32KCTRL_CFDCTRL_SWBACK;

    // Wait for lock
    while(waitForLock && !OSC32KCTRL->STATUS.bit.XOSC32KRDY); 

  // disable   
  } else {
    OSC32KCTRL->XOSC32K.bit.ENABLE = 0;
      while(OSC32KCTRL->STATUS.bit.XOSC32KRDY);
  }
}


int get_xosc32k_freq() {
  if (OSC32KCTRL->XOSC32K.bit.ENABLE) {
    if (OSC32KCTRL->XOSC32K.bit.EN1K) return XOSC32K_EN1K_FREQ;
    if (OSC32KCTRL->XOSC32K.bit.EN32K) return XOSC32K_EN32K_FREQ;
  }
  return 0;
}


bool set_dfll(bool enabled, unsigned int freq, bool closedLoopMode, bool waitForLock) {
  if (freq < DFLL_BASE_FREQ) return false;

  if (enabled) {
    // dfll multiplier
    OSCCTRL->DFLLMUL.bit.MUL = static_cast<uint16_t>(dfll_config.ceilFreq ? 
      DIV_CEIL<int>(freq, DFLL_BASE_FREQ) : DIV<int>(freq, DFLL_BASE_FREQ));
    while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

    // dfll calibration 
    if (!closedLoopMode) {
      OSCCTRL->DFLLVAL.reg = 
          OSCCTRL_DFLLVAL_COARSE(dfll_config.coarseAdj)
        | OSCCTRL_DFLLVAL_FINE(dfll_config.fineAdj);
        while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);
    }
    // dfll enable
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 1;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE);

    OSCCTRL->DFLLVAL.reg = OSCCTRL->DFLLVAL.reg;
      while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

    // dfll wait for lock
    if (closedLoopMode && waitForLock) {
      while(!OSCCTRL->STATUS.bit.DFLLRDY);
    }
    // dfll config
    OSCCTRL->DFLLCTRLA.reg = 
        (static_cast<uint8_t>(dfll_config.onDemand) << OSCCTRL_DFLLCTRLA_ONDEMAND_Pos)
      | (static_cast<uint8_t>(dfll_config.runInStandby) << OSCCTRL_DFLLCTRLA_RUNSTDBY_Pos)
      | (OSCCTRL->DFLLCTRLA.bit.ENABLE << OSCCTRL_DFLLCTRLA_ENABLE_Pos);

    OSCCTRL->DFLLCTRLB.reg = 
        (!static_cast<uint8_t>(dfll_config.quickLockEnabled) << OSCCTRL_DFLLCTRLB_QLDIS_Pos)
      | (!static_cast<uint8_t>(dfll_config.chillCycle) << OSCCTRL_DFLLCTRLB_CCDIS_Pos)
      | (static_cast<uint8_t>(dfll_config.usbRecoveryMode) << OSCCTRL_DFLLCTRLB_USBCRM_Pos)
      | (!static_cast<uint8_t>(dfll_config.stabalizeFreq) << OSCCTRL_DFLLCTRLB_STABLE_Pos)
      | (OSCCTRL->DFLLCTRLB.bit.MODE << OSCCTRL_DFLLCTRLB_MODE_Pos);
    while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

    OSCCTRL->DFLLMUL.reg = 
        OSCCTRL_DFLLMUL_FSTEP(dfll_config.maxFineAdj)
      | OSCCTRL_DFLLMUL_CSTEP(dfll_config.maxCoarseAdj)
      | OSCCTRL_DFLLMUL_MUL(OSCCTRL->DFLLMUL.bit.MUL);
    while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

    // Wait for lock
    if (waitForLock) {
      while(closedLoopMode && (!OSCCTRL->STATUS.bit.DFLLLCKF || !OSCCTRL->STATUS.bit.DFLLLCKC));
      while(!OSCCTRL->STATUS.bit.DFLLRDY);      
    }
  // dfll disable
  } else {
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 0;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE && OSCCTRL->STATUS.bit.DFLLRDY);
  }
}



bool set_dpll(unsigned int dpllNum, bool enabled, DPLL_REF reference, unsigned int freq) {
  if (dpllNum > OSCCTRL_DPLLS_NUM) return false;
  #define dpll OSCCTRL->Dpll[dpllNum]
  #define config dpll_config[dpllNum]
  
  if (enabled) {

    dpll.DPLLCTRLA.bit.ENABLE = 1;
      while(dpll.DPLLSYNCBUSY.bit.ENABLE);

    int srcFreq = 0;
    switch(reference) {
      case DPLL_REF_GCLK0: {
        int gclkSrc = GCLK->GENCTRL[OSCCTRL_GCLK_ID_FDPLL0].bit.SRC;
        srcFreq = getGclkFreq(OSCCTRL_GCLK_ID_FDPLL0 + dpllNum);
      }
      case DPLL_REF_XOSC0: {
        //// TO DO
      }
      case DPLL_REF_XOSC1: {
        //// TO DO
      }
      case DPLL_REF_XOSC32K: {
        srcFreq = get_xosc32k_freq();
      }
    }

    if (srcFreq > freq) {
      // freq = src freq / ldr
      dpll.DPLLRATIO.bit.LDRFRAC = static_cast<uint8_t>
        (DIV_INT(srcFreq, freq, dpll_config.ceilFreq));
    }

    dpll.DPLLCTRLB.bit.REFCLK = static_cast<uint8_t>(reference);
    

  } else {

  }
}




