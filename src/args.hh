#pragma once

#include <array>
#include <deque>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>

#include "fmt.hh"

namespace devkit
{

namespace details
{

inline std::string
trim(const std::string& str)
{
  const auto is_space_or_newline = [](unsigned char ch) {
    return std::isspace(ch) || ch == '\n';
  };

  const auto start =
    std::find_if_not(str.begin(), str.end(), is_space_or_newline);
  const auto end =
    std::find_if_not(str.rbegin(), str.rend(), is_space_or_newline).base();

  if (start >= end) {
    return "";
  }

  return std::string{ start, end };
}

struct Option
{
  std::string short_name;
  std::string long_name;
  std::string value_type;
  std::string description;
  std::optional<std::string> value;
};

class Options
{
private:
  std::unordered_map<std::string, std::shared_ptr<Option>> options;

public:
  std::shared_ptr<Option>&
  operator[](const std::string& key)
  {
    if (options.find(key) == options.end() || options.at(key) == nullptr) {
      options[key] = std::make_shared<Option>();
    }
    return options.at(key);
  }

  bool
  exist(const std::string& key)
  {
    if (const auto it = options.find(key);
        it != options.end() && options.at(key) != nullptr) {
      return true;
    }
    return false;
  }

  void
  set_short(std::string&& key, std::string&& val)
  {
    if (exist(key)) {
      dk_err("Args: Option {} is set twice! \"{}\" is used.", key, val);
    }
    auto& option = this->operator[](key);
    option->short_name = std::move(key);
    option->value = std::move(val);
  }

  void
  set_long(std::string&& key, std::string&& val)
  {
    if (exist(key)) {
      dk_err("Args: Option {} is set twice! \"{}\" is used.", key, val);
    }
    auto& option = this->operator[](key);
    option->long_name = std::move(key);
    option->value = std::move(val);
  }

  std::unordered_map<std::string, std::string>
  to_map()
  {
    auto map = std::unordered_map<std::string, std::string>{};

    for (const auto& pair : options) {
      if (pair.first == "" || pair.second == nullptr ||
          !pair.second->value.has_value()) {
        continue;
      }

      const auto& option = pair.second;
      const auto& value = option->value.value();

      if (option->long_name != "") {
        map[option->long_name] = value;
      }

      if (option->short_name != "") {
        map[option->short_name] = value;
      }
    }

    return map;
  }
};

} // namespace details

class Args
{
public:
  std::string program;
  std::deque<std::string> subcommands;
  std::deque<std::string> rest_arguments;
  std::deque<std::string> extra_arguments;
  details::Options options;

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
          argv + i + 1, argv + argc, std::back_inserter(extra_arguments));
        break;
      }
      else if (arg_size > 1 && arg[0] == '-' && arg[1] != '-') {
        for (int j = 1; j < arg_size; j += 1) {
          options.set_short(std::string{ arg[j] }, std::string{ "true" });
        }
      }
      else if (arg_size > 2 && arg[0] == '-' && arg[1] == '-') {
        auto eq = arg.find('=');
        if (eq != std::string::npos) {
          options.set_long(arg.substr(2, eq - 2), arg.substr(eq + 1));
        }
        else if (i + 1 < argc && argv[i + 1][0] != '-') {
          i += 1;
          options.set_long(arg.substr(2), std::string{ argv[i] });
        }
        else {
          options.set_long(arg.substr(2), std::string{ "true" });
        }
      }
      else {
        int j = i + 1;
        while (j < argc) {
          auto arg = std::string{ argv[j] };
          if (arg == "--") {
            break;
          }
          if (arg[0] == '-') {
            dk_err("Args: Error parsing arguments!");
            exit(1);
          }
          j += 1;
        }
        std::copy(argv + i, argv + j, std::back_inserter(rest_arguments));
      }

      i += 1;
    }
  }

  void
  document(const std::string& doc)
  {
    parse_options(doc);
  }

