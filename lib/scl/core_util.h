
#pragma once
#include <sam.h>
#include <config/util_config.h>

namespace scl::util {

  void assert(bool statement) {
    if (!statement && ASSERT_ENABLED) {
      __DMB();
      __BKPT(2);
      for(;;);
    }
  }


}