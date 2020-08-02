// Equal.cxx
// definition of EqualRelation, a set of six predicates 

#include "Equal.hxx"

#include "Class.hxx"
#include "TruthVal.hxx"
#include "Interval.hxx"
#include "Keyword1.hxx"
#include "LowRel.hxx"

//! \todo Franci's parser must recognize:
//!	* operator == maps to ALLEQUAL
//!	* operator != maps to NOTALLEQUAL
//! \todo MetaConnective needs to support conversions based on this (in particular, condensations)
//! \todo Interface support for TruthValues
//! \todo OPTIMIZE?: Should the FindRelatedArgs functions have variants that accept a pre-filter
//! function?

// META: Standard form fixups
//	* 2-ary NOTALLEQUAL, DISTINCTFROMALLOF -> ALLDISTINCT
//	* 2-ary NOTALLDISTINCT, EQUALTOONEOF -> ALLEQUAL

EqualRelation::EvaluateToOtherRule EqualRelation::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
  =	{
	&EqualRelation::NAryEQUALSpawn2AryEQUAL,
	&EqualRelation::NAryEqRewriteZeroEq2ArySum,
	&EqualRelation::ReduceIntervalForNOTALLEQUAL
	};

EqualRelation::SelfEvaluateRule EqualRelation::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&EqualRelation::AddInvOutStdAddArg,
	&EqualRelation::MultInvOutStdMultArg,
	&EqualRelation::Ary2EqRewriteZeroEq2ArySum,
	&EqualRelation::MergeIntervals,
	&EqualRelation::DismantleLinearIntervalToEndpoints,
	&EqualRelation::DISTINCTFROMALLOFExtractALLDISTINCT,
	&EqualRelation::ExtendLinearInterval
	};
	
// reference tables
EqualRelation::InvokeEqualArgAux EqualRelation::EqualArgAux[StrictBound_EM]
  =	{
	&EqualRelation::InvokeEqualArgRuleALLEQUAL,
	&EqualRelation::InvokeEqualArgRuleALLDISTINCT,
	&EqualRelation::InvokeEqualArgRuleEQUALTOONEOF,
	&EqualRelation::InvokeEqualArgRuleDISTINCTFROMALLOF,
	&EqualRelation::InvokeEqualArgRuleNOTALLDISTINCT,
	&EqualRelation::InvokeEqualArgRuleNOTALLEQUAL
	};

EqualRelation::InvokeEqualArgAux EqualRelation::UseConstantsAux[StrictBound_EM]
  =	{
	&EqualRelation::UseConstantsALLEQUAL,
	&EqualRelation::UseConstantsALLDISTINCT,
	&EqualRelation::UseConstantsEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseConstantsEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseConstantsNOTALLDISTINCT,
	&EqualRelation::UseConstantsNOTALLEQUAL
	};

EqualRelation::InvokeEqualArgAux EqualRelation::UseStdAdditionAux[StrictBound_EM]
  =	{
	&EqualRelation::UseStdAdditionALLEQUAL,
	&EqualRelation::UseStdAdditionALLDISTINCT,
	&EqualRelation::UseStdAdditionEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseStdAdditionEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseStdAdditionNOTALLDISTINCT,
	&EqualRelation::UseStdAdditionNOTALLEQUAL
	};

EqualRelation::InvokeEqualArgAux EqualRelation::UseStdMultiplicationAux[StrictBound_EM]
  =	{
	&EqualRelation::UseStdMultiplicationALLEQUAL,
	&EqualRelation::UseStdMultiplicationALLDISTINCT,
	&EqualRelation::UseStdMultiplicationEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseStdMultiplicationEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseStdMultiplicationNOTALLDISTINCT,
	&EqualRelation::UseStdMultiplicationNOTALLEQUAL
	};

EqualRelation::InvokeEqualArgAux EqualRelation::UseDomainsAux[StrictBound_EM]
  =	{
	&EqualRelation::UseDomainsALLEQUAL,
	&EqualRelation::UseDomainsALLDISTINCT,
	&EqualRelation::UseDomainsEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseDomainsEQUALTOONEOForDISTINCTFROMALLOF,
	&EqualRelation::UseDomainsNOTALLDISTINCT,
	&EqualRelation::UseDomainsNOTALLEQUAL
	};

EqualRelation::ImpliesAux EqualRelation::StrictlyImpliesAux[StrictBound_EM]
  =	{
	&EqualRelation::StrictlyImpliesALLEQUAL,
	&EqualRelation::StrictlyImpliesALLDISTINCT,
	&EqualRelation::StrictlyImpliesEQUALTOONEOF,
	&EqualRelation::StrictlyImpliesDISTINCTFROMALLOF,
	&EqualRelation::StrictlyImpliesNOTALLDISTINCT,
	&EqualRelation::StrictlyImpliesNOTALLEQUAL
	};

EqualRelation::ImpliesAux EqualRelation::CanStrictlyModifyAux[StrictBound_EM]
  =	{
	&EqualRelation::CanStrictlyModifyALLEQUAL,
	&EqualRelation::CanStrictlyModifyALLDISTINCT,
	&EqualRelation::CanStrictlyModifyEQUALTOONEOF,
	&EqualRelation::CanStrictlyModifyDISTINCTFROMALLOF,
	&EqualRelation::CanStrictlyModifyNOTALLDISTINCT,
	&EqualRelation::CanStrictlyModifyNOTALLEQUAL
	};

EqualRelation::ModifiesAux EqualRelation::StrictlyModifiesAux[StrictBound_EM]
  =	{
	&EqualRelation::StrictlyModifiesALLEQUAL,
	&EqualRelation::StrictlyModifiesALLDISTINCT,
	&EqualRelation::StrictlyModifiesEQUALTOONEOF,
	&EqualRelation::StrictlyModifiesDISTINCTFROMALLOF,
	&EqualRelation::StrictlyModifiesNOTALLDISTINCT,
	&EqualRelation::StrictlyModifiesNOTALLEQUAL
	};

// META: inspect the SelfLogicalNOT map.  Since EQUALTOONEOF and DISTINCTFROMALLOF
// map to each other without qualification [both ary3+], we can define away two functions.
// The other four should remain because of arity mangling of the SelfLogicalNOT map.

EqualRelation::FindDetailedRuleAux EqualRelation::LogicalANDDetailedRuleAux[StrictBound_EM]
  = {
	&EqualRelation::LogicalANDFindDetailedRuleALLEQUAL,
	NULL,
	NULL,
	&EqualRelation::LogicalANDFindDetailedRuleDISTINCTFROMALLOF,
	NULL,
	NULL
	};

bool IsClearlyDistinctFrom(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/13/2002
	// distinct constants are clearly distinct
	if (   lhs.IsExplicitConstant() && rhs.IsExplicitConstant()
		&& lhs!=rhs)
		return true;

	// LinearInterval of UltimateType _Z_ is distinct from anything not enumerated by it
	// [it's really an arglist compression].  Note that this is *not* an ExplicitConstant.
	// Two LinearIntervals will correctly (but misleadingly) detect as not in each other.
	if (lhs.IsTypeMatch(LinearInterval_MC,&Integer))
		{
		const LinearInterval& VR_lhs = static_cast<const LinearInterval&>(lhs);
		if (rhs.IsTypeMatch(LinearInterval_MC,&Integer))
			{
			const LinearInterval& VR_rhs = static_cast<const LinearInterval&>(rhs);
			if (VR_lhs.ClearlyNotOverlapping(VR_rhs)) return true;
			if (VR_lhs.ClearlyOverlapping(VR_rhs)) return false;
			}
		else{
			if (VR_lhs.DoesNotHaveAsElement(rhs)) return true;
			if (VR_lhs.HasAsElement(rhs)) return false;
			}
		}
	else if (rhs.IsTypeMatch(LinearInterval_MC,&Integer))
		{
		const LinearInterval& VR_rhs = static_cast<const LinearInterval&>(rhs);
		if (VR_rhs.DoesNotHaveAsElement(lhs)) return true;
		if (VR_rhs.HasAsElement(lhs)) return false;
		};

	// Type mismatches
	if (    lhs.IsExplicitConstant() && !rhs.IsUltimateType(NULL)
		&& !rhs.UltimateType()->HasAsElement(lhs))
		return true;
	if (    rhs.IsExplicitConstant() && !lhs.IsUltimateType(NULL)
		&& !lhs.UltimateType()->HasAsElement(rhs))
		return true;
	if (   !lhs.IsUltimateType(NULL) && !rhs.IsUltimateType(NULL)
		&&  lhs.UltimateType()->IntersectionWithIsNULLSet(*rhs.UltimateType()))
		return true;

	// Sign information
#define DISTINCT_ON_SIGN(LHS,RHS)	\
	if (LHS.IsPositive())	\
		{	\
		if (RHS.IsNotPositive()) return true;	\
		}	\
	else if (LHS.IsZero())	\
		{	\
		if (RHS.IsNotZero()) return true;	\
		}	\
	else if (LHS.IsNegative())	\
		{	\
		if (RHS.IsNotNegative()) return true;	\
		}

	DISTINCT_ON_SIGN(lhs,rhs);
	DISTINCT_ON_SIGN(rhs,lhs);

#undef DISTINCT_ON_SIGN

	return false;
}

bool IsEllipsisArgList(const MetaConcept& lhs)
{
	return lhs.IsTypeMatch(LinearInterval_MC,&Integer);
}

bool IsNotEllipsisArgList(const MetaConcept& lhs)
{
	return !lhs.IsTypeMatch(LinearInterval_MC,&Integer);
}

bool IsExpandableEllipsisArgList(const MetaConcept& lhs)
{
	unsigned long Cardinality;
	if (   lhs.IsTypeMatch(LinearInterval_MC,&Integer)
		&& static_cast<const LinearInterval&>(lhs).IsExpandable(Cardinality))
		return true;
	return false;
}


//! \todo use IsVirtualExplicitConstant in place of IsExplicitConstant, when appropriate
bool IsVirtualExplicitConstant(const MetaConcept& lhs)
{
	if (lhs.IsExplicitConstant()) return true;
	if (   lhs.IsTypeMatch(LinearInterval_MC,&Integer)
		&& lhs.ArgN(0)->IsExplicitConstant()
		&& lhs.ArgN(1)->IsExplicitConstant())
		return true;
	return false;
}

bool IsClearlyIndistinctFrom(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: 7/19/2002
	if 		(lhs.IsTypeMatch(LinearInterval_MC,&Integer))
		{
		if (rhs.IsTypeMatch(LinearInterval_MC,&Integer))
			return static_cast<const LinearInterval&>(lhs).ClearlyOverlapping(static_cast<const LinearInterval&>(rhs));
		else
			return static_cast<const LinearInterval&>(lhs).HasAsElement(rhs);
		}
	else if (rhs.IsTypeMatch(LinearInterval_MC,&Integer))
		return static_cast<const LinearInterval&>(rhs).HasAsElement(lhs);
	return lhs==rhs;
}

bool IsSubclassAsEnumeratedSet(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/19/2002
	if 		(lhs.IsTypeMatch(LinearInterval_MC,&Integer))
		{
		if (rhs.IsTypeMatch(LinearInterval_MC,&Integer))
			return static_cast<const LinearInterval&>(lhs).Subclass(static_cast<const LinearInterval&>(rhs));
		}
	else if (rhs.IsTypeMatch(LinearInterval_MC,&Integer))
		return static_cast<const LinearInterval&>(rhs).HasAsElement(lhs);
	return lhs==rhs;
}

// rest of implementation
EqualRelation::EqualRelation(MetaConcept**& NewArgList, EqualRelationModes LinkageType)
:	MetaConceptWithArgArray((ExactType_MC)(LinkageType+ALLEQUAL_MC),NewArgList)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/2/2000
	// If the syntax is already bad, then we don't need to proceed further
	if (!SyntaxOK()) return;

	// this imposes the fixup
	if (2==fast_size() && !ArgArray[0]->IsExactType(LinearInterval_MC) && !ArgArray[1]->IsExactType(LinearInterval_MC))
		{
		if      (NOTALLDISTINCT_EM<=LinkageType)
			SetExactType((ExactType_MC)(LinkageType+ALLEQUAL_MC-4));
		else if (EQUALTOONEOF_EM<=LinkageType)
			SetExactType((ExactType_MC)(LinkageType+ALLEQUAL_MC-2));
		};
	ForceStdForm();
}

//  Type ID functions
const AbstractClass* EqualRelation::UltimateType() const {return &TruthValues;}

