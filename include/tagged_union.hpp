#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <type_traits>
#include <algorithm>
#include <cassert>
#include <utility>

#include <boost/version.hpp>
#ifndef BOOST_VERSION
#error ("BOOST_VERSION is not defined! Boost is required for TAGGED_UNION.")
#endif

#include <boost/config.hpp>
#include <boost/assert.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/rest_n.hpp>
#include <boost/preprocessor/seq/reverse.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/remove.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
  
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
#define __TAGGED_UNION_STRIP_PARENS_IMPL(...) __VA_ARGS__
#define __TAGGED_UNION_STRIP_PARENS(x) __TAGGED_UNION_STRIP_PARENS_IMPL x

#define __TAGGED_UNION_IS_VOID(...)					\
  __TAGGED_UNION_IS_VOID_IMPL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), (__VA_ARGS__))

#define __TAGGED_UNION_IS_VOID_IMPL(size, args) \
  BOOST_PP_IF(BOOST_PP_EQUAL(size, 1),		\
	      __TAGGED_UNION_CHECK_VOID_DEFER,	\
	      __TAGGED_UNION_RETURN_ZERO) args

#define __TAGGED_UNION_CHECK_VOID_DEFER(x)				\
  BOOST_PP_IS_EMPTY(BOOST_PP_CAT(__TAGGED_UNION_IS_VOID_HELPER_, x))

#define __TAGGED_UNION_RETURN_ZERO(...) 0

#define __TAGGED_UNION_IS_VOID_HELPER_void

#define TAGGED_UNION_TAGNAME(triplet)		\
  BOOST_PP_TUPLE_ELEM(0, triplet)

#define TAGGED_UNION_TUPLETYPE(triplet)					\
  __TAGGED_UNION_STRIP_PARENS(BOOST_PP_TUPLE_ELEM(3, 1, triplet))

#define TAGGED_UNION_FIELDNAME(triplet)					\
  BOOST_PP_TUPLE_ELEM(BOOST_PP_DEC(BOOST_PP_TUPLE_SIZE(triplet)), triplet)

#define TAGGED_UNION_TUPLETYPE_IS_VOID(triplet)			\
  __TAGGED_UNION_IS_VOID(TAGGED_UNION_TUPLETYPE(triplet))

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
#define TAGGED_UNION_TUPLE_MIDDLE_TO_SEQ(tuple)				\
  BOOST_PP_TUPLE_REMOVE(BOOST_PP_TUPLE_REMOVE(tuple, BOOST_PP_DEC(BOOST_PP_TUPLE_SIZE(tuple))), 0)
#define TAGGED_UNION_MIDDLE_WRAP(r, data, tuple)			\
  ((TAGGED_UNION_TAGNAME(tuple),					\
    TAGGED_UNION_TUPLE_MIDDLE_TO_SEQ(tuple),				\
    TAGGED_UNION_FIELDNAME(tuple)))
#define TAGGED_UNION_VARIADIC_TO_TRIPLETS(triplets...)		\
  BOOST_PP_SEQ_FOR_EACH(TAGGED_UNION_MIDDLE_WRAP, _,		\
			BOOST_PP_VARIADIC_TO_SEQ(triplets))
#define TAGGED_UNION_VARIADIC_FOREACH(fn, data, triplets...)		\
  BOOST_PP_SEQ_FOR_EACH(fn, data, TAGGED_UNION_VARIADIC_TO_TRIPLETS(triplets))
#define TAGGED_UNION_ENUM_FROM_TRIPLET(r, data, triplet)	\
  TAGGED_UNION_TAGNAME(triplet),
#define TAGGED_UNION_MAX_VARIANT_ENUM(triplets)				\
  TAGGED_UNION_TAGNAME(BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(triplets)))
#define TAGGED_UNION_TYPE_PACK_FROM_TRIPLET(r, data, triplet)	\
  BOOST_PP_IF							\
  (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),			\
   BOOST_PP_EMPTY(),						\
   BOOST_PP_VARIADIC_TO_SEQ(TAGGED_UNION_TUPLETYPE(triplet)))
#define TAGGED_UNION_TYPENAME_LIST(triplets...)				\
  BOOST_PP_SEQ_ENUM(TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_TYPE_PACK_FROM_TRIPLET, _, triplets))
