#include <tagged_union.hpp>
#include <iostream>

// Example of what an "empty" variant may look like.
// This compiles away entirely in release mode, but provides checks in debug mode.
//
// This is an advanced example with manual memory management.
// This use is discouraged unless you are a competent programmer and really,
// honestly need to save those last few cycles.

struct Target {
  TAGGED_UNION(Target,
	       (ID, size_t, id),
	       (EXPLICITLY_UNASSIGNED, void, void)
#ifndef NDEBUG
	       , (NOTYETASSIGNED, void, void)
#endif
	       )

  static Target create_empty() noexcept {
#ifdef NDEBUG
    // Release: we just leave this entirely uninitialized.
    // Debug mode includes a check to crash out when freeing
    // a NOTYETASSIGNED type, which will protect from
    // accidental frees of uninitilized values in release mode.
    // This lets the destructor run a bit faster.

    // We're allowed to leave these uninitialized and require a call
    // to set_type_and_data later so long as all union types are
    // trivially constructable per https://eel.is/c++draft/class.union#general-1

    // Note that the above is only true in release mode.
    // In debug mode, obviously, the type can be more complex.
    // Although we use size_t in this example, this pattern will
    // apply to *all custom types.
    // *: Actually, all types with literally empty destructors are fine,
    //    but there's no way to test for that in C++. So it is recommended
    //    to only explicitly assert for the types which are expected to be
    //    trivially destructable, and thoroughly document types that are
    //    expected to have literally empty destructors.
    //    One example of such a type would be another TAGGED_UNION with
    //    an empty case -- it's legal to nest these and the destructors
    //    will all have zero code (thanks to if constexpr). Fast!
    static_assert(std::is_trivially_constructible_v<size_t>,
		  "Target::create_empty() requires IDType to be trivially constructible in release mode");
    static_assert(std::is_trivially_destructible_v<size_t>,
		  "Target::create_empty() requires IDType to be trivially destructible in release mode");
    
    // Note: This is technically UB.
    alignas(Target) std::byte storage[sizeof(Target)];
    auto* ptr = reinterpret_cast<Target*>(&storage);

    // This is a trick to make valgrind not complain about the
    // uninitialized tag read in the destructor. Note that this
    // is genuinely not an issue, since the destructor will only
    // call literally empty constructors. In fact, this specific
    // example won't even generate a constructor because TAGGED_UNION
    // will check for all trivially destructible variants.
    //
    // The macro is un-commented here as to not pull in valgrind as
    // a dependency, but it's useful. You only need the header:
    //   <valgrind/memcheck.h>
    // VALGRIND_MAKE_MEM_DEFINED(&ptr->type, sizeof(ptr->type));
	  
    auto ret = std::move(*ptr);
    ret.attr.__empty = {};
    return ret;
#else
    // Debug
    return create<NOTYETASSIGNED>();
#endif
  }

  // It is assumed that the programmer won't interact with an empty
  // object. In debug mode, there will be a check for this.
  // But in release mode, this is VERY undefined behavior.
};

int main() {
  Target t = Target::create_empty();

  // std::cout << t.id() << std::endl; // <- NO!!!!!

  t.set_type_and_data<Target::ID>(5); // Perfectly fine
  std::cout << t.id() << std::endl;   // And now the object is well formed
}
