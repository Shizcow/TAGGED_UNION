#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct Variant {
  TAGGED_UNION(Variant,
	       (LONG, long, lo),
	       (INTEGER, int, in))
};

int main() {
  Variant v = Variant::create<Variant::LONG>(101l);

  std::cout << v.lo() << std::endl;

  v.set_type_and_data<Variant::INTEGER>(42);
  
  std::cout << v.in() << std::endl;
}
