// Variable.cxx
// implementation of class Variable

#include "Class.hxx"
#include "Variable.hxx"
#include "LowRel.hxx"

//! \todo We have severe conceptual problems.
//!       We need three types of class-set variables: predefined, class-descriptor,
//!       and set-descriptor.  This makes AbstractClass an abstract C++ class, with (at least)
//!       three derived classes.  The current coding is for predefined.  [Alternatively,
//!		 AbstractClass is multi-modal; this may (or may not) have uses.]
//!       Predefined: name, various data flags
//!       Class-descriptor: Free variable, proposition using only that free variable unbound
//!       Set-descriptorV1: Domained variable, proposition using only that domained variable unbound

// META: Variable's Arg1 points to a MetaQuantifier...so _msize takes its name's strlen()

void Variable::CopyInto(Variable*& dest) const
{
	if (!dest) dest = new Variable(*this);
	else *dest = *this;
}

bool Variable::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: 1/21/2003, Kenneth Boyd
	//! \todo ASSUMPTION: MetaConceptWith1Arg::EqualAux2 only checks for 
	//! syntactical equality of Arg1 members.  Variable needs a stronger test 
	//! to prevent false matches across scoping.
	//! NOTE: Comparing pointers directly still isn't safe in 32-bit i386: 
	//! there's a selector for each 4GB page
	return *Arg1==*(static_cast<const Variable&>(rhs).Arg1);
}

bool Variable::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	const Variable& VR_rhs = static_cast<const Variable&>(rhs);
	if (*Arg1!=*VR_rhs.Arg1) return Arg1->InternalDataLT(*VR_rhs.Arg1);

	// tie-breakers
	// TruthValues: A < NOT A; rely on correct syntax assumptions
	if (   !(MultiPurposeBitmap & LogicalNegated_VF)
		&& (VR_rhs.MultiPurposeBitmap & LogicalNegated_VF))
		return true;
	// UltimateType supports StdAddition: A < -A; rely on correct syntax assumptions
	if (   !(MultiPurposeBitmap & StdAdditionInv_VF)
		&& (VR_rhs.MultiPurposeBitmap & StdAdditionInv_VF))
		return true;
	return false;
}

//  Evaluation functions
bool Variable::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/19/2000
	if (   NULL!=Arg1
	    && typeid(MetaQuantifier)==typeid(*Arg1)
		&& Arg1->SyntaxOK()
		&& (!(LogicalNegated_VF & MultiPurposeBitmap) || IsUltimateType(&TruthValues))
		&& (!(StdAdditionInv_VF & MultiPurposeBitmap) || (!IsUltimateType(NULL) && UltimateType()->SupportsThisOperation(StdAddition_MC))))
		return true;
	return false;
}

void Variable::SelfLogicalNOT()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/27/1999
	if (IsUltimateType(&TruthValues))
		{
		MultiPurposeBitmap ^= LogicalNegated_VF;
		return;
		};
	MetaConcept::SelfLogicalNOT();
}

bool Variable::StrictlyImplies(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/13/1999
	if (!SelfLogicalNOTWorks()) return false;
	return MetaConcept::TValStrictlyImpliesDefault(rhs);
}

void Variable::ConvertVariableToCurrentQuantification(MetaQuantifier& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/17/1999
	if (src==*Arg1) Arg1 = &src;
}

// MetaQuantifier functions
bool MetaQuantifier::DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const
{
	assert(BasisClauseCount()>Idx);
	assert(NULL==dest);
	if (*Arg1==TruthValues)
		{	// handle this directly...range is 0..1
		dest = new(nothrow) Variable(const_cast<MetaQuantifier*>(this));
		if (NULL!=dest)
			{
			if (1==Idx) dest->SelfLogicalNOT();
			return true;
			}
		};
#if 0
	else{	// enumerated.  Delegate one arg of ALLEQUAL
		MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(2);
		if (NULL==NewArgArray) return false;
		if (!Arg1->DirectCreateBasisClauseIdx(Idx,NewArgArray[1]))
			{
			free(NewArgArray);
			return false;
			}

		try	{
			NewArgArray[0] = new Variable(const_cast<MetaQuantifier*>(this));
			dest = new EqualRelation(NewArgArray,ALLEQUAL_EM);
			}
		catch(const bad_alloc&)
			{
			BLOCKDELETEARRAY(NewArgArray);
			return false;
			}
		}
#endif
	return false;
}

