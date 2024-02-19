
#pragma once
#include <functional>
#include <type_traits>

namespace dma {

  enum class ERROR_TYPE {

  };

  typedef void (&transferCallbackType)(int channelIndex);
  typedef void (&errorCallbackType)(int channelIndex, enum ERROR_TYPE); 


  namespace sys {
    inline bool enabled(bool);
    inline bool enabled();

    inline bool errorCallback(errorCallbackType);
    inline errorCallbackType errorCallback(void);

    inline bool transferCallback(transferCallbackType);
    inline transferCallbackType transferCallback(void);

  };

};

//////////////////////////// MISC ///////////////////////////////

#define _UNIQUE_GET_T_(_name_) val_t (&_name_)(void)
#define _UNIQUE_SET_T_(_name_) bool  (&_name_)(const val_t&)
#define _SHARED_GET_T_(_name_) val_t (&_name_)(const val_t&, const int&)
#define _SHARED_SET_T_(_name_) bool  (&_name_)(const int&)

#define PROP_MAX_ADDARGS 12

/// @internal Removes type modifiers 
template<typename T> ///////////////////// NOTE -> NEEDS TO BE IMPLEMENTED!!!!
struct _BaseType_ { 
  typedef std::remove_cv<std::remove_reference
    <std::remove_pointer<T>::type>::type>::type type;
};

/// @internal Custom type_info structs for validating property types 
template<typename T>
struct _IsBasic_ {
  static const bool value = std::is_integral<T>::value; // TO DO
};
template<typename T>
struct _IsEnum_ {
  static const bool value = std::is_enum<T>::value; // TO DO
};
template<typename T>
struct _IsPointer_ {
  static const bool value = std::is_pointer<T>::value; // TO DO
};
template<typename T>
struct _IsAddr_ {
  static const bool value = std::is_void<T>::value;
};

/// @internal Determines if template func param is of shared type
template<typename val_t, int val, typename get_t,typename set_t> 
struct _IsShared_ {
  static_assert("Property is Ill-Formed: Invalid function types.");
};
template<typename val_t, int val> 
struct _IsShared_<val_t, val, val_t (&)(void), void> {
  typedef std::false_type value;
  static const int num = -1;
};
template<typename val_t, int val>
struct _IsShared_<val_t, val, val_t (&)(const int&), void> {
  typedef std::true_type value;
  static const int num = val;
};
template<typename val_t, int val>
struct _IsShared_<val_t, val, val_t (&)(void), bool (&)(const val_t&)> {
  typedef std::false_type value;
  static const int num = -1;
};
template<typename val_t, int val>
struct _IsShared_<val_t, val, val_t (&)(const int&), 
  bool (&)(const val_t&, const int&)> {
  typedef std::true_type value;
  static const int num = val;
};

/// @internal Unpacks tupple recursively
template<size_t N>
struct _ApplyFunctr_ {
  template<typename func_t, typename tup_t, typename... A>
  static inline auto _apply_(func_t && f, tup_t && t, A &&... a)
    -> decltype(_ApplyFunctr_<N-1>::_apply_(std::forward<func_t>(f), 
      std::forward<tup_t>(t), std::get<N-1>(std::forward<tup_t>(t)), 
      std::forward<A>(a)...)) {

    return _ApplyFunctr_<N-1>::_apply_(std::forward<func_t>(f), 
      std::forward<tup_t>(t), std::get<N-1>(std::forward<tup_t>(t)), 
      std::forward<A>(a)...);
  }
};
/// @internal Base case for tuple unpacking recursion.
template<>
struct _ApplyFunctr_<0> {
  template<typename func_t, typename tup_t, typename... A>
  static inline auto _apply_(func_t && f, tup_t &&, A &&... a) 
    -> decltype(std::forward<func_t>(f)(std::forward<A>(a)...)) {
    return std::forward<func_t>(f)(std::forward<A>(a)...); 
  }
};
/// @internal "Public pair" for initiating application/unpacking of tuple 
///   into method call
template<typename func_t, typename tup_t>
inline auto _apply_(func_t && f, tup_t && t)
  -> decltype(_ApplyFunctr_<std::tuple_size<typename std::decay<tup_t>::type>
    ::value>::_apply_(std::forward<func_t>(f), std::forward<tup_t>(t))) {

  return _ApplyFunctr_<std::tuple_size<typename std::decay<tup_t>::type>
    ::value>::_apply_(std::forward<func_t>(f), std::forward<tup_t>(t));
}

