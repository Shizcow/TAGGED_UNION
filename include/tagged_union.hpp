#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <type_traits>
#include <algorithm>
#include <cassert>

#include <magic_enum.hpp>

#include <boost/version.hpp>
#ifndef BOOST_VERSION
#error ("BOOST_VERSION is not defined! Boost is required for TAGGED_UNION.")
#endif

#include <boost/config.hpp>
#include <boost/assert.hpp>

#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/rest_n.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
  
// C++23: brings std::unreachable, but that doesn't work
//        well with GCC's optimizer sometimes. So do this
//        the old-fashoned way.
// https://en.cppreference.com/w/c/program/unreachable
#ifdef __GNUC__ // GCC, Clang, ICC
#define __TAGGED_UNION_UNREACHABLE() __builtin_unreachable()
#else
#ifdef _MSC_VER // MSVC
#define __TAGGED_UNION_UNREACHABLE() __assume(false)
#else
// Even if no extension is used, undefined behavior is still raised by
// the empty function body and the noreturn attribute.
#defined __TAGGED_UNION_NEEDS_UNREACHABLE_FALLBACK
#define __TAGGED_UNION_UNREACHABLE() ::util::detail::unreachable()
#endif
#endif

// Workaround stuff for aggregate initialization
#if __cplusplus < 202002L
// Please note this doesn't wrap the argument in ()!
#define __TAGGED_UNION_ONLY_UNDER_CPP17(x) x
#else
#define __TAGGED_UNION_ONLY_UNDER_CPP17(x)
#endif

// Similarly:
#if __cplusplus >= 202002L
#define __TAGGED_UNION_ONLY_CPP20_PLUS(x) x
#else
#define __TAGGED_UNION_ONLY_CPP20_PLUS(x)
#endif

// Super helpful macros
#define __TAGGED_UNION_IS_VOID(x) BOOST_PP_IS_EMPTY(BOOST_PP_CAT(__TAGGED_UNION_IS_VOID_HELPER_, x))
#define __TAGGED_UNION_IS_VOID_HELPER_void

#define __TAGGED_UNION_STRIP_PARENS_IMPL(...) __VA_ARGS__
#define __TAGGED_UNION_STRIP_PARENS(x) __TAGGED_UNION_STRIP_PARENS_IMPL x

namespace tagged_union::detail {
  // There's no need to pull in std::variant just for an std::monostate
  // that we don't even need all of the methods of:
  struct monostate {};

  // == Fall-back unreachable implementation ==
  // If neither BOOST_NORETURN nor BOOST_FORCEINLINE are defined... good luck.
  //
  // Also, a piece of trivia: the external definition of unreachable_impl
  // must be emitted in a separated TU due to the rule for inline functions in C.
#ifdef __TAGGED_UNION_NEEDS_UNREACHABLE_FALLBACK
  BOOST_NORETURN BOOST_FORCEINLINE void unreachable() {
    abort(); // And just in case the compiler really can't figure it out
  }
#endif

  template<typename A, typename B, typename C>
  auto constexpr constexpr_if_trivial_destructor(B b, C c) {
    if constexpr(std::is_trivially_destructible_v<A>) {
      return b;
    } else {
      return c;
    }
  }

  // At least one type is not trivially destructible
  template <typename... Ts>
  static constexpr bool UseExplicitDestructor = (... || !std::is_trivially_destructible_v<Ts>);
    
  template <typename... Ts>
  static constexpr bool UseNoexceptDestructor = (... && std::is_nothrow_destructible_v<Ts>);
    
  template <typename... Ts>
  static constexpr bool UseCopyConstructor = (... && std::is_copy_constructible_v<Ts>);
    
  template <typename... Ts>
  static constexpr bool UseNoexceptCopyConstructor = (... && std::is_nothrow_copy_constructible_v<Ts>);

  template <typename... Ts>
  static constexpr bool UseMoveConstructor = (... && std::is_move_constructible_v<Ts>);

