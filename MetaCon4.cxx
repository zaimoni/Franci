// MetaCon4.cxx
// implementation of MetaConceptWith2Args

#include "MetaCon4.hxx"
#include "Quantify.hxx"
#include "LowRel.hxx"

MetaConceptWith2Args::EvaluateToOtherRule MetaConceptWith2Args::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
  =	{
	&MetaConceptWith2Args::ForceLHSArg1,
	&MetaConceptWith2Args::ForceRHSArg2
	};

MetaConceptWith2Args::SelfEvaluateRule MetaConceptWith2Args::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&MetaConceptWith2Args::EvaluateLHSArg1,
	&MetaConceptWith2Args::EvaluateRHSArg2,
	&MetaConceptWith2Args::EvaluateBothArgs
	};

// rest of implementation
MetaConceptWith2Args::MetaConceptWith2Args(const MetaConceptWith2Args& src)
:	MetaConcept(src),
	LHS_Arg1(src.LHS_Arg1),
	RHS_Arg2(src.RHS_Arg2),
	IdxCurrentEvalRule(src.IdxCurrentEvalRule),
	IdxCurrentSelfEvalRule(src.IdxCurrentSelfEvalRule)
{
}

void MetaConceptWith2Args::MoveIntoAux(MetaConceptWith2Args& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/29/2006
	dest.MetaConcept::operator=(*this);
	LHS_Arg1.MoveInto(dest.LHS_Arg1);
	RHS_Arg2.MoveInto(dest.RHS_Arg2);
	dest.IdxCurrentEvalRule=IdxCurrentEvalRule;
	dest.IdxCurrentSelfEvalRule=IdxCurrentSelfEvalRule;
}

void MetaConceptWith2Args::operator=(const MetaConceptWith2Args& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/29/2006
	autoval_ptr<MetaConcept> Tmp(src.LHS_Arg1);
	RHS_Arg2 = src.RHS_Arg2;
	Tmp.MoveInto(LHS_Arg1);

	// now safe
	MetaConcept::operator=(src);
	IdxCurrentEvalRule = src.IdxCurrentEvalRule,
	IdxCurrentSelfEvalRule = src.IdxCurrentSelfEvalRule;
}

// FORMALLY CORRECT: Kenneth Boyd, 9/22/1998
#define ARGN_BODY	\
	return (0 == n) ? LHS_Arg1	\
         : (1 == n) ? RHS_Arg2 : (MetaConcept*)NULL;

STANDARD_DECLARE_ARGN(MetaConceptWith2Args,ARGN_BODY)

#undef ARGN_BODY

bool MetaConceptWith2Args::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	const MetaConceptWith2Args& VR_rhs = static_cast<const MetaConceptWith2Args&>(rhs);
	if (IsUltimateType(rhs.UltimateType()))
		{
		if (   *LHS_Arg1==*VR_rhs.LHS_Arg1
			&& *RHS_Arg2==*VR_rhs.RHS_Arg2)
			return true;
		if (   IsSymmetric()
		    && *LHS_Arg1==*VR_rhs.RHS_Arg2
			&& *RHS_Arg2==*VR_rhs.LHS_Arg1)
			return true;
		}
	return false;
}

bool MetaConceptWith2Args::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	const MetaConceptWith2Args& VR_RHS = static_cast<const MetaConceptWith2Args&>(rhs);
	// type is known to be the same
	if (LHS_Arg1->InternalDataLT(*VR_RHS.LHS_Arg1)) return true;
	return !VR_RHS.LHS_Arg1->InternalDataLT(*LHS_Arg1)
		&& RHS_Arg2->InternalDataLT(*VR_RHS.RHS_Arg2);
}

bool MetaConceptWith2Args::_IsExplicitConstant() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/10/2000
	return LHS_Arg1->IsExplicitConstant()
        && RHS_Arg2->IsExplicitConstant();
}

bool MetaConceptWith2Args::IsAbstractClassDomain() const
{
	return LHS_Arg1->IsAbstractClassDomain()
        && RHS_Arg2->IsAbstractClassDomain();
}

void MetaConceptWith2Args::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/2/2005
	if (!RHS_Arg2.empty()) RHS_Arg2->ForceStdForm();
	if (!LHS_Arg1.empty())
		{
		LHS_Arg1->ForceStdForm();
		if (IsSymmetric())
			{
			if (RHS_Arg2.empty())
				{
				LHS_Arg1.MoveInto(RHS_Arg2);
				return;
				}
			else if (RHS_Arg2->InternalDataLT(*LHS_Arg1))
				swap(LHS_Arg1,RHS_Arg2);
			return;
			}
		}		
}

bool MetaConceptWith2Args::CanEvaluate() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/23/2002
	const_cast<MetaConceptWith2Args* const>(this)->DiagnoseInferenceRules();
	if (None_SER>IdxCurrentSelfEvalRule) return false;
	return IdxCurrentEvalRule || IdxCurrentSelfEvalRule;
}

bool MetaConceptWith2Args::CanEvaluateToSameType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/23/2002
	const_cast<MetaConceptWith2Args* const>(this)->DiagnoseInferenceRules();
	if (None_SER>IdxCurrentSelfEvalRule) return false;
	return IdxCurrentSelfEvalRule;
}

