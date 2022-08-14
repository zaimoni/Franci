// MetaRAM2.hpp
// more C++ memory interface functions
// these require the Iskandria memory manager
// (C)2009,2010,2015,2020,2022 Kenneth Boyd, license: LICENSE.md

#ifndef ZAIMONI_METARAM2_HPP
#define ZAIMONI_METARAM2_HPP 1

#include "MetaRAM.hpp"
#ifdef ZAIMONI_HAVE_ACCURATE_MSIZE
#include "z_memory.h"
#endif
#include "Logging.h"
#include "Augment.STL/algorithm"

#ifdef ZAIMONI_FORCE_ISO
#define ZAIMONI_NONISO_ISO_PARAM(A, B) , B
#define ZAIMONI_ISO_PARAM(A) , A
#define ZAIMONI_ISO_SRC(A) A
#define ZAIMONI_NONISO_SRC(A)
#define ZAIMONI_NONISO_ISO_SRC(A, B) B
#else
#define ZAIMONI_NONISO_ISO_PARAM(A, B) , A
#define ZAIMONI_ISO_PARAM(A)
#define ZAIMONI_ISO_SRC(A)
#define ZAIMONI_NONISO_SRC(A) A
#define ZAIMONI_NONISO_ISO_SRC(A, B) A
#endif

namespace zaimoni	{

template <class T> void
BLOCKDELETEARRAY(T**& Target)
{
	autotransform(Target,Target+ArraySize(Target),DELETE<T>);
	DELETEARRAY(Target);
}

template <class T> void
BLOCKDELETEARRAY_AND_NULL(T**& Target)
{
	autotransform(Target,Target+ArraySize(Target),DELETE<T>);
	DELETEARRAY_AND_NULL(Target);
}

template <class T> void
BLOCKFREEARRAY(T**& Target)
{
	autotransform(Target,Target+ArraySize(Target),DELETE<T>);
	FREE(Target);
}

template <class T> void
BLOCKFREEARRAY_AND_NULL(T**& Target)
{
	autotransform(Target,Target+ArraySize(Target),DELETE<T>);
	FREE_AND_NULL(Target);
}

template<typename T>
void FlushNULLFromArray(T**& _ptr, size_t StartIdx)
{	/* FORMALLY CORRECT: Kenneth Boyd, 11/12/2004 */\
	assert(_ptr);
	const size_t Limit = ArraySize(_ptr);
	while(StartIdx<Limit)
		{
		if (!_ptr[StartIdx++])
			{
			size_t Offset = 1;
			StartIdx--;
			while(StartIdx+Offset<Limit)
				if (!_ptr[StartIdx+Offset])
					Offset++;
				else{
					_ptr[StartIdx] = _ptr[StartIdx+Offset];
					StartIdx++;
					};
			_ptr = REALLOC(_ptr,sizeof(T*)*(Limit-Offset));
			return;
			}
		}
}

// indirection glue

// _new_buffer_nonNULL and _flush have to be synchronized for ISO C++
// _new_buffer and _new_buffer_nonNULL_throws are in MetaRAM.hpp (they don't depend on Logging.h)
template<typename T>
T* _new_buffer_nonNULL(size_t i)
{
	T* tmp = (std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>) ? reinterpret_cast<T*>(calloc(i,sizeof(T))) : new(std::nothrow) T[i];
	if (!tmp) _fatal("Irrecoverable failure to allocate memory");
	return tmp;
}

template<typename T>
typename std::enable_if<!(std::is_trivially_destructible_v<T> && std::is_trivially_copy_assignable_v<T>), void>::type
_delete_idx(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t i)
{
	assert(_ptr ZAIMONI_ISO_SRC(&& 0 < _ptr_size));
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#endif
	if (1==_ptr_size)
		{
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
		}
	T* Tmp = _new_buffer_nonNULL<T>(_ptr_size-1);
	if (i+1<_ptr_size)
		_copy_expendable_buffer(Tmp+i,_ptr+i+1,_ptr_size-(i+1));
	if (0<i)
		_copy_expendable_buffer(Tmp,_ptr,i);
	_flush(_ptr);
	_ptr = Tmp;		
#ifdef ZAIMONI_FORCE_ISO
	--_ptr_size;
#endif
}

template<typename T>
void _flush(T* _ptr)
{
	if constexpr (std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>) {
		free(_ptr);
	} else {
		delete[] _ptr;
	}
}

template<typename T>
void _flush(T** _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size))
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0<_ptr_size : 0==_ptr_size);
#endif
	if (_ptr)
		{
		size_t i = ZAIMONI_NONISO_ISO_SRC(ArraySize(_ptr), _ptr_size);
		do	_single_flush(_ptr[--i]);
		while(0<i);
		free(_ptr);
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		}
}

