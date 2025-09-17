#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct Variant {
  TAGGED_UNION(Variant,
	       (STRING, std::string, string),
	       (INTEGER, int, integer))
};

int main() {
  Variant v = Variant::create<Variant::STRING>("hello world!");

  std::cout << v.string() << std::endl;

  v.set_type_and_data<Variant::INTEGER>(42);
  
  std::cout << v.integer() << std::endl;
}
