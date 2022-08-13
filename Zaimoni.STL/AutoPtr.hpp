// AutoPtr.hpp
// a family of pointers that automatically delete themselves when going out of scope
// (C)2009-2011,2020,2022 Kenneth Boyd, license: MIT.txt

// autoval_ptr: single pointer
// weakautoarray_ptr: array of weak pointers
// autoarray_ptr: array of owned pointers

// this file is radically different than <memory>, don't pretend to be interoperable
// NOTE: explicit-grabbing semantics for operator= breaks assigning function results (e.g., operator new)

#ifndef ZAIMONI_AUTOPTR_HPP
#define ZAIMONI_AUTOPTR_HPP 1

#include "POD_autoarray.hpp"

namespace zaimoni	{

// meta type for autodel_ptr, autoval_ptr (centralizes core code)
template<typename T>
class _meta_auto_ptr
{
protected:
	T* _ptr;

	explicit _meta_auto_ptr() noexcept : _ptr(nullptr) {}
	explicit _meta_auto_ptr(T*& src) noexcept : _ptr(src) {src = nullptr;}
	explicit _meta_auto_ptr(const _meta_auto_ptr& src) : _ptr(nullptr) {*this=src;}
	explicit _meta_auto_ptr(_meta_auto_ptr&& src) noexcept : _ptr(src._ptr) { src._ptr = nullptr; }
	~_meta_auto_ptr() { _single_flush(_ptr); _ptr = nullptr; }

	void operator=(T* src) noexcept {
		if (_ptr != src) {
			_single_flush(_ptr);
			_ptr = src;
		}
	}

	void operator=(const _meta_auto_ptr& src) {
		if (!src._ptr) {
			_single_flush(_ptr);
			_ptr = nullptr;
			return;
		};
		if (_ptr && *_ptr == *src._ptr) return;
		CopyInto(*src._ptr, _ptr);
	}

	void operator=(_meta_auto_ptr&& src) noexcept {
		_single_flush(_ptr);
		_ptr = src._ptr;
		src._ptr = nullptr;
	}
public:
	void reset() { _single_flush(_ptr); _ptr = 0; }
	void reset(T*& src) {	// this convolution handles a recursion issue
		T* TmpPtr = src;
		src = nullptr;
		if (TmpPtr != _ptr) {
			if (_ptr) _single_flush(_ptr);
			_ptr = TmpPtr;
		};
	}

	void MoveInto(_meta_auto_ptr<T>& dest) {dest.reset(_ptr);}

	template<typename U> void TransferOutAndNULL(U*& dest) { _single_flush(dest); dest = _ptr; _ptr = nullptr; }
	T* release() { T* tmp = _ptr; _ptr = nullptr; return tmp; }
	bool empty() const {return !_ptr;};
	void NULLPtr() { _ptr = nullptr; }

	// typecasts
	operator T*&() { return _ptr; }
	operator T* const&() const { return _ptr; }

	// NOTE: C/C++ -> of const pointer to nonconst data is not const
	T* operator->() const { return _ptr; }
};

template<typename T>
class autoval_ptr : public _meta_auto_ptr<T>
{
public:
	explicit autoval_ptr() = default;
	explicit autoval_ptr(T*& src) noexcept : _meta_auto_ptr<T>(src) {};
	explicit autoval_ptr(const autoval_ptr& src) = default;
	explicit autoval_ptr(autoval_ptr&& src) = default;
	autoval_ptr& operator=(const autoval_ptr & src) = default;
	autoval_ptr& operator=(autoval_ptr&& src) = default;
	~autoval_ptr() = default;

	autoval_ptr& operator=(T* src) noexcept { _meta_auto_ptr<T>::operator=(src); return *this; }

