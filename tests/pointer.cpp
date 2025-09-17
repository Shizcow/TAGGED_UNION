#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct Variant {
  TAGGED_UNION(Variant,
	       (CONST, int const*, c),
	       (MUT,   int *, m))
};

int main() {
  int i = 0;
  Variant v = Variant::create<Variant::MUT>(&i);

  std::cout << *v.m() << std::endl;

  *v.m() += 1;

  v.set_type_and_data<Variant::CONST>(&i); // Const cast not required
  
  std::cout << *v.c() << std::endl;
}
