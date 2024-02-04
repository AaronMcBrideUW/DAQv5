
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

/// @internal Typedefs for nvm
typedef uint32_t fmem_t;                  // Type of flash memory
typedef __uint128_t findex_t;             // Type of flash index

/// @internal Macros for nvm functions
#define F_I2M(_index_)          ((_index_) * (sizeof(findex_t) / sizeof(fmem_t)))
#define F_M2I(_mem_)            ((_mem_) * (sizeof(fmem_t) / sizeof(findex_t)))
#define F_B2M(_byte_)           ((_byte_) == 0 ? 0 : ((_byte_) / sizeof(fmem_t)))
#define F_B2I(_byte_)           ((_byte_) == 0 ? 0 : ((_byte_) / sizeof(findex_t)))
#define ALIGN_DOWN(_val_, _al_) ((_val_) == 0 ? 0 : ((_val_) / (_al_)) * (_al_))
#define ALIGN_UP(_val_, _al_)   ((_val_) == 0 ? 0 : (((_val_) + (_al_) - 1) / (_al_)) * (_al_))
#define F_REGION_COUNT 32

/// @internal Storage buffer for flash erase function  
static findex_t eraseBuffer[F_B2I(NVMCTRL_BLOCK_SIZE) * 2] = { 0 };

///////////////////////////////////////////////////////////////////////////////////////////////////

static inline uintptr_t f_get_addr(const unsigned int flashIndex) {
  return F_B2M(FLASH_ADDR) + F_I2M(flashIndex);
}

// Gets the address of a flash index
static inline uintptr_t f_index_addr(unsigned int index) {
  static const uintptr_t start = F_B2M(FLASH_ADDR);
  return start + F_I2M(index);
}

// Ensures index is within bounds of flash mem
static inline bool f_valid_index(unsigned int index) {
  return (flash_config.boundAddr && F_B2M(FLASH_ADDR + NVMCTRL->PARAM.bit.NVMP 
    * FLASH_PAGE_SIZE));
}
// Executes command in NVMC
static inline void f_cmd(uint8_t cmdVal) {
  while(!NVMCTRL->STATUS.bit.READY);
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | cmdVal << NVMCTRL_CTRLB_CMD_Pos; 
}

// Gets & clears any error interrupt flags
static inline FLASH_ERROR flash_get_error() {
  if (NVMCTRL->INTFLAG.bit.NVME) {
    decltype(NVMCTRL->INTFLAG.reg) flagReg = NVMCTRL->INTFLAG.reg;
    NVMCTRL->INTFLAG.bit.NVME = 1;

    if (flagReg & NVMCTRL_INTFLAG_LOCKE) {
      NVMCTRL->INTFLAG.bit.LOCKE = 1;
      return FLASH_ERROR_LOCK;

    } else if (flagReg & NVMCTRL_INTFLAG_PROGE) {
      NVMCTRL->INTFLAG.bit.PROGE = 1;
      return FLASH_ERROR_PROG;

    } else if (flagReg & NVMCTRL_INTFLAG_ADDRE) {
      NVMCTRL->INTFLAG.bit.ADDRE = 1;
      return FLASH_ERROR_ADDR;
    } 
  }
  return FLASH_ERROR_NONE;
}

