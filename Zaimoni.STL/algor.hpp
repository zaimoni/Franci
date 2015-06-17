// algor.hpp
// algorithms header

#ifndef ZAIMONI_ALGOR_HPP
#define ZAIMONI_ALGOR_HPP 1

#include "Logging.h"
#include "boost_core.hpp"
#include "Repair.STL/algorithm"
#include <boost/concept_check.hpp>
#include "boost/functional.hpp"

namespace zaimoni
{
#include "bits/algor.hpp"

template<typename T, typename U,typename V>
void
_vector_op(T* LHS, const U* RHS, size_t Idx, V op)
{
	assert(NULL!=LHS);
	assert(NULL!=RHS);
	assert(0<Idx);
	do	{
		--Idx;
		op(LHS[Idx],RHS[Idx]);
		}
	while(0<Idx);
}

template<typename T, typename U,typename W,typename O1,typename O2>
void
_vector_op(T* LHS, const U* RHS, const W* RHS2, size_t Idx, O1 op, O2 op2)
{
	assert(LHS);
	assert(RHS);
	assert(RHS2);
	assert(0<Idx);
	do	{
		--Idx;
		op(LHS[Idx],op2(RHS[Idx],RHS2[Idx]));
		}
	while(0<Idx);
}

template<typename T, typename U,typename W,typename O1,typename O2>
void
_vector_op(T* LHS, const U& RHS, const W* RHS2, size_t Idx, O1 op, O2 op2)
{
	assert(LHS);
	assert(RHS);
	assert(RHS2);
	assert(0<Idx);
	do	{
		--Idx;
		op(LHS[Idx],op2(RHS,RHS2[Idx]));
		}
	while(0<Idx);
}

// for unary operations
template<typename T,typename V>
void
_elementwise_op(T* LHS, size_t Idx, V op)
{
	assert(LHS);
	assert(0<Idx);
	do	op(LHS[--Idx]);
	while(0<Idx);
}

// numerics for array arguments
template<typename T>
T _max(const T* const TargetArray, size_t Idx)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	assert(0<Idx);
	assert(TargetArray);
	T Tmp = TargetArray[--Idx];
	while(0<Idx)
		if (Tmp<TargetArray[--Idx])
			Tmp = TargetArray[Idx];
	return Tmp;
}

template<typename T, typename U, typename V>
V _max(const T* const TargetArray, size_t Idx, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	assert(0<Idx);
	assert(TargetArray);
	V Tmp = transform(TargetArray[--Idx]);
	while(0<Idx)
		{
		V Aux = transform(TargetArray[--Idx]);
		if (Tmp<Aux)
			Tmp = Aux;
		}
	return Tmp;
}

template<typename T, typename U, typename V, typename W, typename X>
V _max(const T* const TargetArray, const X* const TargetArray2, size_t Idx, V (&transform)(U,W))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	assert(0<Idx);
	assert(TargetArray);
	assert(TargetArray2);
	--Idx;
	V Tmp = transform(TargetArray[Idx],TargetArray2[Idx]);
	while(0<Idx)
		{
		--Idx;
		V Aux = transform(TargetArray[Idx],TargetArray2[Idx]);
		if (Tmp<Aux)
			Tmp = Aux;
		}
	return Tmp;
}

template<typename T>
T _min(const T* const TargetArray, size_t Idx)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	assert(0<Idx);
	assert(TargetArray);
	T Tmp = TargetArray[--Idx];
	while(0<Idx)
		if (TargetArray[--Idx]<Tmp)
			Tmp = TargetArray[Idx];
	return Tmp;
}

template<typename T, typename U, typename V>
V _min(const T* const TargetArray, size_t Idx, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	assert(0<Idx);
	assert(TargetArray);
	V Tmp = transform(TargetArray[--Idx]);
	while(0<Idx)
		{
		V Aux = transform(TargetArray[--Idx]);
		if (Tmp<Aux)
			Tmp = Aux;
		}
	return Tmp;
}

template<typename T, typename U, typename V, typename W, typename X>
V _min(const T* const TargetArray, const X* const TargetArray2, size_t Idx, V (&transform)(U,W))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	assert(0<Idx);
	assert(TargetArray);
	assert(TargetArray2);
	--Idx;
	V Tmp = transform(TargetArray[Idx],TargetArray2[Idx]);
	while(0<Idx)
		{
		--Idx;
		V Aux = transform(TargetArray[Idx],TargetArray2[Idx]);
		if (Tmp<Aux)
			Tmp = Aux;
		}
	return Tmp;
}