  template <typename... Ts>
  static constexpr bool UseNoexceptMoveConstructor = (... && std::is_nothrow_move_constructible_v<Ts>);
    
  template <typename... Ts>
  static constexpr bool UseCopyAssigner = (... && std::is_copy_assignable_v<Ts>);
    
  template <typename... Ts>
  static constexpr bool UseNoexceptCopyAssigner = (... && std::is_nothrow_copy_assignable_v<Ts>);

  template <typename... Ts>
  static constexpr bool UseMoveAssigner = (... && std::is_move_assignable_v<Ts>);

  template <typename... Ts>
  static constexpr bool UseNoexceptAssigner = (... && std::is_nothrow_move_assignable_v<Ts>);
}

/*     TAGGED_UNION_IMPLEMENTATION     */
// ++:++++++-++++++++++:+++++:::    ...:. ..:::::::::..  .                      
// ---+-------      ++++++++++++++++.  ++++++++++++++++++++++++:::....::::::::::
// ----+---   BEWARE   ++++++++++++    .+++++++++++++-+++++++++++++++++++++:::::
// ----+-                ++++++++.      ++++++++++++-+++++++++++++++++++++++++++
// ++++++ HERE BE MAGIC! +++++++++..     :++++++++++++++++++++++++++++++++++++++
// ++++++-              +++++++--+.       ++++++++-++-+++--------+++++++++++++++
// --++--------------------+-----:        ++++++---+-----------------+++++++++++
// -------------.-----------------+        +------+---------------------++++++++
// -------------+ :+--------------:         +----------------------------+++++++
// -------------:    :++----------:         ------------------++----------++++++
// ------------.       ::+--------.         -------------------++---------++++++
// ----------            :-----+            :-----+.:. :-------+-----------+++++
// ---------:              ::--+:               :+:.         ...+----------+++++
// --------.                 ..--              .+--:              +-------++++++
// -------:                                       :                +------++::++
// ------.        +.                                               +----++   :++
// ------    .  ----                                                ::::   :++++
// ------+::.+-------                                                    ++++::+
// ------------------.                                        --:      .....:++-
// ------------------:.                          .----.      :+--+:.   ...:+--+-
// -------------------:                        :-------      +-----+++:.    :+:+
// --------------------:                     .----------        +---++++--++++++
// --------------------+-:                 .------------.   +   +++++++::::::...
// -----------------------.           ..+:--------------+    :  ..              
// -------------------------::    :---+---------------++.                       
// ---------------------------:    :-----------:::...                           
// ---------------------------+   ---+++++::..                                  
// ---------------------------:   -+:                                           
// --------------------+++::..                                                  
#define TAGGED_UNION_ENUM_FROM_TRIPLET(r, data, triplet)	\
  BOOST_PP_TUPLE_ELEM(3, 0, triplet),
#define TAGGED_UNION_TYPE_PACK_FROM_TRIPLET(r, data, triplet) \
  BOOST_PP_IF( \
    __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)), \
    BOOST_PP_EMPTY(), \
    (BOOST_PP_TUPLE_ELEM(3, 1, triplet)))
#define TAGGED_UNION_MEMBERS_FROM_TRIPLET(r, data, triplet) \
  BOOST_PP_IF( \
    __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)), \
    /* emit nothing if void*/, \
    BOOST_PP_TUPLE_ELEM(3, 1, triplet) BOOST_PP_TUPLE_ELEM(3, 2, triplet));
#define TAGGED_UNION_ACCESSOR_FROM_TRIPLET(r, union_name, triplet)	\
  BOOST_PP_IF(							\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  /* emit nothing if void*/,						\
  [[nodiscard]] constexpr const BOOST_PP_TUPLE_ELEM(3, 1, triplet)& BOOST_PP_TUPLE_ELEM(3, 2, triplet)() const { \
    check_type(BOOST_PP_TUPLE_ELEM(3, 0, triplet));			\
    return attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet);			\
  }									\
  [[nodiscard]] constexpr BOOST_PP_TUPLE_ELEM(3, 1, triplet)& BOOST_PP_TUPLE_ELEM(3, 2, triplet)() { \
    check_type(BOOST_PP_TUPLE_ELEM(3, 0, triplet));			\
    return attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet);			\
  })
