
#pragma once
#include <sam.h>

/// @internal CONFIG
#define ASSERT_ENABLED true

#define clamp(_value_, _min_, _max_) (_value_ < _min_ ? _min_ \ 
  : (_value_ > _max_ ? _max_ : _value_))

#define super(_this_, _super_) explicit _this_(_super_ *super) \
  : super(super) {}; const _super_ *super

template<class inst_t>
inline const int init_seq(inst_t inst, const int &start, const int &incr) {
  static int count = 0;
  return count += count ? incr : start; 
}


void assert(bool statement) {
  if (!statement && ASSERT_ENABLED) {
    __disable_irq();
    __DMB();
    NVIC_SystemReset();
    while(true);
  }
}
void deny(bool statement) {
  assert(!statement);
}



