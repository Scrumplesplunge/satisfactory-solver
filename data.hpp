#ifndef DATA_HPP_
#define DATA_HPP_

#include "rational.hpp"

#include <iosfwd>
#include <map>
#include <string_view>
#include <vector>

namespace satisfactory {

struct Recipe {
  std::map<std::string_view, int> inputs, outputs;
  int duration;
  Rational cost;
};

struct Demand {
  std::string_view name;
  Rational units_per_minute;
};

struct Input {
  std::vector<Recipe> recipes;
  std::vector<Demand> demands;
};

struct Solution {
  const Input* input;
  // uses[i] is the total fractional throughput of input->recipes[i] required by
  // this solution.
  std::vector<Rational> uses;
  // Rate, in units/min, of consumption (inputs) or production (outputs) for the
  // resource of the given name.
  std::map<std::string_view, Rational> inputs, outputs;
  // The total cost of this solution.
  Rational cost;
};

std::ostream& operator<<(std::ostream&, const Recipe&);
std::ostream& operator<<(std::ostream&, const Demand&);
std::ostream& operator<<(std::ostream&, const Input&);
std::ostream& operator<<(std::ostream&, const Solution&);

}  // namespace satisfactory

#endif  // DATA_HPP_
