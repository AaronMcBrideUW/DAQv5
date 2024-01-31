///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "sam.h"
#include "math.h"
#include "string.h"

/// @brief Oscillator statuses
enum OSC_STATUS {
  OSC_STATUS_NULL,
  OSC_STATUS_ENABLED_LOCKING,
  OSC_STATUS_ENABLED_LOCKED,
  OSC_STATUS_DISABLED,
  OSC_STATUS_ERROR
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SYS CLK
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief RTC clock signal sources
enum RTC_SOURCE : uint8_t {
  RTC_OSCULP1K = OSC32KCTRL_RTCCTRL_RTCSEL_ULP1K_Val,
  RTC_OSCULP32K = OSC32KCTRL_RTCCTRL_RTCSEL_ULP32K_Val,
  RTC_XOSC1K = OSC32KCTRL_RTCCTRL_RTCSEL_XOSC1K_Val,
  RTC_XOSC32K = OSC32KCTRL_RTCCTRL_RTCSEL_XOSC32K_Val
};

/// @brief Sets the frequency of high speed or regular speed cpu domain 
/// @param freq Frequency of the cpu domain (must be less than that of GCLK0)
/// @param highSpeedDomain True = set high speed domain freq, false = set regular speed domain freq
/// @return True if frequency was set, false otherwise
bool set_cpu_freq(int freq, bool highSpeedDomain);


/// @brief Gets the frequency of the specified cpu domain
/// @param highSpeedDomain True = get high speed domain freq, false = get regular speed domain freq
/// @return An integer, equal to the frequency of the specified cpu domain
int get_cpu_freq(bool highSpeedDomain);


/// @brief Sets the source for the real time clock
/// @param source The source oscillator (ENUM - RTC_...) that the real time clock will use
/// @return True if the rtc source was set, false otherwise.
bool set_rtc_src(RTC_SOURCE source);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> GCLK
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief GCLK configurations
struct {
  bool runInStandby = false;
  bool improveDutyCycle = false;
  bool enableOutput = false;
}gclk_config[GCLK_NUM];

/// @brief GCLK clock signal sources
enum GCLK_SOURCE : uint8_t {
  GCLK_NULL,
  GCLK_XOSC32K = GCLK_SOURCE_XOSC32K,
  GCLK_OSCULP32K = GCLK_SOURCE_OSCULP32K,
  GCLK_DFLL = GCLK_SOURCE_DFLL,
  GCLK_DPLL0 = GCLK_SOURCE_DPLL0,
  GCLK_DPLL1 = GCLK_SOURCE_DPLL1
};

/// @brief Enables/Disables a GCLK
/// @param gclkNum The id number of the GCLK to set 
/// - NOTE: GCLK 0 is the GCLK source for the CPU 
/// @param enabled True = enable GCLK, false = disable GCLK
/// @param source The source oscillator of the GCLK (if enabling only) (ENUM - GCLK_...)
/// @param freq The frequency of the GCLK (if enabling only)
/// @return True if the GCLK was set, false otherwise
bool set_gclk(int gclkNum, bool enabled, GCLK_SOURCE source = GCLK_NULL, int freq = 0);


/// @brief Configures a GCLK peripheral channel
/// @param channelNum The id number of the channel to configure 
/// @param linkEnabled True = enable channel, false = disable channel 
/// @param gclkNum The number of the GCLK to link to the channel (if enabling only)
/// @return True if the channel was set, false otherwise
bool set_gclk_channel(int channelNum, bool channelEnabled, int gclkNum = 0);


/// @brief Gets the frequency of a GCLK
/// @param gclkNum The id number of the GCLK
/// @return An integer equal to the current frequency of the GCLK
int get_gclk_freq(int gclkNum);


/// @brief Gets all channels linked to a GCLK
/// @param gclkNum The id number of the GCLK
/// @param resultArray Result array to store the channel numbers (optional)
/// @param arrayLength Length of the result array (required if result array != null)
/// @return An integer equal to the number of channels attached to the GCLK or -1 if
///         an error occured
int get_gclk_channels(int gclkNum, int *resultArray, int arrayLength);


/// @brief Gets the id number of the GCLK linked to a channel
/// @param channeNum The number of the GCLK channel
/// @return An integer equal to the ID number of the GCLK linked to the channel, or -1
///         if the channel is disabled, not linked, or an error occured.
int get_channel_gclk(int channeNum);


/// @brief Gets the status of a GCLK 
/// @param gclkNum The id number of the GCLK
/// @return The status of the GCLK channel (ENUM - GCLK_STATUS_...)
OSC_STATUS get_gclk_status(int gclkNum);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> OSCULP32K 
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Valid OSCULP32K frequencies
enum OSCULP_FREQ {
  OSCULP_FREQ_NULL,
  OSCULP_FREQ_1KHZ,
  OSCULP_FREQ_32KHZ
};

/// @brief Enables/Disables the ultra low power oscillator (OSCULP)
/// @param enabled True = enable oscillator, false = disable oscillator
/// @param freqSel The frequency mode of the oscillator (ENUM - OSCULP_FREQ_...)
/// @return True if the oscillator was set, false otherwise
bool set_osculp(bool enabled, OSCULP_FREQ freqSel = OSCULP_FREQ_NULL);


/// @brief Gets the frequency of the ultra lower power oscillator (OSCULP)
/// @return An integer equal to the OSCULP, or -1 if it is disabled
int get_osculp_freq();


/// @brief Gets the status of the ultra lower power oscillator (OSCULP)
/// @return The status of the OSCULP (ENUM - OSC_STATUS_...)
OSC_STATUS get_osculp_status();


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> XOSC32K 
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief XOSC32K configuration
struct {
  bool highSpeedMode = false;
  bool onDemand = true;
  bool runInStandby = false;
  bool outputSignal = false;
  bool enableCFD = true;
}xosc32k_config;

/// @brief Valid XOSC32K frequencies
enum XOSC32K_FREQ {
  XOSC32K_FREQ_NULL,
  XOSC32K_FREQ_1KHZ,
  XOSC32K_FREQ_32KHZ
};

/// @brief Enables/Disables the external crystal oscillator (XOSC32K)
/// @param enabled True = enable XOSC32K, false = disable XOSC32K
/// @param freqSel The frequency mode of the XOSC32K (if enabling only) (ENUM - XOSC32K_FREQ_...)
/// @param startupTime The startup time of the XOSC32K in milliseconds.
/// @param waitForLock True = function is blocking until freq is locked, false = non-blocking
/// @return True if the XOSC32K was set, false otherwise.
bool set_xosc32k(bool enabled, XOSC32K_FREQ freqSel = XOSC32K_FREQ_NULL, int startupTime = 0, 
  bool waitForLock = false);


/// @brief Gets the frequency of the external crystal oscillator (XOSC32K)
/// @return An integer equal to the frequency of the XOSC32K, or -1 if it is disabled
int get_xosc32k_freq();


/// @brief Gets the status of the external crystal oscillator (XOSC32K)
/// @return The status of the XOSC32K (ENUM - OSC_STATUS_...)
OSC_STATUS get_xosc32k_status();


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DFLL
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief DFLL configuration
struct {
  bool onDemand = true;
  bool runInStandby = false;
  bool waitForLock = false;
  bool quickLock = false;
  bool chillCycle = true;
  bool usbRecoveryMode = true;
  bool bypassCoarseLock = false;
  bool stabalizeFreq = true;
  uint8_t maxFineAdj = 1;
  uint8_t maxCoarseAdj = 1;
  uint8_t coarseAdj = 0;
  uint8_t fineAdj = 0;
}dfll_config;

/// @brief Enables/Disables the digital frequency locked loop oscillator (DFLL)
/// @param enabled True = enable DFLL, false = disable DFLL
/// @param freq The target frequency of the DFLL (if enabling only)
/// @param closedLoopMode True = closed loop mode, false = open loop mode (if enabling only)
/// @param gclkNum The id number of the GCLK to use as the DFLL's reference (if closed loop mode & enabling only)
/// @param waitForLock True = function is blocking until freq is locked, false = function is non-blocking
/// @return True if DFLL was set, false otherwise.
bool set_dfll(bool enabled, int freq = 0, bool closedLoopMode = false, int gclkNum = -1, 
  bool waitForLock = true);


/// @brief Gets the current frequency of the digital frequency locked loop oscillator (DFLL)
/// @return An integer equal to the frequency of the DFLL, or -1 if it is disabled
int get_dfll_freq();


/// @brief Gets the error in the dfll's current frequency
/// @return A float dennoting the error percentage in the frequency reported by get_dfll_freq (0-1)
float get_dfll_drift();


/// @brief Gets the status of the digital frequency locked loop oscillator (DFLL)
/// @return The status of the DFLL (ENUM - OSC_STATUS_...)
OSC_STATUS get_dfll_status();


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> DPLL
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief DPLL configurations
struct {
  bool onDemand = true;
  bool runInStandby = false;
  bool dcoFilterEnabled = false;
  uint8_t dcoFilterSel = 0;
  bool lockTimeout = true;
  bool wakeUpFast = false;
  uint8_t integralFilterSel = 0;
  bool ceilFreq = false;
  unsigned int maxFreqOffset = 10000;
}dpll_config[OSCCTRL_DPLLS_NUM];

/// @brief Valid DPLL references
enum DPLL_SRC : uint8_t {
  DPLL_SRC_NULL = 2,
  DPLL_SRC_GCLK0 = 0,
  DPLL_SRC_XOSC32K = 1
};

/// @brief Enables/Disables one of the digital phase locked loop oscillators (DPLL)
/// @param dpllNum The id number of the DPLL to set
/// @param enabled True = enable target DPLL, false = disable target DPLL (if enabling only)
/// @param source The reference oscillator for the target DPLL to use (if enabling only) (ENUM - DPLL_SRC_...)
/// @param freq The target frequency for the target DPLL (if enabling only)
/// @param waitForLock True = function is blocking until freq is locked, false = function is non-blocking
/// @return True if the target DPLL was set, false otherwise
bool set_dpll(int dpllNum, bool enabled, DPLL_SRC source = DPLL_SRC_NULL, int freq = 0, 
  bool waitForLock = true);


/// @brief Gets the frequency of a digital phase locked loop oscillator (DPLL)
/// @param dpllNum The id number of the target DPLL
/// @return An integer equal to the current frequency of the target DPLL, or -1 if it is disabled.
int get_dpll_freq(int dpllNum);


/// @brief Gets the status of a digital phase locked loop oscillator (DPLL)
/// @param dpllNum The id number of the target DPLL
/// @return The status of the target DPLL (ENUM - OSC_STATUS_...)
OSC_STATUS get_dpll_status(int dpllNum);