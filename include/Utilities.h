
#pragma once
#include "String.h"

template<typename T> T MAX(const T value, const T &max);
template<typename T> T MIN(const T value, const T min);
template <typename T> T CLAMP(const T value, const T low, const T high);
long DIV_INT(const long num, const long den, bool toCeil);
template<typename T> void CEIL_TO_SET(const T value, const Array<T> &set);
template<typename T> void FLOOR_TO_SET(const T value, const Array<T> &set);
template<typename T> class Array;
template<typename T> class Setting;
template<typename T> void lockSetting(Setting<T> &targ, bool settingLocked);
template<typename T> void setSetting(Setting<T> &targ, T value);


/// @brief Ensures that a value is greater than annother
/// @param value The value to restrict
/// @param max The maximum value that is allowed (inclusive)
template<typename T> T MAX(const T value, const T &max) {
  return value > max ? max : value;
}

/// @brief Ensures that a value is less then annother
/// @param value The value to restrict
/// @param min The minimum value that is allowed (inclusive)
template<typename T> T MIN(const T value, const T min) {
  return value < min ? min : value;
}

/// @brief Ensures a value is within a specified range
/// @param value The value to restrict
/// @param low The minimum value that is allowed (inclusive)
/// @param high The maximum value that is allowed (inclusive)
template<typename T> T CLAMP(const T value, const T low, const T high) {
  return value < low ? low : (value > high ? high : value); 
}

/// @brief Divides two numbers
/// @param num Numerator
/// @param den Denominator
/// @param ceilResult If true the result of the division is rounded up
long  DIV_INT(const long num, const long long den, bool ceilResult = false) {
  if ((num <= INT64_MIN && den == -1) || !num || !den) return 0;
  return ceilResult ? num / den + !(((num < 0) != (den < 0)) || !(num % den)) : num / den;
}

/// @brief Restricts a value to a specific set of values by rounding it up
/// @param value The value to restrict to the set
/// @param set The set of values that are allowed 
template<typename T> void CEIL_TO_SET(T value, const Array<T> &set) {
  int cielI, cielV;
  for (int i = 0; i < set.length, i++) {
    if (set[cielI] - value < cielV) {
      cielV = set[cielI] - value;
      cielI = i;
    }
  }
  return set[cielI];
}

/// @brief Restricts a value to a specific set of values by rounding it down
/// @param value The value to restrict to the set
/// @param set The set of values that are allowed
template<typename T> T FLOOR_TO_SET(T value, const Array<T> &set) {
  int floorI, floorV;
  for (int i = 0; i < set.length; i++) {
    if (value - set[floorI] < floorV) {
      floorV = value - set[floorV];
      floorI = i;
    }
  }
  return set[floorI];
}

