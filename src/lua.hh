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

template<class T>
struct is_map : std::false_type
{};

template<class Key, class Value>
struct is_map<std::map<Key, Value>> : std::true_type
{};

template<class Key, class Value>
struct is_map<std::unordered_map<Key, Value>> : std::true_type
{};

template<class T>
constexpr bool is_map_v = is_map<T>::value;

template<typename T, typename = void>
struct is_iterable : std::false_type
{};

template<typename T>
struct is_iterable<T,
                   std::void_t<decltype(std::begin(std::declval<T>())),
                               decltype(std::end(std::declval<T>()))>>
  : std::true_type
{};

template<class T>
constexpr bool is_iterable_v = is_iterable<T>::value;

} // namespace details

class Lua
{
private:
  lua_State* L;

  bool
  checkLua(int r)
  {
    if (r != LUA_OK) {
      const auto errorMsg = std::string{ lua_tostring(L, -1) };
      lua_pop(L, 1);

      error("Lua: error {}", errorMsg);
      return false;
    }
    return true;
  }

  template<typename Ret>
  Ret
  get_return_value()
  {
    if constexpr (std::is_same_v<Ret, bool>) {
      return static_cast<bool>(lua_toboolean(L, -1));
    }
    else if constexpr (std::is_same_v<Ret, int>) {
      return luaL_checkinteger(L, -1);
    }
    else if constexpr (std::is_same_v<Ret, double>) {
      return luaL_checknumber(L, -1);
    }
    else if constexpr (std::is_same_v<Ret, std::string>) {
      return std::string{ luaL_checkstring(L, -1) };
    }
    else if constexpr (details::is_map_v<Ret>) {
      std::map<std::string, std::string> map;
      lua_pushnil(L);
      while (lua_next(L, -2) != 0) {
        if (lua_isstring(L, -1) && lua_isstring(L, -2)) {
          auto key = std::string{ lua_tostring(L, -2) };
          auto value = std::string{ lua_tostring(L, -1) };
          map[key] = value;
          lua_pop(L, 1);
        }
        else {
          error("Lua: Wrong return type.");
          exit(1);
        }
      }
      return map;
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
    else if constexpr (std::is_same_v<Arg, double>) {
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
      error("Lua: Wrong argument type!");
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

      error("Lua: error {}", errorMsg);
      return std::nullopt;
    }

    auto result = get_return_value<Ret>();
    lua_pop(L, 1);

    return result;
  }

public:
  using map = std::map<std::string, std::string>;

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
    if (!checkLua(luaL_dofile(L, filepath.c_str()))) {
      lua_close(L);
    }
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

      error("Lua: No member function: {}", name);
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

      error("Lua: No global function: {}", name);
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
}

#endif