private:
  void
  parse_options(const std::string& doc)
  {
    const std::regex re_delimiter{
      "(?:^|\\n)[ \\t]*" // a new line with leading whitespace
      "(?=-{1,2})" // [split happens here] (positive lookahead) ... and followed
                   // by one or two hyphes
    };

    for (auto s : parse_section(doc, "options:")) {
      if (const auto pos = s.find(':'); pos != std::string::npos) {
        s.erase(0, pos + 1);
      }

      std::vector<std::string> opts;
      std::for_each(
        std::sregex_token_iterator{ s.begin(), s.end(), re_delimiter, -1 },
        std::sregex_token_iterator{},
        [&](const std::string& match) {
          opts.push_back(match);
        });

      for (const auto& opt : opts) {
        if (opt.empty()) {
          continue;
        }

        if (opt[0] != '-') {
          dk_err("Args: Error parsing document \"{}\"!", opt);
          continue;
        }

        auto match = std::smatch{};
        auto regex = std::regex{
          "(?:(?:-(\\w)[, ]+\\s*)?--(\\w+)(?:\\s+<(\\w+)>)?)\\s+(.*)"
        };
        if (!std::regex_match(opt, match, regex)) {
          dk_err("Args: Error parsing document \"{}\"!", opt);
          continue;
        }

        auto&& short_name = match.str(1);
        auto&& long_name = match.str(2);
        auto&& value_type = match.str(3);
        auto&& description = match.str(4);
        std::shared_ptr<details::Option> option = nullptr;

        bool exist_long = options.exist(long_name);
        bool exist_short = options.exist(short_name);

        if (exist_long) {
          option = options[long_name];
        }
        else if (exist_short) {
          option = options[short_name];
        }

        if (option == nullptr) {
          option = std::make_shared<details::Option>();
        }

        if (exist_long && exist_short) {
          dk_err("Args: Option {},{} is set twice! \"{}\" is used.",
                 short_name,
                 long_name,
                 option->value.value());
        }

        option->long_name = long_name;
        option->short_name = short_name;
        option->value_type = value_type;
        option->description = description;

        options[long_name] = option;
        if (short_name != "") {
          options[short_name] = option;
        }
      }
    }
  }

  std::vector<std::string>
  parse_section(const std::string& doc, const std::string& name)
  {
    const std::regex re_section_pattern{
      "(?:^|\\n)" // anchored at a linebreak (or start of string)
      "("
      "[^\\n]*" +
        name +
        "[^\\n]*(?=\\n?)"            // a line that contains the usage
        "(?:\\n[ \\t].*?(?=\\n|$))*" // followed by any number of lines that are
                                     // indented
        ")",
      std::regex::icase
    };

    std::vector<std::string> ret;
    std::for_each(
      std::sregex_iterator(doc.begin(), doc.end(), re_section_pattern),
      std::sregex_iterator(),
      [&](const std::smatch& match) {
        ret.push_back(details::trim(match[1].str()));
      });
    return ret;
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
    CHECK(args.options.to_map()["a"] == "true");
    CHECK(args.options.to_map()["b"] == "true");
  }

  SUBCASE("sub -a --flag=1")
  {
    auto argv = std::array{ "test", "sub", "-a", "--flag=1" };
    auto args = devkit::Args(argv.size(), argv.data());
    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options.to_map()["a"] == "true");
    CHECK(args.options.to_map()["flag"] == "1");
  }

  SUBCASE("sub -a --path your_path --flag -- --build -- -j3")
  {
    auto argv = std::array{ "test",   "sub", "-a",      "--path", "your_path",
                            "--flag", "--",  "--build", "--",     "-j3" };

    auto args = devkit::Args(argv.size(), argv.data());

    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options.to_map()["a"] == "true");
    CHECK(args.options.to_map()["path"] == "your_path");
    CHECK(args.options.to_map()["flag"] == "true");
    CHECK(args.extra_arguments[0] == "--build");
    CHECK(args.extra_arguments[1] == "--");
    CHECK(args.extra_arguments[2] == "-j3");
  }

  SUBCASE("sub -a --path your_path rest1 rest2")
  {
    auto argv = std::array{ "test",      "sub",   "-a",   "--path",
                            "your_path", "rest1", "rest2" };

    auto args = devkit::Args(argv.size(), argv.data());

    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options.to_map()["a"] == "true");
    CHECK(args.options.to_map()["path"] == "your_path");
    CHECK(args.rest_arguments[0] == "rest1");
    CHECK(args.rest_arguments[1] == "rest2");
  }

  SUBCASE("sub -a --path your_path rest1 rest2 -- --build -- -j3")
  {
    auto argv =
      std::array{ "test",  "sub", "-a",      "--path", "your_path", "rest1",
                  "rest2", "--",  "--build", "--",     "-j3" };

    auto args = devkit::Args(argv.size(), argv.data());

    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options.to_map()["a"] == "true");
    CHECK(args.options.to_map()["path"] == "your_path");
    CHECK(args.rest_arguments[0] == "rest1");
    CHECK(args.rest_arguments[1] == "rest2");
    CHECK(args.extra_arguments[0] == "--build");
    CHECK(args.extra_arguments[1] == "--");
    CHECK(args.extra_arguments[2] == "-j3");
  }
}

TEST_CASE("testing document")
{
  const auto doc = R"(
      Usage: test args [-abc] [--path --store]

      Options:
        -a, --A         This is a
        -b, --B         This is b
        -c, --C <file>  This is c
        --path  <dir>   This is path
        --store <dir>   This is store
      )";

  SUBCASE("sub -a --path your_path rest1 rest2 -- --build -- -j3")
  {
    auto argv =
      std::array{ "test",  "sub", "-a",      "--path", "your_path", "rest1",
                  "rest2", "--",  "--build", "--",     "-j3" };

    auto args = devkit::Args(argv.size(), argv.data());
    args.document(doc);

    auto map = args.options.to_map();

    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(map["a"] == "true");
    CHECK(map["A"] == "true");
    CHECK(map["path"] == "your_path");
    CHECK(map.find("b") == map.end());
    CHECK(map.find("B") == map.end());
    CHECK(map.find("store") == map.end());
    CHECK(args.options.exist("b"));
    CHECK(args.options.exist("B"));
    CHECK(!args.options.exist("D"));
    CHECK(args.options["B"] == args.options["b"]);
    CHECK(args.options["B"]->value_type == "");
    CHECK(args.options["B"]->description == "This is b");
    CHECK(args.options["C"] == args.options["c"]);
    CHECK(args.options["C"]->value_type == "file");
    CHECK(args.options["C"]->description == "This is c");
    CHECK(args.options["store"]->value_type == "dir");
    CHECK(args.options["store"]->description == "This is store");
    CHECK(args.rest_arguments[0] == "rest1");
    CHECK(args.rest_arguments[1] == "rest2");
    CHECK(args.extra_arguments[0] == "--build");
    CHECK(args.extra_arguments[1] == "--");
    CHECK(args.extra_arguments[2] == "-j3");
  }
}
#endif