	friend void swap(autoval_ptr& lhs, autoval_ptr& rhs) { std::swap(lhs._ptr, rhs._ptr); }
};

template<typename T>
class _meta_weakautoarray_ptr : public POD_autoarray_ptr<T>
{
protected:
	explicit _meta_weakautoarray_ptr() noexcept {this->NULLPtr();};
	explicit _meta_weakautoarray_ptr(T*& src ZAIMONI_ISO_PARAM(size_t src_size)) noexcept { this->_ptr = src; ZAIMONI_ISO_SRC(this->_size = src_size;) src = 0; }
	explicit _meta_weakautoarray_ptr(size_t n) { this->de_novo(n); }
	explicit _meta_weakautoarray_ptr(const std::nothrow_t& tracer, size_t n) noexcept { this->de_novo_nothrow(n); }
	explicit _meta_weakautoarray_ptr(const _meta_weakautoarray_ptr& src) { this->NULLPtr(); *this = src; }
	explicit _meta_weakautoarray_ptr(_meta_weakautoarray_ptr&& src) noexcept { this->NULLPtr(); *this = std::move(src); }
	~_meta_weakautoarray_ptr() { _weak_flush(this->_ptr); }

#ifndef ZAIMONI_FORCE_ISO
	void operator=(T* src) noexcept {
		if (this->_ptr != src) {
			_weak_flush(this->_ptr);
			this->_ptr = src;
		}
	}
#endif
	void operator=(const _meta_weakautoarray_ptr& src) {	// this convolution handles a recursion issue
		const size_t ub = src.size();
		if (0 >= ub) reset();
		else {
			this->resize(ub);
			_copy_buffer(this->c_array(), src.data(), ub);
		};
	}
	void operator=(_meta_weakautoarray_ptr&& src) noexcept {	// this convolution handles a recursion issue
		const size_t ub = src.size();
		if (0 >= ub) reset();
		else {
			reset(src._ptr ZAIMONI_ISO_PARAM(src._size));
			ZAIMONI_ISO_SRC(src._size = 0);
		};
	}

public:
	template<typename U> bool value_copy_of(const U& src);	// STL interfaces required of U: size(),data()
	void reset() noexcept {_weak_flush(this->_ptr); this->NULLPtr();}
	void TransferOutAndNULL(T*& dest) noexcept {_weak_flush(dest); dest = this->_ptr; this->NULLPtr();}
	void reset(T*& src ZAIMONI_ISO_PARAM(size_t n)) noexcept {	// this convolution handles a recursion issue
		T* tmp = src;
		src = 0;
		if (tmp != this->_ptr) {
			if (this->_ptr) _weak_flush(this->_ptr);
			this->_ptr = tmp;
#ifdef ZAIMONI_FORCE_ISO
			this->_size = n;
#endif
		};
	}

	void MoveInto(_meta_weakautoarray_ptr<T>& dest) { dest.reset(this->_ptr ZAIMONI_ISO_PARAM(this->_size)); }
	bool Resize(size_t n) { return _weak_resize(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n); }
	void FastDeleteIdx(size_t n) { _weak_delete_idx(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n); }
	void DeleteIdx(size_t n) { _safe_weak_delete_idx(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n); }
	void DeleteNSlotsAt(size_t n, size_t Idx) { _weak_delete_n_slots_at(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n, Idx); }
	void resize(size_t n) {if (!Resize(n)) throw std::bad_alloc();}

	// Perl grep
	// next two require of U: STL size(),data()
	template<typename U,typename op> bool grep(const U& src,op Predicate);
	template<typename U,typename op> bool invgrep(const U& src,op Predicate);

	// erase all elements
	void clear() {_weak_flush(this->_ptr); this->NULLPtr();}
};

template<typename T>
class weakautoarray_ptr : public _meta_weakautoarray_ptr<T>
{
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	explicit weakautoarray_ptr() = default;
	explicit weakautoarray_ptr(T*& src) noexcept : _meta_weakautoarray_ptr<T>(src) {}
	explicit weakautoarray_ptr(size_t n) noexcept : _meta_weakautoarray_ptr<T>(std::nothrow,n) {}
	explicit weakautoarray_ptr(const weakautoarray_ptr& src) = delete;
	explicit weakautoarray_ptr(weakautoarray_ptr&& src) : _meta_weakautoarray_ptr<T>(std::move(src)) {}
	~weakautoarray_ptr() = default;

#ifndef ZAIMONI_FORCE_ISO
	const weakautoarray_ptr& operator=(T* src) noexcept {_meta_weakautoarray_ptr<T>::operator=(src); return *this;};
#endif
	const weakautoarray_ptr& operator=(const weakautoarray_ptr& src) = delete;
	const weakautoarray_ptr& operator=(weakautoarray_ptr&& src) noexcept { _meta_weakautoarray_ptr<T>::operator=(std::move(src)); return *this; }