template<typename T> requires(!(std::is_pointer_v<T> || std::is_member_pointer_v<T>))
void _value_copy_buffer(T* dest,const T* src, size_t Idx) {_copy_buffer(dest,src,Idx);}

template<typename T, typename U>
void _value_copy_buffer(U** dest, const T* const* src, size_t Idx) requires requires(U* _dest, const T* _src) { _src->CopyInto(_dest); }
{
	do {
		if (src[--Idx]) src[Idx]->CopyInto(dest[Idx]);
		else {
			_flush(dest[Idx]);
			dest[Idx] = 0;
		}
	} while (0 < Idx);
}

template<typename T>
void _value_copy_buffer(T** dest, const T* const* src, size_t Idx) requires requires(T* _dest, const T* _src) { _src->CopyInto(_dest); }
{
	do {
		if (src[--Idx]) src[Idx]->CopyInto(dest[Idx]);
		else {
			_flush(dest[Idx]);
			dest[Idx] = nullptr;
		}
	} while (0 < Idx);
}

template<typename T>
void _value_copy_buffer(T** dest, const T* const* src, size_t Idx) requires (!requires(T* _dest, const T* _src) { _src->CopyInto(_dest); } && requires(T* _dest, const T* _src) { CopyInto(*_src, _dest); })
{
	do {
		if (src[--Idx]) CopyInto(*src[Idx], dest[Idx]);
		else {
			_flush(dest[Idx]);
			dest[Idx] = nullptr;
		}
	} while (0 < Idx);
}

template<typename T>
typename std::enable_if<std::is_trivially_destructible_v<T>&& std::is_trivially_copy_assignable_v<T>, void>::type
_delete_idx(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t i)
{
	assert(_ptr);
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#else
	assert(0<_ptr_size);
#endif
	assert(i < _ptr_size);
	if (1==_ptr_size)
		{
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
		}
	if (2<=_ptr_size-i)
		memmove(_ptr+i,_ptr+i+1,sizeof(*_ptr)*(_ptr_size-i-1));
	_ptr = REALLOC(_ptr, sizeof(*_ptr) * ZAIMONI_NONISO_ISO_SRC((_ptr_size - 1), --_ptr_size));
}

template<typename T>
inline void _safe_delete_idx(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t i)
{
	if (_ptr && i < ZAIMONI_NONISO_ISO_SRC(ArraySize(_ptr), _ptr_size)) _delete_idx(_ptr ZAIMONI_ISO_PARAM(_ptr_size), i);
}

template<typename T>
void _weak_delete_idx(T**& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t i)
{
	assert(_ptr);
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#endif
	assert(i < _ptr_size);
	if (1 == _ptr_size)
	{
		_weak_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
	}
	if (2 <= _ptr_size - i)
		memmove(_ptr + i, _ptr + i + 1, sizeof(T*) * (_ptr_size - i - 1));
	_ptr = REALLOC(_ptr, sizeof(T*) * (_ptr_size - 1));
#ifdef ZAIMONI_FORCE_ISO
	--_ptr_size;
#endif
}

