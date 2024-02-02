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

#define FLASH_REGION_INDICIES 8

struct {
  bool lowPowerOnSleep = false;
  bool lowPowerMode = false;
  bool writePage = false;
  bool boundAddr = true;
  void (*flash_error_interrupt)(FLASH_ERROR) = nullptr;
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
  const size_t bytes);

FLASH_ERROR erase_flash(const unsigned int flashIndex, const size_t indexCount 
  = FLASH_REGION_INDICIES);

int get_flashAddr_page(uintptr_t flashAddr);

int get_flashAddr_block(uintptr_t flashAddr);

int get_flashAddr_region(uintptr_t flashAddr);

FLASH_ERROR get_flash_error(); 

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
  bool checkAddr = true;
}seeprom_config;

bool init_seeprom(int minBytes, bool restartNow = true); // NEEDS CHECK

bool update_seeprom_config(); // DONE

bool exit_seeprom(bool restartNow = true); 

int write_seeprom_data(uintptr_t seeAddr, void *data, size_t byteCount, 
  bool blocking = false);

int read_seeprom_data(uintptr_t seeAddr, void *dest, size_t byteCount, 
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