void EqualRelation::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 6/22/2000
	if (!IdxCurrentSelfEvalRule && !IdxCurrentEvalRule)
		{
		ForceStdFormAux();
		if (IsSymmetric())
			{	// Normal form for ALLEQUAL, NOTALLEQUAL is to put the constant arg at arg0,
				// and *then* sort.  [If there's more than one, it won't hang around long.]
				// Normal form for ALLDISTINCT, NOTALLDISTINCT is to put the constant args
				// first, and *then* sort.
			if 		(IsExactType(ALLEQUAL_MC))
				{
				if (FindExplicitConstantArg(ArgArray,fast_size(),InferenceParameter1))
					{	// if there's more than one, it won't survive long
					if (0!=InferenceParameter1)
						SwapArgs(0,InferenceParameter1);

					ForceLexicalArgOrder(1,fast_size());

					size_t i = fast_size();
					do	ArgArray[--i]->ForceUltimateType(ArgArray[0]->UltimateType());
					while(1<i);
					return;
					};
				}
			else if (IsExactType(NOTALLEQUAL_MC))
				{
				if (FindExplicitConstantArg(ArgArray,fast_size(),InferenceParameter1))
					{	// if there's more than one, it won't survive long
					if (0!=InferenceParameter1)
						SwapArgs(0,InferenceParameter1);

					ForceLexicalArgOrder(1,fast_size());
					return;
					};
				}
			else{	// ALLDISTINCT, NOTALLDISTINCT
				if (FindExplicitConstantArg(ArgArray,fast_size(),InferenceParameter2))
					{
					if (0<InferenceParameter2)
						{
						size_t i = 0;
						do	{
							while(ArgArray[i]->IsExplicitConstant()) i++;
							if (i<InferenceParameter2)
								{
								SwapArgs(i,InferenceParameter2);
								FindExplicitConstantArg(ArgArray,InferenceParameter2,InferenceParameter2);
								};
							}
						while(i<InferenceParameter2);
						if (2<=InferenceParameter2)
							ForceLexicalArgOrder(0,InferenceParameter2+1);
						};
					if (fast_size()-2>InferenceParameter2)
						{
						if (   FindArgWithUnaryProperty(IsVirtualExplicitConstant)
							&& InferenceParameter1>InferenceParameter2)
							{
							if (InferenceParameter2+1<InferenceParameter1)
								{
								size_t i = InferenceParameter2+1;
								do	{
									while(IsVirtualExplicitConstant(*ArgArray[i])) i++;
									if (i<InferenceParameter1)
										{
										SwapArgs(i,InferenceParameter1);
										FindArgWithUnaryProperty(IsVirtualExplicitConstant,InferenceParameter1);
										};
									}
								while(i<InferenceParameter1);
								if (2<=InferenceParameter1-InferenceParameter2)
									ForceLexicalArgOrder(InferenceParameter2+1,InferenceParameter1+1);
								};
							if (fast_size()-2>InferenceParameter1)
								ForceLexicalArgOrder(InferenceParameter1+1,fast_size());
							return;
							}
						ForceLexicalArgOrder(InferenceParameter2+1,fast_size());
						}
					return;
					}
				else if (FindArgWithUnaryProperty(IsVirtualExplicitConstant))
					{
					if (0!=InferenceParameter1)
						{
						size_t i = 0;
						do	{
							while(IsVirtualExplicitConstant(*ArgArray[i])) i++;
							if (i<InferenceParameter1)
								{
								SwapArgs(i,InferenceParameter1);
								FindArgWithUnaryProperty(IsVirtualExplicitConstant,InferenceParameter1);
								};
							}
						while(i<InferenceParameter1);
						if (1<=InferenceParameter1)
							ForceLexicalArgOrder(0,InferenceParameter1+1);
						}
					if (fast_size()-2>InferenceParameter1)
						ForceLexicalArgOrder(InferenceParameter1+1,fast_size());
					return;
					}
				}
			ForceTotalLexicalArgOrder();
			return;
			}
		else	// EQUALTOONEOF, DISTINCTFROMALLOF
			ForceLexicalArgOrder(1,fast_size());
		};
}

//  Evaluation functions
bool EqualRelation::SyntaxOK() const
{	// FORMALLY CORRECT: 6/9/2002
	if (!SyntaxOKAux()) return false;
	// NOTE: LinearInterval of UltimateType _Z_ in arg 0 is a syntax error
	// for DISTINCTFROMALLOF, EQUALTOONEOF
	if (   (IsExactType(DISTINCTFROMALLOF_MC) || IsExactType(EQUALTOONEOF_MC))
		&&  ArgArray[0]->IsTypeMatch(LinearInterval_MC,&Integer))
		return false;
	return true;
}

void EqualRelation::_ForceArgSameImplementation(size_t n) { NARY_FORCEARGSAMEIMPLEMENTATION_BODY; }

void EqualRelation::SelfLogicalNOT()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/27/2002
	// ALLEQUAL <-> NOTALLEQUAL: 
	// ALLDISTINCT <-> NOTALLDISTINCT: 
	// EQUALTOONEOF <-> DISTINCTFROMALLOF: 
	// 2-ary: ALLEQUAL<->ALLDISTINCT
	if (2==fast_size() && !ArgArray[0]->IsExactType(LinearInterval_MC) && !ArgArray[1]->IsExactType(LinearInterval_MC))
		{
		SetExactType((ExactType_MC)((ALLDISTINCT_MC+ALLEQUAL_MC)-ExactType()));
		_forceStdForm();
		}
	else
		SetExactType((ExactType_MC)((NOTALLEQUAL_MC+ALLEQUAL_MC)-ExactType()));
}

int EqualRelation::_strictlyImplies(const MetaConcept& rhs) const
{	// \todo IMPLEMENT
	if (StrictlyImplies(rhs)) return 2;	// stopgap
	if (StrictlyImpliesLogicalNOTOf(*this, rhs)) return -2;	// stopgap
	return 0;
}

//! \todo explore use of EQUALTOONEOF w/constant args as an infinity-ary DISTINCTFROMALLOF [all args constant]
//! StrictlyImplies, StrictlyImpliesLogicalNOTOf, StrictlyModifies, LogicalANDAux are targets
//! LogicalANDAux depends on implementation of DISTINCTFROMALLOF targets for ALLDISTINCT extraction.
bool EqualRelation::StrictlyImplies(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/26/2000
	return (this->*StrictlyImpliesAux[array_index()])(rhs);
}

//! \todo ALL StrictlyImplies variants should be upgraded to handle global injective maps;
//! first instance is global addition of common expression
//! \todo X+const1==const2 is not properly reduced for 3+-ary ALLDISTINCT, DISTINCTFROMALLOF
//! with constant vertex, EQUALTOONEOF with constant vertex, NOTALLDISTINCT, or NOTALLEQUAL.
//! compensate in the StrictlyImplies case, then propagate to mirrors in other routines.

//! \todo We have integer-range intervals; use them as follows:
//! A==const:
//! * ALLDISTINCT(A,interval,...): const in interval causes ALLDISTINCT to false-out
//! * A DISTINCTFROMALLOF interval, ... : const in interval causes DISTINCTFROMALLOF to false-out
//! * A EQUALTOONEOF interval, ... : const in interval causes EQUALTOONEOF to true-out
//!   const not in any const interval, does not match any const, no vars causes EQUALTOONEOF to false-out
//! * NOTALLDISTINCT(A,interval,...): const in interval causes NOTALLDISTINCT to true out
//! ....

//! in general, intervals act like arg ranges; constant-endpoint intervals act like the corresponding range of constants
//! deal with this at the logic engine level: 
//! NOTE: reorganized the logic engine to force evaluation before StrictlyImplies checking.
//! \todo To complete this, should define a 'ForceNoEvaluation' function (determining evaluation may 
//! be expensive) for use in the meta-StrictlyImplies code
// * DONE: MetaConceptWithArgArray has an 'expandable interval detector'
// * if any target args are expandable, expand them *before* processing; compress them as possible
// ** at this stage, ALLEQUAL, NOTALLEQUAL should not contain argrange intervals
// * alternatively: real-time defense (processing hog!)
bool EqualRelation::StrictlyImpliesALLEQUAL(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/5/2002
	// ALLEQUAL subvector arglist: implied
	// EQUALTOONEOF: Arg0, 1 arg in ALLEQUAL arglist: implied
	// NOTALLDISTINCT: two args in ALLEQUAL arglist: implied
	// META: A==const1 => A!=const2 [but substitution will also catch this]
	//! \todo FIX: Cascade effects when UltimateType TruthValues involved
	// This now handles argrange intervals of type _Z_
	if (typeid(EqualRelation)==typeid(rhs))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if 		(VR_rhs.IsExactType(ALLEQUAL_MC))
			return VR_rhs.SubvectorArgList(*this);
		else if (VR_rhs.IsExactType(ALLDISTINCT_MC))
			{	// use constant-hood as additional information (constants are 
				// easier to trip IsClearlyDistinctFrom
			if (ArgArray[0]->IsExplicitConstant())
				{	// important short-circuit
				if (VR_rhs.FindArgRelatedToLHS(*ArgArray[0],IsClearlyDistinctFrom))
					return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,0,VR_rhs.InferenceParameter1);				
				// not useful, except to set up simplification
				if (FindArgRelatedToLHS(*VR_rhs.ArgArray[0],IsClearlyDistinctFrom))
					return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,InferenceParameter1,0);
				if (FindTwoRelatedArgs(VR_rhs,IsClearlyDistinctFrom,0,0))
					return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,InferenceParameter1,VR_rhs.InferenceParameter1);
				return false;				
				}
			if (FindTwoRelatedArgs(VR_rhs,IsClearlyDistinctFrom))
				return FindTwoRelatedArgs(VR_rhs,AreSyntacticallyEqual,InferenceParameter1,VR_rhs.InferenceParameter1);
			return false;
			}
		else if (VR_rhs.IsExactType(DISTINCTFROMALLOF_MC))
			{
			if (FindArgRelatedToLHS(*VR_rhs.ArgArray[0],IsClearlyDistinctFrom))
				return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,InferenceParameter1,0);
			else if (FindArgRelatedToLHS(*VR_rhs.ArgArray[0],AreSyntacticallyEqual))
				return FindTwoRelatedArgs(VR_rhs,IsClearlyDistinctFrom,InferenceParameter1,0);
			return false;
			}
		else if (VR_rhs.IsExactType(EQUALTOONEOF_MC))
			{
			if (FindArgRelatedToLHS(*VR_rhs.ArgArray[0],AreSyntacticallyEqual))
				return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,InferenceParameter1,0);
			}
		else if (VR_rhs.IsExactType(NOTALLDISTINCT_MC))
			{	// this should handle A in RHS interval, B in RHS interval: match
			if (FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom))
				return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,InferenceParameter1,VR_rhs.InferenceParameter1);
			}
		else if (VR_rhs.IsExactType(NOTALLEQUAL_MC))
			{	// this should handle A in RHS interval, B in RHS interval: match
			if (FindTwoRelatedArgs(VR_rhs,AreSyntacticallyEqual))
				return FindTwoRelatedArgs(VR_rhs,IsClearlyDistinctFrom,InferenceParameter1,VR_rhs.InferenceParameter1);
			};
		};
	return false;
}

bool EqualRelation::StrictlyImpliesALLDISTINCT(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/5/2002
	// ALLDISTINCT subvector arglist: implied
	// DISTINCTFROMALLOF subarglist: implied
	// NOTALLEQUAL: two args in ALLDISTINCT arglist: implied
	//! \todo FIX: Cascade effects when UltimateType TruthValues involved
	//! \todo FIX: handle LinearIntervals of UltimateType _Z_
	if (typeid(EqualRelation)==typeid(rhs))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if 		(VR_rhs.IsExactType(ALLDISTINCT_MC))
			return VR_rhs.SubvectorArgList(*this);
		else if (VR_rhs.IsExactType(DISTINCTFROMALLOF_MC))
			return VR_rhs.ArgListHasInjectionIntoRHSArgListRelatedThisWay(*this,AreSyntacticallyEqual);
		else if (VR_rhs.IsExactType(NOTALLEQUAL_MC))
			{	// this has been fixed to handle LinearIntervals of UltimateType _Z_
			if (FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom))
				return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,InferenceParameter1,VR_rhs.InferenceParameter1);
			};
		};
	return false;
}

bool EqualRelation::StrictlyImpliesEQUALTOONEOF(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/26/2000
	// EQUALTOONEOF Arg0==Arg0, subvector arglist: implied
	// NOTALLDISTINCT: superarglist: implied
	//! \todo FIX: Cascade effects when UltimateType TruthValues involved
	//! \todo FIX: handle LinearIntervals of UltimateType _Z_
	if (typeid(EqualRelation)==typeid(rhs))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if		(VR_rhs.IsExactType(EQUALTOONEOF_MC))
			{	//! \todo OPTIMIZE: this has an implicit redundant == test
			if (*ArgArray[0]==*VR_rhs.ArgArray[0])
				return SubvectorArgList(VR_rhs);
			}
		else if (VR_rhs.IsExactType(NOTALLDISTINCT_MC))
			return ArgListHasInjectionIntoRHSArgListRelatedThisWay(VR_rhs,AreSyntacticallyEqual);
		else if (   VR_rhs.IsExactType(DISTINCTFROMALLOF_MC)
				 && VerifyArgsExplicitConstant(1,fast_size()))	// DISTINCTFROMALLOF mirror code
			{
			if (   *ArgArray[0]==*VR_rhs.ArgArray[0]
				&& VR_rhs.VerifyArgsExplicitConstant(1,VR_rhs.fast_size()))
				return !FindTwoRelatedArgs(VR_rhs,AreSyntacticallyEqual,0,0);
			};
		};
	return false;
}

