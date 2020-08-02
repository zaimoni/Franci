// SymConst.cxx
// implementation for SymbolicConstant

#include "Class.hxx"
#include "SymConst.hxx"

// Textual representations for infinity
// <font face="Symbol">&yen;</font>
// <font face="Symbol">&#165;</font>
// \u221E	[C preprocessor UNICODE]
// &infin;	[output mode; only one that works for input]

// META: We want to arrange those constants that are in the total 
// ordering for _R_ in that order.  In this scenario, we probably should require only 
// positive versions.

bool SymbolicConstant::SyntacticalStandardLTAux(const MetaConcept& rhs) const
{	// MUTABLE
	if 		(typeid(SymbolicConstant)==typeid(rhs))
		{	// handle here
		if (IsExactType(rhs.ExactType()))
			{	// same base constant
			//! \todo MultInv support for SymbolicConstant::SyntacticalStandardLTAux (functions don't handle this!) when we have 
			//! something that MultInv respects (pi or e)
			if (IsMetaAddInverted() && !rhs.IsMetaAddInverted())
				return true;
			}
		}
	else if (!rhs.IsUltimateType(NULL) && rhs.UltimateType()->Subclass(Real))
		{	// delegate
		return SymbolicConstantSyntacticalLTReal(rhs);
		}
	return false;
}

// These two will eventually be member function arrays
bool
SymbolicConstant::SymbolicConstantSyntacticalLTReal(const MetaConcept& rhs) const
{	// MUTABLE
	if (IsExactType(LinearInfinity_MC))
		{
		if (rhs.UltimateType()->Subclass(Real))
			{
			if (IsMetaAddInverted()) return true;
			}
		}
	return false;
}

bool
SymbolicConstant::SymbolicConstantSyntacticalGTReal(const MetaConcept& rhs) const
{	// MUTABLE
	if (IsExactType(LinearInfinity_MC))
		{
		if (rhs.UltimateType()->Subclass(Real))
			{
			if (!IsMetaAddInverted()) return true;
			}
		}
	return false;
}

bool SymbolicConstant::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/10/2002
	return false;
}

//  Type ID functions
const AbstractClass* SymbolicConstant::UltimateType() const
{	// MUTABLE
	// Ultimately will have to be an array lookup
	// if (IsExactType(LinearInfinity_MC))
	return &Integer;
}

// text I/O functions
std::string SymbolicConstant::to_s_aux() const
{
	// if (IsExactType(LinearInfinity_MC))
	return "&infin;";
}

//  Evaluation functions
bool SymbolicConstant::CanEvaluate() const {return false;}
bool SymbolicConstant::CanEvaluateToSameType() const {return false;}
bool SymbolicConstant::SyntaxOK() const {return true;}
bool SymbolicConstant::Evaluate(MetaConcept*& dest) {return false;}
bool SymbolicConstant::DestructiveEvaluateToSameType() {return false;}

// next two are ultimately member-function array-switched
// LinearInfinity_MC:
//		SelfInverse-* doesn't exist
// 		SelfInverse-+ exists, but doesn't "work" [inf-inf is undefined].  This is not 
//	a syntax error, but it should be very fatal to further evaluation of the expression.
bool SymbolicConstant::SelfInverse(const ExactType_MC Operation)
{	// MUTABLE
	// if (IsExactType(LinearInfinity_MC))
	if (StdMultiplication_MC==Operation)	//  *: no self-inverse possible
		return false;
	return MetaConcept::SelfInverse(Operation);	// +: OK
}

bool
SymbolicConstant::SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const
{	// MUTABLE
	// if (IsExactType(LinearInfinity_MC))
	//	+: not self-inverse to anything [inf-inf is undefined]
	//  *: no self-inverse possible
	return false;
}

bool SymbolicConstant::IsPositive() const	// needs total order *and* IsZero
{
//	if (LinearInfinity_MC>=ExactType())		// in extended reals, by assumption
//		{
		return !IsMetaAddInverted();
//		}
}

bool SymbolicConstant::IsNegative() const	// needs total order *and* IsZero
{
//	if (LinearInfinity_MC>=ExactType())		// in extended reals, by assumption
//		{
		return IsMetaAddInverted();
//		}
}


// MetaConcept members that need to be here
bool MetaConcept::SyntacticalStandardLT(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/19/2002
	if (   !IsUltimateType(NULL)
		&& !rhs.IsUltimateType(NULL)
		&& UltimateType()->HasStandardPartialOrdering()
		&& rhs.UltimateType()->HasStandardPartialOrdering())
		{
		if (SyntacticalStandardLTAux(rhs)) return true;
#if 0
		if (   MinSymbolicConstant_MC<=rhs.ExactType()
			&& MaxSymbolicConstant_MC>=rhs.ExactType())
			return static_cast<const SymbolicConstant&>(rhs).SymbolicConstantSyntacticalGTReal(*this);
#endif
		}
	return false;
}

