// POD_autoarray.hpp
// standard-format core for the autoarray family (AutoPtr.hpp) suitable for use
// in standard-layout structs that have to be C-compatible
// (C)2011,2020 Kenneth Boyd, license: MIT.txt

// this file is radically different than <memory>, don't pretend to be interoperable
// NOTE: explicit-grabbing semantics for operator= breaks assigning function results (e.g., operator new)

#ifndef ZAIMONI_POD_AUTOARRAY_HPP
#define ZAIMONI_POD_AUTOARRAY_HPP 1

#include "MetaRAM2.hpp"
#include "metatype/c_array.hpp"

namespace zaimoni	{

// POD backing class for the autoarray and weak autoarray family
template<typename T>
class POD_autoarray_ptr : public c_array_CRTP<POD_autoarray_ptr<T>, T>
{
protected:
	T* _ptr;
#ifdef ZAIMONI_FORCE_ISO
	size_t _size;
#endif
public:
	ZAIMONI_STL_TYPE_GLUE_ARRAY(T);
	// core infrastructure
	void NULLPtr() noexcept { _ptr = 0; ZAIMONI_ISO_SRC(_size = 0;) }
	void de_novo(size_t n) { _ptr = n ? _new_buffer_nonNULL_throws<T>(n) : 0; ZAIMONI_ISO_SRC(_size = n;) }
	void de_novo_nothrow(size_t n) { _ptr = _new_buffer<T>(n); ZAIMONI_ISO_SRC(_size = n;) }

	size_t ArraySize() const { return ZAIMONI_NONISO_ISO_SRC(zaimoni::ArraySize(_ptr), _size); };
	template<typename U> bool InsertSlotAt(size_t i, U _default) { return _insert_slot_at(_ptr ZAIMONI_ISO_PARAM(_size), i, _default); }
	bool InsertNSlotsAt(size_t n, size_t i) { return _insert_n_slots_at(_ptr ZAIMONI_ISO_PARAM(_size), n, i); }
	void insertNSlotsAt(size_t n, size_t i) { if (!_insert_n_slots_at(_ptr ZAIMONI_ISO_PARAM(_size), n, i)) throw std::bad_alloc(); }
	void Overwrite(T*& src ZAIMONI_ISO_PARAM(size_t n)) { _ptr = src; src = 0; ZAIMONI_ISO_SRC(_size = n;) }

	void swap(POD_autoarray_ptr<T>& rhs) { std::swap(_ptr, rhs._ptr); ZAIMONI_ISO_SRC(std::swap(_size, rhs._size);) }

	// STL support	
	size_t size() const { return ZAIMONI_NONISO_ISO_SRC(zaimoni::SafeArraySize(_ptr), _size); };
	T* release() { T* tmp = _ptr; _ptr = 0; ZAIMONI_ISO_SRC(_size = 0;) return tmp; };
	T* c_array() {return _ptr;};
	const T* data() const {return _ptr;};
	bool empty() const { return NULL==_ptr; };
	static size_t max_size() { return size_t(-1)/sizeof(T); };	// XXX casting -1 to an unsigned type gets the maximum of that type

	void rangecheck(size_t i) const { if (i>=size()) FATAL("out-of-bounds array access"); };

	// Perl grep
	template<typename U> void destructive_grep(U& x,bool (&equivalence)(typename zaimoni::param<U>::type,typename zaimoni::param<U>::type));
	template<typename U> void destructive_invgrep(U& x,bool (&equivalence)(typename zaimoni::param<U>::type,typename zaimoni::param<U>::type));

	// typecasts
	operator T*&() {return _ptr;};
	operator T* const&() const {return _ptr;};

	// generic case is owned-values, so put autovalarray_ptr family implementations here
	bool Resize(size_t n) { return _resize(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n); }
	void Shrink(size_t n) { _shrink(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n); }
	void FastDeleteIdx(size_t n) { _delete_idx(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n); }
	void DeleteIdx(size_t n) { _safe_delete_idx(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n); };
	void DeleteNSlotsAt(size_t n, size_t Idx) { _delete_n_slots_at(this->_ptr ZAIMONI_ISO_PARAM(this->_size), n, Idx); };
	void DeleteNSlots(size_t* indexes, size_t n) { _delete_n_slots(this->_ptr ZAIMONI_ISO_PARAM(this->_size), indexes, n); };
	void resize(size_t n) { if (!this->Resize(n)) throw std::bad_alloc(); }
};

template<class T>
bool
operator==(const POD_autoarray_ptr<T>& lhs, const POD_autoarray_ptr<T>& rhs)
{
	const size_t ub = rhs.size();
	if (ub!=lhs.size()) return false;
	if (0==ub) return true;
	return _value_vector_equal(lhs.data(),rhs.data(),ub);
}

template<class T>
template<typename U>
void
POD_autoarray_ptr<T>::destructive_grep(U& x,bool (&equivalence)(typename zaimoni::param<U>::type,typename zaimoni::param<U>::type))
{
	size_t i = this->size();
	do	if (!equivalence(x,_ptr[--i]))
			{
			size_t j = i;
			while(0<j && !equivalence(x,_ptr[j-1])) --j;
			if (j<i)
				{
				this->DeleteNSlotsAt(j,(i-j)+1);
				i = j;
				}
			else
				this->DeleteIdx(i);
			}
	while(0<i);
}

template<class T>
template<typename U>
void
POD_autoarray_ptr<T>::destructive_invgrep(U& x,bool (&equivalence)(typename zaimoni::param<U>::type,typename zaimoni::param<U>::type))
{
	size_t i = this->size();
	do	if (equivalence(x,_ptr[--i]))
			{
			size_t j = i;
			while(0<j && equivalence(x,_ptr[j-1])) --j;
			if (j<i)
				{
				this->DeleteNSlotsAt(j,(i-j)+1);
				i = j;
				}
			else
				this->DeleteIdx(i);
			}
	while(0<i);
}

}		// end namespace zaimoni

#endif
