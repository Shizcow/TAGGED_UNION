## TAGGED_UNION

A performant tagged union implementation for C++17 and above.
It supports complex types, data-less variants, multiple same-type variants, and is faster than either `boost::variant` or `std::variant`.
Tagged unions are never-empty, but support `placement new` style empty initialization hacks so long as each variant type is trivially constructable and destructable (or another never-empty-able tagged union) -- see `tests/empty.cpp`.

### Quick Start
```C++
#include <tagged_union.hpp>

struct Property {
  TAGGED_UNION(Property,
               (DENSITY, float, density),
               (COUNT, int, count))
};
```

More examples can be found in the `tests` directory.

### Syntax
Calling the `TAGGED_UNION` macro from within a `struct`/`class` definition will transform it into a tagged union type.
The syntax of this macro is:
```
TAGGED_UNION(<struct_name>,
             (<enum variant name>, <variant type>, <variant accessor name>),
             ...)
```

Note that multiple variants of the same type are allowed:
```
struct Dimension {
  TAGGED_UNION(Property,
               (WIDTH, float, width),
               (HEIGHT, float, height))
};
```

As are `void` variants that store no data:
```
struct Dimension {
  TAGGED_UNION(Property,
               (WIDTH, float, width),
               (HEIGHT, float, height),
			   (NULL, void, void))
};
```

A `TAGGED_UNION` type supports the meta methods:
- `<tag_type> const& get_type()`
  - E.g. returns `WIDTH`
- `template<TAG_TYPE tag_type> static <struct_name> create(tag_type data)`
  - E.g. `Dimension d = Dimension::create<Dimension::WIDTH>(3.0);`
- `template<TAG_TYPE tag_type> void set_type_and_data(tag_type data)`
  - E.g. `d.set_type_and_data<Dimension::HEIGHT>(9.0);`
  
And then each of the variants have their own auto-generated reference getter method.
For example, `d.width()` will get a reference to internal `float` data and also perform a type check for `WIDTH` (if `NDEBUG` is not defined).

Equality operators, copy/move constructors, and other creature comforts are automatically generated when the variant types support them.
In this way, `TAGGED_UNION` is as flexible as the types it contains.

### Notes on C++ Version
This library is built to be portable, extremely fast, and sensitive to the C++ version used.
Although the project officially supports C++17, using C++20 or above will improve language features (e.g. `constexpr` destructors).
This is all to say that any `TAGGED_UNION` types should act exactly as expected relative to the C++ version currently in use.

Note that while many C++17 features are relied on, the bones of this library could theoretically be written for C++11 (would require replacing `magic_enum`, doing many of the `if constexpr` checks at runtime, etc).
This is to say, if you're reading this and you *really* want to use this project for an older C++ version, it is possible with effort, though the code will run slower.

### Building the Tests
```sh
cmake -S . -B build
cmake --build build
ctest --output-on-failure --test-dir=build/tests
```

### Building as a CMake Dependency
The following is sufficient to import this as a dependency in a Cmake project:
```cmake
include(FetchContent)

FetchContent_Declare(
  tagged_union
  GIT_REPOSITORY https://github.com/Shizcow/TAGGED_UNION.git
  GIT_TAG master
)

# If desired:
set(TAGGED_UNION_BUILD_TESTS OFF CACHE BOOL "Disable TU tests" FORCE)

FetchContent_MakeAvailable(tagged_union)

# And then later:
target_link_libraries(your_thing PRIVATE tagged_union)
```

### Bugs
This started as an in-house development, so while it has been rigorously tested in vivio, it is not guaranteed to work with *your* project.
Please, file any issues so this tool can get better for everyone!