	friend void swap(weakautoarray_ptr& lhs, weakautoarray_ptr& rhs) {lhs.swap(rhs);};
};

template<typename T>
class weakautoarray_ptr_throws : public _meta_weakautoarray_ptr<T>
{
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	explicit weakautoarray_ptr_throws() = default;
	explicit weakautoarray_ptr_throws(T*& src) noexcept : _meta_weakautoarray_ptr<T>(src) {}
	explicit weakautoarray_ptr_throws(size_t n) : _meta_weakautoarray_ptr<T>(n) {}
	explicit weakautoarray_ptr_throws(const weakautoarray_ptr_throws& src) = delete;
	explicit weakautoarray_ptr_throws(weakautoarray_ptr_throws&& src) noexcept : _meta_weakautoarray_ptr<T>(std::move(src)) {}
	~weakautoarray_ptr_throws() = default;

#ifndef ZAIMONI_FORCE_ISO
	const weakautoarray_ptr_throws& operator=(T* src) noexcept {_meta_weakautoarray_ptr<T>::operator=(src); return *this;};
#endif
	const weakautoarray_ptr_throws& operator=(const weakautoarray_ptr_throws& src) = delete;
	const weakautoarray_ptr_throws& operator=(weakautoarray_ptr_throws&& src) noexcept { _meta_weakautoarray_ptr<T>::operator=(std::move(src)); return *this; }

	friend void swap(weakautoarray_ptr_throws& lhs, weakautoarray_ptr_throws& rhs) {lhs.swap(rhs);};
};

template<typename T>
class weakautovalarray_ptr : public _meta_weakautoarray_ptr<T>
{
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	explicit weakautovalarray_ptr() = default;
	explicit weakautovalarray_ptr(T*& src) noexcept : _meta_weakautoarray_ptr<T>(src) {}
	explicit weakautovalarray_ptr(size_t n) : _meta_weakautoarray_ptr<T>(std::nothrow,n) {}
	weakautovalarray_ptr(const weakautovalarray_ptr& src) : _meta_weakautoarray_ptr<T>(src) {}
	weakautovalarray_ptr(const _meta_weakautoarray_ptr<T>& src) : _meta_weakautoarray_ptr<T>(src) {}
	~weakautovalarray_ptr() = default;

#ifndef ZAIMONI_FORCE_ISO
	const weakautovalarray_ptr& operator=(T* src) noexcept {_meta_weakautoarray_ptr<T>::operator=(src); return *this;}
#endif
	const weakautovalarray_ptr& operator=(const weakautovalarray_ptr& src) { _meta_weakautoarray_ptr<T>::operator=(src); return *this;}
	const weakautovalarray_ptr& operator=(weakautovalarray_ptr&& src) noexcept { _meta_weakautoarray_ptr<T>::operator=(std::move(src)); return *this; }

	friend void swap(weakautovalarray_ptr& lhs, weakautovalarray_ptr& rhs) {lhs.swap(rhs);}
};

template<typename T>
class weakautovalarray_ptr_throws : public _meta_weakautoarray_ptr<T>
{
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	explicit weakautovalarray_ptr_throws() = default;
	explicit weakautovalarray_ptr_throws(T*& src) noexcept : _meta_weakautoarray_ptr<T>(src) {}
	explicit weakautovalarray_ptr_throws(size_t n) : _meta_weakautoarray_ptr<T>(n) {}
	weakautovalarray_ptr_throws(const weakautovalarray_ptr_throws& src) : _meta_weakautoarray_ptr<T>(src) {}
	weakautovalarray_ptr_throws(weakautovalarray_ptr_throws&& src) noexcept : _meta_weakautoarray_ptr<T>(std::move(src)) {}
	~weakautovalarray_ptr_throws() = default;

#ifndef ZAIMONI_FORCE_ISO
	const weakautovalarray_ptr_throws& operator=(T* src) noexcept {_meta_weakautoarray_ptr<T>::operator=(src); return *this;}
#endif
	const weakautovalarray_ptr_throws& operator=(const weakautovalarray_ptr_throws& src) { _meta_weakautoarray_ptr<T>::operator=(src); return *this; }
	const weakautovalarray_ptr_throws& operator=(weakautovalarray_ptr_throws&& src) noexcept { _meta_weakautoarray_ptr<T>::operator=(std::move(src)); return *this; }

