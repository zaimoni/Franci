#ifndef ZAIMONI_STL_LEXPARSE_FORMAL_WORD_HPP
#define ZAIMONI_STL_LEXPARSE_FORMAL_WORD_HPP 1

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace formal {
	static constexpr const unsigned long long Comment = 1ULL; // reserve this flag for both word and lex_node
	static constexpr const unsigned long long Error = (1ULL << 63); // reserve this flag for both word and lex_node

	// this belongs elsewhere
	struct src_location {
		std::shared_ptr<const std::filesystem::path> path;
		std::pair<size_t, ptrdiff_t> line_pos;

		src_location() noexcept : line_pos(0, 0) {}
		src_location(std::pair<int, int> src) noexcept : line_pos(src) {}
		src_location(std::pair<int, int> src, const std::shared_ptr<const std::filesystem::path>& from) noexcept : path(from), line_pos(0, 0) {}
		src_location(std::pair<int, int> src, std::shared_ptr<const std::filesystem::path>&& from) noexcept : path(std::move(from)), line_pos(0, 0) {}
		src_location(const src_location& src) = default;
		src_location(src_location&& src) = default;
		src_location& operator=(const src_location& src) = default;
		src_location& operator=(src_location&& src) = default;
		~src_location() = default;

		src_location& operator+=(int delta) {
			line_pos.second += delta;
			return *this;
		}

		std::string to_s() const {
			return std::string("(")+std::to_string(line_pos.first + 1) + ',' + std::to_string(line_pos.second + 1) + ')';
		}
	};

	inline src_location operator+(src_location lhs, ptrdiff_t rhs) { return lhs += rhs; }
	inline src_location operator+(ptrdiff_t lhs, src_location rhs) { return rhs += lhs; }

	class word {
	private:
		src_location _origin;
		std::variant<std::string_view, std::shared_ptr<const std::string> > _token;
		unsigned long long _code; // usually used as a bitmap

	public:
		word() = default;
		word(const word& src) = default;
		word(word&& src) = default;
		word& operator=(const word& src) = default;
		word& operator=(word&& src) = default;
		~word() = default;

		// slicing constructor
		word(const word& src, ptrdiff_t offset, size_t len, unsigned long long code = 0)
			: _origin(src._origin + offset),
			_token(src.slice(offset, len)),
			_code(code) {}

		word(const std::string_view& src, src_location origin, unsigned long long code = 0) noexcept
			: _origin(origin),
			_token(src),
			_code(code) {}

		word(std::shared_ptr<const std::string> src, src_location origin, unsigned long long code = 0) noexcept
			: _origin(origin),
			_token(src),
			_code(code) {}

		std::string_view value() const;
		size_t size() const;

		auto code() const { return _code; }
		void interpret(unsigned long long src) { _code = src; }

		src_location origin() const { return _origin; }
		src_location after() const { return _origin + size(); }

	private:
		static size_t len(const std::variant<size_t, std::string_view>& src);
		std::shared_ptr<const std::string> slice(ptrdiff_t offset, size_t len) const;
	};

	// parsing does not belong here.
	// programming languages need: leading, trailing
	// math also needs formatting: superscript, subscript, ...
} // namespace formal

#endif