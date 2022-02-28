#include "data.hpp"

#include <iostream>

namespace satisfactory {
namespace {

struct ResourceList {
  const std::map<std::string_view, int>& value;
};

std::ostream& operator<<(std::ostream& output, ResourceList list) {
  bool first = true;
  for (const auto& [resource, quantity] : list.value) {
    if (first) {
      first = false;
    } else {
      output << " + ";
    }
    if (quantity > 0) {
      output << quantity << ' ' << resource;
    } else {
      output << '(' << resource << ')';
    }
  }
  return output;
}

}  // namespace

std::ostream& operator<<(std::ostream& output, const Recipe& recipe) {
  return output << ResourceList(recipe.inputs) << " -> "
                << ResourceList(recipe.outputs) << " (" << recipe.duration
                << "s, cost " << recipe.cost << ')';
}

std::ostream& operator<<(std::ostream& output, const Demand& demand) {
  return output << demand.name << " (" << demand.units_per_minute << "/min)";
}

std::ostream& operator<<(std::ostream& output, const Input& input) {
  output << "Produce:\n";
  for (const auto& demand : input.demands) {
    output << "  " << demand << '\n';
  }
  output << "Using:\n";
  for (const auto& recipe : input.recipes) {
    output << "  " << recipe << '\n';
  }
  output << "Minimizing total cost.";
  return output;
}

std::ostream& operator<<(std::ostream& output, const Solution& solution) {
  output << "Produce:\n";
  for (const auto& [name, rate] : solution.outputs) {
    output << "  " << name << " (" << double(rate) << "/min)\n";
  }
  output << "From:\n";
  for (const auto& [name, rate] : solution.inputs) {
    output << "  " << name << " (" << double(rate) << "/min)\n";
  }
  output << "With:\n";
  const int r = solution.input->recipes.size();
  for (int i = 0; i < r; i++) {
    if (solution.uses[i] != 0) {
      output << "  " << double(solution.uses[i]) << "x\t"
             << solution.input->recipes[i] << '\n';
    }
  }
  output << "For a total cost of " << double(solution.cost);
  return output;
}

}  // namespace satisfactory
