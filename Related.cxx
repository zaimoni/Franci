// Related.cxx
// implementation of HasArgRelatedToThisConceptBy(,) and
//	bool MetaConceptPtrRelatedToThisConceptBy(,)

#include "MetaCon2.hxx"
#include "MetaCon4.hxx"
#include "MetaCon5.hxx"
#include "MetaCon6.hxx"

#include "Variable.hxx"
#include "LowRel.hxx"

#if 0
void
ForceCheckForEvaluation(const MetaConcept* Target)
{
	const ExactType_MC Tmp = Target->ExactType();
	if (	(MinClauseNIdx_MC<=Tmp && MaxClauseNIdx_MC<=Tmp)
		||	(MinPhraseNIdx_MC<=Tmp && MaxPhraseNIdx_MC<=Tmp))
		static_cast<const MetaConceptWithArgArray*>(Target)->ForceCheckForEvaluation();
	else if (	(MinClause2Idx_MC<=Tmp && MaxClause2Idx_MC<=Tmp)
			||	(MinPhrase2Idx_MC<=Tmp && MaxPhrase2Idx_MC<=Tmp))
		static_cast<const MetaConceptWith2Args*>(Target)->ForceCheckForEvaluation();
}

void
ForceClauseCheckForEvaluation(const MetaConcept* Target)
{
	const ExactType_MC Tmp = Target->ExactType();
	if (MinClauseNIdx_MC<=Tmp && MaxClauseNIdx_MC<=Tmp)
		static_cast<const MetaConceptWithArgArray*>(Target)->ForceCheckForEvaluation();
	else if (MinClause2Idx_MC<=Tmp && MaxClause2Idx_MC<=Tmp)
		static_cast<const MetaConceptWith2Args*>(Target)->ForceCheckForEvaluation();
}

void
ForcePhraseCheckForEvaluation(const MetaConcept* Target)
{
	const ExactType_MC Tmp = Target->ExactType();
	if (MinPhraseNIdx_MC<=Tmp && MaxPhraseNIdx_MC<=Tmp)
		static_cast<const MetaConceptWithArgArray*>(Target)->ForceCheckForEvaluation();
	else if (MinPhrase2Idx_MC<=Tmp && MaxPhrase2Idx_MC<=Tmp)
		static_cast<const MetaConceptWith2Args*>(Target)->ForceCheckForEvaluation();
}
#endif

//! \todo ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(), etc.: OPTIMIZE: TIME
bool
MetaConceptWith1Arg::ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	try	{
		if (TargetRelation(lhs,*Arg1))
			RHSInducedActionOnArg(Arg1,rhs);
		return Arg1->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,RHSInducedActionOnArg,TargetRelation);
		}
	catch(const bad_alloc&)
		{
		return false;
		};
}

bool
MetaConceptWithArgArray::ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	try	{
		size_t i = fast_size();
		do	if (TargetRelation(lhs,*ArgArray[--i]))
				{
				RHSInducedActionOnArg(ArgArray[i],rhs);
				IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
				}
		while(0<i);

		i = fast_size();
		do	if (ArgArray[--i]->HasArgRelatedToThisConceptBy(lhs,TargetRelation))
				{
				if (!ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,RHSInducedActionOnArg,TargetRelation))
					return false;
				IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
				}
		while(0<i);
		return true;
		}
	catch(const bad_alloc&)
		{
		return false;
		};
}

bool
MetaConceptWith2Args::ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	try	{
		if (TargetRelation(lhs,*LHS_Arg1))
			{
			RHSInducedActionOnArg(LHS_Arg1,rhs);
			IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
			}
		if (TargetRelation(lhs,*RHS_Arg2))
			{
			RHSInducedActionOnArg(RHS_Arg2,rhs);
			IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
			}
		if (LHS_Arg1->HasArgRelatedToThisConceptBy(lhs,TargetRelation))
			{
			if (!LHS_Arg1->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,RHSInducedActionOnArg,TargetRelation))
				return false;
			IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
			}
		if (RHS_Arg2->HasArgRelatedToThisConceptBy(lhs,TargetRelation))
			{
			if (!RHS_Arg2->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,RHSInducedActionOnArg,TargetRelation))
				return false;
			IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
			}
		return true;
		}
	catch(const bad_alloc&)
		{
		return false;
		};
}

bool Variable::ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs,
	LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	if (typeid(MetaQuantifier)==typeid(lhs))
		{	// testing against quantifier....
		if (   RHSInducedActionOnArg==&SetLHSToRHS
			&& TargetRelation==&AreSyntacticallyEqual
			&& typeid(MetaQuantifier)==typeid(rhs))
			{	// This *can* work
			if (*Arg1==lhs)
				Arg1=static_cast<MetaQuantifier*>(const_cast<MetaConcept*>(&rhs));
			return true;
			}
		FATAL(AlphaMiscallVFunction);
		}
	return true;
}

// HasArgRelatedToThisConcept(), etc.
bool
MetaConceptWithArgArray::ThisConceptUsesNon1stArg(const MetaConcept& Target) const
{	// FORMALLY CORRECT: 11/15/1999
	size_t i = fast_size();
	while(0< --i)
		if (Target.HasArgRelatedToThisConceptBy(*ArgArray[i],IsSyntacticallyEqualOrAntiIdempotentTo))
			return true;
	return false;
}

// #define FRANCI_WARY 1

bool
MetaConceptWithArgArray::HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/21/2003
	if (!ArgArray.empty())
		{
		size_t i = fast_size();
		do	if (   TargetRelation(Target,*ArgArray[--i])
				|| ArgArray[i]->HasArgRelatedToThisConceptBy(Target,TargetRelation))
#ifdef FRANCI_WARY
				{
				LOG(Target);
				LOG(*ArgArray[i]);
				return true;
				}
#else
				return true;
#endif
		while(0<i);
		}
	return false;
}

#undef FRANCI_WARY

bool
MetaConceptWith2Args::HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/21/2003
	return    TargetRelation(Target,*LHS_Arg1)
		   || TargetRelation(Target,*RHS_Arg2)
		   || LHS_Arg1->HasArgRelatedToThisConceptBy(Target,TargetRelation)
		   || RHS_Arg2->HasArgRelatedToThisConceptBy(Target,TargetRelation);
}

bool
MetaConceptWith1Arg::HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/21/2003
	return    TargetRelation(Target,*Arg1)
		   || Arg1->HasArgRelatedToThisConceptBy(Target,TargetRelation);
}

bool
MetaConceptZeroArgs::HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const
{return false;}

//! \todo Optimize MetaConcept::MetaConceptPtrRelatedToThisConceptBy for time
bool
MetaConcept::MetaConceptPtrRelatedToThisConceptBy(const MetaConcept* Target, LowLevelBinaryRelation* TargetRelation) const
{
	if (Target)
		{
		if (   TargetRelation(*this,*Target)
			|| Target->HasArgRelatedToThisConceptBy(*this,TargetRelation))
			return true;
		}
	return false;
}


