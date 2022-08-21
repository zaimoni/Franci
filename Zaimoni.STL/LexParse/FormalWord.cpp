#include "FormalWord.hpp"

namespace formal {

	std::string_view word::value() const {
		// std::visit failed, here
		if (decltype(auto) test2 = std::get_if<std::shared_ptr<const std::string> >(&_token)) return (std::string_view)(*test2->get());
		return std::get<std::string_view>(_token);
	}

	size_t word::size() const {
		if (decltype(auto) test2 = std::get_if<std::shared_ptr<const std::string> >(&_token)) return test2->get()->size();
		return std::get<std::string_view>(_token).size();
	}

	size_t word::len(const std::variant<size_t, std::string_view>& src) {
		if (auto ret = std::get_if<size_t>(&src)) return *ret;
		return std::get<std::string_view>(src).size();
	}

	std::shared_ptr<const std::string> word::slice(ptrdiff_t offset, size_t len) const {
		auto src = value();
		if (0 > offset) return nullptr;
		if (0 >= len) return nullptr;
		const auto _size = src.size();
		if (_size <= offset) return nullptr;
		if (auto ub = _size - offset; ub < len) len = ub;
		return std::shared_ptr<const std::string>(new std::string(src.data() + offset, len));
	}


}	// namespace formal
