#pragma once

#include <cstdio>
#include <fmt/core.h>

namespace devkit
{

template<typename... T>
static std::string
fmt(fmt::format_string<T...> fmt, T&&... args)
{
  return fmt::format(fmt, std::forward<T>(args)...);
}

template<typename... T>
static void
log(fmt::format_string<T...> fmt, T&&... args)
{
  fmt::println(stdout, fmt, std::forward<T>(args)...);
}

template<typename... T>
static void
error(fmt::format_string<T...> fmt, T&&... args)
{
  fmt::println(stderr, fmt, std::forward<T>(args)...);
}

} // namespace devkit