// sorts
// sort nets
template<typename T>
void
sort_net_2(T* TargetArray)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (TargetArray[1]<TargetArray[0])
		swap(TargetArray[0],TargetArray[1]);
}

template<typename T, typename U, typename V>
void
sort_net_2(T* TargetArray, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (transform(TargetArray[1])<transform(TargetArray[0]))
		swap(TargetArray[0],TargetArray[1]);
}

template<typename T, typename U>
typename std::enable_if<boost::is_convertible<T, U>::value, void>::type
sort_net_2(T* TargetArray, bool (&less_than)(U,U))
{
	if (less_than(TargetArray[1],TargetArray[0]))
		swap(TargetArray[0],TargetArray[1]);
}

// pointer to pointer versions
template<typename T>
void
sort_net_2(T** TargetArray)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (*TargetArray[1]<*TargetArray[0])
		swap(*TargetArray[0],*TargetArray[1]);
}

template<typename T, typename U, typename V>
void
sort_net_2(T** TargetArray, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (transform(*TargetArray[1])<transform(*TargetArray[0]))
		swap(TargetArray[0],TargetArray[1]);
}

template<typename T, typename U>
typename std::enable_if<boost::is_convertible<T, U>::value, void>::type
sort_net_2(T** TargetArray, bool (&less_than)(U,U))
{
	if (less_than(*TargetArray[1],*TargetArray[0]))
		swap(TargetArray[0],TargetArray[1]);
}

// heapsorts
// O(n*log(n)), but also o(n*log(n))
// not order preserving
// inplace
// increasing sort version
template<typename T>
void
heapsort(T* const TargetArray, size_t N)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray || 1==N) return;
	if (2==N)
		{
		sort_net_2(TargetArray);
		return;
		}

	// build the heap
	signed long v = N/2-1;
	do zaimoni::detail::downheap(TargetArray,v,N);
	while(0<=v--);

	// finish the heapsort
	while(1<N)
		{
		swap(TargetArray[0],TargetArray[--N]);
		zaimoni::detail::downheap(TargetArray,0,N);
		}
}

template<typename T, typename U, typename V>
void
heapsort(T* const TargetArray, size_t N, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (!TargetArray || 1==N) return;
	if (2==N)
		{
		sort_net_2(TargetArray,transform);
		return;
		}

	// build the heap
	signed long v = N/2-1;
	do zaimoni::detail::downheap(TargetArray,v,N,transform);
	while(0<=v--);

	// finish the heapsort
	while(1<N)
		{
		swap(TargetArray[0],TargetArray[--N]);
		zaimoni::detail::downheap(TargetArray,0,N,transform);
		}
}

template<typename T, typename U>
typename std::enable_if<boost::is_convertible<T, U>::value, void>::type
heapsort(T* const TargetArray, size_t N, bool (&less_than)(U,U))
{
	if (!TargetArray || 1==N) return;
	if (2==N)
		{
		sort_net_2(TargetArray,less_than);
		return;
		}

	// build the heap
	signed long v = N/2-1;
	do zaimoni::detail::downheap(TargetArray,v,N,less_than);
	while(0<=v--);

	// finish the heapsort
	while(1<N)
		{
		swap(TargetArray[0],TargetArray[--N]);
		zaimoni::detail::downheap(TargetArray,0,N,less_than);
		}
}

template<typename T>
void
heapsort(T* const TargetArray)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray) return;
	heapsort(TargetArray,ArraySize(TargetArray));
}

template<typename T, typename U, typename V>
void
heapsort(T* const TargetArray, V (&transform)(U))
{
	if (!TargetArray) return;
	heapsort(TargetArray,ArraySize(TargetArray),transform);
}

template<typename T, typename U>
typename std::enable_if<boost::is_convertible<T, U>::value, void>::type
heapsort(T* const TargetArray, bool (&less_than)(U,U))
{
	if (!TargetArray) return;
	heapsort(TargetArray,ArraySize(TargetArray),less_than);
}

