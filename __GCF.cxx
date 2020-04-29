// __GCF.cpp

#include "__GCF.hxx"
#include "Zaimoni.STL/algor.hpp"

using namespace zaimoni;

// NOTE: 0 is a "bad case" for the definition of GCF.
// Franci uses the following definitions to handle 0 and 1-ary cases
//	*	GCF(0,a)=|a|, a in _Z_
//	*	GCF(0,0)=0
//  *	GCF(a) = |a|, a in _Z_
//  These definitions enable Franci to consider GCF to be both commutative and associative
// without any holes in the definitions.
// Now, let us consider (_Z_,GCF,*) as a potential ring structure:
//	*	GCF has identity element 0 (good)
//	*   Multiplication by integers distributes over GCF (good)
//      a*GCF(b,c)=GCF(ab,ac)
//	*   Sticking point: GCF(a,...,a)=a regardless of how many a in _Z_ (that is, powers with GCF as operation cancel out)
//	*	Sticking point: no way to define GCF-inverse reasonably.  NO RING.
//  *	Sticking point: GCF has annihilator element: 1

_GCF::_GCF(_IntegerNumeral**& NewArgList)
:	ArgArray(NewArgList)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/11/2001
	if (SyntaxOK()) ForceStdForm();
}

void _GCF::MoveInto(_GCF*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/10/2001
	if (!dest) dest = new _GCF();
	ArgArray.MoveInto(dest->ArgArray);
}

void _GCF::ForceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/11/2007
	assert(!ArgArray.empty());

	size_t i = ArgArray.size();
	do	if (ArgArray[--i]->IsNegative())
			ArgArray[i]->SelfInverse(_IntegerNumeral::Addition);
	while(0<i);
	heapsort(ArgArray.c_array(),ArgArray.size());
	// zap leading zeros, except last when all args are zeros
	if (ArgArray[0]->IsZero())
		{
		i = 1;
		while(ArgArray[i]->IsZero() && ArgArray.size()-1> ++i);
		ArgArray.DeleteNSlotsAt(i,0);
		}
	// clear duplicate arguments
	i = ArgArray.size();
	while(0<--i)
		if (*ArgArray[i-1]==*ArgArray[i])
			ArgArray.DeleteIdx(i);
}

// template function target for Zaimoni.STL
bool ValidateArgArray(const _IntegerNumeral* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999 (twanged 7/11/2007)
	if (NULL!=ArgArray)
		{
		const size_t LocalArraySize = ArraySize(ArgArray);
		size_t i = LocalArraySize;
		do	if (NULL==ArgArray[--i])
				{
				INFORM("ALPHA-TEST ERROR: An n-ary concept was found with a NULL argument:");	// should be SEVERE_WARNING
				_fatal("Please contact tech support.  I QUIT!");
				}
		while(0<i);
		return and_range_n([](const _IntegerNumeral* x) { return x->SyntaxOK(); }, ArgArray, LocalArraySize);
		};
	return false;
}

bool _GCF::SyntaxOK() const
{
	if (ValidateArgArray(ArgArray))	// NOTE: this routine catches NULL entries
		return and_range([](const _IntegerNumeral* x) { return x->in_Z(); }, ArgArray.begin(), ArgArray.end());
	return false;
}

_IntegerNumeral&
_GCF::operator()()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/11/2007
	while(1<ArgArray.size())
		{
		if (ArgArray[0]->IsOne())
			// It's 1
			ArgArray.DeleteNSlotsAt(ArgArray.size()-1,1);
		else if (ArgArray[0]->ResetLHSRHSToGCF(*ArgArray[1]))
			ArgArray.DeleteIdx(1);
		};
	return *ArgArray[0];	
}