	friend void swap(weakautovalarray_ptr_throws& lhs, weakautovalarray_ptr_throws& rhs) {lhs.swap(rhs);}
};

template<typename T>
class _meta_autoarray_ptr : public POD_autoarray_ptr<T>
{
protected:
	explicit _meta_autoarray_ptr() noexcept {this->NULLPtr();}
	explicit _meta_autoarray_ptr(T*& src ZAIMONI_ISO_PARAM(size_t src_size)) noexcept { this->_ptr = src; ZAIMONI_ISO_SRC(this->_size = src_size;) src = 0; }
	explicit _meta_autoarray_ptr(size_t n) {this->de_novo(n);}
	explicit _meta_autoarray_ptr(const std::nothrow_t& tracer, size_t n) noexcept {this->de_novo_nothrow(n);}
	explicit _meta_autoarray_ptr(const _meta_autoarray_ptr& src) {this->NULLPtr(); *this=src;}
	explicit _meta_autoarray_ptr(_meta_autoarray_ptr&& src) noexcept { POD_autoarray_ptr<T>::operator=(src); src.NULLPtr(); }
	~_meta_autoarray_ptr() {_flush(this->_ptr);}

#ifndef ZAIMONI_FORCE_ISO
	void operator=(T* src) noexcept {
		if (this->_ptr != src) {
			_flush(this->_ptr);
			this->_ptr = src;
		}
	}
#endif
	void operator=(const _meta_autoarray_ptr& src) {
		const size_t ub = src.size();
		if (0 >= ub) reset();
		else {
			this->resize(ub);
			_value_copy_buffer(this->c_array(), src.data(), ub);
		};
	}

	void operator=(_meta_autoarray_ptr&& src) noexcept {
		_flush(this->_ptr);
		POD_autoarray_ptr<T>::operator=(src);
		src.NULLPtr();
	}
public:
	typedef bool UnaryPredicate(const T&);

	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	template<typename U> bool value_copy_of(const U& src);	// STL interfaces required of U: size(),data()
	void reset() noexcept {_flush(this->_ptr); this->NULLPtr();}
	void TransferOutAndNULL(T*& dest) noexcept {_flush(dest); dest = this->_ptr; this->NULLPtr();}

	void reset(T*& src ZAIMONI_ISO_PARAM(size_t n));
	void MoveInto(_meta_autoarray_ptr<T>& dest) { dest.reset(this->_ptr ZAIMONI_ISO_PARAM(this->_size)); }

	// Perl grep
	// these two assume T has valid * operator
	template<typename U> bool grep(UnaryPredicate* Predicate,_meta_autoarray_ptr<U*>& dest) const;
	template<typename U> bool invgrep(UnaryPredicate* Predicate,_meta_autoarray_ptr<U*>& dest) const;

	void clear() {_flush(this->_ptr); this->NULLPtr();} // erase all elements
};

template<typename T>
class autoarray_ptr : public _meta_autoarray_ptr<T>
{
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	explicit autoarray_ptr() = default;
	explicit autoarray_ptr(T*& src) noexcept : _meta_autoarray_ptr<T>(src) {}
	explicit autoarray_ptr(size_t n) noexcept : _meta_autoarray_ptr<T>(std::nothrow,n) {}
	explicit autoarray_ptr(const autoarray_ptr& src) = delete;
	explicit autoarray_ptr(autoarray_ptr&& src) noexcept : _meta_autoarray_ptr<T>(std::move(src)) {}
	~autoarray_ptr() = default;

