
#pragma once

template<class inst_t>
inline const int init_seq(inst_t inst, const int &start, const int &incr) {
  static int count = 0;
  return count += count ? incr : start; 
}



