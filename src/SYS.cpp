
#include "SYS.h"

//// MASK DEFS ////
#define pmask (1 << pin.number)
#define pgroup PORT->Group[pin.group]

//// FLASH DEFS ////
/*
#define FLASH_START_           (FLASH_ADDR)
#define FLASH_END_             (FLASH_START_ - FLASH_PAGE_COUNT_ * FLASH_PAGE_SIZE_)
#define FLASH_LENGTH_          ((FLASH_END_ - FLASH_START_) / __SIZEOF_POINTER__)

#define FLASH_DATA_SIZE_       (__SIZEOF_POINTER__)
#define FLASH_QUAD_SIZE_       (__SIZEOF_SHORT__ * 4)
#define FLASH_PAGE_SIZE_       (FLASH_PAGE_SIZE)
#define FLASH_BLOCK_SIZE_      (NVMCTRL_BLOCK_SIZE)
#define FLASH_REGION_SIZE_     ((FLASH_PAGE_SIZE * FLASH_NB_OF_PAGES * 1000) / 32)

#define FLASH_REGION_COUNT_    (FLASH_TOTAL_SIZE_ / FLASH_REGION_SIZE_)
#define FLASH_BLOCK_COUNT_     (FLASH_TOTAL_SIZE_ / FLASH_BLOCK_SIZE_)
#define FLASH_QUAD_COUNT_      (FLASH_TOTAL_SIZE_ / FLASH_QUAD_SIZE_)
#define FLASH_PAGE_COUNT_      (NVMCTRL->PARAM.bit.NVMP) 
*/


//// SEEPROM DEFS ////
#define SEE_DEFAULT_BLOCKS 2
#define SEE_DEFAULT_PAGES 64
#define SEE_MAX_WRITE NVMCTRL_PAGE_SIZE
#define SEE_MAX_READ NVMCTRL_PAGE_SIZE

static uint8_t UP[FLASH_USER_PAGE_SIZE] = { 0 }; // User Page
static const uint32_t SEE_REF[8][3] {
  {512, 1, 4},
  {1024, 1, 8},
  {2048, 1, 16},
  {4096, 1, 32},
  {8192, 2, 64},
  {16384, 3, 128},
  {32768, 5, 256},
  {65536, 10, 512}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN LOCAL
///////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t PIN_PULLSTATE[BOARD_PIN_COUNT] = { 0 };

static bool validPin(const int &pinID) {
  return pinID >= 0 && pinID < BOARD_PIN_COUNT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool reset_pin(int pinID) {
  if (!validPin(pinID)) 
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];

  pgroup.DIRCLR.reg |= pmask;
  pgroup.OUTCLR.reg |= pmask;
  pgroup.PINCFG[pin.number].reg &= PORT_PINCFG_RESETVALUE;
  pgroup.PMUX[pin.number].reg &= PORT_PMUX_RESETVALUE;
  return true;
}

bool attach_pin(int pinID, int periphID) {
  if (!validPin(pinID))
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID]; 

  if (periphID >= 0) {
    pgroup.PMUX[pin.number].reg = (pin.number % 2) 
      ? PORT_PMUX_PMUXO((uint8_t)periphID) : PORT_PMUX_PMUXE((uint8_t)periphID);
    pgroup.PINCFG[pin.number].bit.PMUXEN = 1;
  } else {
    pgroup.PINCFG[pin.number].bit.PMUXEN = 0;      
  }
  return true;
}

bool set_pin(int pinID, int pinState, bool pullPin) {
  if (!validPin(pinID))
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];
  pgroup.PINCFG[pin.number].bit.PULLEN = (uint8_t)pullPin;
  pgroup.PINCFG[pin.number].bit.INEN = 0;
  
  if (pinState) {
    pgroup.PINCFG[pin.number].bit.DRVSTR = (uint8_t)(abs(pinState) >= 2);
    if (pinState >= 1) {
      pgroup.OUTSET.reg |= pmask;
    } else {
      pgroup.OUTCLR.reg |= pmask;
    }
    if (pullPin) {
      pgroup.DIRCLR.reg |= pmask;
    } else {
      pgroup.DIRSET.reg |= pmask; 
    }
  } else {
    pgroup.DIRCLR.reg |= pmask;
    pgroup.OUTCLR.reg |= pmask;
  }
  return true;
}

