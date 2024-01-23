///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <SYS.h>

#define DPLL_FAIL_MASK (OSCCTRL_STATUS_DPLL1TO | OSCCTRL_STATUS_DPLL1LCKF \
                      | OSCCTRL_STATUS_DPLL0TO | OSCCTRL_STATUS_DPLL0LCKF)
#define DFLL_REF_FREQ 32768
#define DPLL_REF_FREQ 32768
#define MAIN_GCLK_NUM 0
#define MAIN_OSC_NUM_DEFAULT GCLK_GENCTRL_SRC_DPLL0_Val
#define OSC32KCTRL_EN1K_FREQ 1000
#define OSC32KCTRL_EN32K_FREQ 32768
#define DFLL_GCLK_REF_CH 0

SystemConfig &defaultStartup;

const IRQn_Type CLK_IRQS[] = {
  OSC32KCTRL_IRQn, 
  OSCCTRL_0_IRQn, 
  OSCCTRL_1_IRQn, 
  OSCCTRL_2_IRQn, 
  OSCCTRL_3_IRQn,
  OSCCTRL_4_IRQn
};

enum CLK_SRC_ID : uint8_t {
  SRC_NULL,
  SRC_OSCULP_OUTPUT1,
  SRC_OSCULP_OUTPUT32,
  SRC_XOSC32K_OUTPUT1,
  SRC_XOSC32K_OUTPUT32,
  SRC_XOSC0,
  SRC_XOSC1,
  SRC_DFLL,
  SRC_DPLL0,
  SRC_DPLL1
};

struct CLK_SRC {
  CLK_SRC_ID id;
  uint32_t freq;
};

const uint8_t PERIPH_REF[ID_PERIPH_COUNT] =  {};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> CLK IRQ
///////////////////////////////////////////////////////////////////////////////////////////////////

void OSCCTRL_0_Handler(void) {
  // TO DO 
}

void OSC32KCTRL_Handler (void) {
  // TO DO 
}

void OSCCTRL_1_Handler ( void ) __attribute__((weak, alias("OSCCTRL_0_Handler")));
void OSCCTRL_2_Handler ( void ) __attribute__((weak, alias("OSCCTRL_0_Handler")));
void OSCCTRL_3_Handler ( void ) __attribute__((weak, alias("OSCCTRL_0_Handler")));
void OSCCTRL_4_Handler ( void ) __attribute__((weak, alias("OSCCTRL_0_Handler")));

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYSTEM CLASS METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

int8_t System_::CLK_::allocGCLK(uint32_t freq, uint16_t maxFreqOffset,
   SYS_PERIPHERAL periph) {
  if (!init) return -1;

  uint32_t nearestDiff = UINT32_MAX;
  int8_t src = -1;
  int8_t gclkNum = -1;

  // Find & allocate clock
  for (int16_t i = 0; i < GCLK_NUM; i++) {
    if (!agclk[i]) {
      agclk[i] = true;
      gclkNum = i;
      break;
    }
  }
  if (gclkNum == -1) return false;
  
  // Find source
  for (int16_t i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
    if (sources[i] != 0 && sources[i] >= freq && sources[i] - freq > nearestDiff) {
      src = i;
      nearestDiff = freq - sources[i];
    }
  }
  if (src == -1) {
    agclk[gclkNum] = false;
    return false;
  }
  // Set GENCTRL reg
  GCLK->GENCTRL[gclkNum].reg = 0;
    while(GCLK->SYNCBUSY.reg & (1 << gclkNum + GCLK_SYNCBUSY_GENCTRL0_Pos));

  GCLK->GENCTRL[gclkNum].bit.SRC = src;
    while(GCLK->SYNCBUSY.reg & (1 << gclkNum + GCLK_SYNCBUSY_GENCTRL0_Pos));

  uint16_t divFactor = sources[src] / freq;
  if (freq - sources[src] > super->config->CLK.GEN.minFreqDiff) {
    GCLK->GENCTRL[gclkNum].reg |=
        GCLK_GENCTRL_DIV(divFactor)
      | GCLK_GENCTRL_DIVSEL;
      while(GCLK->SYNCBUSY.reg & (1 << gclkNum + GCLK_SYNCBUSY_GENCTRL0_Pos));
  }

  uint32_t freqOffset = freq - sources[src] < 0 ? sources[src] - freq : freq - sources[src];
  if (freqOffset <= maxFreqOffset) {

    GCLK->GENCTRL[gclkNum].bit.GENEN = 1;
      while(GCLK->SYNCBUSY.reg & (1 << gclkNum + GCLK_SYNCBUSY_GENCTRL0_Pos))
    return gclkNum;
  } else {
    agclk[gclkNum] = false;
    GCLK->GENCTRL[gclkNum].reg = 0;
      while(GCLK->SYNCBUSY.reg & (1 << gclkNum + GCLK_SYNCBUSY_GENCTRL0_Pos));
    return -1;
  }
  // Attemp to assign to peripheral channel (if applicable)
  if (periph != PERIPH_NONE) {
    uint8_t periphNum = PERIPH_REF[(uint8_t)periph];

    if (periphAlloc[periphNum] == -1) {
      periphAlloc[periphNum] = gclkNum;
      GCLK->PCHCTRL[gclkNum].reg = 0;

      GCLK->PCHCTRL[gclkNum].reg |=
          GCLK_PCHCTRL_GEN(periphNum)
        | GCLK_PCHCTRL_CHEN;
    }
  }
  return gclkNum;
}