bool
EqualRelation::StrictlyImpliesDISTINCTFROMALLOF(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/5/2002
	// ALLDISTINCT: 2-ary, subarglist, 1 arg DISTINCTFROMALLOF Arg0: implied
	// DISTINCTFROMALLOF Arg0==Arg0, subvector arglist: implied
	// NOTALLEQUAL: superarglist: implied
	//! \todo FIX: Cascade effects when UltimateType TruthValues involved
	//! \todo FIX: handle LinearIntervals of UltimateType _Z_
	if (typeid(EqualRelation)==typeid(rhs))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if 		(VR_rhs.IsExactType(ALLDISTINCT_MC))
			{	// This has been fixed to handle LinearIntervals of UltimateType _Z_
			if (   2==VR_rhs.fast_size()
				&& (   (*ArgArray[0]==*VR_rhs.ArgArray[0] && FindArgRelatedToLHS(*VR_rhs.ArgArray[1],IsClearlyIndistinctFrom))
				    || (*ArgArray[0]==*VR_rhs.ArgArray[1] && FindArgRelatedToLHS(*VR_rhs.ArgArray[0],IsClearlyIndistinctFrom))))
				return true;
			}
		else if (VR_rhs.IsExactType(DISTINCTFROMALLOF_MC))
			{	//! \todo OPTIMIZE: this has an implicit redundant == test
			if (*ArgArray[0]==*VR_rhs.ArgArray[0])
				return VR_rhs.SubvectorArgList(*this);
			}
		else if (VR_rhs.IsExactType(NOTALLEQUAL_MC))
			{	// This has been fixed to handle LinearIntervals of UltimateType _Z_
			if (VR_rhs.FindArgRelatedToLHS(*ArgArray[0],AreSyntacticallyEqual))
				return FindTwoRelatedArgs(VR_rhs,IsClearlyIndistinctFrom,0,VR_rhs.InferenceParameter1);
			};
		};
// mirror: StrictlyImplies EQUALTOONEOF code
// An DISTINCTFROMALLOF with all RHS args constant is an infinity-ary EQUALTOONEOF
// but both interesting rules require an infinity-ary injection into finite-arity:
// automatic failure
	return false;
}

bool EqualRelation::StrictlyImpliesNOTALLDISTINCT(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/26/2000
	// NOTALLDISTINCT supervector arglist: implied
	//! \todo FIX: Cascade effects when UltimateType TruthValues involved
	//! \todo FIX: handle LinearIntervals of UltimateType _Z_
	if (rhs.IsExactType(NOTALLDISTINCT_MC))
		return SubvectorArgList(static_cast<const EqualRelation&>(rhs));
	return false;
}

bool EqualRelation::StrictlyImpliesNOTALLEQUAL(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/27/2002
	// NOTALLEQUAL supervector arglist: implied
	//! \todo FIX: Cascade effects when UltimateType TruthValues involved
	if (rhs.IsExactType(NOTALLEQUAL_MC))
		return SubvectorArgList(static_cast<const EqualRelation&>(rhs));
	return false;
}

// Architecture problem: StrictlyModifies has to be a deep-scan relation, 
// but would benefit from a more integrated approach to modifications (detector should also 
// assign change instructions)
void EqualRelation::StrictlyModifies(MetaConcept*& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/31/2000
	assert(rhs);
	(this->*StrictlyModifiesAux[array_index()])(rhs);
}

void EqualRelation::StrictlyModifiesALLEQUAL(MetaConcept*& rhs) const
{	//! \todo IMPLEMENT
}

void EqualRelation::StrictlyModifiesALLDISTINCT(MetaConcept*& rhs) const
{	//! \todo FIX: handle LinearIntervals of UltimateType _Z_
	if (rhs->IsExactType(NOTALLDISTINCT_MC))
		{	// TODO: Revise this if the NOTALLDISTINCT case gets more versatile
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		// swap missing arg with arg0, then retype NOTALLDISTINCT to EQUALTOONEOF
		VR_rhs.SwapArgs(0,VR_rhs.InferenceParameter1);
		VR_rhs.InferenceParameter1 = EQUALTOONEOF_MC;
		VR_rhs.CompatibleRetype();
		return;
		}
	else if	(rhs->IsExactType(EQUALTOONEOF_MC))
		{	// VR_RHS.InferenceParameter1 := triggering Idx in second layer
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		size_t i = VR_rhs.InferenceParameter1;
		VR_rhs.IdxCurrentSelfEvalRule = SelfEvalRuleAry2CorrectedCleanArg_SER;
		VR_rhs.InferenceParameter2 = ALLEQUAL_MC;
		// check for vector delete; if overdelete, change EQUALTOONEOF to FALSE
		while(1<i)
			if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],AreSyntacticallyEqual))
				{	// set up vector delete
				VR_rhs.IdxCurrentSelfEvalRule = SelfEvalRuleAry2CorrectedCleanTrailingArg_SER;
				SwapArgs(VR_rhs.InferenceParameter1,VR_rhs.fast_size()-1);
				VR_rhs.InferenceParameter1 = VR_rhs.fast_size()-2;
				SwapArgs(i,VR_rhs.InferenceParameter1);
				};
		while(1<i)
			if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],AreSyntacticallyEqual))
				// continue vector delete
				SwapArgs(i,--VR_rhs.InferenceParameter1);
		// this is caught by StrictlyImpliesLogicalNOTOf, ALLDISTINCT version
		assert(!(1==VR_rhs.InferenceParameter1 && SelfEvalRuleAry2CorrectedCleanTrailingArg_SER==VR_rhs.IdxCurrentSelfEvalRule));
		LOG("Using this ALLDISTINCT clause to clean this arg of EQUALTOONEOF clause:");
		LOG(*this);
		LOG(*VR_rhs.ArgArray[VR_rhs.InferenceParameter1]);
		LOG(VR_rhs);
		if (SelfEvalRuleAry2CorrectedCleanTrailingArg_SER==VR_rhs.IdxCurrentSelfEvalRule)
			{
			VR_rhs.SelfEvalRuleAry2CorrectedCleanTrailingArg();
			return;
			}
		assert(SelfEvalRuleAry2CorrectedCleanArg_SER==VR_rhs.IdxCurrentSelfEvalRule);
		VR_rhs.SelfEvalRuleAry2CorrectedCleanArg();
		return;
		}
	else if (rhs->IsExactType(DISTINCTFROMALLOF_MC))
		{	// RHS.InferenceParameter1 is already set up
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		if (0<VR_rhs.InferenceParameter1)
			{	// arg0 of DISTINCTFROMALLOF clause in ALLDISTINCT clause: clean args
			size_t i = VR_rhs.InferenceParameter1;	// this Idx tripped this off
			VR_rhs.DeleteIdx(i);
			while(1< --i)
				if (FindArgRelatedToLHS(*VR_rhs.ArgArray[i],AreSyntacticallyEqual))
					VR_rhs.DeleteIdx(i);
			if (2==VR_rhs.fast_size())
				{
				VR_rhs.InferenceParameter1 = ALLDISTINCT_MC;
				VR_rhs.CompatibleRetype();
				};
			return;
			};
		// arg0 of DISTINCTFROMALLOF clause not in ALLDISTINCT clause:
		// length and subvector arglist verified, convert DISTINCTFROMALLOF to ALLDISTINCT
		VR_rhs.InferenceParameter1 = ALLDISTINCT_MC;
		VR_rhs.CompatibleRetype();
		return;
		};
}

void EqualRelation::StrictlyModifiesEQUALTOONEOF(MetaConcept*& rhs) const
{	//! \todo FIX: handle LinearIntervals of UltimateType _Z_
	if 		(rhs->IsExactType(NOTALLEQUAL_MC))
		{	//! \todo modify if this case becomes more versatile
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		LOG("Using this EQUALTOONEOF clause to remove this arg from:");
		LOG(*this);
		LOG(*ArgArray[0]);
		VR_rhs.FindArgRelatedToLHS(*ArgArray[0],AreSyntacticallyEqual);
		VR_rhs.InferenceParameter2 = ALLDISTINCT_MC;
		LOG(VR_rhs);
		VR_rhs.SelfEvalRuleAry2CorrectedCleanArg();
		}
	else if (rhs->IsExactType(EQUALTOONEOF_MC))
		{
		//	VR_RHS.InferenceParameter1 is the Idx of the VR_RHS that tripped this;
		// *ArgArray[0]==*VR_RHS.ArgArray[0], all LHS args ExplicitConstant already
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		// case: all LHS args constant: test RHS constant args against LHS.
		//	delete those not matched
		size_t i = VR_rhs.InferenceParameter1;
		VR_rhs.IdxCurrentSelfEvalRule = SelfEvalRuleAry2CorrectedCleanArg_SER;
		VR_rhs.InferenceParameter2 = ALLEQUAL_MC;
Restart1:
		while(1<i)
			if (VR_rhs.ArgArray[--i]->IsExplicitConstant())
				{
				size_t j = fast_size();
				do	if (*ArgArray[--j]==*VR_rhs.ArgArray[i])
						goto Restart1;
				while(1<j);
				// mark VR_RHS arg Idx for deletion
				if	(SelfEvalRuleAry2CorrectedCleanArg_SER==VR_rhs.IdxCurrentSelfEvalRule)
					{	// vector delete.  InferenceParameter1 already set up.
					VR_rhs.IdxCurrentSelfEvalRule = SelfEvalRuleAry2CorrectedCleanTrailingArg_SER;
					SwapArgs(VR_rhs.InferenceParameter1,VR_rhs.fast_size()-1);
					VR_rhs.InferenceParameter1 = VR_rhs.fast_size()-2;
					SwapArgs(i,VR_rhs.InferenceParameter1);
					}
				else{
					assert(SelfEvalRuleAry2CorrectedCleanTrailingArg_SER==VR_rhs.IdxCurrentSelfEvalRule);
					// continue vector delete
					SwapArgs(i,--VR_rhs.InferenceParameter1);
					}
				};
		if (SelfEvalRuleAry2CorrectedCleanArg_SER==VR_rhs.IdxCurrentSelfEvalRule)
			VR_rhs.SelfEvalRuleAry2CorrectedCleanArg();
		else{
			assert(SelfEvalRuleAry2CorrectedCleanTrailingArg_SER==VR_rhs.IdxCurrentSelfEvalRule);
			VR_rhs.SelfEvalRuleAry2CorrectedCleanTrailingArg();
			}
		};
}

void EqualRelation::StrictlyModifiesDISTINCTFROMALLOF(MetaConcept*& rhs) const
{	//! \todo FIX: handle LinearIntervals of UltimateType _Z_
	if	(rhs->IsExactType(EQUALTOONEOF_MC))
		{	// TODO: modify if this case becomes more versatile
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		if (0==InferenceParameter1)
			{	// arg0 EQUALTOONEOF arg1, ...; arg0 DISTINCTFROMALLOF arg1, ...: EQUALTOONEOF blots arg1
			// InferenceParameter2 is the Idx that tripped this
			size_t i = InferenceParameter2;
			VR_rhs.InferenceParameter1 = i;
			VR_rhs.IdxCurrentSelfEvalRule = SelfEvalRuleAry2CorrectedCleanArg_SER;
			VR_rhs.InferenceParameter2 = ALLEQUAL_MC;
			// check for vector delete; if overdelete, change DISTINCTFROMALLOF to FALSE
			while(1<i)
				if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],AreSyntacticallyEqual))
					{	// set up vector delete
					VR_rhs.IdxCurrentSelfEvalRule = SelfEvalRuleAry2CorrectedCleanTrailingArg_SER;
					VR_rhs.SwapArgs(InferenceParameter1,fast_size()-1);
					VR_rhs.InferenceParameter1 = fast_size()-2;
					VR_rhs.SwapArgs(i,InferenceParameter1);
					};
			while(1<i)
				if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],AreSyntacticallyEqual))
					// continue vector delete
					VR_rhs.SwapArgs(i,--InferenceParameter1);
			if 		(SelfEvalRuleAry2CorrectedCleanArg_SER == VR_rhs.IdxCurrentSelfEvalRule)
				{
				LOG("Using this DISTINCTFROMALLOF clause to blot this arg of EQUALTOONEOF clause:");
				LOG(*this);
				LOG(*VR_rhs.ArgArray[VR_rhs.InferenceParameter1]);
				LOG(VR_rhs);
				VR_rhs.SelfEvalRuleAry2CorrectedCleanArg();
				return;
				};
			assert(SelfEvalRuleAry2CorrectedCleanTrailingArg_SER == VR_rhs.IdxCurrentSelfEvalRule);
			// This was caught by StrictlyImpliesLogicalNOTOf
			assert(1!=VR_rhs.InferenceParameter1);
			LOG("Using this DISTINCTFROMALLOF clause to blot all args at or after this arg of EQUALTOONEOF clause:");
			LOG(*this);
			LOG(*VR_rhs.ArgArray[VR_rhs.InferenceParameter1]);
			LOG(VR_rhs);
			VR_rhs.SelfEvalRuleAry2CorrectedCleanTrailingArg();
			return;
			};
		assert(VR_rhs.FindArgRelatedToLHS(*ArgArray[0],AreSyntacticallyEqual));
		// FLAG: 1 == InferenceParameter1
		// arg0 EQUALTOONEOF arg1, ...; arg1 DISTINCTFROMALLOF arg0, ...: EQUALTOONEOF blots arg1
		// VR_RHS.InferenceParameter1 already set up
		LOG("Using this DISTINCTFROMALLOF clause to blot this arg of EQUALTOONEOF clause:");
		LOG(*this);
		LOG(*VR_rhs.ArgArray[VR_rhs.InferenceParameter1]);
		LOG(VR_rhs);
		VR_rhs.InferenceParameter2 = ALLEQUAL_MC;
		VR_rhs.SelfEvalRuleAry2CorrectedCleanArg();
		return;
		}
	else if (rhs->IsExactType(NOTALLDISTINCT_MC))
		{	//! \todo alter if this becomes more versatile
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		// blot arg0 from NOTALLDISTINCT
		// VR_RHS.InferenceParameter1 already set up
		LOG("Using this DISTINCTFROMALLOF clause to blot this arg of NOTALLDISTINCT:");
		LOG(*this);
		LOG(*VR_rhs.ArgArray[VR_rhs.InferenceParameter1]);
		LOG(VR_rhs);
		VR_rhs.InferenceParameter2 = ALLEQUAL_MC;
		VR_rhs.SelfEvalRuleAry2CorrectedCleanArg();
		return;
		}
	else if (rhs->IsExactType(ALLDISTINCT_MC))
		{	// ALLDISTINCT subvectorarglist DISTINCTFROMALLOF, and arg0 of DISTINCTFROMALLOF
			// *not* in ALLDISTINCT.  Add arg0 to ALLDISTINCT.
		EqualRelation& VR_rhs = *static_cast<EqualRelation*>(rhs);
		MetaConcept* TmpArg = NULL;
		try	{
			ArgArray[0]->CopyInto(TmpArg);
			}
		catch(const bad_alloc&)
			{
			DELETE(TmpArg);
			UnconditionalRAMFailure();
			};
		if (!VR_rhs.AddArgAtEndAndForceCorrectForm(TmpArg))
			{
			DELETE(TmpArg);
			UnconditionalRAMFailure();
			}
		};
}

