#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct Variant {
  TAGGED_UNION(Variant,
	       // we use string_view because if we are compiling in C++17 std::string is non-constexpr
	       (STRING, std::string_view, string),
	       (INTEGER, int, integer))
};

constexpr size_t foo(Variant const& v) {
  return v.string().size();
}

struct VariantLiteralString {
  TAGGED_UNION(VariantLiteralString,
	       (STRING, std::string, string),
	       (INTEGER, int, integer))
};

size_t foo(VariantLiteralString const& v) {
  return v.string().size();
}

int main() {
  constexpr std::string_view data("hello world!");
  
  {
    // We can declare the type, but we can't actually create it in constexpr context
    // unless we're in C++20 or up.
#if __cplusplus > 201703L
    constexpr Variant v = Variant::create<Variant::STRING>(data);
    constexpr size_t s = foo(v);
    std::cout << s << std::endl;
#endif
  }

  {
    // But it should be fine to create the non-constexpr version
    VariantLiteralString v = VariantLiteralString::create<VariantLiteralString::STRING>(std::string(data));
    size_t s = foo(v);
    std::cout << s << std::endl;
  }
}