void NVMCTRL_0_Handler(void) {
  if (NVMCTRL->INTFLAG.bit.NVME && flash_config.errorInterrupt) {
    flash_config.errorInterrupt(flash_get_error()); 
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////// 

// Syncs sys with config struct
FLASH_ERROR flash_update_config() {
  NVMCTRL->CTRLA.bit.PRM = flash_config.lowPowerOnSleep 
    ? NVMCTRL_CTRLA_PRM_FULLAUTO_Val : NVMCTRL_CTRLA_PRM_MANUAL_Val;

  f_cmd(flash_config.lowPowerMode ? NVMCTRL_CTRLB_CMD_SPRM_Val 
    : NVMCTRL_CTRLB_CMD_CPRM_Val);

  if (flash_config.errorInterrupt == nullptr) {
    NVMCTRL->INTENCLR.bit.NVME;
  } else {
    NVMCTRL->INTENSET.bit.NVME;
  }
}

// Writes data (quad-aligned)
FLASH_ERROR flash_write_data(unsigned int &flashIndex, const void *data, 
  const size_t bytes) {

  const fmem_t *alignedData = (const fmem_t*)data;
  volatile fmem_t *flashPtr = (volatile fmem_t*)(F_B2M(FLASH_ADDR) 
    + F_I2M(flashIndex));

  if (!alignedData || !flashPtr) {
    return FLASH_ERROR_PARAM;

  } else if (flash_config.boundAddr && !f_valid_index(flashIndex + F_B2I(bytes)) 
    || !f_valid_index(flashIndex)) {
    return FLASH_ERROR_ADDR;
  }

  NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_WMODE_MAN;  
  if (NVMCTRL->STATUS.bit.LOAD) {
    f_cmd(NVMCTRL_CTRLB_CMD_PBC_Val);
  }
  const unsigned int flashWriteSize = F_B2M(flash_config.writePage 
    ? FLASH_PAGE_SIZE : sizeof(findex_t));
  const unsigned int alignCount = F_I2M(ALIGN_DOWN(bytes, sizeof(fmem_t)));  

  // Write aligned data
  for (int i = 0; i < alignCount; i++) {      
    flashPtr[i] = alignedData[i];
    if (i % flashWriteSize == flashWriteSize - 1) {
      f_cmd(flash_config.writePage ? NVMCTRL_CTRLB_CMD_WP_Val 
        : NVMCTRL_CTRLB_CMD_WQW_Val);
    }    
  }
  // Write unaligned data (if applicable)
  if (bytes > alignCount * sizeof(fmem_t)) {   
    fmem_t alignBuffer = 0;
    const uint8_t *byteData = (const uint8_t*)data;

    for (int i = 0; i < bytes - alignCount * sizeof(fmem_t); i++) {
      alignBuffer |= (byteData[i + alignCount * sizeof(fmem_t)] << i);
    }
    flashPtr[alignCount] = alignBuffer;
  }
  if (NVMCTRL->STATUS.bit.LOAD) {
    f_cmd(flash_config.writePage ? NVMCTRL_CTRLB_CMD_WP_Val 
      : NVMCTRL_CTRLB_CMD_WQW_Val);
  }
  while(!NVMCTRL->STATUS.bit.READY);

  FLASH_ERROR error = flash_get_error();
  if (error == FLASH_ERROR_NONE) {
    flashIndex += F_M2I(alignCount);
  }
  return error;
}

FLASH_ERROR flash_read_data(unsigned int &flashIndex, void *dest, 
  const size_t bytes) {
  if (!dest || !f_valid_index(F_B2I(ALIGN_UP(flashIndex + bytes, sizeof(fmem_t)))))
    return FLASH_ERROR_ADDR;

  const void *flashPtr = (const void*)(F_I2M(flashIndex) 
    + F_B2M(FLASH_ADDR));
  
  memcpy(dest, flashPtr, bytes);
  while(!NVMCTRL->STATUS.bit.READY);

  FLASH_ERROR error = flash_get_error();
  if (error == FLASH_ERROR_NONE) {
    flashIndex += F_B2I(bytes);
  }
  return error;
}

FLASH_ERROR flash_erase(unsigned int flashIndex, size_t indexCount, 
  bool forceAligned) {

  const unsigned int alStart = ALIGN_DOWN(flashIndex, 
    F_B2I(NVMCTRL_BLOCK_SIZE));
  const unsigned int alEnd = ALIGN_UP(flashIndex + indexCount, 
    F_B2I(NVMCTRL_BLOCK_SIZE));

 if (flash_config.boundAddr && (!f_valid_index(alStart)
    || !f_valid_index(alEnd)))
      return FLASH_ERROR_ADDR; 

  if (!forceAligned) {
    memcpy(eraseBuffer, (const void*)(f_index_addr(alStart)), 
      (flashIndex - alStart) * sizeof(findex_t));

    memcpy(&eraseBuffer[F_B2I(NVMCTRL_BLOCK_SIZE)], (const void*)(f_index_addr  
      (flashIndex + indexCount)), (alEnd - flashIndex + indexCount) 
        * sizeof(findex_t));
  }
  for (int i = alStart; i < alEnd; i += F_B2I(NVMCTRL_BLOCK_SIZE)) {
    NVMCTRL->ADDR.bit.ADDR = FLASH_ADDR + i * NVMCTRL_BLOCK_SIZE;
    f_cmd(NVMCTRL_CTRLB_CMD_EB_Val);
  }
  if (!forceAligned) { 
    f_cmd(NVMCTRL_CTRLB_CMD_PBC_Val);
    while(!NVMCTRL->STATUS.bit.READY);
    volatile fmem_t* flashPtr = (volatile fmem_t*)f_index_addr(alStart);
    
    for (int i = F_I2M(alStart); i < F_I2M(flashIndex); i++) {
      flashPtr[i] = eraseBuffer[i - F_I2M(alStart)];

      if (i % F_B2M(sizeof(findex_t)) == F_B2M(sizeof(findex_t)) - 1) {
        f_cmd(NVMCTRL_CTRLB_CMD_WQW_Val);
      }
    } 
    for (int i = F_I2M(flashIndex + indexCount); i < F_I2M(alEnd); i++) {
      flashPtr[i] = eraseBuffer[i - F_I2M(flashIndex + indexCount) 
        + F_B2I(NVMCTRL_BLOCK_SIZE)];

      if (i % F_B2M(sizeof(findex_t)) == F_B2M(sizeof(findex_t)) - 1) {
        f_cmd(NVMCTRL_CTRLB_CMD_WQW_Val);
      }
    }
  }
  memset(eraseBuffer, 0, sizeof(eraseBuffer));
  while(!NVMCTRL->STATUS.bit.READY);
  return flash_get_error();
}

FLASH_ERROR flash_clear() {
  static const unsigned int blocks = ceil((double)(NVMCTRL->PARAM.bit.NVMP 
    * FLASH_PAGE_SIZE) / NVMCTRL_BLOCK_SIZE) * NVMCTRL_BLOCK_SIZE;
  
  for (int i = 0; i < blocks; i++) {
    NVMCTRL->ADDR.bit.ADDR = FLASH_ADDR + i * NVMCTRL_BLOCK_SIZE;
    f_cmd(NVMCTRL_CTRLB_CMD_EB_Val);
  }
  while(!NVMCTRL->STATUS.bit.READY);
  return flash_get_error();
}

bool flash_get_region_locked(unsigned int regionIndex) {
  return (NVMCTRL->RUNLOCK.reg & (1 << (regionIndex * F_B2M(FLASH_ADDR 
    + (FLASH_SIZE / 32)))));
}

bool flash_is_free(unsigned int flashIndex, size_t bytes) {
  uint alignedBytes = ALIGN_UP(bytes, sizeof(findex_t));
  uint8_t *flashPtr = (uint8_t*)f_get_addr(flashIndex);
  
  if (flash_config.boundAddr && !f_valid_index(flashIndex + alignedBytes)) 
    return false;
  return memcmp(flashPtr, flashPtr + 1, bytes - 1); 
}

int flash_find_free(unsigned int flashStartIndex, size_t bytes) {
  static const uintptr_t maxAddr = FLASH_ADDR + NVMCTRL->PARAM.bit.NVMP 
    * FLASH_PAGE_SIZE;   
  
  const uint8_t *flashPtr = (const uint8_t*)f_get_addr(flashStartIndex);
  const uint alignedBytes = ALIGN_UP(bytes, sizeof(findex_t));
  
  if (flash_config.boundAddr && !f_valid_index(flashStartIndex 
    + F_B2I(alignedBytes)))
      return -1;

  uint found = 0; 
  uint size = 1;  
  while((uintptr_t)flashPtr < maxAddr - 1) {

    if (memcmp(flashPtr, flashPtr + 1, size)) {
      found += size;
      flashPtr += size;

      if (found + (1 << size) < alignedBytes) {
        size = (1 << size);
      } else if (found < alignedBytes) {
        size = alignedBytes - found;
      } else {
        return F_B2I((uintptr_t)flashPtr);
      }
    } else {
      size = 1;
      found = 0;
    }
  }
  return -1;
}

unsigned int flash_query(size_t requiredBytes) {
  const volatile fmem_t *flashPtr = (const volatile fmem_t*)(F_B2M(FLASH_ADDR));
}

FLASH_ERROR flash_set_region_lock(unsigned int regionIndex, bool locked) {
  if (flash_config.boundAddr && regionIndex > 32)
    return FLASH_ERROR_ADDR;    
  else if (flash_get_region_locked(regionIndex) && !locked)
    return FLASH_ERROR_LOCK;

  NVMCTRL->ADDR.bit.ADDR = regionIndex * F_B2M(FLASH_ADDR + (FLASH_SIZE / 32));
  f_cmd(NVMCTRL_CTRLB_CMD_LR_Val);

  while(!NVMCTRL->STATUS.bit.READY);
  return flash_get_error();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> SEEPROM
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef uint8_t sindex_t; 

///////////////// HELPER FUNCTIONS ///////////////////
static inline bool see_valid_index(uint seepromIndex, uint bytes) {
  return (seepromIndex + bytes < seeprom_get_size());
}

static inline uintptr_t see_index_addr(uint seepromIndex) {
  return SEEPROM_ADDR + seepromIndex;
}

enum SEEPROM_STATE {

};

static inline SEEPROM_ERROR see_(std::initializer_list<SEEPROM_STATE> ignoreStates) {
  if (NVMCTRL->INTFLAG.bit.SEESOVF) {
    return 
  }


};

///////////////// CORE FUNCTIONS ///////////////////

bool seeprom_get_init() { return NVMCTRL->SEESTAT.bit.PSZ && NVMCTRL->PARAM.bit.SEE; }

bool seeprom_get_full() { return NVMCTRL->INTFLAG.bit.SEESFULL; }

uint seeprom_get_size() {
  static int qRef = -1;;
  if (!seeprom_get_init()) 
    return SEEPROM_ERROR_STATE;
    
  if (qRef == -1) {
    for (int i = 0; i < sizeof(SEE_REF) / sizeof(SEE_REF[0]); i++) {
      if (SEE_REF[i][1] == NVMCTRL->SEESTAT.bit.SBLK
        && SEE_REF[i][2] == NVMCTRL->SEESTAT.bit.PSZ) {
        
        qRef = SEE_REF[i][0];
      }
    }
  }
  return qRef;
}

SEEPROM_ERROR seeprom_init(int minBytes, bool restartNow) { 
  uint8_t blockCount = 0;
  uint16_t pageCount = 0;

  if (!NVMCTRL->PARAM.bit.SEE)
    return SEEPROM_ERROR_STATE;

  else if (NVMCTRL->SEESTAT.bit.LOCK 
    || NVMCTRL->SEESTAT.bit.RLOCK)
      return SEEPROM_ERROR_LOCK;

  for (int16_t i = 0; i < (sizeof(SEE_REF) / sizeof(SEE_REF[0])); i++) {
    if (SEE_REF[i][0] >= minBytes) {
      blockCount = SEE_REF[i][1];
      pageCount = SEE_REF[i][2];
      break;
    }
  }
  if (!blockCount || !pageCount)
    return SEEPROM_ERROR_PARAM;

  memcpy(UP, (const void*)NVMCTRL_USER, FLASH_USER_PAGE_SIZE);

  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;
  NVMCTRL->ADDR.bit.ADDR = NVMCTRL_USER;
  f_cmd(NVMCTRL_CTRLB_CMD_EP_Val);
  f_cmd(NVMCTRL_CTRLB_CMD_PBC_Val);
  
  UP[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 
      (NVMCTRL_FUSES_SEESBLK(blockCount) 
    | NVMCTRL_FUSES_SEEPSZ((uint8_t)(log2(pageCount) - 2)));

  for (int i = 0; i < FLASH_USER_PAGE_SIZE; i += sizeof(findex_t)) {
    memcpy((void*)(NVMCTRL_USER + i), UP + i, sizeof(findex_t));
  }
  f_cmd(NVMCTRL_CTRLB_CMD_WP_Val);
  while(!NVMCTRL->STATUS.bit.READY);

  if (restartNow) {
    if (memcmp((void*)NVMCTRL_USER, UP, FLASH_USER_PAGE_SIZE)) {
      prog_restart();
    } else {
      return SEEPROM_ERROR_STATE;
    }
  }
  return see_get_error();
}

SEEPROM_ERROR exit_seeprom(bool restartNow) {
  if (!seeprom_get_init())
    return SEEPROM_ERROR_STATE;

  f_cmd(NVMCTRL_CTRLB_CMD_LSEE_Val);
  memcpy(UP, (const void*)NVMCTRL_USER, FLASH_USER_PAGE_SIZE);
  
  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;
  NVMCTRL->ADDR.bit.ADDR = NVMCTRL_USER;
  f_cmd(NVMCTRL_CTRLB_CMD_EP_Val);
  f_cmd(NVMCTRL_CTRLB_CMD_PBC_Val);

  UP[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 
      NVMCTRL_FUSES_SEESBLK(0)
    | NVMCTRL_FUSES_SEEPSZ(0);
  
  for (int i = 0; i < FLASH_USER_PAGE_SIZE; i += sizeof(findex_t)) {
    memcpy((void*)(NVMCTRL_USER + i), UP + i, sizeof(findex_t));
  }
  f_cmd(NVMCTRL_CTRLB_CMD_WP_Val);
  while(!NVMCTRL->STATUS.bit.READY);

  if (restartNow) {
    if (memcmp((void*)NVMCTRL_USER, UP, FLASH_USER_PAGE_SIZE)) {
      prog_restart();
    } else {
      SEEPROM_ERROR error = see_get_error();
      return error == SEEPROM_ERROR_NONE ? SEEPROM_ERROR_STATE : error;
    }
  }
  f_cmd(NVMCTRL_CTRLB_CMD_LSEER_Val);
  return see_get_error();
}

SEEPROM_ERROR update_seeprom_config() { 
  if (!get_seeprom_init())
    return SEEPROM_ERROR_STATE;
  if (NVMCTRL->SEESTAT.bit.RLOCK)
    return SEEPROM_ERROR_LOCK;

  f_cmd(seeprom_config.locked ? NVMCTRL_CTRLB_CMD_LSEE_Val 
    : NVMCTRL_CTRLB_CMD_USEE_Val);
  NVMCTRL->SEECFG.bit.APRDIS = (uint8_t)seeprom_config.pageBuffer;
  NVMCTRL->SEECFG.bit.WMODE = (uint8_t)seeprom_config.autoRealloc;
  return see_get_error();
}

SEEPROM_ERROR seeprom_write_data(uint &seepromIndex, void *data, uint bytes,
  bool blocking) {
  if (!data)
    return SEEPROM_ERROR_PARAM;
  if (NVMCTRL->SEESTAT.bit.LOCK || NVMCTRL->SEESTAT.bit.RLOCK)
    return SEEPROM_ERROR_LOCK;

  NVMCTRL->SEECFG.reg = 
      ((bool)seeprom_config.autoRealloc << NVMCTRL_SEECFG_APRDIS_Pos)
    | ((bool)seeprom_config.pageBuffer << NVMCTRL_SEECFG_WMODE_Pos);

  while(NVMCTRL->SEESTAT.bit.BUSY);  
  memcpy((void*)see_index_addr(seepromIndex), data, bytes);
  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);

  seepromIndex += bytes;
  return see_get_error();
}

SEEPROM_ERROR seeprom_read_data(uint &seepromIndex, void *data, uint bytes,
  bool blocking) {
  if (!data)
    return SEEPROM_ERROR_PARAM;
  if (NVMCTRL->SEESTAT.bit.LOCK || NVMCTRL->SEESTAT.bit.RLOCK)
    return SEEPROM_ERROR_LOCK;

  while(NVMCTRL->SEESTAT.bit.BUSY);
  memcpy(data, (const void*)seepromIndex, bytes);
  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);

  seepromIndex += bytes;
  return see_get_error();
}

SEEPROM_ERROR seeprom_flush_buffer() {
  if (!seeprom_get_init())
    return SEEPROM_ERROR_STATE;
  
  f_cmd(NVMCTRL_CTRLB_CMD_SEEFLUSH_Val);
  while(!NVMCTRL->STATUS.bit.READY);
  return see_get_error();
}

SEEPROM_ERROR seeprom_realloc() {
  if (!seeprom_get_init())
    return SEEPROM_ERROR_STATE;
  
  f_cmd(NVMCTRL_CTRLB_CMD_SEERALOC_Val);
  while(!NVMCTRL->STATUS.bit.READY);
  return see_get_error();
} 

SEEPROM_ERROR seeprom_clear_buffer() {
  if (!seeprom_get_init()) 
    return SEEPROM_ERROR_STATE;

  f_cmd(NVMCTRL_CTRLB_CMD_PBC_Val);
  while(!NVMCTRL->STATUS.bit.READY);
  return see_get_error();
}