void EqualRelation::StrictlyModifiesNOTALLDISTINCT(MetaConcept*& rhs) const
{	//! \todo IMPLEMENT
}

void EqualRelation::StrictlyModifiesNOTALLEQUAL(MetaConcept*& rhs) const
{	//! \todo IMPLEMENT
}

bool EqualRelation::CanStrictlyModify(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/31/2000
	return (this->*CanStrictlyModifyAux[array_index()])(rhs);
}

bool EqualRelation::CanStrictlyModifyALLEQUAL(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	return false;
}

bool EqualRelation::CanStrictlyModifyALLDISTINCT(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if (rhs.IsExactType(NOTALLDISTINCT_MC))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if (fast_size()+1==VR_rhs.fast_size())
			return SubvectorArgList(VR_rhs);
		}
	else if	(rhs.IsExactType(EQUALTOONEOF_MC))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if (FindArgRelatedToLHS(*VR_rhs.ArgArray[0],AreSyntacticallyEqual))
			{
			size_t i = VR_rhs.fast_size();
			do	if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],AreSyntacticallyEqual))
					{
					VR_rhs.InferenceParameter1=i;
					return true;
					}
			while(1<i);
			};
		}
	else if (rhs.IsExactType(DISTINCTFROMALLOF_MC))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if (FindArgRelatedToLHS(*VR_rhs.ArgArray[0],AreSyntacticallyEqual))
			{
			size_t i = VR_rhs.fast_size();
			do	if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],AreSyntacticallyEqual))
					{
					VR_rhs.InferenceParameter1=i;
					return true;
					}
			while(1<i);
			}
		else if (   fast_size()+1==VR_rhs.fast_size()
				 && SubvectorArgList(VR_rhs))
			return 0==VR_rhs.InferenceParameter1;	// flag for this version
		};
	return false;
}

bool EqualRelation::CanStrictlyModifyEQUALTOONEOF(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if 		(rhs.IsExactType(NOTALLEQUAL_MC))
		{
		const EqualRelation& VR_rhs= static_cast<const EqualRelation&>(rhs);
		if (   3==fast_size()
			&& 3==VR_rhs.fast_size())
			return ArgListHasInjectionIntoRHSArgListRelatedThisWay(VR_rhs,AreSyntacticallyEqual);
		}
	else if (rhs.IsExactType(EQUALTOONEOF_MC))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if (   *ArgArray[0]==*VR_rhs.ArgArray[0]
			&& VerifyArgsExplicitConstant(1,fast_size()))
			{
			size_t i = VR_rhs.fast_size();
Restart1:	do	if (VR_rhs.ArgArray[--i]->IsExplicitConstant())
					{
					size_t j = fast_size();
					do	if (*ArgArray[--j]==*VR_rhs.ArgArray[i])
							goto Restart1;
					while(1<j);
					VR_rhs.InferenceParameter1 = i;
					return true;
					}
			while(1<i);
			};
		};
	return false;
}

bool
EqualRelation::CanStrictlyModifyDISTINCTFROMALLOF(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if	(rhs.IsExactType(EQUALTOONEOF_MC))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if (FindArgRelatedToLHS(*VR_rhs.ArgArray[0],AreSyntacticallyEqual))
			{
			if (0==InferenceParameter1)
				{	// arg0 EQUALTOONEOF arg1, ...; arg0 DISTINCTFROMALLOF arg1, ...: EQUALTOONEOF blots arg1
				size_t i = VR_rhs.fast_size();
				do	if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],AreSyntacticallyEqual))
						{
						InferenceParameter1 = 0;
						InferenceParameter2 = i;
						return true;
						}
				while(1<i);
				}
			else if (VR_rhs.FindArgRelatedToLHS(*ArgArray[0],AreSyntacticallyEqual))
				{	// arg0 EQUALTOONEOF arg1, ...; arg1 DISTINCTFROMALLOF arg0, ...: EQUALTOONEOF blots arg1
				InferenceParameter1 = 1;
				return true;
				}
			};
		}
	else if (rhs.IsExactType(ALLDISTINCT_MC))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if (VR_rhs.SubvectorArgList(*this))
			return 0==InferenceParameter1;
		}
	else if (rhs.IsExactType(NOTALLDISTINCT_MC))
		{
		const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
		if (VR_rhs.ArgListHasInjectionIntoRHSArgListRelatedThisWay(*this,AreSyntacticallyEqual))
			return VR_rhs.FindArgRelatedToLHS(*ArgArray[0],AreSyntacticallyEqual);	// blot arg0 from NOTALLDISTINCT
		};
	return false;
}

bool
EqualRelation::CanStrictlyModifyNOTALLDISTINCT(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	return false;
}

bool EqualRelation::CanStrictlyModifyNOTALLEQUAL(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	return false;
}

bool
EqualRelation::LogicalANDFindDetailedRule(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2000
	return (this->*LogicalANDDetailedRuleAux[array_index()])(rhs,LHSIdx,RHSIdx,Param1,Param2,SelfEvalIdx,EvalIdx);
}

// LogicalANDFindDetailedRuleALLEQUAL: primary rules are the substitution and symmetric-transitivity metarules
bool
EqualRelation::LogicalANDFindDetailedRuleALLEQUAL(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/26/2000
	if (rhs.IsExactType(ALLEQUAL_MC))
		{	// check for arg splicing on additive-inverse args
		if (FindTwoRelatedArgs(static_cast<EqualRelation&>(rhs),IsStdAdditionInverseTo))
			{
			Param1 = LHSIdx;
			Param2 = RHSIdx;
			SelfEvalIdx = LogicalANDSpliceALLEQUALAddInvArg_SER;
			return true;
			}
		};
	return false;
}

// LogicalANDFindDetailedRuleALLDISTINCT: completely relocated
	// ALLDISTINCT, ALLDISTINCT: AND doesn't work well 
	// ALLDISTINCT, EQUALTOONEOF: [moved this to StrictlyModify ALLDISTINCT]
	// * ALLDISTINCT(arg0,arg1), arg0 EQUALTOONEOF arg1, ...: EQUALTOONEOF blots arg1
	// ALLDISTINCT, DISTINCTFROMALLOF [this has been disassembled]
	// * ALLDISTINCT(arg0,arg1), arg2 DISTINCTFROMALLOF arg0, arg1, ...:
	//   this allows condensing out ALLDISTINCT(arg0,arg1,arg2)
	//   arity 3 DISTINCTFROMALLOF should retype, otherwise add arg2 to ALLDISTINCT and
	//   blot arg0, arg1 from DISTINCTFROMALLOF; n-ary version of this rule (uses SubvectorArglist)
	// ALLDISTINCT, NOTALLDISTINCT [over in StrictlyModifies]
	// * ALLDISTINCT(arg0,arg1), NOTALLDISTINCT(arg0,arg1,arg2)
	//   retype NOTALLDISTINCT to arg2 EQUALSONEOF arg0, arg1; n-ary version of this rule
	//   key: ALLDISTINCT arity+1==NOTALLDISTINCT arity; ALLDISTINCT subarglist NOTALLDISTINCT;
	//	 viewpoint arg of EQUALTOONEOF is the arg *not* in ALLDISTINCT
	//   we can use vector subarglist
	//   arity difference more than one: OR-clause results, gets messy

// LogicalANDFindDetailedRuleEQUALTOONEOF: completely relocated
	// EQUALTOONEOF, EQUALTOONEOF: AND doesn't work cleanly. [moved to StrictlyModifies EQUALTOONEOF]
	// * arg0==arg0, all other args const: remove all const args in other EQUALTOONEOF not in
	//   this one.  If all non-VP args removed, assert FALSE next higher statement.
	// EQUALTOONEOF, DISTINCTFROMALLOF:	[moved to StrictlyModifies DISTINCTFROMALLOF]
	// * arg0 EQUALTOONEOF arg1, ...; arg0 DISTINCTFROMALLOF arg1, ...: EQUALTOONEOF blots arg1
	// * arg0 EQUALTOONEOF arg1, ...; arg1 DISTINCTFROMALLOF arg0, ...: EQUALTOONEOF blots arg1
	// EQUALTOONEOF, NOTALLDISTINCT: AND doesn't work
	// EQUALTOONEOF, NOTALLEQUAL:	[moved to StrictlyModifies EQUALTOONEOF]
	// * arg0 EQUALTOONEOF arg1, arg2; NOTALLEQUAL(arg0,arg1,arg2)
	//   this is AND(OR(arg0==arg1,arg0==arg2),OR(arg0!=arg1,arg0!=arg2,arg1!=arg2))
	//   in either case, arg1!=arg2: assert this statement and TRUE out NOTALLEQUAL
	//   could be viewed as a "NOTALLEQUAL: delete arg0 and compatible-retype as ALLDISTINCT"
	//   analog rules: SelfEvalConvertToANDOtherArgs, SelfEvalConvertToOROtherArgs
	//   crashes at arity 4

bool
EqualRelation::LogicalANDFindDetailedRuleBothDISTINCTFROMALLOFAux(EqualRelation& rhs, size_t LHSIdx, size_t RHSIdx, size_t& VRParam1, size_t& VRParam2)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This is Franci's ALLDISTINCT extraction recognizer.
	if 		(FindArgRelatedToLHS(*rhs.ArgArray[0],AreSyntacticallyEqual))
		{	// RHS.Arg0 is arg1 of template
		if (fast_size()-1!=InferenceParameter1)
			SwapArgs(InferenceParameter1,fast_size()-1);
		size_t i = rhs.fast_size();
		do	if (   FindArgRelatedToLHS(*rhs.ArgArray[--i],AreSyntacticallyEqual)
				&& 0!=InferenceParameter1)
				{	// InferenceParameter1 points to the second match
					// InferenceParameter2 points to the first match
					// Param1 is the base from which to extract ALLDISTINCT
				VRParam1 = LHSIdx;
				VRParam2 = RHSIdx;
				return true;
				}
		while(1<i);
		};
	return false;
}

bool
EqualRelation::LogicalANDFindDetailedRuleDISTINCTFROMALLOF(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// DISTINCTFROMALLOF, DISTINCTFROMALLOF
	// * arg0==arg0: merge arglists on arg0
	// * arg0 DISTINCTFROMALLOF arg1, arg2, ...; arg1 DISTINCTFROMALLOF arg2, ...:
	//	 this allows a condensation to ALLDISTINCT(arg0,arg1,arg2)
	//   then arg0 DISTINCTFROMALLOF blots arg1, arg2 and arg1 DISTINCTFROMALLOF arg2, ... blots arg2
	//   3-ary DISTINCTFROMALLOF should simply retype itself; higher arities will add an argument
	//   retype version is a StrictlyModifies DISTINCTFROMALLOF mirror; arg0 may or may not be
	//	 in second clause
	// * latter two rules have n-ary versions
	// DISTINCTFROMALLOF, NOTALLDISTINCT [moved to StrictlyModifies DISTINCTFROMALLOF]
	// arg0 DISTINCTFROMALLOF arg1, arg2, ...; NOTALLDISTINCT(arg0,arg1,arg2,...)
	// * NOTALLDISTINCT subarglist DISTINCTFROMALLOF: NOTALLDISTINCT should blot arg0
	if 		(rhs.IsExactType(DISTINCTFROMALLOF_MC))
		{
		EqualRelation& VR_rhs = static_cast<EqualRelation&>(rhs);
		if (*ArgArray[0]==*VR_rhs.ArgArray[0])
			{	// Merge arglists on arg0
			InferenceParameter1 = 0;
			VR_rhs.InferenceParameter1 = 0;
			Param1 = LHSIdx;
			Param2 = RHSIdx;
			SelfEvalIdx = LogicalANDSpliceNAryEqualArg_SER;
			return true;
			};
		// both extractions of ALLDISTINCT
		if (   LogicalANDFindDetailedRuleBothDISTINCTFROMALLOFAux(VR_rhs,LHSIdx,RHSIdx,Param1,Param2)
			|| VR_rhs.LogicalANDFindDetailedRuleBothDISTINCTFROMALLOFAux(*this,LHSIdx,RHSIdx,Param2,Param1))
			{
			SelfEvalIdx = DISTINCTFROMALLOFExtractALLDISTINCT_SER;
			return true;
			};
		};
	return false;
}

