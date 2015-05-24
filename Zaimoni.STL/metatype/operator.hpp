// operator.hpp
// (C)2009,2011 Kenneth Boyd, license: MIT.txt

// some macros to assist in extension of operators
// modeled on <boost/operator.hpp>, this is a backup header

#ifndef ZAIMONI_STL_METATYPE_OPERATOR_HPP
#define ZAIMONI_STL_METATYPE_OPERATOR_HPP 1

// this defines non-member operators >, <=, >=
// use Boost when cross-subtype not needed
#define ZAIMONI_CROSSSUBTYPE_TOTAL_ORDERING(TEMPLATE_SPEC,CLASS_SPEC,CLASS_SPEC2)	\
	TEMPLATE_SPEC inline bool operator>(const CLASS_SPEC& x, const CLASS_SPEC2& y) {return y<x;}	\
	TEMPLATE_SPEC inline bool operator<=(const CLASS_SPEC& x, const CLASS_SPEC2& y) {return !(y<x);}	\
	TEMPLATE_SPEC inline bool operator>=(const CLASS_SPEC& x, const CLASS_SPEC2& y) {return !(x<y);}

#endif
