#ifndef SOLVER_HPP_
#define SOLVER_HPP_

#include "data.hpp"

#include <optional>

namespace satisfactory {

std::optional<Solution> Solve(const Input& input);

}  // namespace satisfactory

#endif  // SOLVER_HPP_
