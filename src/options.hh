#pragma once

#include <cxxopts.hpp>
#include <fmt/core.h>
#include <string>
#include <vector>

namespace devkit
{

namespace details
{

struct OptionsArg
{
  std::string opt;
  std::optional<std::string> short_opt;

  std::string desp;
  std::optional<std::string> default_value;
  std::optional<std::string> implicit_value;

  std::string
  opt_str() const
  {
    if (short_opt.has_value()) {
      return fmt::format("{},{}", short_opt.value(), opt);
    }
    return opt;
  }
};

} // namespace details

class Options
{
private:
  cxxopts::Options options;
  const std::string version;

public:
  Options(const std::string& name, const std::string& help)
    : options{ name, help }
    , version{ "0.0.1" }
  {
    // clang-format off
    options
      .set_width(70)
      .set_tab_expansion()
      .add_options()
        ("h,help", "Show help message")
        ("v,version", "Show version");
    // clang-format on
  }

  template<typename T>
  Options&
  operator()(T& v, const details::OptionsArg& arg)
  {
    const auto value = cxxopts::value<T>(v);
    if (arg.default_value.has_value()) {
      value->default_value(arg.default_value.value());
    }
    if (arg.implicit_value.has_value()) {
      value->implicit_value(arg.implicit_value.value());
    }
    options.add_options()(arg.opt_str(), arg.desp, value);

    return *this;
  }

  cxxopts::ParseResult
  parse(const int argc, const char* const* argv)
  {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      print_help();
      exit(0);
    }

    if (result.count("version")) {
      print_version();
      exit(0);
    }

    return result;
  }

  void
  print_help()
  {
    fmt::println("{}", options.help());
  }

  void
  print_version()
  {
    fmt::println("Version: {}", version);
  }
};

} // namespace devkit

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest/doctest.h>

TEST_CASE("testing options")
{
  // NOLINTBEGIN
  bool a{};
  bool b{};
  auto path = std::string{};

  // clang-format off
  auto options = devkit::Options{ "test", "help message" }
    ( a, {
      .opt = "aaa", .short_opt = "a",
      .desp = "Desp a"
      })
    ( b, {
      .opt = "bbb", .short_opt = "b",
      .desp = "Desp b"
      })
    ( path, {
      .opt = "path",
      .desp = "Desp path",
      .default_value = "default/path"
      });
  // clang-format on
  // NOLINTEND

  SUBCASE("-ab")
  {
    auto result = options.parse(2, std::array{ "test", "-ab" }.data());
    CHECK(a == true);
    CHECK(b == true);
    CHECK(path == "default/path");
  }

  SUBCASE("-a --path your_path")
  {
    auto result = options.parse(
      4, std::array{ "test", "-a", "--path", "your_path" }.data());
    CHECK(a == true);
    CHECK(b == false);
    CHECK(path == "your_path");
  }
}
#endif
