#include "integer.hpp"

#define CHECK_EQ(a, b) ::DoCheckPred<std::equal_to<>{}>(#a, "==", #b, (a), (b))
#define CHECK_LT(a, b) ::DoCheckPred<std::less<>{}>(#a, "<", #b, (a), (b))
#define CHECK_LE(a, b) \
  ::DoCheckPred<std::less_equal<>{}>(#a, "<=", #b, (a), (b))
#define CHECK_GT(a, b) ::DoCheckPred<std::greater<>{}>(#a, ">", #b, (a), (b))
#define CHECK_GE(a, b) \
  ::DoCheckPred<std::greater_equal{}>(#a, ">=", #b, (a), (b))
#define CHECK_NE(a, b) \
  ::DoCheckPred<std::not_equal_to{}>(#a, "!=", #b, (a), (b))
template <auto pred, typename L, typename R>
void DoCheckPred(const char* l_string, const char* op, const char* r_string,
                 const L& l, const R& r) {
  std::cout << "Check \x1b[36m" << l_string << ' ' << op << ' ' << r_string
            << "\x1b[0m: ";
  if (pred(l, r)) {
    std::cout << "\x1b[32m" "PASSED" "\x1b[0m\n";
  } else {
    std::cerr << "\x1b[31m" "FAILED" "\x1b[0m\n"
              << "  " << l_string << " = " << l << '\n'
              << "  " << r_string << " = " << r << '\n';
    std::exit(1);
  }
}

using ::satisfactory::uint128;
using ::satisfactory::int128;

int main() {
  // Check that small integers are represented correctly.
  CHECK_EQ(uint128(0x8000'0000ULL), 0x8000'0000ULL);
  CHECK_EQ(uint128(0x1'0000'0000ULL), 0x1'0000'0000ULL);

  // Check that we can shift across 32-bit boundaries.
  CHECK_EQ((uint128(0x8000'0000) << 1), 0x1'0000'0000ULL);
  CHECK_EQ((uint128(0x1'0000'0000) >> 1), 0x8000'0000ULL);

  // Check that we can multiply values.
  CHECK_EQ(uint128(0x1'0001) * uint128(0x1'0001), 0x1'0002'0001ULL);

  // Check that we can parse small integers.
  CHECK_EQ(uint128("1"), 1);
  CHECK_EQ(uint128("4294967298"), 4294967298ULL);

  // Check that division by a small divisor works.
  CHECK_EQ(uint128("1000000016000000063") / 1'000'000'007, 1'000'000'009);
  CHECK_EQ(uint128("1000000016000000063") % 1'000'000'007, 0);
  CHECK_EQ(uint128("1000000016000000062") % 1'000'000'007, 1'000'000'006);
  CHECK_EQ(uint128("1000000016000000064") % 1'000'000'007, 1);

  // Check that division by a long divisor works.
  CHECK_EQ(uint128("999999999999000001999999") / uint128("999999000001"),
           uint128("1000000999999"));
  CHECK_EQ(uint128("999999999999000001999999") % uint128("999999000001"), 0);
  CHECK_EQ(uint128("999999999999000001999998") % uint128("999999000001"),
           uint128("999999000000"));
  CHECK_EQ(uint128("999999999999000002000000") % uint128("999999000001"), 1);
}
