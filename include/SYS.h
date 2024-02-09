/*

///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stdint.h>
#include <math.h>
#include "string.h"
#include "Board.h"

enum FLASH_ERROR {
  FLASH_ERROR_NONE,
  FLASH_ERROR_PARAM,
  FLASH_ERROR_LOCK,
  FLASH_ERROR_PROG,
  FLASH_ERROR_ADDR
};

enum SEEPROM_ERROR {
  SEEPROM_ERROR_NONE,
  SEEPROM_ERROR_INT,
  SEEPROM_ERROR_STATE,
  SEEPROM_ERROR_FULL,
  SEEPROM_ERROR_PARAM,
  SEEPROM_ERROR_LOCK,
  SEEPROM_ERROR_ADDR
};

/// @defgroup Important program settings
#define PROG_DEBUG_MODE false   
#define PROG_ASSERT_ENABLED true
#define PROG_HARD_RESET_ENABLED true

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Resets the state & settings of a pin
/// @param pinID The id number of the pin to reset
/// @return True if the opperation was successful, false otherwise.
bool pin_reset(unsigned int pinID);

/// @brief Attaches a pin to a peripheral, allowing the periphal to control it
/// @param pinID The id number of the pin to target
/// @param periphID The id number of the periphal to link to
/// @return True if the opperation was successful, false otherwise
bool pin_attach(unsigned int pinID, unsigned int periphID);

/// @brief Ensures that a given pinID is valid (i.e. exists on board) 
/// @param pinID The pinID to check
/// @return True if the pin associated with the given ID is valid, false otherwise.
bool pin_valid(const unsigned int pinID);

/// @brief Sets the digital output of a GPIO pin
/// @param pinID The id of the pin to set
/// @param pinState 1 = drive high, -1 = drive low, 0 = dont drive
/// @param pullPin If true pullup ressitor is connected to pin. When a pin
///   with the pullup is driven high -> "pulled up" driven low -> "pulled down"
/// @return True if the digital output of the pin was successfully set.
bool pin_set_digital(unsigned int pinID, unsigned int pinState, bool pullPin = false);

/// @brief Returns the current reading (digital) of a GPIO pin.
/// @param pinID The id of the pin to target
/// @return 1 if the pin reads "high", -1 if the pin reads "low" and 0 if neither.                  CHECKS THIS <----------------- 
int pin_read_digital(unsigned int pinID);

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> FLASH MEM FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  bool lowPowerOnSleep = false;
  bool lowPowerMode = false;
  bool writePage = false;
  bool boundAddr = true;
  void (*errorInterrupt)(FLASH_ERROR) = nullptr;
}flash_config;

struct flash_properties_ {
  static const unsigned int index_size;
  static const unsigned int page_size;
  static const unsigned int region_size;
  static const unsigned int total_size;
  static const unsigned int index_count;
  static const unsigned int page_count;
  static const unsigned int region_count;
  static const unsigned int default_align;
};
extern flash_properties_ flash_properties;


FLASH_ERROR flash_update_config();

FLASH_ERROR flash_write_data(unsigned int &flashIndex, const void *data, 
  const unsigned int bytes);

FLASH_ERROR flash_copy_data(unsigned int &flashIndex, void *dest, 
  const unsigned int bytes);

const volatile void *flash_read_data(unsigned int &flashIndex, unsigned int bytes);

FLASH_ERROR flash_erase(unsigned int flashIndex, unsigned int indexCount, 
  bool forceAligned = false);

FLASH_ERROR flash_clear();

bool flash_is_free(unsigned int flashIndex, unsigned int bytes);

int flash_find_free(unsigned int bytes, unsigned int flashStartIndex);

int flash_query(const void *data, unsigned int bytes, unsigned int startFlashIndex = 0);

FLASH_ERROR flash_set_region_lock(unsigned int regionIndex, bool locked);

bool flash_get_region_locked(unsigned int regionIndex);

unsigned int flash_index2region(unsigned int flashIndex);

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SMART EEPROM FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  bool autoRealloc = true;
  bool pageBuffer = false;
  bool locked = false;
  bool checkAddr = true;
  void (*errorInterrupt)(SEEPROM_ERROR) = nullptr;
}seeprom_config;

struct seeprom_properties_ {
  static const unsigned int max_index_count;
  static const unsigned int max_total_size;
  static const unsigned int max_page_size;
  static const unsigned int index_size;
  static const unsigned int default_align; 
};
extern seeprom_properties_ seeprom_properties;


SEEPROM_ERROR seeprom_update_config();

SEEPROM_ERROR seeprom_init(int minBytes, bool restartNow = true); 

SEEPROM_ERROR seeprom_exit(bool restartNow = true); 

SEEPROM_ERROR seeprom_write_data(unsigned int &seepromIndex, void *data, unsigned int bytes,
  bool blocking = true);

SEEPROM_ERROR seeprom_copy_data(unsigned int &seepromIndex, void *dest, unsigned int bytes,
  bool blocking = true);

const volatile void *seeprom_read_data(unsigned int &seepromIndex, unsigned int bytes = 0);

SEEPROM_ERROR seeprom_realloc(); 

SEEPROM_ERROR seeprom_flush_buffer(); 

SEEPROM_ERROR seeprom_clear_buffer(); 

bool seeprom_get_init();

bool seeprom_get_full();

bool seeprom_get_busy();

bool seeprom_get_pending_data();

unsigned int seeprom_get_size();

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: PROG FUNCTIONS (NEEDS TO BE COMPLETED...)
///////////////////////////////////////////////////////////////////////////////////////////////////

enum PROG_RESET_REASON {
  UNKNOWN_RESET,
  BACKUP_RESET,
  SYSTEM_REQ_RESET,
  WATCHDOG_RESET,
  EXTERNAL_RESET,
  NVM_RESET,
  BROWNOUT_RESET,
  POWERON_RESET
};

enum PROG_SLEEP_MODE {
  SLEEP_IDLE = 2,
  SLEEP_STANDBY = 4,
  SLEEP_HIBERNATE = 5,
  SLEEP_BACKUP = 6
};

__attribute__ ((long_call, section (".ramfunc")))
void prog_reset(bool hardReset = false); // NOT COMPLETE

PROG_RESET_REASON prog_get_reset_reason();

unsigned int prog_get_serial_number(uint8_t *resultArray);

unsigned int prog_get_cpuid(PROG_SLEEP_MODE mode);

bool prog_sleep(PROG_SLEEP_MODE mode); // NOT COMPLETE

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: WATCHDOG TIMER
/////////////////////////////////////////////////////////////////////////////////////////////////// NEEDS REFACTOR

enum WDT_SETTING {
  WDT_RESET_TIMEOUT,
  WDT_INTERRUPT_TIMEOUT,
  WDT_WINDOW_TIMEOUT,
  WDT_ENABLED
};

struct {
  bool alwaysOn = false;
  bool windowMode = false;
  void (*wdtInterrupt)(void) = nullptr;
}wdt_config;

bool wdt_update_config();

bool wdt_set(bool enabled, unsigned int resetTimeout, unsigned int interruptTimeoutOffset,
  unsigned int windowTimeout);

void wdt_clear();

unsigned int wdt_get_setting(WDT_SETTING settingSel);

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: RTC
///////////////////////////////////////////////////////////////////////////////////////////////////

*/