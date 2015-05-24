// bits/lite_alg.hpp
// do not include directly
// do ensure included from namespace zaimoni
// (C)2009 Kenneth Boyd, license: MIT.txt

namespace detail {

template<class T>
typename boost::enable_if<boost::is_same<unsigned long,T>, unsigned int>::type
nonstrictly_bounded_above_by_this_power_of_n(T x,T n)
{
	unsigned int power_bound = 0;
	while(1<x)
		{
		power_bound++;
		x /= n;
		}
	return power_bound;
}

template<class T>
typename boost::enable_if<boost::is_same<unsigned long,T>, unsigned long>::type
fast_mod_sum(T a,T b,T m)
{
	const T threshold = m-a;
	if (threshold>=b) return (a+b)%m;
	return b-(threshold);
}

template<class T>
typename boost::enable_if<boost::is_same<unsigned long,T>, unsigned long>::type
fast_mod_subtract(T a,T b,T m)
{
	if (a>=b) return a-b;
	return m-(b-a);
}

template<class T>
T
GCF_machine(T LHS, T RHS)
{
	if (LHS==RHS || RHS==0) return LHS;
	if (LHS==0) return RHS;
	if (LHS==1 || RHS==1) return T(1);
	do	if (LHS>RHS)
			{
			LHS %= RHS;
			if (LHS<=1)
				return (LHS==0) ? RHS : T(1);
			}
		else{
			RHS %= LHS;
			if (RHS<=1)
				return (RHS==0) ? LHS : T(1);
			}
	while(1);
}

template<class T>
T LCM_machine(T LHS, T RHS)
{
    if (LHS==0 || RHS==0) return T(0);

    LHS /= GCF_machine(LHS, RHS);
    LHS *= RHS;
    return LHS;
}

}
