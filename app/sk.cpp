#include <filesystem>

#include "args.hh"
#include "lua.hh"
#include "task.hh"

namespace dk = devkit;
namespace fs = std::filesystem;

extern "C" int
lua_list_dir(lua_State* L)
{
  if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
    lua_pushstring(L, "Invalid argument. Expected a directory path string.");
    lua_error(L);
    return 0;
  }

  std::string path = lua_tostring(L, 1);
  lua_newtable(L);

  lua_pushstring(L, "dirs");
  lua_newtable(L);
  int dir_index = 1;

  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_directory()) {
      lua_pushstring(L, entry.path().filename().string().c_str());
      lua_rawseti(L, -2, dir_index++); // dirs[subIndex] = "dirname"
    }
  }
  lua_settable(L, -3); // result["dirs"] = dirs

  lua_pushstring(L, "files");
  lua_newtable(L);
  int file_index = 1;

  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_regular_file()) {
      lua_pushstring(L, entry.path().filename().string().c_str());
      lua_rawseti(L, -2, file_index++); // files[fileIndex] = "filename"
    }
  }
  lua_settable(L, -3); // result["files"] = files

  return 1;
}

extern "C" int
lua_exists(lua_State* L)
{
  if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
    lua_pushstring(
      L, "Invalid argument. Expected a file or directory path string.");
    lua_error(L);
    return 0;
  }

  auto path = std::string{ lua_tostring(L, 1) };
  bool exists = std::filesystem::exists(path);

  lua_pushboolean(L, exists);

  return 1;
}

extern "C" int
lua_join(lua_State* L)
{
  int nargs = lua_gettop(L);
  auto joined_path = std::filesystem::path{};

  for (int i = 1; i <= nargs; ++i) {
    if (!lua_isstring(L, i)) {
      lua_pushstring(L, "Invalid argument. All arguments must be strings.");
      lua_error(L);
      return 0;
    }

    auto part = std::string{ lua_tostring(L, i) };

    if (!part.empty() && part[0] == '~') {
      const char* home = std::getenv("HOME");
      if (home != nullptr) {
        part = dk::fmt("{}{}", home, part.substr(1));
      }
    }

    if (joined_path.empty()) {
      joined_path = part;
    }
    else {
      joined_path = joined_path / part;
    }
  }

  lua_pushstring(L, joined_path.string().c_str());

  return 1;
}

int
main(int argc, char** argv)
{
  const auto home = fs::path{ std::getenv("HOME") };
  const auto args = dk::Args{ argc, const_cast<const char**>(argv) };

  if (args.subcommands.empty()) {
    dk::error("No subcommand specified.");
    exit(1);
  }

  auto store = home / ".devkit";
  if (args.options.find("store") != args.options.end()) {
    store = args.options.at("store");
  }

  if (!fs::is_directory(store)) {
    dk::error("Store path invalid.");
    exit(1);
  }

  const auto file = store / "sk.lua";
  if (!fs::is_regular_file(file)) {
    dk::error("sk.lua not found.");
    exit(1);
  }

  constexpr auto fs_func = std::array{ luaL_Reg{ "ls_dir", lua_list_dir },
                                       luaL_Reg{ "exists", lua_exists },
                                       luaL_Reg{ "join", lua_join },
                                       luaL_Reg{ nullptr, nullptr } };
  auto lua = dk::Lua{ file };
  lua.register_module("fs", fs_func);

  const auto result = lua.call_module<dk::Lua::map>(args.subcommands.front(),
                                                    fs::current_path().string(),
                                                    args.subcommands,
                                                    args.options,
                                                    args.rest_arguments);
  if (!result.has_value()) {
    dk::error("Subcommand {} not found.", args.subcommands.front());
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
