#include <tagged_union.hpp>
#include <string>
#include <iostream>

struct Easy {
  TAGGED_UNION(Easy,
	       (STRING, const char*, string),
	       (INTEGER, int, integer))
};
struct Medium {
  TAGGED_UNION(Medium,
	       (STRING, std::string_view, string),
	       (INTEGER, int, integer))
};
struct Hard {
  // Even in C++17, simply declaring this class
  // should not generate any constexpr-related
  // warnings.
  TAGGED_UNION(Hard,
	       (STRING, std::string, string),
	       (INTEGER, int, integer))
};

template<typename T>
constexpr size_t v_size(T const& v) {
  return v.string().size();
}

template<>
constexpr size_t v_size(Easy const& v) {
    std::size_t len = 0;
    while (v.string()[len] != '\0')
        ++len;
    return len;
}

int main() {
  constexpr const char* data_cstr = "hello world!";

  constexpr Easy easy = Easy::create<Easy::STRING>(data_cstr);
  constexpr size_t easy_size = v_size(easy);
  std::cout << "Easy: " << easy_size << " chars" << std::endl;
  
  constexpr std::string_view data_sv(data_cstr);

  constexpr Medium medium = Medium::create<Medium::STRING>(data_cstr);
  constexpr size_t medium_size = v_size(medium);
  std::cout << "Medium: " << medium_size << " chars" << std::endl;

  // Even into C++20, we can't create a constexpr std::string:
  std::string data_ss(data_cstr);

  Hard hard = Hard::create<Hard::STRING>(std::string(data_ss));
  size_t hard_size = v_size(hard);
  std::cout << "Hard: " << hard_size << " chars" << std::endl;

  // Now, just because we can't constexpr-create a string-variant Hard,
  // doesn't mean we can't constexpr-create a different variant.
  //
  // This is only true in C++20 though, since we still have the
  // non-trivial destructor check.
  __TAGGED_UNION_ONLY_CPP20_PLUS(constexpr)
    Hard hard2 = Hard::create<Hard::INTEGER>(100);
  
  std::cout << "Hard2: " << hard2.integer() << " integer" << std::endl;
}
