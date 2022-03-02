#include "rational.hpp"

#include <iostream>
#include <sstream>

namespace satisfactory {

std::ostream& operator<<(std::ostream& output, const Rational& rational) {
  std::ostringstream temp;
  const int128 quotient = rational.numerator() / rational.denominator();
  const int128 remainder = rational.numerator() % rational.denominator();
  if (remainder == 0) {
    temp << quotient;
  } else if (quotient == 0) {
    temp << remainder << '/' << rational.denominator();
  } else {
    temp << quotient << '+' << remainder << '/' << rational.denominator();
  }
  return output << temp.str();
}

}  // namespace satisfactory
