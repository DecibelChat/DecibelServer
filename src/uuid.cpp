#include "uuid.hpp"

#include <sstream>

namespace websocket_server::uuid
{
  UUID4::UUID4() : random_generator_(random_device_()), distribution_1_(0, 15), distribution_2_(8, 11)
  {
  }

  std::string UUID4::operator()()
  {
    std::stringstream ss;
    ss << std::hex;

    constexpr auto n_1 = 8;
    constexpr auto n_2 = 4;
    constexpr auto n_3 = 3;
    constexpr auto n_4 = 3;
    constexpr auto n_5 = 12;

    auto generate_n = [this, &ss](std::size_t n) {
      for (std::size_t ii = 0; ii < n; ++ii)
      {
        ss << distribution_1_(random_generator_);
      }
    };

    generate_n(n_1);

    ss << "-";

    generate_n(n_2);

    ss << "-4";

    generate_n(n_3);

    ss << "-";
    ss << distribution_2_(random_generator_);

    generate_n(n_4);

    ss << "-";

    generate_n(n_5);

    return ss.str();
  }

} // namespace websocket_server::uuid