///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "inttypes.h"
#include "math.h"
#include "string.h"
#include "Board.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool reset_pin(int pinID);

bool attach_pin(int pinID, int periphID);

bool set_pin(int pinID, int pinState, bool pullPin = false);

int read_pin(int pinID);

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> NVM FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

struct {
  bool lowPowerMode = false;
  bool activeLoading = false;
}flash_config;

enum FLASH_ERROR {
  FLASH_ERROR_NONE,
  FLASH_ERROR_LOCK,
  FLASH_ERROR_PROGRAM,
  FLASH_ERROR_ADDR
};

int write_flash(const volatile void *flash, void *data, int writeCount, 
  int dataAlignment, bool writePage);

int read_flash(const volatile void *flash, void *dest, int copyCount, int dataAlignment);

bool erase_flash(int blockIndex);

const volatile void *flash_addr(int addr); // To do -> Add page num primary

bool get_flash_locked(int pageNumber);
bool get_flash_locked(const volatile void *flash, int bytes = 0); // To do -> Add page num primary

bool set_flash_lock(const volatile void *flash, bool locked); // To do -> Add page num primary

FLASH_ERROR get_flash_error();

template<typename T>
T get_flash_value(const volatile void *flash) {
  if (!flash || (uint32_t)flash > FLASH_MAX_ADDR || get_flash_locked(flash))
    return T();
  T *valuePtr = static_cast<T*>(flash);
  return valuePtr == nullptr ? T() : *valuePtr;
}

template<typename T>
const volatile T *get_flash_ptr(const volatile void *flash) {
  if (!flash || (uint32_t)flash > FLASH_MAX_ADDR || get_flash_locked(flash))
    return nullptr;
  return reinterpret_cast<T*>(flash);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PROG FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

void prog_restart();

void NOCALL_prog_assert(bool statement, const int line, const char *func, const char *file);
#define prog_assert(statement) NOCALL_prog_assert(statement, __LINE__, __FUNCTION__, __FILE__);

void NOCALL_prog_deny(bool statement, const int line, const char *func, const char *file);
#define prog_deny(statement) NOCALL_prog_deny(statement, __LINE__, __FUNCTION__, __FILE__);

