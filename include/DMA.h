/*

///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: DIRECT MEMORY ACCESS CONTROLLER (HEADER)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "sam.h"
#include "inttypes.h"
#include "initializer_list"

#include "SYS.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: DMA FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dma {

  /// @brief Specifies the settings for a channel.
  typedef struct ChannelDescriptor {
    unsigned int transferThreshold = 1;
    unsigned int burstLength = 0;
    unsigned int priorityLvl = 2;
    bool runInStandby = true;
  };
    
  /// @brief Denotes the current state of a DMA channel
  enum DMACH_STATE {
    DMACH_STATE_UNKNOWN,
    DMACH_STATE_DISABLED,
    DMACH_STATE_IDLE,
    DMACH_STATE_BUSY,
    DMACH_STATE_PEND,
  };

  /// @brief Denotes errors that can occur 
  enum DMACH_ERROR {
    DMACH_ERROR_NONE,
    DMACH_ERROR_UNKNOWN,
    DMACH_ERROR_CRC,
    DMACH_ERROR_DESC,
    DMACH_ERROR_TRANSFER
  };

  /// @brief System-wide dma settings
  struct {
    bool roundRobinMode = false;
    unsigned int serviceQuality = 2;
    unsigned int irqPriority = 1;
    void (*errorCallback)(unsigned int, DMACH_ERROR) = nullptr;
    void (*transferCallback)(unsigned int) = nullptr;
  }sys_config;


  /// @brief Initializes the DMA system
  /// @return True if initialization was successful, false otherwise
  bool sys_init(); 

  /// @brief Resets/disables the DMA system
  /// @return True if the exit was successful, false otherwise
  bool sys_exit(); 

  /// @brief Updates the dma system with the settings in "sys_config"
  /// @return True if the system was updated successfuly and false otherwise,
  ///   Note: method with return false if parameter in sys_config is invalid (OOB)
  bool sys_update_config(); 

  /// @brief  Gets the channel number of the dma channel that is currently busy
  /// @return The number of the currently busy channel.
  unsigned int sys_get_active_channel(); 

  /// @brief Gets the number of blocks remaining in the currently active transfer
  /// @return The number of blocks remaining in active transfer
  unsigned int sys_get_active_blockCount(); 

  /// @brief Enables a specific channel
  /// @param channelNum The ID number of the channel to enable
  /// @return True if channel was successfuly enabled, false otherwise 
  bool channel_enable(unsigned int channelNum); 

  /// @brief Disables a specific channel
  /// @param channelNum The ID number of the channel to disable
  /// @return True if the channel was successfuly disabled
  bool channel_disable(unsigned int channelNum); 

  /// @brief Reset a channel to it's initial settings/state
  /// @param channelNum The ID number of the channel to reset
  /// @return True if the reset was successful, false otherwise
  bool channel_reset(unsigned int channelNum); 

  /// @brief Sets the channel descriptor for a specific channel
  /// @param channelNum The ID number of the channel to target
  /// @param desc The channel descriptor 
  /// @return True if the channel descriptor was successfuly set, false otherwise
  bool channel_set_chd(unsigned int channelNum, ChannelDescriptor &desc); 

  bool channel_set_base_trd(unsigned int channelNum, DmacDescriptor *baseDesc); 

  bool channel_set_trigger(unsigned int channelNum, unsigned int triggerSrc,
    unsigned int triggerAction); 

  bool channel_trigger(unsigned int channelNum); 

  bool channel_set_cmd(unsigned int channelNum, int cmd); 



  DMACH_ERROR channel_get_error(unsigned int channelNum); 

  DMACH_STATE channel_get_state(unsigned int channelNum); 

  DmacDescriptor *channel_get_wbd(unsigned int channelNum); 

  DmacDescriptor *channel_get_base_trd(unsigned int channelNum); 



  const uintptr_t crc_input_addr = (uintptr_t)DMAC->CRCDATAIN.reg;

  const uintptr_t crc_output_addr = (uintptr_t)DMAC->CRCCHKSUM.reg;

  bool crc_set_config(unsigned int channelNum, unsigned int crcType, 
    unsigned int crcSize); 

  decltype(DMAC->CRCCHKSUM.reg) crc_get_chksum(); 



  DmacDescriptor *trd_link_group(std::initializer_list<DmacDescriptor*> descList); 

  bool trd_unlink_group(DmacDescriptor *baseDesc); 

  bool trd_insert(DmacDescriptor *baseDesc, DmacDescriptor *insertDesc, 
    unsigned int insertIndex); 

  DmacDescriptor *trd_remove(DmacDescriptor *baseDesc, unsigned int removeIndex); 

  bool trd_set_loop(DmacDescriptor *baseDesc, bool looped); 

  bool trd_factory(DmacDescriptor *desc, void *src, void *dest, 
    unsigned int incrSrc, unsigned int incrDest, unsigned int beatCount, 
      unsigned int beatSize, unsigned int transferAction); 

}

*/