// pointer-to-pointer versions
template<typename T>
void
heapsort(T** const TargetArray, size_t N)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray || 1==N) return;
	if (2==N)
		{
		sort_net_2(TargetArray);
		return;
		}

	// build the heap
	signed long v = N/2-1;
	do zaimoni::detail::downheap(TargetArray,v,N);
	while(0<=v--);

	// finish the heapsort
	while(1<N)
		{
		swap(TargetArray[0],TargetArray[--N]);
		zaimoni::detail::downheap(TargetArray,0,N);
		}
}

template<typename T, typename U, typename V>
void
heapsort(T** const TargetArray, size_t N, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (!TargetArray || 1==N) return;
	if (2==N)
		{
		sort_net_2(TargetArray,transform);
		return;
		}

	// build the heap
	signed long v = N/2-1;
	do zaimoni::detail::downheap(TargetArray,v,N,transform);
	while(0<=v--);

	// finish the heapsort
	while(1<N)
		{
		swap(TargetArray[0],TargetArray[--N]);
		zaimoni::detail::downheap(TargetArray,0,N,transform);
		}
}

template<typename T, typename U>
typename std::enable_if<boost::is_convertible<T, U>::value, void>::type
heapsort(T** const TargetArray, size_t N, bool (&less_than)(U,U))
{
	if (!TargetArray || 1==N) return;
	if (2==N)
		{
		sort_net_2(TargetArray,less_than);
		return;
		}

	// build the heap
	signed long v = N/2-1;
	do zaimoni::detail::downheap(TargetArray,v,N,less_than);
	while(0<=v--);

	// finish the heapsort
	while(1<N)
		{
		swap(TargetArray[0],TargetArray[--N]);
		zaimoni::detail::downheap(TargetArray,0,N,less_than);
		}
}

template<typename T>
void
heapsort(T** const TargetArray)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray) return;
	heapsort(TargetArray,ArraySize(TargetArray));
}

template<typename T, typename U, typename V>
void
heapsort(T** const TargetArray, V (&transform)(U))
{
	if (!TargetArray) return;
	heapsort(TargetArray,ArraySize(TargetArray),transform);
}

template<typename T, typename U>
typename std::enable_if<boost::is_convertible<T, U>::value, void>::type
heapsort(T** const TargetArray, bool (&less_than)(U,U))
{
	if (!TargetArray) return;
	heapsort(TargetArray,ArraySize(TargetArray),less_than);
}

// one iteration of bubble sort
template<typename T>
void
bubble_up(T* const TargetArray, size_t Idx, size_t N)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray) return;
	while(++Idx<N && TargetArray[Idx]<TargetArray[Idx-1])
		swap(TargetArray[Idx-1],TargetArray[Idx]);
}

template<typename T, typename U, typename V>
void
bubble_up(T* const TargetArray, size_t Idx, size_t N, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (!TargetArray) return;
	const V Tmp = transform(TargetArray[Idx]);
	while(++Idx<N && transform(TargetArray[Idx])<Tmp)
		swap(TargetArray[Idx-1],TargetArray[Idx]);
}

template<typename T, typename U>
void
bubble_up(T* const TargetArray, size_t Idx, size_t N, bool (&less_than)(U,U))
{
	if (!TargetArray) return;
	while(++Idx<N && less_than(TargetArray[Idx],TargetArray[Idx-1]))
		swap(TargetArray[Idx-1],TargetArray[Idx]);
}

template<typename T>
void
bubble_down(T* const TargetArray, size_t Idx)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray) return;
	while(0<Idx && TargetArray[Idx]<TargetArray[Idx-1])
		{
		swap(TargetArray[Idx-1],TargetArray[Idx]);
		--Idx;
		}
}

template<typename T, typename U, typename V>
void
bubble_down(T* const TargetArray, size_t Idx, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (!TargetArray) return;
	const V Tmp = transform(TargetArray[Idx]);
	while(0<Idx && Tmp<transform(TargetArray[Idx-1]))
		{
		swap(TargetArray[Idx-1],TargetArray[Idx]);
		--Idx;
		}
}

template<typename T, typename U>
void
bubble_down(T* const TargetArray, size_t Idx, bool (&less_than)(U,U))
{
	if (!TargetArray) return;
	while(0<Idx && less_than(TargetArray[Idx],TargetArray[Idx-1]))
		{
		swap(TargetArray[Idx-1],TargetArray[Idx]);
		--Idx;
		}
}

