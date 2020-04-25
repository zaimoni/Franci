// c_array.hpp
// (C)2009,2011 Kenneth Boyd, license: MIT.txt

// base types to signal that a class type is meant to be used like a C array.

// c_array<T>: potentially dynamic-length
// static_c_array<T,N>: length fixed at compile time.  loosely modeled on Boost.

#ifndef ZAIMONI_STL_METATYPE_C_ARRAY_HPP
#define ZAIMONI_STL_METATYPE_C_ARRAY_HPP 1

#include "../Logging.h"
#include "../MetaRAM.hpp"
#include <algorithm>
#include "../logic_lt.hpp"
#include "operator.hpp"

namespace zaimoni {

// pull these into namespace zaimoni to keep Koenig lookup happy (GCC implementations OK)
using std::swap;
using std::max;
using std::min;

}	// namespace zaimoni

// this macro goes in the class body
// turns out the STL type definition glue doesn't inherit properly (in GCC)
// there are no arrays of references, fortunately...
#define ZAIMONI_STL_TYPE_GLUE_ARRAY(TYPE)	\
	typedef T			value_type;	\
	typedef T*			iterator;	\
	typedef const T*	const_iterator;	\
	typedef T&			reference;	\
	typedef const T&	const_reference;	\
	typedef size_t		size_type;	\
	typedef ptrdiff_t	difference_type;	\
	typedef std::reverse_iterator<iterator> reverse_iterator;	\
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator

// this macro goes in the class body
#define ZAIMONI_C_ARRAY_HELPER_CORE(PTR,TYPE,SIZE)	\
	const TYPE* data() const {return PTR;};	\
	TYPE* c_array() {return PTR;};	\
	\
	TYPE* begin() { return PTR; }	\
	const TYPE* begin() const { return PTR; }	\
	TYPE* end() { return (empty()) ? NULL : PTR+SIZE; }	\
	const TYPE* end() const { return (empty()) ? NULL : PTR+SIZE; }	\
	\
	TYPE& front() {return PTR[0];};	\
	const TYPE& front() const {return PTR[0];};	\
	TYPE& back() {return PTR[SIZE-1];};	\
	const TYPE& back() const {return PTR[SIZE-1];};	\
	\
	typedef std::reverse_iterator<iterator> reverse_iterator;	\
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;	\
	\
	reverse_iterator rbegin() { return reverse_iterator(end()); };	\
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); };	\
	reverse_iterator rend() { return reverse_iterator(begin()); };	\
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); };

namespace zaimoni {

template<class T>
struct c_array
{
	// type should also provide the following STL glue.  Cf. ZAIMONI_C_ARRAY_HELPER macro.
	// T& operator[](size_t Idx)
	// const T& operator[](size_t Idx) const
	// size_t size() const
	// const T* data() const
	// T* c_array()	

	// bool Resize(size_t Idx);
	// void resize(size_t Idx);
	// template<size_t Idx> void resize() {resize(Idx);};

	// bool empty() is useful, but need not have the usual semantics in the static case.
};

// use Curiously Recurring Template Pattern
// requires: data(), c_array(), empty(), size()
template<class Derived, class T>
struct c_array_CRTP : public c_array<T>
{
	// STL iterator support
	T* begin() {return static_cast<Derived*>(this)->c_array();};
	const T* begin() const {return static_cast<const Derived*>(this)->data();};
	T* end() {return (static_cast<Derived*>(this)->empty()) ? NULL : static_cast<Derived*>(this)->c_array()+static_cast<Derived*>(this)->size();};
	const T* end() const {return (static_cast<const Derived*>(this)->empty()) ? NULL : static_cast<const Derived*>(this)->data()+static_cast<const Derived*>(this)->size();};

	T& front() {return *begin();};
	const T& front() const {return *begin();};
	T& back() {return *(end()-1);};
	const T& back() const {return *(end()-1);};