#define TAGGED_UNION_MEMBERS_FROM_TRIPLET(r, data, triplet)		\
  __TAGGED_UNION_STRIP_PARENS						\
  (BOOST_PP_IF								\
   (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
    (/* emit nothing if void*/),					\
    (TAGGED_UNION_TUPLETYPE(triplet) TAGGED_UNION_FIELDNAME(triplet);)))
#define TAGGED_UNION_ACCESSOR_FROM_TRIPLET(r, union_name, triplet)	\
  __TAGGED_UNION_STRIP_PARENS						\
  (BOOST_PP_IF								\
   (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
    (/* emit nothing if void*/),					\
    (/* We std::as_const because if this the type is already const, */	\
     /* like a int const*, then we'll get a duplicate const error */	\
     [[nodiscard]] constexpr std::add_const_t<TAGGED_UNION_TUPLETYPE(triplet)>& TAGGED_UNION_FIELDNAME(triplet)() const { \
      check_type(TAGGED_UNION_TAGNAME(triplet));			\
      return storage.attr.TAGGED_UNION_FIELDNAME(triplet);		\
    }									\
     [[nodiscard]] constexpr TAGGED_UNION_TUPLETYPE(triplet)& TAGGED_UNION_FIELDNAME(triplet)() { \
      check_type(TAGGED_UNION_TAGNAME(triplet));			\
      return storage.attr.TAGGED_UNION_FIELDNAME(triplet);		\
    })))
#define TAGGED_UNION_COPY_CONS_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case TAGGED_UNION_TAGNAME(triplet):					\
  __TAGGED_UNION_STRIP_PARENS						\
  (BOOST_PP_IF								\
   (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
    (/* emit nothing if void*/),					\
    (new (&storage.attr.TAGGED_UNION_FIELDNAME(triplet)) TAGGED_UNION_TUPLETYPE(triplet)(other.storage.attr.TAGGED_UNION_FIELDNAME(triplet))))); \
  break;
#define TAGGED_UNION_MOVE_CONS_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case TAGGED_UNION_TAGNAME(triplet):					\
  __TAGGED_UNION_STRIP_PARENS						\
  (BOOST_PP_IF								\
   (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
    (/* emit nothing if void*/),					\
    (new (&storage.attr.TAGGED_UNION_FIELDNAME(triplet)) TAGGED_UNION_TUPLETYPE(triplet)(std::move(other.storage.attr.TAGGED_UNION_FIELDNAME(triplet)))))); \
  break;
#define TAGGED_UNION_MOVE_ASSGN_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case TAGGED_UNION_TAGNAME(triplet):					\
  BOOST_PP_IF								\
  (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
   /* emit nothing if void*/,						\
   storage.attr.TAGGED_UNION_FIELDNAME(triplet) = std::move(other.storage.attr.TAGGED_UNION_FIELDNAME(triplet))); \
  break;
#define TAGGED_UNION_COPY_ASSGN_CASE_FROM_TRIPLET(r, union_name, triplet) \
  case TAGGED_UNION_TAGNAME(triplet):					\
  BOOST_PP_IF								\
  (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
   /* emit nothing if void*/,						\
   storage.attr.TAGGED_UNION_FIELDNAME(triplet) = other.storage.attr.TAGGED_UNION_FIELDNAME(triplet)); \
  break;
#define TAGGED_UNION_DTOR_SWITCH_FROM_TRIPLET(r, union_name, triplet)	\
  __TAGGED_UNION_STRIP_PARENS						\
  (BOOST_PP_IF								\
   (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
    (/* emit nothing if void... we don't even need to print the case label*/), \
    (case (TAGGED_UNION_TAGNAME(triplet)				\
	   + std::is_trivially_destructible_v<TAGGED_UNION_TUPLETYPE(triplet)> \
	   ? max_val+1 : 0):						\
     if constexpr(std::is_trivially_destructible_v<TAGGED_UNION_TUPLETYPE(triplet)>) { \
       /* We want to signal to the compiler that this path will never */ \
       /* be taken for optimization purposes. However, if we include */	\
       /* an assert, it can trigger in a C++20 constexpr destructor */	\
       /* call. So we need to if constexpr whether or not we're */	\
       /* being evaluated in a constexpr context */			\
       /* Because this is only a problem in C++20, we can use: */	\
       __TAGGED_UNION_ONLY_CPP20_PLUS(if constexpr(!std::is_constant_evaluated())) \
       __TAGGED_UNION_UNREACHABLE();					\
     } else {								\
       using __typePunt = TAGGED_UNION_TUPLETYPE(triplet);		\
       attr.TAGGED_UNION_FIELDNAME(triplet).~__typePunt();		\
       break;								\
     })))
