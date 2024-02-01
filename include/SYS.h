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
//// SECTION -> FLASH MEM FUNCTIONS
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

int write_flash_data(uint32_t flashAddr, const volatile void *dataAddr, int writeCount, 
  int dataAlignment, bool writePage);
int write_flash_data(const volatile void *flashPtr, const volatile void *dataPtr, int writeCount, 
  int dataAlignment, bool writePage);

int read_flash_data(uint32_t flashAddr, void *destPtr, int copyCount, int dataAlignment);
int read_flash_data(const volatile void *flashPtr, void *destPtr, int copyCount, int dataAlignment);

bool erase_flash(int blockIndex);
bool erase_flash(const volatile void *flashPtr);

bool get_flash_lock(int regionIndex);
bool get_flash_lock(const volatile void *flashPtr);

bool set_flash_lock(int regionIndex, bool locked);
bool set_flash_lock(const volatile void *flashPtr, bool locked);

int get_flash_page(uint32_t flashAddr);
int get_flash_page(const volatile void *flashPtr);

int get_flash_block(uint32_t flashAddr);
int get_flash_block(const volatile void *flashPtr);

int get_flash_region(uint32_t flashAddr);
int get_flash_region(const volatile void *flashPtr);

FLASH_ERROR get_flash_error(); 

template<typename T>
T read_flash_value(uint32_t flashAddr) {
  if (!validFAddr(flashAddr))
    return T();
  T *valuePtr = static_cast<T*>(flashAddr);
  return (valuePtr == null ? T() : *valuePtr);
}
template<typename T>
T read_flash_value(const volatile void *flashPtr) {
  return read_flash_value<T>((uint32_t)flashPtr);
}

template<typename T>
const volatile T *read_flash_ptr(uint32_t flashAddr) {
  if (!validFAddr(flashAddr))
    return nullptr;
  return reinterpret_cast<T*>(flashAddr);
}
template<typename T>
const volatile T *read_flash_ptr(const volatile void *flashPtr) {
  return read_flash_ptr<T>((uint32_t)flashPtr);
}

/* TO DO
template<typename T>
int write_flash_value(uint32_t flashAddr, T value) {
  if (!validFAddr(flashAddr))
    return 0;
  (const volatile void*)flashAddr = ()
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SMART EEPROM FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum SEEPROM_STATUS {
  SEEPROM_INIT,
  SEEPROM_BUSY,
  SEEPROM_PENDING,
  SEEPROM_FULL
};

struct {
  bool autoRealloc = true;
  bool pageBuffer = false;
  bool locked = false;
  bool bypassChecks = true;
}seeprom_config;

bool init_seeprom(int minBytes, bool restartNow = true); // NEEDS CHECK

bool update_seeprom_config(); // DONE

bool exit_seeprom(bool restartNow = true); 


int write_seeprom_data(uintptr_t seeAddr, void *data, size_t byteCount, 
  bool blocking = false);
int write_seeprom_data(const volatile void *seePtr, void *data, size_t byteCount,
  bool blocking = false);

int read_seeprom_data(uintptr_t seeAddr, void *dest, size_t byteCount, 
  bool blocking = false);
int write_seeprom_data(const volatile void *seePtr, void *data, size_t byteCount,
  bool blocking = false);

bool switch_seeprom_sector(); // DONE

bool realloc_seeprom(); // DONE

bool flush_seeprom_buffer(); // DONE

bool clear_seeprom_buffer(); //DONE


int get_seeprom_size(); // DONE

bool get_seeprom_status(); // DONE

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PROG FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

void prog_restart();

void NOCALL_prog_assert(bool statement, const int line, const char *func, const char *file);
#define prog_assert(statement) NOCALL_prog_assert(statement, __LINE__, __FUNCTION__, __FILE__);

void NOCALL_prog_deny(bool statement, const int line, const char *func, const char *file);
#define prog_deny(statement) NOCALL_prog_deny(statement, __LINE__, __FUNCTION__, __FILE__);

