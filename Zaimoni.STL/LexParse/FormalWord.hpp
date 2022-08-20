#ifndef ZAIMONI_STL_LEXPARSE_FORMAL_WORD_HPP
#define ZAIMONI_STL_LEXPARSE_FORMAL_WORD_HPP 1

#include <filesystem>
#include <functional>
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

	src_location operator+(src_location lhs, ptrdiff_t rhs) { return lhs += rhs; }
	src_location operator+(ptrdiff_t lhs, src_location rhs) { return rhs += lhs; }

	class word {
	private:
		src_location _origin;
		std::variant<std::string_view, std::shared_ptr<const std::string> > _token;
		unsigned long long _code; // usually used as a bitmap

	public:
		// offset, size, code
		using sub = std::tuple<ptrdiff_t,
			std::variant<size_t, std::string_view>,
			int>;
		using lexed = std::optional<sub>;

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

		std::string_view value() const {
			// std::visit failed, here
			if (decltype(auto) test2 = std::get_if<std::shared_ptr<const std::string> >(&_token)) return (std::string_view)(*test2->get());
			return std::get<std::string_view>(_token);
		}

		size_t size() const {
			if (decltype(auto) test2 = std::get_if<std::shared_ptr<const std::string> >(&_token)) return test2->get()->size();
			return std::get<std::string_view>(_token).size();
		}

		auto code() const { return _code; }
		void interpret(unsigned long long src) { _code = src; }

		src_location origin() const { return _origin; }
		src_location after() const { return _origin + size(); }

		// maybe not strictly needed
		lexed lex(std::function<lexed(std::string_view)> lex_rule) const { return lex_rule(value()); }

		std::vector<word> split(const sub& src) {
			std::vector<word> ret;
			const auto offset = std::get<0>(src);
			if (0<offset) ret.push_back(word(*this, 0, offset));

			decltype(auto) want_this = std::get<1>(src);
			auto _len = len(want_this);

			if (auto non_owned = std::get_if<std::string_view>(&want_this)) {
				ret.push_back(word(*non_owned, _origin + offset, std::get<2>(src)));
			} else {
				ret.push_back(word(*this, offset, _len, std::get<2>(src)));
			}

			const auto terminal = offset + _len;
			const auto _size = size();
			if (_size > terminal) ret.push_back(word(*this, terminal, _size-terminal));
			return ret;
		}

	private:
		static size_t len(const std::variant<size_t, std::string_view>& src) {
			if (auto ret = std::get_if<size_t>(&src)) return *ret;
			return std::get<std::string_view>(src).size();
		}

		std::shared_ptr<const std::string> slice(ptrdiff_t offset, size_t len) const {
			auto src = value();
			if (0 > offset) return nullptr;
			if (0 >= len) return nullptr;
			const auto _size = src.size();
			if (_size <= offset) return nullptr;
			if (auto ub = _size - offset; ub < len) len = ub;
			return std::shared_ptr<const std::string>(new std::string(src.data() + offset, len));
		}
	};

	// parsing does not belong here.
	// programming languages need: leading, trailing
	// math also needs formatting: superscript, subscript, ...
}

#endif