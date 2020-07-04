// LowRel.cxx
// header for low-level relations

#include "LowRel.hxx"
#include "MetaCon1.hxx"

// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
#define DECLARE_BINARY_RELATION(A)	\
bool	\
A(const MetaConcept& LHS, const MetaConcept& RHS)	\
{	\
	return LHS.A(RHS);	\
}

// Arity 2, symmetric
bool
AreSyntacticallyEqual(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/11/1999
	return LHS==RHS;
}

bool
AreSyntacticallyUnequal(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/4/2002
	return !(LHS==RHS);
}

bool IsSyntacticallyEqualOrAntiIdempotentTo(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	return lhs==rhs || IsAntiIdempotentTo(lhs,rhs);
}

bool
IsStdAdditionInverseTo(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/15/2000
	return LHS.SelfInverseTo(RHS,StdAddition_MC);
}

bool
IsStdMultiplicationInverseTo(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/6/2003
	return LHS.SelfInverseTo(RHS,StdMultiplication_MC);
}

// Arity 2, antisymmetric
DECLARE_BINARY_RELATION(StrictlyImplies)
DECLARE_BINARY_RELATION(CanStrictlyModify)

#undef DECLARE_BINARY_RELATION

bool NonStrictlyImpliesThisOrLogicalNOTOf(const MetaConcept& lhs, const MetaConcept& rhs)
{
	return lhs==rhs || IsAntiIdempotentTo(lhs,rhs)
		|| lhs.StrictlyImplies(rhs)
		|| StrictlyImpliesLogicalNOTOf(lhs,rhs);
}

bool NonStrictlyImplies(const MetaConcept& lhs, const MetaConcept& rhs)
{
	return lhs==rhs || lhs.StrictlyImplies(rhs);
}

bool NonStrictlyImpliesLogicalNOTOf(const MetaConcept& lhs, const MetaConcept& rhs)
{
	return IsAntiIdempotentTo(lhs,rhs)
		|| StrictlyImpliesLogicalNOTOf(lhs,rhs);
}

bool LogicalNOTOfNonStrictlyImplies(const MetaConcept& lhs, const MetaConcept& rhs)
{
	return IsAntiIdempotentTo(lhs,rhs)
		|| LogicalNOTOfStrictlyImplies(lhs,rhs);
}

bool
CanDeepStrictlyModify(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2003
	return RHS.HasArgRelatedToThisConceptBy(LHS,::CanStrictlyModify);
}

bool
DeepLogicallyImplies(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2003
	return RHS.HasArgRelatedToThisConceptBy(LHS,NonStrictlyImpliesThisOrLogicalNOTOf);
}

// Actions
void
SetLHSToRHS(MetaConcept*& Target, const MetaConcept& Inducer)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	Inducer.CopyInto(Target);
}

void
SetLHSToLogicalNOTOfRHS(MetaConcept*& Target, const MetaConcept& Inducer)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	Inducer.CopyInto(Target);
	Target->SelfLogicalNOT();
}

void
SetLHSToStdAdditionInverseOfRHS(MetaConcept*& Target, const MetaConcept& Inducer)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/6/2003
	Inducer.CopyInto(Target);
	Target->SelfInverse(StdAddition_MC);	// safe
}

void
SetLHSToStdMultiplicationInverseOfRHS(MetaConcept*& Target, const MetaConcept& Inducer)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/6/2003
	if (!Inducer.IsZero())
		{
		Inducer.CopyInto(Target);
		Target->SelfInverse(StdMultiplication_MC);	// safe when IsZero() fails
		}
	else
		throw bad_alloc();
}