// NOTALLDISTINCT, NOTALLDISTINCT: AND of these doesn't work well
// NOTALLDISTINCT, NOTALLEQUAL:
// ary-3: NOTALLDISTINCT(A,B,C), NOTALLEQUAL(A,B,C):
// rewrites to AND(OR(A==B,A==C,B==C),OR(A!=B,A!=C,B!=C))
// because of transitivity, this actually is
// AND(XOR(A==B,A==C,B==C),OR(A!=B,A!=C,B!=C))
// i.e XOR(A==B!=C,A==C!=B,B==C!=A)
// this crashes at arity 4
// this probably isn't worth acting on: this is *not* RAM-conservative.
// Franci's future-proofed code would use this rewrite to suppress ALLEQUAL/ALLDISTINCT basis
// pairs, which is counter-productive.

// LogicalANDFindDetailedRuleNOTALLEQUAL: no effect
bool EqualRelation::LogicalANDNonTrivialFindDetailedRule() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2000
	return NULL!=LogicalANDDetailedRuleAux[array_index()];
}

bool EqualRelation::LogicalANDOrthogonalClause() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/22/2000
	if 		(IsExactType(ALLEQUAL_MC))
		{
		if (   ArgArray[fast_size()-1]->IsExactType(Variable_MC)
			&& (    ArgArray[0]->IsExactType(Variable_MC)
				|| (ArgArray[0]->IsExplicitConstant() && ArgArray[1]->IsExactType(Variable_MC))))
			return true;
		};
	return false;
}

// Basis clause handlers for EqualRelation
// EQUALTOONEOF: n-l 2-ary ALLEQUAL clauses
// NOTALLDISTINCT: n EQUALTOONEOF clauses
//! \todo IMPLEMENT: n-ary NOTALLEQUAL: n(n-1)/2 [triangular number n] ALLDISTINCT pairs.
size_t EqualRelation::BasisClauseCount() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/8/2000
	if 		(IsExactType(EQUALTOONEOF_MC))
		return fast_size()-1;
	else if (IsExactType(NOTALLDISTINCT_MC))
		return fast_size();
	return 0;
}

//! \todo this is the dismember definition.  Change to more powerful paradigm when 
//! appropriate: AND(dismember,negation of everything else) along with inference rules 
//! to force correct reduction.
bool EqualRelation::DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/8/2000
	assert(BasisClauseCount()>Idx);
	assert(NULL==dest);
	MetaConcept** TmpArgArray = NULL;
	try	{
		if 		(IsExactType(EQUALTOONEOF_MC))
			{
			TmpArgArray = _new_buffer<MetaConcept*>(2);
			if (!TmpArgArray) return false;
			ArgArray[0]->CopyInto(TmpArgArray[0]);
			ArgArray[Idx+1]->CopyInto(TmpArgArray[1]);
			dest = new EqualRelation(TmpArgArray,ALLEQUAL_EM);
			return true;
			}
		else if (IsExactType(NOTALLDISTINCT_MC))
			{
			TmpArgArray = _new_buffer<MetaConcept*>(fast_size());
			if (!TmpArgArray) return false;
			ArgArray[Idx]->CopyInto(TmpArgArray[0]);
			size_t i = fast_size();
			while(Idx< --i) ArgArray[i]->CopyInto(TmpArgArray[i]);
			while(0<i)
				{
				i--;
				ArgArray[i]->CopyInto(TmpArgArray[i+1]);
				};
			dest = new EqualRelation(TmpArgArray,EQUALTOONEOF_EM);
			return true;
			};
		}
	catch(const bad_alloc&)
		{
		BLOCKDELETEARRAY_AND_NULL(TmpArgArray);
		return false;
		};
	FATAL(AlphaMiscallVFunction);
	return false;
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > EqualRelation::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

//  Helper functions for CanEvaluate... routines
void EqualRelation::DiagnoseInferenceRules() const
{	// MUTABLE
	assert(2!=size() || ArgArray[0]->IsExactType(LinearInterval_MC) || ArgArray[1]->IsExactType(LinearInterval_MC) || EQUALTOONEOF_MC>ExactType());
	//! \todo (A,~A,...): [A UltimateType TruthValue]
	//! TruthValue effects
	//!  * ALLEQUAL: rewrite to IFF (this is *not* sametype!)
	//!  * ALLDISTINCT:	3+ UltimateType TruthValue -> CONTRADICTION
	//!  * NOTALLDISTINCT: 3 UltimateType TruthValue -> TRUE
	//!	* NOTALLEQUAL: if all args TruthValue
	if (   DiagnoseEqualArgs()								// do == check here
		|| DiagnoseEvaluatableArgs()						// further rules benefit from maximal evaluation
		|| (this->*UseConstantsAux[array_index()])()		// constant processing
		|| (this->*UseDomainsAux[array_index()])()			// use domains
		|| (this->*UseStdAdditionAux[array_index()])()		// use StdAddition
		|| (this->*UseStdMultiplicationAux[array_index()])())	// use StdMultiplication
		return;
	IdxCurrentSelfEvalRule=SelfEvalSyntaxOKNoRules_SER;
}

bool EqualRelation::InvokeEqualArgRule() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/25/2000
	// ALLEQUAL: vector clean; arity 2 TRUE
	// ALLDISTINCT: |-> CONTRADICTION	// done
	// EQUALTOONEOF: arg 0 |-> TRUE, others: clean; 2-ary reduces to ALLEQUAL
	// DISTINCTFROMALLOF: arg 0 |-> CONTRADICTION, others: clean; 2-ary reduces to ALLDISTINCT
	// NOTALLDISTINCT |-> TRUE	// done
	// NOTALLEQUAL:	vector clean; arity 2 reduces to ALLDISTINCT
	// InferenceParameter1>InferenceParameter2, by prologue to this routine
	return (this->*EqualArgAux[array_index()])();
}

bool EqualRelation::InvokeEqualArgRuleALLEQUAL() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/25/2000
	if (2==fast_size())
		{	// TRUE
		IdxCurrentEvalRule=EvalForceTrue_ER;
		return true;
		}
	// CLEAN
	IdxCurrentSelfEvalRule=SelfEvalRuleCleanArg_SER;
	return CheckForTrailingCleanArg(AreSyntacticallyEqual,SelfEvalRuleCleanTrailingArg_SER,2);
}

bool EqualRelation::InvokeEqualArgRuleALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/25/2000
	IdxCurrentEvalRule=EvalForceContradiction_ER;
	return true;		
}

bool EqualRelation::InvokeEqualArgRuleEQUALTOONEOF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/25/2000
	if (0==InferenceParameter2)
		{	// TRUE
		IdxCurrentEvalRule=EvalForceTrue_ER;
		return true;
		}
	// CLEAN; leftover ary-2 converts
	IdxCurrentSelfEvalRule=SelfEvalRuleAry2CorrectedCleanArg_SER;
	InferenceParameter2=ALLEQUAL_MC;
	return CheckForTrailingCleanArg(AreSyntacticallyEqual,SelfEvalRuleAry2CorrectedCleanTrailingArg_SER,2);
}

bool EqualRelation::InvokeEqualArgRuleDISTINCTFROMALLOF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/25/2000
	if (0==InferenceParameter2)
		{	// CONTRADICTION
		IdxCurrentEvalRule=EvalForceContradiction_ER;
		return true;
		}
	// CLEAN; leftover ary-2 converts
	IdxCurrentSelfEvalRule=SelfEvalRuleAry2CorrectedCleanArg_SER;
	InferenceParameter2=ALLDISTINCT_MC;
	return CheckForTrailingCleanArg(AreSyntacticallyEqual,SelfEvalRuleAry2CorrectedCleanTrailingArg_SER,2);
}

bool EqualRelation::InvokeEqualArgRuleNOTALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/25/2000
	IdxCurrentEvalRule=EvalForceTrue_ER;
	return true;		
}

bool EqualRelation::InvokeEqualArgRuleNOTALLEQUAL() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/25/2000
	IdxCurrentSelfEvalRule=SelfEvalRuleAry2CorrectedCleanArg_SER;
	InferenceParameter2=ALLDISTINCT_MC;
	return CheckForTrailingCleanArg(AreSyntacticallyEqual,SelfEvalRuleAry2CorrectedCleanTrailingArg_SER,2);
}

//! \todo Must integrate LinearInterval of UltimateType _Z_ into the EqualRelation logic engine.
//!  Modification target: UseConstantsAux.
//!  In general, LinearIntervals of UltimateType _Z_ act like arglists of integer constants.
//!						overlap	solo					
//!  ALLEQUAL			merge	force endpoints equal
//!  ALLDISTINCT		FALSE	
//!  DISTINCTFROMALLOF	merge	
//!  EQUALTOONEOF		merge	
//!  NOTALLDISTINCT		TRUE	
//!  NOTALLEQUAL		TRUE for arity 2+

// META: UseConstants is called *after* StandardDiagnoseRules.
// thus: all args are evaluated already, and are known not to be syntactically equal.
// for constants, syntactical inequality implies inequality....
//! \todo consider code condensation of UseConstantsALLEQUAL, UseConstantsNOTALLEQUAL
bool EqualRelation::UseConstantsALLEQUAL() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/5/2002
	// ALLEQUAL has two constants: this is FALSE
	if (FindTwoRelatedArgs(IsClearlyDistinctFrom))
		{
		InvokeEvalForceFalse();
		return true;
		}

	// start to handle ellipses
	if (FindArgTypeMatch(LinearInterval_MC,&Integer))
		{
		if (IsClearlyDistinctFrom(*ArgArray[InferenceParameter1]->ArgN(0),*ArgArray[InferenceParameter1]->ArgN(1)))
			{
			InvokeEvalForceFalse();
			return true;
			}
		IdxCurrentSelfEvalRule=DismantleLinearIntervalToEndpoints_SER;
		return true;
		};
	return false;
}

bool EqualRelation::UseConstantsALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/12/2002
	// we use the standard form: all constant args are first.
	// we also have all args distinct.
	if (ArgArray[fast_size()-1]->IsExplicitConstant())
		{
		IdxCurrentEvalRule=EvalForceTrue_ER;
		return true;
		}
	else if (2==fast_size())
		{
		if (IsClearlyDistinctFrom(*ArgArray[0],*ArgArray[1]))
			{
			IdxCurrentEvalRule=EvalForceTrue_ER;
			return true;
			};
		if (IsClearlyIndistinctFrom(*ArgArray[0],*ArgArray[1]))
			{
			InvokeEvalForceFalse();
			return true;
			};
		if 		(ArgArray[0]->IsTypeMatch(LinearInterval_MC,&Integer))
			{
			if (!ArgArray[1]->IsTypeMatch(LinearInterval_MC,&Integer))
				{
				SwapArgs(0,1);
				InferenceParameter1 = DISTINCTFROMALLOF_MC;
				IdxCurrentSelfEvalRule = CompatibleRetype_SER;
				return true;
				}
			}
		else if (ArgArray[1]->IsTypeMatch(LinearInterval_MC,&Integer))
			{
			InferenceParameter1 = DISTINCTFROMALLOF_MC;
			IdxCurrentSelfEvalRule = CompatibleRetype_SER;
			return true;
			};
		return false;
		}
	else if (   3<=fast_size()
			 && ArgArray[fast_size()-2]->IsExplicitConstant())
		{
		SwapArgs(0,fast_size()-1);
		InferenceParameter1 = DISTINCTFROMALLOF_MC;
		IdxCurrentSelfEvalRule = CompatibleRetype_SER;
		return true;
		};

	if (FindTwoRelatedArgs(IsClearlyIndistinctFrom))
		{
		InvokeEvalForceFalse();
		return true;
		};

	size_t* EllipsisIndexMap = NULL;
	if (::GrepArgList(EllipsisIndexMap,IsEllipsisArgList,ArgArray))
		{
		// arglists that overlap, false out
		// arglists that are only mergeable, merge
		if (2<=ArraySize(EllipsisIndexMap))
			{
			size_t i = ArraySize(EllipsisIndexMap);
			do	{
				size_t j = --i;
				// mergeable => use merge code
				do	if (static_cast<LinearInterval*>(ArgArray[EllipsisIndexMap[j]])->ClearlyMergeable(*static_cast<LinearInterval*>(ArgArray[EllipsisIndexMap[i]])))
						{
						InferenceParameter1 = EllipsisIndexMap[j];
						InferenceParameter2 = EllipsisIndexMap[i];
						free(EllipsisIndexMap);
						IdxCurrentSelfEvalRule=MergeIntervals_SER;
						return true;
						}
				while(0<j);
				}
			while(1<i);
			}
		free(EllipsisIndexMap);
		}

	if 		(IsVirtualExplicitConstant(*ArgArray[fast_size()-1]))
		{
		IdxCurrentEvalRule=EvalForceTrue_ER;
		return true;
		}
	else if (   3<=fast_size()
			 && IsVirtualExplicitConstant(*ArgArray[fast_size()-2]))
		{
		SwapArgs(0,fast_size()-1);
		InferenceParameter1 = DISTINCTFROMALLOF_MC;
		IdxCurrentSelfEvalRule = CompatibleRetype_SER;
		return true;
		}

	//! \todo IMPLEMENT if arity 4+, k of n constants, 2+ constant and 2+ non-constant:
	//! rewrite to AND of:
	//! n-k k+1-ary DISTINCTFROMALLOF clauses, vertex nonconstant, other args constant
	//! 1 n-k-ary ALLDISTINCT clause
	return false;
}

