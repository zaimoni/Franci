#include "FormalWord.hpp"

namespace formal {

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
