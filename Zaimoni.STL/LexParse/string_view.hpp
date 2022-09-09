#ifndef ZAIMONI_STL_LEXPARSE_STRING_VIEW
#define ZAIMONI_STL_LEXPARSE_STRING_VIEW 1

#include <string_view>
#include <array>
#include <optional>
#include <functional>

// Exemplars from Perl
void ltrim(std::string_view& src);
void rtrim(std::string_view& src);
void trim(std::string_view& src);

constexpr std::string_view substr(std::string_view src, size_t origin, ptrdiff_t len) {
	if (src.size() <= origin) return std::string_view();
	src.remove_prefix(origin);
	if (0 > len) {
		size_t prune = -len;
		if (src.size() <= prune) return std::string_view();
		src.remove_suffix(prune);
		return src;
	}
	if (0 == len) return std::string_view();
	if (src.size() > len) src.remove_suffix(src.size() - len);
	return src;
}

// start compile-time only ... = map {...} analogs

template<size_t n>
consteval std::array<std::string_view, n> substr(const std::array<std::string_view, n>& src, size_t origin, ptrdiff_t len) {
	std::array<std::string_view, n> ret;
	ptrdiff_t i = -1;
	for (decltype(auto) x : src) {
		ret[++i] = substr(x, origin, len);
	};
	return ret;
}

namespace perl {

	template<size_t N, class T, size_t n> requires(1 <= N && N < n)
		consteval auto shift(const std::array<T, n>& src) {
		std::pair<std::array<T, N>, std::array<T, n - N> > ret;
		ptrdiff_t i = -1;
		for (decltype(auto) x : src) {
			if (++i < N) ret.first[i] = x;
			else ret.second[i - N] = x;
		};
		return ret;
	}

	template<class T, size_t n>
	consteval auto push(const std::array<T, n>& dest, const T& src) {
		std::array<T, n+1> ret(dest);
		ret.back() = src;
		return ret;
	}

	template<class T, size_t n>
	consteval auto unshift(const std::array<T, n>& dest, const T& src) {
		std::array<T, n + 1> ret;
		std::copy_n(dest.begin(), n, ret.begin() + 1);
		ret.front() = src;
		return ret;
	}
}

// end exemplars from Perl

size_t is_alphabetic(const std::string_view& src);
size_t is_alphanumeric(const std::string_view& src);
size_t is_digit(const std::string_view& src);
size_t is_hex_digit(const std::string_view& src);
std::optional<std::pair<std::string_view, size_t> > kleene_star(const std::string_view& src, std::function<size_t(const std::string_view&)> ok);

#endif
