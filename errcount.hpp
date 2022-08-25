// errcount.hpp

#ifndef ERRCOUNT_HPP
#define ERRCOUNT_HPP 1

#include "Zaimoni.STL/Logging.h"

template<class T> requires(T(0) < T(-1))
class error_counter final
{
private:
	T _ub;
	T errors;
	const char* const _fatalmsg;

	error_counter(const error_counter& src) = delete; // yes, not copyable
	error_counter(error_counter&& src) = delete;
	error_counter& operator=(const error_counter& src) = delete;
	error_counter& operator=(error_counter&& src) = delete;

public:
	error_counter(T ub, const char* fatalmsg) noexcept : _ub(ub), errors(0), _fatalmsg(fatalmsg) {}
	~error_counter() = default;

	void operator++() { if (_ub <= ++errors) _fatal(_fatalmsg); }
	void operator++(int) { if (_ub <= ++errors) _fatal(_fatalmsg); }

	void set_ub(T src) { if (0 < src && errors < src) _ub = src; }
	T count() const { return errors; }
};

#endif

