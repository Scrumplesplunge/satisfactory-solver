#ifndef INTEGER_HPP_
#define INTEGER_HPP_

#include <algorithm>
#include <bit>
#include <cassert>
#include <compare>
#include <cstdint>
#include <iostream>
#include <span>

namespace satisfactory {
namespace integer {

int RealSize(std::span<const std::uint32_t> value) noexcept;

// destination += source
void Add(std::span<std::uint32_t> destination, std::uint32_t source) noexcept;
void Add(std::span<std::uint32_t> destination,
         std::span<const std::uint32_t> source) noexcept;

// destination -= source
void Subtract(std::span<std::uint32_t> destination,
              std::uint32_t source) noexcept;
void Subtract(std::span<std::uint32_t> destination,
              std::span<const std::uint32_t> source) noexcept;

// destination = a * b
void Multiply(std::span<std::uint32_t> destination,
              std::span<const std::uint32_t> a,
              std::span<const std::uint32_t> b) noexcept;

// Divide the destination by the source. Returns the remainder.
std::uint32_t Divide(std::span<std::uint32_t> destination,
                     std::uint32_t source) noexcept;

// Sets destination equal to remainder / divisor and sets remainder equal to
// remainder % divisor.
void DivMod(std::span<std::uint32_t> quotient,
            std::span<std::uint32_t> remainder,
            std::span<const std::uint32_t> divisor) noexcept;

void ShiftLeft(std::span<std::uint32_t> value, int amount) noexcept;
void ShiftRight(std::span<std::uint32_t> value, int amount) noexcept;

bool Equal(std::span<const std::uint32_t> a,
           std::span<const std::uint32_t> b) noexcept;
std::strong_ordering Compare(std::span<const std::uint32_t> a,
                             std::span<const std::uint32_t> b) noexcept;

// Parse a decimal value from a string_view. If the decimal value exceeds the
// representable range of the destination, then it will be wrapped modularly.
// scratch must be a buffer of at least the same size as destination, and will
// be used as temporary storage space.
void ParseDecimal(std::span<std::uint32_t> destination,
                  std::span<std::uint32_t> scratch,
                  std::string_view input) noexcept;

// Encode the given integer as a decimal string stored in the given buffer,
// returning the value. The buffer must be big enough to store the full
// decimal value. Note: the source value is destructively modified.
std::span<char> EncodeDecimal(std::span<char> buffer,
                              std::span<std::uint32_t> value) noexcept;

}  // namespace integer

template <int n>
class Uint {
 public:
  constexpr Uint() noexcept = default;

  template <std::integral T>
  constexpr Uint(T x) noexcept {
    assert(x >= 0);
    for (int i = 0; i < kNumWords; i++) {
      value_[i] = x;
      x /= std::uint64_t(1) << 32;
    }
  }

  template <int m>
  constexpr explicit Uint(const Uint<m>& u) {
    std::copy(std::begin(u.value_), std::begin(u.value_) + kNumWords, value_);
  }

  constexpr explicit Uint(std::string_view value) noexcept {
    std::uint32_t scratch[kNumWords];
    integer::ParseDecimal(value_, scratch, value);
  }

  constexpr explicit operator double() const noexcept {
    double result = 0;
    for (int i = kNumWords - 1; i >= 0; i++) {
      result = result * (std::uint64_t(1) << 32) + value_[i];
    }
    return result;
  }

  constexpr Uint& operator+=(const Uint& u) noexcept {
    integer::Add(value_, u.value_);
    return *this;
  }

  constexpr Uint& operator-=(const Uint& u) noexcept {
    integer::Subtract(value_, u.value_);
    return *this;
  }

  constexpr Uint& operator*=(const Uint& u) noexcept {
    Uint temp = *this;
    integer::Multiply(value_, temp.value_, u.value_);
    return *this;
  }

  constexpr Uint& operator/=(std::uint32_t x) noexcept {
    integer::Divide(value_, x);
    return *this;
  }

  constexpr Uint& operator/=(const Uint& u) noexcept {
    Uint copy = *this;
    integer::DivMod(value_, copy.value_, u.value_);
    return *this;
  }

