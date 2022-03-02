#include "integer.hpp"

#include <cassert>
#include <charconv>
#include <iomanip>

namespace satisfactory {
namespace integer {
namespace {

// destination -= source * factor
// mod (1 << (32 * destination.size()))
void SubtractMultiple(std::span<std::uint32_t> destination,
                      std::span<const std::uint32_t> source,
                      std::uint32_t factor) noexcept {
  const int n = std::min(destination.size(), source.size() + 1);
  // Since we are subtracting factor * (source << (32 * shift)), digits in the
  // range destination.subspan(0, shift) cannot be affected.
  std::uint32_t mul_carry = 0, sub_carry = 0;
  for (int i = 0; i < n; i++) {
    // Calculate the ith digit of source * factor
    const std::uint64_t source_i =
        std::uint64_t(source[i]) * factor + mul_carry;
    mul_carry = source_i >> 32;
    // Calculate the ith digit of destination - source * factor
    const std::uint64_t temp =
        std::uint64_t(destination[i]) - std::uint32_t(source_i) - sub_carry;
    destination[i] = temp;
    sub_carry = bool(temp >> 32);
  }
  assert(std::uint64_t(mul_carry) + sub_carry < (std::uint64_t(1) << 32));
  Subtract(destination.subspan(n), mul_carry + sub_carry);
}

template <typename T>
std::span<T> Narrow(std::span<T> value) noexcept {
  return value.subspan(0, RealSize(value));
}

}  // namespace

int RealSize(std::span<const std::uint32_t> value) noexcept {
  for (int i = value.size() - 1; i >= 0; i--) {
    if (value[i]) return i + 1;
  }
  return 0;
}

void Add(std::span<std::uint32_t> destination, std::uint32_t source) noexcept {
  std::uint32_t carry = source;
  const int n = destination.size();
  for (int i = 0; i < n && carry; i++) {
    std::uint64_t temp = std::uint64_t(destination[i]) + carry;
    destination[i] = temp;
    carry = temp >> 32;
  }
}

void Add(std::span<std::uint32_t> destination,
         std::span<const std::uint32_t> source) noexcept {
  const int n = std::min(destination.size(), source.size());
  std::uint32_t carry = 0;
  for (int i = 0; i < n; i++) {
    std::uint64_t temp = std::uint64_t(destination[i]) + source[i] + carry;
    destination[i] = temp;
    carry = temp >> 32;
  }
  Add(destination.subspan(n), carry);
}

void Subtract(std::span<std::uint32_t> destination,
              std::uint32_t source) noexcept {
  std::uint32_t carry = source;
  const int n = destination.size();
  for (int i = 0; i < n && carry; i++) {
    std::uint64_t temp = std::uint64_t(destination[i]) - carry;
    destination[i] = temp;
    carry = bool(temp >> 32);
  }
}

void Subtract(std::span<std::uint32_t> destination,
              std::span<const std::uint32_t> source) noexcept {
  const int n = std::min(destination.size(), source.size());
  std::uint32_t carry = 0;
  for (int i = 0; i < n; i++) {
    std::uint64_t temp = std::uint64_t(destination[i]) - source[i] - carry;
    destination[i] = temp;
    carry = bool(temp >> 32);
  }
  Subtract(destination.subspan(n), carry);
}

void Multiply(std::span<std::uint32_t> destination,
              std::span<const std::uint32_t> a,
              std::span<const std::uint32_t> b) noexcept {
  std::ranges::fill(destination, 0);
  a = Narrow(a);
  b = Narrow(b);
  const int n = destination.size();
  const int a_size = a.size();
  const int b_size = b.size();
  for (int i = 0; i < a_size; i++) {
    if (i >= n) break;
    const int end = std::min(b_size - 1, n - i);
    for (int j = 0; j <= end; j++) {
      const std::uint64_t temp = std::uint64_t(a[i]) * std::uint64_t(b[j]);
      const std::uint32_t words[] = {std::uint32_t(temp),
                                     std::uint32_t(temp >> 32)};
      Add(destination.subspan(i + j), words);
    }
  }
}

std::uint32_t Divide(std::span<std::uint32_t> destination,
                     std::uint32_t source) noexcept {
  std::uint64_t carry = 0;
  for (int i = destination.size() - 1; i >= 0; i--) {
    const std::uint64_t x = carry << 32 | destination[i];
    destination[i] = x / source;
    carry = x % source;
  }
  return carry;
}

void DivMod(std::span<std::uint32_t> quotient,
            std::span<std::uint32_t> remainder,
            std::span<const std::uint32_t> divisor) noexcept {
  remainder = Narrow(remainder);
  divisor = Narrow(divisor);
  std::fill(quotient.begin(), quotient.end(), 0);
  assert(!divisor.empty());
  while (true) {
    // If the remainder is strictly smaller than the divisor, then the quotient
    // is 0 and the remainder is simply the original number.
    if (remainder.size() < divisor.size()) return;
    int shift = int(remainder.size()) - int(divisor.size());

    // If the divisor doesn't fit into the remainder at least once when aligning
    // the leading digits, reduce the shift.
    std::uint64_t remainder_prefix = remainder.back();
    if (Compare(divisor, remainder.subspan(shift)) > 0) {
      // If the shift is 0, the divisor doesn't fit into the remainder at all,
      // so we are done.
      if (shift == 0) return;
      shift--;
      remainder_prefix =
          remainder_prefix << 32 | remainder[remainder.size() - 2];
    }

    // Now we know that the divisor fits into the remainder at least once with
    // the given alignment. Create an estimate for how many times it fits into
    // the remainder.
    assert(Compare(divisor, remainder.subspan(shift)) <= 0);
    assert(divisor.back() + 1 > (remainder_prefix >> 32));
    assert(divisor.back() <= remainder_prefix);

    // Generate an underestimate for how many times the divisor fits into
    // remainder.subspan(shift), by assuming the worst case (where the rest of
    // the remainder is 0s after the prefix, and the rest of the divisor is 1s
    // after the prefix).
    const std::uint32_t estimate = remainder_prefix / (divisor.back() + 1);
    if (estimate > 1) {
      SubtractMultiple(remainder.subspan(shift), divisor, estimate);
      Add(quotient.subspan(shift), estimate);
    } else if (Compare(divisor, remainder.subspan(shift)) <= 0) {
      Subtract(remainder.subspan(shift), divisor);
      Add(quotient.subspan(shift), 1);
    } else {
      assert(estimate == 0);
      assert(shift == 0);
      break;
    }
    remainder = Narrow(remainder);
  }
}

void ShiftLeft(std::span<std::uint32_t> value, int amount) noexcept {
  const int major_shift = amount / 32;
  const int minor_shift = amount % 32;
  if (major_shift >= int(value.size())) {
    std::ranges::fill(value, 0);
    return;
  }
  if (minor_shift == 0) {
    std::ranges::copy_backward(value.subspan(0, value.size() - major_shift),
                               value.end());
  } else {
    for (int i = value.size() - 1; i > major_shift; i--) {
      value[i] = value[i - major_shift] << minor_shift |
                 value[i - major_shift - 1] >> (32 - minor_shift);
    }
    value[major_shift] = value[0] << minor_shift;
  }
  std::ranges::fill(value.subspan(0, major_shift), 0);
}

void ShiftRight(std::span<std::uint32_t> value, int amount) noexcept {
  const int major_shift = amount / 32;
  const int minor_shift = amount % 32;
  if (major_shift >= int(value.size())) {
    std::ranges::fill(value, 0);
    return;
  }
  const int n = value.size() - major_shift - 1;
  if (minor_shift == 0) {
    std::ranges::copy(value.subspan(major_shift), value.begin());
  } else {
    for (int i = 0; i < n; i++) {
      value[i] = value[i + major_shift] >> minor_shift |
                 value[i + major_shift + 1] << (32 - minor_shift);
    }
    value[n] = value[n + major_shift] >> minor_shift;
  }
  std::ranges::fill(value.subspan(n + 1), 0);
}

bool Equal(std::span<const std::uint32_t> a,
           std::span<const std::uint32_t> b) noexcept {
  return std::ranges::equal(Narrow(a), Narrow(b));
}

std::strong_ordering Compare(std::span<const std::uint32_t> a,
                             std::span<const std::uint32_t> b) noexcept {
  a = Narrow(a);
  b = Narrow(b);
  if (auto result = a.size() <=> b.size(); result != 0) return result;
  for (int i = a.size() - 1; i >= 0; i--) {
    if (auto result = a[i] <=> b[i]; result != 0) return result;
  }
  return std::strong_ordering::equal;
}

void ParseDecimal(std::span<std::uint32_t> destination,
                  std::span<std::uint32_t> scratch,
                  std::string_view input) noexcept {
  assert(scratch.size() >= destination.size());
  scratch = scratch.subspan(0, destination.size());
  constexpr int kBatchSize = 9;
  constexpr std::uint32_t kBatchFactor = 1'000'000'000;  // 10^kBatchSize
  assert(!destination.empty());
  const int n = input.size();
  const char* const first = input.data();
  const int first_batch_size = n % kBatchSize;
  const int num_batches = 1 + n / kBatchSize;

  // Each multiplication will toggle between using the destination buffer or the
  // scratch buffer, so since we know exactly how many batches of digits there
  // are, we can arrange that the final multiplication writes to the destination
  // buffer.
  std::span<std::uint32_t> a = destination;
  std::span<std::uint32_t> b = scratch;
  if (num_batches % 2 == 0) std::swap(a, b);

  std::ranges::fill(a, 0);
  std::from_chars(first, first + first_batch_size, a[0]);
  for (int i = first_batch_size; i < n; i += kBatchSize) {
    std::swap(a, b);
    Multiply(a, b, std::span(&kBatchFactor, 1));
    std::uint32_t value = 0;
    std::from_chars(first + i, first + i + kBatchSize, value);
    Add(a, std::span(&value, 1));
  }
  assert(a.data() == destination.data());
}

std::span<char> EncodeDecimal(std::span<char> buffer,
                              std::span<std::uint32_t> source) noexcept {
  constexpr int kBatchSize = 9;
  constexpr std::uint32_t kBatchFactor = 1'000'000'000;  // 10^kBatchSize
  assert(!buffer.empty());
  char* o = buffer.data() + buffer.size();
  while (true) {
    const std::uint32_t remainder = Divide(source, kBatchFactor);
    o -= kBatchSize;
    const auto [p, error] = std::to_chars(o, o + kBatchSize, remainder);
    std::copy_backward(o, p, o + kBatchSize);
    const int w = p - o;
    if (Equal(source, {})) {
      // This is the last batch.
      const char* const start = o + kBatchSize - w;
      return buffer.subspan(start - buffer.data());
    }
    // There are more significant non-zero digits.
    std::fill(o, o + kBatchSize - w, '0');
  }
}

}  // namespace integer
}  // namespace satisfactory
