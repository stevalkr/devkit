#pragma once

#include <array>
#include <deque>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>

#include "fmt.hh"

namespace devkit
{

namespace details
{

struct Option
{
  std::string short_name;
  std::string long_name;
  std::string value_type;
  std::string description;
  std::optional<std::string> value;

  bool
  to_bool()
  {
    if (value.has_value()) {
      return value.value() == "true";
    }
    return false;
  }

  std::string
  to_string()
  {
    if (value.has_value()) {
      return value.value();
    }
    return "";
  }
};

class Options
{
private:
  std::unordered_map<std::string, std::shared_ptr<Option>> options_map;

  bool
  exist(const std::string& key)
  {
    if (options_map.find(key) != options_map.end() &&
        options_map.at(key) != nullptr) {
      return true;
    }
    return false;
  }

public:
  std::shared_ptr<Option>&
  operator[](const std::string& key)
  {
    if (!exist(key)) {
      options_map[key] = std::make_shared<Option>();
    }
    return options_map.at(key);
  }

  void
  set_short(std::string&& key, std::string&& val)
  {
    std::shared_ptr<Option> option = nullptr;
    if (exist(key)) {
      dk_err("Args: Option {} is set twice! \"{}\" is used.", key, val);
      option = options_map[key];
    }
    else {
      option = std::make_shared<Option>();
      options_map[key] = option;
    }
    option->short_name = std::move(key);
    option->value = std::move(val);
  }

  void
  set_long(std::string&& key, std::string&& val)
  {
    std::shared_ptr<Option> option = nullptr;
    if (exist(key)) {
      dk_err("Args: Option {} is set twice! \"{}\" is used.", key, val);
      option = options_map[key];
    }
    else {
      option = std::make_shared<Option>();
      options_map[key] = option;
    }
    option->long_name = std::move(key);
    option->value = std::move(val);
  }

