
#include "SYSv2.h"

const uint8_t GCLK_VALID_SRC[] = { 
  GCLK_SOURCE_OSCULP32K, 
  GCLK_SOURCE_XOSC32K, 
  GCLK_SOURCE_DFLL,
  GCLK_SOURCE_DPLL0,
  GCLK_SOURCE_DPLL1
};

#define XOSC32K_EN1K_FREQ 1000
#define XOSC32K_EN32K_FREQ 32768
const uint8_t SUTIME_REF[] = {62, 125, 500, 10000, 20000, 40000, 80000};

#define DFLL_BASE_FREQ 48000000
#define DFLL_FREQ_MIN 12000000
#define DFLL_FREQ_MAX 240000000

#define DPLL_LDRFRAC_DEN 32.0
#define DPLL_FREQ_MAX 1000000000 // TO DO
#define DPLL_FREQ_MIN 8000
#define DPLL_DEFAULT_TIMEOUT_SEL OSCCTRL_DPLLCTRLB_LTIME_1MS_Val

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> GCLK FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////


bool set_gclk(int gclkNum, bool enabled, uint8_t source, int freq) {
  if (gclkNum < 0 || gclkNum > GCLK_GEN_NUM)
    return false;
  
  if (enabled) {
    for (int i = 0; i < sizeof(GCLK_VALID_SRC) / sizeof(GCLK_VALID_SRC[0]); i++) {
      if (GCLK_VALID_SRC[i] == source) {
        break;
      } else if (i >= (sizeof(GCLK_VALID_SRC) / sizeof(GCLK_VALID_SRC[0])) - 1) {
        return false;
      }
    }
    uint16_t divFac = 0;
    if (freq > 0) {
      int srcFreq = 0;
      switch(source) {
        case GCLK_SOURCE_OSCULP32K: {
          srcFreq = get_osculp_freq();
        }
        case GCLK_SOURCE_XOSC32K: {
          srcFreq = get_xosc32k_freq();
        }
        case GCLK_SOURCE_DFLL: {
          srcFreq = get_dfll_freq();
        }
        case GCLK_SOURCE_DPLL0: {
          srcFreq = get_dpll_freq(0);
        }
        case GCLK_SOURCE_DPLL1: {
          srcFreq = get_dpll_freq(1);
        }
      }
      divFac = srcFreq / freq;
      if (srcFreq <= 0 || freq > srcFreq || (divFac > UINT8_MAX && gclkNum != 1)) 
        return false;
    }
    GCLK->GENCTRL[gclkNum].reg = 
        ((uint8_t)gclk_config.runInStandby << GCLK_GENCTRL_RUNSTDBY_Pos)
      | ((uint8_t)gclk_config.improveDutyCycle << GCLK_GENCTRL_IDC_Pos)
      | ((uint8_t)gclk_config.enableOutput << GCLK_GENCTRL_OE_Pos)
      | (GCLK_GENCTRL_SRC(source))
      | ((divFac > 0 ? 1 : 0) << GCLK_GENCTRL_DIVSEL_Pos)
      | (GCLK_GENCTRL_DIV(divFac));
    while(GCLK->SYNCBUSY.reg & (GCLK_SYNCBUSY_GENCTRL0_Pos + gclkNum));

  } else {
    GCLK->GENCTRL[gclkNum].bit.GENEN = 0;
      while(GCLK->SYNCBUSY.reg & (GCLK_SYNCBUSY_GENCTRL0_Pos + gclkNum));
  }
  return true;
};


bool link_gclk(int channelNum, bool linkEnabled, int gclkNum) {
  if (channelNum < 0 || channelNum > GCLK_NUM - 1)
    return false;

  if (linkEnabled) {
    if (gclkNum < 0 || gclkNum > GCLK_GEN_NUM) 
      return false;

    GCLK->PCHCTRL[channelNum].reg = 
        (GCLK_PCHCTRL_CHEN) 
      | ((uint8_t)gclkNum << GCLK_PCHCTRL_GEN_Pos);
  
  } else {
    GCLK->PCHCTRL[channelNum].bit.CHEN = 0;
  } 
}


