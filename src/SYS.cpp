///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <SYS.h>

#define DPLL_FAIL_MASK (OSCCTRL_STATUS_DPLL1TO | OSCCTRL_STATUS_DPLL1LCKF \
                      | OSCCTRL_STATUS_DPLL0TO | OSCCTRL_STATUS_DPLL0LCKF)

const IRQn_Type CLK_IRQS[] = {
  OSC32KCTRL_IRQn, 
  OSCCTRL_0_IRQn, 
  OSCCTRL_1_IRQn, 
  OSCCTRL_2_IRQn, 
  OSCCTRL_3_IRQn,
  OSCCTRL_4_IRQn
};

enum DPLL_REFERENCE : uint8_t {
  REF_GCLK,
  REF_XOSC32K,
  REF_XOSC0,
  REF_XOSC1
};

enum SYS_PERIPHERAL : uint8_t {

};

enum SOURCE_ID : uint8_t {
  OSCULP32K,
  XOSC32K,
  XOSC0,
  XOSC1,
  DFLL,
  DPLL0,
  DPLL1
};

struct CLK_SOURCE_DESCRIPTOR {
  bool enabled;
  SOURCE_ID id; 
  uint32_t currentFreq;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> CLK IRQ
///////////////////////////////////////////////////////////////////////////////////////////////////

void oscHandler(void) {
  // TO DO 
}

void OSC32KCTRL_Handler (void) {
  // TO DO 
}

void OSCCTRL_0_Handler ( void ) __attribute__((weak, alias("oscHandler")));
void OSCCTRL_1_Handler ( void ) __attribute__((weak, alias("oscHandler")));
void OSCCTRL_2_Handler ( void ) __attribute__((weak, alias("oscHandler")));
void OSCCTRL_3_Handler ( void ) __attribute__((weak, alias("oscHandler")));
void OSCCTRL_4_Handler ( void ) __attribute__((weak, alias("oscHandler")));

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYSTEM CLASS METHODS
///////////////////////////////////////////////////////////////////////////////////////////////////

SystemConfig &defaultStartup;

int8_t System_::CLK_::allocGCLK(const uint32_t &targFreq)  {
  if (!init) return false;

  // Find clock to alloc
  uint8_t num = 0;
  for (uint16_t i = 0; i < sizeof(agclk) / sizeof(agclk[0]); i++) {
    if (!agclk[i]) {
      agclk[i] = true;
      num = i;
    } else if (i == (sizeof(agclk) / sizeof(agclk[0]) - 1)) {
      return false;
    }
  }

  // Find source for clock
  /*
  for (uint16_t i = 0; i < sizeof(CLK_SOURCES) / sizeof(CLK_SOURCES[0]); i++) {
    if (CLK_SOURCES[i].currentFreq > targFreq) {
      // Set as source...
    } else if (i == sizeof(CLK_SOURCE) / sizeof(CLK_SOURCE[0])) {
      // Activate new source

    }
  }
  */

}


void System_::CLK_::initialize() { ///// TO DO -> ADD MAIN CLOCK STUFF HERE
  if (init) return;
  init = true;

  NVMCTRL->CTRLA.bit.RWS = 5;

  //// DISABLE ALL INTERRUPTS & FLAGS ////

  OSC32KCTRL->INTENCLR.reg |= OSC32KCTRL_INTENCLR_MASK;    
  OSC32KCTRL->INTFLAG.reg |= OSC32KCTRL_INTFLAG_MASK;       

  OSCCTRL->INTENCLR.reg |= OSCCTRL_INTENCLR_MASK;
  OSCCTRL->INTFLAG.reg |= OSCCTRL_INTFLAG_MASK;

  //// SETUP THE 32K CRYSTAL ////

  OSC32KCTRL->XOSC32K.reg = 0;     // Disable xosc32k
  OSC32KCTRL->XOSC32K.bit.RUNSTDBY = (uint8_t)(super->config->CLK.XOSC32K.runInStandby);
  OSC32KCTRL->XOSC32K.bit.XTALEN = (uint8_t)(super->config->CLK.XOSC32K.exportSignal);
  
  OSC32KCTRL->XOSC32K.reg |=
      OSC32KCTRL_XOSC32K_CGM(OSC32KCTRL_XOSC32K_CGM_XT_Val)  // Set to standard mode
    | OSC32KCTRL_XOSC32K_EN1K                                // Enable 32khz output
    | OSC32KCTRL_XOSC32K_EN32K;                              // Enable 1khz output

  OSC32KCTRL->CFDCTRL.reg = 0;
  OSC32KCTRL->CFDCTRL.reg |= 
      OSC32KCTRL_CFDCTRL_SWBACK     // Enable clock switching on failiure
    | OSC32KCTRL_CFDCTRL_CFDEN;     // Enable clock failiure detection

  //// SETUP THE DFLL ////

  // Set the CTRLA reg
  OSCCTRL->DFLLCTRLA.reg = 0;
    while(OSCCTRL->DFLLSYNC.bit.ENABLE && OSCCTRL->DFLLCTRLA.bit.ENABLE); 
  OSCCTRL->DFLLCTRLA.bit.ONDEMAND = (uint8_t)super->config->CLK.DFLL.onDemand;
  OSCCTRL->DFLLCTRLA.bit.RUNSTDBY = (uint8_t)super->config->CLK.DFLL.runInStandby;

  // Set CTRLB reg
  OSCCTRL->DFLLCTRLB.reg = 0;
    while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

  OSCCTRL->DFLLCTRLB.bit.CCDIS = (uint8_t)(!super->config->CLK.DFLL.chillCycle);
    while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
  OSCCTRL->DFLLCTRLB.bit.STABLE = (uint8_t)(!super->config->CLK.DFLL.stabalizeFreq);
    while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
  OSCCTRL->DFLLCTRLB.bit.MODE = (uint8_t)(super->config->CLK.DFLL.closedLoopMode);
    while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

  OSCCTRL->DFLLCTRLB.bit.USBCRM = 1;
    while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);

