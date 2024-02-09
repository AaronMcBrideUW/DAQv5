/*

#include "SYS.h"

void NVMCTRL_0_Handler(void);

static inline uintptr_t f_index_addr_(const unsigned int index);
static inline bool f_valid_index_(unsigned int index);
static inline void f_cmd_(uint8_t cmdVal);
static inline FLASH_ERROR f_get_errors_();

static inline bool see_valid_index_(unsigned int seepromIndex, unsigned int bytes);
static inline uintptr_t see_index_addr_(unsigned int seepromIndex);
static inline SEEPROM_ERROR see_get_errors_();

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> PIN LOCAL
///////////////////////////////////////////////////////////////////////////////////////////////////

bool pin_reset(unsigned int pinID) {
  if (!pin_valid(pinID)) 
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];
  PORT->Group[pin.group].DIRCLR.reg |= (1 << pin.number);
  PORT->Group[pin.group].OUTCLR.reg |= (1 << pin.number);
  PORT->Group[pin.group].PINCFG[pin.number].reg &= PORT_PINCFG_RESETVALUE;
  PORT->Group[pin.group].PMUX[pin.number].reg &= PORT_PMUX_RESETVALUE;
  return true;
}

bool pin_attach(unsigned int pinID, unsigned int periphID) {
  if (!pin_valid(pinID))
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID]; 
  if (periphID >= 0) {
    PORT->Group[pin.group].PMUX[pin.number].reg = (pin.number % 2) 
      ? PORT_PMUX_PMUXO((uint8_t)periphID) : PORT_PMUX_PMUXE((uint8_t)periphID);
    PORT->Group[pin.group].PINCFG[pin.number].bit.PMUXEN = 1;
  } else {
    PORT->Group[pin.group].PINCFG[pin.number].bit.PMUXEN = 0;      
  }
  return true;
}

bool pin_valid(const unsigned int pinID) {
  return pinID < BOARD_PIN_COUNT;
}

bool pin_set_digital(unsigned int pinID, unsigned int pinState, bool pullPin) {
  if (!pin_valid(pinID))
    return false;

  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];
  PORT->Group[pin.group].PINCFG[pin.number].bit.PULLEN = (uint8_t)pullPin;
  PORT->Group[pin.group].PINCFG[pin.number].bit.INEN = 0;

  if (pinState) {
    PORT->Group[pin.group].PINCFG[pin.number].bit.DRVSTR = (uint8_t)(pinState >= 2);
    if (pinState >= 1) 
      PORT->Group[pin.group].OUTSET.reg |= (1 << pin.number);
    else               
      PORT->Group[pin.group].OUTCLR.reg |= (1 << pin.number);
    if (pullPin) 
      PORT->Group[pin.group].DIRCLR.reg |= (1 << pin.number);
    else         
      PORT->Group[pin.group].DIRSET.reg |= (1 << pin.number); 
  } else {
    PORT->Group[pin.group].DIRCLR.reg |= (1 << pin.number);
    PORT->Group[pin.group].OUTCLR.reg |= (1 << pin.number);
  }
  return true;
}

int pin_read_digital(unsigned int pinID) {
  if (!pin_valid(pinID)) 
    return -1;
    
  const PIN_DESCRIPTOR &pin = BOARD_PINS[pinID];
  PORT->Group[pin.group].PINCFG[pin.number].bit.INEN = 1;
  return (int)(PORT->Group[pin.group].IN.reg & (1 << pin.number));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: NVM MISC
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef uint32_t fmem_t;                      /// Type for flash memory
typedef struct { uint64_t hi, lo; }findex_t;  // Type for flash index   

typedef uint8_t seemem_t;                     // Type for seeprom memory
typedef uint8_t seeindex_t;                   // Type for seeprom index

#define F_I2M(_index_)          ((_index_) * (sizeof(findex_t) / sizeof(fmem_t)))   // Flash index -> memory
#define F_M2I(_mem_)            ((_mem_) * (sizeof(fmem_t) / sizeof(findex_t)))     // Flash memory -> index
#define F_B2M(_byte_)           ((_byte_) == 0 ? 0 : ((_byte_) / sizeof(fmem_t)))   // Bytes -> flash memory
#define F_B2I(_byte_)           ((_byte_) == 0 ? 0 : ((_byte_) / sizeof(findex_t))) // Bytes -> flash index
#define ALIGN_DOWN(_val_, _al_) ((_val_) == 0 ? 0 : ((_val_) / (_al_)) * (_al_))    // Aligns value(1) DOWN to multiple(2)
#define ALIGN_UP(_val_, _al_)   ((_val_) == 0 ? 0 : (((_val_) + (_al_) - 1) / (_al_)) * (_al_))  // Aligns value(1) UP to multiple(2)
#define F_REGION_COUNT 32
#define F_ERASEBUFF_SIZE NVMCTRL_BLOCK_SIZE 

static const unsigned int f_pagesize_i = F_B2I(NVMCTRL_PAGE_SIZE);
static const unsigned int f_erasebuff_addr = (NVMCTRL->PARAM.bit.NVMP * NVMCTRL_PAGE_SIZE
  - F_ERASEBUFF_SIZE);

static uint8_t userPageBuffer[FLASH_USER_PAGE_SIZE] = { 0 };         // For editing user page
static const uint32_t SEE_REF[8][3] {  // Reference for seeprom size settings
  {512,   1,  4},                      // [][0] = bytes, [][1] = pages, [][2] = blocks
  {1024,  1,  8},                       
  {2048,  1,  16},                      
  {4096,  1,  32},
  {8192,  2,  64},
  {16384, 3,  128},
  {32768, 5,  256},
  {65536, 10, 512}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: NVM PROPERTY DECLARATIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

/// flash property decl
const unsigned int flash_properties_::total_size 
  = NVMCTRL->PARAM.bit.NVMP * flash_properties_::page_size;
const unsigned int flash_properties_::index_size 
  = sizeof(findex_t);
const unsigned int flash_properties_::page_size 
  = FLASH_PAGE_SIZE;
const unsigned int flash_properties_::region_size 
  = FLASH_SIZE / F_REGION_COUNT; 
const unsigned int flash_properties_::index_count 
  = ceil((double)flash_properties_::total_size / flash_properties_::index_size);
const unsigned int flash_properties_::region_count 
  = ceil((double)flash_properties_::total_size / flash_properties_::region_size);
const unsigned int flash_properties_::page_count 
  = flash_properties_::total_size / flash_properties_::page_size;
const unsigned int flash_properties_::default_align 
  = sizeof(fmem_t);

const unsigned int seeprom_properties_::max_index_count
  = seeprom_properties_::max_total_size / sizeof(seeindex_t);
const unsigned int seeprom_properties_::max_total_size
  = SEE_REF[sizeof(SEE_REF)][2];
const unsigned int seeprom_properties_::max_page_size 
  = NVMCTRL_PAGE_SIZE;
const unsigned int seeprom_properties_::index_size 
  = sizeof(seeindex_t);
const unsigned int seeprom_properties_::default_align
  = sizeof(seemem_t);

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: NVM LOCAL METHODS 
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Calls interrupt callbacks 
void NVMCTRL_0_Handler(void) {
  if (NVMCTRL->SEESTAT.bit.BUSY) {
    if ((NVMCTRL->INTFLAG.bit.NVME || NVMCTRL->INTFLAG.bit.SEESFULL)
        && seeprom_config.errorInterrupt) {
      seeprom_config.errorInterrupt(see_get_errors_());
    }
  } else {
    if (NVMCTRL->INTFLAG.bit.NVME && flash_config.errorInterrupt) {
      flash_config.errorInterrupt(f_get_errors_()); 
    }
  } 
}

/// @internal Gets the address of a flash index
static inline uintptr_t f_index_addr_(const unsigned int index) {
  return FLASH_ADDR + index * sizeof(findex_t);
}

/// @internal Ensures index is within bounds of flash mem
static inline bool f_valid_index_(unsigned int index) {
  return !(flash_config.boundAddr && (index > F_B2I(f_erasebuff_addr)
    || index < F_B2I(FLASH_ADDR)));
}
/// @internal Executes command in NVMC
static inline void f_cmd_(uint8_t cmdVal) {
  while(!NVMCTRL->STATUS.bit.READY);
  NVMCTRL->CTRLB.reg = 
      NVMCTRL_CTRLB_CMDEX_KEY
    | cmdVal << NVMCTRL_CTRLB_CMD_Pos; 
}

/// @internal Gets & clears any errors in flash memory
static inline FLASH_ERROR f_get_errors_() {
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

// Checks to ensure index is valid
static inline bool see_valid_index_(unsigned int seepromIndex, unsigned int bytes) {
  return !(seeprom_config.checkAddr && seepromIndex 
    + ceil((double)bytes / sizeof(seeindex_t)) >= seeprom_get_size());
}
// Gets the address of a index
static inline uintptr_t see_index_addr_(unsigned int seepromIndex) {
  return (SEEPROM_ADDR / sizeof(seeindex_t)) + seepromIndex;
}
// Gets & clears any current errors
static inline SEEPROM_ERROR see_get_errors_() {
  SEEPROM_ERROR error = SEEPROM_ERROR_NONE;  
  if (NVMCTRL->INTFLAG.bit.SEESFULL) {
    NVMCTRL->INTFLAG.bit.SEESFULL = 1;
    error = SEEPROM_ERROR_FULL;
  }
  if (NVMCTRL->INTFLAG.bit.NVME) {
    NVMCTRL->INTFLAG.bit.NVME = 1;
    error = SEEPROM_ERROR_STATE;

      if  (NVMCTRL->INTFLAG.bit.LOCKE) {
        NVMCTRL->INTFLAG.bit.LOCKE = 1;
        error = SEEPROM_ERROR_LOCK;
      }
      if (NVMCTRL->INTFLAG.bit.PROGE) {
        NVMCTRL->INTFLAG.bit.PROGE = 1;
        error = SEEPROM_ERROR_STATE;
      } 
      if (NVMCTRL->INTFLAG.bit.ADDRE) {
        NVMCTRL->INTFLAG.bit.ADDRE = 1;
        error = SEEPROM_ERROR_ADDR;                                                                                                                                                                             
      } 
  }
  return error;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: FLASH FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

FLASH_ERROR flash_update_config() {
  NVMCTRL->CTRLA.bit.PRM = flash_config.lowPowerOnSleep 
    ? NVMCTRL_CTRLA_PRM_FULLAUTO_Val : NVMCTRL_CTRLA_PRM_MANUAL_Val;

  f_cmd_(flash_config.lowPowerMode ? NVMCTRL_CTRLB_CMD_SPRM_Val 
    : NVMCTRL_CTRLB_CMD_CPRM_Val);

  if (flash_config.errorInterrupt == nullptr) {
    NVMCTRL->INTENCLR.bit.NVME;
  } else {
    NVMCTRL->INTENSET.bit.NVME;
  }
  return f_get_errors_();
}

FLASH_ERROR flash_write_data(unsigned int &flashIndex, volatile const void *data, 
  const unsigned int bytes) {

  volatile const fmem_t *alignedData = (volatile const fmem_t*)data;
  volatile fmem_t *flashPtr = (volatile fmem_t*)f_index_addr_(flashIndex);

  if (!alignedData || !flashPtr) {
    return FLASH_ERROR_PARAM;

  } else if (!f_valid_index_(flashIndex + F_B2I(bytes))) {
    return FLASH_ERROR_ADDR;
  }

  NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_WMODE_MAN;  
  if (NVMCTRL->STATUS.bit.LOAD) {
    f_cmd_(NVMCTRL_CTRLB_CMD_PBC_Val);
  }
  const unsigned int flashWriteSize = F_B2M(flash_config.writePage 
    ? FLASH_PAGE_SIZE : sizeof(findex_t));
  const unsigned int alignCount = F_I2M(ALIGN_DOWN(bytes, sizeof(fmem_t)));  

  // Write aligned data
  for (int i = 0; i < alignCount; i++) {      
    flashPtr[i] = alignedData[i];
    if (i % flashWriteSize == flashWriteSize - 1) {
      f_cmd_(flash_config.writePage ? NVMCTRL_CTRLB_CMD_WP_Val 
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
    f_cmd_(flash_config.writePage ? NVMCTRL_CTRLB_CMD_WP_Val 
      : NVMCTRL_CTRLB_CMD_WQW_Val);
  }
  while(!NVMCTRL->STATUS.bit.READY);

  FLASH_ERROR error = f_get_errors_();
  if (error == FLASH_ERROR_NONE) {
    flashIndex += F_M2I(alignCount);
  }
  return error;
}

FLASH_ERROR flash_copy_data(unsigned int &flashIndex, void *dest, 
  const unsigned int bytes) {
  if (!dest || !f_valid_index_(flashIndex + F_B2I(ALIGN_UP(bytes, sizeof(findex_t)))))
    return FLASH_ERROR_ADDR;

  const void *flashPtr = (const void*)(flashIndex * sizeof(findex_t) + FLASH_ADDR);
  memcpy(dest, flashPtr, bytes);
  while(!NVMCTRL->STATUS.bit.READY);

  FLASH_ERROR error = f_get_errors_();
  if (error == FLASH_ERROR_NONE) {
    flashIndex += F_B2I(ALIGN_UP(bytes, sizeof(findex_t)));
  }
  return error;
}

const volatile void *flash_read_data(unsigned int &flashIndex, unsigned int bytes) {
  if (bytes != 0 || !f_valid_index_(flashIndex 
    + F_B2I(ALIGN_UP(bytes, sizeof(findex_t)))))
      return nullptr;

  flashIndex = F_B2I(ALIGN_UP(bytes, sizeof(findex_t)));
  return (const volatile void*)f_index_addr_(flashIndex);
}

/// NOTE: CONSIDER ADDING A LOCK CHECK TO THIS METHOD....

FLASH_ERROR flash_erase(unsigned int flashIndex, unsigned int indexCount, 
  bool forceAligned) {

  if (!f_valid_index_(flashIndex) || !f_valid_index_(flashIndex + indexCount))
      return FLASH_ERROR_ADDR; 

  const unsigned int endIndex = flashIndex + indexCount;
  const unsigned int alStart = ALIGN_DOWN(flashIndex, F_B2I(NVMCTRL_BLOCK_SIZE));
  const unsigned int alEnd = ALIGN_UP(endIndex,F_B2I(NVMCTRL_BLOCK_SIZE));

  bool rwStartFlag = false;
  bool rwEndFlag = false;

  auto eraseBlock = [&](long addr, int index) -> void {   
    if (addr < 0 && index >= 0) {
      NVMCTRL->ADDR.bit.ADDR = f_index_addr_(index);
    } else if (addr < 0) {
      NVMCTRL->ADDR.bit.ADDR = addr;
    } else {
      return;
    }
    f_cmd_(NVMCTRL_CTRLB_CMD_EB_Val);    
  };

  if (!forceAligned) {
    if (flashIndex % F_B2I(flash_properties.region_size) != 0) {
      unsigned int index = (unsigned int)F_B2I(f_erasebuff_addr);
      
      if (!flash_write_data(index, (volatile void*)f_index_addr_(alStart), 
        (flashIndex - alStart) * sizeof(findex_t)) == FLASH_ERROR_NONE) {
        rwStartFlag = true;
      } else {
        goto bufferErrorExit;
      }
    }
    if (endIndex % F_B2I(flash_properties.region_size) != 0) {
      unsigned int index = (unsigned int)F_B2I(f_erasebuff_addr + indexCount);

      if (flash_write_data(index, (volatile void*)f_index_addr_(endIndex), 
        (alEnd - endIndex) * sizeof(findex_t)) == FLASH_ERROR_NONE) {
        rwEndFlag = true;
      } else {
        goto bufferErrorExit; 
      }
    }
  }
  for (int i = flashIndex; i < alEnd; i += (flash_properties.region_size 
    / sizeof(findex_t))) {
    eraseBlock(-1, i);
  }
  if (!forceAligned) {
    if (rwStartFlag) {
      unsigned int writeIndex = alStart;
      if(!flash_write_data(writeIndex, (volatile void*)f_erasebuff_addr, 
        (flashIndex - alStart) * sizeof(findex_t)) != FLASH_ERROR_NONE) {
        goto bufferErrorExit;
      }
    }
    if (rwEndFlag) {
      unsigned int writeIndex = endIndex;
      if (!flash_write_data(writeIndex, (volatile void*)(f_erasebuff_addr 
        + (flashIndex - alStart) * sizeof(findex_t)), (alEnd - endIndex) 
        * sizeof(findex_t)) != FLASH_ERROR_NONE) {
        goto bufferErrorExit;
      }
    }
    eraseBlock(f_erasebuff_addr + 10, -1);
  }
  while(!NVMCTRL->STATUS.bit.READY);
  return f_get_errors_();

  bufferErrorExit: {
    eraseBlock(f_erasebuff_addr + 10, -1);
    while(!NVMCTRL->STATUS.bit.READY);
    return FLASH_ERROR_PROG;
  }
}

FLASH_ERROR flash_clear() {
  static const unsigned int blocks = ceil((double)(NVMCTRL->PARAM.bit.NVMP 
    * FLASH_PAGE_SIZE) / NVMCTRL_BLOCK_SIZE) * NVMCTRL_BLOCK_SIZE;
  
  for (int i = 0; i < blocks; i++) {
    NVMCTRL->ADDR.bit.ADDR = FLASH_ADDR + i * NVMCTRL_BLOCK_SIZE;
    f_cmd_(NVMCTRL_CTRLB_CMD_EB_Val);
  }
  while(!NVMCTRL->STATUS.bit.READY);
  return f_get_errors_();
}

FLASH_ERROR flash_set_region_lock(unsigned int regionIndex, bool locked) {
  if (flash_config.boundAddr && regionIndex > 32)
    return FLASH_ERROR_ADDR;    
  else if (flash_get_region_locked(regionIndex) && !locked)
    return FLASH_ERROR_LOCK;

  NVMCTRL->ADDR.bit.ADDR = regionIndex * F_B2M(FLASH_ADDR + (FLASH_SIZE / 32));
  f_cmd_(NVMCTRL_CTRLB_CMD_LR_Val);

  while(!NVMCTRL->STATUS.bit.READY);
  return f_get_errors_();
}

bool flash_get_region_locked(unsigned int regionIndex) {
  return (NVMCTRL->RUNLOCK.reg & (1 << (regionIndex * F_B2M(FLASH_ADDR 
    + (FLASH_SIZE / 32)))));
}

bool flash_is_free(unsigned int flashIndex, unsigned int bytes) {
  unsigned int alignedBytes = ALIGN_UP(bytes, sizeof(findex_t));
  uint8_t *flashPtr = (uint8_t*)f_index_addr_(flashIndex);
  
  if (!f_valid_index_(flashIndex + alignedBytes)) 
    return false;
  return memcmp(flashPtr, flashPtr + 1, bytes - 1); 
}

int flash_find_free(unsigned int bytes, unsigned int flashStartIndex) {
  static const uintptr_t maxAddr = FLASH_ADDR + NVMCTRL->PARAM.bit.NVMP 
    * FLASH_PAGE_SIZE;   
  
  const uint8_t *flashPtr = (const uint8_t*)f_index_addr_(flashStartIndex);
  const unsigned int alignedBytes = ALIGN_UP(bytes, sizeof(findex_t));
  
  if (!f_valid_index_(flashStartIndex + F_B2I(alignedBytes)))
      return -1;

  unsigned int found = 0; 
  unsigned int size = 1;  
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
      flashPtr = (const uint8_t*)ALIGN_UP((uintptr_t)flashPtr, sizeof(findex_t));
      size = 1; 
      found = 0;
    }
  }
  return -1;
}

int flash_query(const void* data, unsigned int bytes, unsigned int startIndex) {
  if (!data || !bytes || (!f_valid_index_(startIndex)))
    return -2;

  const volatile fmem_t *flashPtr = (const volatile fmem_t*)(F_B2M(FLASH_ADDR));
  const fmem_t *alData = (const fmem_t*)data; 
  unsigned int run = 0;
  unsigned int lastIndex = 0;

  while(run < bytes && lastIndex * sizeof(findex_t) <= NVMCTRL->PARAM.bit.NVMP 
    * FLASH_PAGE_SIZE - bytes) {

    if (bytes - run > sizeof(fmem_t)) {
      if (flashPtr == alData) {
        flashPtr++;
        alData++;
        run += sizeof(fmem_t);
      } else {
        run = 0;
        lastIndex += run;
      }
    } else if (bytes - run > 0) {
      const uint8_t *byteData = (const uint8_t*)data;
      const uint8_t *bytePtr = (const uint8_t*)flashPtr;

      for (int j = 0; j < bytes - run; j++) {
        if (bytePtr[j] != byteData[run + j]) {
          run++;
          flashPtr++;
          lastIndex = run += sizeof(fmem_t);
        }
        return lastIndex;
      }
    } else {
      return lastIndex;
    }
  }
  return -1;
}

unsigned int flash_index2region(unsigned int flashIndex) {
  return (flashIndex * sizeof(findex_t)) / flash_properties.region_size;  
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: SEEPROM FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

SEEPROM_ERROR seeprom_update_config() { 
  if (!seeprom_get_init())
    return SEEPROM_ERROR_STATE;
  if (NVMCTRL->SEESTAT.bit.RLOCK)
    return SEEPROM_ERROR_LOCK;

  f_cmd_(seeprom_config.locked ? NVMCTRL_CTRLB_CMD_LSEE_Val 
    : NVMCTRL_CTRLB_CMD_USEE_Val);
  seeprom_config.locked = NVMCTRL->SEESTAT.bit.LOCK 
    || NVMCTRL->SEESTAT.bit.RLOCK;

  NVMCTRL->SEECFG.bit.APRDIS = (uint8_t)seeprom_config.pageBuffer;
  NVMCTRL->SEECFG.bit.WMODE = (uint8_t)seeprom_config.autoRealloc;
  return see_get_errors_();
}

SEEPROM_ERROR seeprom_init(int minBytes, bool restartNow) { 
  unsigned int blockCount = 0;
  unsigned int pageCount = 0;

  if (!NVMCTRL->PARAM.bit.SEE)
    return SEEPROM_ERROR_STATE;

  else if (NVMCTRL->SEESTAT.bit.LOCK 
    || NVMCTRL->SEESTAT.bit.RLOCK)
      return SEEPROM_ERROR_LOCK;

  for (int i = 0; i < (sizeof(SEE_REF) / sizeof(SEE_REF[0])); i++) {
    if (SEE_REF[i][0] >= minBytes) {
      blockCount = SEE_REF[i][1];
      pageCount = SEE_REF[i][2];
      break;
    }
  }
  if (!blockCount || !pageCount)
    return SEEPROM_ERROR_PARAM;

  memcpy(userPageBuffer, (const void*)NVMCTRL_USER, FLASH_USER_PAGE_SIZE);

  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;
  NVMCTRL->ADDR.bit.ADDR = NVMCTRL_USER;
  f_cmd_(NVMCTRL_CTRLB_CMD_EP_Val);
  f_cmd_(NVMCTRL_CTRLB_CMD_PBC_Val);
  
  userPageBuffer[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 
      (NVMCTRL_FUSES_SEESBLK(blockCount) 
    | NVMCTRL_FUSES_SEEPSZ((uint8_t)(log2(pageCount) - 2)));

  for (int i = 0; i < FLASH_USER_PAGE_SIZE; i += sizeof(findex_t)) {
    memcpy((void*)(NVMCTRL_USER + i), userPageBuffer + i, sizeof(findex_t));
  }
  f_cmd_(NVMCTRL_CTRLB_CMD_WP_Val);
  while(!NVMCTRL->STATUS.bit.READY);

  if (restartNow) {
    if (memcmp((void*)NVMCTRL_USER, userPageBuffer, FLASH_USER_PAGE_SIZE)) {
      prog_reset();
    } else {
      return SEEPROM_ERROR_STATE;
    }
  }
  return see_get_errors_();
}

SEEPROM_ERROR seeprom_exit(bool restartNow) {
  if (!seeprom_get_init())
    return SEEPROM_ERROR_STATE;

  memcpy(userPageBuffer, (const void*)NVMCTRL_USER, FLASH_USER_PAGE_SIZE);
  
  NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN_Val;
  NVMCTRL->ADDR.bit.ADDR = NVMCTRL_USER;
  f_cmd_(NVMCTRL_CTRLB_CMD_EP_Val);
  f_cmd_(NVMCTRL_CTRLB_CMD_PBC_Val);

  userPageBuffer[NVMCTRL_FUSES_SEEPSZ_ADDR - NVMCTRL_USER] = 
      NVMCTRL_FUSES_SEESBLK(0)
    | NVMCTRL_FUSES_SEEPSZ(0);
  
  for (int i = 0; i < FLASH_USER_PAGE_SIZE; i += sizeof(findex_t)) {
    memcpy((void*)(NVMCTRL_USER + i), userPageBuffer + i, sizeof(findex_t));
  }
  f_cmd_(NVMCTRL_CTRLB_CMD_WP_Val);
  while(!NVMCTRL->STATUS.bit.READY);

  if (restartNow) {
    if (memcmp((void*)NVMCTRL_USER, userPageBuffer, FLASH_USER_PAGE_SIZE)) {
      prog_reset();
    } else {
      SEEPROM_ERROR error = see_get_errors_();
      return error == SEEPROM_ERROR_NONE ? SEEPROM_ERROR_STATE : error;
    }
  }
  f_cmd_(NVMCTRL_CTRLB_CMD_LSEE_Val);
  f_cmd_(NVMCTRL_CTRLB_CMD_LSEER_Val);
  return see_get_errors_();
}

SEEPROM_ERROR seeprom_write_data(unsigned int &seepromIndex, void *data, unsigned int bytes,
  bool blocking) {
  if (!data)
    return SEEPROM_ERROR_PARAM;
  if (NVMCTRL->SEESTAT.bit.LOCK || NVMCTRL->SEESTAT.bit.RLOCK)
    return SEEPROM_ERROR_LOCK;
  if (seeprom_config.checkAddr && !see_valid_index_(seepromIndex, bytes))
    return SEEPROM_ERROR_ADDR;

  NVMCTRL->SEECFG.reg = 
      ((bool)seeprom_config.autoRealloc << NVMCTRL_SEECFG_APRDIS_Pos)
    | ((bool)seeprom_config.pageBuffer << NVMCTRL_SEECFG_WMODE_Pos);

  while(NVMCTRL->SEESTAT.bit.BUSY);  
  memcpy((void*)see_index_addr_(seepromIndex), data, bytes / sizeof(seemem_t));
  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);

  seepromIndex += ceil((double)bytes / sizeof(seeindex_t));
  return see_get_errors_();
}

SEEPROM_ERROR seeprom_copy_data(unsigned int &seepromIndex, void *data, unsigned int bytes,
  bool blocking) {
  if (!data)
    return SEEPROM_ERROR_PARAM;
  if (seeprom_config.checkAddr && !see_valid_index_(seepromIndex, bytes))
    return SEEPROM_ERROR_ADDR;

  while(NVMCTRL->SEESTAT.bit.BUSY);
  memcpy(data, (const void*)seepromIndex, bytes);
  while(blocking && NVMCTRL->SEESTAT.bit.BUSY);

  seepromIndex += ceil((double)bytes / sizeof(seeindex_t));
  return see_get_errors_();
}

const volatile void *seeprom_read_data(unsigned int &seepromIndex, const volatile void *ptr,
  unsigned int bytes) {
  if (seeprom_config.checkAddr && bytes != 0 
    && !see_valid_index_(seepromIndex, bytes))
    return nullptr;

  seepromIndex += ceil((double)bytes / sizeof(seeindex_t));
  return (const volatile void*)see_index_addr_(seepromIndex);
}

SEEPROM_ERROR seeprom_flush_buffer() {
  if (!seeprom_get_init())
    return SEEPROM_ERROR_STATE;
  
  f_cmd_(NVMCTRL_CTRLB_CMD_SEEFLUSH_Val);
  while(!NVMCTRL->STATUS.bit.READY);
  return see_get_errors_();
}

SEEPROM_ERROR seeprom_realloc() {
  if (!seeprom_get_init())
    return SEEPROM_ERROR_STATE;
  
  f_cmd_(NVMCTRL_CTRLB_CMD_SEERALOC_Val);
  while(!NVMCTRL->STATUS.bit.READY);
  return see_get_errors_();
} 

SEEPROM_ERROR seeprom_clear_buffer() {
  if (!seeprom_get_init()) 
    return SEEPROM_ERROR_STATE;

  f_cmd_(NVMCTRL_CTRLB_CMD_PBC_Val);
  while(!NVMCTRL->STATUS.bit.READY);
  return see_get_errors_();
}

unsigned int seeprom_get_size() {
  static int qRef = -1;
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

bool seeprom_get_init() { 
  return NVMCTRL->SEESTAT.bit.PSZ 
    && NVMCTRL->PARAM.bit.SEE 
    && !NVMCTRL->SEESTAT.bit.RLOCK; 
}

bool seeprom_get_full() { 
  return NVMCTRL->INTFLAG.bit.SEESFULL; 
}

bool seeprom_get_busy() { 
  return NVMCTRL->SEESTAT.bit.BUSY; 
}

bool seeprom_get_pending_data() { 
  return NVMCTRL->SEESTAT.bit.LOAD; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: PROGRAM FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Exported by linker script
extern uint32_t __sketch_vectors_ptr;

/// @brief Stores address of each byte of serial num
///           for a specific board
#ifdef BOARD_FEATHER_M4_CAN
  static const uintptr_t snAddrArray[] = 
    {8413692, 8413692, 8413692, 8413692, 
     8413200, 8413200, 8413200, 8413200,
     8413204, 8413204, 8413204, 8413204, 
     8413208, 8413208, 8413208, 8413208};
#else
  static const uintptr_t *snAddrArray = nullptr;
#endif

/// @brief Defs for program functions
#define PROG_RESET_KEY 0xFA05U


void prog_reset(bool hardReset) {
  auto requestSysReset = [](void) -> void {
    SCB->AIRCR = (decltype(SCB->AIRCR)) 
        (PROG_RESET_KEY << SCB_AIRCR_VECTKEY_Pos)
      | (1 << SCB_AIRCR_SYSRESETREQ_Pos);
  };
  __disable_irq();
  __DSB();
  if (hardReset) {
    requestSysReset();
  } else {
    #ifdef BOARD_FEATHER_M4_CAN
      __set_MSP(__sketch_vectors_ptr + 4);
    #else
      return;
    #endif
  }
  __DSB();
  for (;;) {
    asm volatile ("nop");
  }
}

PROG_RESET_REASON prog_get_reset_reason() {
  switch(RSTC->RCAUSE.reg) {
    case RSTC_RCAUSE_BACKUP: return BACKUP_RESET;
    case RSTC_RCAUSE_SYST: return SYSTEM_REQ_RESET;
    case RSTC_RCAUSE_WDT: return WATCHDOG_RESET;
    case RSTC_RCAUSE_EXT: return EXTERNAL_RESET;
    case RSTC_RCAUSE_NVM: return NVM_RESET;
    case RSTC_RCAUSE_BODCORE: return BROWNOUT_RESET;
    case RSTC_RCAUSE_BODVDD: return BROWNOUT_RESET;
    case RSTC_RCAUSE_POR: return POWERON_RESET;
  }
  return UNKNOWN_RESET;
}

unsigned int prog_get_serial_number(uint8_t *resultArray) {
  if (!snAddrArray || !resultArray)
    return 0;
  for (int i = 0; i < sizeof(snAddrArray); i++) {
    resultArray[i] == *reinterpret_cast<uint8_t*>(snAddrArray[i]);
  }
  return sizeof(snAddrArray);
}

unsigned int prog_get_cpuid() {
  return (decltype(SCB->CPUID))(SCB->CPUID & SCB_CPUID_PARTNO_Msk);
}

bool prog_sleep(PROG_SLEEP_MODE mode) {
  if (PM->SLEEPCFG.bit.SLEEPMODE != 0);
    return false;
  __ISB();
  PM->SLEEPCFG.bit.SLEEPMODE = (uint8_t)mode;
  __DMB();
  __WFI();
  return true;
}

bool NOCALL_prog_assert(bool statement, const int line, const char *func, 
  const char *file) {

  if (!PROG_ASSERT_ENABLED)
    return statement;

  if (statement) {
    NVIC_SystemReset();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: WATCHDOG TIMER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

#define WDT_MAX_TIMEOUT 
#define WDT_CLK_KHZ 1024UL
#define WDT_MIN_CYCLES 8UL
#define WDT_MAX_CYCLES 16384UL

void WDT_Handler(void) {
  WDT->INTFLAG.bit.EW = 1;
  if (wdt_config.wdtInterrupt) {
    wdt_config.wdtInterrupt();
  }
}

bool wdt_update_config() {
  if (WDT->CTRLA.bit.ENABLE)
    return false;

  WDT->CTRLA.bit.ALWAYSON = (uint8_t)wdt_config.alwaysOn;
  while(WDT->SYNCBUSY.bit.ALWAYSON);
  WDT->CTRLA.bit.WEN = (uint8_t)wdt_config.windowMode;
  while(WDT->SYNCBUSY.bit.WEN);

  if (wdt_config.wdtInterrupt) 
    WDT->INTENSET.bit.EW = 1;
  else
    WDT->INTENCLR.bit.EW = 1;
  return true;
}

bool wdt_set(bool enabled, unsigned int resetTimeout, unsigned int interruptTimeoutOffset,
  unsigned int windowTimeout) {
  auto convert2reg = [](unsigned int timeoutRaw) -> unsigned int {
    unsigned int cycles = timeoutRaw * (WDT_CLK_KHZ / 1000);
    cycles = cycles > WDT_MAX_CYCLES ? WDT_MAX_CYCLES : cycles < WDT_MIN_CYCLES 
      ? WDT_MIN_CYCLES : cycles;
    return log2(cycles) - log2(WDT_MIN_CYCLES);
  };
  WDT->CTRLA.bit.ENABLE = 0;
  while(WDT->SYNCBUSY.bit.ENABLE);
  if (enabled) {
    if (resetTimeout) 
      WDT->CONFIG.bit.PER = convert2reg(resetTimeout);
    if (windowTimeout)
      WDT->CONFIG.bit.WINDOW = convert2reg(windowTimeout);
    if (interruptTimeoutOffset)
      WDT->EWCTRL.bit.EWOFFSET = convert2reg(interruptTimeoutOffset);
  }
  wdt_update_config();
  WDT->CTRLA.bit.ENABLE = (uint8_t)enabled;
  while(WDT->SYNCBUSY.bit.ENABLE);
  return true;
}

void wdt_clear() {
  WDT->CLEAR.bit.CLEAR = WDT_CLEAR_CLEAR_KEY_Val;
  while(WDT->SYNCBUSY.bit.CLEAR);
}

unsigned int wdt_get_setting(WDT_SETTING settingSel) {
  auto reg2timeout = [](unsigned int regval) -> unsigned int {
    unsigned int cycles = pow(2, regval + log2(WDT_MIN_CYCLES));
    return cycles * (1000 / WDT_CLK_KHZ);
  };
  switch(settingSel) {
    case WDT_RESET_TIMEOUT: return reg2timeout(WDT->CONFIG.bit.PER);
    case WDT_INTERRUPT_TIMEOUT: return reg2timeout(WDT->EWCTRL.bit.EWOFFSET);
    case WDT_WINDOW_TIMEOUT: return reg2timeout(WDT->CONFIG.bit.WINDOW);
    case WDT_ENABLED: return (uint8_t)WDT->CTRLA.bit.ENABLE;
  }
  return 0;
}

*/