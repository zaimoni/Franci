// bits/algor.hpp
// do not include directly
// include from namespace zaimoni

namespace detail {

template<typename T>
void downheap(T* const TargetArray, size_t v, size_t N) requires requires() { TargetArray[0] < TargetArray[1]; }
{
	size_t w = 2*v+1;
	while(w<N)
		{
		if (w+1<N && TargetArray[w]<TargetArray[w+1]) w++;	// force maximum label
		if (!(TargetArray[v]<TargetArray[w])) return;	// v has heap property
		swap(TargetArray[v],TargetArray[w]);
		v = w;
		w = 2*v+1;
		}
}

template<typename T, typename U, typename V>
void downheap(T* const TargetArray,size_t v,size_t N, V (&transform)(U)) requires requires() { transform(TargetArray[0]) < transform(TargetArray[1]); }
{
	size_t w = 2*v+1;
	while(w<N)
		{
		if (w+1<N && transform(TargetArray[w]<transform(TargetArray[w+1]))) w++;	// force maximum label
		if (!(transform(TargetArray[v])<transform(TargetArray[w]))) return;			// v has heap property
		swap(TargetArray[v],TargetArray[w]);
		v = w;
		w = 2*v+1;
		}
}

template<typename T, typename U>
std::enable_if_t<std::is_convertible_v<T, U>, void>
downheap(T* const TargetArray,size_t v,size_t N, bool (&less_than)(U,U))
{
	size_t w = 2*v+1;
	while(w<N)
		{
		if (w+1<N && less_than(TargetArray[w],TargetArray[w+1])) w++;	// force maximum label
		if (!(less_than(TargetArray[v],TargetArray[w]))) return;		// v has heap property
		swap(TargetArray[v],TargetArray[w]);
		v = w;
		w = 2*v+1;
		}
}

// pointer to pointer versions
template<typename T>
void
downheap(T** const TargetArray,size_t v,size_t N)
{
	size_t w = 2*v+1;
	while(w<N)
		{
		if (w+1<N && *TargetArray[w]<*TargetArray[w+1]) w++;	// force maximum label
		if (!(*TargetArray[v]<*TargetArray[w])) return;	// v has heap property
		swap(TargetArray[v],TargetArray[w]);
		v = w;
		w = 2*v+1;
		}
}

template<typename T, typename U, typename V>
void downheap(T** const TargetArray,size_t v,size_t N, V (&transform)(U)) requires requires() { transform(TargetArray[0]) < transform(TargetArray[1]); }
{
	size_t w = 2*v+1;
	while(w<N)
		{
		if (w+1<N && transform(*TargetArray[w]<transform(*TargetArray[w+1]))) w++;	// force maximum label
		if (!(transform(*TargetArray[v])<transform(*TargetArray[w]))) return;			// v has heap property
		swap(TargetArray[v],TargetArray[w]);
		v = w;
		w = 2*v+1;
		}
}

template<typename T, typename U>
std::enable_if_t<std::is_convertible_v<T, U>, void>
downheap(T** const TargetArray,size_t v,size_t N, bool (&less_than)(U,U))
{
	size_t w = 2*v+1;
	while(w<N)
		{
		if (w+1<N && less_than(*TargetArray[w],*TargetArray[w+1])) w++;	// force maximum label
		if (!(less_than(*TargetArray[v],*TargetArray[w]))) return;		// v has heap property
		swap(TargetArray[v],TargetArray[w]);
		v = w;
		w = 2*v+1;
		}
}

}	// namespace detail
