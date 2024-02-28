/*

#include "dma_core.h"

namespace {

  #define DMA_PRILVL_COUNT 4
  #define DMA_IRQ_COUNT 5
  #define DMA_IRQPRI_MAX 5
  #define DMA_QOS_MAX 3
  #define MAX_BURSTLENGTH 16
  #define DMA_MAX_TASKS 256

  static const int BURSTLEN_REF[] = {1, 2, 4};

  static DmacDescriptor wbDescArray[DMAC_CH_NUM] SECTION_DMAC_DESCRIPTOR __ALIGNED(16);
  static DmacDescriptor baseDescArray[DMAC_CH_NUM] SECTION_DMAC_DESCRIPTOR __ALIGNED(16);
  static dma::taskDescriptor *baseTasks[DMAC_CH_NUM] = { nullptr };
  static dma::errorCallbackType errorCB = nullptr;
  static dma::transferCallbackType transferCB = nullptr;

}



namespace dma {

  namespace sys {

    bool setInit(const bool &value) {
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
            (configGroup::prilvl_enabled[i] << (DMAC_CTRL_LVLEN0_Pos + i));
          DMAC->PRICTRL0.reg |=
            (configGroup::prilvl_rr_mode[i] << (DMAC_PRICTRL0_RRLVLEN0_Pos + i * 8))
          | (configGroup::prilvl_service_qual[i] << (DMAC_PRICTRL0_QOS0_Pos + i * 8));
        }
      }
      return true;
    }
    bool getInit() {
      return DMAC->BASEADDR.bit.BASEADDR && DMAC->WRBADDR.bit.WRBADDR;
    }

    bool setEnabled(const bool &value) {
      if (!getInit()) {
        return false;
      }
      DMAC->CTRL.bit.DMAENABLE = value;
      while(DMAC->CTRL.bit.DMAENABLE != value);
      return true;
    }
    bool getEnabled() {
      return DMAC->CTRL.bit.DMAENABLE;
    }

    bool setErrorCallback(errorCallbackType &errorCallback) {
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
      return true;
    }
    errorCallbackType getErrorCallback() {
      return errorCB;
    }

    bool setTransferCallback(transferCallbackType &transferCallback) {
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
    bool getTransferCallback() {
      return transferCB;
    }

  }

  namespace ach {

    int getbytes() {
      return DMAC->ACTIVE.bit.BTCNT * DMAC->ACTIVE.bit.ABUSY
        * wbDescArray[DMAC->ACTIVE.bit.ID].BTCTRL.bit.BEATSIZE;
    }

    int getIndex() {
      return DMAC->ACTIVE.bit.ID;
    }

    bool getBusy() {
      return DMAC->ACTIVE.bit.ABUSY;
    }

  }

  namespace crc {

    bool setInputChannel(const int &value) {
      if (value >= DMAC_CH_NUM || DMAC->CRCSTATUS.bit.CRCBUSY) {
        return false;
      }
      DMAC->CRCCTRL.bit.CRCSRC = value + 32;
      return DMAC->CRCCTRL.bit.CRCSRC == value + 32;
    }
    int getInputChannel() {
      return DMAC->CRCCTRL.bit.CRCSRC - 32;
    }

    bool setMode(const CRC_MODE &value) {
      DMAC->CRCCTRL.bit.CRCPOLY = value;
      return DMAC->CRCCTRL.bit.CRCPOLY == value;
    }
    CRC_MODE getMode() {
      return static_cast<CRC_MODE>(DMAC->CRCCTRL.bit.CRCPOLY);
    }

    CRC_STATUS getStatus() {
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

  }

  namespace ch {

    bool setInit(const bool &value, const int &index) {
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
    bool getInit(const int &index) {
      return memcmp((void*)&baseDescArray[index], (void*)&wbDescArray[index],
        sizeof(DmacDescriptor)) || DMAC->Channel[index].CHCTRLA.reg
        != DMAC_CHCTRLA_RESETVALUE;
    }

    bool setState(const CHANNEL_STATE &value, const int &index) {
      if (value == getState(index)) {
        return true;
      }
      switch(value) {
        case STATE_DISABLED: {
          DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
          while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
          DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_NOACT_Val;
          DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
          return true;
        }
        case STATE_IDLE: {
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
        case STATE_SUSPENDED: {
          DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
          DMAC->Channel[index].CHINTFLAG.bit.SUSP = 1;
          DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;
          while(!DMAC->Channel[index].CHINTFLAG.bit.SUSP);
          return true;
        }
        case STATE_ACTIVE: {
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
    CHANNEL_STATE getState(const int &index) {
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

    bool setPeripheral(const PERIPHERAL_LINK &value, const int &index) {
      DMAC->Channel[index].CHCTRLA.bit.TRIGSRC = value;
      return DMAC->Channel[index].CHCTRLA.bit.TRIGSRC == value;
    }
    PERIPHERAL_LINK getPeripheral(const int &index) {
      return static_cast<PERIPHERAL_LINK>(DMAC->Channel[index]
        .CHCTRLA.bit.TRIGSRC);
    }

    bool setTransferMode(const TRANSFER_MODE &value, const int &index) {
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

    CHANNEL_ERROR getError(const int &index) {
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

    taskDescriptor getCurrentTask(const int &index) {
      return dma::taskDescriptor(&wbDescArray[index]);
    }

    taskDescriptor &getTask(const int &taskIndex, const int &index) {
      taskDescriptor *current = ::baseTasks[index];
      for (int i = 0; i < taskIndex; i++) {
        if (!current->getLinkedTask() || current->getLinkedTask() 
          == ::baseTasks[index]) {
          return *current;
        }
        current = current->getLinkedTask();
      }
      return *current;
    }

    int indexOf(const taskDescriptor &task, const int &index) {
      int currIndex = 0;
      taskDescriptor *current = ::baseTasks[index];
      if (!current) {
        if (current == &task) {
          return 0;
        }
        while(current->linked && current->linked != ::baseTasks[index]) {
          current = current->linked;
          currIndex++;
          if (current == &task) {
            return currIndex;
          }
        }
      }
      return -1;
    }

    int getTaskCount(const int &index) {
      taskDescriptor *current = ::baseTasks[index];
      if (!current) {
        return 0;
      }
      int count = 0;
      while(current->linked && current->linked != ::baseTasks[index]) {
        current = current->linked;
        count++;
      }
      return count + 1;
    }

    bool addTask(const int &reqIndex, taskDescriptor &task, const int &index) {
      int taskIndex = clamp(reqIndex, 0, DMA_MAX_TASKS);
      if (task.info.assignedCh != -1 || task.linked) {
        return false;
      }
      task.info.assignedCh = index;
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
        taskDescriptor *current = ::baseTasks[index];
        taskDescriptor *prev = nullptr;

        for (int i = 0; i < taskIndex; i++) {
          if (!current->linked || current == ::baseTasks[index]) {
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
        taskDescriptor *nextTask = nullptr;
        if (::baseTasks[index] != nullptr) {
          nextTask = ::baseTasks[index];
          nextTask->desc = new DmacDescriptor;
          nextTask->info.alloc = true;
          memcpy(nextTask->desc, &baseDescArray[index], sizeof(DmacDescriptor));

          task.desc->DESCADDR.bit.DESCADDR = (uintptr_t)nextTask->desc;
          task.linked = nextTask;
        }
        memcpy(&baseDescArray[index], task.desc, sizeof(DmacDescriptor));
        if (task.info.alloc) {
          delete task.desc;
        }
        task.info.alloc = false;
        task.desc = &baseDescArray[index];
        ::baseTasks[index] = &task;
      }
      if (suspFlag) {
        DMAC->Channel[index].CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_RESUME_Val;
      }
      return true;
    }

    bool setTasks(std::initializer_list<taskDescriptor*>
      taskList, const int &index) {
      bool enableFlag = false;
      if (DMAC->Channel[index].CHCTRLA.bit.ENABLE) {
        enableFlag = true;
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
        while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
      }
      if (!clearTasks()) {
        return false;
      }
      taskDescriptor *prev = nullptr;
      for (taskDescriptor *task : taskList) {
        if (task->info.assignedCh != -1 || task->linked) {
          return false;
        }
        task->info.assignedCh = index;
        if (!prev) {
          prev = task;
          memcpy(&baseDescArray[index], task->desc, sizeof(DmacDescriptor));
          if (task->info.alloc) {
            delete task->desc;
          }
          task->info.alloc = false;
          task->desc = &baseDescArray[index];
          ::baseTasks[index] = task;
          continue;
        }
        prev->desc->DESCADDR.bit.DESCADDR = (uintptr_t)task->desc;
        task->desc->DESCADDR.bit.DESCADDR = 0;
        prev->linked = task;
      }
      if (enableFlag) {
        DMAC->Channel[index].CHCTRLA.bit.ENABLE = 1;
      }
      return true;
    }

    taskDescriptor &removeTask(const int &reqIndex, const int &index) {
      int taskIndex = clamp(reqIndex, 0, DMA_MAX_TASKS);
      taskDescriptor *removed = nullptr;

      if (!taskIndex) {
        removed = baseTasks[index];
        //assert(removed && removed->assignedCh == index); //////////////////////////////// TO DO
        removed->desc = new DmacDescriptor;
        memcpy(removed->desc, &baseDescArray[index], sizeof(DmacDescriptor));

        removed->info.alloc = true;
        removed->info.assignedCh = -1;
        removed->desc->DESCADDR.bit.DESCADDR = 0;

        if (removed->linked) {
          taskDescriptor *newBase = removed->linked;
          //assert(newBase && newBase->assignedCh == index); /////////////////////////////// TO DO
          memcpy(&baseDescArray[index], newBase->desc, sizeof(DmacDescriptor));
          if (newBase->info.alloc) {
            delete newBase->desc;
          }
          newBase->info.alloc = false;
          newBase->desc = &baseDescArray[index];
        }
      } else {
        removed = baseTasks[index];
        taskDescriptor *prev = nullptr;
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
        removed->info.assignedCh = -1;
      }
      return *removed;
    }

    bool clearTasks(const int &index) {
      auto resetTask = [&](taskDescriptor *task) {
        task->info.assignedCh = -1;
        task->linked = nullptr;
        task->desc->DESCADDR.bit.DESCADDR = 0;
      };
      if (baseTasks[index]) {
        taskDescriptor *current = baseTasks[index]->linked;
        taskDescriptor *next = nullptr;

        if (DMAC->Channel[index].CHCTRLA.bit.ENABLE) {
          DMAC->Channel[index].CHCTRLA.bit.ENABLE = 0;
          while(DMAC->Channel[index].CHCTRLA.bit.ENABLE);
        }
        baseTasks[index]->desc = new DmacDescriptor;
        memcpy(&baseTasks[index]->desc, &baseDescArray[index],
          sizeof(DmacDescriptor));
        baseTasks[index]->info.alloc = true;
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

  } 

  taskDescriptor::taskDescriptor() {
    desc = new DmacDescriptor;
    info.alloc = true;
    info.srcAlign = -1;
    info.destAlign = -1;
  }
  taskDescriptor::taskDescriptor(DmacDescriptor *desc) {
    this->desc = desc;
    info.alloc = false;
    info.srcAlign = -1;
    info.destAlign = -1;
  }
  taskDescriptor::taskDescriptor(const taskDescriptor &other) {
    desc = other.desc;
    info.alloc = other.info.alloc;
    info.srcAlign = other.info.srcAlign;
    info.destAlign = other.info.destAlign;
  }

  taskDescriptor &taskDescriptor::operator=
    (const taskDescriptor &other) {
    info.srcAlign = other.info.srcAlign;
    info.destAlign = other.info.destAlign;
    memcpy(&desc, &other.desc, sizeof(DmacDescriptor));
    return *this;
  }
  taskDescriptor::operator DmacDescriptor*() {
    return desc;
  }
  taskDescriptor::operator bool() const {
    return desc->BTCTRL.bit.VALID;
  }

  bool taskDescriptor::setEnabled(const bool &value) {
    desc->BTCTRL.bit.VALID = value && desc->SRCADDR.bit.SRCADDR != 0
      && desc->DSTADDR.bit.DSTADDR != 0 && desc->BTCNT.bit.BTCNT != 0;
    return desc->BTCTRL.bit.VALID == value;
  }
  bool taskDescriptor::getEnabled() const {
    return desc->BTCTRL.bit.VALID;
  }

  bool taskDescriptor::setDesc_(const void *value, const int &size,
    const bool &isVol, const int &index, const bool &isSrc) {
    if (!value) {
      if (isSrc) {
        desc->SRCADDR.bit.SRCADDR = DMAC_SRCADDR_RESETVALUE;
        desc->BTCTRL.bit.SRCINC = 0;
        info.srcAlign = 0;
      } else {
        desc->DSTADDR.bit.DSTADDR = DMAC_SRCADDR_RESETVALUE;
        desc->BTCTRL.bit.DSTINC = 0;
        info.destAlign = 0;
      }
      return true;
    }
    bool validAlign = false;
    for (int i = 0; i < sizeof(BURSTLEN_REF) / sizeof(BURSTLEN_REF[0]); i++) {
      if (size % BURSTLEN_REF[i] == 0) {
        if (isSrc) {
          info.srcAlign = BURSTLEN_REF[i];
        } else {
          info.destAlign = BURSTLEN_REF[i];
        }
        validAlign = true;
      }
    }
    if (validAlign) {
      bool enableFlag = false;
      if (info.assignedCh != -1 && DMAC->Channel[info.assignedCh].CHCTRLA.bit.ENABLE) {
        DMAC->Channel[info.assignedCh].CHCTRLA.bit.ENABLE = 0;
        while(DMAC->Channel[info.assignedCh].CHCTRLA.bit.ENABLE);
      }
      if (isSrc) {
        desc->BTCTRL.bit.SRCINC = (bool)(index > 1);
      } else {
        desc->BTCTRL.bit.DSTINC = (bool)(index > 1);
      }
      if (info.srcAlign && info.destAlign) {
        desc->BTCTRL.bit.BEATSIZE = info.srcAlign < info.destAlign 
          ? info.srcAlign : info.destAlign;

        if (desc->BTCTRL.bit.SRCINC && info.srcAlign
          < desc->BTCTRL.bit.BEATSIZE) {
          desc->BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
          desc->BTCTRL.bit.STEPSIZE = desc->BTCTRL.bit.BEATSIZE / info.srcAlign;

        } else if (desc->BTCTRL.bit.DSTINC && info.destAlign
          < desc->BTCTRL.bit.BEATSIZE) {
          desc->BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;
          desc->BTCTRL.bit.STEPSIZE = desc->BTCTRL.bit.BEATSIZE / info.destAlign;
        }
      }
      if ((uintptr_t)value == (uintptr_t)&crc::CRC_INPUT) {
        if (!isSrc) {
          desc->DSTADDR.bit.DSTADDR = (uintptr_t)REG_DMAC_CRCDATAIN;
        } else {
          return false;
        }
      } else if ((uintptr_t)value == (uintptr_t)&crc::CRC_OUTPUT) {
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
        DMAC->Channel[info.assignedCh].CHCTRLA.bit.ENABLE = 1;
      }
      return true;
    }
    return false;
  }
  void *taskDescriptor::getSource() const {
    return (void*)desc->SRCADDR.bit.SRCADDR;
  }
  void *taskDescriptor::getDestination() const {
    return (void*)desc->DESCADDR.bit.DESCADDR;
  }

  bool taskDescriptor::setLength(const int &value) {
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
  int taskDescriptor::getLength() const {
    return desc->BTCNT.bit.BTCNT;
  }

  bool taskDescriptor::setSuspendChannel(const bool &value) {
    const int regVal = value ? DMAC_BTCTRL_BLOCKACT_SUSPEND_Val
      : DMAC_BTCTRL_BLOCKACT_NOACT_Val;
    desc->BTCTRL.bit.BLOCKACT = regVal;
    return desc->BTCTRL.bit.BLOCKACT == regVal;
  }
  bool taskDescriptor::getSuspendChannel() const {
    return desc->BTCTRL.bit.BLOCKACT == DMAC_BTCTRL_BLOCKACT_SUSPEND_Val;
  }

  void taskDescriptor::reset() {
    memset(&desc, 0, sizeof(DmacDescriptor));
    info.srcAlign = -1;
    info.destAlign = -1;
  }
  taskDescriptor *taskDescriptor::getLinkedTask() {
    return linked;
  }
  int taskDescriptor::getAssignedChannel() {
    return info.assignedCh;
  }

  taskDescriptor::~taskDescriptor() {
    if (info.assignedCh != -1) {
      ch::removeTask(ch::indexOf(*this, info.assignedCh), info.assignedCh);
    }
    if (info.alloc) {
      delete desc;
    }
  }

}

*/


