///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: HARDWARE TYPE 
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <type_traits>

template<typename val_t, bool indexed>
struct Property {
  constexpr Property(bool (&set)(const val_t&), val_t (&get)(void)) 
    : set(set), get(get) {}
  typedef val_t type;

  inline bool operator = (const val_t &value) {
    return set(value);
  }
  inline operator val_t() const {
    return get();
  }
  protected:
    bool (&set)(const val_t&);
    val_t (&get)(void);
};






