/////////////////// PROPERTY OPERATORS ///////////////////////

/// @internal Base template for property base struct 
template<class deriv_t, typename val_t, typename impl = std::nullptr_t>
struct _PropBase_ {
  static_assert("Property is ill-formed: Invalid value type");
};


/// @brief Contains operators for numeric (incl. bool) type properties.
template<class deriv_t, typename val_t> 
struct _PropBase_<deriv_t, val_t, typename std::conditional
  <_IsBasic_<val_t>::value, val_t, std::nullptr_t>::type>  {

  /// @brief Value type of property
  typedef val_t type;
    
  /// @brief Assignment operators, returns true only if 
  ///   assignment is successful.
  inline bool operator = (val_t &&value) {
    return prop->set(std::forward<val_t>(value));
  }
  inline bool operator = (const val_t &value) {
    return prop->set(value);
  }
  template<typename val_to, typename get_to, typename set_to>
  bool operator = (Property<val_to, get_to, set_to, void> &other) { /////////// WRONG
    if (other == *prop) {
      return false;
    } else if (std::is_same<val_t, val_to>::value) {
      return prop->set(other.get());
    
    } else if (std::is_convertible<other::type, type>::value 
        || std::is_enum<other::type>::value) {
      return prop->set(static_cast<val_t>(other.get())); /// THERE HAS TO BE SAFER WAY....
    } 
    return false;
  }
  /// @brief Cast operator to primary value type.
  inline bool operator val_t() {
    return get();
  }
  protected:
    static deriv_t prop = static_cast<deriv_t*>(this);
};


/// @brief Contains operators for enum type properties
template<class deriv_t, typename val_t> 
struct _PropBase_<deriv_t, val_t, typename std::conditional 
  <_IsEnum_<val_t>::value, val_t, std::nullptr_t>::type> {

  /// @brief Property types 
  typedef val_t type;

  /// @brief Assignment operators (set) for enum, int & property type
  bool operator = (val_t &&value) {
    return prop->set(std::forward<val_t>(value));
  }
  bool operator = (const val_t &value) {
    return prop->set(value);
  }
  bool operator = (typename std::underlying_type<val_t>::type &value) {
    return prop->set(static_cast<val_t>(value));
  }
  template<typename val_to, typename get_to, typename set_to>
  bool operator = (Property<val_to, get_to, set_to, void> &other) {
    if (other == *prop) {
      return false;
    } else if (_IsEnum_<typename other::type>::value 
      || std::is_integral<typename other::type>::value) {
      prop->set((val_t)other.get());
    }
    return false;
  }
  /// @brief Cast operators (get) for enum & int type
  operator val_t() const {
    return prop->get();
  }
  explicit operator typename std::underlying_type<val_t>::type() const {
    return static_cast<typename std::underlying_type<val_t>::type>(prop->get());
  }
  protected:
    static deriv_t prop = static_cast<deriv_t*>(this);
};

/// @brief Contains operators for property of pointer type 
template<class deriv_t, typename val_t> 
struct _PropBase_<deriv_t, val_t, typename std::conditional 
  <_IsPointer_<val_t>::value, val_t, std::nullptr_t>::type> {

  /// @brief Type of property (underlying)
  typedef val_t type;

  constexpr _PropBase_(const bool ownPtr) : ownPtr(ownPtr), 
    ptr(nullptr);

  bool operator = (const val_t &*ptr) {
    bool result = prop->set(std::move(ptr));
    if (ownPtr) {
      ptr = nullptr;
    }
  };
  

  protected:
    const bool ownPtr;
    static deriv_t prop = static_cast<deriv_t*>(this);
};


/////////////////// PROPERTY TEMPLATES ///////////////////////

