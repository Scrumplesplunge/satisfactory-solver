#ifndef RATIONAL_HPP_
#define RATIONAL_HPP_

#include <cassert>
#include <compare>
#include <cstdint>
#include <iosfwd>
#include <numeric>

namespace satisfactory {

class Rational {
 public:
  constexpr Rational() = default;
  constexpr Rational(std::int64_t x) : numerator_(x), denominator_(1) {}
  constexpr Rational(std::int64_t numerator, std::int64_t denominator) noexcept
      : numerator_(numerator), denominator_(denominator) {
    assert(denominator_ > 0);
    Normalize();
  }

  explicit operator double() const noexcept {
    return static_cast<double>(numerator_) / denominator_;
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
    if (std::int64_t x = std::gcd(l.numerator_, r.denominator_); x != 1) {
      l.numerator_ /= x;
      r.denominator_ /= x;
    }
    if (std::int64_t x = std::gcd(r.numerator_, l.denominator_); x != 1) {
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

  std::int64_t numerator() const noexcept { return numerator_; }
  std::int64_t denominator() const noexcept { return denominator_; }

 private:
  void Normalize() {
    const std::int64_t x = std::gcd(numerator_, denominator_);
    numerator_ /= x;
    denominator_ /= x;
  }

  std::int64_t numerator_ = 0;
  std::int64_t denominator_ = 1;
};

std::ostream& operator<<(std::ostream& output, const Rational& rational);

}  // namespace satisfactory

#endif  // RATIONAL_HPP_
