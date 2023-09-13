#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace t {

namespace concepts {

template <typename T>
concept signed_integral_remove_cvref =
    std::signed_integral<std::remove_cvref_t<T>>;

template <typename T>
concept duration = requires(T d) {
  std::signed_integral<std::remove_reference_t<decltype(d.sec)>>;
  std::signed_integral<std::remove_reference_t<decltype(d.nsec)>>;
};

template <typename T>
concept time = requires(T d) {
  std::unsigned_integral<std::remove_reference_t<decltype(d.sec)>>;
  std::unsigned_integral<std::remove_reference_t<decltype(d.nsec)>>;
};

} // namespace concepts

struct duration {
  int32_t sec, nsec;

  duration() : sec{0}, nsec{0} {}

  duration(int32_t sec_, int32_t nsec_) : sec{sec_}, nsec{nsec_} {
    normalize(sec, nsec);
  }

  explicit duration(double sec_) { from_sec(sec_); }

  explicit duration(concepts::duration auto d) : sec{d.sec}, nsec{d.nsec} {
    normalize(sec, nsec);
  }

  template <concepts::duration T> operator T() {
    auto t = T{};
    t.sec = sec;
    t.nsec = nsec;
    return t;
  }

  double to_sec() const {
    return static_cast<double>(sec) + 1e-9 * static_cast<double>(nsec);
  }

  duration &from_sec(double sec_) {
    int64_t sec64 = static_cast<int64_t>(std::floor(sec_));
    if (sec64 < std::numeric_limits<int32_t>::min() ||
        sec64 > std::numeric_limits<int32_t>::max())
      throw std::runtime_error("Duration is out of dual 32-bit range");
    sec = static_cast<int32_t>(sec64);
    nsec = static_cast<int32_t>(std::round((sec_ - sec) * 1e9));
    int32_t rollover = nsec / 1000000000;
    sec += rollover;
    nsec %= 1000000000;
    return *this;
  }

  int64_t to_nsec() const {
    return static_cast<int64_t>(sec) * 1000000000 + static_cast<int64_t>(nsec);
  }

  duration &from_nsec(int64_t nsec_) {
    int64_t sec64 = nsec_ / 1000000000;
    if (sec64 < std::numeric_limits<int32_t>::min() ||
        sec64 > std::numeric_limits<int32_t>::max())
      throw std::runtime_error("Duration is out of dual 32-bit range");
    sec = static_cast<int32_t>(sec64);
    nsec = static_cast<int32_t>(nsec_ % 1000000000);

    normalize(sec, nsec);

    return *this;
  }

  template <std::signed_integral T>
  static auto normalize(T &sec, T &nsec) -> void {
    int64_t nsec_part = nsec % 1000000000;
    int64_t sec_part = sec + nsec / 1000000000;
    if (nsec_part < 0) {
      nsec_part += 1000000000;
      --sec_part;
    }

    if (sec_part < std::numeric_limits<int32_t>::min() ||
        sec_part > std::numeric_limits<int32_t>::max())
      throw std::runtime_error("Duration is out of dual 32-bit range");

    sec = sec_part;
    nsec = nsec_part;
  }

  auto operator<=>(const duration &rhs) const = default;

  duration operator+(const duration &rhs) const {
    duration t;
    return t.from_nsec(to_nsec() + rhs.to_nsec());
  }

  duration operator-(const duration &rhs) const {
    duration t;
    return t.from_nsec(to_nsec() - rhs.to_nsec());
  }

  duration operator-() const {
    duration t;
    return t.from_nsec(-to_nsec());
  }

  duration operator*(double scale) const { return duration(to_sec() * scale); }

  duration &operator+=(const duration &rhs) {
    *this = *this + rhs;
    return *this;
  }

  duration &operator-=(const duration &rhs) {
    *this += (-rhs);
    return *this;
  }

  duration &operator*=(double scale) {
    from_sec(to_sec() * scale);
    return *this;
  }
};

struct time {
  uint32_t sec, nsec;

  time() : sec{0}, nsec{0} {}

  time(uint32_t sec_, uint32_t nsec_) : sec{sec_}, nsec{nsec_} {
    normalize(sec, nsec);
  }

  explicit time(double sec_) { from_sec(sec_); }

  explicit time(const concepts::time auto &d) : sec{d.sec}, nsec{d.nsec} {
    normalize(sec, nsec);
  }

  template <concepts::time T> operator T() const {
    auto t = T{};
    t.sec = sec;
    t.nsec = nsec;
    return t;
  }

  double to_sec() const {
    return static_cast<double>(sec) + 1e-9 * static_cast<double>(nsec);
  }

  time &from_sec(double t) {
    int64_t sec64 = static_cast<int64_t>(floor(t));
    if (sec64 < 0 || sec64 > std::numeric_limits<uint32_t>::max())
      throw std::runtime_error("Time is out of dual 32-bit range");
    sec = static_cast<uint32_t>(sec64);
    nsec = static_cast<uint32_t>(std::round((t - sec) * 1e9));
    // avoid rounding errors
    sec += (nsec / 1000000000);
    nsec %= 1000000000;
    return *this;
  }

  uint64_t to_nsec() const {
    return static_cast<uint64_t>(sec) * 1000000000 +
           static_cast<uint64_t>(nsec);
  }

  time &from_nsec(uint64_t t) {
    uint64_t sec64 = 0;
    uint64_t nsec64 = t;

    normalize(sec64, nsec64);

    sec = static_cast<uint32_t>(sec64);
    nsec = static_cast<uint32_t>(nsec64);

    return *this;
  }

