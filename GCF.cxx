// GCF.cxx
// implementation for the class GCF, which handles Greatest Common Factor.

#include "Class.hxx"
#include "Integer1.hxx"
#include "GCF.hxx"

#include <functional>

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

GCF::GCF(MetaConcept**& NewArgList)
:	MetaConceptWithArgArray(GCF_MC,NewArgList)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/11/2001
	if (SyntaxOK()) _forceStdForm();
}

void GCF::MoveInto(GCF*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/29/2007
	if (!dest) dest = new GCF;
	MoveIntoAux(*dest);
}

const AbstractClass* GCF::UltimateType() const
{	//! \todo FIX this when more interesting rings show up (union)
	return &Integer;
}

void GCF::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/10/2001
	if (!ArgArray.empty() && !IdxCurrentSelfEvalRule && !IdxCurrentEvalRule)
		{
		size_t i = fast_size();
		do	if (ArgArray[--i]->IsNegative())
				ArgArray[i]->SelfInverse(StdAddition_MC);
		while(0<i);
		ForceStdFormAux();
		ForceTotalLexicalArgOrder();
		// zap leading zeros when 2+ args
		if (ArgArray[0]->IsZero() && 3<=fast_size())
			{
			i = 1;	// strict upper bound of zeros
			while(ArgArray[i]->IsZero() && fast_size()-2> ++i);
			InferenceParameter1 = i;
			SelfEvalRuleCleanLeadingArg();
			};
		};
}

bool GCF::SyntaxOK() const
{	//! \todo FIX when more interesting rings come along (all rings where primes are defined)
	if (SyntaxOKAux())	// NOTE: this routine catches NULL entries
		return and_range_n([](MetaConcept* x) {return x->IsUltimateType(&Integer); }, ArgArray.begin(), fast_size());
	return false;
}

bool GCF::_IsOne() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/13/2001
	return ArgArray[0]->IsOne();
}

bool GCF::_IsZero() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/12/2001
	if (   2==fast_size() && ArgArray[1]->IsZero()
		&& ArgArray[0]->IsZero())
		return true;
	return false;
}

bool
GCF::ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/4/04
	if (ArgArray[ArgIdx]->IsOne())
		{
		SelfEvalRule = None_SER;
		EvalRule = EvalForceArg_ER;
		return true;
		}
	return false;
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > GCF::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

void GCF::DiagnoseInferenceRules() const
{	//! \todo IMPLEMENT
	// These rules are domain-independent
	if (ArgArray[0]->IsOne())
		{	// It's 1!
		InvokeEvalForceArg(0);
		return;
		}
	if (2==fast_size() && ArgArray[0]->IsZero())
		{	// GCF(0,a) = |a| (took |a| earlier)
		InvokeEvalForceArg(1);
		return;
		}
	//! \todo META: restrict these to domain _Z_/reevaluate when domain augmented
	if (   ArgArray[1]->IsExactType(IntegerNumeral_MC)
		&& static_cast<IntegerNumeral*>(ArgArray[0])->ResetLHSRHSToGCF(*static_cast<IntegerNumeral*>(ArgArray[1])))
		{
		if (2==fast_size())
			{
			InvokeEvalForceArg(1);
			return;
			}
		else{
			InferenceParameter1 = 1;
			IdxCurrentSelfEvalRule = SelfEvalRuleCleanArg_SER;
			return;
			}
		}
	if (DiagnoseStandardEvalRules()) return;
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
	return;	
}

bool GCF::InvokeEqualArgRule() const
{	// GCF::InvokeEqualArgRule cancels excess args
	if (2==fast_size())
		{
		IdxCurrentEvalRule = EvalForceArg_ER;
		}
	else{	//! \todo FIX: use an idempotent-clean-all-at-once rule instead of SelfEvalRuleCleanArg_SER
		IdxCurrentSelfEvalRule = SelfEvalRuleCleanArg_SER;		
		}
	return true;
}