  std::shared_ptr<Option>
  add_document(const std::string& short_name,
               const std::string& long_name,
               const std::string& value_type,
               const std::string& description)
  {
    std::shared_ptr<Option> option = nullptr;

    bool exist_long = exist(long_name);
    bool exist_short = exist(short_name);

    if (exist_long) {
      option = options_map[long_name];
    }
    else if (exist_short) {
      option = options_map[short_name];
    }
    else {
      option = std::make_shared<Option>();
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

    options_map[long_name] = option;
    if (!short_name.empty()) {
      options_map[short_name] = option;
    }

    return option;
  }

  std::unordered_map<std::string, std::string>
  to_map()
  {
    auto map = std::unordered_map<std::string, std::string>{};

    for (const auto& pair : options_map) {
      if (pair.first.empty() || pair.second == nullptr ||
          !pair.second->value.has_value()) {
        continue;
      }

      const auto& option = pair.second;
      const auto& value = option->value.value();

      if (!option->long_name.empty()) {
        map[option->long_name] = value;
      }

      if (!option->short_name.empty()) {
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

private:
  std::set<std::pair<std::string, std::string>> commands_doc;
  std::set<std::shared_ptr<details::Option>> options_doc;

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
      const auto arg = std::string{ argv[i] };
      const auto arg_size = arg.size();

      if (arg == "--") {
        std::copy(
          argv + i + 1, argv + argc, std::back_inserter(extra_arguments));
        break;
      }
      else if (arg_size > 1 && arg[0] == '-' && arg[1] != '-') {
        for (int j = 1; j < arg_size; j += 1) {
          if (arg[j] == '=') {
            options.set_short(std::string{ arg[j - 1] }, arg.substr(j + 1));
            break;
          }
          else if (j == arg_size - 1 && i + 1 < argc && argv[i + 1][0] != '-') {
            i += 1;
            options.set_short(std::string{ arg[j] }, std::string{ argv[i] });
            break;
          }
          else {
            options.set_short(std::string{ arg[j] }, std::string{ "true" });
          }
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
    parse_commands(doc);
    parse_options(doc);
  }

  std::deque<std::pair<std::string, std::string>>
  complete(std::string prefix)
  {
    auto completions = std::deque<std::pair<std::string, std::string>>{};

    bool check_long = false;
    bool check_short = false;
    bool check_command = false;

    if (prefix.empty()) {
      check_long = true;
      check_short = true;
      check_command = true;
    }
    else if (prefix == "-") {
      check_long = true;
      check_short = true;
    }
    else if (prefix[0] == '-') {
      if (prefix[1] == '-') {
        check_long = true;
      }
      else {
        check_short = true;
      }
    }
    else {
      check_command = true;
    }

    const auto pos = prefix.find_first_not_of('-');
    if (pos != std::string::npos) {
      prefix = prefix.substr(pos);
    }
    else {
      prefix.clear();
    }

    for (const auto& option : options_doc) {
      if (check_long && !option->long_name.empty() &&
          option->long_name.starts_with(prefix)) {
        completions.emplace_back("--" + option->long_name, option->description);
      }
      if (check_short && !option->short_name.empty()) {
        completions.emplace_back("-" + option->short_name, option->description);
      }
    }

    for (const auto& [cmd, desc] : commands_doc) {
      if (check_command && cmd.starts_with(prefix)) {
        completions.emplace_back(cmd, desc);
      }
    }

    return completions;
  }

private:
  void
  parse_commands(const std::string& doc)
  {
    const std::regex re_delimiter{ "(?:^|\\n)\\s*" };

    for (auto s : parse_section(doc, "commands:")) {
      if (const auto pos = s.find(':'); pos != std::string::npos) {
        s.erase(0, pos + 1);
      }

      std::vector<std::string> cmds;
      std::for_each(
        std::sregex_token_iterator{ s.begin(), s.end(), re_delimiter, -1 },
        std::sregex_token_iterator{},
        [&](const std::string& match) {
          cmds.push_back(match);
        });

      for (auto& cmd : cmds) {
        cmd = std::regex_replace(cmd, std::regex("^\\s+|\\s+$"), "");

        if (cmd.empty()) {
          continue;
        }

        auto match = std::smatch{};
        auto regex = std::regex{ "(\\S+)(?:\\s+)(.*)" };
        if (!std::regex_match(cmd, match, regex)) {
          dk_err("Args: Error parsing commands \"{}\"!", cmd);
          continue;
        }

        commands_doc.insert({ match.str(1), match.str(2) });
      }
    }
  }

  void
  parse_options(const std::string& doc)
  {
    const std::regex re_delimiter{
      "(?:^|\\n)\\s*" // a new line with leading whitespace
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

      for (auto& opt : opts) {
        opt = std::regex_replace(opt, std::regex("^\\s+|\\s+$"), "");
        opt = std::regex_replace(opt, std::regex("\\s+"), " ");

        if (opt.empty()) {
          continue;
        }

        if (opt[0] != '-') {
          dk_err("Args: Error parsing options \"{}\"!", opt);
          continue;
        }

        auto match = std::smatch{};
        auto regex = std::regex{
          "(?:(?:-(\\w)[, ]+\\s*)?--(\\w+)(?:\\s+<(\\w+)>)?)\\s+(.*)"
        };
        if (!std::regex_match(opt, match, regex)) {
          dk_err("Args: Error parsing options \"{}\"!", opt);
          continue;
        }

        auto option = options.add_document(
          match.str(1), match.str(2), match.str(3), match.str(4));
        options_doc.insert(std::move(option));
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
      std::sregex_iterator{ doc.begin(), doc.end(), re_section_pattern },
      std::sregex_iterator{},
      [&](const std::smatch& match) {
        ret.push_back(match[1].str());
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

  SUBCASE("sub -a -bc=2 --flag=1")
  {
    auto argv = std::array{ "test", "sub", "-a", "-bc=2", "--flag=1" };
    auto args = devkit::Args(argv.size(), argv.data());
    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options.to_map()["a"] == "true");
    CHECK(args.options.to_map()["b"] == "true");
    CHECK(args.options.to_map()["c"] == "2");
    CHECK(args.options.to_map()["flag"] == "1");
  }

  SUBCASE("sub -a -bc 2 --path your_path --flag -- --build -- -j3")
  {
    auto argv =
      std::array{ "test",      "sub",    "-a", "-bc",     "2",  "--path",
                  "your_path", "--flag", "--", "--build", "--", "-j3" };

    auto args = devkit::Args(argv.size(), argv.data());

    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(args.options.to_map()["a"] == "true");
    CHECK(args.options.to_map()["b"] == "true");
    CHECK(args.options.to_map()["c"] == "2");
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

      Commands:
        cmd1            This is cmd1
        cmd2            This is cmd2

      Options:
        -a, --A         This is a
        -b, --B         This is b
        -c, --C <file>  This is c
                        with new line

      More Options:
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
    CHECK(args.options["B"] == args.options["b"]);
    CHECK(args.options["B"]->value_type == "");
    CHECK(args.options["B"]->description == "This is b");
    CHECK(args.options["C"] == args.options["c"]);
    CHECK(args.options["C"]->value_type == "file");
    CHECK(args.options["C"]->description == "This is c with new line");
    CHECK(args.options["store"]->value_type == "dir");
    CHECK(args.options["store"]->description == "This is store");
    CHECK(args.rest_arguments[0] == "rest1");
    CHECK(args.rest_arguments[1] == "rest2");
    CHECK(args.extra_arguments[0] == "--build");
    CHECK(args.extra_arguments[1] == "--");
    CHECK(args.extra_arguments[2] == "-j3");
  }

  SUBCASE("sub -a -A")
  {
    auto argv = std::array{ "test", "sub", "-a", "-A" };

    auto args = devkit::Args(argv.size(), argv.data());
    args.document(doc);

    auto map = args.options.to_map();

    CHECK(args.program == "test");
    CHECK(args.subcommands[0] == "sub");
    CHECK(map["a"] == "true");
    CHECK(map["A"] == "true");
  }
}
#endif
