// boost_core.hpp
// (C)2009,2015 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_BOOST_CORE_HPP
#define ZAIMONI_BOOST_CORE_HPP 1

// critical: type_traits, static_assert, call_traits
#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/call_traits.hpp>

// use these macros from within the Boost namespace

// ZAIMONI_POD_STRUCT
#if BOOST_IS_POD==false
#define ZAIMONI_POD_STRUCT(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct is_pod<TYPE_SPEC > : public is_pod<PASS_SPEC > {};
#else
#define ZAIMONI_POD_STRUCT(TEMPLATE_SPEC,TYPE_SPEC,DERIVE_SPEC)
#endif

// ZAIMONI_POD
#if BOOST_HAS_TRIVIAL_DESTRUCTOR==false
#if BOOST_HAS_TRIVIAL_COPY==false
#if BOOST_HAS_TRIVIAL_ASSIGN==false
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct has_trivial_assign<TYPE_SPEC > : public has_trivial_assign<PASS_SPEC > {};	\
TEMPLATE_SPEC struct has_trivial_copy<TYPE_SPEC > : public has_trivial_copy<PASS_SPEC > {};	\
TEMPLATE_SPEC struct has_trivial_destructor<TYPE_SPEC > : public has_trivial_destructor<PASS_SPEC > {};
#else
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct has_trivial_copy<TYPE_SPEC > : public has_trivial_copy<PASS_SPEC > {};	\
TEMPLATE_SPEC struct has_trivial_destructor<TYPE_SPEC > : public has_trivial_destructor<PASS_SPEC > {};
#endif
#else
#if BOOST_HAS_TRIVIAL_ASSIGN==false
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct has_trivial_assign<TYPE_SPEC > : public has_trivial_assign<PASS_SPEC > {};	\
TEMPLATE_SPEC struct has_trivial_destructor<TYPE_SPEC > : public has_trivial_destructor<PASS_SPEC > {};
#else
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct has_trivial_destructor<TYPE_SPEC > : public has_trivial_destructor<PASS_SPEC > {};
#endif
#endif
#else
#if BOOST_HAS_TRIVIAL_COPY==false
#if BOOST_HAS_TRIVIAL_ASSIGN==false
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct has_trivial_assign<TYPE_SPEC > : public has_trivial_assign<PASS_SPEC > {};	\
TEMPLATE_SPEC struct has_trivial_copy<TYPE_SPEC > : public has_trivial_copy<PASS_SPEC > {};
#else
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct has_trivial_copy<TYPE_SPEC > : public has_trivial_copy<PASS_SPEC > {};
#endif
#else
#if BOOST_HAS_TRIVIAL_ASSIGN==false
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)	\
TEMPLATE_SPEC struct has_trivial_assign<TYPE_SPEC > : public has_trivial_assign<PASS_SPEC > {};
#else
#define ZAIMONI_POD(TEMPLATE_SPEC,TYPE_SPEC,PASS_SPEC)
#endif
#endif
#endif

#endif