template<typename T>
void _safe_weak_delete_idx(T*& __ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t i)
{
	if (__ptr && i < ZAIMONI_NONISO_ISO_SRC(ArraySize(__ptr), _ptr_size)) _weak_delete_idx(__ptr ZAIMONI_ISO_PARAM(_ptr_size), i);
}

// How to tell difference between T* (single) and T* (array) in resize/shrink?
// we don't, assume single

template<typename T>
inline typename std::enable_if<std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>, T*>::type
_new_buffer_uninitialized(size_t i)
{
	if (((size_t)(-1))/sizeof(T)<i) return 0; // CERT C MEM07
	return reinterpret_cast<T*>(malloc(i*sizeof(T)));
}

template<typename T>
inline typename std::enable_if<std::is_trivially_constructible_v<T>&& std::is_trivially_destructible_v<T>, T*>::type
_new_buffer_uninitialized_nonNULL(size_t i)
{
	if (((size_t)(-1))/sizeof(T)<i) // CERT C MEM07
		_fatal("requested memory exceeds SIZE_T_MAX");
	T* tmp = reinterpret_cast<T*>(malloc(i*sizeof(T)));
	if (!tmp) _fatal("Irrecoverable failure to allocate memory");
	return tmp;
}

template<typename T>
inline typename std::enable_if<std::is_trivially_constructible_v<T>&& std::is_trivially_destructible_v<T>, T*>::type
_new_buffer_uninitialized_nonNULL_throws(size_t i)
{
	if (((size_t)(-1))/sizeof(T)<i) throw std::bad_alloc(); // CERT C MEM07
	T* tmp = reinterpret_cast<T*>(malloc(i*sizeof(T)));
	if (!tmp) throw std::bad_alloc();
	return tmp;
}

#ifndef ZAIMONI_FORCE_ISO
template<typename T> requires(std::is_trivially_destructible_v<T> && std::is_trivially_copy_assignable_v<T>)
bool __resize2(T*& _ptr, size_t n)
{
	if (n<=ArraySize(_ptr)) return _ptr = REALLOC(_ptr,n*sizeof(T)),true;
	return false;
}

template<typename T> requires(!(std::is_trivially_destructible_v<T>&& std::is_trivially_copy_assignable_v<T>))
bool __resize2(T*& _ptr, size_t n) { return n == ArraySize(_ptr); }
#else
constexpr bool __resize2(size_t _ptr_size, size_t n) { return n == _ptr_size; }
#endif

template<typename T>
typename std::enable_if<!(std::is_trivially_constructible_v<T>&& std::is_trivially_destructible_v<T>&& std::is_trivially_copy_assignable_v<T>), bool>::type
_resize(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0 < _ptr_size : 0 == _ptr_size);
#endif
	if (0 >= n) {
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return true;
	};
	if (!_ptr) {
		_ptr = _new_buffer<T>(n);
#ifndef ZAIMONI_FORCE_ISO
		return 0 != _ptr;
#else
		if (!_ptr) return false;
		_ptr_size = n;
		return true;
#endif
	};

	if (__resize2(ZAIMONI_NONISO_ISO_SRC(_ptr, _ptr_size), n)) return true;

	if (T* Tmp = _new_buffer<T>(n)) {
		_copy_expendable_buffer(Tmp, _ptr, min(ZAIMONI_NONISO_ISO_SRC(ArraySize(_ptr), _ptr_size), n));
		_flush(_ptr);
		_ptr = Tmp;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = n;
#endif
		return true;
	}
	return false;
}

template<typename T>
typename std::enable_if<std::is_trivially_constructible_v<T>&& std::is_trivially_destructible_v<T>&& std::is_trivially_copy_assignable_v<T>, bool>::type
_resize(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0 < _ptr_size : 0 == _ptr_size);
#endif
	if (0 >= n) {
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return true;
	};
	if (T* Tmp = REALLOC(_ptr, n * sizeof(T))) {
		_ptr = Tmp;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = n;
#endif
		return true;
	};
	return false;
}

