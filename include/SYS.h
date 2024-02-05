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

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool pin_reset(unsigned int pinID);

bool pin_attach(unsigned int pinID, unsigned int periphID);

bool pin_valid(const unsigned int pinID);

bool pin_set_digital(unsigned int pinID, unsigned int pinState, bool pullPin = false);

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
//// SECTION -> PROG FUNCTIONS
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

__attribute__ ((long_call, section (".ramfunc")))
void prog_reset(bool hardReset = false);

PROG_RESET_REASON prog_get_reset_reason();





unsigned int prog_get_serial_number(uint8_t *resultArray);

void NOCALL_prog_assert(bool statement, const int line, const char *func, const char *file);
#define prog_assert(statement) NOCALL_prog_assert(statement, __LINE__, __FUNCTION__, __FILE__)

void NOCALL_prog_deny(bool statement, const int line, const char *func, const char *file);
#define prog_deny(statement) NOCALL_prog_deny(statement, __LINE__, __FUNCTION__, __FILE__)

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> WDT
///////////////////////////////////////////////////////////////////////////////////////////////////

enum WDT_ERROR {

};

struct {
  bool alwaysOn = false;
  bool windowMode = false;
  void (*wdtInterrupt)(void) = nullptr;
}wdt_config;

bool wdt_update_config();

bool wdt_enable(unsigned int resetTimeout);

bool wdt_disable();

bool wdt_clear();

bool wdt_get_timeout(bool windowTimeout);

