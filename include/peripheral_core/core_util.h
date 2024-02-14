///////////////////////////////////////////////////////////////////////////////////////////////////
//// FILE: PROPERTY TEMPLATE 
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "type_traits"
#include <sam.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: PROPERTY TYPE DERIVATIVES
///////////////////////////////////////////////////////////////////////////////////////////////////


template<class T, bool id, class Enable = void>
class Property {
  constexpr Property(...) {
    static_assert(true, "Property is ill-formed: Invalid type");
  }
};

template<class T, bool id>  
class Property<T, false, typename std::enable_if<std::is_integral<T>::value>::type> {
    constexpr Property(bool (*set)(const T&), T (*get)(void), int *regValue) {}
      : set(set), get(get) {};

    operator T() const {
      return get();
    }
    bool operator = (const T &value) {
      return set(value);
    }
  protected:
    bool (*set)(const T&, const int&);
    T (*get)(const int*);
};

template<class T, bool id>  
class Property<T, false, typename std::enable_if<std::is_enum<T>::value>::type> {
    constexpr Property(bool (*set)(const T&), T (*get)(void)) {}
      : set(set), get(get) {};

    operator T() const {
      return get();
    }
    operator std::underlying_type<T>::type() const {
      return static_cast<std::underlying_type<T>::type>(get());
    }
    bool operator = (const T &value) {
      return set(value);
    }
    bool operator = (const std::underlying_type<T>::type &value) {
      return set(static_cast<const T>(value));
    }
    operator == (const T &value) {
      return value == get();
    }

  protected:
    bool (*set)(const T&);
    T (*get)(void);
};

template<class T, bool id>
class Property<T, true, typename std::enable_if<std::is_integral<T>::value>::type> {
  public: 
    constexpr Property(bool (*set)(const T&), T (*get)(void), int *regValue) {}
      : set(set), get(get) {};

    operator T() const {
      return get();
    }
    bool operator = (const T &value) {
      return set(value);
    }
  protected:
    bool (*set)(const T&, const int&);
    T (*get)(const int*);
}