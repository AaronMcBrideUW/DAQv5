///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: PROPERTY TEMPLATE 
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "type_traits"
#include <sam.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: TEMPLATES 
///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, typename mod = void>
class Property {};

template<typename T>
struct _isBasic {
  static const bool result 
    = std::is_integral<T>::value 
    && !std::is_pointer<T>::value
    && !std::is_same<T, uintptr_t>::value;
};

template<typename T>
class Property<T, typename std::conditional<_isBasic<T>::result, T, void>::type> {
  public:
    Property(bool (&set)(const T&), T (&get)(void)) : set(set), get(get) {};

    bool operator = (const T &value) {
      return set(value);
    }
    operator T() {
      return get();
    }
  private:
    bool (&set)(const T&);
    T (&get)(void);
};

template<typename T>
struct _isEnum {
  static const bool result 
    = std::is_enum<T>::value 
    && !std::is_pointer<T>::value
    && !std::is_same<T, uintptr_t>::value;
};

template<typename T>
class Property<T, typename std::conditional<_isEnum<T>::result, T, void>::type> {
  public:
    Property(bool (&set)(const T&), T (&get)(void)) : set(set), get(get) {}; 

    bool operator = (const T &value) {
      d
    }
    bool operator = (const typename std::underlying_type<T>::type &value) {
      return set((T)value);
    }
    operator T() {
      return get();
    }
    operator typename std::underlying_type<T>::type() {
      return (typename std::underlying_type<T>::type)get();
    }
    bool operator == (const T &value) {
      return (value == get());
    }
  private:
    bool (&set)(const T&);
    T (&get)(void);
};

template<typename T>
struct _isPtr {
  static const bool result 
    = std::is_pointer<T>::value
    && std::is_same<t, uintptr_t>::value
    && !std::is_class<T>::value;
};

/// @brief For addresses
template<typename T>
class Property<T, typename std::conditional<_isPtr<T>::type, T, void>::type> {
  public:
    Property(bool (&set)(T), T (&get)(void)) : set(set), get(get) {}; 

    bool operator = (T value) {
      return set(value);
    }
    bool operator = (const uintptr_t &value) {
      return set((T)value);
    }
    operator T() {
      return get();
    }
    operator uintptr_t() {
      return (uintptr_t)get();
    }
    operator bool() {
      return get() != nullptr;
    }
    bool operator == (const T value) {
      return value = get(); 
    }
    T operator ++ (&this) {
      set(get()--);
      return get();
    }
    T operator -- (&this) {
      set(get()--);
    }
  
  private:
    bool (&set)(const T&);
    T (&get)(void);
};

template<typename T>
class Property<T, typename std::enable_if<std::is_array<T>::value>::type> {
  private:
    bool (&set)(const T&);
    T (&get)(void);
};