  if (super->config->CLK.DFLL.closedLoopMode) {
    OSCCTRL->DFLLCTRLB.bit.WAITLOCK = 1;
      while(OSCCTRL->DFLLSYNC.bit.DFLLCTRLB);
  }

  // Set MUL reg
  OSCCTRL->DFLLMUL.reg = 0;                                  
    while(OSCCTRL->DFLLSYNC.bit.DFLLMUL); 

  if (super->config->CLK.DFLL.closedLoopMode) {
    OSCCTRL->DFLLMUL.reg |=
        OSCCTRL_DFLLMUL_CSTEP(super->config->CLK.DFLL.maxCoarseAdjStep)
      | OSCCTRL_DFLLMUL_FSTEP(super->config->CLK.DFLL.maxFineAdjStep);
      while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);
  }
  // Set VAL reg
  OSCCTRL->DFLLVAL.reg = 0;
    while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

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

  //// SET UP THE DPLL's ////

  for (int16_t i = 0; i < OSCCTRL_DPLLS_NUM; i++) {

    // Set CTRLA reg
    OSCCTRL->Dpll[i].DPLLCTRLA.reg = 0;
      while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.ENABLE);
    OSCCTRL->Dpll[i].DPLLCTRLA.bit.ONDEMAND = (uint8_t)super->config->CLK.DPLL.onDemand;
    OSCCTRL->Dpll[i].DPLLCTRLA.bit.RUNSTDBY = (uint8_t)super->config->CLK.DPLL.runInStandby;
      
    // Set CTRLB reg 
    OSCCTRL->Dpll[i].DPLLCTRLB.reg = 0;
    OSCCTRL->Dpll[i].DPLLCTRLB.reg |= 
        OSCCTRL_DPLLCTRLB_DIV(super->config->CLK.DPLL.xoscDivFactor)
      | OSCCTRL_DPLLCTRLB_DCOFILTER(super->config->CLK.DPLL.dcoFilterSelection)
      | OSCCTRL_DPLLCTRLB_FILTER(super->config->CLK.DPLL.integralFilterSelection)
      | OSCCTRL_DPLLCTRLB_LBYPASS;
    OSCCTRL->Dpll[i].DPLLCTRLB.bit.DCOEN = (uint8_t)((bool)super->config->CLK.DPLL.dcoFilterSelection);
    OSCCTRL->Dpll[i].DPLLCTRLB.bit.LTIME = (uint8_t)(super->config->CLK.DPLL.lockTimeoutSelection);

    // Clear the ratio register
    OSCCTRL->Dpll[i].DPLLRATIO.reg = 0;
      while(OSCCTRL->Dpll[i].DPLLSYNCBUSY.bit.DPLLRATIO);
  }

  //// SET UP THE GCLKS ////

  GCLK->CTRLA.bit.SWRST = 1;
    while(GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_SWRST || GCLK->CTRLA.bit.SWRST);

  // Set GENCTRL reg
  for (int16_t i = 0; i < GCLK_NUM; i++) {
    GCLK->GENCTRL[i].bit.RUNSTDBY = (uint8_t)super->config->CLK.GEN.runInStandby;
      while(GCLK->SYNCBUSY.reg & (1 << i + GCLK_SYNCBUSY_GENCTRL0_Pos));
    GCLK->GENCTRL[i].bit.IDC = (uint8_t)super->config->CLK.GEN.improveDutyCycle;
      while(GCLK->SYNCBUSY.reg & (1 << i + GCLK_SYNCBUSY_GENCTRL0_Pos));
  }

  // Enable interrupts
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


