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

// quotient = 0
// remainder = input
// while ?? {
//
// }

class StreamSaver {
 public:
  StreamSaver(std::ostream& stream)
      : stream_(stream), flags_(stream.flags()), fill_(stream.fill()) {}

  ~StreamSaver() {
    stream_.flags(flags_);
    stream_.fill(fill_);
  }

 private:
  std::ostream& stream_;
  std::ios_base::fmtflags flags_;
  char fill_;
};

template <typename T> struct Hex { const T& value; };
template <typename T> Hex(T) -> Hex<T>;

template <typename T>
std::ostream& operator<<(std::ostream& output, Hex<T> h) {
  StreamSaver save(output);
  return output << std::hex << h.value;
}

struct DebugInt { std::span<const std::uint32_t> value; };

std::ostream& operator<<(std::ostream& output, DebugInt x) {
  StreamSaver save(output);
  const int n = x.value.size();
  output.fill('0');
  if (x.value.empty()) return output << std::setw(8) << std::hex << 0;
  bool first = true;
  for (int i = n - 1; i >= 0; i--) {
    if (first) {
      first = false;
    } else {
      output << ' ';
    }
    output << std::setw(8) << std::hex << x.value[i];
  }
  return output;
}

void DivMod(std::span<std::uint32_t> quotient,
            std::span<std::uint32_t> remainder,
            std::span<const std::uint32_t> divisor) noexcept {
  remainder = Narrow(remainder);
  divisor = Narrow(divisor);
  std::fill(quotient.begin(), quotient.end(), 0);
  assert(!divisor.empty());
  // std::cout << "divisor = " << DebugInt(divisor) << '\n';
  while (true) {
    // std::cout << "Begin iteration\n";
    // std::cout << "quotient = " << DebugInt(quotient) << '\n'
    //           << "remainder = " << DebugInt(remainder) << '\n';
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
    // std::cout << "remainder_prefix = " << Hex(remainder_prefix) << '\n'
    //           << "estimate         = " << estimate << " (base 10), or "
    //           << Hex(estimate) << " (base 16)\n";
    if (estimate > 1) {
      SubtractMultiple(remainder.subspan(shift), divisor, estimate);
      Add(quotient.subspan(shift), estimate);
      // std::cout << "->\n"
      //           << "quotient = " << DebugInt(quotient) << '\n'
      //           << "remainder = " << DebugInt(remainder) << '\n';
    } else if (Compare(divisor, remainder.subspan(shift)) <= 0) {
      Subtract(remainder.subspan(shift), divisor);
      Add(quotient.subspan(shift), 1);
      // std::cout << "->\n"
      //           << "quotient = " << DebugInt(quotient) << '\n'
      //           << "remainder = " << DebugInt(remainder) << '\n';
    } else {
      assert(estimate == 0);
      assert(shift == 0);
      break;
    }
    // std::cout << "=======\n";
    remainder = Narrow(remainder);
    //assert(Compare(divisor, remainder.subspan(shift)) > 0);
  }
}

