// functional.hpp

#ifndef ZAIMONI_STL_BOOST_FUNCTIONAL_HPP
#define ZAIMONI_STL_BOOST_FUNCTIONAL_HPP 1

#include <functional>

// The STL made a design decision not to directly support mixed-mode operators.  We do support this.
// Stripped out C++03 adapter analogs (replaced by C++17 lambda functions) 2020-04-29

namespace zaimoni {

// assignment operators
// note that boost::bind2nd won't work on these (discard const qualifiers error) as of 1.34
template<class T,class U>
struct assign
{
	void operator()(T& x, typename zaimoni::param<U>::type y) const { x = y; }
};

template<class T,class U>
struct assign_add
{
	void operator()(T& x, typename zaimoni::param<U>::type y) const { x += y; }
};

template<class T,class U>
struct assign_subtract
{
	void operator()(T& x, typename zaimoni::param<U>::type y) { x -= y; }
};

template<class T,class U>
struct assign_multiply
{
	void operator()(T& x, typename zaimoni::param<T>::type y) { x *= y; }
};

template<class T,class U>
struct assign_divide
{
	void operator()(T& x, typename zaimoni::param<T>::type y) { x /= y; }
};

template<class T>
struct assign_modulus
{
	void operator()(T& x, typename zaimoni::param<T>::type y) { x %= y; }
};

// comparisons (use only for mixed-mode comparisons where copy-construction is inefficient or prohibited)
template<class T,class U>
struct equal_to
{
	bool operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) { return x==y; }
};

template<class T,class U>
struct not_equal_to
{
	bool operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) { return x!=y; }
};

template<class T,class U>
struct greater
{
	bool operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) { return x>y; }
};

template<class T,class U>
struct less
{
	bool operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) { return x<y; }
};

template<class T,class U>
struct greater_equal
{
	bool operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) { return x>=y; }
};

template<class T,class U>
struct less_equal
{
	bool operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) { return x<=y; }
};

// cross-type operators (use std:: for same type on both operands)
template<class T,class U>
struct right_multiply
{
	typename zaimoni::param<T>::type operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) {return x*y;};
};

template<class T,class U>
struct left_multiply
{
	typename zaimoni::param<T>::type operator()(typename zaimoni::param<T>::type x, typename zaimoni::param<U>::type y) {return x*y;};
};

// dereferencing adaptors
template<typename T,size_t N>
struct addref_power
{
	typedef typename addref_power<T,N-1>::type type;
};

template<typename T>
struct addref_power<T,1>
{
	typedef typename std::add_pointer_t<T> type;
};

template<typename T>
struct addref_power<T,0>
{
	typedef T type;
};

}	// end namespace zaimoni

#endif