int read_pin(int pinID) {
  if (!validPin(pinID)) 
    return -1;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];
  pgroup.PINCFG[pin.number].bit.INEN = 1;
  return (int)(pgroup.IN.reg & pmask);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> FLASH FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef uint32_t flashMemAl;
typedef uint64_t flashQuadAl;

static volatile flashMemAl *flash_get_ptr(unsigned int flashIndex) {
  if (flashIndex < FLASH_ADDR / sizeof(flashQuadAl)) 
    return nullptr;
  return (volatile flashMemAl*)(FLASH_ADDR + flashIndex * sizeof(flashQuadAl));
}

static bool flash_check_addr(const volatile flashMemAl *ptr, const size_t bytes) {
  return FLASH_ADDR + NVMCTRL->PARAM.bit.NVMP * (FLASH_PAGE_SIZE / sizeof(flashMemAl))
    <= (uintptr_t)ptr + ceil((double)bytes / sizeof(flashQuadAl)) 
      * (sizeof(flashQuadAl) / sizeof(flashMemAl));
} 

static FLASH_ERROR flash_handle_errors(unsigned int *flashIndex = nullptr) {

};

static void flash_cmd(unsigned int cmdVal) {
  while(!NVMCTRL->STATUS.bit.READY);
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | (uint8_t)cmdVal << NVMCTRL_CTRLB_CMD_Pos; 
  while(!NVMCTRL->STATUS.bit.READY);
}

FLASH_ERROR flash_update_config() {
  NVMCTRL->CTRLA.bit.PRM = flash_config.lowPowerOnSleep 
    ? NVMCTRL_CTRLA_PRM_FULLAUTO_Val : NVMCTRL_CTRLA_PRM_MANUAL_Val;

  if (flash_config.flash_error_interrupt == nullptr) {
    NVMCTRL->INTENCLR.bit.NVME;
  } else {
    NVMCTRL->INTENSET.bit.NVME;
  }
}

FLASH_ERROR flash_write_data(unsigned int &flashIndex, const void *data, 
  const size_t bytes) {

  volatile flashMemAl *flashPtr = flash_get_ptr(flashIndex);
  const uint8_t *byteData = (const uint8_t*)data;
  const flashMemAl *alignedData = (const flashMemAl*)data;
  const unsigned int flashWriteSize = flash_config.writePage ? FLASH_PAGE_SIZE / 8 
    : sizeof(flashQuadAl);

  if (!alignedData || !flashPtr)
    return FLASH_ERROR_PARAM;
  else if (flash_config.boundAddr && !flash_check_addr(flashPtr, bytes))
    return FLASH_ERROR_ADDR;

  NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_WMODE_MAN;
  if (NVMCTRL->STATUS.bit.LOAD) {
    flash_cmd(NVMCTRL_CTRLB_CMD_PBC_Val);
  }
  for (int i = 0; i <= bytes / sizeof(flashMemAl); i++) {
    if (bytes - i * sizeof(flashMemAl) > sizeof(flashMemAl)) {
      flashPtr[i] = alignedData[i];
    } else {
      flashMemAl alignBuffer = 0;
      for (int j = 0; j < i * sizeof(flashMemAl); j++) {
        alignBuffer |= (byteData[j + i * sizeof(flashMemAl)] << j);
      }
      goto forceWrite;
    }
    if (i % flashWriteSize == flashWriteSize - 1) {
      forceWrite:
      flash_cmd(NVMCTRL_CTRLB_CMD_WQW_Val);
    }
  }
  return flash_handle_errors(&flashIndex);
}


