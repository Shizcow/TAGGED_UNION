#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct Variant {
  TAGGED_UNION(Variant,
	       (STRING, std::string_view, string),
	       (INTEGER, int, integer))
};

constexpr size_t foo(Variant const& v) {
  return v.string().size();
}

int main() {
  constexpr std::string_view data("hello world!");
  constexpr Variant v = Variant::create<Variant::STRING>(data);
  constexpr size_t s = foo(v);
  std::cout << s << std::endl;
}
