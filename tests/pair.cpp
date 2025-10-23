#include <tagged_union.hpp>
#include <string>
#include <iostream>
#include <utility>

struct Variant {
  TAGGED_UNION(Variant,
	       (SINGLE, bool, single),
	       (FIXED_SPREAD, std::pair<size_t, bool>, fixed_spread),
	       (AUTO_SPREAD, bool, auto_spread),
	       (NONE, void, void))
};

int main() {
  Variant v = Variant::create<Variant::SINGLE>(true);

  std::cout << v.single() << std::endl;

  v.set_type_and_data<Variant::FIXED_SPREAD>(std::make_pair(2, false));

  std::cout << v.fixed_spread().first << " * " << v.fixed_spread().second << std::endl;
  
  v.set_type_and_data<Variant::AUTO_SPREAD>(false);
  
  std::cout << v.auto_spread() << std::endl;
}