FLASH_ERROR flash_read_data(unsigned int &flashIndex, void *dest, 
  const size_t bytes) {
  if (!dest)
    return FLASH_ERROR_ADDR;

  const volatile flashMemAl *flashPtr = flash_get_ptr(flashIndex);
  if (flash_config.boundAddr && !flash_check_addr(flashPtr, bytes))
    return FLASH_ERROR_ADDR;

  memcpy(dest, (const flashMemAl*)flashPtr, bytes);
  return flash_handle_errors();
}

FLASH_ERROR erase_flash(const unsigned int flashIndex, const size_t indexCount) {
  
  const flashMemAl *flashPtr


}



/*
bool erase_flash(int blockIndex) {
  uint32_t const blockAddr = FLASH_START + blockIndex * FLASH_BLOCK_SIZE;
  
  if (blockAddr > FLASH_END || blockAddr < FLASH_START 
    || NVMCTRL->STATUS.bit.SUSP || get_flash_lock(blockAddr))
    return false;

  _FRDY_
  NVMCTRL->ADDR.reg = FLASH_START + blockIndex * FLASH_BLOCK_SIZE;
  NVMCTRL->CTRLB.reg |= 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_EB;
  _FDONE_
  return true;
}

bool get_flash_lock(int regionIndex) {
  if (regionIndex < 0 || regionIndex > FLASH_BLOCK_COUNT)
    return false;
  return (NVMCTRL->RUNLOCK.reg & (1 << regionIndex));
}

bool set_flash_lock(int regionIndex, bool locked) {
  if (regionIndex < 0 || regionIndex > FLASH_BLOCK_COUNT)
    return false;
  _FRDY_
  NVMCTRL->INTFLAG.bit.PROGE = 1;
  NVMCTRL->CTRLB.reg |=
      NVMCTRL_CTRLB_CMDEX_KEY
    | ((locked ? NVMCTRL_CTRLB_CMD_LR_Val : NVMCTRL_CTRLB_CMD_UR) 
        << NVMCTRL_CTRLB_CMD_Pos);
  _FDONE_

  if (NVMCTRL->INTFLAG.bit.PROGE) {
    NVMCTRL->INTFLAG.bit.PROGE = 1;
    return !locked;
  }
  return true;
}

int get_flash_page(uint32_t flashAddr) {
  if (!validFAddr(flashAddr))
    return 0;
  return ((flashAddr - FLASH_START) / FLASH_PAGE_SIZE);
}     

int get_flash_block(uint32_t flashAddr) {
  if (!validFAddr(flashAddr))
    return 0;
  return ((flashAddr - FLASH_START) / FLASH_BLOCK_SIZE);
}

int get_flash_region(uint32_t flashAddr) {
  if (!validFAddr(flashAddr)) 
    return false;
  return ((flashAddr - FLASH_START) / FLASH_REGION_SIZE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SEEPROM
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////// HELPER FUNCTIONS ///////////////////

static inline bool validSEEAddr(uint32_t seeAddr) {
  return (seeAddr > SEEPROM_ADDR && seeAddr < get_seeprom_size());
}

static inline bool validSEEStatus() {
  return get_seeprom_status(SEEPROM_INIT) 
    && !get_seeprom_status(SEEPROM_BUSY)
    && !NVMCTRL->SEESTAT.bit.LOCK;
}

///////////////// CORE FUNCTIONS ///////////////////

bool init_seeprom(int minBytes, bool restartNow) { 
  uint8_t blockCount = 0;
  uint16_t pageCount = 0;

  if (!NVMCTRL->PARAM.bit.SEE 
    || get_seeprom_status(SEEPROM_INIT))
    return false;

  for (int16_t i = 0; i < (sizeof(SEE_REF) / sizeof(SEE_REF[0])); i++) {
    if (SEE_REF[i][0] >= minBytes) {
      blockCount = SEE_REF[i][1];
      pageCount = SEE_REF[i][2];
      break;
    }
  }
  if (!blockCount || pageCount) {
    blockCount = SEE_DEFAULT_BLOCKS;
    pageCount = SEE_DEFAULT_PAGES;
  } 
  memcpy(UP, (const void*)NVMCTRL_USER, FLASH_USER_PAGE_SIZE);
  _FRDY_
  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;
  NVMCTRL->ADDR.bit.ADDR = NVMCTRL_USER;

  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_EP;
  _FDONE_
  _FRDY_
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_PBC;
  _FDONE_

  UP[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 
      (NVMCTRL_FUSES_SEESBLK(blockCount) 
    | NVMCTRL_FUSES_SEEPSZ((uint8_t)(log2(pageCount) - 2)));

  for (int i = 0; i < FLASH_USER_PAGE_SIZE; i += FLASH_QUAD_SIZE * 8) {
    memcpy((void*)(NVMCTRL_USER + i), UP + i, FLASH_QUAD_SIZE * 8);
    _FRDY_
    NVMCTRL->CTRLB.reg = 
        NVMCTRL_CTRLB_CMDEX_KEY
      | NVMCTRL_CTRLB_CMD_WQW; 
    _FDONE_
  }
  if (restartNow) 
    prog_restart();
  return true;
}

bool exit_seeprom(bool restartNow) {
  if (!get_seeprom_status(SEEPROM_INIT))
    return false;
  _FRDY_
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_LSEE;
  _FDONE_

  memcpy(UP, (const void*)NVMCTRL_USER, FLASH_USER_PAGE_SIZE);
  
  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;
  NVMCTRL->ADDR.bit.ADDR = NVMCTRL_USER;

  _FRDY_
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_EP;
  _FDONE_
  _FRDY_
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_PBC;
  _FDONE_

  UP[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 
      NVMCTRL_FUSES_SEESBLK(0)
    | NVMCTRL_FUSES_SEEPSZ(0);
  
  for (int i = 0; i < FLASH_USER_PAGE_SIZE; i += FLASH_QUAD_SIZE) {
    memcpy((void*)(NVMCTRL_USER + i), UP + i, FLASH_QUAD_SIZE * 8);
    _FRDY_
    NVMCTRL->CTRLB.reg = 
        NVMCTRL_CTRLB_CMDEX_KEY
      | NVMCTRL_CTRLB_CMD_WQW; 
    _FDONE_
  }
  if (restartNow)
    prog_restart();
  return true;
}

bool update_seeprom_config() { 
  if (!get_seeprom_status(SEEPROM_INIT)
    ||get_seeprom_status(SEEPROM_BUSY))
    return false;

  if (seeprom_config.locked == 0 || seeprom_config.locked == 1) {
    _FRDY_
    NVMCTRL->CTRLB.reg = 
        NVMCTRL_CTRLB_CMDEX_KEY
      | ((seeprom_config.locked ? NVMCTRL_CTRLB_CMD_LSEE_Val 
          : NVMCTRL_CTRLB_CMD_USEE_Val) << NVMCTRL_CTRLB_CMD_Pos);
    _FDONE_
  }
  if (seeprom_config.pageBuffer == 0 || seeprom_config.pageBuffer == 1) {
    NVMCTRL->SEECFG.bit.APRDIS = (uint8_t)seeprom_config.pageBuffer;
  }
  if (seeprom_config.autoRealloc == 0 || seeprom_config.autoRealloc == 1) {
    NVMCTRL->SEECFG.bit.WMODE = (uint8_t)seeprom_config.autoRealloc;
  }
  return true;
}

///////////////// WRITE/READ FUNCTIONS ///////////////////

int write_seeprom(uintptr_t seeAddr, void *data, size_t byteCount, bool blocking) {
  if (!seeprom_config.bypassChecks) {
    if (!validSEEAddr(seeAddr) 
      ||!validSEEStatus()
      ||get_seeprom_status(SEEPROM_FULL)
      ||byteCount >= SEE_MAX_WRITE
      ||!data)
      return 0;
  }
  memcpy((void*)seeAddr, data, byteCount);
  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);

  if (NVMCTRL->INTFLAG.bit.SEESOVF) {
    NVMCTRL->INTFLAG.bit.SEESOVF = 1;
    return -1;
  }
  return byteCount;
}
int write_seeprom_data(const volatile void *seePtr, void *data, size_t byteCount,
  bool blocking = false) {
  uintptr_t seeAddr = (uintptr_t)seePtr;
  return write_seeprom_data(seeAddr, data, byteCount, blocking);
}


int read_seeprom_data(uintptr_t seeAddr, void *data, size_t byteCount, bool blocking) {
  if (!seeprom_config.bypassChecks) {
    if (!validSEEAddr(seeAddr)
      ||!get_seeprom_status(SEEPROM_INIT)
      ||get_seeprom_status(SEEPROM_BUSY)
      ||byteCount >= SEE_MAX_READ
      ||!data)
      return 0;
  }
  memcpy(data, (const void*)seeAddr, byteCount);
  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);
  return byteCount;
}
int read_seeprom_data(const volatile void *seePtr, void *data, size_t byteCount,
  bool blocking = false) {
  uintptr_t seeAddr = (uintptr_t)seePtr;
  return read_seeprom_data(seeAddr, data, byteCount, blocking);   
}


///////////////// CMD FUNCTIONS ///////////////////

bool switch_seeprom_sector() {
  if (!validSEEStatus())
    return false;

  uint8_t secVal = NVMCTRL->SEESTAT.bit.ASEES ? NVMCTRL_CTRLB_CMD_ASEES0_Val 
    : NVMCTRL_CTRLB_CMD_ASEES1_Val;
  _FRDY_
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | (secVal << NVMCTRL_CTRLB_CMD_Pos);
  _FDONE_
  return true;
}

bool realloc_seeprom() {
  if (!validSEEStatus())
    return false;
  _FRDY_
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_SEERALOC;
  _FDONE_
  return true;
} 

bool flush_seeprom_buffer() {
  if (!validSEEStatus()
    || NVMCTRL->SEECFG.bit.WMODE != NVMCTRL_SEECFG_WMODE_BUFFERED_Val)
    return false;
  _FRDY_
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_SEEFLUSH;
  _FDONE_
  return true;
}

bool clear_seeprom_buffer() {
  if (!validSEEStatus()) 
    return false;
  _FRDY_
  NVMCTRL->CTRLB.reg =
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_PBC;
  _FDONE_
  return true;
}

///////////////// GET FUNCTIONS ///////////////////

int get_seeprom_size() {
  static int quickRef = -1;
  if (!get_seeprom_status(SEEPROM_INIT))
    return -1;

  if (quickRef == -1) {
    for (int i = 0; i < sizeof(SEE_REF) / sizeof(SEE_REF[0]); i++) {
      if (SEE_REF[i][1] == NVMCTRL->SEESTAT.bit.SBLK
        && SEE_REF[i][2] == NVMCTRL->SEESTAT.bit.PSZ) {
        quickRef = SEE_REF[i][0];
      }
    }
  }
  return quickRef;
}

bool get_seeprom_status(SEEPROM_STATUS status) {
  if (get_seeprom_size <= 0)
    return false;

  switch(status) {
    case SEEPROM_INIT: 
      return true;
    case SEEPROM_BUSY: 
      return NVMCTRL->SEESTAT.bit.BUSY || (NVMCTRL->INTFLAG.bit.SEEWRC);
    case SEEPROM_PENDING:
      return NVMCTRL->SEESTAT.bit.LOAD;
    case SEEPROM_FULL:
      return NVMCTRL->INTFLAG.bit.SEESFULL;
  }
  return false;
}

*/





