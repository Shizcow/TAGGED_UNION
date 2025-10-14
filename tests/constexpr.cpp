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
// struct Hard {
//   TAGGED_UNION(Hard,
// 	       (STRING, std::string, string),
// 	       (INTEGER, int, integer))
// };

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
}
