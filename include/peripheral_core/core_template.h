
#include "type_traits"
#include "utility"

template<typename T>
struct _isBasic {
  static const bool result 
  = std::is_integral<T>::value
    && !std::is_const<T>::value 
    && !std::is_volatile<T>::value
    && !std::is_reference<T>::value;
};
template<typename T>
struct _isEnum {
  static const bool result 
    = std::is_enum<T>::value
    && !std::is_const<T>::value 
    && !std::is_volatile<T>::value
    && !std::is_reference<T>::value;
};
template<typename T>
struct _isPtr {
  static const bool result 
    = std::is_pointer<T>::value
    && !std::is_void<T>::value
    && !std::is_volatile<T>::value
    && !std::is_reference<T>::value;
};



template<typename T>
struct _isVoid {
  static const bool result 
    = std::is_void<T>::value

};



template<typename T, typename X = std::nullptr_t>
class PropertyInterface {
  static_assert(true, "Property interface is ill-formed - Invalid type");
};

template<typename T>
class PropertyInterface<T, typename std::conditional<_isBasic<T>::result, T, void>::type> {
  public:
    PropertyInterface(bool (&set)(const T&), T (&get)(void)) : set(set), get(get);

    bool operator = (const T &value) {
      return set(value);
    }
    bool operator = (const PropertyInterface<T> &other) {
      if (&other == this) {
        return false;
      }
      return set(other.get());
    }
    operator T() {
      return get();
    }
    bool operator == (const PropertyInterface<T> &other) {
      if (&other == this) {
        return true;
      }
      return other.get() == get();
    } 

  private:
    bool (&set)(const T&);
    T (&get)(void);
};

template<typename T>
class PropertyInterface<T, typename std::conditional<_isEnum<T>::result, T, void>::type> {
  public:
    PropertyInterface(bool (&set)(const T&), T (&get)(void)) : set(set), get(get);

    bool operator = (const T &value) {
      return set(value);
    }
    bool operator = (const typename std::underlying_type<T>::type &value) {
      return set(static_cast<const T>(value));
    }
    bool operator = (const PropertyInterface<T> &other) {
      if (&other == this) {
        return false;
      }
      return set(other.get());
    }
    operator T() {
      return get();
    }
    operator typename std::underlying_type<T>::type() {
      return static_cast<typename std::underlying_type<T>::type>(get());
    }
    bool operator == (const T &value) {
      return value == get();
    }
    bool operator == (const PropertyInterface<T> &other) {
      return get() == other.get();
    }

  private:
    bool (&set)(const T&);
    T (&get)(void);
};

template<typename T>
class PropertyInterface<T, typename std::conditional<_isPtr<T>::result, 
  T, void>::type> {
  public:
    PropertyInterface(bool (&set)(T), T (&get)(void), bool nullify) 
      : set(set), get(get), nullify(nullify);

    bool operator = (T &value) {
      bool result = set(value);
      if (result && nullify) {
        value = nullptr;
      }
      return result;
    }
    bool operator = (const typename std::remove_pointer<T>::type &value) {
      if ()
    }
    bool operator = (const PropertyInterface<T> &other) {
      if (&other == this) {
        return false;
      }
      set(other.get());
    }
    operator T() {
      return get();
    }
    typename std::remove_pointer<T>::type operator * (PropertyInterface<T, T>) {
      T *result = get();
      if (!result) {
        // THROW HERE?
      }
      return result;
    }


  private:
    bool (&set)(T);
    T (&get)(void);
    const bool nullify;
};


/*
template<typename T>
class PropertyInterface<T, typename std::conditional<_isVoid<T>::result 
  && _isValid<T>::result, T, void>::type> {
  public:
    PropertyInterface(bool (&set)(const void*), const void* (&get)(void)) : set(set), get(get);

    bool operator = (const void *ptr) {
      return set(ptr);
    }
    bool operator = (const uintptr_t &value) {
      return set((const void*)value);
    }
    operator void*() {
      return get();
    }
    operator uintptr_t() {
      return reinterpret_cast<uintptr_t>(const_cast<void*>(get()));
    }
    bool operator == (const void *ptr) {
      return ptr == get();
    }



  private:
    bool (&set)(const void*);
    const void* (&get)(void);
};
*/