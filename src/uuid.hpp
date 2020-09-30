#pragma once

#include <random>

namespace websocket_server::uuid
{
  class UUID4
  {
  public:
    UUID4();

    std::string operator()();

  private:
    std::random_device random_device_;
    std::mt19937 random_generator_;
    std::uniform_int_distribution<> distribution_1_;
    std::uniform_int_distribution<> distribution_2_;
  };

} // namespace websocket_server::uuid