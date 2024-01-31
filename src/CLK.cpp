
#include "CLK.h"

//// SYS REFERENCES ////
#define SYS_DEFAULT_DIV 1
#define SYS_DEFAULT_HSDIV 1
#define SYS_GCLK 0
#define SYS_MAX_FREQ 120000000
#define SYS_MIN_FREQ 1024

//// OSCULP REFERENCES ////
#define OSCULP_EN1K_FREQ 1024;
#define OSCULP_EN32K_FREQ 32746;

//// XOSC32K REFERENCES ////
#define XOSC32K_EN1K_FREQ 1024;
#define XOSC32K_EN32K_FREQ 32746
const uint8_t SUTIME_REF[] = {62, 125, 500, 10000, 20000, 40000, 80000};

//// DFLL REFERENCES ////
#define DFLL_BASE_FREQ 48000000
#define DFLL_FREQ_MIN 12000000
#define DFLL_FREQ_MAX 240000000
#define DFLL_GCLK_PERIPH_CHANNEL 0
#define DFLL_PRIMARY_REF GCLK_SOURCE_OSCULP32K
#define DFLL_PRIMARY_FREQ get_osculp_freq()
#define DFLL_BACKUP_REF GCLK_SOURCE_XOSC32K
#define DFLL_BACKUP_FREQ get_xosc32k_freq()

//// DPLL REFERENCES ////
#define DPLL_LDRFRAC_DEN 32.0
#define DPLL_FREQ_MAX 1000000000 // TO DO
#define DPLL_FREQ_MIN 8000
#define DPLL_DEFAULT_TIMEOUT_SEL OSCCTRL_DPLLCTRLB_LTIME_1MS_Val

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYS FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_cpu_freq(int freq, bool highSpeedDomain) {
  uint8_t div = 0;
  if ((freq < SYS_MIN_FREQ && freq > 0) || freq > SYS_MAX_FREQ 
      || freq > get_gclk_freq(SYS_GCLK))
    return false;

  auto calcDiv = [&](const uint8_t otherDiv) -> bool {
    div = get_gclk_freq(SYS_GCLK) / freq;
    if (div <= 0) 
      return false;
    div = log2(div) * 10;
    return (highSpeedDomain ? div < otherDiv : div > otherDiv);
  };

  //NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_RWS(0);   ----------------> LOOK INTO THIS...
  if (highSpeedDomain) {
    if (freq <= 0) {
      MCLK->HSDIV.reg = MCLK_HSDIV_DIV(SYS_DEFAULT_HSDIV);
    } else {
      if (!calcDiv(MCLK->CPUDIV.bit.DIV))
        return false;
      MCLK->HSDIV.reg = MCLK_HSDIV_DIV(div);
    }
  } else {
    if (freq <= 0) {
      MCLK->CPUDIV.reg = MCLK_CPUDIV_DIV(SYS_DEFAULT_DIV);
    } else {
      if (!calcDiv(MCLK->HSDIV.bit.DIV))
        return false;
      MCLK->CPUDIV.reg = MCLK_CPUDIV_DIV(div);
    }
  }
  return true;
}

int get_cpu_freq(bool highSpeedDomain) {
  return get_gclk_freq(SYS_GCLK) 
    / (highSpeedDomain ? MCLK->HSDIV.bit.DIV : MCLK->CPUDIV.bit.DIV);
}

