#include <cstdlib>
#include <filesystem>
#include <regex>

#include "args.hh"
#include "fmt.hh"
#include "lua.hh"
#include "task.hh"

namespace dk = devkit;
namespace fs = std::filesystem;

extern "C" int
lua_set_env(lua_State* L)
{
  const auto name = std::string{ luaL_checkstring(L, 1) };
  const auto value = std::string{ luaL_checkstring(L, 2) };
  int overwrite = lua_isnone(L, 3) ? 1 : lua_toboolean(L, 3);

  if (setenv(name.data(), value.data(), overwrite) != 0) {
    lua_pushnil(L);
    lua_pushstring(
      L, dk::fmt("Failed to set environment variable '{}'", name).data());
    return 2;
  }

  lua_pushboolean(L, 1);
  return 1;
}

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

extern "C" int
lua_split_path(lua_State* L)
{
  if (lua_gettop(L) != 1 || !lua_isstring(L, 1)) {
    lua_pushstring(L, "Invalid argument. Expected a path string.");
    lua_error(L);
    return 0;
  }

  auto path = std::string{ lua_tostring(L, 1) };

  auto matches = std::smatch{};
  auto pattern = std::regex{ R"((.*[\\/])?([^\\/]*?)(\.[^\\.]*?)?$)" };

  if (std::regex_match(path, matches, pattern)) {
    lua_newtable(L);

    lua_pushstring(L, "dir");
    lua_pushstring(L, matches[1].str().c_str());
    lua_settable(L, -3);

    lua_pushstring(L, "name");
    lua_pushstring(L, matches[2].str().c_str());
    lua_settable(L, -3);

    lua_pushstring(L, "ext");
    lua_pushstring(L, matches[3].str().c_str());
    lua_settable(L, -3);
  }
  else {
    lua_newtable(L);
  }

  return 1;
}

int
main(int argc, char** argv)
{
  const auto home = fs::path{ std::getenv("HOME") };
  auto args = dk::Args{ argc, const_cast<const char**>(argv) };
  const auto options = args.options.to_map();

  if (args.subcommands.empty()) {
    dk_err("No subcommand specified.");
    exit(1);
  }

  auto command = args.subcommands.front();
  args.subcommands.pop_front();

  auto store = home / ".devkit";
  if (options.find("store") != options.end()) {
    store = options.at("store");
  }

  if (!fs::is_directory(store)) {
    dk_err("Store path invalid.");
    exit(1);
  }

  const auto apps = store / "apps";
  if (!fs::is_regular_file(apps / "sk.lua")) {
    dk_err("apps/sk.lua not found.");
    exit(1);
  }

  constexpr auto fs_func = std::array{ luaL_Reg{ "ls_dir", lua_list_dir },
                                       luaL_Reg{ "exists", lua_exists },
                                       luaL_Reg{ "join", lua_join },
                                       luaL_Reg{ "split_path", lua_split_path },
                                       luaL_Reg{ nullptr, nullptr } };
  constexpr auto sh_func = std::array{ luaL_Reg{ "set_env", lua_set_env },
                                       luaL_Reg{ nullptr, nullptr } };

  auto lua = dk::Lua{ apps / "sk.lua" };
  lua.register_module("fs", fs_func);
  lua.register_module("sh", sh_func);

  if (command == "help") {
    auto cmd = std::string{};
    if (!args.subcommands.empty()) {
      cmd = args.subcommands.front();
    }

    const auto help_msg = lua.call_module<std::string>(command, cmd);
    if (!help_msg.has_value()) {
      dk_err("Help message not found.");
      exit(1);
    }
    dk_log("{}", help_msg.value());
    return 0;
  }

  const auto result = lua.call_module<dk::Lua::map>(command,
                                                    fs::current_path().string(),
                                                    args.subcommands,
                                                    options,
                                                    args.rest_arguments,
                                                    args.extra_arguments);
  if (!result.has_value()) {
    dk_err("Subcommand {} not found.", command);
    exit(1);
  }

  auto task_args = result.value();

  const auto to_bool = [](const std::string& str) -> bool {
    if (str == "true") {
      return true;
    }
    return false;
  };
  const auto set_default = [](dk::Lua::map& map,
                              const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
      if (map.find(key) == map.end()) {
        map[key] = "";
      }
    }
  };

  set_default(task_args, { "new_process", "search_path", "command" });

  const auto task =
    dk::Task{ { .new_process = to_bool(task_args.at("new_process")),
                .search_path = to_bool(task_args.at("search_path")),
                .command = task_args.at("command") } };
  return task.run();
}