template<typename T>
bool _resize(T * *&_ptr ZAIMONI_ISO_PARAM(size_t & _ptr_size), size_t n)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0 < _ptr_size : 0 == _ptr_size);
#endif
	if (0>=n)
		{
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return true;
		};
	if (!_ptr)
		{
		_ptr = reinterpret_cast<T**>(calloc(n,sizeof(T*)));
#ifndef ZAIMONI_FORCE_ISO
		return 0!=_ptr;
#else
		if (!_ptr) return false;
		_ptr_size = n;
		return true;
#endif
		}

	const size_t _ptr_size_old = ZAIMONI_NONISO_ISO_SRC(ArraySize(_ptr), _ptr_size);
	if (_ptr_size_old>n)
		{
		size_t i = _ptr_size_old;
		do	_single_flush(_ptr[--i]);
		while(n<i);
		};
	if (T** Tmp = REALLOC(_ptr, n * sizeof(T*)))
		{
		_ptr = Tmp;
		if (_ptr_size_old<n) memset(_ptr+_ptr_size_old,0,sizeof(T*)*(n-_ptr_size_old));
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = n;
#endif
		return true;
		}
	return false;
}

template<typename T>
inline bool ExtendByN(T*& _ptr, size_t N)
{
	return _resize(_ptr,SafeArraySize(_ptr)+N);
}

template<typename T>
void CopyDataFromPtrToPtr(T*& dest, const T* src, size_t src_size)
{	/* FORMALLY CORRECT: Kenneth Boyd, 4/28/2006 */
	if (!_resize(dest,src_size))
		{
		_flush(dest);
		dest = 0;
		return;
		};
	_value_copy_buffer(dest,src,src_size*sizeof(T));
}

template<typename T>
void _shrink(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n)
{
	if constexpr (std::is_trivially_destructible_v<T> && std::is_trivially_copy_assignable_v<T>) {
		assert(_ptr);
#ifndef ZAIMONI_FORCE_ISO
		assert(n < ArraySize(_ptr));
#else
		assert(n < _ptr_size);
		assert(0 < _ptr_size);
#endif
		_ptr = REALLOC(_ptr, n * sizeof(T));
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = n;
#endif
	} else {
		_resize(_ptr ZAIMONI_ISO_PARAM(_ptr_size), n);
	}
}

#ifndef ZAIMONI_FORCE_ISO
template<typename T>
void _shrink(T**& _ptr,size_t n)
{	//! \pre NULL!=Target, n<ArraySize(__ptr)
	assert(_ptr);
	assert(n<ArraySize(_ptr));
	if (0>=n)
		{
		_flush(_ptr);
		_ptr = 0;
		return;
		}
	size_t i = ArraySize(_ptr);
	do	_single_flush(_ptr[--i]);
	while(n<i);
	_ptr = REALLOC(_ptr,n*sizeof(T*));
}
#endif

// specializations for pointer arrays
template<typename T>
bool _weak_resize(T**& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0<_ptr_size : 0==_ptr_size);
#endif
	if (0>=n)
		{
		_weak_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return true;
		}
	if (!_ptr)
		{
		_ptr = reinterpret_cast<T**>(calloc(n,sizeof(T*)));
#ifndef ZAIMONI_FORCE_ISO
		return 0!=_ptr;
#else
		if (!_ptr) return false;
		_ptr_size = n;
		return true;
#endif
		}

	const size_t _ptr_size_old = ZAIMONI_NONISO_ISO_SRC(ArraySize(_ptr), _ptr_size);
	if (T** Tmp = REALLOC(_ptr, n * sizeof(T*)))
		{
		_ptr = Tmp;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = n;
#endif
		if (_ptr_size_old<n) memset(_ptr+_ptr_size_old,0,sizeof(T*)*(n-_ptr_size_old));
		return true;
		}
	return false;
}