/*
  // ATTEMPT 2

  if (divisor.size() == 1) {
    // For division by a small divisor, use the fast division routine. This will
    // compute the quotient, so we'll need to copy it from the remainder span to
    // the quotient span and write the single-digit remainder into the remainder
    // span afterwards.
    const std::uint32_t r = Divide(remainder, divisor[0]);
    const int n = std::min(remainder.size(), quotient.size());
    std::ranges::copy(remainder.subspan(0, n), quotient.begin());
    std::ranges::fill(quotient.subspan(n), 0);
    std::ranges::fill(remainder, 0);
    remainder[0] = r;
    return;
  }

  // If the divisor is longer than the remainder, then it is larger: the
  // quotient is 0 and the remainder is the input value.
  if (divisor.size() > remainder.size()) return;
  assert(divisor.size() >= 2);
  const std::uint64_t divisor_prefix =
      std::uint64_t(divisor[divisor.size() - 1]) << 32 |
      std::uint64_t(divisor[divisor.size() - 2]);
  std::uint32_t value_copy[32];
  char buffer[1024];
  const auto debug = [&](std::span<const std::uint32_t> value) {
    std::ranges::fill(value_copy, 0);
    std::ranges::copy(value, value_copy);
    std::span<char> result = EncodeDecimal(buffer, value_copy);
    return std::string_view(result.data(), result.size());
  };
  std::cout << "divisor = " << debug(divisor) << '\n';
  std::cout << "divisor_prefix = " << divisor_prefix << '\n';
  while (remainder.size() >= 2) {
    std::cout << "tick\n";
    std::cout << "quotient = " << debug(quotient) << '\n';
    std::cout << "remainder = " << debug(remainder) << '\n';
    // Estimate how many times the divisor goes into the remainder.
    const std::uint64_t remainder_prefix =
        std::uint64_t(remainder[remainder.size() - 1]) << 32 |
        std::uint64_t(remainder[remainder.size() - 2]);
    std::cout << "remainder_prefix = " << remainder_prefix << '\n';
    // The guess must not overestimate, but it can underestimate, so we assume
    // the worst case (all digits after the remainder prefix are 0 and all
    // digits after the divisor prefix are 1), and ensure that we would
    // underestimate even that by incrementing the divisor prefix.
    const std::uint64_t guess = remainder_prefix / (divisor_prefix + 1);
    std::cout << "guess = " << guess << '\n';
    const int shift = remainder.size() - divisor.size();
    std::cout << "shift = " << shift << '\n';
    if (guess) SubtractMultiple(remainder.subspan(shift), divisor, guess);
    // The error ratio for the guess is at most 1/2^32, since divisor_prefix is
    // at least 2^32. Furthermore, the guess is strictly less than 2^32, since
    // both the remainder_prefix and divisor_prefix are greater than 2^32. This
    // means that the guess is off by at most 1.
    std::uint64_t factor = guess;
    if (Compare(divisor, remainder.subspan(shift)) <= 0) {
      std::cout << "performing additional subtraction\n";
      Subtract(remainder.subspan(shift), divisor);
      factor++;
    }
    std::uint32_t quotient_delta[] = {std::uint32_t(factor),
                                      std::uint32_t(factor >> 32)};
    Add(quotient.subspan(shift), quotient_delta);
    std::cout << "->\n";
    std::cout << "factor = " << factor << '\n';
    std::cout << "quotient = " << debug(quotient) << '\n';
    std::cout << "remainder = " << debug(remainder) << '\n';
    assert(factor);
    assert(Compare(divisor, remainder.subspan(shift)) > 0);
    //assert(Narrow(remainder).size() < remainder.size());
    remainder = Narrow(remainder);
  }
}

 // ATTEMPT 1
  while (true) {
    assert(!remainder.empty());
    assert(remainder.size() >= divisor.size());
    std::uint64_t temp = remainder.back();
    int shift = remainder.size() - divisor.size();
    // Require that the prefix be strictly bigger than the divisor prefix.
    if (temp <= divisor.back()) {
      // If the shift is 0 and the divisor doesn't fit, then all that remains is
      // the true remainder of the division and so we are done.
      if (shift == 0) break;
      temp = temp << 32 | remainder[remainder.size() - 2];
      shift--;
    }
    // Estimate how many times the divisor goes into the remainder, using at
    // most 64 bits of the remainder and 32 bits of the quotient. This must not
    // be an overestimate, so we increment the divisor prefix which will force
    // us to underestimate.
    const std::uint64_t guess = temp / (std::uint64_t(divisor.back()) + 1);
    if (guess) {
      SubtractMultiple(remainder.subspan(shift), divisor, guess);
    } else if (Compare(divisor, remainder.subspan(shift)) <= 0) {
      Subtract(remainder.subspan(shift), divisor);
    } else {
      
    }
    // Since we know that we underestimated, we may have to do one additional
    // subtraction.
    //
    //   k <= divisor
    //   guess = temp / (divisor + k)
    //
    if (Compare(divisor, remainder.subspan(shift)) <= 0) {
      Subtract(remainder.subspan(shift), divisor);
    }
    remainder = Narrow(remainder);
    assert(Compare(divisor, remainder.subspan(shift)) > 0);
  }
} */

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
