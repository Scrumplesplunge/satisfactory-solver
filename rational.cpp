#include "rational.hpp"

#include <iostream>

namespace satisfactory {

std::ostream& operator<<(std::ostream& output, const Rational& rational) {
  output << rational.numerator();
  if (std::int64_t d = rational.denominator(); d != 1) {
    output << "/" << d;
  }
  return output;
}

}  // namespace satisfactory
