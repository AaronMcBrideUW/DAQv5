
#pragma once
#include "sam.h"

// Default configuration for sys clk
const struct {
  uint16_t XOSC32K_REG =
      OSC32KCTRL_XOSC32K_CGM(OSC32KCTRL_XOSC32K_CGM_XT_Val)
    | OSC32KCTRL_XOSC32K_STARTUP(2);

}CLK_DEFAULT_CONFIG;



void sys_set_xosc32k(const bool enabled, const uint16_t startupTime, const bool outputSignal);