	autoarray_ptr& operator=(T* src) noexcept {_meta_autoarray_ptr<T>::operator=(src); return *this;}
	autoarray_ptr& operator=(const autoarray_ptr& src) = delete;
	autoarray_ptr& operator=(autoarray_ptr&& src) noexcept { _meta_autoarray_ptr<T>::operator=(std::move(src)); return *this; }

	// swaps
	friend void swap(autoarray_ptr<T>& lhs, autoarray_ptr<T>& rhs) {lhs.swap(rhs);}
};

template<typename T>
class autovalarray_ptr : public _meta_autoarray_ptr<T>
{
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	explicit autovalarray_ptr() = default;
	explicit autovalarray_ptr(T * &src ZAIMONI_ISO_PARAM(size_t src_size)) noexcept : _meta_autoarray_ptr<T>(src ZAIMONI_ISO_PARAM(src_size)) {}
	explicit autovalarray_ptr(size_t n) noexcept : _meta_autoarray_ptr<T>(std::nothrow, n) {}
	autovalarray_ptr(const autovalarray_ptr& src) = default;
	autovalarray_ptr(autovalarray_ptr&& src) = default;
	autovalarray_ptr& operator=(const autovalarray_ptr& src) = default;
	autovalarray_ptr& operator=(autovalarray_ptr&& src) = default;
	~autovalarray_ptr() = default;

	const autovalarray_ptr& operator=(T * src) noexcept { _meta_autoarray_ptr<T>::operator=(src); return *this; }

	// swaps
	friend void swap(autovalarray_ptr<T> & lhs, autovalarray_ptr<T> & rhs) { lhs.swap(rhs); }
};

template<typename T>
class autovalarray_ptr_throws : public _meta_autoarray_ptr<T>
{
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);

	explicit autovalarray_ptr_throws() = default;
	explicit autovalarray_ptr_throws(T * &src ZAIMONI_ISO_PARAM(size_t src_size)) noexcept : _meta_autoarray_ptr<T>(src ZAIMONI_ISO_PARAM(src_size)) {}
	explicit autovalarray_ptr_throws(size_t n) : _meta_autoarray_ptr<T>(n) {}
	autovalarray_ptr_throws(const autovalarray_ptr_throws& src) = default;
	autovalarray_ptr_throws(autovalarray_ptr_throws&& src) = default;
	autovalarray_ptr_throws& operator=(const autovalarray_ptr_throws & src) = default;
	autovalarray_ptr_throws& operator=(autovalarray_ptr_throws&& src) = default;
	~autovalarray_ptr_throws() = default;

	const autovalarray_ptr_throws& operator=(T * src) noexcept { _meta_autoarray_ptr<T>::operator=(src); return *this; }

	// swaps
	friend void swap(autovalarray_ptr_throws<T> & lhs, autovalarray_ptr_throws<T> & rhs) { lhs.swap(rhs); }
};

template<typename T>
template<typename U>
bool
_meta_weakautoarray_ptr<T>::value_copy_of(const U& src)
{
	const size_t ub = src.size();
	if (!Resize(ub)) return false;
	if (0<ub) _copy_buffer(this->c_array(),src.data(),ub);
	return true;
}

// Perl grep
template<typename T>
template<typename U,typename op>
bool
_meta_weakautoarray_ptr<T>::grep(const U& src,op Predicate)
{
	if (src.empty()) return reset(),true;
	size_t lb = 0;
	size_t strict_ub = src.size();
	while(!Predicate(src.data()[lb]) && strict_ub> ++lb);
	if (strict_ub==lb) return reset(),true;
	while(!Predicate(src[--strict_ub]));
	++strict_ub;
	if (!Resize(strict_ub-lb)) return false;
	size_t Offset = 0;
	this->_ptr[Offset++] = src.data()[lb++];
	while(lb<strict_ub-1)
		if (Predicate(src.data()[lb++]))
			this->_ptr[Offset++] = src[lb-1];
	this->_ptr[Offset++] = src.data()[lb++];
	return Resize(Offset),true;
}

