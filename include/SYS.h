///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "inttypes.h"
#include <sys/types.h>
#include "math.h"
#include "string.h"
#include "Board.h"
#include <initializer_list>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool reset_pin(int pinID);

bool attach_pin(int pinID, int periphID);

bool set_pin(int pinID, int pinState, bool pullPin = false);

int read_pin(int pinID);

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

enum FLASH_ERROR {
  FLASH_ERROR_NONE,
  FLASH_ERROR_PARAM,
  FLASH_ERROR_LOCK,
  FLASH_ERROR_PROG,
  FLASH_ERROR_ADDR
};

FLASH_ERROR flash_update_config();

FLASH_ERROR flash_write_data(unsigned int &flashIndex, const void *data, 
  const size_t bytes);

FLASH_ERROR flash_read_data(unsigned int &flashIndex, void *dest, 
  const size_t bytes = sizeof(findex_t));

FLASH_ERROR flash_erase(unsigned int flashIndex, size_t indexCount, 
  bool forceAligned = false);

FLASH_ERROR flash_clear();

bool flash_is_free(unsigned int flashIndex, size_t bytes);

int flash_find_free(unsigned int flashStartIndex, size_t bytes);

unsigned int flash_query(size_t requiredBytes, unsigned int startFlashIndex = 0); // Not done

FLASH_ERROR flash_set_region_lock(unsigned int regionIndex, bool locked);

bool flash_get_region_locked(unsigned int regionIndex);

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SMART EEPROM FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum SEEPROM_ERROR {
  SEEPROM_ERROR_NONE,
  SEEPROM_ERROR_INT,
  SEEPROM_ERROR_STATE,
  SEEPROM_ERROR_FULL,
  SEEPROM_ERROR_PARAM,
  SEEPROM_ERROR_LOCK,
  SEEPROM_ERROR_ADDR
};

struct {
  bool autoRealloc = true;
  bool pageBuffer = false;
  bool locked = false;
  bool checkAddr = true;
}seeprom_config;

SEEPROM_ERROR seeprom_init(int minBytes, bool restartNow = true); // NEEDS CHECK

SEEPROM_ERROR seeprom_update_config(); // DONE

SEEPROM_ERROR seeprom_exit(bool restartNow = true); 

SEEPROM_ERROR seeprom_write_data(uint &seepromIndex, void *data, uint bytes,
  bool blocking = true);

SEEPROM_ERROR seeprom_read_data(uint &seepromIndex, void *dest, uint bytes,
  bool blocking = true);

SEEPROM_ERROR seeprom_realloc(); 

SEEPROM_ERROR seeprom_flush_buffer(); 

SEEPROM_ERROR seeprom_clear_buffer(); 

bool seeprom_get_init();

bool seeprom_get_full();

bool seeprom_get_busy();

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PROG FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

void prog_restart() [[noReturn]];

void NOCALL_prog_assert(bool statement, const int line, const char *func, const char *file);
#define prog_assert(statement) NOCALL_prog_assert(statement, __LINE__, __FUNCTION__, __FILE__);

void NOCALL_prog_deny(bool statement, const int line, const char *func, const char *file);
#define prog_deny(statement) NOCALL_prog_deny(statement, __LINE__, __FUNCTION__, __FILE__);

