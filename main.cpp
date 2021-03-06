#include <fstream>
#include <iostream>

#include "parser.hpp"
#include "solver.hpp"

namespace {

std::string GetContents(const char* filename) {
  std::ifstream file(filename);
  const std::string source(std::istreambuf_iterator<char>(file), {});
  if (!file.good()) {
    std::cerr << "Failed to read " << filename << "\n";
    std::exit(1);
  }
  return source;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: solver <filename>\n";
    return 1;
  }
  const std::string source = GetContents(argv[1]);
  const satisfactory::Input input = satisfactory::ParseInput(source);
  const std::optional<satisfactory::Solution> solution =
      satisfactory::Solve(input);
  if (!solution) {
    std::cerr << "A solution could not be found. Is a recipe missing?\n";
    return 1;
  }
  std::cout << *solution << "\n";
}
