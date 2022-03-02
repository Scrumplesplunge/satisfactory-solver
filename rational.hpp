#ifndef RATIONAL_HPP_
#define RATIONAL_HPP_

#include "integer.hpp"

#include <cassert>
#include <compare>
#include <cstdint>
#include <iosfwd>
#include <numeric>

namespace satisfactory {

class Rational {
 public:
  constexpr Rational() = default;

  template <std::integral T>
  constexpr Rational(T x) : numerator_(x), denominator_(1) {}

  constexpr Rational(int128 numerator, int128 denominator) noexcept
      : numerator_(numerator), denominator_(denominator) {
    assert(denominator_ > 0);
    Normalize();
  }

  explicit operator double() const noexcept {
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
  }

  constexpr Rational Inverse() const noexcept {
    return numerator_ > 0 ? Rational(denominator_, numerator_)
                          : Rational(-denominator_, -numerator_);
  }

  inline friend constexpr Rational operator-(Rational r) noexcept {
    return Rational(-r.numerator_, r.denominator_);
  }

  inline friend constexpr Rational operator+(Rational l, Rational r) noexcept {
    return Rational(
        l.numerator_ * r.denominator_ + r.numerator_ * l.denominator_,
        l.denominator_ * r.denominator_);
  }

  inline friend constexpr Rational operator-(Rational l, Rational r) noexcept {
    return l + (-r);
  }

  inline friend constexpr Rational operator*(Rational l, Rational r) noexcept {
    if (int128 x = gcd(l.numerator_, r.denominator_); x != 1) {
      l.numerator_ /= x;
      r.denominator_ /= x;
    }
    if (int128 x = gcd(r.numerator_, l.denominator_); x != 1) {
      r.numerator_ /= x;
      l.denominator_ /= x;
    }
    return Rational(l.numerator_ * r.numerator_,
                    l.denominator_ * r.denominator_);
  }

  inline friend constexpr Rational operator/(Rational l, Rational r) noexcept {
    return l * r.Inverse();
  }

  constexpr bool operator==(const Rational& other) const noexcept = default;

  inline friend constexpr auto operator<=>(Rational l, Rational r) noexcept {
    return l.numerator_ * r.denominator_ <=> r.numerator_ * l.denominator_;
  }

  Rational& operator+=(const Rational& other) {
    return (*this = *this + other);
  }

  Rational& operator-=(const Rational& other) {
    return (*this = *this - other);
  }

  Rational& operator*=(const Rational& other) {
    return (*this = *this * other);
  }

  Rational& operator/=(const Rational& other) {
    return (*this = *this / other);
  }

  int128 numerator() const noexcept { return numerator_; }
  int128 denominator() const noexcept { return denominator_; }

 private:
  void Normalize() {
    const int128 x = gcd(numerator_, denominator_);
    numerator_ /= x;
    denominator_ /= x;
  }

  int128 numerator_ = 0;
  int128 denominator_ = 1;
};

std::ostream& operator<<(std::ostream& output, const Rational& rational);

}  // namespace satisfactory

#endif  // RATIONAL_HPP_