/// @brief Base property template & template for variadic 
///   shared/unshared ("indexed") properties.
template<typename val_t, typename get_t, typename set_t, typename ...arg_t>
struct Property : public _PropBase_<val_t, get_t, set_t> {

  static_assert(sizeof(arg_t...) < PROP_MAX_ADDARGS, "Property is Ill-Formed" 
    + "Too many additional arguments");

  template<typename ...arg_t>
  Property(get_t getFunc, set_t setFunc, arg_t... args)
    : argTup(std::make_tuple(args...)), getSrc(getFunc), setSrc(setFunc) 
      index(_IsShared_<val_t, prevIndex, get_t, set_t>::num) {

    if (_IsShared_<val_t, get_t, set_t>) {
      index = prevIndex++;
    }
  };       
  template<typename ...arg_t>
  Property(get_t getFunc, arg_t... args)
    : argTup(std::make_tuple(args...)), getSrc(getFunc), setSrc(dummySet),
      index(_IsShared_<val_t, prevIndex, get_t, set_t>::num) {
        
    if (_IsShared_<val_t, get_t, set_t>) {
      prevIndex++;
    }
  };
  protected:
    val_t get() {
      if (index == -1) {
        _apply_(&getSrc, &getTup);
      }
      auto targTup = std::tuple_cat<std::make_tuple<const int&>(val_t{}), getTup>;
      return _apply_(&getSrc, &targTup);
    }
    bool set(val_t &&value) {
      if (index == -1) {
        return _apply_(&setSrc, &argTup);
      }
      auto targTup = std::tuple_cat<std::make_tuple<const val_t&, 
        const &int>(std::forward(value), index), setTup>;
      return _apply_(&setSrc, &targTup);
    }
    bool dummySet(const val_t&, arg_t...);
    static std::tuple<arg_t...> argTup; 

    const int index;
    static int index prev;
    static get_t getSrc;
    static set_t setSrc;
};

/// @brief Property specialization for unique read/write functions
template<typename val_t>
struct Property<val_t, _UNIQUE_GET_T_(), _UNIQUE_SET_T_(), void> 
  : public _PropBase_<Property<val_t, _UNIQUE_GET_T_(), 
  _UNIQUE_SET_T_(), void>, val_t> {

  constexpr Property(_UNIQUE_GET_T_(getFunc), _UNIQUE_SET_T_(setFunc)) 
    : get(getFunc), set(setFunc) {}
  constexpr Property(_UNIQUE_GET_T_(getFunc)) : get(getFunc), set(&dummySet);

  protected:
    bool dummySet(const val_t&);
    friend _PropBase_<Property<val_t, _UNIQUE_GET_T_(), 
    _UNIQUE_SET_T_(), void>, val_t>;

    _UNIQUE_GET_T_(get);
    _UNIQUE_SET_T_(set);
};

/// @brief Property specialization for shared ("indexed") properties
template<typename val_t>
struct Property<val_t, _SHARED_GET_T_(), _SHARED_SET_T_(), void> 
  : public _PropBase_<Property<val_t, _SHARED_GET_T_(), 
  _SHARED_SET_T_(), void>, val_t>{

  constexpr Property(_SHARED_GET_T_(getFunc), _SHARED_SET_T_(setFunc))
    : index(prevIndex), get(getFunc), set(setFunc) {
    prevIndex++;
  }
  constexpr Property(_SHARED_GET_T_(getFunc))
    : index(prevIndex), get(getFunc), set(&dummySet) {
    prevIndex++;
  }
  protected:
    val_t get() {
      return getSrc(index);
    }
    bool set(val_t &&value) {
      return setSrc(std::forward<val_t>(value), index);
    }
    friend _PropBase_<Property<val_t, _SHARED_GET_T_(), 
      _UNIQUE_GET_T_(), void>, val_t>;
    bool dummySet(const val_t&, const int&);

    const int index;
    static int prevIndex;
    static _SHARED_GET_T_(getSrc);
    static _SHARED_SET_T_(setSrc);
};