  constexpr Uint& operator%=(std::uint32_t x) noexcept {
    const std::uint32_t remainder = integer::Divide(value_, x);
    std::fill(std::begin(value_), std::end(value_), 0);
    value_[0] = remainder;
    return *this;
  }

  constexpr Uint& operator%=(const Uint& u) noexcept {
    integer::DivMod(std::span<std::uint32_t>(), value_, u.value_);
    return *this;
  }

  constexpr Uint& operator<<=(std::uint32_t amount) noexcept {
    integer::ShiftLeft(value_, amount);
    return *this;
  }

  constexpr Uint& operator>>=(std::uint32_t amount) noexcept {
    integer::ShiftRight(value_, amount);
    return *this;
  }

  friend constexpr Uint operator+(const Uint& l, const Uint& r) noexcept {
    Uint temp = l;
    temp += r;
    return temp;
  }

  friend constexpr Uint operator-(const Uint& l, const Uint& r) noexcept {
    Uint temp = l;
    temp -= r;
    return temp;
  }

  friend constexpr Uint operator*(const Uint& l, const Uint& r) noexcept {
    Uint temp;
    integer::Multiply(temp.value_, l.value_, r.value_);
    return temp;
  }

  friend constexpr Uint operator/(const Uint& l, std::uint32_t x) noexcept {
    Uint temp = l;
    temp /= x;
    return temp;
  }

  friend constexpr Uint operator/(const Uint& l, const Uint& r) noexcept {
    Uint temp = l;
    temp /= r;
    return temp;
  }

  friend constexpr std::uint32_t operator%(const Uint& l,
                                           std::uint32_t x) noexcept {
    Uint temp = l;
    return integer::Divide(temp.value_, x);
  }

  friend constexpr Uint operator%(const Uint& l, const Uint& r) noexcept {
    Uint temp = l;
    temp %= r;
    return temp;
  }

  friend constexpr Uint operator<<(const Uint& l, std::uint32_t r) noexcept {
    Uint temp = l;
    temp <<= r;
    return temp;
  }

  friend constexpr Uint operator>>(const Uint& l, std::uint32_t r) noexcept {
    Uint temp = l;
    temp >>= r;
    return temp;
  }

  friend constexpr bool operator==(const Uint& l, const Uint& r) noexcept {
    return integer::Equal(l.value_, r.value_);
  }

  friend constexpr std::strong_ordering operator<=>(const Uint& l,
                                                    const Uint& r) noexcept {
    return integer::Compare(l.value_, r.value_);
  }

  friend constexpr int countr_zero(const Uint<n>& u) {
    int major = 0;
    while (major < kNumWords && u.value_[major] == 0) major++;
    int minor = major < kNumWords ? std::countr_zero(u.value_[major]) : 0;
    return 32 * major + minor;
  }

  friend constexpr Uint gcd(Uint l, Uint r) {
    if (l == 0) return r;
    if (r == 0) return l;
    const int i = countr_zero(l);
    l >>= i;
    const int j = countr_zero(r);
    r >>= j;
    const int k = std::min(i, j);
    while (true) {
      if (l > r) std::swap(l, r);
      r -= l;
      if (r == 0) return l << k;
      r >>= countr_zero(r);
    }
  }

  friend constexpr std::ostream& operator<<(std::ostream& output,
                                            Uint x) noexcept {
    char temp[kNumWords * 10];
    std::span<char> result = integer::EncodeDecimal(temp, x.value_);
    output.write(result.data(), result.size());
    return output;
  }

 private:
  static constexpr int kNumWords = (n + 31) / 32;

  std::uint32_t value_[kNumWords] = {};
};

template <int n>
class Int {
 public:
  constexpr Int() noexcept = default;
  template <std::unsigned_integral T>
  constexpr Int(T x) noexcept : value_(x) {}
  template <std::integral T>
  constexpr Int(T x) noexcept
      : negative_(x < 0), value_(x < 0 ? -std::make_unsigned_t<T>(x) : x) {}
  constexpr Int(const Uint<n>& value) noexcept : value_(value) {}

