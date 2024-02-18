#pragma once

#include <string>
#include <vector>

#include "fmt.hh"

namespace devkit
{

class Args
{
public:
  std::string program;
  std::vector<std::string> subcommands;
  std::unordered_map<std::string, std::string> options;
  std::vector<std::string> rest_arguments;

public:
  Args(int argc, const char* argv[])
  {
    program = argv[0];

    int i = 1;
    while (i < argc && argv[i][0] != '-') {
      subcommands.emplace_back(argv[i]);
      i += 1;
    }

    while (i < argc) {
      auto arg = std::string{ argv[i] };
      auto arg_size = arg.size();

      if (arg == "--") {
        std::copy(
          argv + i + 1, argv + argc, std::back_inserter(rest_arguments));
        break;
      }
      else if (arg_size > 1 && arg[0] == '-' && arg[1] != '-') {
        for (int j = 1; j < arg_size; j += 1) {
          options[std::string{ arg[j] }] = "true";
        }
      }
      else if (arg_size > 2 && arg[0] == '-' && arg[1] == '-') {
        auto eq = arg.find('=');
        if (eq != std::string::npos) {
          options[arg.substr(2, eq - 2)] = arg.substr(eq + 1);
        }
        else if (i + 1 < argc && argv[i + 1][0] != '-') {
          i += 1;
          options[arg.substr(2)] = std::string{ argv[i] };
        }
        else {
          options[arg.substr(2)] = "true";
        }
      }
      else {
        error("Args: Error parsing arguments!");
        exit(1);
      }

      i += 1;
    }
  }
};

} // namespace devkit

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest/doctest.h>

TEST_CASE("testing args")
{
  SUBCASE("-ab")
  {
    auto argv = std::array{ "test", "-ab" };
    auto args = devkit::Args(argv.size(), argv.data());
    CHECK(args.program == "test");
    CHECK(args.options["a"] == "true");
    CHECK(args.options["b"] == "true");
  }

  SUBCASE("sub -a --flag=1")
  {
    auto argv = std::array{ "test", "sub", "-a", "--flag=1" };
    auto args = devkit::Args(argv.size(), argv.data());
    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options["a"] == "true");
    CHECK(args.options["flag"] == "1");
  }

  SUBCASE("sub -a --path your_path --flag -- --build -- -j3")
  {
    auto argv = std::array{ "test",   "sub", "-a",      "--path", "your_path",
                            "--flag", "--",  "--build", "--",     "-j3" };
    auto args = devkit::Args(argv.size(), argv.data());
    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options["a"] == "true");
    CHECK(args.options["path"] == "your_path");
    CHECK(args.options["flag"] == "true");
    CHECK(args.rest_arguments[0] == "--build");
    CHECK(args.rest_arguments[1] == "--");
    CHECK(args.rest_arguments[2] == "-j3");
  }
}
#endif
