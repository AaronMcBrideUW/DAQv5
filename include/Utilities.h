
#pragma once
#include "String.h"

template<typename T> T MAX(const T &value, const T &max) {
  return value > max ? max : value;
}

template<typename T> T MIN(const T &value, const T &min) {
  return value < min ? min : value;
}

template <typename T> T CLAMP(const T& value, const T& low, const T& high) {
  return value < low ? low : (value > high ? high : value); 
}



template<typename T> class Array {

  public:

    //// CONSTRUCTORS ////
    Array(size_t length) : length(length) { arr = new T[length]; }

    Array(size_t length, const T &value) : Array(length) { fill(value); } 

    Array(const Array &other) { copy(other, true); }

    //// OPERATORS ////
    T &operator [] (const size_t &index) { get(index); }

    void operator = (const Array &other) { copy(other, true); }
    
    //// METHODS ////
    size_t length() const { return length; }

    size_t size() const { return sizeof(T) * length; }

    void fill(const T value) { 
      memset(&arr, value, sizeof(T) * length); 
    }

    T &get(const size_t index) { 
      return arr[MAX<unsigned int>(index, length - 1)];
    }

    void copy(const Array &other, const bool resize) {
      if (length != other.length && resize) {
        delete[] arr;
        arr = new T[other.length];
        this->length = other.length;
      } else {
        fill(T());
      }
      memcpy(&arr, &other, other.length < length ? other.length : size());
    }

  protected:
    T *arr = nullptr;
    size_t length = 0;
};


template<typename T> void ceilToSet(const Array<T, 3> &set, const int setLength, T &value) {

  for (int i = 0; i < setLength, )


};