  template <std::unsigned_integral T>
  static auto normalize(T &sec, T &nsec) -> void {
    T nsec_part = nsec % 1000000000;
    T sec_part = nsec / 1000000000;

    if (sec + sec_part > std::numeric_limits<uint32_t>::max())
      throw std::runtime_error("Time is out of dual 32-bit range");

    sec += sec_part;
    nsec = nsec_part;
  }

  auto operator<=>(const time &rhs) const = default;

  duration operator-(const time &rhs) const {
    duration d;
    return d.from_nsec(to_nsec() - rhs.to_nsec());
  }

  time operator-(const duration &rhs) const { return *this + (-rhs); }

  time operator+(const duration &rhs) const {
    uint64_t sec_sum =
        static_cast<uint64_t>(sec) + static_cast<uint64_t>(rhs.sec);
    uint64_t nsec_sum =
        static_cast<uint64_t>(nsec) + static_cast<uint64_t>(rhs.nsec);

    // Throws an exception if we go out of 32-bit range
    normalize(sec_sum, nsec_sum);

    // now, it's safe to downcast back to uint32 bits
    return time(static_cast<uint32_t>(sec_sum),
                static_cast<uint32_t>(nsec_sum));
  }

  time &operator+=(const duration &rhs) {
    *this = *this + rhs;
    return *this;
  }

  time &operator-=(const duration &rhs) {
    *this += (-rhs);
    return *this;
  }
};

} // namespace t

#include "doctest.h"
TEST_SUITE("testing duration") {
  TEST_CASE("default constructor") {
    const auto d = t::duration(-123456, -789000012);
    CHECK(d.sec == -123457);
    CHECK(d.nsec == 210999988);
  }

  TEST_CASE("from_sec") {
    const auto d = t::duration(-123456.789);
    CHECK(d.sec == -123457);
    CHECK(d.nsec == 211000000);
  }

  TEST_CASE("<=> operator") {
    SUBCASE("sec") {
      const auto d1 = t::duration(12, 456700000);
      const auto d2 = t::duration(13.1234);
      const auto d3 = t::duration(-11.5678);
      CHECK(d1 < d2);
      CHECK(d2 > d1);
      CHECK(d1 > d3);
      CHECK(d3 < d1);
      CHECK(d2 > d3);
      CHECK(d3 < d2);
    }
    SUBCASE("nsec") {
      const auto d1 = t::duration(12, 456700000);
      const auto d2 = t::duration(12.4568);
      const auto d3 = t::duration(12, 456700000);
      CHECK(d1 < d2);
      CHECK(d2 > d1);
      CHECK(d1 <= d2);
      CHECK(d2 >= d1);
      CHECK(d1 == d3);
    }
  }

  TEST_CASE("+-* operator") {
    SUBCASE("+") {
      const auto d1 = t::duration(12.34);
      const auto d2 = t::duration(0.06);
      const auto d3 = t::duration(12.4);
      CHECK(d1 + d2 == d3);
    }
    SUBCASE("-") {
      const auto d1 = t::duration(0.04);
      const auto d2 = t::duration(12.34);
      const auto d3 = t::duration(-12.3);
      const auto d4 = -t::duration(12.3);
      CHECK(d1 - d2 == d3);
      CHECK(d3 == d4);
    }
    SUBCASE("*") {
      const auto d1 = t::duration(12.34);
      const auto d2 = t::duration(123.4);
      CHECK(d1 * 10 == d2);
    }
  }

  TEST_CASE("concept") {
    struct T1 {
      int8_t sec, nsec;
    };
    const auto t1 = T1{1, 2};
    const auto d1 = t::duration(t1);
    CHECK(d1.sec == 1);
    CHECK(d1.nsec == 2);
  }
}

TEST_SUITE("testing time") {
  TEST_CASE("default constructor") {
    const auto t = t::time(123456, 789000012);
    CHECK(t.sec == 123456);
    CHECK(t.nsec == 789000012);
  }

  TEST_CASE("from_sec") {
    const auto t = t::time(123456.789);
    CHECK(t.sec == 123456);
    CHECK(t.nsec == 789000000);
  }

  TEST_CASE("<=> operator") {
    SUBCASE("sec") {
      const auto t1 = t::time(12, 456700000);
      const auto t2 = t::time(13.1234);
      const auto t3 = t::time(11.5678);
      CHECK(t1 < t2);
      CHECK(t2 > t1);
      CHECK(t1 > t3);
      CHECK(t3 < t1);
      CHECK(t2 > t3);
      CHECK(t3 < t2);
    }
    SUBCASE("nsec") {
      const auto t1 = t::time(12, 456700000);
      const auto t2 = t::time(12.4568);
      const auto t3 = t::time(12, 456700000);
      CHECK(t1 < t2);
      CHECK(t2 > t1);
      CHECK(t1 <= t2);
      CHECK(t2 >= t1);
      CHECK(t1 == t3);
    }
  }

  TEST_CASE("+- operator") {
    SUBCASE("+") {
      const auto t1 = t::time(12.34);
      const auto d1 = t::duration(0.06);
      const auto t2 = t::time(12.4);
      CHECK(t1 + d1 == t2);
    }
    SUBCASE("-") {
      const auto t1 = t::time(12.34);
      const auto d1 = t::duration(0.04);
      const auto t2 = t::time(12.3);
      CHECK(t1 - d1 == t2);
      CHECK(t1 - t2 == d1);
    }
  }

  TEST_CASE("concept") {
    struct T1 {
      uint8_t sec, nsec;
    };
    const auto t1 = T1{1, 2};
    const auto d1 = t::time(t1);
    CHECK(d1.sec == 1);
    CHECK(d1.nsec == 2);
  }
}
