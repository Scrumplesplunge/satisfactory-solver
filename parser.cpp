#include "parser.hpp"

#include <iostream>

namespace satisfactory {
namespace {

bool IsWhitespace(char c) { return c == ' ' || c == '\r' || c == '\n'; }
bool IsLower(char c) { return 'a' <= c && c <= 'z'; }
bool IsUpper(char c) { return 'A' <= c && c <= 'Z'; }
bool IsAlpha(char c) { return IsLower(c) || IsUpper(c); }
bool IsDigit(char c) { return '0' <= c && c <= '9'; }
bool IsIdentifier(char c) { return IsAlpha(c) || IsDigit(c); }

class Parser {
 public:
  Parser(std::string_view source) : remaining_(source) {
    if (remaining_.empty() || remaining_.back() != '\n') {
      Advance(remaining_.size());
      Die("input must end with a newline");
    }
  }

  int ParseInt() {
    const std::string_view number = Sequence<IsDigit>("expected an integer");
    int value = 0;
    for (char c : number) value = 10 * value + (c - '0');
    return value;
  }

  std::pair<std::string_view, int> ParseItemCount() {
    if (ConsumePrefix("(")) {
      SkipWhitespace();
      const std::string_view resource_name =
          Sequence<IsIdentifier>("expected a primitive resource name");
      SkipWhitespace();
      if (!ConsumePrefix(")")) Die("expected ')'");
      return {resource_name, 0};
    } else {
      const int count = ParseInt();
      SkipWhitespace();
      const std::string_view resource_name =
          Sequence<IsIdentifier>("expected a resource name");
      return {resource_name, count};
    }
  }

  Recipe ParseRecipe() {
    if (remaining_.empty()) Die("expected recipe");
    Recipe result;
    // Parse the inputs.
    while (true) {
      result.inputs.insert(ParseItemCount());
      SkipWhitespace();
      if (ConsumePrefix("->")) break;
      if (!ConsumePrefix("+")) Die("expected '+' or '->'");
      SkipWhitespace();
    }
    SkipWhitespace();
    // Parse the outputs.
    while (true) {
      result.outputs.insert(ParseItemCount());
      SkipWhitespace();
      if (ConsumePrefix("(")) break;
      if (!ConsumePrefix("+")) Die("expected '+' or '('");
      SkipWhitespace();
    }
    result.duration = ParseInt();
    if (!ConsumePrefix("s, cost ")) Die("expected '(<N>s, cost <N>)'");
    result.cost = ParseInt();
    if (!ConsumePrefix(")")) Die("expected ')'");
    return result;
  }

  Demand ParseDemand() {
    if (remaining_.empty()) Die("expected demand");
    const std::string_view resource_name =
        Sequence<IsIdentifier>("expected a resource name");
    SkipWhitespace();
    if (!ConsumePrefix("(")) Die("expected '('");
    const int units_per_minute = ParseInt();
    if (!ConsumePrefix("/min)")) Die("expected '(<N>/min)'");
    return Demand(resource_name, units_per_minute);
  }

  Input ParseInput() {
    Input input;
    SkipWhitespaceAndComments();
    while (!remaining_.empty()) {
      const char lookahead = remaining_.front();
      if (IsAlpha(lookahead)) {
        input.demands.push_back(ParseDemand());
      } else {
        input.recipes.push_back(ParseRecipe());
      }
      SkipWhitespaceAndComments();
    }
    return input;
  }

 private:
  [[noreturn]] void Die(std::string_view message) const {
    std::cerr << "source:" << line_ << ":" << column_ << ": error: " << message
              << '\n';
    std::exit(1);
  }

  void Advance(int n) {
    for (char c : remaining_.substr(0, n)) {
      if (c == '\n') {
        line_++;
        column_ = 1;
      } else {
        column_++;
      }
    }
    remaining_.remove_prefix(n);
  }

  void SkipWhitespace() {
    const char* const first = remaining_.data();
    const char* const last = first + remaining_.size();
    const char* i = first;
    while (i != last && IsWhitespace(*i)) ++i;
    Advance(i - first);
  }

  template <auto Predicate>
  std::string_view PeekSequence() const noexcept {
    const char* const first = remaining_.data();
    const char* const end = first + remaining_.size();
    const char* i = first;
    while (i != end && Predicate(*i)) i++;
    return std::string_view(first, i - first);
  }

  template <auto Predicate>
  std::string_view Sequence(std::string_view expectation) {
    std::string_view value = PeekSequence<Predicate>();
    if (value.empty()) Die(expectation);
    Advance(value.size());
    return value;
  }

  bool ConsumePrefix(std::string_view prefix) {
    if (!remaining_.starts_with(prefix)) return false;
    Advance(prefix.size());
    return true;
  }

  void SkipWhitespaceAndComments() {
    while (true) {
      SkipWhitespace();
      if (!remaining_.starts_with("//")) return;
      const char* const first = remaining_.data();
      const char* i = first;
      // This is guaranteed to terminate safely: a Source() always has
      // a newline character at the end.
      while (*i != '\n') i++;
      Advance(i - first);
    }
  }

  std::string_view remaining_;
  int line_ = 1;
  int column_ = 1;
};

}  // namespace

Input ParseInput(std::string_view source) {
  return Parser(source).ParseInput();
}

}  // namespace satisfactory