bool MetaConceptWith2Args::Evaluate(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, Jan. 29 2016
	// NOTE: there is exactly one call to Evaluate in Franci.  This call is preceded by
	// a call to CanEvaluate()...so the rules have been initialized, and are guaranteed
	// to be appropriate.  The SelfEvaluate rules have been already done, so this is not
	// required either.
	// There is no need to diagnose the inference rules.
	DEBUG_LOG("Entering MetaConceptWith2Args::Evaluate");
	DEBUG_LOG((intmax_t)IdxCurrentSelfEvalRule);
	DEBUG_LOG((intmax_t)IdxCurrentEvalRule);
	try {
		bool RetVal = (MaxEvalRuleIdx_ER>=IdxCurrentEvalRule) ? (this->*EvaluateRuleLookup[IdxCurrentEvalRule-1])(dest)
					: DelegateEvaluate(dest);
		DEBUG_LOG(*dest);
		return RetVal;
		}
	catch(const std::bad_alloc&)
		{
		DEBUG_LOG("MetaConceptWith2Args::Evaluate failed: std::bad_alloc()");
		return false;
		}
}

bool MetaConceptWith2Args::DestructiveEvaluateToSameType()
{	// FORMALLY CORRECT: Kenneth Boyd, Jan. 29 2016
	DEBUG_LOG(__PRETTY_FUNCTION__);
	DiagnoseInferenceRules();
	DEBUG_LOG("DiagnoseInferenceRules OK");
	DEBUG_LOG(name());
	DEBUG_LOG(IdxCurrentSelfEvalRule);
	if (IdxCurrentSelfEvalRule)
		{
		try	{
			const bool Tmp = (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-1])();
			DEBUG_LOG("selfeval worked");
			SUCCEED_OR_DIE(SyntaxOK());
			DEBUG_LOG(*this);
			return Tmp;
			}
		catch(const std::bad_alloc&)
			{
			DEBUG_LOG("MetaConceptWith2Args::DestructiveEvaluateToSameType failed: std::bad_alloc()");
			return false;	
			}
		}
	return false;
}

void MetaConceptWith2Args::ConvertVariableToCurrentQuantification(MetaQuantifier& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/17/1999
	if (!LHS_Arg1.empty())
		LHS_Arg1->ConvertVariableToCurrentQuantification(src);
	if (!RHS_Arg2.empty())
		RHS_Arg2->ConvertVariableToCurrentQuantification(src);
}

bool MetaConceptWith2Args::UsesQuantifierAux(const MetaQuantifier& x) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/8/1999
	return    x.MetaConceptPtrUsesThisQuantifier(LHS_Arg1)
		   || x.MetaConceptPtrUsesThisQuantifier(RHS_Arg2);
}

bool MetaConceptWith2Args::OrderIndependentPairwiseRelation(const MetaConceptWith2Args& rhs, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/1999
	if		(TargetRelation(*LHS_Arg1,*rhs.LHS_Arg1))
		return TargetRelation(*RHS_Arg2,*rhs.RHS_Arg2);
	else if (TargetRelation(*LHS_Arg1,*rhs.RHS_Arg2))
		return TargetRelation(*RHS_Arg2,*rhs.LHS_Arg1);
	return false;
}

bool
MetaConceptWith2Args::ExactOrderPairwiseRelation(const MetaConceptWith2Args& rhs, LowLevelBinaryRelation* TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/1999
	return TargetRelation(*LHS_Arg1,*rhs.LHS_Arg1)
		&& TargetRelation(*RHS_Arg2,*rhs.RHS_Arg2);
}

void MetaConceptWith2Args::ForceTotalLexicalArgOrder()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/2/2005
	assert(IsSymmetric());
	if (RHS_Arg2->InternalDataLT(*LHS_Arg1)) swap(LHS_Arg1,RHS_Arg2);
}

//! \todo metadiagnostic function for routing these three (EvaluateLHSArg1, EvaluateRHSArg2, EvaluateBothArgs)
bool MetaConceptWith2Args::EvaluateLHSArg1()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2006
	assert(!LHS_Arg1.empty());
	assert(LHS_Arg1->CanEvaluate());
	if (DestructiveSyntacticallyEvaluateOnce(LHS_Arg1))
		{
		assert(SyntaxOK());
		IdxCurrentSelfEvalRule = None_SER;		
		return true;
		};
	return false;
}

bool MetaConceptWith2Args::EvaluateRHSArg2()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2006
	assert(!RHS_Arg2.empty());
	assert(RHS_Arg2->CanEvaluate());
	if (DestructiveSyntacticallyEvaluateOnce(RHS_Arg2))
		{
		assert(SyntaxOK());
		IdxCurrentSelfEvalRule = None_SER;		
		return true;
		};
	return false;
}

bool MetaConceptWith2Args::EvaluateBothArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2006
	VERIFY(	RHS_Arg2.empty() || LHS_Arg1.empty()
			|| !RHS_Arg2->CanEvaluate() || !LHS_Arg1->CanEvaluate(),AlphaCallAssumption);
	bool Flag = false;
	// NOTE: Franci interprets a single evaluation as a "partial success", which is
	// *not* fatal...but is worth a warning.
	if (DestructiveSyntacticallyEvaluateOnce(LHS_Arg1)) Flag = true;
	if (DestructiveSyntacticallyEvaluateOnce(RHS_Arg2)) Flag = true;
	if (Flag)
		{
		assert(SyntaxOK());
		IdxCurrentSelfEvalRule = None_SER;		
		return true;
		};
	return false;
}

bool MetaConceptWith2Args::ForceLHSArg1(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2006
	assert(!dest);
	dest = LHS_Arg1.release();
	return true;
}

bool MetaConceptWith2Args::ForceRHSArg2(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2006
	assert(!dest);
	dest = RHS_Arg2.release();
	return true;
}

