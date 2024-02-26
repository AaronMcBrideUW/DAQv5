#include "peripheral_core/dma_core2.h"

#define BURSTLEN_COUNT (sizeof(BURSTLEN_REF) / sizeof(BURSTLEN_REF[0]))
#define MAX_BURSTLENGTH 16
#define DMA_MAX_TASKS 256
static const int BURSTLEN_REF[BURSTLEN_COUNT] = {1, 2, 4};

/// @internal DMA storage arrays
static __attribute__ ((section(".hsram"))) __attribute__ ((aligned (16))) 
  DmacDescriptor wbDescArray[DMAC_CH_NUM];

static __attribute__ ((section(".hsram"))) __attribute__ ((aligned (16))) 
  DmacDescriptor baseDescArray[DMAC_CH_NUM]; 

static core::dma::taskDescriptor *baseTasks[DMAC_CH_NUM] = { nullptr };
static core::dma::errorCallbackType errorCB = nullptr;
static core::dma::transferCallbackType transferCB = nullptr;

namespace core {

  /***********************************************************************************************/
  /// SECTION: DMA CONTROL 

  bool dma::ctrlGroup::init(const bool &value) {
    DMAC->CTRL.bit.DMAENABLE = 0;
    while(DMAC->CTRL.bit.DMAENABLE);

    for (int i = 0; i < DMA_IRQ_COUNT; i++) {
      NVIC_DisableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
      NVIC_ClearPendingIRQ((IRQn_Type)(DMAC_0_IRQn + i));
    }
    if (value) {
      for (int i = 0; i < DMA_IRQ_COUNT; i++) {
        NVIC_SetPriority((IRQn_Type)(DMAC_0_IRQn + i), dma::configGroup::irqPriority);
        NVIC_EnableIRQ((IRQn_Type)(DMAC_0_IRQn + i));
      }
    }
    DMAC->CRCCTRL.reg &= ~DMAC_CRCCTRL_MASK;
    DMAC->CTRL.bit.SWRST = 1;
    while(DMAC->CTRL.bit.SWRST);

    memset((void*)wbDescArray, 0, sizeof(wbDescArray));
    memset((void*)baseDescArray, 0, sizeof(baseDescArray));
    DMAC->BASEADDR.reg = (uint32_t)baseDescArray;
    DMAC->WRBADDR.reg = (uint32_t)wbDescArray;

    if (value) {
      for (int i = 0; i < DMA_PRILVL_COUNT; i++) {
        DMAC->CTRL.reg |= 
          (dma::configGroup::prilvl_enabled[i] << (DMAC_CTRL_LVLEN0_Pos + i));    
        DMAC->PRICTRL0.reg |= 
          (dma::configGroup::prilvl_rr_mode[i] << (DMAC_PRICTRL0_RRLVLEN0_Pos + i * 8))
        | (dma::configGroup::prilvl_service_qual[i] << (DMAC_PRICTRL0_QOS0_Pos + i * 8));
      }
    }
    return true;
  }
  bool dma::ctrlGroup::init() {
    return DMAC->BASEADDR.bit.BASEADDR && DMAC->WRBADDR.bit.WRBADDR;
  }

  bool dma::ctrlGroup::enabled(const bool &value) {
    if (!dma::ctrlGroup::init()) {
      return false;
    }
    DMAC->CTRL.bit.DMAENABLE = value;
    while(DMAC->CTRL.bit.DMAENABLE != value);
    return true;
  } 
  bool dma::ctrlGroup::enabled() {
    return DMAC->CTRL.bit.DMAENABLE;
  }

