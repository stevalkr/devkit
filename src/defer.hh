#pragma once

#include <memory>

#ifndef defer

#define DEFER_TOKEN(COUNTER) DEFER_##COUNTER
#define defer                                                                  \
  auto DEFER_TOKEN(__COUNTER__) = devkit::details::Deferer{} << [&]()

#endif

namespace devkit
{

namespace details
{

template<typename Fn>
class Defer
{
private:
  Fn fn;

public:
  explicit Defer(Fn&& fn)
    : fn{ std::forward<Fn>(fn) }
  {}
  Defer&
  operator=(Defer&& other) = delete;

  Defer(const Defer&) = delete;
  Defer&
  operator=(const Defer&) = delete;

  ~Defer()
  {
    fn();
  }
};

struct Deferer
{
  template<typename Fn>
  Defer<Fn>
  operator<<(Fn&& fn)
  {
    return Defer{ std::forward<Fn>(fn) };
  }
};

} // namespace details

} // namespace devkit

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest/doctest.h>

TEST_CASE("testing options")
{
  int i = 0;
  SUBCASE("class Defer")
  {
    {
      devkit::details::Defer increase{ [&]() {
        i += 1;
      } };
      CHECK(i == 0);
    }
    CHECK(i == 1);
  }

  SUBCASE("Macro defer")
  {
    {
      defer
      {
        i += 1;
      };
      CHECK(i == 0);
    }
    CHECK(i == 1);
  }
}
#endif
