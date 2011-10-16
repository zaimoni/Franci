// __GCF.hpp
// GCF disconnected from Franci, for _IntegerNumeral support

#ifndef __GCF_HPP
#define __GCF_HPP 1

#include "Zaimoni.STL/AutoPtr.hpp"
#include "__IntegerNumeral.hxx"

class _GCF
{
public:
	_GCF(_IntegerNumeral**& NewArgList);
//	default OK: copy constructor, assignment, destructor

	void MoveInto(_GCF*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
	void ForceStdForm();

//  Evaluation functions
	bool SyntaxOK() const;							
	bool IsOne() const {return ArgArray[0]->IsOne();};
	bool IsZero() const {return ArgArray[0]->IsZero();};	// relies on ForceStdForm to work

	_IntegerNumeral& operator()();
private:
	zaimoni::autovalarray_ptr_throws<_IntegerNumeral*> ArgArray;
	_GCF() {};
};

#endif