#define TAGGED_UNION_COPY_CONS_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case BOOST_PP_TUPLE_ELEM(3, 0, triplet):				\
  BOOST_PP_IF(								\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  /* emit nothing if void*/,						\
  new (&attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet)) BOOST_PP_TUPLE_ELEM(3, 1, triplet)(other.attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet))); \
  break;
#define TAGGED_UNION_MOVE_CONS_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case BOOST_PP_TUPLE_ELEM(3, 0, triplet):				\
  BOOST_PP_IF(								\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  /* emit nothing if void*/,						\
  new (&attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet)) BOOST_PP_TUPLE_ELEM(3, 1, triplet)(std::move(other.attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet)))); \
  break;
#define TAGGED_UNION_MOVE_ASSGN_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case BOOST_PP_TUPLE_ELEM(3, 0, triplet):				\
  BOOST_PP_IF(								\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  /* emit nothing if void*/,						\
  attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet) = std::move(other.attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet))); \
  break;
#define TAGGED_UNION_COPY_ASSGN_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case BOOST_PP_TUPLE_ELEM(3, 0, triplet):				\
  BOOST_PP_IF(								\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  /* emit nothing if void*/,						\
  attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet) = other.attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet)); \
  break;
#define TAGGED_UNION_DTOR_SWITCH_FROM_TRIPLET(r, union_name, triplet)	\
  BOOST_PP_IF(							\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  /* emit nothing if void... we don't even need to print the case label*/, \
  case (::tagged_union::detail::constexpr_if_trivial_destructor<BOOST_PP_TUPLE_ELEM(3, 1, triplet)> \
	(BOOST_PP_TUPLE_ELEM(3, 0, triplet), max_val+1+BOOST_PP_TUPLE_ELEM(3, 0, triplet))): \
  if constexpr(std::is_trivially_destructible_v<BOOST_PP_TUPLE_ELEM(3, 1, triplet)>) { \
    __TAGGED_UNION_UNREACHABLE();							\
  } else {								\
  using __typePunt = BOOST_PP_TUPLE_ELEM(3, 1, triplet);		\
  attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet).~__typePunt();		\
  })
#define TAGGED_UNION_SETTER_FROM_TRIPLET(r, data, triplet)	\
  BOOST_PP_IF(							\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  void set_type_and_data_impl(OfType<BOOST_PP_TUPLE_ELEM(3, 0, triplet)>) BOOST_NOEXCEPT { \
    /* Make sure to conditionally destroy the current data first */	\
    /* Since this is a voidy type the hot path is always destroying */	\
    /* AKA we can just: */						\
    this->~ThisType();							\
    type = BOOST_PP_TUPLE_ELEM(3, 0, triplet);				\
  },									\
  void set_type_and_data_impl(BOOST_PP_TUPLE_ELEM(3, 1, triplet) const& value, OfType<BOOST_PP_TUPLE_ELEM(3, 0, triplet)>) BOOST_NOEXCEPT { \
    /* Because there is data here, we shouldn't use the same hot-path assumption as in the void case */ \
    /* Why? Because destroy+placement new is likely slower than copy/move assignment */	\
    if (type == BOOST_PP_TUPLE_ELEM(3, 0, triplet)) {			\
      /* If same type, use assignment */				\
      attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet) = value;			\
    } else {								\
      /* Otherwise, destruct and the use placement-new. */		\
      /* The idea is we don't want to move on top of uninit'd data */	\
      /* since the destination may have a non-trivial assignment */	\
      this->~ThisType();						\
      new (&attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet)) BOOST_PP_TUPLE_ELEM(3, 1, triplet)(value); \
    }									\
    type = BOOST_PP_TUPLE_ELEM(3, 0, triplet);				\
  }									\
  void set_type_and_data_impl(BOOST_PP_TUPLE_ELEM(3, 1, triplet) && value, OfType<BOOST_PP_TUPLE_ELEM(3, 0, triplet)>) BOOST_NOEXCEPT { \
    /* Same shenanigans as above except with std::move) */		\
    if (type == BOOST_PP_TUPLE_ELEM(3, 0, triplet)) {			\
      attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet) = std::move(value);	\
    } else {								\
      this->~ThisType();						\
      new (&attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet)) BOOST_PP_TUPLE_ELEM(3, 1, triplet)(std::move(value)); \
    }									\
    type = BOOST_PP_TUPLE_ELEM(3, 0, triplet);				\
  })