bool System_::CLK_::setXOSC32K(bool enabled, bool highSpeed) {
  if (enabled) {
    OSC32KCTRL->XOSC32K.bit.ENABLE = 0;
      while(OSC32KCTRL->STATUS.bit.XOSC32KRDY);

    OSC32KCTRL->XOSC32K.bit.CGM = (uint8_t)highSpeed;

    OSC32KCTRL->XOSC32K.bit.ENABLE = 1;
      while(!OSC32KCTRL->STATUS.bit.XOSC32KRDY);
  } else {
    OSC32KCTRL->XOSC32K.bit.ENABLE = 0;
      while(OSC32KCTRL->STATUS.bit.XOSC32KRDY);
  }
  return true;
}


bool System_::CLK_::setDFLL(bool enabled, uint16_t multiplier) {
  if (!init) return false;

  // Enable DFLL
  if (enabled) {
    if (multiplier != 0) {
      OSCCTRL->DFLLCTRLA.bit.ENABLE = 0;
        while(OSCCTRL->DFLLSYNC.bit.ENABLE);

      OSCCTRL->DFLLMUL.bit.MUL = multiplier;
        while(OSCCTRL->DFLLSYNC.bit.DFLLMUL);

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
    }  
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 1;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE);

    OSCCTRL->DFLLVAL.reg = OSCCTRL->DFLLVAL.reg;
      while(OSCCTRL->DFLLSYNC.bit.DFLLVAL);

    while(!OSCCTRL->STATUS.bit.DFLLRDY);

  // Disable DFLL
  } else {
    OSCCTRL->DFLLCTRLA.bit.ENABLE = 0;
      while(OSCCTRL->DFLLSYNC.bit.ENABLE);
  }
}


bool System_::CLK_::setDPLL(uint8_t dpllNum, bool enabled, DPLL_REFERENCE ref, 
  uint8_t multiplyNumerator, uint8_t multiplyDenominator) {
  if (!init) return false;

  if (enabled) {
    // If config is changing -> disable the osc
    if (multiplyNumerator != OSCCTRL->Dpll[dpllNum].DPLLRATIO.bit.LDR
    || multiplyDenominator != OSCCTRL->Dpll[dpllNum].DPLLRATIO.bit.LDRFRAC
    || ref != OSCCTRL->Dpll[dpllNum].DPLLCTRLB.bit.REFCLK) {

      OSCCTRL->Dpll[dpllNum].DPLLCTRLA.bit.ENABLE = 1;
        while(OSCCTRL->Dpll[dpllNum].DPLLSYNCBUSY.bit.ENABLE);
    }
    // Set new configs 
    OSCCTRL->Dpll[dpllNum].DPLLRATIO.reg = 0;
      while(OSCCTRL->Dpll[dpllNum].DPLLSYNCBUSY.bit.DPLLRATIO);

    OSCCTRL->Dpll[dpllNum].DPLLRATIO.reg |=
        OSCCTRL_DPLLRATIO_LDRFRAC(multiplyNumerator)
      | OSCCTRL_DPLLRATIO_LDR(multiplyDenominator); 
      while(OSCCTRL->Dpll[dpllNum].DPLLSYNCBUSY.bit.DPLLRATIO);

    OSCCTRL->Dpll[dpllNum].DPLLCTRLB.bit.REFCLK = ref;

    // Enable the osc -> check for timeout or failiure
    while(!OSCCTRL->Dpll[dpllNum].DPLLSTATUS.reg & OSCCTRL_DPLLSTATUS_MASK 
      != OSCCTRL_DPLLSTATUS_MASK) {
      
      if (OSCCTRL->STATUS.reg & DPLL_FAIL_MASK >= 1) {
        OSCCTRL->Dpll[dpllNum].DPLLCTRLA.bit.ENABLE = 0;
          while(OSCCTRL->Dpll[dpllNum].DPLLSYNCBUSY.bit.ENABLE);
        return false;
      } 
    }
  } else if (!OSCCTRL->Dpll[dpllNum].DPLLCTRLA.bit.ENABLE) {

    // Disable dpll -> & reset interrupt flags
    OSCCTRL->Dpll[dpllNum].DPLLCTRLA.bit.ENABLE = 0;
      while(OSCCTRL->Dpll[dpllNum].DPLLSYNCBUSY.bit.ENABLE);

    if (dpllNum == 0) {
      OSCCTRL->INTFLAG.reg &= ~(OSCCTRL_INTFLAG_DPLL0LTO | OSCCTRL_INTFLAG_DPLL0LCKF 
        | OSCCTRL_INTFLAG_DPLL0LCKR | OSCCTRL_INTFLAG_DPLL0LDRTO);

    } else if (dpllNum == 1) {
      OSCCTRL->INTFLAG.reg &= ~(OSCCTRL_INTFLAG_DPLL1LTO | OSCCTRL_INTFLAG_DPLL1LCKF 
        | OSCCTRL_INTFLAG_DPLL1LCKR | OSCCTRL_INTFLAG_DPLL1LDRTO);
    }
  }
  return true;
}