template<typename T,typename U>
std::enable_if_t<std::is_trivially_constructible_v<T> && std::is_trivially_copy_assignable_v<T>, bool>
_insert_slot_at(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t i, U _default)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0<_ptr_size : 0==_ptr_size);
#endif
	const size_t _ptr_size_old = ZAIMONI_NONISO_ISO_SRC(SafeArraySize(_ptr), _ptr_size);
	if (_resize(_ptr ZAIMONI_ISO_PARAM(_ptr_size),_ptr_size_old+1)) {
		T* const _offset_ptr = _ptr+i;
		if (_ptr_size_old>i)
			memmove(_offset_ptr+1,_offset_ptr,sizeof(*_ptr)*(_ptr_size_old-i));

		*_offset_ptr = _default;	// do not static-cast
		return true;
	}
	return false;
}

template<typename T, typename U>
bool _insert_slot_at(T**& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t i, U* _default)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0 < _ptr_size : 0 == _ptr_size);
#endif
	const size_t _ptr_size_old = ZAIMONI_NONISO_ISO_SRC(SafeArraySize(_ptr), _ptr_size);
	if (_weak_resize(_ptr ZAIMONI_ISO_PARAM(_ptr_size),_ptr_size_old+1))
		{
		T** const _offset_ptr = _ptr+i;
		if (_ptr_size_old>i)
			memmove(_offset_ptr+1,_offset_ptr,sizeof(*_ptr)*(_ptr_size_old-i));

		*_offset_ptr = _default;	// do not static-cast
		return true;
		}
	return false;
}

template<typename T>
std::enable_if_t<std::is_trivially_constructible_v<T>&& std::is_trivially_copy_assignable_v<T>, bool>
_insert_n_slots_at(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n, size_t i)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0 < _ptr_size : 0 == _ptr_size);
#endif
	const size_t _ptr_size_old = ZAIMONI_NONISO_ISO_SRC(SafeArraySize(_ptr), _ptr_size);
	if (_resize(_ptr ZAIMONI_ISO_PARAM(_ptr_size), _ptr_size_old + n)) {
		T* const _offset_ptr = _ptr + i;
		if (_ptr_size_old > i)
			memmove(_offset_ptr + n, _offset_ptr, sizeof(*_ptr) * (_ptr_size_old - i));
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size += n;
#endif
		return true;
	}
	return false;
}

template<typename T>
bool _insert_n_slots_at(T**& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n, size_t i)
{
#ifdef ZAIMONI_FORCE_ISO
	assert(_ptr ? 0<_ptr_size : 0==_ptr_size);
#endif
	const size_t _ptr_size_old = ZAIMONI_NONISO_ISO_SRC(SafeArraySize(_ptr), _ptr_size);
	if (_weak_resize(_ptr ZAIMONI_ISO_PARAM(_ptr_size),_ptr_size_old+n)) {
		T** const _offset_ptr = _ptr+i;
		if (_ptr_size_old>i)
			memmove(_offset_ptr+n,_offset_ptr,sizeof(*_ptr)*(_ptr_size_old-i));

		if (0<n) memset(_offset_ptr,0,sizeof(T*)*n);
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size += n;
#endif
		return true;
	}
	return false;
}

template<typename T>
std::enable_if_t<std::is_trivially_destructible_v<T> && std::is_trivially_copy_assignable_v<T>, void>
_delete_n_slots_at(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n, size_t i)
{
	assert(_ptr);
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#else
	assert(0<_ptr_size);
#endif

	if (0==i && _ptr_size<=n) {
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
	}
	T* const _offset_ptr = _ptr+i;
	if (n<_ptr_size)
		{
		if (i+n<_ptr_size)
			memmove(_offset_ptr,_offset_ptr+n,sizeof(*_ptr)*(_ptr_size-i-n));
		_ptr = REALLOC(_ptr,sizeof(*_ptr)*(_ptr_size-n));
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size -= n;
#endif
	} else{
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
	}
}

