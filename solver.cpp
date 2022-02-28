// A satisfactory machine quantity optimizer. The optimizer takes a list of
// recipes that it can use, as well as a list of production demands that it must
// fulfil, and produces a summary of the recipes that should be used and the
// quantity of machines needed for each of those recipes in order to fulfil the
// demands.
//
// The solver works by reducing the problem to a linear programming problem in
// standard form and then solving that with the Simplex algorithm.
//
// A recipe consists of a list of input resources and their corresponding
// quantities, a duration (how long it takes for this recipe to execute), a list
// of output resources and their corresponding quantities, and a cost for using
// this recipe. This can be transformed into a list of production rates for each
// resource, where inputs have negative rates and outputs have positive rates.
// Resources that are not involved in a given recipe will have zero rates. This
// alternative formulation loses information about the burstiness (e.g. a recipe
// which takes 1s and produces 1 ingot each time is less bursty than a recipe
// which takes 60s and produces 60 ingots each time), but this is irrelevant for
// optimizing overall throughput, as the bursts can be smoothed out with
// sufficient buffering in the conveyors.
//
// A demand consists of a resource type and a desired production rate. The
// optimizer must meet this demand in addition to any intermediate demand that
// it generates as part of the production pipeline. We can model a demand
// constraint as an inequality that lower-bounds the total production rate of
// a given resource.
//
// The overall formulation is as follows:
//
// minimize dot(c, x)
// subject to:
//
//   Rx >= d
//   x >= 0
//
// where:
//
//   * c is an n-dimensional column vector of costs per recipe. Costs must be
//     non-negative.
//   * R is an r x n matrix where each column is a recipe vector (as above).
//   * x is our column vector of unknowns, with one row per recipe. This gives
//     us the fractional number of instances for each recipe that we should use.
//     Note that we will need to build at least ceil(x_i) machines for recipe i
//     in practice, since we can't have fractional machines, but we can
//     underclock those machines to achieve optimal efficiency.
//   * d is our column vector of demands, with one row per resource type. This
//     will be 0 for all other resource types (to ensure that our resulting
//     factory does not rely on externally provided resources). Note that raw
//     resources such as IronOre can be modelled via a recipe that has no
//     inputs.
//
// This linear programming problem is not in standard form yet, since it is
// a minimisation problem, but we need a maximisation problem. To address this,
// we will consider the dual problem:
//
// minimize dot(c, x)      maximize dot(d, y)
// subject to:             subject to:
//                     ->
//   Rx >= d                 R^T y <= c
//   x >= 0                  y >= 0
//
// When we introduce (non-negative) slack variables to the dual problem, we can
// produce a trivial basic feasible solution with y = 0 and x = c:
//
// maximize dot(d, y)
// subject to:
//
//   R^T y + x = c
//   y >= 0, x >= 0
//
// We can then solve the dual problem using the Simplex algorithm and derive the
// optimal values for x from the values of s. To do this, we need to populate a
// simplex tableau:
//
//  R^T  I  0 | c
// -d^T  0  1 | 0
//
// Given r recipes across n resource types, this table will have r + 1 rows and
// n + r + 2 columns. I is an r x r identity matrix representing the variables
// in x, which serve as the slack variables for the dual problem.

#include "solver.hpp"

#include <algorithm>
#include <iostream>
#include <optional>

#include "table.hpp"

