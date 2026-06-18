#pragma once

#include "Token.hxx"
#include "../Utils/utils.hxx"

#include <cctype>
#include <string>
#include <string_view>
#include <vector>


inline namespace pie {
inline namespace lex {


TokenKind keyword(const std::string_view word) noexcept;


bool validNameChar(const char c) noexcept;

[[nodiscard]] Tokens lex(const std::string& src);



} // namespace lex
} // namespace pie
