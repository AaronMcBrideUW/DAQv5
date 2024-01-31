
#include "SYS.h"

//// MASK DEFS ////
#define pmask (1 << pin.number)
#define pgroup PORT->Group[pin.group]

//// FLASH DEFS ////
#define FLASH_BLOCK_SIZE NVMCTRL_BLOCK_SIZE
#define FLASH_PAGE_SIZE (2 << (NVMCTRL->PARAM.bit.PSZ + 3))
#define FLASH_QUAD_SIZE 8
#define FLASH_PAGE_COUNT (NVMCTRL->PARAM.bit.NVMP)
#define FLASH_BLOCK_COUNT (FLASH_PAGE_COUNT / FLASH_BLOCK_SIZE)
#define FLASH_MIN_ADDR 0
#define FLASH_MAX_ADDR ((FLASH_PAGE_COUNT * FLASH_PAGE_SIZE) / 4)
#define FLASH_DATA_SIZE 4
#define _FRDY_ while(!NVMCTRL->STATUS.bit.READY);
#define _FDONE_ while(!NVMCTRL->INTFLAG.bit.DONE); \
                NVMCTRL->INTFLAG.bit.DONE = 1;

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

int write_flash(const volatile void *flash, void *data, int dataCount, 
  int dataAlignment, bool writePage) {

  if (!flash || !data || dataCount <= 0 || dataAlignment <= 0
    || (uint32_t)flash < FLASH_MIN_ADDR || NVMCTRL->STATUS.bit.SUSP)
    return 0;

  volatile uint32_t *flashMem = (volatile uint32_t*)flash;
  const int writeSize = writePage ? FLASH_PAGE_SIZE : FLASH_QUAD_SIZE;
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
    bytesWritten += writeSize;
    commitPending = false;
  };
  
  uint8_t *data8 = (uint8_t*)data;
  uint32_t *data32 = (uint32_t*)data;

  if (dataAlignment > FLASH_DATA_SIZE) {
    const int iterCount = dataCount * (int)ceil((float)dataAlignment 
      / FLASH_DATA_SIZE);
      
    if ((uint32_t)flash + iterCount > FLASH_MAX_ADDR)
      return bytesWritten;

    for (int i = 0; i < iterCount; i++) {
      flashMem[i] = data32[i];
      commitPending = true;

      if ((i * FLASH_DATA_SIZE) % writeSize == writeSize - 1) {
        commitFlash();
      }
    }
  } else {
    uint32_t byteBuff = 0;
    if ((uint32_t)flash + dataCount > FLASH_MAX_ADDR)
      return bytesWritten;

    for (int i = 0; i < dataCount; i++) {

      for (int j = 0; j < dataAlignment; j++) {
        byteBuff |= (data8[j + i * 4] << (j * 8));
      }
      flashMem[i] = byteBuff;
      byteBuff = 0;
      commitPending = true;
      
      if (i % writeSize == writeSize - 1) {
        commitFlash();
      }
    }
  }
  if (commitPending) {
    if ((uint32_t)flash + (bytesWritten / 4) + writeSize / 4 < FLASH_MAX_ADDR) {
      commitFlash();
    }
  }
  return bytesWritten;
}


int copy_flash(const volatile void *flash, void *dest, int copyCount, 
  int dataAlignment) {
  if (!flash || !dest || copyCount <= 0 || dataAlignment <= 0
    || (uint32_t)flash < FLASH_MIN_ADDR)
    return 0;

  int writtenBytes = 0;
  if (dataAlignment % FLASH_DATA_SIZE == 0) {
    if ((uint32_t)flash + copyCount * dataAlignment > FLASH_MAX_ADDR)
      return 0;

    NVMCTRL->CTRLA.bit.SUSPEN = 1;
    memcpy(dest, (const void*)flash, copyCount * dataAlignment);

    NVMCTRL->CTRLA.bit.SUSPEN = 0;
    writtenBytes = copyCount * dataAlignment;

  } else {
    const int flashIter = (dataAlignment + FLASH_DATA_SIZE - 1) / FLASH_DATA_SIZE;
    if ((uint32_t)flash + flashIter * copyCount > FLASH_MAX_ADDR)
      return 0;

    writtenBytes = copyCount;
    NVMCTRL->CTRLA.bit.SUSPEN = 1;

    for (int i = 0; i < copyCount; i++) {
      memcpy(dest, (const void*)((uint32_t*)flash + i * flashIter), dataAlignment); 
    }
    NVMCTRL->CTRLA.bit.SUSPEN = 0;
  }
  return writtenBytes;
};

bool erase_flash(int blockIndex) {
  uint32_t const blockAddr = FLASH_MIN_ADDR + blockIndex * FLASH_BLOCK_SIZE;
  
  if (blockAddr > FLASH_MAX_ADDR || blockAddr < FLASH_MIN_ADDR 
    || NVMCTRL->STATUS.bit.SUSP || get_flash_locked(blockAddr))
    return false;

  _FRDY_
  NVMCTRL->ADDR.reg = FLASH_MIN_ADDR + blockIndex * FLASH_BLOCK_SIZE;
  NVMCTRL->CTRLB.reg |= 
      NVMCTRL_CTRLB_CMDEX_KEY
    | NVMCTRL_CTRLB_CMD_EB;
  _FDONE_
  return true;
}

const volatile void *flash_addr(int addr) {
  if (addr < 0 || addr > FLASH_MAX_ADDR || addr < FLASH_MIN_ADDR)
    return nullptr;
  return (const volatile void*)((uint32_t)addr);
}

bool get_flash_locked(const volatile void *flash, int bytes) {
  if (!flash || (uint32_t)flash < FLASH_MIN_ADDR || (uint32_t)flash > FLASH_MAX_ADDR)
    return false;
 
  if (bytes <= 0) {
    return (NVMCTRL->RUNLOCK.reg & (1 << (uint32_t)flash));
  }
  for (int i = 0; i <= ((uint32_t)flash + bytes - FLASH_MIN_ADDR) / FLASH_BLOCK_SIZE 
    && i * FLASH_BLOCK_SIZE + (uint32_t)flash < FLASH_MAX_ADDR; i++) {

    if (NVMCTRL->RUNLOCK.reg & (1 << i))
      return true;
  }
  return false;
}

bool set_flash_lock(const volatile void *flash, bool locked) {
  if (!flash || (uint32_t)flash < FLASH_MIN_ADDR 
    || (uint32_t)flash > FLASH_MAX_ADDR)
    return false;

  bool currentLock = get_flash_locked(flash);
  if ((currentLock && locked) || (!currentLock && !locked))
    return true;

  _FRDY_
  NVMCTRL->CTRLB.reg |=
      NVMCTRL_CTRLB_CMDEX_KEY
    | ((locked ? NVMCTRL_CTRLB_CMD_LR_Val : NVMCTRL_CTRLB_CMD_UR) 
        << NVMCTRL_CTRLB_CMD_Pos);
  _FDONE_
  return get_flash_locked(flash) == locked;
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