// searches
template<typename T>
signed long
supremum_search_sorted(const T* TargetArray, size_t StrictUB, typename boost::call_traits<T>::param_type supremum)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray) return -1;
	if (supremum<TargetArray[0]) return -1;
	
	size_t LowIdx = 0;
	size_t HighIdx = StrictUB-1;
	while(LowIdx<HighIdx)
		{
		size_t MidIdx = (HighIdx-LowIdx)/2+LowIdx+1;
		if (supremum<TargetArray[MidIdx])
			{			
			HighIdx = MidIdx-1;
			}
		else{
			LowIdx = MidIdx;
			}
		}
	return LowIdx;
}

template<typename T, typename U, typename V>
signed long
supremum_search_sorted(const T* TargetArray, size_t StrictUB, typename boost::call_traits<V>::param_type supremum, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (!TargetArray) return -1;
	if (supremum<transform(TargetArray[0])) return -1;
	
	size_t LowIdx = 0;
	size_t HighIdx = StrictUB-1;
	while(LowIdx<HighIdx)
		{
		size_t MidIdx = (HighIdx-LowIdx)/2+LowIdx+1;
		if (supremum<transform(TargetArray[MidIdx]))
			{			
			HighIdx = MidIdx-1;
			}
		else{
			LowIdx = MidIdx;
			}
		}
	return LowIdx;
}

template<typename T, typename U, typename V, typename W>
signed long
supremum_search_sorted(const T* TargetArray, size_t StrictUB, const U& supremum,bool (&less_than)(V,W))
{
	if (!TargetArray) return -1;
	if (less_than(supremum,TargetArray[0])) return -1;
	
	size_t LowIdx = 0;
	size_t HighIdx = StrictUB-1;
	while(LowIdx<HighIdx)
		{
		size_t MidIdx = (HighIdx-LowIdx)/2+LowIdx+1;
		if (less_than(supremum,TargetArray[MidIdx]))
			{			
			HighIdx = MidIdx-1;
			}
		else{
			LowIdx = MidIdx;
			}
		}
	return LowIdx;
}

template<typename T>
signed long
strict_supremum_search_sorted(const T* TargetArray, size_t StrictUB, typename boost::call_traits<T>::param_type supremum)
{
	boost::function_requires<boost::LessThanComparableConcept<T> >();
	if (!TargetArray) return -1;
	if (supremum<TargetArray[0]) return -1;
	
	size_t LowIdx = 0;
	size_t HighIdx = StrictUB-1;
	while(LowIdx<HighIdx)
		{
		size_t MidIdx = (HighIdx-LowIdx)/2+LowIdx+1;
		if (TargetArray[MidIdx]<supremum)
			{			
			LowIdx = MidIdx;
			}
		else{
			HighIdx = MidIdx-1;
			}
		}
	return LowIdx;
}

template<typename T, typename U, typename V>
signed long
strict_supremum_search_sorted(const T* TargetArray, size_t StrictUB, typename boost::call_traits<V>::param_type supremum, V (&transform)(U))
{
	boost::function_requires<boost::LessThanComparableConcept<V> >();
	if (!TargetArray) return -1;
	if (supremum<transform(TargetArray[0])) return -1;
	
	size_t LowIdx = 0;
	size_t HighIdx = StrictUB-1;
	while(LowIdx<HighIdx)
		{
		size_t MidIdx = (HighIdx-LowIdx)/2+LowIdx+1;
		if (transform(TargetArray[MidIdx])<supremum)
			{			
			LowIdx = MidIdx;
			}
		else{
			HighIdx = MidIdx-1;
			}
		}
	return LowIdx;
}

// oops, inconsistent placement of supremum in less_than calls.  Eliminate a degree of freedom.
template<typename T, typename U, typename V>
signed long
strict_supremum_search_sorted(const T* TargetArray, size_t StrictUB, const U& supremum, bool (&less_than)(V,V))
{
	if (!TargetArray) return -1;
	if (less_than(supremum,TargetArray[0])) return -1;
	
	size_t LowIdx = 0;
	size_t HighIdx = StrictUB-1;
	while(LowIdx<HighIdx)
		{
		size_t MidIdx = (HighIdx-LowIdx)/2+LowIdx+1;
		if (less_than(TargetArray[MidIdx],supremum))
			{			
			LowIdx = MidIdx;
			}
		else{
			HighIdx = MidIdx-1;
			}
		}
	return LowIdx;
}