#define TAGGED_UNION_SETTER_FROM_TRIPLET(r, data, triplet)		\
  __TAGGED_UNION_STRIP_PARENS						\
  (BOOST_PP_IF								\
   (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
    (void set_type_and_data_impl(OfType<TAGGED_UNION_TAGNAME(triplet)>) BOOST_NOEXCEPT { \
      /* Make sure to conditionally destroy the current data first */	\
      /* Since this is a voidy type the hot path is always destroying */ \
      /* AKA we can just: */						\
      this->~ThisType();						\
      storage.type = TAGGED_UNION_TAGNAME(triplet);			\
    }),									\
    (void set_type_and_data_impl(TAGGED_UNION_TUPLETYPE(triplet) const& value, OfType<TAGGED_UNION_TAGNAME(triplet)>) BOOST_NOEXCEPT { \
      /* Because there is data here, we shouldn't use the same hot-path assumption as in the void case */ \
      /* Why? Because destroy+placement new is likely slower than copy/move assignment */ \
      if (storage.type == TAGGED_UNION_TAGNAME(triplet)) {		\
	/* If same type, use assignment */				\
	storage.attr.TAGGED_UNION_FIELDNAME(triplet) = value;		\
      } else {								\
	/* Otherwise, destruct and the use placement-new. */		\
	/* The idea is we don't want to move on top of uninit'd data */ \
	/* since the destination may have a non-trivial assignment */	\
	this->~ThisType();						\
	new (&storage.attr.TAGGED_UNION_FIELDNAME(triplet))		\
	  TAGGED_UNION_TUPLETYPE(triplet)(value);			\
      }									\
      storage.type = TAGGED_UNION_TAGNAME(triplet);			\
    }									\
      void set_type_and_data_impl(TAGGED_UNION_TUPLETYPE(triplet) && value, OfType<TAGGED_UNION_TAGNAME(triplet)>) BOOST_NOEXCEPT { \
	/* Same shenanigans as above except with std::move) */		\
	if (storage.type == TAGGED_UNION_TAGNAME(triplet)) {		\
	  storage.attr.TAGGED_UNION_FIELDNAME(triplet) = std::move(value); \
	} else {							\
	  this->~ThisType();						\
	  new (&storage.attr.TAGGED_UNION_FIELDNAME(triplet))		\
	    TAGGED_UNION_TUPLETYPE(triplet)(std::move(value));		\
	}								\
	storage.type = TAGGED_UNION_TAGNAME(triplet);			\
      })))
#define TAGGED_UNION_CONSTRUCTOR_FROM_TRIPLET(r, struct_name, triplet)	\
  /* Overload constructor */						\
  __TAGGED_UNION_STRIP_PARENS						\
  (BOOST_PP_IF								\
   (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),				\
    (constexpr struct_name(OfType<TAGGED_UNION_TAGNAME(triplet)>)	\
     BOOST_NOEXCEPT : storage(Storage{					\
	 .type = TAGGED_UNION_TAGNAME(triplet) BOOST_PP_COMMA()		\
	   .attr = AttrUnion {.__empty = {}}				\
       }) {}),								\
    (/* We template these to defer the constexpr check */		\
     template<typename DummyDeffer>					\
     constexpr struct_name(DummyDeffer const& value,			\
			   OfType<TAGGED_UNION_TAGNAME(triplet)>)	\
     noexcept(::tagged_union::detail::UseNoexceptCopyConstructor<TAGGED_UNION_TUPLETYPE(triplet)>) \
     : storage{								\
       TAGGED_UNION_TAGNAME(triplet) BOOST_PP_COMMA()			\
       AttrUnion {.TAGGED_UNION_FIELDNAME(triplet) = value}		\
     } {}								\
     template<typename DummyDeffer>					\
     constexpr struct_name(DummyDeffer && value,			\
			   OfType<TAGGED_UNION_TAGNAME(triplet)>)	\
     noexcept(::tagged_union::detail::UseNoexceptMoveConstructor<TAGGED_UNION_TUPLETYPE(triplet)>) \
     : storage{								\
       TAGGED_UNION_TAGNAME(triplet) BOOST_PP_COMMA()			\
	 AttrUnion {.TAGGED_UNION_FIELDNAME(triplet) = std::move(value)} \
     } {})))