bool EqualRelation::UseConstantsEQUALTOONEOForDISTINCTFROMALLOF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/11/2000
	// arg0 constant: other arg constant: delete arg
	if (2==fast_size())
		{
		if (IsClearlyDistinctFrom(*ArgArray[1],*ArgArray[0]))
			{
			IdxCurrentEvalRule = (IsExactType(DISTINCTFROMALLOF_MC)) ? EvalForceTrue_ER : EvalForceFalse_ER;
			return true;
			};
		if (IsClearlyIndistinctFrom(*ArgArray[1],*ArgArray[0]))
			{
			IdxCurrentEvalRule = (IsExactType(DISTINCTFROMALLOF_MC)) ? EvalForceFalse_ER : EvalForceTrue_ER;
			return true;
			};
		}
	else{
		if (FindArgRelatedToLHS(*ArgArray[0],IsClearlyDistinctFrom,1,fast_size()-1))
			{
			IdxCurrentSelfEvalRule=SelfEvalRuleAry2CorrectedCleanArg_SER;
			InferenceParameter2=ExactType()-2;	// this maps	EQUALTOONEOF to ALLEQUAL
												//				DISTINCTFROMALLOF to ALLDISTINCT
			//! \todo OPTIMIZE: check for vector delete
			return true;
			};
		if (FindArgRelatedToLHS(*ArgArray[0],IsClearlyIndistinctFrom,1,fast_size()-1))
			{
			IdxCurrentEvalRule = (IsExactType(DISTINCTFROMALLOF_MC)) ? EvalForceFalse_ER : EvalForceTrue_ER;
			return true;
			};
		if (   FindTwoRelatedArgs(IsSubclassAsEnumeratedSet)
			|| DualFindTwoRelatedArgs(IsSubclassAsEnumeratedSet))
			{
			IdxCurrentSelfEvalRule=SelfEvalRuleAry2CorrectedCleanArg_SER;
			InferenceParameter2=ExactType()-2;	// this maps	EQUALTOONEOF to ALLEQUAL
												//				DISTINCTFROMALLOF to ALLDISTINCT
			//! \todo OPTIMIZE: check for vector delete
			return true;
			}
		}

	size_t* EllipsisIndexMap = NULL;
	size_t* NoEllipsisIndexMap = NULL;
	if (::DualGrepArgList(EllipsisIndexMap,NoEllipsisIndexMap,IsEllipsisArgList,ArgArray))
		{
		if (!EllipsisIndexMap)
			free(NoEllipsisIndexMap);
		else{
			if (NoEllipsisIndexMap)
				{	// Extend checks; want 1...3, 4 to merge to 1...4
				size_t i = ArraySize(EllipsisIndexMap);
				do	{
					i--;
					size_t j = ArraySize(NoEllipsisIndexMap);
					do	{
						if (static_cast<LinearInterval*>(ArgArray[EllipsisIndexMap[i]])->ClearlyExtendedBy(*ArgArray[NoEllipsisIndexMap[--j]]))
							{
							InferenceParameter1 = EllipsisIndexMap[i];
							InferenceParameter2 = NoEllipsisIndexMap[j];
							free(EllipsisIndexMap);
							free(NoEllipsisIndexMap);
							IdxCurrentSelfEvalRule=ExtendLinearInterval_SER;
							return true;
							}
						}
					while(0<j);
					}
				while(0<i);
				free(NoEllipsisIndexMap);
				};
			if (2<=ArraySize(EllipsisIndexMap))
				{
				size_t i = ArraySize(EllipsisIndexMap);
				do	{
					size_t j = --i;
					do	if (static_cast<LinearInterval*>(ArgArray[EllipsisIndexMap[--j]])->ClearlyMergeable(*static_cast<LinearInterval*>(ArgArray[EllipsisIndexMap[i]])))
							{
							InferenceParameter1 = EllipsisIndexMap[j];
							InferenceParameter2 = EllipsisIndexMap[i];
							free(EllipsisIndexMap);
							IdxCurrentSelfEvalRule=MergeIntervals_SER;
							return true;
							}
					while(0<j);
					}
				while(1<i);
				}
			free(EllipsisIndexMap);	
			}
		}
	return false;
}

bool EqualRelation::UseConstantsNOTALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/12/2002
	// we use the standard form: all constant args are first.
	// we also have all args distinct.
	if (ArgArray[fast_size()-1]->IsExplicitConstant())
		{
		InvokeEvalForceFalse();
		return true;
		}
	else if (2==fast_size())
		{
		if (IsClearlyDistinctFrom(*ArgArray[0],*ArgArray[1]))
			{
			InvokeEvalForceFalse();
			return true;
			};
		if (IsClearlyIndistinctFrom(*ArgArray[0],*ArgArray[1]))
			{
			IdxCurrentEvalRule=EvalForceTrue_ER;
			return true;
			};
		if 		(ArgArray[0]->IsTypeMatch(LinearInterval_MC,&Integer))
			{
			if (!ArgArray[1]->IsTypeMatch(LinearInterval_MC,&Integer))
				{
				SwapArgs(0,1);
				InferenceParameter1 = EQUALTOONEOF_MC;
				IdxCurrentSelfEvalRule = CompatibleRetype_SER;
				return true;
				}
			}
		else if (ArgArray[1]->IsTypeMatch(LinearInterval_MC,&Integer))
			{
			InferenceParameter1 = EQUALTOONEOF_MC;
			IdxCurrentSelfEvalRule = CompatibleRetype_SER;
			return true;
			};
		return false;
		}
	else if (   3<=fast_size()
			 && ArgArray[fast_size()-2]->IsExplicitConstant())
		{
		SwapArgs(0,fast_size()-1);
		InferenceParameter1 = DISTINCTFROMALLOF_MC;
		IdxCurrentSelfEvalRule = CompatibleRetype_SER;
		return true;
		};

	if (FindTwoRelatedArgs(IsClearlyIndistinctFrom))
		{
		IdxCurrentEvalRule=EvalForceTrue_ER;
		return true;
		};

	size_t* EllipsisIndexMap = NULL;
	if (::GrepArgList(EllipsisIndexMap,IsEllipsisArgList,ArgArray))
		{
		// arglists that overlap, false out
		// arglists that are only mergeable, merge
		if (2<=ArraySize(EllipsisIndexMap))
			{
			size_t i = ArraySize(EllipsisIndexMap);
			do	{
				size_t j = --i;
				// mergeable => use merge code
				do	if (static_cast<LinearInterval*>(ArgArray[EllipsisIndexMap[j]])->ClearlyMergeable(*static_cast<LinearInterval*>(ArgArray[EllipsisIndexMap[i]])))
						{
						InferenceParameter1 = EllipsisIndexMap[j];
						InferenceParameter2 = EllipsisIndexMap[i];
						free(EllipsisIndexMap);
						IdxCurrentSelfEvalRule=MergeIntervals_SER;
						return true;
						}
				while(0<j);
				}
			while(1<i);
			}
		free(EllipsisIndexMap);
		}

	if 		(IsVirtualExplicitConstant(*ArgArray[fast_size()-1]))
		{
		InvokeEvalForceFalse();
		return true;
		}
	else if (   3<=fast_size()
			 && IsVirtualExplicitConstant(*ArgArray[fast_size()-2]))
		{
		SwapArgs(0,fast_size()-1);
		InferenceParameter1 = EQUALTOONEOF_MC;
		IdxCurrentSelfEvalRule = CompatibleRetype_SER;
		return true;
		}

	//! \todo IMPLEMENT if arity 4+, k of n non-virtual constants, 2+ virtual constant and 2+ non-virtual constant:
	//! rewrite to OR of:
	//! n-k k+1-ary EQUALTOONEOF clauses, vertex nonconstant, other args constant
	//! 1 n-k-ary NOTALLDISTINCT clause
	return false;
}

bool EqualRelation::UseConstantsNOTALLEQUAL() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/5/2002
	// NOTALLEQUAL has two constants: this is TRUE
	if (FindTwoRelatedArgs(IsClearlyDistinctFrom))
		{
		IdxCurrentEvalRule=EvalForceTrue_ER;
		return true;
		}

	// start to handle ellipses
	if (FindArgTypeMatch(LinearInterval_MC,&Integer))
		{
		if (IsClearlyDistinctFrom(*ArgArray[InferenceParameter1]->ArgN(0),*ArgArray[InferenceParameter1]->ArgN(1)))
			{
			IdxCurrentEvalRule=EvalForceTrue_ER;
			return true;
			}
		// sweep both endpoints against all non-EllipsisArgList args
		// any IsClearlyDistinctFrom hits will force true
		size_t i = fast_size();
		do	if (--i!=InferenceParameter1)
				{
				if (   IsClearlyDistinctFrom(*ArgArray[InferenceParameter1]->ArgN(0),*ArgArray[i])
					|| IsClearlyDistinctFrom(*ArgArray[InferenceParameter1]->ArgN(1),*ArgArray[i]))
					{
					IdxCurrentEvalRule=EvalForceTrue_ER;
					return true;
					};
				}
		while(0<i);
		//! \todo final rewrite of NOTALLEQUAL(A,B,C...D)
		//! to OR(ALLDISTINCT(C,D),NOTALLEQUAL(A,B,C))
		IdxCurrentEvalRule = ReduceIntervalForNOTALLEQUAL_ER;
		return true;
		}
	return false;
}

//! \todo (?): global translation-by-constant alignment (default for all 6) (implemented for EQUALTOONEOF, DISTINCTFROMALLOF)
//! should be careful for other four (RAM issues, reduced form issues).
//! \todo global common-summand cancel (probably should call last...natural for private boolean helper function).
// ========
//! \todo X==-X => PERIOD_OF X UNDER + DIVISOR_OF 2
//! \todo X!=-X => PERIOD_OF X UNDER + NOT_DIVISOR_OF 2
//! these reduce to X==0, X!=0, TRUE, FALSE as appropriate.
//! NOTE: short-circuit: check for !UltimateType()->MayHaveElementsOfPeriodNUnderOp(2,StdAddition_MC) [Abstractclass member function]
//! \todo X+const1==X+const2 (distinct) => PERIOD_OF X UNDER + DIVISOR_OF (const2-const1)  [const2-const1 in _Z_]
//! NOTE: short-circuit: ....
//! To make this work:
//! PERIOD_OF X UNDER + : maps to family of 1-ary MetaConcepts Period(X,operationname) with 
//! ultimatetype _Z_, and 1 arg of arbitrary type supporting the indicated operation.  This goes
//! through a 1-ary Phrase.  The parser should intercept this before + tries to coerce vars.
//! NOTE: For this application, this wants the interface function
//! bool CanBeSemanticallyEqualTo(const MetaConcept&) const [this enables symbolics to "know" if 
//! they could satisfy 2-ary ALLEQUAL; failure guarantees distinctness.  This might help the 
//! EqualRelation code overall.]
//! (dual to above is not useful)
//! X DIVISOR_OF Y :
//! X NOT_DIVISOR_OF Y :
//! 2-ary clause pair (logical NOT duality, again).  Arguments must support *.  Automatically
//! TRUE/FALSE for fields (Franci will attempt to autotype to avoid this: _Q_/_R_/_C_ sums will 
//! be autotyped to _Z_, for instance.).  Hardware unsigned long-representable may be cast 
//! as 1-ary clauses for a single instance; if both are, cast to 0-ary clause followed by 
//! evaluation). 0 DIVISOR_OF Y evaluates to UNKNOWN (both types of definition useful)
//! X DIVISOR_OF 0 evaluates to FALSE.
//! X DIVISOR_OF (IntegerNumeral constant) will evaluate to an X EQUALTOONEOF (factors) clause.
//! ========
//! class construction analog: _DEFINED_(+) (AbstractClass needs mode to deal with); retro _ADD_DEF_ to this
bool EqualRelation::UseStdAdditionALLEQUAL() const
{	//! \todo IMPLEMENT
	//! really should look for "terms interact under +"
	//! e.g.: "const interacts with const under +"
	//! "Solve if meaningful" rules
	if (ArgArray[0]->IsExplicitConstant())
		{
		// 0=X+Y rewrites as X=-Y.  More generally, 0==2-ary sum rewrites that way.
		// 0 is always an explicit constant.
		if (ArgArray[0]->IsZero())
			{
			if (2==fast_size())
				{
				if (   ArgArray[1]->IsExactType(StdAddition_MC)
					&& 2==ArgArray[1]->size()
					&& ArgArray[1]->NoMetaModifications())
					{
					IdxCurrentSelfEvalRule = Ary2EqRewriteZeroEq2ArySum_SER;
					return true;					
					}
				}
			else{
				size_t i = fast_size();
				do	if (   ArgArray[--i]->IsExactType(StdAddition_MC)
						&& 2==ArgArray[i]->size()
						&& ArgArray[i]->NoMetaModifications())
						{
						IdxCurrentEvalRule = NAryEqRewriteZeroEq2ArySum_ER;
						InferenceParameter1 = i;
						return true;
						}
				while(1<i);
				}
			}
		if (2==fast_size())
			{
			if (   ArgArray[1]->IsExactType(StdAddition_MC)
				&& ArgArray[1]->NoMetaModifications()
				&& static_cast<MetaConceptWithArgArray*>(ArgArray[1])->FindExplicitConstantArgInArgArray())
				{	// cancel args
				IdxCurrentSelfEvalRule = AddInvOutStdAddArg_SER;
				InferenceParameter1 = 1;	// StdAdd arg, targeted parameter is InferenceParameter1 of this
				return true;
				}
			}
		else{
			size_t i = fast_size();
			while(0<--i)
				if (   ArgArray[i]->IsExactType(StdAddition_MC)
					&& ArgArray[i]->NoMetaModifications()
					&& static_cast<MetaConceptWithArgArray*>(ArgArray[i])->FindExplicitConstantArgInArgArray())
					{	// type-change (turns to AND of two ALLEQUAL clauses, 2-ary and (n-1)-ary
					IdxCurrentEvalRule = NAryEQUALSpawn2AryEQUAL_ER;
					InferenceParameter1 = 0;	// Target
					InferenceParameter2 = i;	// Sum with arg in InferenceParameter1
					return true;
					}
			}
		}
	// ALTERNATIVE: No constants, and stable.  Assume StdAddition_MC is supported by all args.
	// If at least one arg is not a sum, and doesn't have other difficulties [MultInv or worse],
	// all of the StdAdditions can be rewritten as 0==StdAddition-(special arg) 
	// [which is hopefully simple].  We most definitely want to give other options a chance to 
	// fire.
	return false;
}

