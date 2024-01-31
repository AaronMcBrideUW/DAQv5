
#include "SYS.h"

//// MASK DEFS ////
#define pmask (1 << pin.number)
#define pgroup PORT->Group[pin.group]

//// FLASH DEFS ////
#define RAM_START           (HSRAM_ADDR + 4)
#define RAM_END             (HSRAM_ADDR + HSRAM_SIZE)
#define BKUP_RAM_START      BKUPRAM_ADDR
#define BKUP_RAM_END        (BKUPRAM_ADDR + BKUPRAM_SIZE)
#define FLASH_START         ((FLASH_PAGE_COUNT * FLASH_PAGE_SIZE) / 4)
#define FLASH_END           FLASH_ADDR
#define FLASH_REGION_SIZE   ((FLASH_SIZE / 32) * 1000)
#define FLASH_BLOCK_SIZE    NVMCTRL_BLOCK_SIZE
#define FLASH_PAGE_SIZE     (2 << (NVMCTRL->PARAM.bit.PSZ + 3))
#define FLASH_QUAD_SIZE     (__SIZEOF_SHORT__ * 4)
#define FLASH_DATA_SIZE     __SIZEOF_POINTER__
#define FLASH_PAGE_COUNT    (NVMCTRL->PARAM.bit.NVMP)
#define FLASH_MAX_ALIGN     FLASH_QUAD_SIZE
#define FLASH_BLOCK_COUNT   (FLASH_PAGE_COUNT / FLASH_BLOCK_SIZE)
#define _FRDY_              while(!NVMCTRL->STATUS.bit.READY);
#define _FDONE_             while(!NVMCTRL->INTFLAG.bit.DONE); \
                            NVMCTRL->INTFLAG.bit.DONE = 1;

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

