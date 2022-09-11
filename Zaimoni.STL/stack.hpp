#ifndef ZAIMONI_STL_STACK_HPP
#define ZAIMONI_STL_STACK_HPP

#include <array>

namespace zaimoni {

	template<class T, size_t n>
	class stack {
		std::array<T, n> _x;
		size_t ub;

	public:
		constexpr stack() noexcept : _x(), ub(0) {}
		constexpr stack(const stack&) = default;
		constexpr stack(stack&&) = default;
		constexpr stack& operator=(const stack&) = default;
		constexpr stack& operator=(stack&&) = default;
		constexpr ~stack() = default;

		constexpr void clear() { ub = 0; }
		constexpr bool empty() const { return 0 == ub; }
		constexpr size_t size() const { return ub; }
		constexpr T& operator[](size_t N) { return _x[N]; }
		constexpr const T& operator[](size_t N) const { return _x[N]; }

		constexpr auto begin() { return _x.begin(); }
		constexpr auto begin() const { return _x.begin(); }
		constexpr auto end() { return _x.begin() + ub + 1; }
		constexpr auto end() const { return _x.begin() + ub + 1; }

		constexpr void push_back(const T& src) { _x[ub++] = src; }
		constexpr void push_back(T&& src) { _x[ub++] = std::move(src); }
	};

} // end namespace zaimoni

#endif