	// STL reverse iterator support
	std::reverse_iterator<T*> rbegin() { return end(); };
	std::reverse_iterator<const T*> rbegin() const { return end(); };
	std::reverse_iterator<T*> rend() { return begin(); };
	std::reverse_iterator<const T*> rend() const { return begin(); };

	// assign one value to all elements
	void assign(typename boost::call_traits<T>::param_type value)
	{	_vector_assign(begin(),value,static_cast<Derived*>(this)->size());	};
};

// not a true aggregate type, since it has a base class.  Use boost::array if that is really needed.
template<class T,size_t N>
struct static_c_array : public c_array_CRTP<static_c_array<T,N>, T>
{
	BOOST_STATIC_ASSERT(1<=N);
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	T x_i[N];	// the array

	static_c_array() {};
	// static_c_array(const static_c_array& src);	// ok
	// ~static_c_array();	// ok
	// static_c_array& operator=(const static_c_array& src);	// ok

	// default-initialize to given value
	explicit static_c_array(typename boost::call_traits<T>::param_type src) {this->assign(src);};
	// default-initialize from another static_c_array
	template<class U> explicit static_c_array(const static_c_array<U,N>& src) {std::copy_n(src.data(),N,x_i);}
	// default-initialize from iterator
	template<class OutIterator> explicit static_c_array(OutIterator src) {std::copy_n(src,N,x_i);}

	// assignment with type conversion
	template<class T2>
	static_c_array<T,N>& operator=(const static_c_array<T2,N>& src)
		{	_copy_buffer(this->begin(),src.begin(),N);
			return *this;
		}
	template<class OutIterator> static_c_array<T,N>& operator=(OutIterator src)
		{	// T* is classic, but weirder forward iterators will have weirder semantics
			// copy rather than const reference because we use it
			std::copy_n(src,N,x_i);
			return *this;
		}

	void swap(static_c_array& y)
	{	std::swap_ranges(this->begin(),this->end(),y.begin());	};

	T& operator[](size_t Idx) {return x_i[Idx];};
	const T& operator[](size_t Idx) const {return x_i[Idx];};

	T* c_array() {return x_i;};
	const T* data() const {return x_i;};

	static size_t size() {return N;};	
	static size_t max_size() {return N;};	
	static bool empty() {return false;};	
    enum { static_size = N };

	bool Resize(size_t Idx) const {return N==Idx;};
	void resize(size_t Idx) const {if (N!=Idx) FATAL("can't resize static_c_array");};
	template<size_t Idx> void resize() const {BOOST_STATIC_ASSERT(N==Idx);}
};

template<class T,size_t N,class U>
bool operator==(const static_c_array<T,N>& lhs,const static_c_array<U,N>& rhs)
{return _vector_equal(lhs.begin(),rhs.begin(),N);}

template<class T,size_t N,class U>
bool operator!=(const static_c_array<T,N>& lhs,const static_c_array<U,N>& rhs)
{return !(lhs==rhs);}

// optional comparisons
template<class T, class U, size_t N>
inline bool
operator<(const static_c_array<T,N>& x, const static_c_array<U,N>& y)
{	return _logical_vector_less_than(x.begin(),y.begin(),N);	}

#define ZAIMONI_CROSS_TEMPLATE template<class T, class T2, size_t N>
#define ZAIMONI_CROSS_STATIC_C_ARRAY static_c_array<T,N>
#define ZAIMONI_CROSS_STATIC_C_ARRAY2 static_c_array<T2,N>
ZAIMONI_CROSSSUBTYPE_TOTAL_ORDERING(ZAIMONI_CROSS_TEMPLATE,ZAIMONI_CROSS_STATIC_C_ARRAY,ZAIMONI_CROSS_STATIC_C_ARRAY2)
#undef ZAIMONI_CROSS_TEMPLATE
#undef ZAIMONI_CROSS_STATIC_C_ARRAY
#undef ZAIMONI_CROSS_STATIC_C_ARRAY2

// global swap()
template<class T, size_t N>
inline void swap(static_c_array<T,N>& x, static_c_array<T,N>& y)
{	x.swap(y);	}

}	// namespace zaimoni

#endif