int get_gclk_freq(unsigned int gclkNum) {
  if (gclkNum >= GCLK_NUM) return -1;

  switch(GCLK->GENCTRL[gclkNum].bit.SRC) {
    case 4: {
      return get_osculp_freq();
    }
    case 5: {
      return get_xosc32k_freq();
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> OSCULP FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_osculp(bool enabled) {
  if (enabled) {
    OSC32KCTRL->OSCULP32K.reg |=
        OSC32KCTRL_OSCULP32K_EN1K
      | OSC32KCTRL_OSCULP32K_EN32K;
  } else {
    OSC32KCTRL->OSCULP32K.reg = 0;
  }
  return true;
}

int get_osculp_freq() {
  if (OSC32KCTRL->OSCULP32K.bit.EN1K) 
    return OSCULP_EN1K_FREQ;
  if (OSC32KCTRL->OSCULP32K.bit.EN32K) 
    return OSCULP_EN32K_FREQ;
  return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> XOSC32K FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_xosc32k(bool enabled, XOSC32K_FREQ freq = XOSC_FREQ_NULL, int startupTime = 0, 
  bool waitForLock = false) {

  if (enabled) { 
    uint8_t suSel = 0; 
    int suPrev = 0;
    for (int i = 0; i < sizeof(SUTIME_REF) / sizeof(SUTIME_REF); i++) { 
      if (abs(SUTIME_REF[i] - startupTime) < suPrev) {
        suPrev = abs(SUTIME_REF[i] - startupTime);
        suSel = i;
      }
    }
    OSC32KCTRL->XOSC32K.reg =
        ((uint8_t)xosc32k_config.highSpeedMode << OSC32KCTRL_XOSC32K_CGM_Pos)
      | ((uint8_t)xosc32k_config.onDemand << OSC32KCTRL_XOSC32K_ONDEMAND_Pos)
      | ((uint8_t)xosc32k_config.runInStandby << OSC32KCTRL_XOSC32K_RUNSTDBY_Pos)
      | ((uint8_t)xosc32k_config.outputSignal << OSC32KCTRL_XOSC32K_XTALEN_Pos)
      | ((uint8_t)(freq == XOSC32K_FREQ_1KHZ) << OSC32KCTRL_XOSC32K_EN1K_Pos)
      | ((uint8_t)(freq == XOSC32K_FREQ_32KHZ) << OSC32KCTRL_XOSC32K_EN32K_Pos)
      | (OSC32KCTRL_XOSC32K_STARTUP(suSel))
      | (OSC32KCTRL_XOSC32K_ENABLE);

    OSC32KCTRL->CFDCTRL.reg |= 
        ((uint8_t)xosc32k_config.enableCFD << OSC32KCTRL_CFDCTRL_CFDEN_Pos)
      | OSC32KCTRL_CFDCTRL_SWBACK;
    while(waitForLock && !OSC32KCTRL->STATUS.bit.XOSC32KRDY); 

  } else {
    OSC32KCTRL->XOSC32K.bit.ENABLE = 0;
      while(OSC32KCTRL->STATUS.bit.XOSC32KRDY);
  }
  return true;
}


int get_xosc32k_freq() {
  if (OSC32KCTRL->XOSC32K.bit.ENABLE) {
    if (OSC32KCTRL->XOSC32K.bit.EN1K) return XOSC32K_EN1K_FREQ;
    if (OSC32KCTRL->XOSC32K.bit.EN32K) return XOSC32K_EN32K_FREQ;
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DFLL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_dfll(bool enabled, int freq, bool closedLoopMode, bool waitForLock) {                                                      
  if (enabled && (freq < DFLL_FREQ_MIN || freq > DFLL_FREQ_MAX)) return false;  

  if (enabled) {
    OSCCTRL->DFLLMUL.bit.MUL = (uint16_t)(freq / DFLL_BASE_FREQ);
      while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

    if (!closedLoopMode) {
      OSCCTRL->DFLLVAL.reg = 
          OSCCTRL_DFLLVAL_COARSE(dfll_config.coarseAdj)
        | OSCCTRL_DFLLVAL_FINE(dfll_config.fineAdj);
        while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);
    }
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 1;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE);
    OSCCTRL->DFLLVAL.reg = OSCCTRL->DFLLVAL.reg; // Due to chip errata 
      while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

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

    if (waitForLock) {
      while(closedLoopMode && (!OSCCTRL->STATUS.bit.DFLLLCKF || !OSCCTRL->STATUS.bit.DFLLLCKC));
      while(!OSCCTRL->STATUS.bit.DFLLRDY);      
    }
  } else {
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 0;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE && OSCCTRL->STATUS.bit.DFLLRDY);
  }
  return true;
}


int get_dfll_freq() {
  if (OSCCTRL->DFLLCTRLA.bit.ENABLE) {
    return DFLL_BASE_FREQ * OSCCTRL->DFLLMUL.bit.MUL;
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DFLL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_dpll(int dpllNum, bool enabled, DPLL_SRC source, int freq, bool waitForLock) {

  if (dpllNum < 0 || dpllNum > OSCCTRL_DPLLS_NUM) 
    return false;
  if (enabled && (source == DPLL_SRC_NULL || freq < DPLL_FREQ_MIN || freq > DPLL_FREQ_MAX))
    return false;

  #define dpll OSCCTRL->Dpll[dpllNum]
  #define config dpll_config[dpllNum]
  
  if (enabled) {
    int srcFreq = 0;

    if (source == DPLL_SRC_GCLK0) {
      if (!GCLK->GENCTRL[OSCCTRL_GCLK_ID_FDPLL0 + dpllNum].bit.GENEN) 
        return false;
      int gclkSrc = GCLK->GENCTRL[OSCCTRL_GCLK_ID_FDPLL0].bit.SRC;
      srcFreq = get_gclk_freq(OSCCTRL_GCLK_ID_FDPLL0 + dpllNum);
    } else {
      srcFreq = get_xosc32k_freq();
    }
    uint16_t ldr = (freq / srcFreq) -1;
    uint16_t frac = (double)(freq - (srcFreq * (ldr + 1))) 
      * (DPLL_LDRFRAC_DEN / (double)srcFreq);
    int accFreq = srcFreq * ((double)ldr + 1.0 + (double)frac / DPLL_LDRFRAC_DEN);
    
    if (abs(accFreq - freq) > dpll_config[dpllNum].maxFreqOffset 
      || accFreq < DPLL_FREQ_MIN || accFreq > DPLL_FREQ_MAX) 
        return false;

    dpll.DPLLRATIO.reg =
        OSCCTRL_DPLLRATIO_LDR((uint8_t)ldr)
      | OSCCTRL_DPLLRATIO_LDRFRAC(frac);
    while(dpll.DPLLSYNCBUSY.bit.DPLLRATIO);

    dpll.DPLLCTRLB.reg |= 
        OSCCTRL_DPLLCTRLB_REFCLK((uint8_t)source)
      | OSCCTRL_DPLLCTRLB_LBYPASS;             

    dpll.DPLLCTRLA.bit.ENABLE = 1;
      while(dpll.DPLLSYNCBUSY.bit.ENABLE);

    dpll.DPLLCTRLA.reg = 
        ((uint8_t)dpll_config[dpllNum].onDemand << OSCCTRL_DPLLCTRLA_ONDEMAND_Pos)
      | ((uint8_t)dpll_config[dpllNum].runInStandby << OSCCTRL_DPLLCTRLA_RUNSTDBY_Pos)
      | ((uint8_t)dpll.DPLLCTRLA.bit.ENABLE << OSCCTRL_DPLLCTRLA_ENABLE_Pos);

    dpll.DPLLCTRLB.reg = 
        ((uint8_t)dpll_config[dpllNum].dcoFilterEnabled << OSCCTRL_DPLLCTRLB_DCOEN_Pos)
      | OSCCTRL_DPLLCTRLB_DCOFILTER(dpll_config[dpllNum].dcoFilterSel)
      | (((uint8_t)dpll_config[dpllNum].lockTimeout * DPLL_DEFAULT_TIMEOUT_SEL) 
          << OSCCTRL_DPLLCTRLB_LTIME_Pos)
      | (dpll.DPLLCTRLB.bit.REFCLK << OSCCTRL_DPLLCTRLB_REFCLK_Pos)
      | ((uint8_t)dpll_config[dpllNum].wakeUpFast << OSCCTRL_DPLLCTRLB_WUF_Pos)
      | OSCCTRL_DPLLCTRLB_FILTER((uint8_t)dpll_config[dpllNum].integralFilterSel);

    while(waitForLock && !(dpll.DPLLSTATUS.reg & OSCCTRL_DPLLSTATUS_MASK)) {

      if (OSCCTRL->INTFLAG.reg & OSCCTRL_STATUS_DPLL0TO << (8 * dpllNum)) {
        OSCCTRL->INTFLAG.reg &= ~(OSCCTRL_STATUS_DPLL0TO << (8 * dpllNum));
        abort: {
          dpll.DPLLCTRLA.bit.ENABLE = 0;
          return false;
        }
      } else if (OSCCTRL->INTFLAG.reg & OSCCTRL_STATUS_DPLL0LCKF << (8 * dpllNum)) {
        OSCCTRL->INTFLAG.reg &= ~(OSCCTRL_STATUS_DPLL0LCKF << (8 * dpllNum));
        goto abort;
      }
    }
  } else if (!enabled) {
    dpll.DPLLCTRLA.bit.ENABLE = 0;
      while(dpll.DPLLSYNCBUSY.bit.ENABLE);
  }
  return true;
}


int get_dpll_freq(unsigned int dpllNum) {  
  if (dpllNum < 0 || dpllNum > OSCCTRL_DPLLS_NUM)
    return -1;   
                                                        
  if (OSCCTRL->Dpll[dpllNum].DPLLCTRLA.bit.ENABLE) {
    double mul = (double)OSCCTRL->Dpll[dpllNum].DPLLRATIO.bit.LDR + 1.0 
      + ((double)OSCCTRL->Dpll[dpllNum].DPLLRATIO.bit.LDRFRAC / DPLL_LDRFRAC_DEN);

    switch(OSCCTRL->Dpll[dpllNum].DPLLCTRLB.bit.REFCLK) {
      case OSCCTRL_DPLLCTRLB_REFCLK_GCLK_Val: {
        return get_gclk_freq(OSCCTRL_GCLK_ID_FDPLL0 + dpllNum) * mul;
      } 
      case OSCCTRL_DPLLCTRLB_REFCLK_XOSC32_Val: {
        return get_xosc32k_freq() * mul;
      }
      case OSCCTRL_DPLLCTRLB_REFCLK_XOSC0_Val: {
        return -1;
      } 
      case OSCCTRL_DPLLCTRLB_REFCLK_XOSC1_Val: {
        return -1;
      }
    }
  }
  return 0;
}