namespace satisfactory {
namespace {

// Multiplies each element in the row by x.
constexpr void Multiply(std::span<Rational> row, Rational x) noexcept {
  for (Rational& d : row) d *= x;
}

// Adds a scalar multiple of the source row to the destination row.
constexpr void AddMultiple(std::span<Rational> destination,
                           std::span<const Rational> source,
                           Rational x) noexcept {
  assert(destination.size() == source.size());
  Rational* d = destination.data();
  Rational* const end = d + destination.size();
  const Rational* s = source.data();
  while (d != end) {
    *d += *s * x;
    ++d;
    ++s;
  }
}

// Retrieves a sorted list of all resources referenced by recipes or demands.
std::vector<std::string_view> Resources(const Input& input) {
  std::vector<std::string_view> result;
  for (const auto& recipe : input.recipes) {
    for (const auto& [resource, quantity] : recipe.inputs) {
      result.push_back(resource);
    }
    for (const auto& [resource, quantity] : recipe.outputs) {
      result.push_back(resource);
    }
  }
  for (const auto& [name, rate] : input.demands) {
    result.push_back(name);
  }
  std::ranges::sort(result);
  auto [first, last] = std::ranges::unique(result);
  result.erase(first, last);
  result.shrink_to_fit();
  return result;
}

// Given a sorted list of resource types and an input problem, build the initial
// Simplex tableau for the dual problem.
Table<Rational> BuildTableau(std::span<const std::string_view> resources,
                             const Input& input) {
  const int r = input.recipes.size();
  const int n = resources.size();
  const auto column = [&](std::string_view name) -> int {
    const auto first = resources.begin();
    const auto last = resources.end();
    const auto i = std::lower_bound(first, last, name);
    assert(i != last && *i == name);
    return i - first;
  };
  Table<Rational> tableau(n + r + 2, r + 1);
  for (int y = 0; y < r; y++) {
    const Recipe& recipe = input.recipes[y];
    auto row = tableau[y];
    // Populate the recipe rates.
    for (const auto& [resource, quantity] : recipe.inputs) {
      row[column(resource)] = -Rational(quantity) / recipe.duration;
    }
    for (const auto& [resource, quantity] : recipe.outputs) {
      row[column(resource)] = Rational(quantity) / recipe.duration;
    }
    // Populate the cost.
    row.back() = recipe.cost;
    // Populate the appropriate slack variable.
    row[n + y] = 1;
  }
  // Populate the final row of the table.
  const auto final_row = tableau[r];
  for (const auto& demand : input.demands) {
    final_row[column(demand.name)] = -Rational(demand.units_per_minute) / 60;
  }
  final_row[n + r] = 1;
  return tableau;
}

std::optional<int> PivotColumn(const Table<Rational>& tableau) {
  // Find the column with the minimum value in the cost row. This will be the
  // pivot column (assuming that the tableau is not already optimal), as the
  // most negative column is the one which gives the largest improvement in
  // the cost function with respect to change in the corresponding variable.
  const std::span<const Rational> cost_row = tableau[tableau.height() - 1];
  const auto i = std::ranges::min_element(cost_row);
  return *i < 0 ? std::optional<int>(i - cost_row.begin()) : std::nullopt;
}

std::optional<int> PivotRow(const Table<Rational>& tableau, int column) {
  // Find the row with the minimum ratio between its constant term and its
  // coefficient in the pivot column. This minimum ratio test ensures that the
  // other basic variables remain positive (and therefore feasible) after the
  // pivot.
  struct Best {
    int row;
    Rational ratio;
  };
  std::optional<Best> best;
  for (int y = 0; y < tableau.height() - 1; y++) {
    const Rational coefficient = tableau[y][column];
    const Rational value = tableau[y].back();
    // Skip rows which have a non-positive coefficient: the entering variable
    // will have the new value `value / coefficient`, and it is required that
    // `value` is always positive for any feasible solution (which must be the
    // case for the original tableau), so a negative coefficient would result in
    // a negative value for the variable, which is infeasible.
    if (coefficient <= 0) continue;
    const Rational ratio = value / coefficient;
    if (!best || ratio < best->ratio) {
      best = {.row = y, .ratio = ratio};
    }
  }
  // If no best row has been identified, that would mean that the entering
  // variable is unbounded, and it has a positive contribution towards the score
  // function, hence there is would be no optimal solution. Since we know that
  // our primal problem is feasible, this can never happen.
  return best ? std::optional<int>(best->row) : std::nullopt;
}

// Optimize a Simplex tableau.
std::optional<Table<Rational>> Solve(Table<Rational> tableau) {
  const std::span<const Rational> cost_row = tableau[tableau.height() - 1];
  while (true) {
    const Rational previous_score = tableau[tableau.height() - 1].back();
    const std::optional<int> column = PivotColumn(tableau);
    // If we can't identify a pivot column, the tableau is optimal.
    if (!column) return tableau;
    const std::optional<int> row = PivotRow(tableau, *column);
    if (!row) return std::nullopt;
    // Use Gaussian elimination to turn the pivot column into the row'th column
    // of the identity matrix.
    Multiply(tableau[*row], 1 / tableau[*row][*column]);
    assert(tableau[*row][*column] == 1);
    for (int y = 0; y < tableau.height(); y++) {
      if (y == *row) continue;
      // The value of the last column must be non-negative: since any
      // intermediate tableau should represent a basic feasible solution, the
      // value of the last column must be positive as this directly corresponds
      // to the value of one of the variables, and all variables must be
      // non-negative. Note that the value can be 0, and in this case we are
      // considering a degenerate basic variable which will not increase the
      // value of the cost function as part of this pivot.
      assert(tableau[y].back() >= tableau[y][*column] * tableau[*row].back());
      AddMultiple(tableau[y], tableau[*row], -tableau[y][*column]);
      assert(tableau[y][*column] == 0);
    }
    const Rational score = tableau[tableau.height() - 1].back();
    assert(score >= previous_score);
  }
}

// Given a Simplex tableau representing an optimal solution for the dual
// problem, extract the corresponding solution for the primal problem.
std::vector<Rational> ExtractSolution(const Table<Rational>& tableau) {
  // Extract the optimal solution for the primal problem from the tableau. Since
  // the tableau represents the dual problem, this is extracted from the
  // coefficients in the cost function rather than from the final column.
  const int r = tableau.height() - 1;
  const int n = tableau.width() - r - 2;
  const std::span<const Rational> uses =
      tableau[tableau.height() - 1].subspan(n, r);
  return std::vector(uses.begin(), uses.end());
}

Rational GetCost(const Table<Rational>& tableau) {
  return tableau[tableau.height() - 1].back();
}

struct Rates {
  std::map<std::string_view, Rational> total, net;
};

Rates GetRates(const Input& input, std::span<const Rational> uses) {
  assert(input.recipes.size() == uses.size());
  const int r = uses.size();
  Rates rates;
  for (int i = 0; i < r; i++) {
    const Recipe& recipe = input.recipes[i];
    // Populate the recipe rates.
    for (const auto& [resource, quantity] : recipe.inputs) {
      rates.net[resource] -= 60 * uses[i] * quantity / recipe.duration;
    }
    for (const auto& [resource, quantity] : recipe.outputs) {
      rates.total[resource] += 60 * uses[i] * quantity / recipe.duration;
      rates.net[resource] += 60 * uses[i] * quantity / recipe.duration;
    }
  }
  return rates;
}

}  // namespace

std::optional<Solution> Solve(const Input& input) {
  // Retrieve the list of resources referenced by the input problem. The order
  // of elements in this list will determine the column order in the tableau.
  const std::vector<std::string_view> resources = Resources(input);
  // Convert the problem into a Simplex tableau for the dual problem and
  // optimize it.
  const std::optional<Table<Rational>> tableau =
      Solve(BuildTableau(resources, input));
  if (!tableau) return std::nullopt;
  // Extract the optimal solution.
  std::vector<Rational> uses = ExtractSolution(*tableau);
  Rates rates = GetRates(input, uses);
  return Solution{.input = &input,
                  .uses = std::move(uses),
                  .total = std::move(rates.total),
                  .net = std::move(rates.net),
                  .cost = GetCost(*tableau)};
}

}  // namespace satisfactory
