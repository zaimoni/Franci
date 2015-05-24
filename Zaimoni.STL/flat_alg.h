/* flat_alg.h */
/* algorithms that require almost no headers */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#ifndef ZAIMONI_STL_FLAT_ALG_H
#define ZAIMONI_STL_FLAT_ALG_H 1

#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
namespace zaimoni {

/* C string tests */
inline bool is_empty_string(const char* const x) {return NULL==x || '\0'==x[0];}
#else
#define is_empty_string(x) (NULL==x || '\0'==x[0])
#endif

#ifdef __cplusplus

/* from boost::numeric::interval
 * not useful for built-in types, but allows uniform code with interval arithmetic */
inline long double square(const long double& x) { return x*x;}

/* signum function */
template<class T>
inline signed int sgn(T x) {return (0<x) ? 1 : (0==x ? 0 : -1);}

#define ZAIMONI_DEFINE_UNSIGNED_SGN(TYPE) template<> inline signed int sgn<TYPE>(TYPE x) {return 0<x;}
ZAIMONI_DEFINE_UNSIGNED_SGN(unsigned char)
ZAIMONI_DEFINE_UNSIGNED_SGN(unsigned short)
ZAIMONI_DEFINE_UNSIGNED_SGN(unsigned int)
ZAIMONI_DEFINE_UNSIGNED_SGN(unsigned long)
/*ZAIMONI_DEFINE_UNSIGNED_SGN(unsigned long long) */
#undef ZAIMONI_DEFINE_UNSIGNED_SGN

/* cmp function */
template<class T1, class T2>
inline signed int cmp(T1 x, T2 y) {return (x<y) ? -1 : (x==y ? 0 : 1);}

/* -1^N */
template<class T>
inline signed int negative_one_pow(T N) {return (N%2) ? -1 : 1;}

template<typename T,typename U>
bool
trivial_pow(T x, U N)
{
	return x==1
		|| x==-1
		|| N==0
		|| x==0;
}

/* modeled on std::transform
 * but the unary operation is assumed to mutate its operand
 * probably closest conceptually to std::for_each */
/* range version */
template<class IO_Iterator,typename unary_op>
inline void autotransform(IO_Iterator first,IO_Iterator last,unary_op op)
{
	while(first!=last)
		op(*first++);
}

/* forward-iterator version */
template<class IO_Iterator,typename unary_op>
inline void autotransform_n(IO_Iterator first,size_t N,unary_op op)
{
	while(0<N)
		{
		op(*first++);
		--N;
		};
}

/* forward-iterator version */
template<typename binary_op,class IO_Iterator,class IO_Iterator2>
inline void autotransform_n(IO_Iterator first,IO_Iterator2 first2,size_t N,binary_op op)
{
	while(0<N)
		{
		op(*first++,*first2++);
		--N;
		};
}

/* range version */
template<typename unary_op,class IO_Iterator>
inline bool and_range(unary_op op,IO_Iterator first,IO_Iterator last)
{
	while(first!=last)
		if (!op(*first++)) return false;
	return true;
}

/* forward-iterator version */
template<typename unary_op,class IO_Iterator>
inline bool and_range_n(unary_op op,IO_Iterator first,size_t N)
{
	while(0<N)
		{
		if (!op(*first++)) return false;
		--N;
		};
	return true;
}

/* range version */
template<typename binary_op,class IO_Iterator>
inline bool and_range(binary_op op,IO_Iterator first,IO_Iterator last,IO_Iterator first2)
{
	while(first!=last)
		if (!op(*first++,*first2++)) return false;
	return true;
}

/* forward-iterator version */
template<typename binary_op,class IO_Iterator>
inline bool and_range_n(binary_op op,IO_Iterator first,size_t N,IO_Iterator first2)
{
	while(0<N)
		{
		if (!op(*first++,*first2++)) return false;
		--N;
		}
	return true;
}

/* range version */
template<typename unary_op,class IO_Iterator>
inline bool or_range(unary_op op,IO_Iterator first,IO_Iterator last)
{
	while(first!=last)
		if (op(*first++)) return true;
	return false;
}

/* forward-iterator version */
template<typename unary_op,class IO_Iterator>
inline bool or_range_n(unary_op op,IO_Iterator first,size_t N)
{
	while(0<N)
		{
		if (op(*first++)) return true;
		--N;
		};
	return false;
}

template<typename binary_op,class IO_Iterator>
inline bool pairwise_distinct(binary_op op,IO_Iterator first, IO_Iterator last)
{
	while(first!=last)
		{
		IO_Iterator first2 = first;
		while(++first2!=last)
			if (op(*first,*first2))
				return false;
		++first;
		}
	return true;
}

#if 0
template<IntType>
inline void self_abs(IntType& n)
{	/* do not rely on abs for the default (broken compilers, etc.)
	 * user types with efficient self_abs will redefine this */
	if (n<0) n = -n;
}

namespace detail {

/* These two ripped from Boost.Rational and de-robustified
 * lowercased to dodge macro conventions */

/* We use n and m as temporaries in this function, so there is no value
 * in using const IntType& as we would only need to make a copy anyway... */
template<class IntType>
IntType
gcd(IntType n, IntType m)
{
	self_abs(n);
	self_abs(m);
	
    /* As n and m are now positive, we can be sure that %= returns a
     * positive value (the standard guarantees this for built-in types,
     * and we require it of user-defined types). */
	while(true)
	{
		if (m==0) return n;
		n %= m;
		if (n==0) return m;
		m %= n;
	}
}

template<class IntType>
IntType
lcm(IntType n, IntType m)
{
    if (n==0 || m==0)
        return zero;

    n /= gcd(n, m);
    n *= m;

	self_abs(n);
    return n;
}

}	/* namespace detail */

#endif

}
#endif

#endif
