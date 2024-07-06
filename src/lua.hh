#pragma once

#include <cassert>
#include <filesystem>
#include <lua.hpp>
#include <map>
#include <optional>
#include <string>
#include <type_traits>

#include "fmt.hh"

namespace devkit
{

namespace details
{

template<typename T>
struct is_map : std::false_type
{};

template<typename Key, typename Val>
struct is_map<std::map<Key, Val>> : std::true_type
{};

template<typename Key, typename Val>
struct is_map<std::unordered_map<Key, Val>> : std::true_type
{};

template<typename T>
constexpr bool is_map_v = is_map<T>::value;

template<typename T, typename = void, typename = void>
struct is_iterable : std::false_type
{};

template<typename T>
struct is_iterable<
  T,
  typename std::void_t<decltype(std::begin(std::declval<T>())),
                       decltype(std::end(std::declval<T>()))>,
  typename std::enable_if_t<!is_map_v<T> && !std::is_same_v<T, std::string>>>
  : std::true_type
{};

template<typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

} // namespace details

class Lua
{
public:
  using map = std::map<std::string, std::string>;

private:
  lua_State* L;

  bool
  checkLua(int r)
  {
    if (r != LUA_OK) {
      const auto errorMsg = std::string{ lua_tostring(L, -1) };
      lua_pop(L, 1);

      dk_err("Lua: error {}", errorMsg);
      return false;
    }
    return true;
  }

  template<typename Ret>
  std::enable_if_t<details::is_map_v<Ret>, Ret>
  get_return_value()
  {
    using Key = typename Ret::key_type;
    using Val = typename Ret::mapped_type;

    Ret map;
    lua_pushnil(L);

    while (lua_next(L, -2) != 0) {
      if constexpr (std::is_integral_v<Key>) {
        const auto key = luaL_checkinteger(L, -2);
        map[key] = get_return_value<Val>();
        lua_pop(L, 1);
      }
      else if constexpr (std::is_same_v<Key, std::string>) {
        const auto key = std::string{ luaL_checkstring(L, -2) };
        map[key] = get_return_value<Val>();
        lua_pop(L, 1);
      }
      else {
        dk_err("Lua: Wrong return type.");
        exit(1);
      }
    }

    return map;
  }

  template<typename Ret>
  std::enable_if_t<details::is_iterable_v<Ret>, Ret>
  get_return_value()
  {
    using Val = Ret::value_type;

    Ret vec;
    lua_pushnil(L);

    while (lua_next(L, -2) != 0) {
      vec.push_back(get_return_value<Val>());
      lua_pop(L, 1);
    }

    return vec;
  }

  template<typename Ret>
  std::enable_if_t<!details::is_map_v<Ret> && !details::is_iterable_v<Ret>, Ret>
  get_return_value()
  {
    if constexpr (std::is_same_v<Ret, bool>) {
      return static_cast<bool>(lua_toboolean(L, -1));
    }
    else if constexpr (std::is_integral_v<Ret>) {
      return luaL_checkinteger(L, -1);
    }
    else if constexpr (std::is_same_v<Ret, double> ||
                       std::is_same_v<Ret, float>) {
      return luaL_checknumber(L, -1);
    }
    else if constexpr (std::is_same_v<Ret, std::string>) {
      return std::string{ luaL_checkstring(L, -1) };
    }
    else {
      dk_err("Lua: Return type not supported!");
      exit(1);
    }
  }

  template<typename Arg>
  void
  push_arg(Arg arg)
  {
    if constexpr (std::is_same_v<Arg, bool>) {
      lua_pushboolean(L, arg);
    }
    else if constexpr (std::is_same_v<Arg, int>) {
      lua_pushinteger(L, arg);
    }
    else if constexpr (std::is_same_v<Arg, double> ||
                       std::is_same_v<Arg, float>) {
      lua_pushnumber(L, arg);
    }
    else if constexpr (std::is_same_v<Arg, std::string>) {
      lua_pushstring(L, arg.c_str());
    }
    else if constexpr (details::is_map_v<Arg>) {
      lua_newtable(L);
      for (const auto& [key, value] : arg) {
        lua_pushstring(L, key.c_str());
        push_arg(value);
        lua_settable(L, -3);
      }
    }
    else if constexpr (details::is_iterable_v<Arg>) {
      lua_newtable(L);
      int index = 1;
      for (const auto& elem : arg) {
        lua_pushinteger(L, index++);
        push_arg(elem);
        lua_settable(L, -3);
      }
    }
    else {
      dk_err("Lua: Wrong argument type!");
      exit(1);
    }
  }

  template<typename Ret, typename... Args>
  std::optional<Ret>
  call(Args... args)
  {
    (push_arg(args), ...);

    if (lua_pcall(L, sizeof...(Args), 1, 0) != LUA_OK) {
      const auto errorMsg = std::string{ lua_tostring(L, -1) };
      lua_pop(L, 1);

      dk_err("Lua: error {}", errorMsg);
      return std::nullopt;
    }

    auto result = get_return_value<Ret>();
    lua_pop(L, 1);

    return result;
  }

public:
  Lua()
  {
    L = luaL_newstate();
    luaL_openlibs(L);
  }

  explicit Lua(const char* script)
  {
    L = luaL_newstate();
    luaL_openlibs(L);

    exec(script);
  }

