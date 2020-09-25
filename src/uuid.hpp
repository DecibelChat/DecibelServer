#include <random>
#include <sstream>

namespace websocket_server::uuid
{
  class UUID4
  {
  public:
    UUID4() : random_generator_(random_device_()), distribution_1_(0, 15), distribution_2_(8, 11)
    {
    }

    std::string operator()()
    {
      std::stringstream ss;
      ss << std::hex;

      for (auto ii = 0; ii < 8; ++ii)
      {
        ss << distribution_1_(random_generator_);
      }
      ss << "-";
      for (auto ii = 0; ii < 4; ++ii)
      {
        ss << distribution_1_(random_generator_);
      }
      ss << "-4";
      for (auto ii = 0; ii < 3; ++ii)
      {
        ss << distribution_1_(random_generator_);
      }
      ss << "-";
      ss << distribution_2_(random_generator_);
      for (auto ii = 0; ii < 3; ++ii)
      {
        ss << distribution_1_(random_generator_);
      }
      ss << "-";
      for (auto ii = 0; ii < 12; ++ii)
      {
        ss << distribution_1_(random_generator_);
      };
      return ss.str();
    }

  private:
    std::random_device random_device_;
    std::mt19937 random_generator_;
    std::uniform_int_distribution<> distribution_1_;
    std::uniform_int_distribution<> distribution_2_;
  };

} // namespace websocket_server::uuid