#ifndef ZAIMONI_STL_LEXPARSE_STRING_VIEW
#define ZAIMONI_STL_LEXPARSE_STRING_VIEW 1

#include <string_view>
#include <optional>
#include <functional>

void ltrim(std::string_view& src);
void rtrim(std::string_view& src);
void trim(std::string_view& src);

size_t is_alphabetic(const std::string_view& src);
size_t is_alphanumeric(const std::string_view& src);
size_t is_digit(const std::string_view& src);
size_t is_hex_digit(const std::string_view& src);
std::optional<std::pair<std::string_view, size_t> > kleene_star(const std::string_view& src, std::function<size_t(const std::string_view&)> ok);

#endif