template<typename T>
void _delete_n_slots_at(T**& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n, size_t i)
{
	assert(_ptr);
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#else
	assert(0 < _ptr_size);
#endif

	if (0 == i && _ptr_size <= n) {
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
	}
	T** const _offset_ptr = _ptr + i;
	{
		size_t NImage = n;
		do {
			_single_flush(_offset_ptr[--NImage]);
			_offset_ptr[NImage] = 0;
		} while (0 < NImage);
	}
	if (n < _ptr_size) {
		if (i + n < _ptr_size)
			memmove(_offset_ptr, _offset_ptr + n, sizeof(*_ptr) * (_ptr_size - i - n));
		_ptr = REALLOC(_ptr, sizeof(*_ptr) * (_ptr_size - n));
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size -= n;
#endif
	} else {
		free(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
	}
}

template<typename T>
std::enable_if_t<std::is_trivially_destructible_v<T>&& std::is_trivially_copy_assignable_v<T>, void>
_delete_n_slots(T*& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t* _indexes, size_t n)
{
	assert(_ptr);
	assert(_indexes);
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#else
	assert(0<_ptr_size);
#endif
	if (0>=n) return;
	assert(_indexes[0]<_ptr_size);
	if (_ptr_size<=n) {
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
	}

	size_t i = 0;
	while(i<n) {
		assert(i+1>=n || _indexes[i]>_indexes[i+1]);
		if (_indexes[i]+i<_ptr_size)
			memmove(_ptr+_indexes[i],_ptr+_indexes[i]+1,sizeof(*_ptr)*(_ptr_size-(_indexes[i]+i)));
		++i;
	}

	_ptr = REALLOC(_ptr,sizeof(*_ptr)*(_ptr_size-n));
#ifdef ZAIMONI_FORCE_ISO
	_ptr_size -= n;
#endif
}

template<typename T>
void _delete_n_slots(T**& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t* _indexes, size_t n)
{
	assert(_ptr);
	assert(_indexes);
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#else
	assert(0<_ptr_size);
#endif
	if (0>=n) return;
	assert(_indexes[0]<_ptr_size);
	if (_ptr_size<=n) {
		_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
	}

	size_t i = 0;
	while(i<n) {
		assert(i+1>=n || _indexes[i]>_indexes[i+1]);
		_single_flush(_ptr[_indexes[i]]);
		if (_indexes[i]+i<_ptr_size)
			memmove(_ptr+_indexes[i],_ptr+_indexes[i]+1,sizeof(*_ptr)*(_ptr_size-(_indexes[i]+i)));
		++i;
	}

	_ptr = REALLOC(_ptr,sizeof(*_ptr)*(_ptr_size-n));
#ifdef ZAIMONI_FORCE_ISO
	_ptr_size -= n;
#endif
}

template<typename T>
void _weak_delete_n_slots_at(T**& _ptr ZAIMONI_ISO_PARAM(size_t& _ptr_size), size_t n, size_t i)
{
	assert(_ptr);
#ifndef ZAIMONI_FORCE_ISO
	const size_t _ptr_size = ArraySize(_ptr);
#else
	assert(0<_ptr_size);
#endif
	if (0==i && _ptr_size<=n) {
		_weak_flush(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
		return;
	}
	T** const _offset_ptr = _ptr+i;
	if (n<_ptr_size) {
		if (i+n<_ptr_size)
			memmove(_offset_ptr,_offset_ptr+n,sizeof(T*)*(_ptr_size-i-n));
		_ptr = REALLOC(_ptr,sizeof(T*)*(_ptr_size-n));
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size -= n;
#endif
	} else {
		free(_ptr);
		_ptr = 0;
#ifdef ZAIMONI_FORCE_ISO
		_ptr_size = 0;
#endif
	}
}

}	// end namespace zaimoni

#endif