bool drive_pin(int pinID, int pinState, bool pullPin) {
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

static inline bool validFAddr(uint32_t addr) {
  return addr >= FLASH_ADDR && addr <= FLASH_END && !get_flash_lock(addr);
}

static inline bool validRAddr(uint32_t addr) {
  return ((addr >= RAM_START && addr <= RAM_END) 
    || (addr >= BKUP_RAM_START && addr <= BKUP_RAM_END)); 
}

int write_flash_data(uint32_t flashAddr, uint32_t dataAddr, int dataCount, 
  int dataAlignment, bool writePage) {

  if (!validFAddr(flashAddr) 
    ||!validRAddr(dataAddr) 
    ||dataCount <= 0 
    ||dataAlignment <= 0 
    ||dataAlignment > FLASH_MAX_ALIGN
    ||NVMCTRL->STATUS.bit.SUSP)
    return 0;


  volatile uint32_t *flashMem = (volatile uint32_t*)flashAddr;  
  int bytesWritten = 0;
  bool commitPending = false;

  _FRDY_
  NVMCTRL->INTFLAG.bit.DONE = 1;
  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;

  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY 
    | NVMCTRL_CTRLB_CMD_PBC;
  _FDONE_

  auto commitFlash = [&](void) -> void {
    _FRDY_
    NVMCTRL->CTRLB.reg |= 
        NVMCTRL_CTRLB_CMDEX_KEY
      | ((writePage ? NVMCTRL_CTRLB_CMD_WP_Val : NVMCTRL_CTRLB_CMD_WQW_Val) 
          << NVMCTRL_CTRLB_CMD_Pos);
    _FDONE_
    _FRDY_
    NVMCTRL->CTRLB.reg = 
        NVMCTRL_CTRLB_CMDEX_KEY 
      | NVMCTRL_CTRLB_CMD_PBC;
    _FDONE_
    bytesWritten += FLASH_QUAD_SIZE;
    commitPending = false;
  };

  if (dataAlignment % FLASH_DATA_SIZE != 0) { // Regular mode
    uint8_t *data8 = reinterpret_cast<uint8_t*>(dataAddr);
    uint32_t byteBuff = 0;
  
    for (int i = 0; i < dataCount; i++) {

      for (int j = 0; j < dataAlignment; j++) {
        byteBuff |= (data8[(i * dataAlignment) + j] << ((j % FLASH_DATA_SIZE) * 8));
        commitPending = true;

        if (j % FLASH_DATA_SIZE == FLASH_DATA_SIZE - 1 || j == dataAlignment - 1) {
          *flashMem = byteBuff;
          flashMem++;
          byteBuff = 0;
        }
      }
      commitFlash();
    }
  } else { // Fast mode
    uint32_t *data32 = reinterpret_cast<uint32_t*>(dataAddr); 
    const int commitIndex = FLASH_QUAD_SIZE / FLASH_DATA_SIZE; 

    for (int i = 0; i < dataCount * (dataAlignment / FLASH_DATA_SIZE); i++) {
      flashMem[i] = data32[i];
      commitPending = true;

      if (i % commitIndex == commitIndex - 1) 
        commitFlash();
    }
  }
  if (commitPending) 
    commitFlash();
  return bytesWritten;
}


int read_flash_data(uint32_t flashAddr, uint32_t destAddr, int copyCount, 
  int dataAlignment) {

  if (!validFAddr(flashAddr) || !validRAddr(destAddr) || copyCount <= 0 
    || dataAlignment <= 0 || dataAlignment > FLASH_MAX_ALIGN)
    return 0;

  int writtenBytes = 0;
  if (dataAlignment % FLASH_DATA_SIZE == 0) {
    copyCount *= dataAlignment;

    if (flashAddr + copyCount > FLASH_END)
      return 0;

    NVMCTRL->CTRLA.bit.SUSPEN = 1;
    memcpy((void*)destAddr, (const void*)flashAddr, copyCount);

    NVMCTRL->CTRLA.bit.SUSPEN = 0;
    writtenBytes = copyCount;

  } else {
    const int flashIter = (dataAlignment + FLASH_DATA_SIZE - 1) / FLASH_DATA_SIZE;
    if (flashAddr + flashIter * copyCount > FLASH_END)
      return 0;

    writtenBytes = copyCount;
    NVMCTRL->CTRLA.bit.SUSPEN = 1;

    for (int i = 0; i < copyCount; i++) {
      memcpy((void*)destAddr, (const void*)(flashAddr + i * flashIter), dataAlignment); /// CHECK THIS
    }
    NVMCTRL->CTRLA.bit.SUSPEN = 0;
  }
  return writtenBytes;
};

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

FLASH_ERROR get_flash_error() {
  if (NVMCTRL->INTFLAG.bit.NVME) {
    NVMCTRL->INTFLAG.bit.NVME = 1;

    if (NVMCTRL->INTFLAG.bit.ADDRE) {
      NVMCTRL->INTFLAG.bit.ADDRE = 1;
      return FLASH_ERROR_ADDR;

    } else if (NVMCTRL->INTFLAG.bit.PROGE) {
      NVMCTRL->INTFLAG.bit.PROGE = 1;
      return FLASH_ERROR_PROGRAM;

    } else if (NVMCTRL->INTFLAG.bit.LOCKE) {
      NVMCTRL->INTFLAG.bit.LOCKE = 1;
      return FLASH_ERROR_LOCK;
    } else if (NVMCTRL->INTFLAG.bit.ECCDE) {

    }
  }
  return FLASH_ERROR_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> FLASH VOID OVERLOADS
///////////////////////////////////////////////////////////////////////////////////////////////////

int write_flash_data(const volatile void *flashPtr, const volatile void *dataPtr, 
  int writeCount, int dataAlignment, bool writePage) {
  return write_flash_data((uint32_t)flashPtr, dataPtr, writeCount, dataAlignment, 
    writePage);
}

int read_flash_data(const volatile void *flashPtr, void *destPtr, int copyCount,   
  int dataAlignment) {
  return read_flash_data((uint32_t)flashPtr, destPtr, copyCount, dataAlignment);
}

bool erase_flash(const volatile void *flashPtr) {
  return erase_flash(get_flash_region(flashPtr));
}

bool get_flash_lock(const volatile void *flashPtr) {
  return get_flash_lock((uint32_t)flashPtr);
}

bool set_flash_lock(const volatile void *flashPtr, bool locked) {
  return set_flash_lock((uint32_t)flashPtr, locked);
}

int get_flash_page(const volatile void *flashPtr) {
  return get_flash_page((uint32_t)flashPtr);
}

int get_flash_block(const volatile void *flashPtr) {
  return get_flash_block((uint32_t)flashPtr);
}

int get_flash_page(const volatile void *flashPtr) {
  return get_flash_page(reinterpret_cast<uint32_t>(flashPtr));
}

int get_flash_region(const volatile void *flashPtr) {
  return get_flash_region((uint32_t)flashPtr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SEEPROM
///////////////////////////////////////////////////////////////////////////////////////////////////

static inline bool validSEEAddr(uint32_t seeAddr) {
  return (seeAddr > SEEPROM_ADDR && seeAddr < get_seeprom_size());
}

bool init_seeprom(int minBytes, bool restartNow) { 
  uint8_t blockCount = 0;
  uint16_t pageCount = 0;

  if (get_seeprom_init) {
    return false;
  }
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
    | NVMCTRL_CTRLB_CMD_EP;
  _FDONE_

  UP[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 
      (NVMCTRL_FUSES_SEESBLK(blockCount) 
    | NVMCTRL_FUSES_SEEPSZ((uint8_t)(log2(pageCount) - 2)));

  for (int i = 0; i < FLASH_USER_PAGE_SIZE; i += FLASH_QUAD_SIZE * 8) {
    memcpy((void*)(NVMCTRL_USER + i), UP + i, FLASH_QUAD_SIZE * 8);
    _FRDY_
    NVMCTRL->ADDR.bit.ADDR = (uint32_t)(NVMCTRL_USER + i);
    NVMCTRL->CTRLB.reg = 
        NVMCTRL_CTRLB_CMDEX_KEY
      | NVMCTRL_CTRLB_CMD_WQW; 
    _FDONE_
  }
  if (restartNow) 
    prog_restart();
  return true;
}

bool write_seeprom(uint32_t seeAddr, void *data, int byteCount, bool blocking) {
  if (!validSEEAddr(seeAddr) 
    ||NVMCTRL->SEESTAT.bit.LOCK
    ||NVMCTRL->INTFLAG.bit.SEESFULL
    ||!blocking && NVMCTRL->SEESTAT.bit.BUSY
    ||byteCount  >= SEE_MAX_WRITE
    ||byteCount  <= 0
    ||!data)
    return false;

  while(NVMCTRL->SEESTAT.bit.BUSY);
  memcpy((void*)seeAddr, data, byteCount);

  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);

  if (NVMCTRL->INTFLAG.bit.SEESOVF) {         // FIGURE OUT ERROR HANDLING...
    NVMCTRL->INTFLAG.bit.SEESOVF = 0;
    return false;
  }
  return true;
}

bool read_seeprom_data(uint32_t seeAddr, void *data, int byteCount, bool blocking) {
  if (!validSEEAddr(seeAddr)
    ||!blocking && NVMCTRL->SEESTAT.bit.BUSY
    ||byteCount <= 0
    ||byteCount >= SEE_MAX_READ
    ||!data)
    return false;

  while(NVMCTRL->SEESTAT.bit.BUSY);
  memcpy(data, (const void*)seeAddr, byteCount);
  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);
  return true;
}

int get_seeprom_size() {
  static int totalSize = 0;
  if (get_seeprom_state == SEEPROM_NULL) {
    return -1;
  } else if (!totalSize) {
    for (int i = 0; i < sizeof(SEE_REF) / sizeof(SEE_REF[0]); i++) {
      if (SEE_REF[i][1] == NVMCTRL->SEESTAT.bit.SBLK
        && SEE_REF[i][2] == NVMCTRL->SEESTAT.bit.PSZ) {
        totalSize = SEE_REF[i][0];
      }
    }
  }
  return totalSize;
}

bool set_seeprom_lock(bool locked) {
  if (get_seeprom_state() == SEEPROM_NULL)
    return false;

  NVMCTRL->CTRLB.reg |=
      NVMCTRL_CTRLB_CMDEX_KEY
    | ((locked ? NVMCTRL_CTRLB_CMD_USEE_Val : NVMCTRL_CTRLB_CMD_LSEE_Val)
        << NVMCTRL_CTRLB_CMD_Pos);
        
  return true;
}

bool get_seeprom_locked() {
  return NVMCTRL->SEESTAT.bit.LOCK;
} 

SEEPROM_STATE get_seeprom_state() {
  if (NVMCTRL->SEESTAT.bit.SBLK && NVMCTRL->SEESTAT.bit.PSZ) {
    return SEEPROM_NULL;
  } else if (NVMCTRL->INTFLAG.bit.SEESOVF) {
    return SEEPROM_OVERFLOW;
  } else if (NVMCTRL->INTFLAG.bit.SEESFULL) {
    return SEEPROM_FULL;
  } else if (NVMCTRL->SEESTAT.bit.BUSY) {
    return SEEPROM_BUSY;
  } else if (NVMCTRL->SEESTAT.bit.SBLK && NVMCTRL->SEESTAT.bit.PSZ) {
    return SEEPROM_INIT;
  } else {
    return SEEPROM_NULL;
  }
}









