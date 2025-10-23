#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct SimpleExpected {
  TAGGED_UNION(SimpleExpected,
               (VALUE,  std::string, name),
               (BASE, size_t, base))

  std::string as_string() const {
    if(get_type() == BASE)
      return std::to_string(base());
    else
      return name();
  }
};

SimpleExpected fib(size_t n) {
  if (n <= 1) {
    return SimpleExpected::create<SimpleExpected::BASE>(n);
  } else {
    auto a = fib(n - 1);
    auto b = fib(n - 2);

    size_t a_val = a.get_type() == SimpleExpected::BASE ? a.base() : std::stoi(a.name());
    size_t b_val = b.get_type() == SimpleExpected::BASE ? b.base() : std::stoi(b.name());

    return SimpleExpected::create<SimpleExpected::VALUE>(std::to_string(a_val + b_val));
  }
}

int main() {
  for (size_t i = 0; i < 10; ++i) {
    auto result = fib(i);
    std::cout << "fib(" << i << ") = " << result.as_string() << "\n";
  }
}