bool EqualRelation::UseStdAdditionALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/30/2001
	// "Solve if meaningful" rules
	if (ArgArray[0]->IsExplicitConstant())
		{	// NOTE: if n-1 constants, the constant-info rule goes off first
		// IMPLEMENT: dual to ALLEQUAL handling of 0=X-Y
		// 2-ary: OK
		// n-ary(?): 0, k matches, m non-matches
		// breaks up into processing k-ary ALLDISTINCT against 0 (processes), m-ary ALLDISTINCT against 0 (static),
		// and k+m-ary ALLDISTINCT (against each other)
		// ALTERNATIVE: maybe Franci prefers constant<>X-Y...because it lets her centralize
		// inequality info more efficiently?  [she ends up with a bunch of constant-vertex 
		// DISTINCTFROMALLOF clauses, generally]  In fact, does Franci like disintegrating
		// ALLDISTINCT into a cloud of DISTINCTFROMALLOF clauses with constant vertex?
		// this also affects her opinions about DISTINCTFROMALLOF.
		if (   2==fast_size() && ArgArray[1]->IsExactType(StdAddition_MC)
			&& static_cast<MetaConceptWithArgArray*>(ArgArray[1])->FindExplicitConstantArgInArgArray())
			{	// 1 StdAddition arg w/IntegerNumeral, n-1 IntegerNumeral
			IdxCurrentSelfEvalRule = AddInvOutStdAddArg_SER;
			InferenceParameter1 = 1;	// StdAdd arg, targeted parameter is InferenceParameter1 of this
			return true;
			}
		// DO NOT IMPLEMENT: IMPLEMENT DUAL to ALLEQUAL version, which splits off into 2-ary ALLEQUAL
		// and n-1-ary ALLEQUAL
		// this translates to AND of:
		// *	n-ary DISTINCTFROMALLOF(target is vertex): to be solved as normal
		// *	n-1-ary ALLDISTINCT(target excluded)
		// * problem: this decentralizes the != info...and Franci doesn't have a quick integration method.
		}
	return false;
}

bool EqualRelation::UseStdAdditionEQUALTOONEOForDISTINCTFROMALLOF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/30/2001
	// "Solve if meaningful" rules
	// IMPLEMENT(?): parallel to ALLEQUAL handling of 0=X-Y
	// 0-vertex DISTINCTFROMALLOF: n-ary, k matches, m non-matches
	// processes into AND: k results of 2-ary ALLDISTINCT, m+1-ary DISTINCTFROMALLOF 0
	// 0-nonvertex DISTINCTFROMALLOF: processes into AND: 2-ary ALLDISTINCT, n-1 ary DISTINCTFROMALLOF
	// 0-vertex EQUALTOONEOF: dual of DISTINCTFROMALLOF case
	// 0-nonvertex EQUALTOONEOF: dual of DISTINCTFROMALLOF case.
	// ALTERNATIVE: maybe Franci wants to disintegrate DISTINCTFROMALLOF into a series, here
	if (   ArgArray[0]->IsExactType(StdAddition_MC)
		&& static_cast<MetaConceptWithArgArray*>(ArgArray[0])->FindExplicitConstantArgInArgArray())
		{
		IdxCurrentSelfEvalRule = AddInvOutStdAddArg_SER;
		InferenceParameter1 = 0;	// StdAdd arg, targeted parameter is InferenceParameter1 of this
		return true;
		}
	return false;
}

bool EqualRelation::UseStdAdditionNOTALLDISTINCT() const
{	//! \todo IMPLEMENT
	// NOTE: if n-1 constants, constant rule goes off first
	// 2-ary does *not* dualize to here
	// IMPLEMENT: parallel to ALLEQUAL handling of 0=X-Y
	// Result: OR of n-ary EQUALTOONEOF (vertex 0), NOTALLDISTINCT(n-1 ary, lose 0)
	return false;
}

bool EqualRelation::UseStdAdditionNOTALLEQUAL() const
{	//! \todo IMPLEMENT
	// 2-ary does *not* dualize to here
	// not an equiv relation, so cannot dualize other clause
	return false;
}

bool EqualRelation::UseStdMultiplicationALLEQUAL() const
{	//! \todo IMPLEMENT
	// "Solve if meaningful" rules
	if (    ArgArray[0]->IsExplicitConstant()
		&& !ArgArray[0]->IsUltimateType(NULL)
		&&  ArgArray[0]->UltimateType()->SupportsThisOperation(StdMultiplication_MC))
		{
		if (2==fast_size())
			{
			if (   ArgArray[1]->IsExactType(StdMultiplication_MC)
//! \todo this function should be responsible for checking the safety of the move
//! Unsafe moves may be handled via mathematical functions, if suitable workarounds exist.
				&& static_cast<MetaConceptWithArgArray*>(ArgArray[1])->FindLRMultInvCompetentExplicitConstArgInArgArray())
				{	// cancel args
				// Indicated arg is immediately either be a left-inverse or a right-inverse.
				IdxCurrentSelfEvalRule = MultInvOutStdMultArg_SER;
				InferenceParameter1 = 1;	// Target				
				return true;
				}
			}
		else{
			size_t i = fast_size();
			while(0< --i)
				if (   ArgArray[i]->IsExactType(StdMultiplication_MC)
					&& static_cast<MetaConceptWithArgArray*>(ArgArray[i])->FindLRMultInvCompetentExplicitConstArgInArgArray())
					{	// type-change (turns to AND of two ALLEQUAL clauses, 2-ary and (n-1)-ary
					// Indicated arg is immediately either be a left-inverse or a right-inverse.
					IdxCurrentEvalRule = NAryEQUALSpawn2AryEQUAL_ER;
					InferenceParameter1 = 0;	// Target
					InferenceParameter2 = i;	// Product with arg in InferenceParameter1
					return true;
					}
			}
		}
	return false;
}

bool EqualRelation::UseStdMultiplicationALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/30/2001
	// "Solve if meaningful" rules
	if (    ArgArray[0]->IsExplicitConstant()
		&& !ArgArray[0]->IsUltimateType(NULL)
		&&  ArgArray[0]->UltimateType()->SupportsThisOperation(StdMultiplication_MC))
		{	// NOTE: if n-1 constants, the constant-info rule goes off first
		if (   2==fast_size()
			&& ArgArray[1]->IsExactType(StdMultiplication_MC)
			&& static_cast<MetaConceptWithArgArray*>(ArgArray[1])->FindLRMultInvCompetentExplicitConstArgInArgArray())
			{	// 1 StdAddition arg w/IntegerNumeral, n-1 IntegerNumeral
			IdxCurrentSelfEvalRule = MultInvOutStdMultArg_SER;
			InferenceParameter1 = 1;	// StdMult arg, targeted parameter is InferenceParameter1 of this
			return true;
			}
		// DO NOT IMPLEMENT: IMPLEMENT DUAL to ALLEQUAL version, which splits off into 2-ary ALLEQUAL
		// and n-1-ary ALLEQUAL
		// this translates to AND of:
		// *	n-ary DISTINCTFROMALLOF(target is vertex): to be solved as normal
		// *	n-1-ary ALLDISTINCT(target excluded)
		// * problem: this decentralizes the != info...and Franci doesn't have a quick integration method.
		}
	return false;
}

bool EqualRelation::UseStdMultiplicationEQUALTOONEOForDISTINCTFROMALLOF() const
{	// TODO: VERIFY
	// NOTE: must check for both left and right multiplication
	// "Solve if meaningful" rules
	if (   ArgArray[0]->IsExactType(StdMultiplication_MC)
		&& static_cast<MetaConceptWithArgArray*>(ArgArray[0])->FindLRMultInvCompetentExplicitConstArgInArgArray())
		{	//! \todo test that all other args can accept MultInv
		IdxCurrentSelfEvalRule = MultInvOutStdMultArg_SER;
		InferenceParameter1 = 0;	// StdAdd arg, targeted parameter is InferenceParameter1 of this
		return true;
		}
	return false;
}

bool EqualRelation::UseStdMultiplicationNOTALLDISTINCT() const
{	//! \todo IMPLEMENT
	// NOTE: if n-1 constants, constant rule goes off first
	// 2-ary does *not* dualize to here
	return false;
}


bool EqualRelation::UseStdMultiplicationNOTALLEQUAL() const
{	//! \todo IMPLEMENT
	// 2-ary does *not* dualize to here
	// not an equiv relation, so cannot dualize other clause
	return false;
}

// TODO: consider condensing ALLEQUAL, NOTALLEQUAL code
bool EqualRelation::UseDomainsALLEQUAL() const
{	// arg0 constant, argn domain doesn't contain arg0: FALSE
	// NOTE: this has been moved to IsClearlyDistinctFrom

	//! \todo OPTIMIZE: TIME (this is the low-RAM implementation)
	size_t i = fast_size();
	while(ArgArray[--i]->IsUltimateType(&TruthValues))
		if (0==i)
			{	// all args have UltimateType TruthValues: convert ALLEQUAL to IFF
			InferenceParameter1=LogicalIFF_MC;
			IdxCurrentEvalRule=RetypeToMetaConnective_ER;
			return true;
			};

	// two args have domain intersection NULLSet: FALSE
	// NOTE: this has been moved to IsClearlyDistinctFrom
	return false;
}

bool EqualRelation::UseDomainsALLDISTINCT() const
{	//! \todo IMPLEMENT
	// 2-ary: two args have domain intersection NULLSet: TRUE
	// NOTE: 2-ary case is now handled by IsClearlyDistinctFrom in conjunction with ::UseConstants....

	//! \todo n-ary: n-1 args have domain intersection NULLSet: convert to DISTINCTFROMALLOF
	//! \todo n-ary: argn has domain intersection NULLSet with all others: delete argn
	//! \todo 3+ args domain TruthValue: FALSE
	return false;
}

bool EqualRelation::UseDomainsEQUALTOONEOForDISTINCTFROMALLOF() const
{	//! \todo IMPLEMENT
	// NOTE: all of this logic has been moved to IsClearlyDistinctFrom

	return false;
}

bool EqualRelation::UseDomainsNOTALLDISTINCT() const
{	//! \todo IMPLEMENT
	//! \todo n-ary: n-1 args have domain intersection NULLSet : convert to EQUALTOONEOF
	//! \todo n-ary: argn has domain intersection NULLSet with all others: delete argn
	//! \todo 3+ args domain TruthValue: TRUE
	return false;
}

bool EqualRelation::UseDomainsNOTALLEQUAL() const
{	//! \todo IMPLEMENT
	// arg0 constant, argn domain doesn't contain arg0: TRUE
	// NOTE: this has been moved to IsClearlyDistinctFrom

	//! \todo OPTIMIZE: TIME (this is the low-RAM implementation)
	size_t i = fast_size();
	while(ArgArray[--i]->IsUltimateType(&TruthValues))
		if (0==i)
			{	// all args have UltimateType TruthValues: convert NOTALLEQUAL to NIFF
			InferenceParameter1=LogicalNIFF_MC;
			IdxCurrentEvalRule=RetypeToMetaConnective_ER;
			return true;
			};

	// two args have domain intersection NULLSet: FALSE
	// NOTE: this has been moved to IsClearlyDistinctFrom
	return false;
}

// #define FRANCI_WARY

