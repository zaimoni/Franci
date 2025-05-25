#ifndef ZAIMONI_STL_PERL_HPP
#define ZAIMONI_STL_PERL_HPP 1
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#if 0
// for when it's not practical to use a converting constructor, or cast expression
namespace zaimoni {
	template<class dest, class src> dest from(const src& x) {
		if constexpr (requires {*x; })
			return from<dest>(*x);
		else
			static_assert(unconditional_v<bool, false>, "unimplemented");
	}
}
#endif

namespace perl {
	class scalar {
	private:
		std::variant<std::string_view, std::string, std::shared_ptr<const std::string> > _str;
	public:
		scalar() = default;
		scalar(const scalar& src) = default;
		scalar(scalar&& src) = default;
		scalar& operator=(const scalar& src) = default;
		scalar& operator=(scalar&& src) = default;
		~scalar() = default;

		scalar(const std::string_view& src) : _str(src) {}
		scalar(std::string&& src) : _str(std::move(src)) {}
		scalar(const std::string& src) : _str(src) {}
		scalar(std::shared_ptr<const std::string>&& src) : _str(src) {}
		scalar(const std::shared_ptr<const std::string>& src) : _str(src) {}
		scalar(const char* src) : _str(std::string_view(src)) {}
		scalar(std::nullptr_t) = delete;

		scalar& operator=(const std::string_view& src) {
			_str = src;
			return *this;
		}

		scalar& operator=(std::string&& src) {
			_str = std::move(src);
			return *this;
		}

		scalar& operator=(const std::string& src) {
			_str = src;
			return *this;
		}

		scalar& operator=(std::shared_ptr<const std::string>&& src) {
			_str = std::move(src);
			return *this;
		}

		scalar& operator=(const std::shared_ptr<const std::string>& src) {
			_str = src;
			return *this;
		}

		std::string_view view() const {
			if (auto x = std::get_if<std::string>(&_str)) return std::string_view(*x);
			if (auto x = std::get_if<std::shared_ptr<const std::string> >(&_str)) return std::string_view(*(x->get()));
			return std::get<std::string_view>(_str);
		}
	};

	inline std::string join(const std::vector<scalar>& src, const std::string_view sep) {
		std::string ret;
		if (src.empty()) return ret;
//		ret.reserve(src.size() * 2);	// unclear why Copilot wanted this
		for (decltype(auto) x : src) {
			if (!ret.empty()) ret += sep;
			ret += x.view();
		}
		return ret;
	}

} // namespace perl

#endif
