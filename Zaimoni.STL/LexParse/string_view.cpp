#include "string_view.hpp"

#include <ctype.h>

void ltrim(std::string_view& src) {
	while (!src.empty() && isspace(static_cast<unsigned char>(src.front()))) src.remove_prefix(1);
}

void rtrim(std::string_view& src) {
	while (!src.empty() && isspace(static_cast<unsigned char>(src.back()))) src.remove_suffix(1);
}

void trim(std::string_view& src) {
	ltrim(src);
	rtrim(src);
}

size_t is_alphabetic(const std::string_view& src)
{
	if (isalpha(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

size_t is_alphanumeric(const std::string_view& src)
{
	if (isalnum(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

size_t is_digit(const std::string_view& src)
{
	if (isdigit(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

size_t is_hex_digit(const std::string_view& src)
{
	if (isxdigit(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

std::optional<std::pair<std::string_view, size_t> > kleene_star(const std::string_view& src, std::function<size_t(const std::string_view&)> ok) {
	size_t len = src.empty() ? 0 : ok(src);
	if (0 >= len) return std::nullopt;
	size_t matched = 0;
	auto working = src;
	while (0 < len) {
		++matched;
		working.remove_prefix(len);
		len = src.empty() ? 0 : ok(working);
	}

	std::pair<std::string_view, size_t> ret(src, matched);
	ret.first.remove_suffix(working.size());
	return ret;
}