/*

typedef struct sys{

  // More to do -> init() method??

  struct enabled{ 
    inline operator bool();
    void operator = (const bool);
  }enabled;

  struct errorCallback{
    inline operator errorCBType(); 
    inline operator bool();
    inline void operator = (const errorCBType);
  }errorCallback;

  struct transferCallback{
    inline operator transferCBType();
    inline operator bool();
    inline void operator = (const transferCBType);
  }transferCallback;

};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: ACTIVE CHANNEL MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct activeChannel {

  struct remainingBytes{
    inline operator int();
  }remainingBytes;

  struct index{
    inline operator int();
  }index;

  struct busy{
    inline operator bool();
  }busy;

};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: CHANNEL MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct channel { // Rename to channel ctrl
  channel(const int &index):index(index) {};
  const int &index;


  struct descriptorList {
    void operator = (std::initializer_list<transferDescriptor&>);
    transferDescriptor operator[] (const int&);  
    _super_(channel, descriptorList);
  }descriptorList{this};

  struct state {
    bool operator = (const CHANNEL_STATE&);
    inline operator CHANNEL_STATE();
    inline bool operator == (const CHANNEL_STATE);
    _super_(channel, state);
  }state{this};

  struct error {
    inline operator CHANNEL_ERROR();
    inline operator bool();
    inline bool operator == (const CHANNEL_ERROR&);
    _super_(channel, error);
  }error{this};

};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: CHANNEL CONFIG MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////

struct channelConfig {
  explicit channelConfig(const int index):index(index) {};
  const int &index;

  /// @brief Sets the trigger source of the channel.
  /// @param  operator_equals Sets value & does not return. ENUM -> ACTION_...
  /// @param  cast_enum Returns current value.
  struct triggerSource { 
    inline void operator = (const TRIGGER_SOURCE&);
    inline operator TRIGGER_SOURCE();
    _super_(channelConfig, triggerSource);
  }triggerSrc{this};

  /// @brief  Specifies action taken when channel is triggered.
  /// @param  operator_equals Sets value & does not return. ENUM -> TRIGGER_...
  /// @param  cast_enum Returns current value.
  struct triggerAction { 
    inline void operator = (const TRIGGER_ACTION&);
    inline operator TRIGGER_ACTION();
    _super_(channelConfig, triggerAction);
  }triggerAction{this};


  /// @brief Number of values sent per burst (smallest transfer increment).
  /// @param operator_equals Sets quantity & returns true only if quantity is valid.
  /// @param cast_int Returns current quantity.
  struct burstThreshold { 
    inline bool operator = (const int&);
    inline operator int();
    _super_(channelConfig, burstThreshold);
  }burstThreshold{this};

};

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION: TRANSFER DESCRIPTOR MODULE
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct transferDescriptor { 
  friend channel; 
  transferDescriptor();
  transferDescriptor(const transferDescriptor&);
  transferDescriptor(DmacDescriptor*);

  inline void operator = (const transferDescriptor&);

  struct addr {
    inline bool operator = (const void*);
    inline operator void*() const;
    inline operator uintptr_t() const;
    _multi_(transferDescriptor, addr);
  }src{this, 0}, dest{this, 1};

  struct increment {
    bool operator = (const int&);
    operator int() const;
    _multi_(transferDescriptor, increment);
  }incrementSrc{this, 0}, incrementDest{this, 1};

  template<int T>
  void value() {

  }
  
  struct dataSize {
    inline bool operator = (const int&);
    inline operator int() const;
    _super_(transferDescriptor, dataSize);
  }dataSize{this};

  struct length {
    inline bool operator = (const int&);
    inline operator int() const;
    _super_(transferDescriptor, length);
  }length{this};

  struct suspOnCompl {
    inline void operator = (const bool&);
    inline operator bool() const;
    _super_(transferDescriptor, suspOnCompl);
  }suspOnCompl{this};

  struct valid {
    inline bool operator = (const bool&);
    inline operator bool() const;
    _super_(transferDescriptor, valid);
  }valid{this};

  inline void reset();

  private:
    DmacDescriptor *descriptor;
};

*/