#define TAGGED_UNION_CONSTRUCTOR_FROM_TRIPLET(r, struct_name, triplet)	\
  /* Overload constructor */						\
  __TAGGED_UNION_STRIP_PARENS(BOOST_PP_IF(						\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  (constexpr struct_name(OfType<BOOST_PP_TUPLE_ELEM(3, 0, triplet)>)		\
   BOOST_NOEXCEPT : type{BOOST_PP_TUPLE_ELEM(3, 0, triplet)} BOOST_PP_COMMA()\
   attr{AttrUnion {.__empty = {}}} {}),					\
  (/* Note: These are marked as constepxr REGARDLESS of the */		\
   /* constexpr-ness of the values' copy/move constructors */		\
   /* It's impossible to SFINAE these according to the C++ standard: */	\
   /*     https://eel.is/c++draft/class.copy.ctor#1 */			\
   /* For this specific reason, in this SPECIFIC scenario, the */	\
   /* constexpr will be automatically dropped if the value */		\
   /* doesn't have the correct trait. */				\
   constexpr struct_name(BOOST_PP_TUPLE_ELEM(3, 1, triplet) const& value, \
			 OfType<BOOST_PP_TUPLE_ELEM(3, 0, triplet)>)	\
   BOOST_NOEXCEPT : type{BOOST_PP_TUPLE_ELEM(3, 0, triplet)} BOOST_PP_COMMA() \
   attr{AttrUnion {.BOOST_PP_TUPLE_ELEM(3, 2, triplet) = value}} {}	\
   constexpr struct_name(BOOST_PP_TUPLE_ELEM(3, 1, triplet) && value,	\
			 OfType<BOOST_PP_TUPLE_ELEM(3, 0, triplet)>)	\
   BOOST_NOEXCEPT : type{BOOST_PP_TUPLE_ELEM(3, 0, triplet)} BOOST_PP_COMMA() \
   attr{AttrUnion {.BOOST_PP_TUPLE_ELEM(3, 2, triplet) = std::move(value)}} {})))
#define TAGGED_UNION_ATTREQ_FROM_TRIPLET(r, data, triplet)		\
  case BOOST_PP_TUPLE_ELEM(3, 0, triplet):				\
  BOOST_PP_IF(							\
  __TAGGED_UNION_IS_VOID(BOOST_PP_TUPLE_ELEM(3, 1, triplet)),				\
  /* Emit nothing, since we're going to return true*/,			\
  return attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet)			\
  == other.attr.BOOST_PP_TUPLE_ELEM(3, 2, triplet));
