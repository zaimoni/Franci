// custom_scoped_ptr.hpp
// not intended to prevent abuse, just simplify routine maintenance
// intentionally do not use NULL constant, this removes the cstddef header dependency

#ifndef ZAIMONI_STL_CUSTOM_SCOPED_PTR_HPP
#define ZAIMONI_STL_CUSTOM_SCOPED_PTR_HPP 1

namespace zaimoni {

template<class T>
class custom_scoped_ptr
{
public:
	typedef void cleanfunc(T*&);

	custom_scoped_ptr(cleanfunc& cleanup) : _ptr(0),_cleanup(cleanup) {};
	~custom_scoped_ptr() {if (_ptr) _cleanup(_ptr);};

	void operator=(T* src)
	{
		if (_ptr) _cleanup(_ptr);
		_ptr = src;
	};

	void clear() {if (!_ptr) {_cleanup(_ptr); _ptr=0;};};
	operator T*&() {return _ptr;};
	operator T* const&() const {return _ptr;};

	// NOTE: C/C++ -> of const pointer to nonconst data is not const
	T* operator->() const {return _ptr;};
private:
	T* _ptr;
	cleanfunc& _cleanup;

	// don't allow copying
	custom_scoped_ptr(const custom_scoped_ptr& src);
	void operator=(const custom_scoped_ptr& src);
};

}	// end namespace zaimoni

#endif