bool set_rtc_src(RTC_SOURCE source) {
  OSC32KCTRL->RTCCTRL.bit.RTCSEL = (uint8_t)source;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> STATIC FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

static bool validGclk(int gclkNum) {
  return gclkNum >= 0 && gclkNum < GCLK_GEN_NUM; 
}

static bool validDpll(int dpllNum) {
  return dpllNum >= 0 && dpllNum < OSCCTRL_DPLLS_NUM;
}

static int getGclkSrcFreq(uint8_t source) {
  switch(source) {
    case GCLK_GENCTRL_SRC_GCLKGEN1_Val: {
      return get_gclk_freq(1);
    }
    case GCLK_SOURCE_OSCULP32K: {
      return get_osculp_freq();
    }
    case GCLK_SOURCE_XOSC32K: {
      return get_xosc32k_freq();
    }
    case GCLK_SOURCE_DFLL: {
      return get_dfll_freq();
    }
    case GCLK_SOURCE_DPLL0: {
      return  get_dpll_freq(0);
    }
    case GCLK_SOURCE_DPLL1: {
      return get_dpll_freq(1);
    }
    default: {
      return -2;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> GCLK FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_gclk(int gclkNum, bool enabled, GCLK_SOURCE source, int freq) {
  if (!validGclk(gclkNum)) 
    return false;
  
  if (enabled) {
    if (freq <= 0 || source == GCLK_NULL) 
      return false;

    int divFacTemp = getGclkSrcFreq((uint8_t)source) / freq;
    if (divFacTemp <= 0 || (divFacTemp > UINT8_MAX && gclkNum != 1) 
        || (divFacTemp > UINT16_MAX && gclkNum == 1))
      return false;
    uint16_t divFac = static_cast<uint16_t>(divFacTemp);

    GCLK->GENCTRL[gclkNum].reg = 
        ((uint8_t)gclk_config[gclkNum].runInStandby << GCLK_GENCTRL_RUNSTDBY_Pos)
      | ((uint8_t)gclk_config[gclkNum].improveDutyCycle << GCLK_GENCTRL_IDC_Pos)
      | ((uint8_t)gclk_config[gclkNum].enableOutput << GCLK_GENCTRL_OE_Pos)
      | ((divFac > 0 ? 1 : 0) << GCLK_GENCTRL_DIVSEL_Pos)
      | GCLK_GENCTRL_SRC((uint8_t)source)
      | GCLK_GENCTRL_DIV(divFac)
      | GCLK_GENCTRL_GENEN;
    while(GCLK->SYNCBUSY.reg & (GCLK_SYNCBUSY_GENCTRL0_Pos + gclkNum));

  } else {
    GCLK->GENCTRL[gclkNum].bit.GENEN = 0;
      while(GCLK->SYNCBUSY.reg & (GCLK_SYNCBUSY_GENCTRL0_Pos + gclkNum));
  }
  return true;
}

bool set_gclk_channel(int channelNum, bool channelEnabled, int gclkNum) {
  if (channelNum < 0 || channelNum > GCLK_NUM - 1)
    return false;

  if (channelEnabled) {
    if (!validGclk(gclkNum)) 
      return false;

    GCLK->PCHCTRL[channelNum].reg = 
        (GCLK_PCHCTRL_CHEN) 
      | ((uint8_t)gclkNum << GCLK_PCHCTRL_GEN_Pos);
  
  } else {
    GCLK->PCHCTRL[channelNum].bit.CHEN = 0;
  } 
  return true;
}

int get_gclk_freq(int gclkNum) {
  if (!validGclk(gclkNum)) 
    return -1;

  if (GCLK->GENCTRL[gclkNum].bit.GENEN) {
    int srcFreq = getGclkSrcFreq(GCLK->GENCTRL[gclkNum].bit.SRC);
    
    if (srcFreq > 0) {
      return srcFreq / GCLK->GENCTRL[gclkNum].bit.DIVSEL 
        ? GCLK->GENCTRL[gclkNum].bit.DIV : 1;

    } else if (!srcFreq) {
      return 0;
    }
  }
  return -1;
}

int get_gclk_channels(int gclkNum, int *resultArray, int arrayLength) {
  if (!validGclk(gclkNum)) 
    return -1;
  int index = 0;
    
  for (int i = 0; i < GCLK_NUM; i++) {
    if (GCLK->PCHCTRL[i].bit.CHEN && GCLK->PCHCTRL[i].bit.GEN == gclkNum) {
      if (arrayLength > 0 && resultArray != nullptr) {
        if (index == arrayLength)
          return index;
        resultArray[index] = i;
      }
      index++;
    }
  }
  return index;
}

int get_channel_gclk(int channelNum) {
  if (channelNum >= GCLK_NUM || channelNum < 0)
    return false;
  return GCLK->PCHCTRL[channelNum].bit.CHEN ? GCLK->PCHCTRL[channelNum].bit.GEN : -1;
}

OSC_STATUS get_gclk_status(int gclkNum) {
  if (!validGclk(gclkNum))
    return OSC_STATUS_NULL;
    
  return GCLK->GENCTRL[gclkNum].bit.GENEN 
    ? OSC_STATUS_ENABLED_LOCKED : OSC_STATUS_DISABLED;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> OSCULP FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_osculp(bool enabled, OSCULP_FREQ freqSel) {

  if (enabled && freqSel == OSCULP_FREQ_NULL)
    return false;

  if (enabled) {
    OSC32KCTRL->OSCULP32K.reg |=
        ((uint8_t)(freqSel == OSCULP_FREQ_1KHZ) << OSC32KCTRL_OSCULP32K_EN1K_Pos)
      | ((uint8_t)(freqSel == OSCULP_FREQ_32KHZ) << OSC32KCTRL_OSCULP32K_EN32K_Pos);
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

OSC_STATUS get_osculp_status() {
  return get_osculp_freq != 0 ? OSC_STATUS_ENABLED_LOCKED : OSC_STATUS_DISABLED;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> XOSC32K FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_xosc32k(bool enabled, XOSC32K_FREQ freqSel, int startupTime, 
  bool waitForLock) {

  if (enabled && freqSel == XOSC32K_FREQ_NULL) 
    return false;

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
      | ((uint8_t)(freqSel == XOSC32K_FREQ_1KHZ) << OSC32KCTRL_XOSC32K_EN1K_Pos)
      | ((uint8_t)(freqSel == XOSC32K_FREQ_32KHZ) << OSC32KCTRL_XOSC32K_EN32K_Pos)
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

OSC_STATUS get_xosc32k_status() {
  if (OSC32KCTRL->STATUS.bit.XOSC32KSW || OSC32KCTRL->STATUS.bit.XOSC32KFAIL) {
    return OSC_STATUS_ERROR;
  }
  if (OSC32KCTRL->XOSC32K.bit.ENABLE) {
    if (OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
      return OSC_STATUS_ENABLED_LOCKED;
    }
    return OSC_STATUS_ENABLED_LOCKING;
  }
  return OSC_STATUS_DISABLED;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DFLL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_dfll(bool enabled, int freq, bool closedLoopMode, int gclkNum, bool waitForLock) {                                              
  if (enabled && (freq < DFLL_FREQ_MIN || freq > DFLL_FREQ_MAX)) 
    return false;  

  if (enabled) {

    if (closedLoopMode) {

      if (validGclk(gclkNum)) {
        
        if (get_gclk_freq(gclkNum) <= 0) {
          uint8_t refSel = DFLL_PRIMARY_REF;

          if (DFLL_PRIMARY_FREQ <= 0) {
            if (DFLL_BACKUP_FREQ <= 0) 
              return false;
            refSel = DFLL_BACKUP_REF;
          }
          GCLK->GENCTRL[gclkNum].reg = 
              (GCLK_GENCTRL_GENEN)
            | (0 << GCLK_GENCTRL_DIVSEL_Pos)
            | (GCLK_GENCTRL_SRC(refSel));
          while(GCLK->SYNCBUSY.reg && (1 << (GCLK_SYNCBUSY_GENCTRL0_Pos + refSel)));
        } 
        if (!set_gclk_channel(DFLL_GCLK_PERIPH_CHANNEL, true, gclkNum))
          return false;

      } else if (get_gclk_freq(GCLK->PCHCTRL[DFLL_GCLK_PERIPH_CHANNEL].bit.GEN) <= 0) {
        return false;
      }
      GCLK->PCHCTRL[DFLL_GCLK_PERIPH_CHANNEL].bit.CHEN = 1;
    }
    OSCCTRL->DFLLMUL.bit.MUL = (uint16_t)(freq / DFLL_BASE_FREQ);
      while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

    OSCCTRL->DFLLVAL.reg = 
      OSCCTRL_DFLLVAL_COARSE(dfll_config.coarseAdj)
    | OSCCTRL_DFLLVAL_FINE(dfll_config.fineAdj);
    while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

    OSCCTRL->DFLLCTRLA.bit.ENABLE = 1;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE);
    OSCCTRL->DFLLVAL.reg = OSCCTRL->DFLLVAL.reg; // Due to chip errata 
      while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

    OSCCTRL->DFLLCTRLA.reg = 
        ((uint8_t)dfll_config.onDemand << OSCCTRL_DFLLCTRLA_ONDEMAND_Pos)
      | ((uint8_t)dfll_config.runInStandby << OSCCTRL_DFLLCTRLA_RUNSTDBY_Pos)
      | (OSCCTRL->DFLLCTRLA.bit.ENABLE << OSCCTRL_DFLLCTRLA_ENABLE_Pos);

    OSCCTRL->DFLLMUL.reg = 
        OSCCTRL_DFLLMUL_FSTEP(dfll_config.maxFineAdj)
      | OSCCTRL_DFLLMUL_CSTEP(dfll_config.maxCoarseAdj)
      | OSCCTRL_DFLLMUL_MUL(OSCCTRL->DFLLMUL.bit.MUL);
    while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

    OSCCTRL->DFLLCTRLB.reg = 
        ((uint8_t)dfll_config.waitForLock << OSCCTRL_DFLLCTRLB_WAITLOCK_Pos)
      | ((uint8_t)dfll_config.bypassCoarseLock << OSCCTRL_DFLLCTRLB_BPLCKC_Pos)
      | (!(uint8_t)dfll_config.quickLock << OSCCTRL_DFLLCTRLB_QLDIS_Pos)
      | (!(uint8_t)dfll_config.chillCycle << OSCCTRL_DFLLCTRLB_CCDIS_Pos)
      | ((uint8_t)dfll_config.usbRecoveryMode << OSCCTRL_DFLLCTRLB_USBCRM_Pos)
      | (!(uint8_t)dfll_config.stabalizeFreq << OSCCTRL_DFLLCTRLB_STABLE_Pos)
      | (OSCCTRL->DFLLCTRLB.bit.MODE << OSCCTRL_DFLLCTRLB_MODE_Pos);
    while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

    while(waitForLock && !OSCCTRL->STATUS.bit.DFLLRDY);      
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

float get_dfll_drift() {
  if (OSCCTRL->DFLLCTRLA.bit.ENABLE && OSCCTRL->DFLLCTRLB.bit.MODE) {
    return (float)OSCCTRL->DFLLVAL.bit.DIFF / (float)OSCCTRL->DFLLMUL.bit.MUL;
  } else {
    return -1.0;
  }
}

OSC_STATUS get_dfll_status() {
  if (OSCCTRL->STATUS.bit.DFLLOOB || OSCCTRL->STATUS.bit.DFLLRCS) {
    return OSC_STATUS_ERROR;
  }
  if (OSCCTRL->DFLLCTRLA.bit.ENABLE) {
    if (OSCCTRL->STATUS.bit.DFLLRDY) {
      return OSC_STATUS_ENABLED_LOCKED;
    }
    return OSC_STATUS_ENABLED_LOCKING;
  }
  return OSC_STATUS_DISABLED;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DFLL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool set_dpll(int dpllNum, bool enabled, DPLL_SRC source, int freq, bool waitForLock) {  
  if (!validDpll(dpllNum)) 
    return false;

  #define dpll OSCCTRL->Dpll[dpllNum]
  #define config dpll_config[dpllNum]
  
  if (enabled) {
    int srcFreq = 0;
    if (source == DPLL_SRC_NULL || freq < DPLL_FREQ_MIN || freq > DPLL_FREQ_MAX) 
      return false;

    if (source == DPLL_SRC_GCLK0) {
      if (!GCLK->GENCTRL[OSCCTRL_GCLK_ID_FDPLL0 + dpllNum].bit.GENEN) 
        return false;

      int gclkSrc = GCLK->GENCTRL[OSCCTRL_GCLK_ID_FDPLL0].bit.SRC;
      srcFreq = get_gclk_freq(OSCCTRL_GCLK_ID_FDPLL0 + dpllNum);

    } else {
      srcFreq = get_xosc32k_freq();
    }
    if (srcFreq <= 0) 
      return false;

    uint16_t ldr = (freq / srcFreq) -1;
    uint16_t frac = (double)(freq - (srcFreq * (ldr + 1))) 
      * (DPLL_LDRFRAC_DEN / (double)srcFreq);
    int accFreq = srcFreq * ((double)ldr + 1.0 + (double)frac / DPLL_LDRFRAC_DEN);
    
    if (abs(accFreq - freq) > dpll_config[dpllNum].maxFreqOffset 
      || accFreq < DPLL_FREQ_MIN || accFreq > DPLL_FREQ_MAX) 
        return false;

    dpll.DPLLCTRLA.bit.ENABLE = 0;
      while(dpll.DPLLSYNCBUSY.bit.ENABLE);

    dpll.DPLLRATIO.reg =
        OSCCTRL_DPLLRATIO_LDR((uint8_t)ldr)
      | OSCCTRL_DPLLRATIO_LDRFRAC(frac);
    while(dpll.DPLLSYNCBUSY.bit.DPLLRATIO);

    dpll.DPLLCTRLB.reg = 
        OSCCTRL_DPLLCTRLB_REFCLK((uint8_t)source)
      | OSCCTRL_DPLLCTRLB_LBYPASS;    

    dpll.DPLLCTRLA.reg = 
        ((uint8_t)dpll_config[dpllNum].onDemand << OSCCTRL_DPLLCTRLA_ONDEMAND_Pos)
      | ((uint8_t)dpll_config[dpllNum].runInStandby << OSCCTRL_DPLLCTRLA_RUNSTDBY_Pos)
      | ((uint8_t)dpll.DPLLCTRLA.bit.ENABLE << OSCCTRL_DPLLCTRLA_ENABLE_Pos);

    dpll.DPLLCTRLB.reg = 
        ((uint8_t)dpll_config[dpllNum].dcoFilterEnabled << OSCCTRL_DPLLCTRLB_DCOEN_Pos)
      | OSCCTRL_DPLLCTRLB_DCOFILTER(dpll_config[dpllNum].dcoFilterSel)
      | (dpll.DPLLCTRLB.bit.LBYPASS << OSCCTRL_DPLLCTRLB_LBYPASS_Pos)
      | (((uint8_t)dpll_config[dpllNum].lockTimeout * DPLL_DEFAULT_TIMEOUT_SEL) 
          << OSCCTRL_DPLLCTRLB_LTIME_Pos)
      | (dpll.DPLLCTRLB.bit.REFCLK << OSCCTRL_DPLLCTRLB_REFCLK_Pos)
      | ((uint8_t)dpll_config[dpllNum].wakeUpFast << OSCCTRL_DPLLCTRLB_WUF_Pos)
      | OSCCTRL_DPLLCTRLB_FILTER((uint8_t)dpll_config[dpllNum].integralFilterSel);

    dpll.DPLLCTRLA.bit.ENABLE = 1;
      while(dpll.DPLLSYNCBUSY.bit.ENABLE);

    while(waitForLock && (dpll.DPLLSTATUS.reg & OSCCTRL_DPLLSTATUS_MASK) 
        != OSCCTRL_DPLLSTATUS_MASK) {
      if (OSCCTRL->STATUS.reg & (1 << (OSCCTRL_STATUS_DPLL0TO_Pos + 8 * dpllNum))) {
        dpll.DPLLCTRLA.bit.ENABLE = 0;
        return false;
      }
    }
  } else if (!enabled) {
    dpll.DPLLCTRLA.bit.ENABLE = 0;
      while(dpll.DPLLSYNCBUSY.bit.ENABLE);
  }
  return true;
}

int get_dpll_freq(unsigned int dpllNum) {  
  if (!validDpll(dpllNum))
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

OSC_STATUS get_dpll_status(int dpllNum) {
  if (!validDpll(dpllNum))
    return OSC_STATUS_NULL;

  if (OSCCTRL->STATUS.reg & (1 << (OSCCTRL_STATUS_DPLL0TO_Pos + 8 * dpllNum))) {
    return OSC_STATUS_ERROR;
  } 
  if (OSCCTRL->Dpll[dpllNum].DPLLCTRLA.bit.ENABLE) {
    if ((OSCCTRL->Dpll[dpllNum].DPLLSTATUS.reg & OSCCTRL_DPLLSTATUS_MASK) 
        == OSCCTRL_DPLLSTATUS_MASK) {
      return OSC_STATUS_ENABLED_LOCKED;
    }
    return OSC_STATUS_ENABLED_LOCKING;
  }
  return OSC_STATUS_DISABLED;
}