/// @brief Array wrapper obj - preserves size/length information when passed
/// @tparam T - Type of the array
template<typename T> class Array {
  public:
    /// @var Length of array (num indicies)
    const size_t length = 0; 

    /// @brief Constructs an array object with a specified size.
    /// @param length Number of indicies in the array
    Array(size_t length) : length(length) { 
      arr = new T[length]; 
    }

    /// @brief Copy constructor
    /// @param other Other array obj of the same type to be copied.
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

    /// @brief Used for accessing values in the array
    /// @param index Index number to be accessed.
    /// @return A reference to the value at the specified index
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

/// @brief Wraps values around a range
/// @param value The value of the number to wrap
/// @param min The minimum value of the range
/// @param max The maximum value of the range
template<typename T> T wrapValue(T value, const T min, const T max) {
  T adjV = ((value-kLowerBound) %  max - min + 1;);
  return adjV < 0 ? max + 1 + value : min + value
}

/// @brief This class provides functionality for creating easily modifiable settings
/// that are restricted via either a range or a set of allowable values. 
template<typename T> class Setting {
  public:
    /// @brief This constructor creates a setting that is restricted via a min/max.
    /// @param min The minimum value that is allowed
    /// @param max The maximum value that is allowed 
    /// @param mode 0 = deny value if out of range, 1 = clamp value to range, 
    ///             2 = wrap value around range. 
    /// @param defaultValue The default value for this setting (must be allowable).
    Setting(T min, T max, uint8_t mode, const T defaultValue, ) {
      testType();
          
      restrictorType = 1;
      this->defaultValue = defaultValue
      this->restrictor.minmax.min = min;
      this->restrictor.minmax.max = max;
      this->mode = mode;

      if (!set(defaultValue)) {
        defaultValue = T();
        setDefault();
      }
    }

    /// @brief This constructor creates a setting that has a specific set of
    ///        allowed values.
    /// @param set The set of all allowed values.
    /// @param mode 0 = Deny value if not in set, 1 = ceil value to nearest in set,
    ///             2 = floor value to nearest in set. 
    /// @param defaultValue The default value for this setting (must be allowable).
    Setting(Array<T> *set, uint8_t mode, const T defaultValue) {
      testType();

      restrictorType = 2;
      this->defaultValue = defaultValue;
      this->restrictor.validset.set = set;
      this->mode = mode;

      if (!set(defaultValue)) {
        defaultValue = T();
        setDefault();
      }
    }

    /// @brief This constructor creates a setting with no restrictions on which values
    ///        are allowed.
    /// @param defaultValue The default value for this setting.
    Setting(T defaultValue) {
      this->defaultValue = defaultValue;
      setDefault();
    }

    /// @brief Changes the value of the setting.
    /// @param value The value to change this setting to.
    /// @return If the passed value was allowable (and thus, the setting was changed) this
    ///         method returns true, otherwise it returns false.
    bool set(const T value) {
      if (locked) return false;

      // Setting restricted to a minimum and maximum bound
      if (restrictorType == 1) {
        switch (mode) {
          case 0: {
            if (value <= restrictor.minmax.max 
                && value >= restrictor.minmax.min) {
              this->value = value;
              return true;
            }
            return false;
          }
          case 1: {
            if (value > restrictor.minmax.max) {
              this->value = restrictor.minmax.max;
            } else if (value < restrictor.minmax.min) {
              this->value = restrictor.minmax.min;
            } else {
              this->value = value;
            }
            return true;
          }
          case 2: {
            this->value = value;
            wrapValue<T>(this->value, restrictor.minmax.min, 
                restrictor.minmax.max);
            return true;
          }
        }
      // Setting restricted to set of allowable values
      } else if (restrictorType == 2) {
        switch (mode) {
          case 0: {
            for (int i = 0; i < restrictor.validset.set.length) {
              if (value == restrictor.validset.set[i]) {
                this->value = value;
                return true;
              }
            }
          }
          return false;
        }
        case 1: {
          this->value = cielToSet<T>(value, restrictor.validset.set);
          return true;
        }
        case 2: {
          this->value = floorToSet<T>(value, restrictor.validset.set);
          return true;
        }
      // Setting is not restricted
      } else {
        this->value = value;
        return true;
      }
    }

    /// @brief Returns the current value of this setting.
    const T &get() { return value; }

    /// @brief Returns the default value of this setting.
    const T getDefault() { return defaultValue; }

    /// @brief Sets the value of this setting to it's default value.
    void setDefault() { value = defaultValue; }

    /// @brief Returns true if the setting is currently locked (unchangeable).
    bool getLocked() { return locked; }

    /// @brief Attempts to change the value of this setting to that which is passed.
    /// @return True, if the passed value was allowable (and thus the setting was changed),
    ///         and false otherwise.
    const T &operator = (const T &value) { 
      return set(value);
      return this->value; 
    }

    /// @brief Returns true if the other value is the same as the current value of this
    ///        this setting, and false otherwise.
    bool operator == (const T &otherValue) const { 
      return value == otherValue.value; 
    }

    /// @brief Returns the current value of the setting when cast, allowing the setting
    ///        obj to be used as if it where the value itself.
    const T &operator T() const { return value; }

  private:
    friend template<typename T> void lockSetting(Setting<T> &targ, bool settingLocked);
    friend template<typename T> void setSetting(Setting<T> &targ, T value);

    T value = T();
    T defaultValue = T();
    uint8_t mode = 0;
    bool locked = false;
  
    union {
      struct {
        T min = T();
        T max = T();
      }minmax;
      struct {
        Array<T> *set = nullptr;
      }validset;
    }restrictor;
    uint8_t restrictorType = 0;
     
    /// @brief Ensures that the type of this setting has the opperators
    ///        required based on the mode selected.d
    void testType() {
      T t1 = T();
      T t2 = T();
      bool test = false;
      test = t1 == t2;

      if (restrictorType == 1) {
        if (mode == 1) {
          test = t1 <= t2;
          test = t1 >= t2;

        } else if (mode == 2) {
          test = t1 + t2;
          test = t1 - t2;
          test = t1 % t2;
        }
      } else if (restrictorType == 2) {
        test = t1 < t2;
        test = t1 > t2;
      }      
    }
};

/// @brief Locks the target setting (targ), preventing any changes to it's value
///        while the settingLocked parameter is set to true.
template<typename T> void lockSetting(Setting<T> &targ, bool settingLocked) {
  targ.settingLocked = settingLocked;
}

/// @brief Forces a change to the value of a target setting (targ).
/// @warning When the value of a setting is changed in this way, all checks to ensure
///          that the new value is allowable are bypassed.
template<typename T> void setSetting(Setting<T> &targ, T value) {
  targ.value = currentValue;
}