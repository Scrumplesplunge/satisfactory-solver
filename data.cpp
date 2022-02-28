#include "data.hpp"

#include <iomanip>
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
  output << "Recipe Uses:\n\n";
  output << std::setw(12) << "Uses" << '\t' << "Recipe\n";
  const int r = solution.input->recipes.size();
  for (int i = 0; i < r; i++) {
    if (solution.uses[i] != 0) {
      output << "  " << std::setw(10) << solution.uses[i] << '\t'
             << solution.input->recipes[i] << '\n';
    }
  }
  output << "\nTotal Production (units/min):\n\n";
  output << std::setw(12) << "units/min" << '\t' << "Resource\n";
  for (const auto& [name, rate] : solution.total) {
    if (rate != 0) {
      output << "  " << std::setw(10) << rate << '\t' << name << '\n';
    }
  }
  output << "\nNet Production:\n\n";
  output << std::setw(12) << "units/min" << '\t' << "Resource\n";
  for (const auto& [name, rate] : solution.net) {
    if (rate != 0) {
      output << "  " << std::setw(10) << rate << '\t' << name << '\n';
    }
  }
  output << "\nFor a total cost of " << solution.cost;
  return output;
}

}  // namespace satisfactory
