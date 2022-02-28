#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <string_view>

#include "data.hpp"

namespace satisfactory {

Input ParseInput(std::string_view source);

}  // namespace satisfactory

#endif  // PARSER_HPP_
