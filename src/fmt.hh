#pragma once

#include <cstdio>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/core.h>

#ifdef DEBUG

#define dk_log(fmt_str, ...)                                                   \
  fmt::print(stdout,                                                           \
             fg(fmt::color::cyan),                                             \
             "{}:{}:  \n  ",                                                   \
             __FILE__,                                                         \
             __LINE__,                                                         \
             ##__VA_ARGS__);                                                   \
  fmt::println(stdout, fmt_str, ##__VA_ARGS__)

#define dk_err(fmt_str, ...)                                                   \
  fmt::print(stderr,                                                           \
             fmt::emphasis::bold | fg(fmt::color::red),                        \
             "ERROR {}:{}:  \n  ",                                             \
             __FILE__,                                                         \
             __LINE__,                                                         \
             ##__VA_ARGS__);                                                   \
  fmt::println(stderr, fmt_str, ##__VA_ARGS__)

#define dk_panic(fmt_str, ...)                                                 \
  fmt::print(stderr,                                                           \
             fmt::emphasis::bold | fg(fmt::color::red),                        \
             "PANIC {}:{}:  \n  ",                                             \
             __FILE__,                                                         \
             __LINE__,                                                         \
             ##__VA_ARGS__);                                                   \
  fmt::println(stderr, fmt_str, ##__VA_ARGS__);                                \
  exit(1)

#else

#define dk_log(fmt_str, ...) fmt::println(stdout, fmt_str, ##__VA_ARGS__)

#define dk_err(fmt_str, ...) fmt::println(stderr, fmt_str, ##__VA_ARGS__)

#define dk_panic(fmt_str, ...)                                                 \
  fmt::println(stderr, fmt_str, ##__VA_ARGS__);                                \
  exit(1)

#endif

#define dk_fmt(fmt_str, ...) fmt::format(fmt_str, ##__VA_ARGS__)

namespace devkit
{

template<typename... T>
static std::string
fmt(fmt::format_string<T...> fmt, T&&... args)
{
  return fmt::format(fmt, std::forward<T>(args)...);
}

} // namespace devkit
