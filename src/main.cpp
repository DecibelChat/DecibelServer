
#include <cxxopts.hpp>

#include "server.hpp"

#include <fmt/color.h>
#include <fmt/format.h>

websocket_server::Parameters parse_arguments(int argc, char **argv)
{
  using websocket_server::Parameters;
  Parameters params;

  try
  {
    cxxopts::Options options(argv[0], "Run a websocket server");

    options.add_options()("h,help", "Print usage");

    options.add_options()("p,port",
                          "The port to expose the server on",
                          cxxopts::value<decltype(Parameters::port)>(params.port)->default_value("16666"));
    options.add_options()("k,keyfile",
                          "<required> The file containing the SSL private key",
                          cxxopts::value<decltype(Parameters::key_file)>(params.key_file));
    options.add_options()("c,certfile",
                          "<required> The file containing the SSL certificate",
                          cxxopts::value<decltype(Parameters::cert_file)>(params.cert_file));
    options.add_options()("verbose",
                          "If true, server will print verbose debugging information to the console.",
                          cxxopts::value<decltype(Parameters::verbose)>(params.verbose)->default_value("false"));
    options.add_options()("s,logger_max_size",
                          "Max size of rotating log files, in MB. Default is 0, or infinite.",
                          cxxopts::value<decltype(Parameters::max_log_mb)>(params.max_log_mb)->default_value("0"));
    options.add_options()("o,logger_output_file",
                          "Filename for logs.",
                          cxxopts::value<decltype(Parameters::log_file)>(params.log_file));

    auto result = options.parse(argc, argv);

    auto print_help = [&options](int exit_code = EXIT_SUCCESS) {
      fmt::print(stderr, "{}\n", options.help());

      std::exit(exit_code);
    };

    auto handle_required_argument = [&result, print_help](const std::string &argument) {
      if (result.count(argument) == 0)
      {
        fmt::print(stderr, fg(fmt::color::orange_red), "required argument \"{}\" is missing\n", argument);

        print_help(EXIT_FAILURE);
      }
    };

    if (result.count("help") > 0)
    {
      print_help();
    }

    if constexpr (websocket_server::using_TLS)
    {
      handle_required_argument("certfile");
      handle_required_argument("keyfile");
    }
  }
  catch (const cxxopts::OptionException &e)
  {
    fmt::print(stderr, "error parsing command line options: {}\n", e.what());
    std::exit(EXIT_FAILURE);
  }

  return params;
}

int main(int argc, char **argv)
{
  auto options = parse_arguments(argc, argv);

  websocket_server::WSS server(options);

  server.start();
}
