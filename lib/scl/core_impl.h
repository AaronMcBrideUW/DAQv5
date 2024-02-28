
#pragma once
#include <sam.h>

namespace samc::impl {

  template<typename T>
  T clamp(const T &value, const T &min, const T &max) {
    return value < min ? min : (value > max ? max : value);
  }

  template<typename T>
  T clamp_min(const T &value, const T &min) {
    return value < min ? min : value;
  }

  template<typename T>
  T clamp_max(const T &value, const T&max) {
    return value > max ? max : value;
  }

  template<typename T, unsigned int N>
  int indexOf(const T &value, const T (&arr)[N]) {
    for (int i = 0; i < N; i++) {
      if (arr[i] == value) {
        return i;
      }
    }
    return -1;
  }

  template<typename T, unsigned int N>
  bool contained(const T &value, const T (&arr)[N]) {
    return indexOf<T, N>(value, arr) != -1;
  }

  template<typename T>
  T safe_div(const T &den, const T &num) { 
    if (!den) {
      return den{};
    }
    return den / num;
  }    

}

struct read_write {};
struct read_only {};
struct write_only {};

template<typename T, typename ...attr>
class Property {

  


};


/*
namespace core { 

  typedef struct read_only{};
  typedef struct read_write{};
  typedef struct write_only{};

  typedef struct indexed{};
  typedef struct unique{};

  template<typename T>
  struct _ValidType {
    static const bool value = std::is_integral<T>::value
      && !std::is_reference<T>::value && !std::is_pointer<T>::value
      && !std::is_const<T>::value && !std::is_volatile<T>::value;
  };

  template<typename T, typename indexed = void, typename access_t = void>
  class Property {
    static_assert(true, "Property is ill-formed: parameter type invalid");
  }; 

  template<typename T>
  class Property<T, unique, read_write> {
    public:
      Property(bool (&set)(const T&), T (&get)(void), T &&default)
        : set(set), get(get), default(std::move(default)) {};

      static_assert(_ValidType<T>::value, 
        "Property is ill-formed: invalid type");

      typedef T type;

      bool operator = (T &&value) noexcept {
        return set(std::forward(value));
      } 
      operator T() noexcept {
        return get();
      }
      bool setDefault() {
        return set(default);
      }
    private:
      const T default;
      bool (*set)(const T&);
      T (*get)(void);
  };

  template<typename T>
  class Property<T, unique, read_only> {
    public:
      Property(T (&get)(void)) : get(get) {};

    static_assert(_ValidType<T>::value, 
      "Property is ill-formed: invalid type");

    typedef T type;

    operator T() noexcept {
      return get();
    }
    private:
      T (*get)(void);
  };

  template<typename T>
  class Property<T, unique, write_only> {
    public:
      Property(bool (&set)(const T&), const T default) 
        : set(set), default(default) {};

    static_assert(_ValidType<T>::value, 
      "Property is ill-formed: invalid type");

    typedef T type;

    bool operator = (T &&value) noexcept {
      return set(std::forward(value));
    }
    bool setDefault() {
      return set(default);
    }
    private:
      const T default;
      bool (*set)(const T&);
  };

  template<typename T>
  class Property<T, indexed, read_write> {
    public:
      Property(bool (&set)(const T&, const int&), T (&get)(const int&), const int index,
        const T default) : set(set), get(get), index(index), default(default) {};

    static_assert(_ValidType<T>::value, 
      "Property is ill-formed: invalid type");

    typedef T type;

    bool operator = (T &&value) noexcept {
      return set(std::forward(value), index);
    }
    operator T() noexcept {
      return get(index);
    }
    bool setDefault() {
      return set(default, index);
    }
    private:
      const T default;
      const int index;
      bool (*set)(const T&, const int&);
      T (*get)(const int&);
  };

  template<typename T>
  class Property<T, indexed, read_only> {
    public:
      Property(T (*get)(const int&), const int index) : get(get), 
        index(index) {};

    static_assert(_ValidType<T>::value, 
      "Property is ill-formed: invalid type");

    typedef T type;

    operator T() noexcept {
      return get(index);
    }
    private:
      const int index;
      T (*get)(const int&);
  };
  
  template<typename T>
  class Property<T, indexed, write_only> {
    public:
      Property(bool (*set)(const T&, const int&), const int index, 
        const T default) : set(set), index(index), default(default) {};

    static_assert(_ValidType<T>::value, 
      "Property is ill-formed: invalid type");

    typedef T type;

    bool operator = (T &&value) noexcept {
      return set(std::forward(value), index);
    }
    bool setDefault() {
      return set(default, index);
    }
    private:
      const T default;
      const int index;
      bool (*set)(const T&, const int&);
  };
}
*/






