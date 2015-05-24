// POD_autoarray.hpp
// standard-format core for the autoarray family (AutoPtr.hpp) suitable for use
// in standard-layout structs that have to be C-compatible
// (C)2011 Kenneth Boyd, license: MIT.txt

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
#ifndef ZAIMONI_FORCE_ISO
	// core infrastructure
	void NULLPtr() {_ptr = NULL;};
	void de_novo(size_t n) {_ptr = n ? _new_buffer_nonNULL_throws<T>(n) : NULL;};
	void de_novo_nothrow(size_t n) {_ptr = _new_buffer<T>(n);};
	
	size_t ArraySize() const {return zaimoni::ArraySize(_ptr);};
	template<typename U> bool InsertSlotAt(size_t i, U _default) {return _insert_slot_at(_ptr,i,_default);}
	bool InsertNSlotsAt(size_t n,size_t i) {return _insert_n_slots_at(_ptr,n,i);};
	void insertNSlotsAt(size_t n,size_t i) {if (!_insert_n_slots_at(_ptr,n,i)) throw std::bad_alloc();};
	void Overwrite(T*& src) {_ptr = src; src = NULL;};

	void swap(POD_autoarray_ptr<T>& rhs) {std::swap(_ptr,rhs._ptr);};
	
	// STL support	
	size_t size() const { return zaimoni::SafeArraySize(_ptr); };
	T* release() {T* tmp = _ptr; _ptr = NULL; return tmp;};
#else
	void NULLPtr() {_ptr = NULL; _size = 0;};
	void de_novo(size_t n) {_ptr = n ? _new_buffer_nonNULL_throws<T>(n) : NULL; _size = n;};
	void de_novo_nothrow(size_t n) {_ptr = _new_buffer<T>(n); _size = n;};

	size_t ArraySize() const {return _size;};
	template<typename U> bool InsertSlotAt(size_t i, U _default) {return _insert_slot_at(_ptr,_size,i,_default);}
	bool InsertNSlotsAt(size_t n,size_t i) {return _insert_n_slots_at(_ptr,_size,n,i);};
	void insertNSlotsAt(size_t n,size_t i) {if (!_insert_n_slots_at(_ptr,_size,n,i)) throw std::bad_alloc();};
	void Overwrite(T*& src,size_t n) {_ptr = src; src = NULL; _size = n;};

	void swap(POD_autoarray_ptr<T>& rhs) {std::swap(_ptr,rhs._ptr); std::swap(_size,rhs._size);};
	
	// STL support	
	T* release() {T* tmp = _ptr; _ptr = NULL; _size = 0; return tmp;};
	size_t size() const { return _size; };
#endif	
	T* c_array() {return _ptr;};
	const T* data() const {return _ptr;};
	bool empty() const { return NULL==_ptr; };
	static size_t max_size() { return size_t(-1)/sizeof(T); };	// XXX casting -1 to an unsigned type gets the maximum of that type

	void rangecheck(size_t i) const { if (i>=size()) FATAL("out-of-bounds array access"); };

	// Perl grep
	template<typename U> void destructive_grep(U& x,bool (&equivalence)(typename boost::call_traits<U>::param_type,typename boost::call_traits<T>::param_type));
	template<typename U> void destructive_invgrep(U& x,bool (&equivalence)(typename boost::call_traits<U>::param_type,typename boost::call_traits<T>::param_type));

	// typecasts
	operator T*&() {return _ptr;};
	operator T* const&() const {return _ptr;};

	// generic case is owned-values, so put autovalarray_ptr family implementations here
#ifndef ZAIMONI_FORCE_ISO
	bool Resize(size_t n) {return _resize(this->_ptr,n);};
	void Shrink(size_t n) {_shrink(this->_ptr,n);};
	void FastDeleteIdx(size_t n) {_delete_idx(this->_ptr,n);};
	void DeleteIdx(size_t n) {_safe_delete_idx(this->_ptr,n);};
	void DeleteNSlotsAt(size_t n, size_t Idx) {_delete_n_slots_at(this->_ptr,n,Idx);};
	void DeleteNSlots(size_t* indexes,size_t n) {_delete_n_slots(this->_ptr,indexes,n);};
#else
	bool Resize(size_t n) {return _resize(this->_ptr,this->_size,n);};
	void Shrink(size_t n) {_shrink(this->_ptr,this->_size,n);};
	void FastDeleteIdx(size_t n) {_delete_idx(this->_ptr,this->_size,n);};
	void DeleteIdx(size_t n) {_safe_delete_idx(this->_ptr,this->_size,n);};
	void DeleteNSlotsAt(size_t n, size_t Idx) {_delete_n_slots_at(this->_ptr,this->_size,n,Idx);};
	void DeleteNSlots(size_t* indexes,size_t n) {_delete_n_slots(this->_ptr,this->_size,indexes,n);};
#endif
	void resize(size_t n) {if (!this->Resize(n)) throw std::bad_alloc();};	
};

} // namespace zaimoni

namespace boost {

#define ZAIMONI_TEMPLATE_SPEC template<typename T>
#define ZAIMONI_CLASS_SPEC zaimoni::POD_autoarray_ptr<T>
ZAIMONI_POD_STRUCT(ZAIMONI_TEMPLATE_SPEC,ZAIMONI_CLASS_SPEC,char)
#undef ZAIMONI_CLASS_SPEC
#undef ZAIMONI_TEMPLATE_SPEC

} // namespace boost

namespace zaimoni {

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
POD_autoarray_ptr<T>::destructive_grep(U& x,bool (&equivalence)(typename boost::call_traits<U>::param_type,typename boost::call_traits<T>::param_type))
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
POD_autoarray_ptr<T>::destructive_invgrep(U& x,bool (&equivalence)(typename boost::call_traits<U>::param_type,typename boost::call_traits<T>::param_type))
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