#define TAGGED_UNION_ATTREQ_FROM_TRIPLET(r, data, triplet)	\
  case TAGGED_UNION_TAGNAME(triplet):				\
  BOOST_PP_IF							\
  (TAGGED_UNION_TUPLETYPE_IS_VOID(triplet),			\
   /* Emit nothing, since we're going to return true*/,		\
   return storage.attr.TAGGED_UNION_FIELDNAME(triplet)		\
   == other.storage.attr.TAGGED_UNION_FIELDNAME(triplet));
#define TAGGED_UNION(struct_name, triplets...)				\
  public:								\
  using ThisType = struct_name;						\
  enum Type {								\
    TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_ENUM_FROM_TRIPLET, _, triplets) \
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
    ::tagged_union::detail::monostate __empty;				\
    TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_MEMBERS_FROM_TRIPLET, _, triplets) \
  };									\
									\
  template <bool DummyDefer>						\
  union AttrUnionImpl<true, DummyDefer> {				\
    ::tagged_union::detail::monostate __empty;				\
    TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_MEMBERS_FROM_TRIPLET, _, triplets) \
    __TAGGED_UNION_ONLY_CPP20_PLUS(constexpr)				\
    ~AttrUnionImpl() BOOST_NOEXCEPT {/* The containing class needs to handle destruction*/} \
  };									\
  									\
  using AttrUnion = AttrUnionImpl<::tagged_union::detail::UseExplicitDestructor<TAGGED_UNION_TYPENAME_LIST(triplets)>, true>; \
  									\
  /* All these members need to be public, else this is not */		\
  /* an aggregate type and the zero-cost abstraction fails. */		\
  /* Just don't go messing with the members directly, please. */	\
  /*                                                          */	\
  /* We also sub-class out a container for type/attr in order */	\
  /* to allow for destructor erasure if needed. This allows */		\
  /* simple TAGGED_UNIONS to be constexpr-able even in C++17 */		\
  template <bool UseExplicitDestructor, bool DummyDefer>		\
  struct StorageImpl;							\
  									\
  template <bool DummyDefer>						\
  struct StorageImpl<false, DummyDefer> {				\
    Type type;								\
    AttrUnion attr;							\
  };									\
  									\
  template <bool DummyDefer>						\
  struct StorageImpl<true, DummyDefer> {				\
    Type type;								\
    AttrUnion attr;							\
    __TAGGED_UNION_ONLY_CPP20_PLUS(constexpr)				\
    ~StorageImpl()							\
      noexcept(::tagged_union::detail::UseNoexceptDestructor<		\
  	       TAGGED_UNION_TYPENAME_LIST(triplets)>) {			\
      auto constexpr max_val = TAGGED_UNION_MAX_VARIANT_ENUM(BOOST_PP_VARIADIC_TO_SEQ(triplets)); \
      switch(type) {							\
  	TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_DTOR_SWITCH_FROM_TRIPLET, _, triplets) \
      default:;								\
      }									\
    }									\
  };									\
  									\
  using Storage = StorageImpl<::tagged_union::detail::UseExplicitDestructor<TAGGED_UNION_TYPENAME_LIST(triplets)>, true>; \
  									\
  Storage storage;							\
  									\
  /* Accessors and manilupators */					\
  constexpr Type const& get_type() const BOOST_NOEXCEPT {		\
    return storage.type;						\
  }									\
  template<Type T>							\
  struct OfType { static constexpr Type type = T; };			\
  constexpr void check_type(Type const& expected_type) const {		\
    assert(storage.type == expected_type);				\
  }									\
  /* So, C++ handles aggregate status... weirdly. */			\
  /* In C++17, the correct thing to do is delete the default */		\
  /* constructor, so that we can use non-trivial types as variants. */	\
  /* However... in C++20 simply mentioning the constructor */		\
  /* (even if just to delete it) will disqualify the aggregation */	\
  /* of this type. No mention of constructor is okay, due to */		\
  /* changes in union aggregation. */					\
  /* TLDR: In C++17, delete this constructor. In C++20, leave it be. */	\
  __TAGGED_UNION_ONLY_UNDER_CPP17(struct_name() = delete);		\
  /* It's tempting to ask for the default copy/move constructors, */	\
  /* but because AttrUnion isn't copy/move constructable it will  */	\
  /* be ignored. Thus, we need to define them manually.           */	\
  /* Similar logic applies to the others... SFINAE time           */	\
  struct_name(std::enable_if_t<::tagged_union::detail::UseCopyConstructor<TAGGED_UNION_TYPENAME_LIST(triplets)>, struct_name> const& other) noexcept \
    (::tagged_union::detail::UseNoexceptCopyConstructor<TAGGED_UNION_TYPENAME_LIST(triplets)>) \
    : storage(Storage{							\
  	.type = other.storage.type,					\
  	  .attr = /* Deffer */AttrUnion{.__empty = {}}			\
      }) {								\
    switch (storage.type) {						\
      TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_COPY_CONS_CASE_FROM_TRIPLET, _, triplets); \
    }									\
  }									\
  struct_name(std::enable_if_t<::tagged_union::detail::UseMoveConstructor<TAGGED_UNION_TYPENAME_LIST(triplets)>, struct_name>&& other) noexcept \
    (::tagged_union::detail::UseNoexceptMoveConstructor<TAGGED_UNION_TYPENAME_LIST(triplets)>) \
    : storage(Storage{							\
  	.type = std::move(other.storage.type),				\
  	  .attr = /* Deffer */AttrUnion{.__empty = {}}			\
      }) {								\
    switch (storage.type) {						\
      TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_MOVE_CONS_CASE_FROM_TRIPLET, _, triplets); \
    }									\
  }									\
  std::enable_if_t<::tagged_union::detail::UseCopyAssigner<TAGGED_UNION_TYPENAME_LIST(triplets)>, struct_name>& operator=(struct_name const& other) \
    noexcept								\
    (::tagged_union::detail::UseNoexceptCopyAssigner<TAGGED_UNION_TYPENAME_LIST(triplets)>){ \
    if (storage.type == other.storage.type) {				\
      /* In-place assignment*/						\
      switch(storage.type) {						\
	TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_COPY_ASSGN_CASE_FROM_TRIPLET, _, triplets); \
      }									\
    } else {								\
      /* Destroy and re-assign */					\
      this->~struct_name();						\
      new (this) struct_name(other);					\
    }									\
    return *this;							\
  }									\
  std::enable_if_t<::tagged_union::detail::UseMoveAssigner<TAGGED_UNION_TYPENAME_LIST(triplets)>, struct_name>& operator=(struct_name&& other) noexcept \
    (::tagged_union::detail::UseNoexceptAssigner<TAGGED_UNION_TYPENAME_LIST(triplets)>) { \
    if (storage.type == other.storage.type) {				\
      /* In-place move assignment*/					\
      switch(storage.type) {						\
	TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_MOVE_ASSGN_CASE_FROM_TRIPLET, _, triplets); \
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
    if(storage.type != other.storage.type)				\
      return false;							\
    /* Since we know the types are equal, we need to: */		\
    switch(storage.type){						\
      TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_ATTREQ_FROM_TRIPLET, _, triplets) \
  	}								\
    /* Which won't be exhaustive if there are voids be exhaustive */	\
    return true;							\
  }									\
  constexpr bool operator!=(const struct_name& other) BOOST_NOEXCEPT {	\
    return !(*this == other);						\
  }									\
  /* And const versions of */						\
  constexpr bool operator==(const struct_name& other) const BOOST_NOEXCEPT { \
    if(storage.type != other.storage.type)				\
      return false;							\
    /* Since we know the types are equal, we need to: */		\
    switch(storage.type){						\
      TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_ATTREQ_FROM_TRIPLET, _, triplets) \
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
  TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_ACCESSOR_FROM_TRIPLET, _, triplets) \
  TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_SETTER_FROM_TRIPLET, _, triplets) \
  TAGGED_UNION_VARIADIC_FOREACH(TAGGED_UNION_CONSTRUCTOR_FROM_TRIPLET, struct_name, triplets) \
 									\
  /* Destructor is entirely un-specified because we use Storage<_, _> */
// /*     TAGGED_UNION     */

#endif // TAGGED_UNION_H
