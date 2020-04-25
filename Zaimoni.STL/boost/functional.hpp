// functional.hpp

#ifndef ZAIMONI_STL_BOOST_FUNCTIONAL_HPP
#define ZAIMONI_STL_BOOST_FUNCTIONAL_HPP 1

// The STL made a design decision not to directly support mixed-mode operators.  We do support this.

namespace zaimoni {

// assignment operators
// note that boost::bind2nd won't work on these (discard const qualifiers error) as of 1.34
template<class T,class U>
struct assign : public std::binary_function<T&,typename boost::call_traits<U>::param_type, void>
{
	void operator()(T& x,typename boost::call_traits<U>::param_type y) const { x = y; }
};

template<class T,class U>
struct assign_add : public std::binary_function<T&,typename boost::call_traits<U>::param_type, void>
{
	void operator()(T& x,typename boost::call_traits<U>::param_type y) const { x += y; }
};

template<class T,class U>
struct assign_subtract : public std::binary_function<T&,typename boost::call_traits<U>::param_type, void>
{
	void operator()(T& x,typename boost::call_traits<U>::param_type y) { x -= y; }
};

template<class T,class U>
struct assign_multiply : public std::binary_function<T&,typename boost::call_traits<U>::param_type, void>
{
	void operator()(T& x,typename boost::call_traits<U>::param_type y) { x *= y; }
};

template<class T,class U>
struct assign_divide : public std::binary_function<T&,typename boost::call_traits<U>::param_type, void>
{
	void operator()(T& x,typename boost::call_traits<U>::param_type y) { x /= y; }
};

template<class T>
struct assign_modulus : public std::binary_function<T&,typename boost::call_traits<T>::param_type, void>
{
	void operator()(T& x,typename boost::call_traits<T>::param_type y) { x %= y; }
};

// comparisons (use only for mixed-mode comparisons where copy-construction is inefficient or prohibited)
template<class T,class U>
struct equal_to : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type, bool>
{
	bool operator()(typename boost::call_traits<T>::param_type x,typename boost::call_traits<U>::param_type y) { return x==y; }
};

template<class T,class U>
struct not_equal_to : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type, bool>
{
	bool operator()(typename boost::call_traits<T>::param_type x,typename boost::call_traits<U>::param_type y) { return x!=y; }
};

template<class T,class U>
struct greater : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type, bool>
{
	bool operator()(typename boost::call_traits<T>::param_type x,typename boost::call_traits<U>::param_type y) { return x>y; }
};

template<class T,class U>
struct less : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type, bool>
{
	bool operator()(typename boost::call_traits<T>::param_type x,typename boost::call_traits<U>::param_type y) { return x<y; }
};

template<class T,class U>
struct greater_equal : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type, bool>
{
	bool operator()(typename boost::call_traits<T>::param_type x,typename boost::call_traits<U>::param_type y) { return x>=y; }
};

template<class T,class U>
struct less_equal : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type, bool>
{
	bool operator()(typename boost::call_traits<T>::param_type x,typename boost::call_traits<U>::param_type y) { return x<=y; }
};

// cross-type operators (use std:: for same type on both operands)
template<class T,class U>
struct right_multiply : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type,typename boost::call_traits<T>::value_type>
{
	typename boost::call_traits<T>::value_type operator()(typename boost::call_traits<T>::param_type x, typename boost::call_traits<U>::param_type y) {return x*y;};
};

template<class T,class U>
struct left_multiply : public std::binary_function<typename boost::call_traits<T>::param_type, typename boost::call_traits<U>::param_type,typename boost::call_traits<U>::value_type>
{
	typename boost::call_traits<U>::value_type operator()(typename boost::call_traits<T>::param_type x, typename boost::call_traits<U>::param_type y) {return x*y;};
};

// dereferencing adaptors
template<typename T,size_t N>
struct addref_power
{
	typedef typename addref_power<T,N-1>::type type;
};

template<typename T>
struct addref_power<T,1>
{
	typedef typename boost::add_pointer<T>::type type;
};

template<typename T>
struct addref_power<T,0>
{
	typedef T type;
};

template<typename T, size_t N1, size_t N2>
struct addref_binary_functor : public std::binary_function<typename addref_power<typename boost::binary_traits<T>::first_argument_type,N1>::type,typename addref_power<typename boost::binary_traits<T>::second_argument_type,N2>::type,typename boost::binary_traits<T>::result_type>
{
	typename boost::binary_traits<T>::result_type operator()(typename addref_power<typename boost::binary_traits<T>::first_argument_type,N1>::type x,typename addref_power<typename boost::binary_traits<T>::second_argument_type,N2>::type y)
		{return addref_binary_functor<T,N1-1,N2-1>(*x,*y);};
};

template<typename T>
struct addref_binary_functor<T,1,1> : public std::binary_function<typename boost::add_pointer<typename boost::binary_traits<T>::first_argument_type>::type,typename boost::add_pointer<typename boost::binary_traits<T>::second_argument_type>::type,typename boost::binary_traits<T>::result_type>
{
	typename boost::binary_traits<T>::result_type operator()(typename boost::add_pointer<typename boost::binary_traits<T>::first_argument_type>::type x,typename boost::add_pointer<typename boost::binary_traits<T>::second_argument_type>::type y)
		{return T()(*x,*y);};
};

template<typename T, size_t N1>
struct addref_binary_functor<T,N1,0> : public std::binary_function<typename addref_power<typename boost::binary_traits<T>::first_argument_type,N1>::type,typename boost::binary_traits<T>::second_argument_type,typename boost::binary_traits<T>::result_type>
{
	typename boost::binary_traits<T>::result_type operator()(typename addref_power<typename boost::binary_traits<T>::first_argument_type,N1>::type x,typename boost::binary_traits<T>::second_argument_type y)
		{return addref_binary_functor<T,N1-1,0>(*x,y);};
};

template<typename T>
struct addref_binary_functor<T,1,0> : public std::binary_function<typename boost::add_pointer<typename boost::binary_traits<T>::first_argument_type>::type,typename boost::binary_traits<T>::second_argument_type,typename boost::binary_traits<T>::result_type>
{
	typename boost::binary_traits<T>::result_type operator()(typename boost::add_pointer<typename boost::binary_traits<T>::first_argument_type>::type x,typename boost::binary_traits<T>::second_argument_type y)
		{return T()(*x,y);};
};

template<typename T, size_t N2>
struct addref_binary_functor<T,0,N2> : public std::binary_function<typename boost::binary_traits<T>::first_argument_type,typename addref_power<typename boost::binary_traits<T>::second_argument_type,N2>::type,typename boost::binary_traits<T>::result_type>
{
	typename boost::binary_traits<T>::result_type operator()(typename boost::binary_traits<T>::first_argument_type x,typename addref_power<typename boost::binary_traits<T>::second_argument_type,N2>::type y)
		{return addref_binary_functor<T,0,N2-1>(x,*y);};
};

template<typename T>
struct addref_binary_functor<T,0,1> : public std::binary_function<typename boost::binary_traits<T>::first_argument_type,typename boost::add_pointer<typename boost::binary_traits<T>::second_argument_type>::type,typename boost::binary_traits<T>::result_type>
{
	typename boost::binary_traits<T>::result_type operator()(typename boost::binary_traits<T>::first_argument_type x,typename boost::add_pointer<typename boost::binary_traits<T>::second_argument_type>::type y)
		{return T()(x,*y);};
};

template<typename T>
struct addref_binary_functor<T,0,0> : public std::binary_function<typename boost::binary_traits<T>::first_argument_type,typename boost::binary_traits<T>::second_argument_type,typename boost::binary_traits<T>::result_type>
{
	typename boost::binary_traits<T>::result_type operator()(typename boost::binary_traits<T>::first_argument_type x,typename boost::binary_traits<T>::second_argument_type y)
		{return T()(x,y);};
};

// higher-order function information
template <class _Arg1, class _Arg2, class _Arg3, class _Result>
struct ternary_function
{
      typedef _Arg1 first_argument_type;   ///< the type of the first argument
      typedef _Arg2 second_argument_type;  ///< the type of the second argument
      typedef _Arg3 third_argument_type;  ///< the type of the second argument
      typedef _Result result_type;         ///< type of the return type
};

namespace detail {

template <class Operation>
struct ternary_traits_imp;

template <class Operation>
struct ternary_traits_imp<Operation*>
{
    typedef Operation                                function_type;
    typedef const function_type &                    param_type;
    typedef typename Operation::result_type          result_type;
    typedef typename Operation::first_argument_type  first_argument_type;
    typedef typename Operation::second_argument_type second_argument_type;
    typedef typename Operation::third_argument_type  third_argument_type;
};
        
template <class R, class A1, class A2,class A3>
struct ternary_traits_imp<R(*)(A1,A2,A3)>
{
    typedef R (*function_type)(A1,A2,A3);
    typedef R (*param_type)(A1,A2,A3);
    typedef R result_type;
    typedef A1 first_argument_type;
    typedef A2 second_argument_type;
    typedef A3 third_argument_type;
};
}	// end namespace detail

template <class Operation>
struct ternary_traits
{
        typedef typename detail::ternary_traits_imp<Operation*>::function_type        function_type;
        typedef typename detail::ternary_traits_imp<Operation*>::param_type           param_type;
        typedef typename detail::ternary_traits_imp<Operation*>::result_type          result_type;
        typedef typename detail::ternary_traits_imp<Operation*>::first_argument_type  first_argument_type;
        typedef typename detail::ternary_traits_imp<Operation*>::second_argument_type second_argument_type;
        typedef typename detail::ternary_traits_imp<Operation*>::third_argument_type  third_argument_type;
};
    
template <class R, class A1, class A2, class A3>
struct ternary_traits<R(*)(A1,A2,A3)>
{
	typedef R (*function_type)(A1,A2,A3);
	typedef R (*param_type)(A1,A2,A3);
	typedef R result_type;
	typedef A1 first_argument_type;
	typedef A2 second_argument_type;
	typedef A3 third_argument_type;
};


// function adapters
// for the moment, don't allow binding all arguments
// unary
template<class T,size_t SelfIdx=0>
struct func_unary : public std::unary_function<
							typename boost::unary_traits<T>::argument_type,
							typename boost::unary_traits<T>::result_type>
{	// this one catches syntax errors, we should not be catching self-ops or binding here
	BOOST_STATIC_ASSERT(0<SelfIdx);
	typename boost::unary_traits<T>::result_type operator()(typename boost::unary_traits<T>::argument_type x) { return T(x);};
};

// SelfIdx 1 causes argument to loop back (self-operation)
template<class T>
struct func_unary<T,1> : public std::unary_function<typename boost::call_traits<typename boost::remove_cv<typename boost::unary_traits<T>::argument_type>::type>::type,void>
{
	void operator()(typename boost::call_traits<typename boost::remove_cv<typename boost::unary_traits<T>::argument_type>::type>::reference x) { x = T(x);};
};

// binary
template<typename T,size_t SelfIdx=0,size_t BindIdx=0>
struct func_binary : public std::binary_function<typename boost::binary_traits<T>::first_argument_type,typename boost::binary_traits<T>::second_argument_type,typename boost::binary_traits<T>::result_type>
{
	BOOST_STATIC_ASSERT((!boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	typename boost::binary_traits<T>::result_type operator()(	typename boost::binary_traits<T>::first_argument_type x,
																typename boost::binary_traits<T>::second_argument_type y) { return T(x,y);};
};

// SelfIdx 1, 2 cause argument to loop back (self-operation)
template<typename T>
struct func_binary<T,1> : public std::binary_function<typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::first_argument_type>::type>::type,typename boost::binary_traits<T>::second_argument_type,void>
{
	BOOST_STATIC_ASSERT((!boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	void operator()(	typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::first_argument_type>::type>::reference x,
						typename boost::binary_traits<T>::second_argument_type y) { x = T(x,y);};
};

template<typename T>
struct func_binary<T,2> : public std::binary_function<typename boost::binary_traits<T>::first_argument_type,typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::second_argument_type>::type>::type,void>
{
	BOOST_STATIC_ASSERT((!boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	void operator()(	typename boost::binary_traits<T>::first_argument_type x,
						typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::second_argument_type>::type>::reference y) { y = T(x,y);};
};

// BindIdx 1, 2 bind arguments
template<typename T>
struct func_binary<T,0,1> : public std::unary_function<typename boost::binary_traits<T>::second_argument_type,typename boost::binary_traits<T>::result_type>
{
private:
	BOOST_STATIC_ASSERT((!boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	typename boost::remove_cv<
		typename boost::remove_reference<
			typename boost::binary_traits<T>::first_argument_type>::type>::type _Arg1;
public:
	func_binary<T,0,1>(typename boost::call_traits<typename boost::binary_traits<T>::first_argument_type>::param_type x) : _Arg1(x) {};
	typename boost::binary_traits<T>::result_type operator()(typename boost::binary_traits<T>::second_argument_type x) {return T(_Arg1,x);};
};

template<typename T>
struct func_binary<T,0,2> : public std::unary_function<typename boost::binary_traits<T>::first_argument_type,typename boost::binary_traits<T>::result_type>
{
private:
	BOOST_STATIC_ASSERT((!boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	typename boost::remove_cv<
		typename boost::remove_reference<
			typename boost::binary_traits<T>::second_argument_type>::type>::type _Arg2;
public:
	func_binary<T,0,2>(typename boost::call_traits<typename boost::binary_traits<T>::second_argument_type>::param_type x) : _Arg2(x) {};
	typename boost::binary_traits<T>::result_type operator()(typename boost::binary_traits<T>::first_argument_type x) {return T(x,_Arg2);};
};

// of course, we can do both to different arguments
template<typename T>
struct func_binary<T,2,1> : public std::unary_function<typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::second_argument_type>::type>::reference,void>
{
private:
	BOOST_STATIC_ASSERT((!boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	typename boost::remove_cv<
		typename boost::remove_reference<
			typename boost::binary_traits<T>::first_argument_type>::type>::type _Arg1;
public:
	func_binary<T,2,1>(typename boost::call_traits<typename boost::binary_traits<T>::first_argument_type>::param_type x) : _Arg1(x) {};
	void operator()(typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::second_argument_type>::type>::reference x) {return T(_Arg1,x);};
};

template<typename T>
struct func_binary<T,1,2> : public std::unary_function<typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::first_argument_type>::type>::reference,void>
{
private:
	BOOST_STATIC_ASSERT((!boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	typename boost::remove_cv<
		typename boost::remove_reference<
			typename boost::binary_traits<T>::second_argument_type>::type>::type _Arg2;
public:
	func_binary<T,1,2>(typename boost::call_traits<typename boost::binary_traits<T>::second_argument_type>::param_type x) : _Arg2(x) {};
	void operator()(typename boost::call_traits<typename boost::remove_cv<typename boost::binary_traits<T>::first_argument_type>::reference>::type x) {return T(x,_Arg2);};
};

// void binary function objects
// obviously, loopback doesn't mean anything
template<class T,size_t BindIdx=0>
struct void_funcobj_binary : public std::binary_function<typename boost::binary_traits<T>::first_argument_type,typename boost::binary_traits<T>::second_argument_type,typename boost::binary_traits<T>::result_type>,public T
{
	BOOST_STATIC_ASSERT((boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
};

// BindIdx 1, 2 bind arguments
template<class T>
struct void_funcobj_binary<T,1> : public std::unary_function<typename boost::binary_traits<T>::second_argument_type,typename boost::binary_traits<T>::result_type>, public T
{
private:
	BOOST_STATIC_ASSERT((boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	typename boost::remove_cv<
		typename boost::remove_reference<
			typename boost::binary_traits<T>::first_argument_type>::type>::type _Arg1;
public:
	void_funcobj_binary<T,1>(typename boost::call_traits<typename boost::binary_traits<T>::first_argument_type>::param_type x) : _Arg1(x) {};
	void operator()(typename boost::binary_traits<T>::second_argument_type x) {T::operator()(_Arg1,x);};
};

template<class T>
struct void_funcobj_binary<T,2> : public std::unary_function<typename boost::binary_traits<T>::first_argument_type,typename boost::binary_traits<T>::result_type>, public T
{
private:
	BOOST_STATIC_ASSERT((boost::is_same<typename boost::binary_traits<T>::result_type,void>::value));
	typename boost::remove_cv<
		typename boost::remove_reference<
			typename boost::binary_traits<T>::second_argument_type>::type>::type _Arg2;
public:
	void_funcobj_binary<T,2>(typename boost::call_traits<typename boost::binary_traits<T>::second_argument_type>::param_type x) : _Arg2(x) {};
	void operator()(typename boost::binary_traits<T>::first_argument_type x) {T::operator()(x,_Arg2);};
};

// ternary
template<typename T,size_t SelfIdx=0,size_t BindIdx=0>
struct func_ternary : public ternary_function<typename ternary_traits<T>::first_argument_type,typename ternary_traits<T>::second_argument_type,typename ternary_traits<T>::third_argument_type,typename ternary_traits<T>::result_type>
{
	typename ternary_traits<T>::result_type operator()(	typename ternary_traits<T>::first_argument_type x,
														typename ternary_traits<T>::second_argument_type y,
														typename ternary_traits<T>::third_argument_type z) { return T(x,y,z);};
};

// SelfIdx 1, 2, 3 cause argument to loop back (self-operation)
template<typename T>
struct func_ternary<T,1> : public ternary_function<typename ternary_traits<T>::first_argument_type,typename ternary_traits<T>::second_argument_type,typename ternary_traits<T>::third_argument_type,typename ternary_traits<T>::result_type>
{
	void operator()(	typename boost::call_traits<typename boost::remove_cv<typename ternary_traits<T>::first_argument_type>::type>::reference x,
						typename ternary_traits<T>::second_argument_type y,
						typename ternary_traits<T>::third_argument_type z)  { x = T(x,y,z);};
};

template<typename T>
struct func_ternary<T,2> : public ternary_function<typename ternary_traits<T>::first_argument_type,typename ternary_traits<T>::second_argument_type,typename ternary_traits<T>::third_argument_type,typename ternary_traits<T>::result_type>
{
	void operator()(	typename ternary_traits<T>::first_argument_type x,
						typename boost::call_traits<typename boost::remove_cv<typename ternary_traits<T>::second_argument_type>::type>::reference y,
						typename ternary_traits<T>::third_argument_type z)  { y = T(x,y,z);};
};

template<typename T>
struct func_ternary<T,3> : public ternary_function<typename ternary_traits<T>::first_argument_type,typename ternary_traits<T>::second_argument_type,typename ternary_traits<T>::third_argument_type,typename ternary_traits<T>::result_type>
{
	void operator()(	typename ternary_traits<T>::first_argument_type x,
						typename ternary_traits<T>::second_argument_type y,
						typename boost::call_traits<typename boost::remove_cv<typename ternary_traits<T>::third_argument_type>::type>::reference z)  { z = T(x,y,z);};
};

//! \todo BindIdx 1, 2, 3, 4, 5, 6 bind arguments 1,2,3,(1,2),(1,3),(2,3)
//! \todo of course, we can do both


}	// end namespace zaimoni

#endif
