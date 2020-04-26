// lite_alg.hpp
// (C)2009,2015,2020 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_LITE_ALG_HPP
#define ZAIMONI_LITE_ALG_HPP 1

#include "Logging.h"
#include <algorithm>
#include <type_traits>

namespace zaimoni {

inline bool
in_range(unsigned long test, const unsigned long LB, const unsigned long UB)
{
	assert(LB<UB);
	return test-LB<=UB-LB;
}

template<unsigned long LB,unsigned long UB>
bool in_range(unsigned long test)
{
	static_assert(LB<UB);
	return test-LB<=UB-LB;
}

#include "bits/lite_alg.hpp"

template<typename T>
struct ReferenceData
{
	static const unsigned long FactorialTable[13];
};

template<typename T>
const unsigned long ReferenceData<T>::FactorialTable[13]
	=	{	1,
			1,
			2,
			6,
			24,
			120,
			720,
			5040,
			40320,
			362880,
			3628800,
			39916800,
			479001600
		};

// 2^N
#ifdef UINTMAX_C
inline uintmax_t POWER_OF_2(unsigned int x) {assert(CHAR_BIT*sizeof(uintmax_t)>x); return UINTMAX_C(1)<<x;}
#else
inline unsigned long POWER_OF_2(unsigned int x) {assert(CHAR_BIT*sizeof(unsigned long)>x); return 1UL<<x;}
#endif

inline unsigned int
nonstrictly_bounded_above_by_this_power_of_n(unsigned long x,unsigned long n)
{	return zaimoni::detail::nonstrictly_bounded_above_by_this_power_of_n(x,n);}

// 0 is a "bad case" in definition of GCF.  We use
// * GCF(0,a) = a
// * GCF(0,0) = 0
inline unsigned long
GCF_machine(unsigned long LHS, unsigned long RHS)
{	return zaimoni::detail::GCF_machine(LHS,RHS);	}

inline unsigned long
GCF(signed long LHS, signed long RHS)
{	return zaimoni::detail::GCF_machine<unsigned long>(std::abs(LHS),std::abs(RHS));}

inline unsigned long
LCM_machine(unsigned long LHS, unsigned long RHS)
{	return zaimoni::detail::LCM_machine(LHS,RHS);	}

inline unsigned long
LCM(signed long LHS, signed long RHS)
{	return zaimoni::detail::LCM_machine<unsigned long>(std::abs(LHS),std::abs(RHS));}

inline unsigned long
scale_quotient(unsigned long X,unsigned long numerator,unsigned long denominator)
{
	assert(0!=denominator);
	if (numerator==denominator) return X;

	const bool smaller = numerator<denominator;
	const unsigned long Delta = smaller ? denominator-numerator : numerator-denominator;
	const unsigned long Target_div = (X/denominator)*Delta;
	const unsigned long Target_rem = (X%denominator)*Delta;
	const unsigned long Target_rem_div = Target_rem/denominator;
	// round towards original, like Wesnoth
	const unsigned long Target_rem_rem = (2*(Target_rem%denominator) > denominator) ? 1 : 0;
	const unsigned long Adjust = Target_div+Target_rem_div+Target_rem_rem;
	if (smaller)
		return X-Adjust;
	else
		return X+Adjust;
}

template<class T>
void
quotient_of_products_incremental_init(T*& numerator,T*& denominator,typename boost::call_traits<T>::param_type for_numerator, typename boost::call_traits<T>::param_type for_denominator,size_t Idx)
{
	assert(NULL!=numerator);
	assert(NULL!=denominator);
	assert(0<Idx);
	if 		(for_denominator==1)
		{
		do	{
			--Idx;
			*(numerator++) = for_numerator;
			}
		while(0<Idx);
		}
	else if (for_numerator==1)
		{
		do	{
			--Idx;
			*(denominator++) = for_denominator;
			}
		while(0<Idx);
		}
	else{
		do	{
			--Idx;
			*(numerator++) = for_numerator;
			*(denominator++) = for_denominator;
			}
		while(0<Idx);
		};
}

template<class endpoints,class Func,class param>
param piecewise_eval(const endpoints* interval_def,const Func* func_def,size_t i,const param& x)
{
	while(0<i)
		if (interval_def[--i]<=x)
			return func_def[i](x);
	FATAL("invalid invocation of piecewise_eval");
}

template<class T,class CoeffIterator>
T
horners_polynomial_eval(const T& x,CoeffIterator polycoeff,size_t N_plus_1)
{
	assert(1<N_plus_1);
	T accumulator(x);
	accumulator *= *polycoeff++;
	--N_plus_1;
	do	{
		accumulator += *polycoeff++;
		accumulator *= x;
		}
	while(0<--N_plus_1);
	accumulator += *polycoeff;
	return accumulator;
}

template<class T,class CoeffIterator>
T
horners_polynomial_eval_high_order_one(const T& x,CoeffIterator polycoeff,size_t N)
{
	assert(0<N);
	T accumulator(x);
	do	{
		accumulator += *polycoeff++;
		accumulator *= x;
		}
	while(0<--N);
	accumulator += *polycoeff;
	return accumulator;
}

// finite geometric series support
template<class T>
T
one_minus_r_to_n_over_one_minus_r(const T& r, unsigned long n)
{	// 1+r+...+r<sup>n</sup>
	// use Horner's algorithm
	const T T_One(1);
	if (0==n) return T_One;
	T accumulator = r;
	accumulator += T_One;
	while(0<--n)
		{
		accumulator *= r;
		accumulator += T_One;
		}
	return accumulator;
}

template<typename T,typename U>
T
geometric_series(const T& a, const U& r, unsigned long n)
{
	if (0==n) return 0;
	if (r==1) return a*n;
	return a*one_minus_r_to_n_over_one_minus_r(r,n-1);
}

// safer modulo arithmetic
inline unsigned long
fast_mod_sum(unsigned long a, unsigned long b, unsigned long m)
{	return zaimoni::detail::fast_mod_sum(a,b,m);	}

inline unsigned long
mod_sum(unsigned long a, unsigned long b, unsigned long m)
{
	a %= m;
	b %= m;
	return zaimoni::detail::fast_mod_sum(a,b,m);
}

inline unsigned long
fast_mod_subtract(unsigned long a, unsigned long b, unsigned long m)
{	return zaimoni::detail::fast_mod_subtract(a,b,m);	}

inline unsigned long
mod_subtract(unsigned long a, unsigned long b, unsigned long m)
{
	a %= m;
	b %= m;
	return zaimoni::detail::fast_mod_subtract(a,b,m);
}

}

#endif