bool System_::CLK_::freeGCLK(uint8_t gclkNum) {
  if (!init) return false;

  if (agclk[gclkNum]) {
    agclk[gclkNum] = false;

    // Disable clk peripheral channel
    for (int16_t i = 0; i < sizeof(periphAlloc) / sizeof(periphAlloc[0]); i++) {
      if (periphAlloc[i] == gclkNum) {
        periphAlloc[i] = -1;
        GCLK->PCHCTRL[gclkNum].bit.CHEN = 0;
      }
    } // Disable clk
    GCLK->GENCTRL[gclkNum].reg = 0;
      while(GCLK->SYNCBUSY.reg & (1 << gclkNum + GCLK_SYNCBUSY_GENCTRL0_Pos));
    return true;
  }
  return false;
}


bool System_::CLK_::initialize() {
  if (init) return true;
  init = true; 

  //// SYS SETUP //////////////////////////////////////////////////////////////////////////////////

  sources[SRC_OSCULP_OUTPUT1] = OSC32KCTRL_EN1K_FREQ;
  sources[SRC_OSCULP_OUTPUT32] = OSC32KCTRL_EN32K_FREQ;

  OSC32KCTRL->INTENCLR.reg |= OSC32KCTRL_INTENCLR_MASK;    
  OSC32KCTRL->INTFLAG.reg |= OSC32KCTRL_INTFLAG_MASK;       

  OSCCTRL->INTENCLR.reg |= OSCCTRL_INTENCLR_MASK;
  OSCCTRL->INTFLAG.reg |= OSCCTRL_INTFLAG_MASK;

  //// SET UP THE GCLKS ///////////////////////////////////////////////////////////////////////////

  GCLK->CTRLA.bit.SWRST = 1;
    while(GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_SWRST || GCLK->CTRLA.bit.SWRST);

  // Set GENCTRL reg
  for (int16_t i = 0; i < GCLK_NUM; i++) {
    GCLK->GENCTRL[i].bit.RUNSTDBY = (uint8_t)super->config->CLK.GEN.runInStandby;
      while(GCLK->SYNCBUSY.reg & (1 << i + GCLK_SYNCBUSY_GENCTRL0_Pos));
    GCLK->GENCTRL[i].bit.IDC = (uint8_t)super->config->CLK.GEN.improveDutyCycle;
      while(GCLK->SYNCBUSY.reg & (1 << i + GCLK_SYNCBUSY_GENCTRL0_Pos));
  }

  //// SETUP THE 32K CRYSTALS /////////////////////////////////////////////////////////////////////

  // Set OSCULP writelock -> core clock
  OSC32KCTRL->OSCULP32K.reg |= OSC32KCTRL_OSCULP32K_EN1K | OSC32KCTRL_OSCULP32K_EN32K;
  OSC32KCTRL->OSCULP32K.bit.WRTLOCK = 1;

  // Set XOSC32K reg
  if (super->config->CLK.XOSC32K.enabled) {

  if (super->config->CLK.XOSC32K.enabled) {
    sources[SRC_XOSC32K_OUTPUT1] = OSC32KCTRL_EN1K_FREQ;
    sources[SRC_XOSC32K_OUTPUT32] = OSC32KCTRL_EN32K_FREQ;
  }

    OSC32KCTRL->XOSC32K.reg = 0;    
    OSC32KCTRL->XOSC32K.bit.RUNSTDBY = (uint8_t)(super->config->CLK.XOSC32K.runInStandby);
    OSC32KCTRL->XOSC32K.bit.XTALEN = (uint8_t)(super->config->CLK.XOSC32K.exportSignal);
    
    OSC32KCTRL->XOSC32K.reg |=
        OSC32KCTRL_XOSC32K_CGM(OSC32KCTRL_XOSC32K_CGM_XT_Val)                    
      | OSC32KCTRL_XOSC32K_STARTUP(super->config->CLK.XOSC32K.startupTimeConfig)  
      | OSC32KCTRL_XOSC32K_EN1K                                                   
      | OSC32KCTRL_XOSC32K_EN32K
      | OSC32KCTRL_XOSC32K_ENABLE; 
      while(!OSC32KCTRL->STATUS.bit.XOSC32KRDY);
  } else {
    OSC32KCTRL->XOSC32K.bit.ENABLE = 0;
  }

  // set CFDCTRL reg (failiure detection)
  if (super->config->CLK.MISC.failiureDetection) {
    OSC32KCTRL->CFDCTRL.reg = 0;
    OSC32KCTRL->CFDCTRL.reg |= 
        OSC32KCTRL_CFDCTRL_SWBACK    
      | OSC32KCTRL_CFDCTRL_CFDEN;    
  }

  //// SETUP THE DFLL /////////////////////////////////////////////////////////////////////////////

  // Enable GCLK reference channel
  GCLK->GENCTRL[DFLL_GCLK_REF_CH].reg |=
      GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_OSCULP32K_Val)
    | GCLK_GENCTRL_GENEN;
    while(GCLK->SYNCBUSY.reg & (1 << DFLL_GCLK_REF_CH + GCLK_SYNCBUSY_GENCTRL0_Pos));

  if (super->config->CLK.DFLL.enabled) {

    // Calculate multiplier
    uint16_t multiplier;
    if (super->config->CLK.DFLL.freq <= DFLL_REF_FREQ) {
      multiplier = 1;
    } else {
      multiplier = super->config->CLK.DFLL.freq / DFLL_REF_FREQ;
    }    

    if (super->config->CLK.DFLL.enabled) {
      sources[SRC_DFLL] = DFLL_REF_FREQ * multiplier; 
    }

    // Clear registers
    OSCCTRL->DFLLCTRLA.reg = 0;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE && OSCCTRL->DFLLCTRLA.bit.ENABLE); 
    OSCCTRL->DFLLCTRLB.reg = 0;
      while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
    OSCCTRL->DFLLMUL.reg = 0;                                  
      while(OSCCTRL->DFLLSYNC.bit.DFLLMUL); 
    OSCCTRL->DFLLVAL.reg = 0;
      while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

    // Set DFLLVAL reg
    if (!super->config->CLK.DFLL.closedLoopMode) {
      if (super->config->CLK.DFLL.coarseAdjStep) {
        OSCCTRL->DFLLVAL.bit.COARSE = super->config->CLK.DFLL.coarseAdjStep;
        while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);
      }
      if (super->config->CLK.DFLL.fineAdjStep) {
        OSCCTRL->DFLLVAL.bit.FINE = super->config->CLK.DFLL.fineAdjStep;
        while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);
      }
    }
    // Set CTRLA reg -> enable 
    OSCCTRL->DFLLCTRLA.bit.ONDEMAND = (uint8_t)super->config->CLK.DFLL.onDemand;
    OSCCTRL->DFLLCTRLA.bit.RUNSTDBY = (uint8_t)super->config->CLK.DFLL.runInStandby;
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 1;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE);

    OSCCTRL->DFLLVAL.reg = OSCCTRL->DFLLVAL.reg; 
      while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

    // Set DFLLMUL reg
    OSCCTRL->DFLLMUL.bit.MUL = multiplier;
      while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

    if (!super->config->CLK.DFLL.closedLoopMode) {
      OSCCTRL->DFLLMUL.reg |=
          OSCCTRL_DFLLMUL_CSTEP(super->config->CLK.DFLL.maxCoarseAdjStep)
        | OSCCTRL_DFLLMUL_FSTEP(super->config->CLK.DFLL.maxFineAdjStep);
    }
    // Set CTRLB reg
    OSCCTRL->DFLLCTRLB.bit.MODE = (uint8_t)(super->config->CLK.DFLL.closedLoopMode);
      while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
    OSCCTRL->DFLLCTRLB.bit.CCDIS = (uint8_t)(!super->config->CLK.DFLL.chillCycle);
      while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
    OSCCTRL->DFLLCTRLB.bit.STABLE = (uint8_t)(!super->config->CLK.DFLL.stabalizeFreq);
      while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
    OSCCTRL->DFLLCTRLB.bit.QLDIS = (uint8_t)(!super->config->CLK.DFLL.quickLockEnabled);
      while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

    OSCCTRL->DFLLCTRLB.bit.USBCRM = 1;
      while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

    if (!super->config->CLK.DFLL.closedLoopMode) {
      OSCCTRL->DFLLCTRLB.bit.WAITLOCK = 0;
        while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
    }
    while(!OSCCTRL->STATUS.bit.DFLLRDY);

    // Add dfll to sources
    for (int16_t i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
      if (!sources[i]) {
        sources[i] == super->config->CLK.DFLL.freq;
      }
    }

  } else {
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 0;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE);
  }
  //// SET UP THE DPLL's //////////////////////////////////////////////////////////////////////////

  for (int16_t i = 0; i < OSCCTRL_DPLLS_NUM; i++) {

    if (super->config->CLK.DPLL.enabled[i]) {

      uint16_t ldr;
      uint8_t ldrFrac;
      if (super->config->CLK.DPLL.freq[i] <= DPLL_REF_FREQ) {
        ldr = 0;
        ldrFrac = 0;
      } else {
        ldr = (super->config->CLK.DPLL.freq[i] / DPLL_REF_FREQ) - 1;
        ldrFrac = (super->config->CLK.DPLL.freq[i] - ldr) * 32; 
      }

      CLK_SRC_ID sourceID = i == 0 ? SRC_DPLL0 : SRC_DPLL1;
      sources[sourceID] = DPLL_REF_FREQ * (ldr + 1 + ldrFrac / 32);

      // Set CTRLA reg
      OSCCTRL->Dpll[i].DPLLCTRLA.reg = 0;
        while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.ENABLE);
      OSCCTRL->Dpll[i].DPLLCTRLA.bit.ONDEMAND = (uint8_t)super->config->CLK.DPLL.onDemand;
      OSCCTRL->Dpll[i].DPLLCTRLA.bit.RUNSTDBY = (uint8_t)super->config->CLK.DPLL.runInStandby;
        
      // Set CTRLB reg 
      OSCCTRL->Dpll[i].DPLLCTRLB.reg = 0;
      OSCCTRL->Dpll[i].DPLLCTRLB.reg |= 
          OSCCTRL_DPLLCTRLB_DIV(super->config->CLK.DPLL.xoscDivFactor)
        | OSCCTRL_DPLLCTRLB_REFCLK(OSCCTRL_DPLLCTRLB_REFCLK_XOSC32_Val)
        | OSCCTRL_DPLLCTRLB_DCOFILTER(super->config->CLK.DPLL.dcoFilterSelection)
        | OSCCTRL_DPLLCTRLB_FILTER(super->config->CLK.DPLL.integralFilterSelection)
        | OSCCTRL_DPLLCTRLB_LBYPASS;
      OSCCTRL->Dpll[i].DPLLCTRLB.bit.DCOEN = (uint8_t)((bool)super->config->CLK.DPLL.dcoFilterSelection);
      OSCCTRL->Dpll[i].DPLLCTRLB.bit.LTIME = (uint8_t)(super->config->CLK.DPLL.lockTimeoutSelection);

      // Set the DPLLRATIO register
      OSCCTRL->Dpll[i].DPLLRATIO.reg = 0;
        while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.DPLLRATIO);

      OSCCTRL->Dpll[i].DPLLRATIO.reg |=
          OSCCTRL_DPLLRATIO_LDRFRAC(ldrFrac)
        | OSCCTRL_DPLLRATIO_LDR(ldr);
        while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.DPLLRATIO);

      // Enable the osc -> check for timeout or failiure
      OSCCTRL->Dpll[i].DPLLCTRLA.bit.ENABLE = 1;
        while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.ENABLE);

      while(!OSCCTRL->Dpll[i].DPLLSTATUS.reg & OSCCTRL_DPLLSTATUS_MASK 
        != OSCCTRL_DPLLSTATUS_MASK) {
        
        if (OSCCTRL->STATUS.reg & DPLL_FAIL_MASK >= 1) {
          OSCCTRL->Dpll[i].DPLLCTRLA.bit.ENABLE = 0;
            while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.ENABLE);
          return false;
        } 
      }
      // Add dpll sources to source array
      for (int16_t j = 0; j < sizeof(sources) / sizeof(sources[0]); j++) {
        if (!sources[j]) {
          sources[j] = super->config->CLK.DPLL.freq[i];
        }
      }
    } else {
      OSCCTRL->Dpll[i].DPLLCTRLA.bit.ENABLE = 0;
        while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.ENABLE);
    }
  } 

  // TO DO -> ADD SUPORT FOR REGULAR XOSC CRYSTALS

  for (int16_t i = 0; i < OSCCTRL_XOSCS_NUM; i++) {

    if (super->config->CLK.XOSC.enabled[i]) {
      uint8_t currentRef = 0;
      uint8_t currentMulti = 0;

      // Find current multiplier & reference
      if (super->config->CLK.XOSC.freq[i] > 1000000 
      && super->config->CLK.XOSC.freq[i] < 2000000) {
        currentMulti = 1;
        currentRef = 1;
      } else if (super->config->CLK.XOSC.freq[i] < 4000000) {
        currentMulti = 2;
        currentRef = 1;
      } else if (super->config->CLK.XOSC.freq[i] < 8000000) {
        currentMulti = 3;
        currentRef = 2;
      } else if (super->config->CLK.XOSC.freq[i] < 16000000) {
        currentMulti = 4;
        currentRef = 3;
      } else if (super->config->CLK.XOSC.freq[i] < 24000000) {
        currentMulti = 5;
        currentRef = 3;
      } else {
        currentMulti = 6;
        currentRef = 3;
      }
      // Set XOSCTRL reg
      OSCCTRL->XOSCCTRL[i].reg = 0;

      OSCCTRL->XOSCCTRL[i].bit.ENALC = (uint8_t)super->config->CLK.XOSC.autoLoopCtrl;
      OSCCTRL->XOSCCTRL[i].bit.LOWBUFGAIN = (uint8_t)super->config->CLK.XOSC.lowBufferGain;
      OSCCTRL->XOSCCTRL[i].bit.ONDEMAND = (uint8_t)super->config->CLK.XOSC.onDemand;
      OSCCTRL->XOSCCTRL[i].bit.RUNSTDBY = (uint8_t)super->config->CLK.XOSC.runInStandby;

      OSCCTRL->XOSCCTRL[i].bit.CFDEN = (uint8_t)super->config->CLK.MISC.failiureDetection;
      OSCCTRL->XOSCCTRL[i].bit.SWBEN = (uint8_t)super->config->CLK.MISC.failiureDetection;

      OSCCTRL->XOSCCTRL[i].reg |=
          OSCCTRL_XOSCCTRL_STARTUP(super->config->CLK.XOSC.startupTimeSel)
        | OSCCTRL_XOSCCTRL_IMULT(currentMulti)
        | OSCCTRL_XOSCCTRL_IPTAT(currentRef)
        | OSCCTRL_XOSCCTRL_ENABLE;

      for (int j = 0; j < sizeof(sources) / sizeof(sources[0]); j++) {
        if (!sources[j]) {
          sources[j] == super->config->CLK.XOSC.freq[i];
        }
      }
    }
  }

  //// MAIN (CPU) CLOCKS //////////////////////////////////////////////////////////////////////////

  NVMCTRL->CTRLA.bit.RWS = 5; 

  if (!GCLK->GENCTRL[MAIN_GCLK_NUM].bit.GENEN) {
    GCLK->GENCTRL[MAIN_GCLK_NUM].reg = 
        GCLK_GENCTRL_SRC(super->config->CLK.MISC.cpuSrc) 
      | GCLK_GENCTRL_IDC;

    while(GCLK->SYNCBUSY.reg & (1 << MAIN_GCLK_NUM + GCLK_SYNCBUSY_GENCTRL0_Pos)); 
  }

  GCLK->GENCTRL[MAIN_GCLK_NUM].bit.RUNSTDBY = 1;
    while(GCLK->SYNCBUSY.reg & (1 << MAIN_GCLK_NUM + GCLK_SYNCBUSY_GENCTRL0_Pos));

  MCLK->CPUDIV.bit.DIV = super->config->CLK.MISC.cpuDivSelection;

  //// ENABLE INTERRUPTS //////////////////////////////////////////////////////////////////////////

  if (super->config->CLK.MISC.failiureDetection) {
    
    for (int i = 0; i < sizeof(CLK_IRQS) / sizeof(CLK_IRQS[0]); i++) {
      NVIC_ClearPendingIRQ(CLK_IRQS[i]);
      NVIC_SetPriority(CLK_IRQS[i], super->config->CLK.MISC.irqPriority);
      NVIC_EnableIRQ(CLK_IRQS[i]);
    }

    OSC32KCTRL->INTENSET.bit.XOSC32KFAIL = 1;
    OSCCTRL->INTENSET.reg |= 
        OSCCTRL_INTENSET_DFLLOOB 
      | OSCCTRL_INTFLAG_XOSCFAIL_Msk;
  }

  return true;
}







