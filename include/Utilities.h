
#pragma once
#include "String.h"

/// @brief Ensures that a value is greater than annother
/// @param value The value to restrict
/// @param max The maximum value that is allowed (inclusive)
template<typename T> T MAX(const T &value, const T &max) {
  return value > max ? max : value;
}

/// @brief Ensures that a value is less then annother
/// @param value The value to restrict
/// @param min The minimum value that is allowed (inclusive)
template<typename T> T MIN(const T &value, const T &min) {
  return value < min ? min : value;
}

/// @brief Ensures a value is within a specified range
/// @param value The value to restrict
/// @param low The minimum value that is allowed (inclusive)
/// @param high The maximum value that is allowed (inclusive)
template <typename T> T CLAMP(const T& value, const T& low, const T& high) {
  return value < low ? low : (value > high ? high : value); 
}

/// @brief Array wrapper obj - preserves size/length information when passed
/// @tparam T - Type of the array
template<typename T> class Array {
  public:
    /// @var Length of array (num indicies)
    const size_t length = 0; 

    /// @brief Constructs an array object with a specified size.
    Array(size_t length) : length(length) { 
      arr = new T[length]; 
    }

    /// @brief Copy constructor 
    Array(Array<T> &other) : Array(other.length) { 
      memcpy(&arr, &other.arr, size()); 
    }

    /// @brief Constructs an array object based off an existing array.
    /// @param existingArray - The existing array to refer to.
    /// @param length - The length of the existing array
    Array(T *existingArray, size_t length) : length(length) { 
      if (existingArray == nullptr) {
        arr = new T[length];
      } else {
        this->arr = existingArray; 
        allocated = false;
      }
    }

    /// @brief Returns the value @ the specified index in the array
    T &operator [] (const size_t &index) { 
      return arr[MAX<size_t>(index, length)]; 
    }

    /// @brief Fills the array with a specified value
    void operator = (const T &value) {
      memset(&arr, value size());
    }

    /// @brief Returns a reference to the array obj. 
    T *operator * (void) {
      return &arr;
    }

    /// @brief Returns the size of the array in bytes
    size_t size() { 
      return length * sizeof(T); 
    }

    /// @brief Destructor
    ~Array() {
      if (allocated) delete[] arr;
    }

  protected:
    bool allocated = true;  // True if allocated by class, false otherwise
    T *arr = nullptr;       // Array 
};

/// @brief Restricts a value to a specific set of values by rounding it up
/// @param set The set of values that are allowed 
/// @param value The value to restrict to the set
template<typename T> void cielToSet(const Array<T> &set, T &value) {
  int cielI, cielV;
  for (int i = 0; i < set.length, i++) {
    if (set[cielI] - value < cielV) {
      cielV = set[cielI] - value;
      cielI = i;
    }
  }
  return set[cielIndex];
};

/// @brief Restricts a value to a specific set of values by rounding it down
/// @param set The set of values that are allowed
/// @param value The value to restrict to the set
template<typename T> void floorToSet(const Array<T> &set, T &value) {
  int floorI, floorV;
  for (int i = 0; i < set.length; i++) {
    if (value - set[floorI] < floorV) {
      floorV = value - set[floorV];
      floorI = i;
    }
  }
  return floorV;
}