#define TAGGED_UNION(struct_name, triplets...)				\
  public:								\
  using ThisType = struct_name;						\
  enum Type {								\
    BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_ENUM_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
  };									\
									\
  /* We need several layers of indirection here */			\
  /* The core issue is that we want to specify an explicitly empty */	\
  /* destructor, but only when non-trivially destructible types */	\
  /* are present. This allows us to manually manage destruction */	\
  /* at the caller level with zero cost abstraction. */			\
  template <bool UseExplicitDestructor, bool DummyDefer>		\
  union AttrUnionImpl;							\
									\
  template <bool DummyDefer>						\
  union AttrUnionImpl<false, DummyDefer> {				\
    ::tagged_union::detail::monostate __empty;					\
    BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_MEMBERS_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
  };									\
									\
  template <bool DummyDefer>						\
  union AttrUnionImpl<true, DummyDefer> {				\
    ::tagged_union::detail::monostate __empty;					\
    BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_MEMBERS_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
    __TAGGED_UNION_ONLY_CPP20_PLUS(constexpr)							\
    ~AttrUnionImpl() BOOST_NOEXCEPT {/* The containing class needs to handle destruction*/} \
  };									\
									\
  using AttrUnion = AttrUnionImpl<::tagged_union::detail::UseExplicitDestructor<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>, true>; \
									\
  /* All these members need to be public, else this is not */		\
  /* an aggregate type and the zero-cost abstraction fails. */		\
  /* Just don't go messing with the members directly, please. */	\
  Type type;								\
  AttrUnion attr;							\
  /* Accessors and manilupators */					\
  constexpr Type const& get_type() const BOOST_NOEXCEPT {		\
    return type;							\
  }									\
  template<Type T>							\
  struct OfType { static constexpr Type type = T; };			\
  constexpr void check_type(Type const& expected_type) const {		\
    assert(type == expected_type);					\
  }									\
  /* So, C++ handles aggregate status... weirdly. */			\
  /* In C++17, the correct thing to do is delete the default */		\
  /* constructor, so that we can use non-trivial types as variants. */	\
  /* However... in C++20 simply mentioning the constructor */		\
  /* (even if just to delete it) will disqualify the aggregation */	\
  /* of this type. No mention of constructor is okay, due to */		\
  /* changes in union aggregation. */					\
  /* TLDR: In C++17, delete this constructor. In C++20, leave it be. */	\
  __TAGGED_UNION_ONLY_UNDER_CPP17(struct_name() = delete);				\
  /* It's tempting to ask for the default copy/move constructors, */	\
  /* but because AttrUnion isn't copy/move constructable it will  */	\
  /* be ignored. Thus, we need to define them manually.           */	\
  /* Similar logic applies to the others... SFINAE time           */	\
  struct_name(std::enable_if_t<::tagged_union::detail::UseCopyConstructor<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>, struct_name> const& other) noexcept				\
    (::tagged_union::detail::UseNoexceptCopyConstructor<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>) \
    : type(other.type), attr(/* Deffer */AttrUnion{.__empty = {}}) {	\
    switch (type) {							\
      BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_COPY_CONS_CASE_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)); \
    }									\
  }									\
  struct_name(std::enable_if_t<::tagged_union::detail::UseMoveConstructor<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>, struct_name>&& other) noexcept				\
    (::tagged_union::detail::UseNoexceptMoveConstructor<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>) \
    : type(std::move(other.type)), attr(/* Deffer */AttrUnion{.__empty = {}}) {	\
    switch (type) {							\
      BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_MOVE_CONS_CASE_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)); \
    }									\
  }									\
  std::enable_if_t<::tagged_union::detail::UseCopyAssigner<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>, struct_name>& operator=(struct_name const& other) \
    noexcept								\
    (::tagged_union::detail::UseNoexceptCopyAssigner<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>){	\
    if (type == other.type) {						\
      /* In-place assignment*/						\
      switch(type) {							\
	BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_COPY_ASSGN_CASE_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)); \
      }									\
    } else {								\
      /* Destroy and re-assign */					\
      this->~struct_name();						\
      new (this) struct_name(other);					\
    }									\
    return *this;							\
  }									\
  std::enable_if_t<::tagged_union::detail::UseMoveAssigner<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>, struct_name>& operator=(struct_name&& other) noexcept			\
    (::tagged_union::detail::UseNoexceptAssigner<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>) { \
    if (type == other.type) {						\
      /* In-place move assignment*/					\
      switch(type) {							\
	BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_MOVE_ASSGN_CASE_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)); \
      }									\
    } else {								\
      /* Destroy and re-assign */					\
      this->~struct_name();						\
      new (this) struct_name(std::move(other));				\
    }									\
    return *this;							\
  }									\
									\
  /* Plus, to help (default isn't available pre-C++20): */		\
  constexpr bool operator==(const struct_name& other) BOOST_NOEXCEPT {	\
    if(type != other.type)						\
      return false;							\
    /* Since we know the types are equal, we need to: */		\
    switch(type){							\
      BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_ATTREQ_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
	}								\
    /* Which won't be exhaustive if there are voids be exhaustive */	\
    return true;							\
  }									\
  constexpr bool operator!=(const struct_name& other) BOOST_NOEXCEPT {	\
    return !(*this == other);						\
  }									\
  /* And const versions of */						\
  constexpr bool operator==(const struct_name& other) const BOOST_NOEXCEPT { \
    if(type != other.type)						\
      return false;							\
    /* Since we know the types are equal, we need to: */		\
    switch(type){							\
      BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_ATTREQ_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
	}								\
    /* Which aught be exhaustive */					\
    return true;							\
  }									\
  constexpr bool operator!=(const struct_name& other) const BOOST_NOEXCEPT { \
    return !(*this == other);						\
  }									\
  /* Now we can move on with: */					\
  template<Type T, typename DataType>					\
  constexpr static ThisType create(DataType const& data) BOOST_NOEXCEPT { \
    return ThisType(data, OfType<T>());					\
  }									\
  template<Type T, typename DataType>					\
  constexpr static ThisType create(DataType && data) BOOST_NOEXCEPT {	\
    return ThisType(data, OfType<T>());					\
  }									\
  template<Type T> /* For void types */					\
  constexpr static ThisType create() BOOST_NOEXCEPT {			\
    return ThisType(OfType<T>());					\
  }									\
									\
  template<Type T, typename DataType>					\
  constexpr void set_type_and_data(DataType const& data) BOOST_NOEXCEPT { \
    return set_type_and_data_impl(data, OfType<T>());			\
  }									\
  template<Type T, typename DataType>					\
  constexpr void set_type_and_data(DataType && data) BOOST_NOEXCEPT {	\
    return set_type_and_data_impl(std::move(data), OfType<T>());	\
  }									\
  template<Type T> /* For void types */					\
  constexpr void set_type_and_data() BOOST_NOEXCEPT {			\
    return set_type_and_data_impl(OfType<T>());				\
  }									\
									\
  BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_ACCESSOR_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
  BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_SETTER_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
  BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_CONSTRUCTOR_FROM_TRIPLET, struct_name, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
									\
  /* Destructor requires manual management */				\
  /* We use some tricks to make this zero-cost, including: */		\
  /* - Out of band case labels */					\
  /* - Procedurally generated out-of-band values using magic_enum */	\
  /* - constexpr if spam */						\
  /* - __TAGGED_UNION_UNREACHABLE spam */				\
  __TAGGED_UNION_ONLY_CPP20_PLUS(constexpr)				\
  ~struct_name() noexcept(::tagged_union::detail::UseNoexceptDestructor< \
			  BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH	\
					    (TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, \
					     _,				\
					     BOOST_PP_VARIADIC_TO_SEQ(triplets)))>) { \
    if constexpr (::tagged_union::detail::UseExplicitDestructor<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)))>) { \
      auto constexpr values = magic_enum::enum_values<Type>();		\
      auto constexpr max_val = *std::max_element(values.begin(), values.end()); \
      switch(type) {							\
	BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_DTOR_SWITCH_FROM_TRIPLET, _, BOOST_PP_VARIADIC_TO_SEQ(triplets)) \
      default:;								\
      }									\
    }									\
  }
/*     TAGGED_UNION     */

#endif // TAGGED_UNION_H
