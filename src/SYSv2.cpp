
#include "SYSv2.h"

void sys_set_xosc32k(const bool enabled, const uint16_t startupTime, const bool outputSignal) {

  OSCCTRL->XOSCCTRL->reg &= ~OSCCTRL_XOSCCTRL_MASK;
  OSCCTRL->XOSCCTRL->reg |= CLK_DEFAULT_CONFIG.XOSC32K_REG;

 

}

