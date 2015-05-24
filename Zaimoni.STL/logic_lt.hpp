// logic_lt.hpp
// logical less than
// (C)2009 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_STL_LOGIC_LT_HPP
#define ZAIMONI_STL_LOGIC_LT_HPP 1

#include <cstring>

namespace zaimoni {

// template for total sorting order (defaults to operator <, hook for sorting on types without <)
template<class T,class U>
inline bool
_logical_less_than(const T& LHS,const U& RHS)
{return LHS<RHS;}

template<class T,class U>
bool
_logical_less_than(const T* LHS,const U* RHS)
{	// bubble NULL up
	if (NULL==RHS) return NULL!=LHS;
	if (NULL==LHS) return false;
	return _logical_less_than(*LHS,*RHS);
}

// force lexical ordering for strings
// inline to prevent multiple definitions
template<>
inline bool
_logical_less_than<char,char>(const char* LHS,const char* RHS)
{	// bubble NULL up
	if (NULL==RHS) return NULL!=LHS;
	if (NULL==LHS) return false;
	return -1==strcmp(LHS,RHS);
}

template<class T,class U>
bool
_logical_vector_less_than(const T* LHS,const U* RHS,size_t Idx)
{
	size_t SweepIdx = 0;
	while(SweepIdx<Idx)
		{
		if (LHS[SweepIdx]!=RHS[SweepIdx]) return _logical_less_than(LHS[SweepIdx],RHS[SweepIdx]);
		++SweepIdx;
		}
	return true;
}

}

#endif