// Assistants to QStatement
// mimic this code as necessary in evaluation rules
// ALLEQUAL template affects: ALLEQUAL, ALLDISTINCT(2-ary), NOTALLEQUAL
// * ALLEQUAL, NOTALLEQUAL could use a specialist routine (screen out ultimate types)?
// * ALLDISTINCT needs a disjoint-graph-checker to enable n-ary
// EQUALTOONEOF template affects: EQUALTOONEOF, DISTINCTFROMALLOF [fix it first]
// if the intersection result type is NULL, then it short-circuits.
//! \todo IMPLEMENT: An ExplicitConstant's type is a singleton set (we may simply have to explicitly code this)
void EqualRelation::ImproviseDomainsALLEQUAL(bool& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/19/2001
	// #1: take intersection of all domains
	// #2: if nonNULL, set all domains to this one
	// #3: if constant is arg0, set all domains to constant domain
	if (ArgArray[0]->IsExplicitConstant())
		{	// ArgArray[0] constant: all types must be compatible
#ifdef FRANCI_WARY
		LOG("Entering ImproviseDomainsALLEQUAL: explicit constant branch");
#endif
		size_t i = fast_size();
		//! \todo FIX: the HasAsElement calls should be TruthValue-based (it is possible not to 
		//! know whether an explicitly constant element is in the domain of a proper class).
		do	if (ArgArray[--i]->IsUltimateType(NULL))
				{
				if (MinClausePhraseIdx_MC<=ArgArray[i]->ExactType() && MaxClausePhraseIdx_MC>=ArgArray[i]->ExactType())
					return;	
				if (   !ArgArray[i]->ForceUltimateType(ArgArray[0]->UltimateType())
					|| !ArgArray[i]->UltimateType()->HasAsElement(*ArgArray[0]))
					{
					InvokeEvalForceFalse();
#ifdef FRANCI_WARY
					LOG("Leaving ImproviseDomainsALLEQUAL: eval FALSE, initial type NULL");
					LOG(ArgArray[i]->name());
#endif
					return;
					};
				Target = true;
				}
			else if (   !ArgArray[i]->ForceUltimateType(ArgArray[0]->UltimateType())
					 || !ArgArray[i]->UltimateType()->HasAsElement(*ArgArray[0]))
				{
				InvokeEvalForceFalse();
#ifdef FRANCI_WARY
				LOG("Leaving ImproviseDomainsALLEQUAL: eval FALSE, initial type non-NULL");
				LOG(ArgArray[i]->name());
#endif
				return;
				}
		while(1<i);
#ifdef FRANCI_WARY
		LOG("Leaving ImproviseDomainsALLEQUAL: no rule");
#endif
		return;
		};
	// Franci should take the intersection of all types, then force all args
	// to this intersection.
#ifdef FRANCI_WARY
	LOG("Entering ImproviseDomainsALLEQUAL: non-explicit constant branch");
#endif
	size_t i = fast_size();
	AbstractClass* TargetType = NULL;
	try	{
		do	if (ArgArray[--i]->UltimateType())
				{
				TargetType = new AbstractClass(*ArgArray[i]->UltimateType());
				while(0<i)
					if (   ArgArray[--i]->UltimateType()
						&& !TargetType->Subclass(*ArgArray[i]->UltimateType()))
						{
						TargetType->IntersectWith(*ArgArray[i]->UltimateType());
						if (NULLSet==*TargetType)
							{
							delete TargetType;
							InvokeEvalForceFalse();
							return;
							};
						}
				}
		while(0<i);
#ifdef FRANCI_WARY
		LOG("Scan loop OK");
#endif
		if (TargetType)
			{
#ifdef FRANCI_WARY
			LOG("Entering ForceType loop");
#endif
			i = fast_size();
			do	if (!ArgArray[--i]->ForceUltimateType(TargetType))
					{
					if (ArgArray[i]->IsUltimateType(NULL) && MinClausePhraseIdx_MC<=ArgArray[i]->ExactType() && MaxClausePhraseIdx_MC>=ArgArray[i]->ExactType())
						return;
					delete TargetType;
					InvokeEvalForceFalse();
					return;
					}
			while(0<i);
#ifdef FRANCI_WARY
			LOG("ForceType loop OK");
#endif
			delete TargetType;
			};
		}
	catch(const bad_alloc&)
		{
		DELETE(TargetType);
		UnconditionalRAMFailure();
		};
}

#undef FRANCI_WARY

bool EqualRelation::ImproviseDomainsEQUALTOONEOF(bool& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/16/2002
	//! return true iff statement is redundant and should be rewritten to TRUE
	// NOTE: if even one of the RHS domains is NULL, there isn't much to be done....
	// take type of LHS, force all RHS to be compatible
	bool ArgZeroProperSuperclassOfAll = true;
	size_t i = fast_size();
	const AbstractClass* UltTypeArgZero = ArgArray[0]->UltimateType();
	if (UltTypeArgZero)
		{
		do	if (!ArgArray[--i]->ForceUltimateType(UltTypeArgZero))
				return false;
		while(1<i);	// if this succeeds, all RHS UltimateTypes are nonNULL
		i = fast_size();
		do	if (!UltTypeArgZero->ProperSuperclass(*ArgArray[--i]->UltimateType()))
				ArgZeroProperSuperclassOfAll = false;
		while(1<i);
		}
	else{
		do	if (ArgArray[--i]->IsUltimateType(NULL)) return false;
		while(1<i);
		};

	// #1: take union of all domains on RHS
	// #2: set LHS domain to this one
	if (ArgZeroProperSuperclassOfAll)
		{
		try	{
			AbstractClass* Tmp = new AbstractClass(*ArgArray[1]->UltimateType());
			if (Tmp->SyntaxOK() && 3<=fast_size())
				{
				i = fast_size();
				do	if (!Tmp->UnionWith(*ArgArray[--i]->UltimateType()))
						{
						DELETE(Tmp);
						return false;
						}
				while(2<i);
				};
			ArgArray[0]->ForceUltimateType(Tmp);
			delete Tmp;
			}
		catch(const bad_alloc&)
			{
			return false;
			};
		}

	// X EQUALTOONEOF -&infin;...&infin; not only forces domain _Z_, it zaps
	// this statement!
#if 1	// specialized one-shot code
	if (    ArgArray[0]->IsExactType(Variable_MC) && 2==fast_size()
		&& ArgArray[0]->IsUltimateType(&Integer)
		&& ArgArray[1]->IsTypeMatch(LinearInterval_MC,&Integer)
		&& ArgArray[1]->ArgN(0)->IsExactType(LinearInfinity_MC)
		&& ArgArray[1]->ArgN(1)->IsExactType(LinearInfinity_MC)
		&& *ArgArray[1]->ArgN(0)!=*ArgArray[1]->ArgN(1))
		return true;	
#else	// generic template; retain against future alterations
	if (    ArgArray[0]->IsExactType(Variable_MC)
		&& !ArgArray[0]->IsUltimateType(NULL))
		{
		if (   2==fast_size() && ArgArray[0]->IsUltimateType(&Integer)
			&& ArgArray[1]->IsTypeMatch(LinearInterval_MC,&Integer)
			&& ArgArray[1]->ArgN(0)->IsExactType(LinearInfinity_MC)
			&& ArgArray[1]->ArgN(1)->IsExactType(LinearInfinity_MC)
			&& *ArgArray[1]->ArgN(0)!=*ArgArray[1]->ArgN(1))
			return true;	
		}
#endif
	return false;
}

bool EqualRelation::DelegateEvaluate(MetaConcept*& dest)
{
	assert(MetaConceptWithArgArray::MaxEvalRuleIdx_ER<IdxCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxEvalRuleIdx_ER>=IdxCurrentEvalRule);
	return (this->*EvaluateRuleLookup[IdxCurrentEvalRule-(MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1)])(dest);
}

bool EqualRelation::DelegateSelfEvaluate()
{
	assert(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER<IdxCurrentSelfEvalRule);
	assert(MaxSelfEvalRuleIdx_SER+MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER>=IdxCurrentSelfEvalRule);
	return (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1)])();
}

bool EqualRelation::DISTINCTFROMALLOFExtractALLDISTINCT()
{	// FORMALLY CORRECT: Kenneth Boyd, 6/19/2000
	// InferenceParameter1 points to DISTINCTFROMALLOF from which to extract
	// InferenceParameter2 points to the other DISTINCTFROMALLOF
	// ArgArray[InferenceParameter1].InferenceParameter_ point to two args on right of 
	// DISTINCTFROMALLOF to use; third is arg0 (extraction DISTINCTFROMALLOF)
	// 3-ary is a compatible retype (which could be moved to StrictlyModifies, but probably
	// is not useful to).
	// others require construction of 3-ary ALLDISTINCT (which *will* strictly-modify the
	// original two triggers)
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(InferenceParameter1!=InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(DISTINCTFROMALLOF_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(DISTINCTFROMALLOF_MC));
	EqualRelation& VR_Target = *static_cast<EqualRelation*>(ArgArray[InferenceParameter1]);
	if (3==VR_Target.fast_size())
		{	// 3-ary compatible retype
		VR_Target.InferenceParameter1 = ALLDISTINCT_MC;
		VR_Target.CompatibleRetype();
		if (VR_Target.CanStrictlyModify(*ArgArray[InferenceParameter2]))
			VR_Target.StrictlyModifies(ArgArray[InferenceParameter2]);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	// n-ary extract
	EqualRelation* NewArg = NULL;
	try	{
		NewArg = new EqualRelation(ALLDISTINCT_EM);
		NewArg->insertNSlotsAt(3,0);
		VR_Target.ArgArray[0]->CopyInto(NewArg->ArgArray[0]);
		}
	catch(const bad_alloc&)
		{
		delete NewArg;
		return false;
		};

	if (!InsertSlotAt(fast_size(),NewArg))
		{
		delete NewArg;
		return false;
		}
	// past here, code is RAM-safe
	VR_Target.FastTransferOutAndNULL(VR_Target.InferenceParameter1,NewArg->ArgArray[1]);
	VR_Target.FastTransferOutAndNULL(VR_Target.fast_size()-1,NewArg->ArgArray[2]);
	VR_Target.FastDeleteIdx(VR_Target.fast_size()-1);
	VR_Target.FastDeleteIdx(VR_Target.InferenceParameter1);
	if (2==VR_Target.fast_size()) VR_Target.SetExactTypeV2(ALLDISTINCT_MC);

	NewArg->CanStrictlyModify(*ArgArray[InferenceParameter2]);
	NewArg->StrictlyModifies(ArgArray[InferenceParameter2]);

	_forceStdForm();
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool EqualRelation::Ary2EqRewriteZeroEq2ArySum()
{	// FORMALLY CORRECT: 10/17/2004
	// Arg1 is a stable 2-ary sum (to be disposed of)
	assert(ArgArray[0]->IsZero());
	assert(ArgArray[1]->IsExactType(StdAddition_MC));
	MetaConcept** tmp = NULL;
	static_cast<MetaConceptWithArgArray*>(ArgArray[1])->OverwriteAndNULL(tmp);
	ArgArray = tmp;
	if (    ArgArray[0]->IsMetaAddInverted()
		|| (!ArgArray[1]->IsMetaAddInverted() && ArgArray[0]->IsExplicitConstant()))
		SUCCEED_OR_DIE(ArgArray[0]->SelfInverse(StdAddition_MC));
	else
		SUCCEED_OR_DIE(ArgArray[1]->SelfInverse(StdAddition_MC));
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool EqualRelation::MergeIntervals()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/16/2002
	assert(ArgArray[InferenceParameter1]->IsExactType(LinearInterval_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(LinearInterval_MC));
	//! \todo OPTIMIZE: using this rule indirectly invokes the metaknowledge 3 times: once
	// to get here, twice to figure out which variant to use.  Only one invocation is 
	// necessary.
	if (static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->DestructiveMergeWith(*static_cast<LinearInterval*>(ArgArray[InferenceParameter2])))
		{
		DeleteIdx(InferenceParameter2);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		};
	if (static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->NonDestructiveMergeWith(*static_cast<LinearInterval*>(ArgArray[InferenceParameter2])))
		{
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	return false;
}

bool EqualRelation::DismantleLinearIntervalToEndpoints()
{	// FORMALLY CORRECT: 10/17/2004
	assert(ArgArray[InferenceParameter1]->IsExactType(LinearInterval_MC));
	assert(ArgArray[InferenceParameter1]->IsUltimateType(&Integer));
	// InferenceParameter1 has index to LinearInterval with ultimate domain _Z_
	// want to replace it with its arguments
	if (InsertSlotAt(InferenceParameter1+1,NULL))
		{	// NOTE: two referenced arg transfer functions are probably MetaConcept2Ary
		MetaConcept* Tmp = NULL;
		static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->TransferOutRHSAndNULL(ArgArray[InferenceParameter1+1]);
		static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->TransferOutLHSAndNULL(Tmp);
		DELETE(ArgArray[InferenceParameter1]);
		ArgArray[InferenceParameter1]=Tmp;
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	return false;
}

bool EqualRelation::ExtendLinearInterval()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/6/2002
	assert(ArgArray[InferenceParameter1]->IsExactType(LinearInterval_MC));
	if (static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->DestructiveExtendBy(ArgArray[InferenceParameter2]))
		{
		DeleteIdx(InferenceParameter2);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		};
	return false;
}