  constexpr explicit Int(std::string_view value) noexcept {
    if (value.starts_with("-")) {
      negative_ = true;
      value.remove_prefix(1);
    }
    value_ = Uint<n>(value);
  }

  constexpr explicit operator double() const noexcept {
    const double temp = static_cast<double>(value_);
    return negative_ ? -temp : temp;
  }

  constexpr Int& operator+=(const Int& other) noexcept {
    if (negative_ == other.negative_) {
      // Signs are equal, so addition won't change the sign.
      value_ += other.value_;
    } else if (value_ < other.value_) {
      // Signs are opposed and the other value has a larger magnitude, so the
      // sign will flip.
      negative_ = !negative_;
      value_ = other.value_ - value_;
    } else {
      // Signs are opposed but this value has equal or larger magnitude, so the
      // sign will stay the same.
      value_ = value_ - other.value_;
    }
    return *this;
  }

  constexpr Int& operator-=(const Int& other) noexcept {
    if (negative_ != other.negative_) {
      // Signs are opposed, so subtraction won't change the sign.
      value_ += other.value_;
    } else if (value_ < other.value_) {
      // Signs are the same and the other value has a larger magnitude, so the
      // sign will flip.
      negative_ = !negative_;
      value_ = other.value_ - value_;
    } else {
      // Signs are the same but this value has equal or larger magnitude, so the
      // sign will stay the same.
      value_ = value_ - other.value_;
    }
    return *this;
  }

  constexpr Int& operator*=(const Int& other) noexcept {
    negative_ = (negative_ != other.negative_);
    value_ *= other.value_;
    return *this;
  }

  constexpr Int& operator/=(const Int& other) noexcept {
    assert(other.value_ != 0);
    negative_ = (negative_ != other.negative_);
    value_ /= other.value_;
    return *this;
  }

  constexpr Int& operator%=(const Int& other) noexcept {
    assert(other.value_ != 0);
    value_ %= other.value_;
    return *this;
  }

  friend constexpr Int operator-(const Int& x) noexcept {
    Int temp = x;
    temp.negative_ = !temp.negative_;
    return temp;
  }

  friend constexpr Int operator+(const Int& l, const Int& r) noexcept {
    Int temp = l;
    temp += r;
    return temp;
  }

  friend constexpr Int operator-(const Int& l, const Int& r) noexcept {
    Int temp = l;
    temp -= r;
    return temp;
  }

  friend constexpr Int operator*(const Int& l, const Int& r) noexcept {
    Int temp = l;
    temp *= r;
    return temp;
  }

  friend constexpr Int operator/(const Int& l, const Int& r) noexcept {
    Int temp = l;
    temp /= r;
    return temp;
  }

  friend constexpr Int operator%(const Int& l, const Int& r) noexcept {
    Int temp = l;
    temp %= r;
    return temp;
  }

  friend constexpr bool operator==(const Int& l, const Int& r) noexcept {
    if (l.value_ == 0 && r.value_ == 0) return true;
    return l.negative_ == r.negative_ && l.value_ == r.value_;
  }

  friend constexpr std::strong_ordering operator<=>(const Int& l,
                                                    const Int& r) noexcept {
    if (l.value_ == 0 && r.value_ == 0) return std::strong_ordering::equal;
    if (l.negative_ && !r.negative_) return std::strong_ordering::less;
    if (!l.negative_ && r.negative_) return std::strong_ordering::greater;
    return !l.negative_ ? l.value_ <=> r.value_ : r.value_ <=> l.value_;
  }

  friend constexpr Int gcd(const Int& l, const Int& r) {
    return gcd(l.value_, r.value_);
  }

  friend constexpr std::ostream& operator<<(std::ostream& output,
                                            const Int& x) noexcept {
    if (x.negative_) output << '-';
    return output << x.value_;
  }

 private:
  bool negative_ = false;
  Uint<n> value_;
};

using uint128 = Uint<128>;
using int128 = Int<128>;

}  // satisfactory

#endif  // INTEGER_HPP_