template<typename T>
template<typename U,typename op>
bool
_meta_weakautoarray_ptr<T>::invgrep(const U& src,op Predicate)
{
	if (src.empty()) return reset(),true;
	size_t lb = 0;
	size_t strict_ub = src.size();
	while(Predicate(src.data()[lb]) && strict_ub> ++lb);
	if (strict_ub==lb) return reset(),true;
	while(Predicate(src[--strict_ub]));
	++strict_ub;
	if (!Resize(strict_ub-lb)) return false;
	size_t Offset = 0;
	this->_ptr[Offset++] = src.data()[lb++];
	while(lb<strict_ub-1)
		if (!Predicate(src.data()[lb++]))
			this->_ptr[Offset++] = src[lb-1];
	this->_ptr[Offset++] = src.data()[lb++];
	return Resize(Offset),true;
}

template<typename T>
template<typename U>
bool
_meta_autoarray_ptr<T>::value_copy_of(const U& src)
{
	const size_t ub = src.size();
	if (!this->Resize(ub)) return false;
	if (0<ub)
		try	{
			_value_copy_buffer(this->c_array(),src.data(),ub);
			}
		catch(const std::bad_alloc&)
			{
			return false;
			}
	return true;
}

template<typename T>
void _meta_autoarray_ptr<T>::reset(T*& src ZAIMONI_ISO_PARAM(size_t n))
{	// this convolution handles a recursion issue
	T* tmp = src;
	src = NULL;
	if (tmp!=this->_ptr)
		{
		if (this->_ptr) _flush(this->_ptr);
		this->_ptr = tmp;
#ifdef ZAIMONI_FORCE_ISO
		this->_size = n;
#endif		
		};
}

template<typename T>
template<typename U>
bool
_meta_autoarray_ptr<T>::grep(UnaryPredicate* Predicate,_meta_autoarray_ptr<U*>& dest) const
{
	dest.reset();
	if (this->empty()) return true;

	size_t lb = 0;
	size_t strict_ub = this->ArraySize();
	while(!Predicate(*(this->_ptr)[lb]) && strict_ub> ++lb);
	if (strict_ub==lb) return true;

	while(!Predicate(*(this->_ptr)[--strict_ub]));
	++strict_ub;
	if (!dest.Resize(strict_ub-lb)) return false;

	size_t Offset = 0;
	try	{
		dest[Offset++] = new U(*(this->_ptr)[lb++]);
		while(lb<strict_ub-1)
			if (Predicate(*(this->_ptr)[lb++]))
				dest[Offset++] = new U(*(this->_ptr)[lb-1]);
		dest[Offset++] = new U(*(this->_ptr)[lb++]);
		}
	catch(const std::bad_alloc&)
		{
		return dest.Resize(Offset),false;	
		}
	return dest.Resize(Offset),true;
}

template<typename T>
template<typename U>
bool
_meta_autoarray_ptr<T>::invgrep(UnaryPredicate* Predicate,_meta_autoarray_ptr<U*>& dest) const
{
	dest.reset();
	if (this->empty()) return true;

	size_t lb = 0;
	size_t strict_ub = this->ArraySize();
	while(Predicate(*(this->_ptr)[lb]) && strict_ub> ++lb);
	if (strict_ub==lb) return true;

	while(Predicate(*(this->_ptr)[--strict_ub]));
	++strict_ub;
	if (!dest.Resize(strict_ub-lb)) return false;

	size_t Offset = 0;
	try	{
		dest[Offset++] = new U(*(this->_ptr)[lb++]);
		while(lb<strict_ub-1)
			if (!Predicate(*(this->_ptr)[lb++]))
				dest[Offset++] = new U(*(this->_ptr)[lb-1]);
		dest[Offset++] = new U(*(this->_ptr)[lb++]);
		}
	catch(const std::bad_alloc&)
		{
		return dest.Resize(Offset),false;	
		}
	return dest.Resize(Offset),true;
}

// Resize won't compile without this [CSVTable.cxx]
template<typename T>
inline void
_copy_buffer(autoarray_ptr<T>* dest, autoarray_ptr<T>* src, size_t Idx)
{
	do	{
		--Idx;
		dest[Idx] = src[Idx];
		}
	while(0<Idx);
}

}		// end namespace zaimoni

#include "bits/cstdio_autoptr.hpp"

#endif