template<typename T,typename U>
inline bool
arraydetect(const T* WorkSpace, size_t StrictUB, const U& Test)
{
	do	if (Test(WorkSpace[--StrictUB]))
			return true;
	while(0<StrictUB);
	return false;
}

template<typename T,typename W,typename U>
inline bool
arraydetect(const T* LHS_WorkSpace, const W* RHS_WorkSpace, size_t StrictUB, const U& Test)
{
	do	{
		--StrictUB;
		if (Test(LHS_WorkSpace[StrictUB],RHS_WorkSpace[StrictUB]))
			return true;
		}
	while(0<StrictUB);
	return false;
}

template<typename T, typename U, typename V>
bool
boolean_search(const T* TargetArray, const size_t StrictUB, V (&transform)(U), size_t& Idx1)
{
	assert(0<StrictUB);
	assert(TargetArray);
	size_t HighIdx = StrictUB;
	do	if (transform(TargetArray[--HighIdx]))
			{
			Idx1 = HighIdx;
			return true;
			}
	while(0<HighIdx);
	return false;
}

// symmetric versions guarantee Idx1<Idx2
template<typename T, typename U>
bool
symmetric_boolean_search(const T* TargetArray, const size_t StrictUB, bool (&less_than)(U,U), size_t& Idx1, size_t& Idx2)
{
	assert(1<StrictUB);
	assert(TargetArray);
	size_t HighIdx = StrictUB;
	do	{
		size_t LowIdx = --HighIdx;
		do	if (less_than(TargetArray[--LowIdx],TargetArray[HighIdx]))
				{
				Idx1 = LowIdx;
				Idx2 = HighIdx;
				return true;
				}
		while(0<LowIdx);		
		}
	while(1<HighIdx);
	return false;
}

template<typename T, typename U, typename V, typename W>
bool
symmetric_boolean_search(const T* TargetArray, const size_t StrictUB, bool (&less_than)(U,U), W (&RHStransform)(V), size_t& Idx1, size_t& Idx2)
{
	assert(1<StrictUB);
	assert(TargetArray);
	size_t HighIdx = StrictUB;
	do	if (RHStransform(TargetArray[--HighIdx]))
			{
			size_t LowIdx = HighIdx;
			do	if (less_than(TargetArray[--LowIdx],TargetArray[HighIdx]))
					{
					Idx1 = LowIdx;
					Idx2 = HighIdx;
					return true;
					}
			while(0<LowIdx);
			}
	while(1<HighIdx);
	return false;
}

template<typename T, typename U>
bool
boolean_search(const T* TargetArray, const size_t StrictUB, bool (&less_than)(U,U), size_t& Idx1, size_t& Idx2)
{
	assert(0<StrictUB);
	assert(TargetArray);
	size_t HighIdx = StrictUB;
	do	{
		--HighIdx;
		size_t LowIdx = StrictUB;
		do	if (   --LowIdx!=HighIdx
				&& less_than(TargetArray[LowIdx],TargetArray[HighIdx]))
				{
				Idx1 = LowIdx;
				Idx2 = HighIdx;
				return true;
				}
		while(0<LowIdx);
		}
	while(1<HighIdx);
	return false;
}

template<typename T, typename U, typename V, typename W>
bool
boolean_search(const T* TargetArray, const size_t StrictUB, bool (&less_than)(U,U), W (&RHStransform)(V), size_t& Idx1, size_t& Idx2)
{
	assert(1<StrictUB);
	assert(TargetArray);
	size_t HighIdx = StrictUB;
	do	if (RHStransform(TargetArray[--HighIdx]))
			{
			size_t LowIdx = StrictUB;
			do	if (   --LowIdx!=HighIdx
					&& less_than(TargetArray[LowIdx],TargetArray[HighIdx]))
					{
					Idx1 = LowIdx;
					Idx2 = HighIdx;
					return true;
					}
			while(0<LowIdx);
			}
	while(1<HighIdx);
	return false;
}

}	// namespace zaimoni

#endif
