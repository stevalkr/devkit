#include "args.hh"
#include "lua.hh"
#include "task.hh"
#include <filesystem>

namespace dk = devkit;
namespace fs = std::filesystem;

int
main(int argc, char** argv)
{
  const auto home = fs::path{ std::getenv("HOME") };
  const auto args = dk::Args{ argc, const_cast<const char**>(argv) };

  if (args.subcommands.empty()) {
    fmt::println(stderr, "No subcommand specified.");
    exit(1);
  }

  auto store = home / ".devkit";
  if (args.options.find("store") != args.options.end()) {
    store = args.options.at("store");
  }

  if (!fs::is_directory(store)) {
    fmt::println(stderr, "Store path invalid.");
    exit(1);
  }

  const auto file = store / "sk.lua";
  if (!fs::is_regular_file(file)) {
    fmt::println(stderr, "sk.lua not found.");
    exit(1);
  }

  auto lua = dk::Lua{ file };

  const auto result = lua.call_module<dk::Lua::map>(args.subcommands.front(),
                                                    fs::current_path().string(),
                                                    args.subcommands,
                                                    args.options,
                                                    args.rest_arguments);
  if (!result.has_value()) {
    fmt::println(stderr, "Subcommand {} not found.", args.subcommands.front());
    exit(1);
  }

  const auto options = result.value();
  const auto to_bool = [](const std::string& str) -> bool {
    if (str == "true") {
      return true;
    }
    return false;
  };

  const auto task =
    dk::Task{ { .new_process = to_bool(options.at("new_process")),
                .search_path = to_bool(options.at("search_path")),
                .command = options.at("command") } };
  return task.run();
}