  bool dma::ctrlGroup::errorCallback(errorCallbackType &errorCallback) {
    errorCB = errorCallback;
    if (errorCallback) {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENSET.bit.TERR = 1;
      }
    } else {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTFLAG.bit.TERR = 1;
        DMAC->Channel[i].CHINTENCLR.bit.TERR = 1;
      }
    }
  }
  bool dma::ctrlGroup::errorCallback() {
    return errorCB;
  }

  bool dma::ctrlGroup::transferCallback(transferCallbackType &transferCallback) {
    transferCB = transferCallback; 
    if (transferCallback && !transferCB) {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENSET.bit.TCMPL = 1;
      }
    } else if (!transferCallback) {
      for (int i = 0; i < DMAC_CH_NUM; i++) {
        DMAC->Channel[i].CHINTENCLR.bit.TCMPL = 1;
        DMAC->Channel[i].CHINTFLAG.bit.TCMPL = 1;
      }
    }
    return true;
  }
  bool dma::ctrlGroup::transferCallback() {
    return transferCB;
  }

  /***********************************************************************************************/
  /// SECTION: ACTIVE CHANNEL 

  int dma::activeChannelGroup::remainingBytes() {
    return DMAC->ACTIVE.bit.BTCNT * DMAC->ACTIVE.bit.ABUSY
      * wbDescArray[DMAC->ACTIVE.bit.ID].BTCTRL.bit.BEATSIZE;
  }

  int dma::activeChannelGroup::index() {
    return DMAC->ACTIVE.bit.ID; 
  }

  bool dma::activeChannelGroup::busy() {
    return DMAC->ACTIVE.bit.ABUSY; 
  }

  /***********************************************************************************************/
  /// SECTION: CRC

  //// CRC SOURCE CHANNEL
  bool dma::crcGroup::inputChannel(const int &value) {
    if (value >= DMAC_CH_NUM || DMAC->CRCSTATUS.bit.CRCBUSY) {
      return false;
    }
    DMAC->CRCCTRL.bit.CRCSRC = value + 32;
    return DMAC->CRCCTRL.bit.CRCSRC == value + 32;
  }
  int dma::crcGroup::inputChannel() {
    return DMAC->CRCCTRL.bit.CRCSRC - 32;
  }

  bool dma::crcGroup::mode(const CRC_MODE &value) {
    DMAC->CRCCTRL.bit.CRCPOLY = value;
    return DMAC->CRCCTRL.bit.CRCPOLY == value;
  }
  dma::CRC_MODE dma::crcGroup::mode() {
    return static_cast<CRC_MODE>(DMAC->CRCCTRL.bit.CRCPOLY);
  }

  dma::CRC_STATUS dma::crcGroup::status() {
    if (DMAC->CRCCTRL.bit.CRCSRC == DMAC_CRCCTRL_CRCSRC_DISABLE_Val) {
      return CRC_DISABLED;
    } else if (DMAC->CRCSTATUS.bit.CRCERR) {
      return CRC_ERROR;
    } else if (DMAC->CRCSTATUS.bit.CRCBUSY) {
      return CRC_BUSY;
    } else if (DMAC->CRCSTATUS.bit.CRCZERO) {
      return CRC_IDLE;
    }
    return CRC_DISABLED;
  }

  /***********************************************************************************************/
  /// SECTION: CHANNEL

  bool dma::channelGroup::init(const bool &value) {
    DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
    while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
    DMAC->Channel[index].CHCTRLA.bit.SWRST = 1;
    while(DMAC->Channel[index].CHCTRLA.bit.SWRST);

    memset((void*)&baseDescArray[index], 0, sizeof(DmacDescriptor));
    memset((void*)&wbDescArray[index], 0, sizeof(DmacDescriptor));
    if (value) {
      DMAC->Channel[index].CHCTRLA.bit.RUNSTDBY 
        = configGroup::chRunStandby[index];
      DMAC->Channel[index].CHPRILVL.bit.PRILVL
        = configGroup::chPrilvl[index];
      DMAC->Channel[index].CHINTENSET.reg |=
          ((int)errorCB << DMAC_CHINTENSET_TERR_Pos)
        | ((int)transferCB << DMAC_CHINTENSET_TCMPL_Pos);
    }
    return true;
  } 
  bool dma::channelGroup::init() const {
    return memcmp((void*)&baseDescArray[index], (void*)&wbDescArray[index], 
      sizeof(DmacDescriptor)) || DMAC->Channel[index].CHCTRLA.reg 
      != DMAC_CHCTRLA_RESETVALUE;
  }

  bool dma::channelGroup::state(const CHANNEL_STATE &value) {    
    if (value == this->state()) {
      return true;
    }
    switch(value) {
      case STATE_DISABLED: { // DONE
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
        while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
        DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
        DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
        return true;
      }
      case STATE_IDLE: { // DONE
        if (DMAC->Channel[index].CHSTATUS.reg & (DMAC_CHSTATUS_BUSY 
          | DMAC_CHSTATUS_PEND)) {
          DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
          while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
        }
        DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
        DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
        return true;
      }
      case STATE_SUSPENDED: { // DONE
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
        DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
        DMAC->Channel[index].CHCTRLB.bit.CMD == DMAC_CHCTRLB_CMD_SUSPEND_Val;
        while(!DMAC->Channel[index].CHINTFLAG.bit.SUSP);
        return true;
      }
      case STATE_ACTIVE: { // DONE
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
        DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
        DMAC->Channel[index].CHINTFLAG.reg = DMAC_CHINTFLAG_RESETVALUE;
        DMAC->SWTRIGCTRL.reg |= (1 << index);
        return true;
      }
      default: {
        return false;
      }
    }
  }
  dma::CHANNEL_STATE dma::channelGroup::state() const {
    if (!DMAC->Channel[index].CHCTRLA.bit.ENABLE) {
      return STATE_DISABLED;
    } else if (DMAC->Channel[index].CHINTFLAG.bit.SUSP) {
      return STATE_SUSPENDED;
    } else if (DMAC->Channel[index].CHSTATUS.bit.BUSY 
      || DMAC->Channel[index].CHSTATUS.bit.PEND) {
      return STATE_ACTIVE;
    }
    return STATE_IDLE;    
  }

  // LINKED PERIPHERAL TRIGGER
  bool dma::channelGroup::linkedPeripheral(const PERIPHERAL_LINK &value) {
    DMAC->Channel[index].CHCTRLA.bit.TRIGSRC = value;
    return DMAC->Channel[index].CHCTRLA.bit.TRIGSRC == value;
  }
  dma::PERIPHERAL_LINK dma::channelGroup::linkedPeripheral() const{
    return static_cast<PERIPHERAL_LINK>(DMAC->Channel[index]
      .CHCTRLA.bit.TRIGSRC);
  }

  //// TRANSFER MODE
  bool dma::channelGroup::transferMode(const TRANSFER_MODE &value) {
    if (value == MODE_TRANSFER_TASK) {
      DMAC->Channel[index].CHCTRLA.bit.TRIGACT 
        = DMAC_CHCTRLA_TRIGACT_BLOCK_Val;
      return DMAC->Channel[index].CHCTRLA.bit.TRIGACT
        == DMAC_CHCTRLA_TRIGACT_BLOCK_Val;

    } else if (value == MODE_TRANSFER_ALL) {
      DMAC->Channel[index].CHCTRLA.bit.TRIGACT 
        = DMAC_CHCTRLA_TRIGACT_TRANSACTION_Val;
      return DMAC->Channel[index].CHCTRLA.bit.TRIGACT
        == DMAC_CHCTRLA_TRIGACT_TRANSACTION_Val;
    } 
    DMAC->Channel[index].CHCTRLA.bit.TRIGACT = DMAC_CHCTRLA_TRIGACT_BURST_Val;
    DMAC->Channel[index].CHCTRLA.bit.BURSTLEN = value 
      - (1 - DMAC_CHCTRLA_BURSTLEN_SINGLE_Val);
    return DMAC->Channel[index].CHCTRLA.bit.TRIGACT
      == DMAC_CHCTRLA_TRIGACT_BURST_Val;
  }

  //// CHANNEL ERROR
  dma::CHANNEL_ERROR dma::channelGroup::error() const {
    if (DMAC->Channel[index].CHINTFLAG.bit.TERR) {
      if (DMAC->Channel[index].CHSTATUS.bit.FERR) {
        return ERROR_DESC;
      }
      return ERROR_TRANSFER;
    } else if (DMAC->Channel[index].CHSTATUS.bit.CRCERR) {
      return ERROR_CRC;
    } 
    return ERROR_NONE;
  }

  //// WRITEBACK DESCRIPTOR
  dma::taskDescriptor dma::channelGroup::writebackDescriptor() {
    return dma::taskDescriptor(&wbDescArray[index]);
  }

  //// GET TASK
  dma::taskDescriptor &dma::channelGroup::getTask(const int &taskIndex) {
    dma::taskDescriptor *current = baseTasks[index];
    for (int i = 0; i < taskIndex; i++) {
      if (!current->linked || current->linked == baseTasks[index]) {
        return *current;
      }
      current = current->linked;
    }
    return *current;
  }

  //// GET TASK COUNT
  int dma::channelGroup::taskCount() {
    dma::taskDescriptor *current = baseTasks[index];
    if (!current) {
      return 0;
    }
    int count = 1;
    while(current->linked && current->linked != baseTasks[index]) {
      current = current->linked;
      count++;
    }
    return count;
  }

  //// ADD TASK
  bool dma::channelGroup::addTask(const int &reqIndex, dma::taskDescriptor &task) {    
    int taskIndex = clamp(reqIndex, 0, DMA_MAX_TASKS);
    if (task.assignedCh != -1 || task.linked) {
      return false;
    }
    task.assignedCh = index;
    task.linked = nullptr;
    task.desc->DESCADDR.bit.DESCADDR = 0;

    bool suspFlag = false;
    if (!DMAC->Channel[index].CHINTFLAG.bit.SUSP) {
      suspFlag = true;
      DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;
      while(DMAC->Channel[index].CHCTRLB.bit.CMD 
        == DMAC_CHCTRLB_CMD_SUSPEND_Val);
    }
    if (taskIndex) {
      dma::taskDescriptor *current = baseTasks[index];
      dma::taskDescriptor *prev = nullptr;

      for (int i = 0; i < taskIndex; i++) {
        if (!current->linked || current == baseTasks[index]) {
          task.desc->DESCADDR.bit.DESCADDR 
            = current->desc->DESCADDR.bit.DESCADDR;
          current->desc->DESCADDR.bit.DESCADDR = (uintptr_t)task.desc;
          task.linked = current;
          current->linked = &task;
          break;
        }
        prev = current;
        current = current->linked;
      }
      prev->desc->DESCADDR.bit.DESCADDR = (uintptr_t)task.desc;
      task.desc->DESCADDR.bit.DESCADDR = (uintptr_t)current->desc;
      prev->linked = &task;
      task.linked = current;
    } else {
      dma::taskDescriptor *nextTask = nullptr;
      if (baseTasks[index] != nullptr) {
        nextTask = baseTasks[index];
        nextTask->desc = new DmacDescriptor;
        nextTask->alloc = true;
        memcpy(nextTask->desc, &baseDescArray[index], sizeof(DmacDescriptor));

        task.desc->DESCADDR.bit.DESCADDR = (uintptr_t)nextTask->desc;
        task.linked = nextTask;
      }
      memcpy(&baseDescArray[index], task.desc, sizeof(DmacDescriptor));
      if (task.alloc) {
        delete task.desc;
      }
      task.alloc = false;
      task.desc = &baseDescArray[index];
      baseTasks[index] = &task;
    }  
    if (suspFlag) {
      DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_RESUME_Val;
    }
    return true;
  }

  //// SET TASKS
  bool dma::channelGroup::setTasks(std::initializer_list<dma::taskDescriptor&> 
    taskList) {
    bool enableFlag = false;
    if (DMAC->Channel[index].CHCTRLA.bit.ENABLE) {
      enableFlag = true;
      DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
      while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
    }
    if (!clearTasks()) { 
      return false; 
    }
    dma::taskDescriptor *prev = nullptr;
    for (dma::taskDescriptor &task : taskList) {
      if (task.assignedCh != -1 || task.linked) {
        return false;
      } 
      task.assignedCh = index;     
      if (!prev) {
        prev = &task;
        memcpy(&baseDescArray[index], task.desc, sizeof(DmacDescriptor));
        if (task.alloc) {
          delete task.desc;
        }
        task.alloc = false;
        task.desc = &baseDescArray[index];
        baseTasks[index] = &task;
        continue;
      }
      prev->desc->DESCADDR.bit.DESCADDR = (uintptr_t)task.desc;
      task.desc->DESCADDR.bit.DESCADDR = 0;
      prev->linked = &task;
    }
    if (enableFlag) {
      DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
    }
    return true;
  }

  dma::taskDescriptor &dma::channelGroup::removeTask(const int &reqIndex) {
    int taskIndex = clamp(reqIndex, 0, DMA_MAX_TASKS);
    dma::taskDescriptor *removed = nullptr;

    if (!taskIndex) {
      removed = baseTasks[index];
      assert(removed && removed->assignedCh == index);
      removed->desc = new DmacDescriptor;
      memcpy(removed->desc, &baseDescArray[index], sizeof(DmacDescriptor));

      removed->alloc = true;
      removed->assignedCh = -1;
      removed->desc->DESCADDR.bit.DESCADDR = 0;

      if (removed->linked) {
        dma::taskDescriptor *newBase = removed->linked;
        assert(newBase && newBase->assignedCh == index);
        memcpy(&baseDescArray[index], newBase->desc, sizeof(DmacDescriptor));
        if (newBase->alloc) {
          delete newBase->desc;
        }
        newBase->alloc = false;
        newBase->desc = &baseDescArray[index];
      }
    } else {
      removed = baseTasks[index];
      dma::taskDescriptor *prev = nullptr;
      for (int i = 0; i < taskIndex; i++) {
        if (!removed->linked || removed->linked == baseTasks[index]) {
          break;
        }
        prev = removed;
        removed = removed->linked;
      }
      prev->desc->DESCADDR.bit.DESCADDR 
        = removed->desc->DESCADDR.bit.DESCADDR;
      prev->linked = removed->linked;
      removed->linked = nullptr;
      removed->assignedCh = -1; 
    }
    return *removed;
  }

  //// CLEAR TASKS
  bool dma::channelGroup::clearTasks() {
    auto resetTask = [&](dma::taskDescriptor *task) {
      task->assignedCh = -1;
      task->linked = nullptr;
      task->desc->DESCADDR.bit.DESCADDR = 0;
    };
    if (baseTasks[index]) {
      dma::taskDescriptor *current = baseTasks[index]->linked;
      dma::taskDescriptor *next = nullptr;
      
      if (DMAC->Channel[index].CHCTRLA.bit.ENABLE) {
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
        while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
      }
      baseTasks[index]->desc = new DmacDescriptor;
      memcpy(&baseTasks[index]->desc, &baseDescArray[index], 
        sizeof(DmacDescriptor));
      baseTasks[index]->alloc = true;
      resetTask(baseTasks[index]);

      while(current->linked && current->linked != baseTasks[index]) {
        next = current->linked;
        resetTask(next);
      }
      memset(&baseDescArray[index], 0, sizeof(DmacDescriptor));
      baseTasks[index] = nullptr;
    }
    return true;
  }

  /***********************************************************************************************/
  /// SECTION: TASK DESCRIPTOR 

  //// CONSTRUCTORS
  dma::taskDescriptor::taskDescriptor() {
    desc = new DmacDescriptor;
    alloc = true;
    srcAlign = -1;
    destAlign = -1;
  }
  dma::taskDescriptor::taskDescriptor(DmacDescriptor *desc) {
    this->desc = desc;
    alloc = false;
    srcAlign = -1;
    destAlign = -1;
  }
  dma::taskDescriptor::taskDescriptor(const taskDescriptor &other) {
    desc = other.desc;
    alloc = other.alloc;
    srcAlign = other.srcAlign;
    destAlign = other.destAlign;
  }

  //// OPERATORS
  dma::taskDescriptor dma::taskDescriptor::operator=  
    (const taskDescriptor &other) {
    srcAlign = other.srcAlign;
    destAlign = other.destAlign;
    memcpy(&desc, &other.desc, sizeof(DmacDescriptor));
  }
  dma::taskDescriptor::operator DmacDescriptor*() {
    return desc;
  }
  dma::taskDescriptor::operator bool() const {
    return desc->BTCTRL.bit.VALID;
  }

  //// RESET
  void dma::taskDescriptor::reset() {
    memset(&desc, 0, sizeof(DmacDescriptor));
    srcAlign = -1;
    destAlign = -1;
  }

  //// TASK ENABLED
  bool dma::taskDescriptor::enabled(const bool &value) {
    desc->BTCTRL.bit.VALID = value && desc->SRCADDR.bit.SRCADDR != 0
      && desc->DSTADDR.bit.DSTADDR != 0 && desc->BTCNT.bit.BTCNT != 0;
    return desc->BTCTRL.bit.VALID == value;
  }
  bool dma::taskDescriptor::enabled() const {
    return desc->BTCTRL.bit.VALID;
  }

  //// TRANSFER DESTINATION/SOURCE
  bool dma::taskDescriptor::setDesc_(const void *value, const int &size, 
    const bool &isVol, const int &index, const bool &isSrc) {
    if (!value) {
      if (isSrc) {
        desc->SRCADDR.bit.SRCADDR = DMAC_SRCADDR_RESETVALUE;
        desc->BTCTRL.bit.SRCINC = 0;
        srcAlign = 0;
      } else {
        desc->DSTADDR.bit.DSTADDR = DMAC_SRCADDR_RESETVALUE;
        desc->BTCTRL.bit.DSTINC = 0;
        destAlign = 0;
      }
      return true;
    }
    bool validAlign = false;
    for (int i = 0; i < BURSTLEN_COUNT; i++) {
      if (size % BURSTLEN_REF[i] == 0) {
        if (isSrc) {
          srcAlign = BURSTLEN_REF[i];
        } else {
          destAlign = BURSTLEN_REF[i];
        }
        validAlign = true;
      }
    }
    if (validAlign) {
      bool enableFlag = false;
      if (assignedCh != -1 && DMAC->Channel[assignedCh].CHCTRLA.bit.ENABLE) {
        DMAC->Channel[assignedCh].CHCTRLA.bit.ENABLE = 0;
        while(DMAC->Channel[assignedCh].CHCTRLA.bit.ENABLE);
      }
      if (isSrc) {
        desc->BTCTRL.bit.SRCINC = (bool)(index > 1);
      } else {
        desc->BTCTRL.bit.DSTINC = (bool)(index > 1);
      }
      if (srcAlign && destAlign) {
        desc->BTCTRL.bit.BEATSIZE == srcAlign < destAlign 
          ? srcAlign : destAlign;

        if (desc->BTCTRL.bit.SRCINC && srcAlign 
          < desc->BTCTRL.bit.BEATSIZE) {
          desc->BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
          desc->BTCTRL.bit.STEPSIZE = desc->BTCTRL.bit.BEATSIZE / srcAlign;

        } else if (desc->BTCTRL.bit.DSTINC && destAlign 
          < desc->BTCTRL.bit.BEATSIZE) {
          desc->BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;
          desc->BTCTRL.bit.STEPSIZE = desc->BTCTRL.bit.BEATSIZE / destAlign;
        }
      }
      if ((uintptr_t)value == (uintptr_t)&dma::crcGroup::CRC_INPUT) {
        if (!isSrc) {
          desc->DSTADDR.bit.DSTADDR = (uintptr_t)REG_DMAC_CRCDATAIN;
        } else {
          return false;
        }
      } else if ((uintptr_t)value == (uintptr_t)&dma::crcGroup::CRC_OUTPUT) {
        if (isSrc) {
          desc->SRCADDR.bit.SRCADDR = (uintptr_t)REG_DMAC_CRCCHKSUM; 
        } else {
          return false;
        }
      } else {
        uintptr_t addr = (uintptr_t)&value;
        if (!isVol) {
          addr += desc->BTCNT.bit.BTCNT * (desc->BTCTRL.bit.BEATSIZE + 1) 
            * (1 << desc->BTCTRL.bit.STEPSIZE * (desc->BTCTRL.bit.STEPSEL 
            == isSrc ? DMAC_BTCTRL_STEPSEL_SRC_Val : DMAC_BTCTRL_STEPSEL_DST_Val));
        }
        if (isSrc) {
          desc->SRCADDR.bit.SRCADDR = addr;
          return desc->SRCADDR.bit.SRCADDR == addr;
        } else {
          desc->DSTADDR.bit.DSTADDR = addr;
          return desc->DSTADDR.bit.DSTADDR == addr;
        }
      }
      if (enableFlag) {
        DMAC->Channel[assignedCh].CHCTRLA.bit.ENABLE = 1;
      }
      return true;
    }
    return false;
  }
  void *dma::taskDescriptor::source() const {
    return (void*)desc->SRCADDR.bit.SRCADDR;
  }
  void *dma::taskDescriptor::destination() const {
    return (void*)desc->DESCADDR.bit.DESCADDR;
  }

  //// TRANSFER LENGTH
  bool dma::taskDescriptor::length(const int &value) {
    int prevLength = desc->BTCNT.bit.BTCNT;
    desc->BTCNT.bit.BTCNT = value;
    if (desc->SRCADDR.bit.SRCADDR) {
      uintptr_t baseAddr = desc->SRCADDR.bit.SRCADDR - prevLength 
        * (desc->BTCTRL.bit.BEATSIZE + 1) * (1 << desc->BTCTRL.bit.STEPSIZE 
        * (desc->BTCTRL.bit.STEPSEL == DMAC_BTCTRL_STEPSEL_SRC_Val));
      desc->SRCADDR.bit.SRCADDR = baseAddr +
          (desc->BTCTRL.bit.BEATSIZE + 1) * (1 << desc->BTCTRL.bit.STEPSIZE 
        * (desc->BTCTRL.bit.STEPSEL == DMAC_BTCTRL_STEPSEL_SRC_Val));
    }
    if (desc->DSTADDR.bit.DSTADDR) {
      uintptr_t baseAddr = desc->DSTADDR.bit.DSTADDR - prevLength 
        * (desc->BTCTRL.bit.BEATSIZE + 1) * (1 << desc->BTCTRL.bit.STEPSIZE 
        * (desc->BTCTRL.bit.STEPSEL == DMAC_BTCTRL_STEPSEL_DST_Val));
      desc->DSTADDR.bit.DSTADDR = baseAddr + desc->BTCNT.bit.BTCNT 
        * (desc->BTCTRL.bit.BEATSIZE + 1) * (1 << desc->BTCTRL.bit.STEPSIZE 
        * (desc->BTCTRL.bit.STEPSEL == DMAC_BTCTRL_STEPSEL_DST_Val));
    }
    return desc->BTCNT.bit.BTCNT == value;
  }
  int dma::taskDescriptor::length() const {
    return desc->BTCNT.bit.BTCNT;
  }

  //// SUSPEND CHANNNEL
  bool dma::taskDescriptor::suspendChannel(const bool &value) {
    const int regVal = value ? DMAC_BTCTRL_BLOCKACT_SUSPEND_Val 
      : DMAC_BTCTRL_BLOCKACT_NOACT_Val;
    desc->BTCTRL.bit.BLOCKACT = regVal;
    return desc->BTCTRL.bit.BLOCKACT == regVal; 
  }
  bool dma::taskDescriptor::suspendChannel() const {
    return desc->BTCTRL.bit.BLOCKACT == DMAC_BTCTRL_BLOCKACT_SUSPEND_Val;
  }

  //// MISC
  dma::taskDescriptor &dma::taskDescriptor::linkedTask() {
    return *linked;
  }
  int dma::taskDescriptor::assignedChannel() {
    return assignedCh;
  }

  //// DESTRUCTOR
  dma::taskDescriptor::~taskDescriptor() {   
    if (alloc) {
      delete desc;
    } 
  }


} // NAMESPACE DMA