  explicit Lua(const std::string& script)
  {
    L = luaL_newstate();
    luaL_openlibs(L);

    exec(script);
  }

  explicit Lua(const std::filesystem::path& filepath)
  {
    L = luaL_newstate();
    luaL_openlibs(L);

    exec_file(filepath);
  }

  ~Lua()
  {
    if (L) {
      lua_close(L);
    }
  }

  Lua(const Lua&) = delete;
  Lua&
  operator=(const Lua&) = delete;

  Lua(Lua&& other)
    : L(other.L)
  {
    other.L = nullptr;
  }

  Lua&
  operator=(Lua&& other)
  {
    if (this != &other) {
      lua_close(L);
      L = other.L;
      other.L = nullptr;
    }
    return *this;
  }

  void
  exec(const std::string& script)
  {
    if (!checkLua(luaL_dostring(L, script.c_str()))) {
      lua_close(L);
    }
  }

  void
  exec_file(const std::filesystem::path& filepath)
  {
    if (!checkLua(luaL_dostring(
          L,
          ("package.path = package.path .. ';" +
           (filepath.parent_path() / "?.lua").string() + ";" +
           (filepath.parent_path() / "?/init.lua").string() + ";'")
            .c_str()))) {
      lua_close(L);
    }

    if (!checkLua(luaL_dofile(L, filepath.c_str()))) {
      lua_close(L);
    }
  }

  template<typename Arg>
  void
  register_variable(const std::string& name, Arg arg)
  {
    push_arg(arg);
    lua_setglobal(L, name.c_str());
  }

  void
  register_function(const std::string& name, lua_CFunction function)
  {
    lua_register(L, name.c_str(), function);
  }

  template<size_t Size>
  void
  register_module(const std::string& name,
                  const std::array<luaL_Reg, Size>& reg)
  {
    static luaL_Reg arr[Size];
    std::copy(reg.begin(), reg.end(), arr);

    luaL_requiref(
      L,
      name.c_str(),
      [](lua_State* L) -> int {
        luaL_newlib(L, arr);
        return 1;
      },
      1);
    lua_pop(L, 1);
  }

  template<typename Ret, typename... Args>
  std::optional<Ret>
  call_module(const std::string& name, Args... args)
  {
    lua_getfield(L, -1, name.c_str());

    if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);

      dk_err("Lua: No member function: {}", name);
      return std::nullopt;
    }

    return call<Ret>(args...);
  }

  template<typename Ret, typename... Args>
  std::optional<Ret>
  call_global(const std::string& name, Args... args)
  {
    lua_getglobal(L, name.c_str());

    if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);

      dk_err("Lua: No global function: {}", name);
      return std::nullopt;
    }

    return call<Ret>(args...);
  }
};

} // namespace devkit

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest/doctest.h>

TEST_CASE("testing lua")
{
  SUBCASE("square")
  {
    auto lua = devkit::Lua{ "function square(n) return n * n end" };
    auto result = lua.call_global<int>("square", 5);

    CHECK(result.has_value());
    CHECK(result.value() == 25);
  }

  SUBCASE("add")
  {
    auto lua = devkit::Lua{ "function add(a, b) return a + b end" };
    auto result = lua.call_global<double>("add", 12.1, 13);

    CHECK(result.has_value());
    CHECK(result.value() == 25.1);
  }

  SUBCASE("no function")
  {
    auto lua = devkit::Lua{ "function add(a, b) return a + b end" };
    auto result = lua.call_global<double>("no_func", 5);

    CHECK(!result.has_value());
  }

  SUBCASE("map")
  {
    auto lua = devkit::Lua{ R"(
      local M = {}
      M.test_map = function (map)
        map['one'] = tonumber(map['one']) * 2
        map['two'] = tonumber(map['two']) * 2
        return map
      end
      return M
      )" };

    auto map = std::map<std::string, int>{ { "one", 1 }, { "two", 2 } };
    auto result = lua.call_module<devkit::Lua::map>("test_map", map);

    CHECK(result.has_value());
    CHECK(result.value()["one"] == "2");
    CHECK(result.value()["two"] == "4");
  }

  SUBCASE("complex map")
  {
    auto lua = devkit::Lua{ R"(
      local M = {}
      M.test_complex_map = function ()
        map = {}
        map['num'] = { '1', '2' }
        map['str'] = { 'one', 'two' }
        return map
      end
      return M
      )" };

    auto result =
      lua.call_module<std::map<std::string, std::map<int, std::string>>>(
        "test_complex_map");

    CHECK(result.has_value());
    CHECK(result.value()["num"][1] == "1");
    CHECK(result.value()["num"][2] == "2");
    CHECK(result.value()["str"][1] == "one");
    CHECK(result.value()["str"][2] == "two");
  }

  SUBCASE("complex map array")
  {
    auto lua = devkit::Lua{ R"(
      local M = {}
      M.test_complex_map = function ()
        map = {}
        map['one'] = { 1, 2 }
        map['two'] = { 3, 4 }
        return map
      end
      return M
      )" };

    auto result = lua.call_module<std::map<std::string, std::vector<uint8_t>>>(
      "test_complex_map");

    CHECK(result.has_value());
    CHECK(result.value()["one"][0] == 1);
    CHECK(result.value()["one"][1] == 2);
    CHECK(result.value()["two"][0] == 3);
    CHECK(result.value()["two"][1] == 4);
  }
}

#endif
