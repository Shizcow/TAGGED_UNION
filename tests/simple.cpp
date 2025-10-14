#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct Variant {
  TAGGED_UNION(Variant,
	       (SHORT, short, sh),
	       (INTEGER, int, in))
};

int main() {
  Variant v = Variant::create<Variant::SHORT>(101);

  std::cout << v.sh() << std::endl;

  v.set_type_and_data<Variant::INTEGER>(42);
  
  std::cout << v.in() << std::endl;
}
