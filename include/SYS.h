///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE -> SYS
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "inttypes.h"
#include "math.h"
#include "Board.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

enum PIN_MODE {
  PIN_DISABLED = 14,
  PIN_GPIO     = 15,
  PIN_PERIPH_A = 0, 
  PIN_PERIPH_B = 1,
  PIN_PERIPH_C = 2, 
  PIN_PERIPH_D = 3, 
  PIN_PERIPH_E = 4,
  PIN_PERIPH_F = 5, 
  PIN_PERIPH_G = 6, 
  PIN_PERIPH_H = 7, 
  PIN_PERIPH_J = 8, 
  PIN_PERIPH_I = 9,
  PIN_PERIPH_K = 10, 
  PIN_PERIPH_L = 11, 
  PIN_PERIPH_M = 12, 
  PIN_PERIPH_N = 13
};

bool set_pin(int pinID, PIN_MODE mode, bool strongDrive = true);

bool pull_pin(int pinID, int pullType = 1);

bool drive_pin(int pinID, bool high);

int read_pin(int pinID);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PROG FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

void NOCALL_prog_assert(bool statement, const int line, const char *func, const char *file);
#define prog_assert(statement) NOCALL_prog_assert(statement, __LINE__, __FUNCTION__, __FILE__);

void NOCALL_prog_deny(bool statement, const int line, const char *func, const char *file);
#define prog_deny(statement) NOCALL_prog_deny(statement, __LINE__, __FUNCTION__, __FILE__);

void prog_restart();
