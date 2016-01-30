// MetaCon2.cxx
// implementation of MetaConceptWithArgArray

#include "MetaCon2.hxx"
#include "Class.hxx"
#include "Quantify.hxx"
#include "TruthVal.hxx"
#include "LowRel.hxx"

#include "Zaimoni.STL/boost/functional.hpp"
#include "Zaimoni.STL/Pure.C/logging.h"

MetaConceptWithArgArray::EvaluateToOtherRule MetaConceptWithArgArray::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
  =	{
	&MetaConceptWithArgArray::EvalForceArg,
	&MetaConceptWithArgArray::EvalForceNotArg,
	&MetaConceptWithArgArray::EvalForceTrue,
	&MetaConceptWithArgArray::EvalForceFalse,
	&MetaConceptWithArgArray::EvalForceContradiction,
	&MetaConceptWithArgArray::EvalForceUnknown,
	&MetaConceptWithArgArray::ForceValue,
	&MetaConceptWithArgArray::RetypeToMetaConnective
	};

MetaConceptWithArgArray::SelfEvaluateRule MetaConceptWithArgArray::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&MetaConceptWithArgArray::SelfEvalRuleEvaluateArgs,
	&MetaConceptWithArgArray::SelfEvalRuleEvaluateArg,
	&MetaConceptWithArgArray::SelfEvalRuleEvaluateArgOneStage,
	&MetaConceptWithArgArray::SelfEvalStrictlyModify,
	&MetaConceptWithArgArray::VirtualStrictlyModify,
	&MetaConceptWithArgArray::SelfEvalRuleForceArgSameImplementation,
	&MetaConceptWithArgArray::SelfEvalAddArgAtEndAndForceCorrectForm,
	&MetaConceptWithArgArray::SelfEvalRuleCleanArg,
	&MetaConceptWithArgArray::SelfEvalRuleCleanTrailingArg,
	&MetaConceptWithArgArray::SelfEvalRuleCleanLeadingArg,
	&MetaConceptWithArgArray::SelfEvalRuleNIFFXORCleanArg,
	&MetaConceptWithArgArray::SelfEvalRuleNIFFXORCleanTrailingArg,
	&MetaConceptWithArgArray::SelfEvalRuleAry2CorrectedCleanArg,
	&MetaConceptWithArgArray::SelfEvalRuleAry2CorrectedCleanTrailingArg,
	&MetaConceptWithArgArray::SelfEvalRuleUnrollGeneralizedAssociativity,
	&MetaConceptWithArgArray::CompatibleRetypeOtherArgs,
	&MetaConceptWithArgArray::CompatibleRetype,
	&MetaConceptWithArgArray::LogicalANDSpliceNAryEqualArg,
	&MetaConceptWithArgArray::LogicalANDSpliceALLEQUALAddInvArg,	
	&MetaConceptWithArgArray::ReplaceThisArgWithLeadArg,
	&MetaConceptWithArgArray::LogicalORXORCompactANDArgHyperCube,
	&MetaConceptWithArgArray::LogicalANDORXORCondenseORANDANDArgHyperCube,
	&MetaConceptWithArgArray::TranslateInterval
	};

MetaConceptWithArgArray::MetaConceptWithArgArray(const MetaConceptWithArgArray& src)
:	MetaConcept(src),
	ArgArray(src.ArgArray),
	InferenceParameter1(src.InferenceParameter1),
	InferenceParameter2(src.InferenceParameter2),
	InferenceParameterMC(src.InferenceParameterMC),
	IdxCurrentEvalRule(src.IdxCurrentEvalRule),
	IdxCurrentSelfEvalRule(src.IdxCurrentSelfEvalRule)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/29/2006
}

void MetaConceptWithArgArray::MoveIntoAux(MetaConceptWithArgArray& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/29/2006
	dest.MetaConcept::operator=(*this);
	ArgArray.MoveInto(dest.ArgArray);
	dest.InferenceParameter1=InferenceParameter1;
	dest.InferenceParameter2=InferenceParameter2;
	InferenceParameterMC.MoveInto(dest.InferenceParameterMC);
	dest.IdxCurrentEvalRule=IdxCurrentEvalRule;
	dest.IdxCurrentSelfEvalRule=IdxCurrentSelfEvalRule;
}

void MetaConceptWithArgArray::operator=(const MetaConceptWithArgArray& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/29/2006
	autovalarray_ptr_throws<MetaConcept*> tmp(src.ArgArray);
	InferenceParameterMC = src.InferenceParameterMC;
	
	// OK now.
	tmp.MoveInto(ArgArray);
	MetaConcept::operator=(src);
	InferenceParameter1 = src.InferenceParameter1;
	InferenceParameter2 = src.InferenceParameter2;
	IdxCurrentEvalRule = src.IdxCurrentEvalRule;
	IdxCurrentSelfEvalRule = src.IdxCurrentSelfEvalRule;
}

bool MetaConceptWithArgArray::CanEvaluate() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2000
	if 		(None_SER>IdxCurrentSelfEvalRule) {assert(NULL==InferenceParameterMC); return false;}
	else if (IdxCurrentSelfEvalRule || IdxCurrentEvalRule) return true;
	assert(NULL==InferenceParameterMC);
	DEBUG_LOG(__PRETTY_FUNCTION__);
	DEBUG_LOG(name());
	DEBUG_STATEMENT(const long revert_to = get_checkpoint());
	DiagnoseInferenceRules();
	if 		(None_SER>IdxCurrentSelfEvalRule)
		{
		DEBUG_STATEMENT(revert_checkpoint(revert_to));
		DEBUG_LOG("return-false: bool MetaConceptWithArgArray::CanEvaluate() const");
		return false;
		}
	else if (IdxCurrentSelfEvalRule)
		{
		DEBUG_STATEMENT(revert_checkpoint(revert_to));
		DEBUG_LOG("self-eval: bool MetaConceptWithArgArray::CanEvaluate() const");
		return true;
		};
	if (EvalForceArg_ER==IdxCurrentEvalRule && HasSameImplementationAs(*ArgArray[InferenceParameter1]))
		{
		IdxCurrentEvalRule = None_ER;
		IdxCurrentSelfEvalRule = SelfEvalRuleForceArgSameImplementation_SER;
		DEBUG_STATEMENT(revert_checkpoint(revert_to));
		DEBUG_LOG("rewrite self-eval: bool MetaConceptWithArgArray::CanEvaluate() const");
		return true;
		};
	DEBUG_STATEMENT(revert_checkpoint(revert_to));
	DEBUG_LOG("eval: bool MetaConceptWithArgArray::CanEvaluate() const");
	return IdxCurrentEvalRule;
}

bool MetaConceptWithArgArray::CanEvaluateToSameType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2000
	if      (None_SER>IdxCurrentSelfEvalRule) {assert(NULL==InferenceParameterMC); return false;}
	else if (IdxCurrentSelfEvalRule) return true;
	else if (IdxCurrentEvalRule) return false;
	DEBUG_LOG(__PRETTY_FUNCTION__);
	DEBUG_LOG(name());
	assert(InferenceParameterMC.empty());
	DEBUG_LOG("Attempting DiagnoseInferenceRules/CanEvaluateToSameType");
	DiagnoseInferenceRules();
	DEBUG_LOG("DiagnoseInferenceRules OK");
	if 		(None_SER>IdxCurrentSelfEvalRule) return false;
	else if (IdxCurrentSelfEvalRule) return true;
	if (EvalForceArg_ER==IdxCurrentEvalRule && HasSameImplementationAs(*ArgArray[InferenceParameter1]))
		{
		IdxCurrentEvalRule = None_ER;
		IdxCurrentSelfEvalRule = SelfEvalRuleForceArgSameImplementation_SER;
		return true;
		};
	return false;
}

bool MetaConceptWithArgArray::Evaluate(MetaConcept*& dest)		// same, or different type
{	// FORMALLY CORRECT: 4/22/2000
	// NOTE: there is exactly one call to Evaluate in Franci.  This call is preceded by
	// a call to CanEvaluate()...so the rules have been initialized, and are guaranteed
	// to be appropriate.  The SelfEvaluate rules have been already done, so this is not
	// required either.
	// There is no need to diagnose the inference rules.
	DEBUG_LOG("MetaConceptWithArgArray::Evaluate");
	DEBUG_LOG(*this);
	try {
		bool RetVal = (MaxEvalRuleIdx_ER>=IdxCurrentEvalRule) ? (this->*EvaluateRuleLookup[IdxCurrentEvalRule-1])(dest)
					: DelegateEvaluate(dest);
		DEBUG_LOG(*dest);
		return RetVal;
		}
	catch(const std::bad_alloc&)
		{
		DEBUG_LOG("MetaConceptWithArgArray::Evaluate failed: std::bad_alloc()");
		return false;
		}
}

bool MetaConceptWithArgArray::DestructiveEvaluateToSameType()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/16/1999
	DEBUG_LOG(__PRETTY_FUNCTION__);
	DEBUG_LOG(name());
	DEBUG_LOG(*this);

	if (!IdxCurrentSelfEvalRule && !IdxCurrentEvalRule)
		DiagnoseInferenceRules();

	DEBUG_LOG("DiagnoseInferenceRules OK");
	DEBUG_LOG(InferenceParameter1);
	DEBUG_LOG(InferenceParameter2);

	if (None_SER<IdxCurrentSelfEvalRule)
		{	// it evaluates to something of the same type
		try	{
			const bool Tmp = (MaxSelfEvalRuleIdx_SER>=IdxCurrentSelfEvalRule) ? (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-1])()
							: DelegateSelfEvaluate();
			DEBUG_LOG("selfeval worked");

			if (!SyntaxOK())
				FATAL((INFORM(*this),AlphaBadSyntaxGenerated));

			DEBUG_LOG(*this);
			return Tmp;
			}
		catch(const std::bad_alloc&)
			{
			DEBUG_LOG("MetaConceptWithArgArray::DestructiveEvaluateToSameType failed: std::bad_alloc()");
			return false;	
			}
		}
	return false;
}

// FORMALLY CORRECT: 4/20/1998
#define ARGN_BODY return (fast_size()>n) ? ArgArray[n] : NULL;

STANDARD_DECLARE_ARGN(MetaConceptWithArgArray,ARGN_BODY)

#undef ARGN_BODY

bool MetaConceptWithArgArray::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/29/2007
	if (fast_size()==static_cast<const MetaConceptWithArgArray&>(rhs).fast_size())
		return and_range_n(addref_binary_functor<std::equal_to<MetaConcept>,1,1>(),ArgArray.begin(),fast_size(),static_cast<const MetaConceptWithArgArray&>(rhs).ArgArray.begin());
	return false;
}

bool MetaConceptWithArgArray::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	assert(!ArgArray.empty());
	const MetaConceptWithArgArray& VR_rhs = static_cast<const MetaConceptWithArgArray&>(rhs);
	assert(!VR_rhs.ArgArray.empty());
	switch(cmp(fast_size(),VR_rhs.fast_size()))
	{
	case -1:return true;
	case 0:	{
			const MetaConcept* const * const rhs_array = VR_rhs.ArgArray.data();
			size_t i = fast_size();
			do	{
				--i;
				if (ArgArray[i]->InternalDataLT(*rhs_array[i])) return true;
				if (rhs_array[i]->InternalDataLT(*ArgArray[i])) return false;
				}
			while(0<i);
			}
	}
	return false;
}

bool MetaConceptWithArgArray::IsAbstractClassDomain() const
{
	if (!ArgArray.empty())
		{
		const MetaConcept* const * const lhs_array = ArgArray.data();
		size_t i = fast_size();
		do	if (!lhs_array[--i] || !lhs_array[i]->IsAbstractClassDomain())
				return false;
		while(0<i);
		}
	return true;
}

void MetaConceptWithArgArray::ConvertVariableToCurrentQuantification(MetaQuantifier& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	if (!ArgArray.empty())
		{
		MetaConcept* const * const lhs_array = ArgArray.data();
		size_t i = fast_size();
		do	if (lhs_array[--i])
				lhs_array[i]->ConvertVariableToCurrentQuantification(src);
		while(0<i);
		}
}

bool MetaConceptWithArgArray::UsesQuantifierAux(const MetaQuantifier& x) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/29/2007
	return or_range(boost::bind1st(boost::mem_fun_ref(&MetaQuantifier::MetaConceptPtrUsesThisQuantifier),x),ArgArray.begin(),ArgArray.end());
}

bool MetaConceptWithArgArray::EvalForceArg(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/17/2000
	// only called from Evaluate: destroys own integrity.
	assert(!dest);
	dest = ArgArray[InferenceParameter1];
	ArgArray[InferenceParameter1]=NULL;
	return true;
}

bool MetaConceptWithArgArray::EvalForceNotArg(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/17/2000
	// only called from Evaluate: destroys own integrity.
	assert(!dest);
	dest = ArgArray[InferenceParameter1];
	dest->SelfLogicalNOT();
	ArgArray[InferenceParameter1]=NULL;
	return true;
}

bool MetaConceptWithArgArray::EvalForceTrue(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	assert(!dest);
	dest = new TruthValue(TruthValue::True);
	return true;
}

bool MetaConceptWithArgArray::EvalForceFalse(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	assert(!dest);
	dest = new TruthValue(TruthValue::False);
	return true;
}

bool MetaConceptWithArgArray::EvalForceContradiction(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	assert(!dest);
	dest = new TruthValue(TruthValue::Contradiction);
	return true;
}

bool MetaConceptWithArgArray::EvalForceUnknown(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	assert(!dest);
	dest = new TruthValue(TruthValue::Unknown);
	return true;
}

bool MetaConceptWithArgArray::ReplaceNon1stArgWith1stArgInThisConcept(MetaConcept*& dest) const
{	// FORMALLY CORRECT: 1/6/2003
	assert(2<=size());
	const MetaConcept* const * const lhs_array = ArgArray.data();
	size_t i = fast_size();
	while(0<--i)
		if (!StrongVarSubstitute(dest,*lhs_array[i],*lhs_array[0]))
			return false;
	return true;
}

void MetaConceptWithArgArray::DeleteNSlotsAt(size_t n, size_t i)
{	// FORMALLY CORRECT: 11/15/1999
	assert(0<n);
	assert(size()>i);
	assert(size()-i>=n);
	ArgArray.DeleteNSlotsAt(n,i);
}

void MetaConceptWithArgArray::insertNSlotsAt(size_t n, size_t i)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/22/2005
	assert(0<n);
	assert(size()>=i);
	ArgArray.insertNSlotsAt(n,i);
}

bool MetaConceptWithArgArray::InsertNSlotsAtV2(size_t n, size_t i)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/22/2005
	assert(0<n);
	assert(size()>=i);
	return ArgArray.InsertNSlotsAt(n,i);
}

void MetaConceptWithArgArray::TransferOutAndNULL(size_t i, MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	assert(size()>i);
	dest = ArgArray[i];
	ArgArray[i] = NULL;
}

void MetaConceptWithArgArray::CleanTransferOutAndNULL(size_t i, MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	assert(size()>i);
	delete dest;
	dest = ArgArray[i];
	ArgArray[i] = NULL;
}

// NOTE: semantics here are probably required by LexParse [which uses a pointer directly
// created by new for the 2nd argument]
void MetaConceptWithArgArray::TransferInAndOverwriteRaw(size_t i, MetaConcept* src)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	assert(size()>i);
	delete ArgArray[i];
	ArgArray[i]=src;
}

// this one copies all args, except the one pointed to by InferenceParameter1.
void MetaConceptWithArgArray::CopyAllArgsButOneIntoArgList(MetaConcept** const dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/20/1999
	assert(dest);
	assert(2<=size());
	size_t i = fast_size();
	do	switch(cmp(InferenceParameter1,--i))
		{
		case -1:{
				ArgArray[i]->CopyInto(dest[i-1]);
				break;
				}
		case 1:{
				ArgArray[i]->CopyInto(dest[i]);
				break;
				}
		}
	while(0<i);
}

bool MetaConceptWithArgArray::AddArgAtEndAndForceCorrectForm(MetaConcept*& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/2005
	assert(src);
	if (!InsertSlotAt(fast_size(),src)) return false;

	src = NULL;
	IdxCurrentEvalRule = None_ER;
	IdxCurrentSelfEvalRule = None_SER;
	_forceStdForm();
	return true;
}

bool MetaConceptWithArgArray::AddArgAtStartAndForceCorrectForm(MetaConcept*& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/2005
	assert(src);
	if (!InsertSlotAt(0,src)) return false;

	src = NULL;
	IdxCurrentEvalRule = None_ER;
	IdxCurrentSelfEvalRule = None_SER;
	_forceStdForm();
	return true;
}

void MetaConceptWithArgArray::ForceStdFormAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/28/2007
	assert(!ArgArray.empty());
	MetaConcept* const * const lhs_array = ArgArray.data();
	size_t i = fast_size();
	do	lhs_array[--i]->ForceStdForm();
	while(0<i);
}

bool ValidateArgArray(const MetaConcept* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	if (ArgArray)
		{
		const size_t LocalArraySize = ArraySize(const_cast<MetaConcept**>(ArgArray));
		size_t i = LocalArraySize;
		do	SUCCEED_OR_DIE(ArgArray[--i]);
		while(0<i);

		i =  LocalArraySize;
		do	if (!ArgArray[--i]->SyntaxOK()) return false;
		while(0<i);
		return true;
		};
	return false;
}

bool MetaConceptWithArgArray::OrderIndependentPairwiseRelation(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/25/1999
	// We cannot assume entry distinctness
	if (   !ArgArray.empty() && !rhs.ArgArray.empty()
		&& fast_size() == rhs.fast_size())
		{
		size_t LowLHSNonTrivialIdx = 0;
		size_t LowRHSNonTrivialIdx = 0;
		size_t HighLHSNonTrivialIdx = fast_size()-1;
		size_t HighRHSNonTrivialIdx = HighLHSNonTrivialIdx;
		// first, nibble away at the ends.  [This reduces the range that needs to be repeatedly swept.]
		// Then, execute the safe algorithm.
		do	{
			if      (TargetRelation(*ArgArray[LowLHSNonTrivialIdx],*rhs.ArgArray[LowRHSNonTrivialIdx]))
				{
				LowLHSNonTrivialIdx++;
				LowRHSNonTrivialIdx++;
				}
			else if (TargetRelation(*ArgArray[LowLHSNonTrivialIdx],*rhs.ArgArray[HighRHSNonTrivialIdx]))
				{
				LowLHSNonTrivialIdx++;
				HighRHSNonTrivialIdx--;
				}
			else if (TargetRelation(*ArgArray[HighLHSNonTrivialIdx],*rhs.ArgArray[LowRHSNonTrivialIdx]))
				{
				HighLHSNonTrivialIdx--;
				LowRHSNonTrivialIdx++;
				}
			else if (TargetRelation(*ArgArray[HighLHSNonTrivialIdx],*rhs.ArgArray[HighRHSNonTrivialIdx]))
				{
				HighLHSNonTrivialIdx--;
				HighRHSNonTrivialIdx--;
				}
			else{
				const size_t VR_fast_size = HighRHSNonTrivialIdx-LowRHSNonTrivialIdx+1;
				// We have, for free: ArgArray[LHSNontrivial__]!=ArgArray[RHSNontrivial__] [4 tests Franci has already done]
				// Need at least two slots free to have a chance
				if (4>VR_fast_size) return false;
				MetaConcept* const * const VR_LHSArgArray = ArgArray.data()+LowLHSNonTrivialIdx;
				MetaConcept* const * const VR_RHSArgArray = rhs.ArgArray.data()+LowRHSNonTrivialIdx;
				size_t i = 0;
				// RAM-safe algorithm.
				// 1) count how many instances of ArgArray[i] we already have.
				//		a) If there are any behind us, we have already tested it: continue
				// 2) see if this count equals what we have on the RHS.
				//		a) If not, return false
				// 3) repeat until all ArgArray[Idx] tested.
				// 4) if we're still here, return true.
				do	{
				// 1) count how many instances of ArgArray[Idx] we already have.
				//		a) If there are any behind us, we have already tested it: continue
					{
					size_t j = i;
					while(0<j)
						if (TargetRelation(*VR_LHSArgArray[i],*VR_LHSArgArray[--j]))
							goto StartNextIteration;
					}
					{
					size_t Count = 1;
					size_t Count2 = 0;
					size_t j = i+1;
					while(j<VR_fast_size)
						if (TargetRelation(*VR_LHSArgArray[i],*VR_LHSArgArray[j++]))
							Count++;
					// 2) see if this count equals what we have on the RHS.
					//		a) If not, return false
					// TODO: OPTIMIZE: TIME
					j = 0;
					while(j<VR_fast_size)
						if (   TargetRelation(*VR_LHSArgArray[i],*VR_RHSArgArray[j++])
						    && ++Count2>Count)
							return false;
					if (Count2<Count) return false;
					};
StartNextIteration:;
					}
				while(++i<VR_fast_size);
				return true;				
				}
			}
		while(LowLHSNonTrivialIdx<=HighLHSNonTrivialIdx);
		return true;
		}
	return false;
}

// obsolete, eliminate
static bool PairwiseUnrelatedAux(MetaConcept* const * const ArgArray, const size_t strict_ub, LowLevelBinaryRelation& TargetRelation)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/5/1999
	assert(ArgArray);
	assert(0<strict_ub);
	size_t i = 0;
	do	{
		size_t j = i+1;
		while(strict_ub>j)
			if (TargetRelation(*ArgArray[i],*ArgArray[j++]))
				return false;
		}
	while(strict_ub> ++i);
	return true;
}

bool MetaConceptWithArgArray::ArgsPairwiseUnrelated(LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/5/1999
	return PairwiseUnrelatedAux(ArgArray,fast_size(),TargetRelation);
}

bool MetaConceptWithArgArray::ArgRangePairwiseUnrelated(LowLevelBinaryRelation& TargetRelation, size_t lb, size_t nonstrict_ub) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/5/1999
	assert(lb<=nonstrict_ub);
	assert(nonstrict_ub<fast_size());
	return PairwiseUnrelatedAux(ArgArray+lb,nonstrict_ub-lb+1,TargetRelation);
}

bool MetaConceptWithArgArray::ExactOrderPairwiseRelation(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	if (!ArgArray.empty() && !rhs.ArgArray.empty())
		{
		size_t i = fast_size();
		if (i==rhs.fast_size())
			{
			const MetaConcept* const * lhs_args = ArgArray.data();
			const MetaConcept* const * rhs_args = rhs.ArgArray.data();
			do	{
				--i;
				if (!TargetRelation(*lhs_args[i],*rhs_args[i])) return false;
				}
			while(0<i);
			return true;
			}
		}
	return false;
}

bool MetaConceptWithArgArray::ForceValue(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/17/2006
	// only called from Evaluate: destroys own integrity.
	assert(!dest);
	assert(!InferenceParameterMC.empty());
	dest = InferenceParameterMC.release();
	return true;
}


// \bug MetaConceptWithArgArray::SelfEvalRuleEvaluateArg doesn't check for annihilators or identity
bool MetaConceptWithArgArray::SelfEvalRuleEvaluateArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 1/24/2002
	assert(size()>InferenceParameter1);
	DEBUG_LOG(__PRETTY_FUNCTION__);
	DEBUG_LOG(*this);
	DEBUG_LOG(*ArgArray[InferenceParameter1]);
	if (DestructiveSyntacticallyEvaluateOnce(ArgArray[InferenceParameter1]))
		{
		ArgArray[InferenceParameter1]->ForceStdForm();
		DEBUG_LOG(*this);
		if (!ArgArray[InferenceParameter1]->CanEvaluate())
			IdxCurrentSelfEvalRule=None_SER;
		assert(SyntaxOK());
		return true;
		};
	// check precondition here for efficiency; this breaks an infinite-loop bug
	SUCCEED_OR_DIE(ArgArray[InferenceParameter1]->CanEvaluate());
	return false;
}

bool MetaConceptWithArgArray::SelfEvalRuleEvaluateArgOneStage()
{	// FORMALLY CORRECT: Kenneth Boyd, 9/23/2000
	assert(size()>InferenceParameter1);
	if (DestructiveSyntacticallyEvaluateOnce(ArgArray[InferenceParameter1]))
		{
		ArgArray[InferenceParameter1]->ForceStdForm();
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		};
	return false;
}

#define DEBUG_FUNC 1
bool MetaConceptWithArgArray::SelfEvalStrictlyModify()
{	// FORMALLY CORRECT: Kenneth Boyd, 1/24/2002
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);

	LOG("Using");
	LOG(*ArgArray[InferenceParameter1]);
	LOG("to modify");
	LOG(*ArgArray[InferenceParameter2]);
	LOG("to:");

#ifdef DEBUG_FUNC
	MetaConcept* old = NULL;
	ArgArray[InferenceParameter2]->CopyInto(old);
#endif
	
	ArgArray[InferenceParameter1]->StrictlyModifies(ArgArray[InferenceParameter2]);
	ArgArray[InferenceParameter2]->ForceStdForm();
	LOG(*ArgArray[InferenceParameter2]);	

#ifdef DEBUG_FUNC
	SUCCEED_OR_DIE(*old != *ArgArray[InferenceParameter2]);
	delete old;
	old = NULL;
#endif

	if 		(ArgArray[InferenceParameter2]->StrictlyImplies(*ArgArray[InferenceParameter1]))
		{
		if (2==fast_size())
			{
			InvokeEvalForceArg(InferenceParameter1);
			assert(SyntaxOK());
			return true;
			};
		LOG("which implies the initial clause doing the modification.");
		FastDeleteIdx(InferenceParameter1);
		if (InferenceParameter1<InferenceParameter2) --InferenceParameter2;
		}
	else if (ArgArray[InferenceParameter2]->CanStrictlyModify(*ArgArray[InferenceParameter1]))
		{
		LOG("which modifies the initial clause doing the modification to:");
		ArgArray[InferenceParameter2]->StrictlyModifies(ArgArray[InferenceParameter1]);
		ArgArray[InferenceParameter1]->ForceStdForm();
		LOG(*ArgArray[InferenceParameter1]);
		if (ArgArray[InferenceParameter1]->CanEvaluate())
			{
			if (!ArgArray[InferenceParameter2]->CanEvaluate())
				{
				IdxCurrentSelfEvalRule=SelfEvalRuleEvaluateArg_SER;
				assert(SyntaxOK());
				return true;
				}
			else{
				IdxCurrentSelfEvalRule=SelfEvalRuleEvaluateArgs_SER;
				assert(SyntaxOK());
				return true;
				}
			}
		};
	if (!ArgArray[InferenceParameter2]->CanEvaluate())
		{
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	else{
		InferenceParameter1 = InferenceParameter2;
		IdxCurrentSelfEvalRule=SelfEvalRuleEvaluateArg_SER;
		assert(SyntaxOK());
		return true;
		}
}
#undef DEBUG_FUNC

#define DEBUG_FUNC 1
bool MetaConceptWithArgArray::VirtualStrictlyModify()
{
	assert(!InferenceParameterMC.empty());
	assert(size()>InferenceParameter1);
	LOG("Using this");
	LOG(*InferenceParameterMC);
	LOG("to modify this");
	LOG(*ArgArray[InferenceParameter1]);
	LOG("to this:");

#ifdef DEBUG_FUNC
	MetaConcept* old = NULL;
	ArgArray[InferenceParameter1]->CopyInto(old);
#endif
	
	InferenceParameterMC->StrictlyModifies(ArgArray[InferenceParameter1]);
	ArgArray[InferenceParameter1]->ForceStdForm();
	InferenceParameterMC.reset();

	LOG(*ArgArray[InferenceParameter1]);	

#ifdef DEBUG_FUNC
	SUCCEED_OR_DIE(*old != *ArgArray[InferenceParameter1]);
	delete old;
	old = NULL;
#endif

	if (!ArgArray[InferenceParameter1]->CanEvaluate())
		{
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	else{
		IdxCurrentSelfEvalRule=SelfEvalRuleEvaluateArg_SER;
		assert(SyntaxOK());
		return true;
		}
}
#undef DEBUG_FUNC


bool MetaConceptWithArgArray::SelfEvalRuleEvaluateArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 1/24/2002
	// NOTE: de-flagging increases code size without measurably speeding
	// anything up.
	assert(size()>InferenceParameter2);
	assert(InferenceParameter1<=InferenceParameter2);
	DEBUG_LOG(__PRETTY_FUNCTION__);
	bool Flag = false;
	size_t i = InferenceParameter1;
	if (HasAnnihilatorKey())
		{
		do	if (DestructiveSyntacticallyEvaluateOnce(ArgArray[i]))
				{
				ArgArray[i]->ForceStdForm();
				// annihilator key detection
				if (ThisIsAnnihilatorKey(i,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
					{	// final cleanup
					InferenceParameter1 = i;
					assert(SyntaxOK());
					return true;		
					};
				Flag = true;
				}
		while(InferenceParameter2>= ++i);
		}
	else{
		do	if (DestructiveSyntacticallyEvaluateOnce(ArgArray[i]))
				{
				ArgArray[i]->ForceStdForm();
				Flag = true;
				}
		while(InferenceParameter2>= ++i);
		};
	if (Flag)
		{
		DEBUG_LOG("Entering interval collapse");
		DEBUG_LOG("Next");
		DEBUG_LOG(InferenceParameter1);
		DEBUG_LOG(*ArgArray[InferenceParameter1]);
		while(!ArgArray[InferenceParameter1]->CanEvaluate())
			{
			if (++InferenceParameter1>InferenceParameter2)
				{
				DEBUG_LOG("loop 1 succeeded");
				assert(SyntaxOK());
				return SelfEvalCleanEnd();
				};
			DEBUG_LOG("Next");
			DEBUG_LOG(InferenceParameter1);
			DEBUG_LOG(*ArgArray[InferenceParameter1]);
			}
		DEBUG_LOG("past loop 1");
		while(!ArgArray[InferenceParameter2]->CanEvaluate()) --InferenceParameter2;
		DEBUG_LOG("past loop 2");
		if (InferenceParameter1==InferenceParameter2)
			IdxCurrentSelfEvalRule = SelfEvalRuleEvaluateArg_SER;
		DEBUG_LOG("Leaving interval collapse");
		assert(SyntaxOK());
		return true;
		};
	return false;
}

bool MetaConceptWithArgArray::SelfEvalRuleForceArgSameImplementation()
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2000
	// InferenceParameter1 points to arg.
	assert(size()>InferenceParameter1);
	assert(ArgArray[InferenceParameter1]);
	assert(typeid(*this)==typeid(*ArgArray[InferenceParameter1]));
	MetaConceptWithArgArray* src = static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1]);
	ArgArray[InferenceParameter1]=NULL;
	src->ArgArray.MoveInto(ArgArray);
	src->InferenceParameterMC.MoveInto(InferenceParameterMC);
	VFTable1 = src->VFTable1;
	InferenceParameter1 = src->InferenceParameter1;
	InferenceParameter2 = src->InferenceParameter2;
	IdxCurrentEvalRule = src->IdxCurrentEvalRule;
	IdxCurrentSelfEvalRule = src->IdxCurrentSelfEvalRule;
	delete src;
	assert(SyntaxOK());
	return true;
}

bool MetaConceptWithArgArray::SelfEvalRuleCleanArg()
{	// FORMALLY CORRECT: 10/18/1999
	assert(size()>InferenceParameter1);
	DeleteIdx(InferenceParameter1);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalRuleCleanTrailingArg()
{	// FORMALLY CORRECT: 10/22/2005
	assert(size()>InferenceParameter1);
	ArgArray.Shrink(InferenceParameter1);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalRuleCleanLeadingArg()
{	// FORMALLY CORRECT: 5/18/2000
	assert(size()>InferenceParameter1);
	DeleteNSlotsAt(InferenceParameter1,0);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalRuleNIFFXORCleanArg()
{	// FORMALLY CORRECT: 10/18/1999
	assert(size()>InferenceParameter1);
	DeleteIdx(InferenceParameter1);
	if (2==fast_size())
		{
		SetExactType(LogicalIFF_MC);
		ArgArray[1]->SelfLogicalNOT();
		}
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalRuleNIFFXORCleanTrailingArg()
{	// FORMALLY CORRECT: 10/22/2005
	assert(size()>InferenceParameter1);
	ArgArray.Shrink(InferenceParameter1);
	if (2==fast_size())
		{
		SetExactType(LogicalIFF_MC);
		ArgArray[1]->SelfLogicalNOT();
		}
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalRuleAry2CorrectedCleanArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/14/2000
	assert(size()>InferenceParameter1);
	DeleteIdx(InferenceParameter1);
	if (2==fast_size()) SetExactType((ExactType_MC)(InferenceParameter2));
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalRuleAry2CorrectedCleanTrailingArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/22/2005
	assert(size()>InferenceParameter1);
	ArgArray.Shrink(InferenceParameter1);
	if (2==InferenceParameter1)
		SetExactType((ExactType_MC)(InferenceParameter2));
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalRuleUnrollGeneralizedAssociativity()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/2003
	assert(size()>InferenceParameter1);
	do	{
		assert(ArgArray[InferenceParameter1]);
		MetaConceptWithArgArray* Tmp = static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1]);
		const size_t InsertArgCount = Tmp->fast_size();
		if (!InsertNSlotsAtV2(InsertArgCount-1,InferenceParameter1+1))
			return false;

		memmove(&ArgArray[InferenceParameter1],Tmp->ArgArray,_msize(Tmp->ArgArray));
		std::fill_n(Tmp->ArgArray.begin(),InsertArgCount,(MetaConcept*)NULL);
		delete Tmp;
		}
	// Franci: the diagnosis rule should have sorted first
	while(   IsSymmetric() && 0<InferenceParameter1
		  && ArgArray[--InferenceParameter1]->IsExactType(ExactType()));

	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::SelfEvalAddArgAtEndAndForceCorrectForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/2005
	// InferenceParameter1 points to the clause to be inserted
	assert(!InferenceParameterMC.empty());
	LOG("Adding this:");
	LOG(*InferenceParameterMC);	
	LOG("to:");
	LOG(*this);
	if (!AddArgAtEndAndForceCorrectForm(InferenceParameterMC))
		{	// not fatal.  If the situation doesn't improve, we could have a RAM stall.
		LOG("RAM Failure: arg insertion");
		return false;
		}
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::CompatibleRetype()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/11/2000
	SetExactTypeV2((ExactType_MC)(InferenceParameter1));
	assert(SyntaxOK());
	return true;
}

bool MetaConceptWithArgArray::CompatibleRetypeOtherArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/3/2000
	DeleteIdx(InferenceParameter1);
	SetExactTypeV2((ExactType_MC)(InferenceParameter2));
	assert(SyntaxOK());
	return true;
}

bool MetaConceptWithArgArray::DiagnoseStandardEvalRules() const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/8/2002
	// Can evaluate to same or different type?
	// Alteration is strictly for code maintainability; this should decelerate compared to inline
	//! \todo identity scan (needs each nary-type to know what its identity element, if any, is)
	if (   DiagnoseEqualArgs() || DiagnoseEvaluatableArgs()
		|| DiagnoseSelfAssociativeArgs())
		return true;
	return false;
}

bool MetaConceptWithArgArray::DiagnoseEqualArgs() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/15/2001
	if (FindTwoEqualArgs() && InvokeEqualArgRule())
		{
		LOG("(Using syntactically equal arguments)");
		LOG(*ArgArray[InferenceParameter1]);
		LOG(*this);
		return true;
		}
	return false;
}

bool MetaConceptWithArgArray::DiagnoseEvaluatableArgs() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/15/2001
	if (!ArgArray.empty())
		{
		const MetaConcept* const * const lhs_array = ArgArray.data();
		InferenceParameter2 = fast_size();
		do	if (lhs_array[--InferenceParameter2]->CanEvaluate())
				{
				InferenceParameter1 = 0;
				while(    InferenceParameter1<InferenceParameter2
					  && !lhs_array[InferenceParameter1]->CanEvaluate())
					InferenceParameter1++;
				IdxCurrentSelfEvalRule = (InferenceParameter1<InferenceParameter2) ? SelfEvalRuleEvaluateArgs_SER : SelfEvalRuleEvaluateArg_SER;
				return true;
				}
		while(0<InferenceParameter2);
		}
	return false;
}

bool MetaConceptWithArgArray::SilentDiagnoseSelfAssociativeArgs() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/2003
	if (SelfAssociative_LITBMP1MC & VFTable1->Bitmap1)
		{
		InferenceParameter1 = size();
		while(0<InferenceParameter1)
			if (   ArgArray[--InferenceParameter1]->IsExactType(ExactType())
				&& ArgArray[InferenceParameter1]->NoMetaModifications())
				{
				IdxCurrentSelfEvalRule=SelfEvalRuleUnrollGeneralizedAssociativity_SER;
				return true;
				}
		};
	return false;
}

bool MetaConceptWithArgArray::DiagnoseSelfAssociativeArgs() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/2003
	if (SelfAssociative_LITBMP1MC & VFTable1->Bitmap1)
		{
		InferenceParameter1 = size();
		while(0<InferenceParameter1)
			if (   ArgArray[--InferenceParameter1]->IsExactType(ExactType())
				&& ArgArray[InferenceParameter1]->NoMetaModifications())
				{
				LOG("(Using generalized associativity to remove unnecessary grouping)");
				IdxCurrentSelfEvalRule=SelfEvalRuleUnrollGeneralizedAssociativity_SER;
				return true;
				}
		};
	return false;
}

bool MetaConceptWithArgArray::OneEqualOneIdempotentFirstTwoArgs(const MetaConceptWithArgArray& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/20/1998
	assert(2<=size());
	assert(2<=rhs.size());

	if		(*ArgArray[0]==*rhs.ArgArray[0])
		return IsAntiIdempotentTo(*ArgArray[1],*rhs.ArgArray[1]);
	else if (*ArgArray[0]==*rhs.ArgArray[1])
		return IsAntiIdempotentTo(*ArgArray[1],*rhs.ArgArray[0]);
	else if (*ArgArray[1]==*rhs.ArgArray[0])
		return IsAntiIdempotentTo(*ArgArray[0],*rhs.ArgArray[1]);
	else if (*ArgArray[1]==*rhs.ArgArray[1])
		return IsAntiIdempotentTo(*ArgArray[0],*rhs.ArgArray[0]);
	return false;
}

bool MetaConceptWithArgArray::SubvectorArgList(const MetaConceptWithArgArray& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/18/1999
	assert(!ArgArray.empty());
	// NOTE: this stores the first-failure index in RHS.InferenceParameter1, when returning
	// true
	const size_t lhs_size = fast_size();
	if (lhs_size<rhs.size())
		{
		size_t LowBoundIdx = 0;
		while(*ArgArray[LowBoundIdx]==*rhs.ArgArray[LowBoundIdx])
			if (lhs_size== ++LowBoundIdx)
				{
				rhs.InferenceParameter1=LowBoundIdx;
				return true;
				}
		size_t TolerateFailureCount = rhs.fast_size()-lhs_size;
		size_t HighBoundIdx = lhs_size-1;
		do	{
			while(*ArgArray[HighBoundIdx]==*rhs.ArgArray[HighBoundIdx+TolerateFailureCount])
				if (HighBoundIdx--==LowBoundIdx)
					{
					rhs.InferenceParameter1=LowBoundIdx;
					return true;
					};
			// known failures at RHS LowBoundIdx, HighBoundIdx+TolerateFailureCount
			do	if (0==TolerateFailureCount)
					return false;
			while(*ArgArray[HighBoundIdx]!=*rhs.ArgArray[HighBoundIdx+ --TolerateFailureCount]);
			}
		while(HighBoundIdx-- !=LowBoundIdx);
		rhs.InferenceParameter1=LowBoundIdx;
		return true;		
		};
	return false;
}

// NOTE: this routine returns the indices of the match in InferenceParameter1 and InferenceParameter2,
// if it succeeds; InferenceParameter1>InferenceParameter2
// This returns the *lowest* matching pair.
bool MetaConceptWithArgArray::FindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/28/2000
	const size_t strict_ub = size();
	assert(2<=strict_ub);

	size_t i = 1;
	do	{
		size_t j = i;
		do	if (TargetRelation(*ArgArray[i],*ArgArray[--j]))
				{
				InferenceParameter1 = i;
				InferenceParameter2 = j;
				return true;
				}
		while(0<j);
		}
	while(strict_ub> ++i);
	return false;
}

bool MetaConceptWithArgArray::FindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation, size_t StartIdx) const
{
	const size_t strict_ub = size();
	assert(2<=strict_ub);
	if (strict_ub<=StartIdx) return false;

	do	{
		size_t j = StartIdx;
		do	if (TargetRelation(*ArgArray[StartIdx],*ArgArray[--j]))
				{
				InferenceParameter1 = StartIdx;
				InferenceParameter2 = j;
				return true;
				}
		while(0<j);
		}
	while(strict_ub> ++StartIdx);
	return false;
}

// NOTE: this routine returns the indices of the match in InferenceParameter1 and InferenceParameter2,
// if it succeeds; InferenceParameter1<InferenceParameter2
// This returns the *lowest* matching pair.
bool MetaConceptWithArgArray::DualFindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/28/2000
	const size_t strict_ub = size();
	assert(2<=strict_ub);

	size_t i = 1;
	do	{
		size_t j = i;
		do	if (TargetRelation(*ArgArray[--j],*ArgArray[i]))
				{
				InferenceParameter1 = j;
				InferenceParameter2 = i;
				return true;
				}
		while(0<j);
		}
	while(strict_ub> ++i);
	return false;
}

bool MetaConceptWithArgArray::DualFindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation, size_t StartIdx) const
{
	const size_t strict_ub = size();
	assert(2<=strict_ub);
	if (strict_ub<=StartIdx) return false;

	do	{
		size_t j = StartIdx;
		do	if (TargetRelation(*ArgArray[--j],*ArgArray[StartIdx]))
				{
				InferenceParameter1 = j;
				InferenceParameter2 = StartIdx;
				return true;
				}
		while(0<j);
		}
	while(strict_ub> ++StartIdx);
	return false;
}

bool MetaConceptWithArgArray::FindTwoRelatedArgs(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: 1/23/2000
	assert(!ArgArray.empty());
	assert(!rhs.ArgArray.empty());

	size_t i = fast_size();
	do	{
		--i;
		size_t j = rhs.fast_size();
		do	if (TargetRelation(*ArgArray[i],*rhs.ArgArray[--j]))
				{
				InferenceParameter1 = i;
				rhs.InferenceParameter1 = j;
				return true;
				}
		while(0<j);
		}
	while(0<i);
	return false;
}

bool MetaConceptWithArgArray::FindTwoRelatedArgs(const MetaConceptWithArgArray& rhs,
											LowLevelBinaryRelation& TargetRelation,
											size_t IgnoreLHSArg, size_t IgnoreRHSArg) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/13/2000
	assert(!ArgArray.empty());
	assert(!rhs.ArgArray.empty());

	size_t i = fast_size();
	do	if (IgnoreLHSArg!= --i)
			{
			size_t j = rhs.fast_size();
			do	if (IgnoreRHSArg!=--j && TargetRelation(*ArgArray[i],*rhs.ArgArray[j]))
					{
					InferenceParameter1 = i;
					rhs.InferenceParameter1 = j;
					return true;
					}
			while(0<j);
			}
	while(0<i);
	return false;
}

bool MetaConceptWithArgArray::FindTwoEqualArgs() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/2000
	assert(2<=size());
	size_t i = 1;
	if (IsSymmetric())
		{
		do	if (*ArgArray[i]==*ArgArray[i-1])
				{
				InferenceParameter1 = i;
				InferenceParameter2 = i-1;
				return true;
				}
		while(fast_size()> ++i);
		return false;
		};

	do	{
		size_t j = i;
		do	if (*ArgArray[i]==*ArgArray[--j])
				{
				InferenceParameter1 = i;
				InferenceParameter2 = j;
				return true;
				}
		while(0<j);
		}
	while(fast_size()> ++i);
	return false;
}

bool MetaConceptWithArgArray::FindTwoEqualArgsLHSRHSLexicalOrderedArgs(const MetaConceptWithArgArray& rhs) const
{	// FORMALLY CORRECT: 4/22/2000
	assert(!ArgArray.empty());
	assert(!rhs.ArgArray.empty());
	size_t Idx1 = 0;
	size_t Idx2 = 0;
Restart:
	if 		(*ArgArray[Idx1]==*rhs.ArgArray[Idx2])
		{
		InferenceParameter1 = Idx1;
		rhs.InferenceParameter1 = Idx2;
		return true;
		}
	else if (ArgArray[Idx1]->InternalDataLT(*rhs.ArgArray[Idx2]))
		{
		if (fast_size()> ++Idx1) goto Restart;
		return false;
		}
	else{
		if (rhs.fast_size()> ++Idx2) goto Restart;
		return false;
		};
}

static bool SymmetricFindTwoAntiIdempotentArgs(const MetaConcept* const * const ArgArray, size_t& InferenceParameter1,
								   size_t& InferenceParameter2, const size_t strict_ub)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/9/2000	
	assert(!ArgArray.empty());
	assert(2<=strict_ub);
	size_t i = 1;
	do	{
		size_t j = i;
		do	if (IsAntiIdempotentTo(*ArgArray[i],*ArgArray[--j]))
				{
				InferenceParameter1 = i;
				InferenceParameter2 = j;
				return true;
				}
		while(0<j);
		}
	while(strict_ub> ++i);
	return false;
}

bool MetaConceptWithArgArray::FindTwoAntiIdempotentArgsSymmetric() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/6/2003
	assert(2<=ArgArray.size());
	if (ArgArray[0]->IsExactType(Variable_MC))
		{
		size_t HighBoundIdx = 1;
		while(ArgArray[HighBoundIdx]->IsExactType(Variable_MC))
			if 		(IsAntiIdempotentTo(*ArgArray[HighBoundIdx],*ArgArray[HighBoundIdx-1]))
				{
				InferenceParameter1 = HighBoundIdx;
				InferenceParameter2 = HighBoundIdx-1;
				return true;
				}
			else if (fast_size()<= ++HighBoundIdx)
				return false;
		size_t VR_HighBound = fast_size()-HighBoundIdx;
		if (1==VR_HighBound) return false;
		if (SymmetricFindTwoAntiIdempotentArgs(ArgArray+HighBoundIdx,InferenceParameter1,InferenceParameter2,VR_HighBound))
			{
			InferenceParameter1 += HighBoundIdx;
			InferenceParameter2 += HighBoundIdx;
			return true;
			}
		return false;
		};
	return SymmetricFindTwoAntiIdempotentArgs(ArgArray,InferenceParameter1,InferenceParameter2,fast_size());
}

// LHS and RHS of TargetRelation, respectively
// Arg is pointed to by InferenceParameter1
bool FindArgRelatedToLHS(const MetaConcept& lhs, LowLevelBinaryRelation& TargetRelation, const MetaConcept* const * const ArgArray, size_t& Result)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2000
	size_t i = SafeArraySize(ArgArray);
	while(0<i)
		if (TargetRelation(lhs,*ArgArray[--i]))
			{
			Result = i;
			return true;
			};
	return false;
}

bool MetaConceptWithArgArray::FindArgRelatedToLHS(const MetaConcept& lhs, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2000
	size_t i = ArgArray.size();
	while(0<i)
		if (TargetRelation(lhs,*ArgArray[--i]))
			{
			InferenceParameter1 = i;
			return true;
			};
	return false;
}

bool MetaConceptWithArgArray::FindArgRelatedToLHSViewPoint(size_t ViewPoint, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2003
	assert(ArgArray.size()>ViewPoint);

	size_t i = ArgArray.size();
	while(0<i)
		if (   ViewPoint!= --i
			&& TargetRelation(*ArgArray[ViewPoint],*ArgArray[i]))
			{
			InferenceParameter1 = ViewPoint;
			InferenceParameter2 = i;
			return true;
			};
	return false;
}

bool MetaConceptWithArgArray::FindArgRelatedToRHSViewPoint(size_t ViewPoint, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2003
	assert(ArgArray.size()>ViewPoint);

	size_t i = ArgArray.size();
	while(0<i)
		if (   ViewPoint!= --i
			&& TargetRelation(*ArgArray[i],*ArgArray[ViewPoint]))
			{
			InferenceParameter1 = ViewPoint;
			InferenceParameter2 = i;
			return true;
			};
	return false;
}

bool MetaConceptWithArgArray::FindArgRelatedToLHS(const MetaConcept& lhs, LowLevelBinaryRelation& TargetRelation, const size_t NonStrictLB, const size_t NonStrictUB) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/26/2000
	assert(ArgArray.size()>NonStrictUB);
	assert(NonStrictLB<=NonStrictUB);

	size_t i = NonStrictUB+1;
	do	if (TargetRelation(lhs,*ArgArray[--i]))
			{
			InferenceParameter1 = i;
			return true;
			}
	while(NonStrictLB<i);
	return false;
}

bool MetaConceptWithArgArray::FindArgRelatedToRHS(const MetaConcept& rhs, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2000
	size_t i = ArgArray.size();
	while(0<i)
		if (TargetRelation(*ArgArray[--i],rhs))
			{
			InferenceParameter1 = i;
			return true;
			};
	return false;
}

bool MetaConceptWithArgArray::FindArgRelatedToRHS(const MetaConcept& rhs, LowLevelBinaryRelation& TargetRelation, const size_t NonStrictLB, const size_t NonStrictUB) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/26/2000
	assert(ArgArray.size()>NonStrictUB);
	assert(NonStrictLB<=NonStrictUB);

	size_t i = NonStrictUB+1;
	do	if (TargetRelation(*ArgArray[--i],rhs))
			{
			InferenceParameter1 = i;
			return true;
			}
	while(NonStrictLB<i);
	return false;
}

bool MetaConceptWithArgArray::FindArgOfExactType(ExactType_MC Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/27/2001
	size_t i = ArgArray.size();
	while(0<i)
		if (ArgArray[--i]->IsExactType(Target))
			{
			InferenceParameter1 = i;
			return true;
			};
	return false;
}

bool MetaConceptWithArgArray::FindArgTypeMatch(ExactType_MC Target, const AbstractClass* UltimateTarget) const
{
	size_t i = ArgArray.size();
	while(0<i)
		if (ArgArray[--i]->IsTypeMatch(Target,UltimateTarget))
			{
			InferenceParameter1 = i;
			return true;
			};
	return false;
}


bool MetaConceptWithArgArray::ArgListHasInjectionIntoRHSArgListRelatedThisWay(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2000
	// Do this by:
	// 1: matching LHS-arg to a candidate RHS-arg.
	// 2: verifying that this LHS-arg is the only LHS-arg matching that RHS-arg.
	// 3: verifying that this LHS-arg only matches this RHS-arg.
	// this algorithm does not use dynamic RAM allocation.
	if (   !ArgArray.empty()
	    && !rhs.ArgArray.empty()
		&& fast_size()<=rhs.fast_size())
		{
		TruthValue TmpArg(TruthValue::True);
		size_t LHSArgIdx = 0;
		do	{
			// 1: matching LHS-arg to a candidate RHS-arg.
			if (!rhs.FindArgRelatedToLHS(*ArgArray[LHSArgIdx],TargetRelation))
				return false;
			size_t RHSArgIdx = rhs.InferenceParameter1;
			// 2: verifying that this LHS-arg is the only LHS-arg matching that RHS-arg.
			MetaConcept* Tmp = ArgArray[LHSArgIdx];
			ArgArray[LHSArgIdx] = &TmpArg;
			if (FindArgRelatedToRHS(*rhs.ArgArray[RHSArgIdx],TargetRelation))
				{
				ArgArray[LHSArgIdx] = Tmp;
				return false;
				};
			ArgArray[LHSArgIdx] = Tmp;
			// 3: verifying that this LHS-arg only matches this RHS-arg.
			Tmp = rhs.ArgArray[RHSArgIdx];
			rhs.ArgArray[RHSArgIdx] = &TmpArg;
			if (rhs.FindArgRelatedToLHS(*ArgArray[LHSArgIdx],TargetRelation))
				{
				rhs.ArgArray[RHSArgIdx] = Tmp;
				return false;
				};
			rhs.ArgArray[RHSArgIdx] = Tmp;
			}
		while(fast_size()> ++LHSArgIdx);
		return true;
		};
	return false;
}

// Verify Unary property rules.  May have to create metacode eventually.
bool MetaConceptWithArgArray::VerifyArgsExplicitConstant(size_t lb, size_t strict_ub) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/22/2000
	assert(ArgArray.size()>=strict_ub);
	assert(lb<strict_ub);
	return and_range(boost::mem_fun(&MetaConcept::IsExplicitConstant),ArgArray+lb,ArgArray+strict_ub);
}

bool FindExplicitConstantArg(const MetaConcept* const * const ArgArray, size_t i, size_t& Param1)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/11/2000
	assert(SafeArraySize(ArgArray)>=i);
	assert(0<i);
	do	if (ArgArray[--i]->IsExplicitConstant())
			{
			Param1 = i;
			return true;
			}
	while(0<i);
	return false;	
}

bool MetaConceptWithArgArray::FindArgWithUnaryProperty(LowLevelUnaryProperty& Property) const
{
	size_t i = ArgArray.size();
	while(0<i)
		if (Property(*ArgArray[--i]))
			{
			InferenceParameter1 = i;
			return true;
			};
	return false;
}

bool MetaConceptWithArgArray::FindArgWithUnaryProperty(LowLevelUnaryProperty& Property, size_t StrictUB) const
{
	if (fast_size()<StrictUB) StrictUB = fast_size();

	do	if (Property(*ArgArray[--StrictUB]))
			{
			InferenceParameter1 = StrictUB;
			return true;
			}
	while(0<StrictUB);
	return false;
}

bool MetaConceptWithArgArray::FindLRMultInvCompetentExplicitConstArgInArgArray() const
{	// TODO: VERIFY
	// NOTE: we rely on StdMultiplication to concentrate constants where we need them.
	assert(!ArgArray.empty());
	if (    ArgArray[0]->IsExplicitConstant()
		&& !ArgArray[0]->IsUltimateType(NULL)
		&&  ArgArray[0]->UltimateType()->SupportsThisOperation(StdMultiplication_MC))
		{
		InferenceParameter1 = 0;	// left inverse
		return true;
		}
	if (    ArgArray[fast_size()-1]->IsExplicitConstant()
		&& !ArgArray[fast_size()-1]->IsUltimateType(NULL)
		&&  ArgArray[fast_size()-1]->UltimateType()->SupportsThisOperation(StdMultiplication_MC))
		{
		InferenceParameter1 = fast_size()-1;	// right inverse
		return true;
		}
	return false;
}

bool MetaConceptWithArgArray::BlockableAnnihilatorScan(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule, LowLevelBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/13/2003
	bool BlockedAbove = false;
	bool BlockedBelow = false;
	const size_t old_size = fast_size();
	size_t Above = 0;
	size_t Below = 0;

	// can't do anything? not really an annihilator
	if (0<ArgIdx)
		{
		size_t i = ArgIdx;
		do	if (TargetRelation(*ArgArray[ArgIdx],*ArgArray[--i]))
				{
				BlockedBelow = true;
				Below = i;
				break;
				}
		while(0<i);
		}
	if (old_size>ArgIdx+1)
		{
		size_t i = ArgIdx+1;
		do	if (TargetRelation(*ArgArray[ArgIdx],*ArgArray[i]))
				{
				BlockedAbove = true;
				Above = i;
				break;
				}
		while(old_size> ++i);
		}
	
	if (BlockedAbove)
		{
		if (BlockedBelow)
			{	// eliminate args between, between
			if (Below+1<ArgIdx)
				swap(ArgArray[ArgIdx],ArgArray[Below+1]);
			else if (ArgIdx+1==Above)
				return false;

			ArgIdx = Below+1;
			while(Above<old_size)
				swap(ArgArray[++ArgIdx],ArgArray[Above++]);

			ArgIdx++;
			SelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
			EvalRule = None_ER;
			return true;
			}
		else{	// eliminate args between, below
			if (ArgIdx+1<Above)
				swap(ArgArray[ArgIdx],ArgArray[Above-1]);
			else if (0==ArgIdx)
				return false;

			ArgIdx = Above-2;
			SelfEvalRule = SelfEvalRuleCleanLeadingArg_SER;
			EvalRule = None_ER;
			return true;
			}
		}
	else if (BlockedBelow)
		{	// eliminate args between, above
		if (Below+1<ArgIdx)
			swap(ArgArray[ArgIdx],ArgArray[Below+1]);
		else if (ArgIdx+1==old_size)
			return false;

		ArgIdx = Below+2;
		SelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
		EvalRule = None_ER;
		return true;
		}
	else{	// classic annihilator
		SelfEvalRule = None_SER;
		EvalRule = EvalForceArg_ER;
		return true;
		}
}

bool MetaConceptWithArgArray::BlockableAnnihilatorScan(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule, LowLevelSideEffectBinaryRelation& TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/13/2003
	bool BlockedAbove = false;
	bool BlockedBelow = false;
	size_t Above = 0;
	size_t Below = 0;

	// can't do anything? not really an annihilator
	if (0<ArgIdx)
		{
		size_t i = ArgIdx;
		do	if (TargetRelation(*ArgArray[ArgIdx],*ArgArray[--i]))
				{
				BlockedBelow = true;
				Below = i;
				break;
				}
		while(0<i);
		}
	if (fast_size()>ArgIdx+1)
		{
		size_t i = ArgIdx+1;
		do	if (TargetRelation(*ArgArray[ArgIdx],*ArgArray[i]))
				{
				BlockedAbove = true;
				Above = i;
				break;
				}
		while(fast_size()> ++i);
		}
	
	if (BlockedAbove)
		{
		if (BlockedBelow)
			{	// eliminate args between, between
			if (Below+1<ArgIdx)
				swap(ArgArray[ArgIdx],ArgArray[Below+1]);
			else if (ArgIdx+1==Above)
				return false;

			ArgIdx = Below+1;
			while(Above<fast_size())
				swap(ArgArray[++ArgIdx],ArgArray[Above++]);

			ArgIdx++;
			SelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
			EvalRule = None_ER;
			return true;
			}
		else{	// eliminate args between, below
			if (ArgIdx+1<Above)
				swap(ArgArray[ArgIdx],ArgArray[Above-1]);
			else if (0==ArgIdx)
				return false;

			ArgIdx = Above-2;
			SelfEvalRule = SelfEvalRuleCleanLeadingArg_SER;
			EvalRule = None_ER;
			return true;
			}
		}
	else{
		if (BlockedBelow)
			{	// eliminate args between, above
			if (Below+1<ArgIdx)
				swap(ArgArray[ArgIdx],ArgArray[Below+1]);
			else if (ArgIdx+1==fast_size())
				return false;

			ArgIdx = Below+2;
			SelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
			EvalRule = None_ER;
			return true;
			}
		else{	// classic annihilator
			SelfEvalRule = None_SER;
			EvalRule = EvalForceArg_ER;
			return true;
			}
		}
}

static void InsertSortSnapDown(MetaConcept** const ArgArray, size_t StartIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2000
	if (ArgArray[StartIdx]->InternalDataLT(*ArgArray[StartIdx-1]))
		do	swap(ArgArray[StartIdx],ArgArray[StartIdx-1]);
		while(0<--StartIdx && ArgArray[StartIdx]->InternalDataLT(*ArgArray[StartIdx-1]));
}

// TODO: check for safe short-circuits
void TotalSortByLexicalOrder(MetaConcept** const ArgArray, size_t StrictMaxIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2000
	// TODO: IMPROVE ON INSERTSORT
	if (1>=StrictMaxIdx) return;
	// sort the n arguments
	// Until a better idea occurs, use Insertsort
	// INSERTSORT
	size_t StartIdx = 1;
	do	InsertSortSnapDown(ArgArray,StartIdx);
	while(++StartIdx<StrictMaxIdx);
}

void MetaConceptWithArgArray::ForceTotalLexicalArgOrder() const
{	// FORMALLY CORRECT, Kenneth Boyd 1/24/2002
	assert(Symmetric_LITBMP1MC & VFTable1->Bitmap1);
	TotalSortByLexicalOrder(ArgArray,fast_size());
}

void MetaConceptWithArgArray::ForceLexicalArgOrder(size_t lb, size_t strict_ub) const
{	// FORMALLY CORRECT: 6/6/2000
	if (fast_size()<strict_ub)
		strict_ub = fast_size();
	assert(lb<strict_ub);
	TotalSortByLexicalOrder(ArgArray+lb,strict_ub-lb);
}

void MetaConceptWithArgArray::CompactLowLevelArrayVectorToDeleteTrailingArgs(unsigned long* IndexArray) const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/22/2000
	assert(!ArgArray.empty());
	const size_t IndexArray_size = ArraySize(IndexArray);
	IdxCurrentSelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
	InferenceParameter1 = fast_size()-IndexArray_size;

	// force args to top
	size_t i = IndexArray_size;
	size_t TargetIdx = fast_size()-1;
	do	{
Resume:
		size_t j = i;
		do	if (   IndexArray[--j]==TargetIdx
				&& --i>j)
				{
				memmove(IndexArray+j,IndexArray+j+1,sizeof(unsigned long)*(i-j));
				if (0<i)
					goto Resume;
				else
					goto BreakOut;
				}
		while(0<j);
		SwapArgs(IndexArray[--i],TargetIdx--);
		}
	while(0<i);
BreakOut:
	delete [] IndexArray;
}

// TODO: META: where is this called from, and is the TargetRelation parameter necessary?
bool MetaConceptWithArgArray::CheckForTrailingCleanArg(LowLevelBinaryRelation& TargetRelation, SelfEvalRuleIdx_SER ChangeToRule, size_t ForceArity) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/18/2003
	// ASSUMPTION: TargetRelation is symmetric [then we should have used FindTwoRelatedArgs(_),
	// and InferenceParameter1>InferenceParameter2.  This will be the *lowest* matching pair.
	// NOTE: this is being invoked to clean redundant args by InvokeEqualArgRule.  It may be 
	// better behavior to "pretend to fail".  Since this involves calling a non-const member function
	// from a const member function, this is somewhat delicate: we cannot use a CompatibleRetype
	// effect safely.
	if (fast_size()>InferenceParameter1+1)
		{
		const MetaConcept& LHS = *ArgArray[InferenceParameter1];
		size_t i = InferenceParameter1+1;
		do	if (TargetRelation(LHS,*ArgArray[i]))
				{	// InferenceParameter1, i both due to be blotted
				LOG(*ArgArray[i]);
				IdxCurrentSelfEvalRule = ChangeToRule;
				if (fast_size()-1>i)
					SwapArgs(i,fast_size()-1);
				if (fast_size()-2>InferenceParameter1)
					SwapArgs(InferenceParameter1,fast_size()-2);
				InferenceParameter2 = InferenceParameter1;
				InferenceParameter1 = fast_size()-2;
				while(	 i+1<InferenceParameter1
					  && ForceArity<InferenceParameter1
					  && TargetRelation(LHS,*ArgArray[i]))
					SwapArgs(i,--InferenceParameter1);
				while(	 InferenceParameter2+1<InferenceParameter1
					  && ForceArity<InferenceParameter1
					  && TargetRelation(LHS,*ArgArray[InferenceParameter2]))
					SwapArgs(InferenceParameter2,--InferenceParameter1);
				while(InferenceParameter1> ++i)
					while(   TargetRelation(LHS,*ArgArray[i])
						  && ForceArity<InferenceParameter1
						  && i+1<InferenceParameter1)
						SwapArgs(i,--InferenceParameter1);
				const_cast<MetaConceptWithArgArray* const>(this)->DestructiveEvaluateToSameType();
				if (FindTwoEqualArgs())
					return InvokeEqualArgRule();
				return false;
				}
		while(fast_size()> ++i);
		}
	// here: InferenceParameter1>InferenceParameter2, nothing else tripped
	LOG(*ArgArray[InferenceParameter1]);
	{	// all clear
	const_cast<MetaConceptWithArgArray* const>(this)->DestructiveEvaluateToSameType();
	if (FindTwoEqualArgs()) return InvokeEqualArgRule();
	}
	return false;
}

void MetaConceptWithArgArray::UseArg0ForPropagateAddMultInv(MetaConcept*& dest)
{
	assert(!dest);
	FastTransferOutAndNULL(0,dest);

	// propagate Add/MultInv to Target
	if (IsMetaAddInverted()) dest->SelfInverse(StdAddition_MC);
	if (IsMetaMultInverted()) dest->SelfInverse(StdMultiplication_MC);

	assert(dest->SyntaxOK());
}

#if 0
bool GrepArgList(MetaConcept**& MirrorArgList,size_t*& IndexMap,LowLevelUnaryProperty& WantThis, MetaConcept** const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/2/2001
	DEBUG_LOG(__PRETTY_FUNCTION__);
	assert(NULL==MirrorArgList);
	assert(NULL==IndexMap);
	assert(NULL!=WantThis);
	assert(ValidateArgArray(ArgArray));
	size_t i = ArraySize(ArgArray);
	do	if (WantThis(*ArgArray[--i]))
			{
			size_t j = 0;
			do	if (WantThis(*ArgArray[j]))
					{
					size_t k = 0;
					MirrorArgList = _new_buffer_uninitialized<MetaConcept*>(i-j+1);
					if (!MirrorArgList) return false;
					IndexMap = _new_buffer_uninitialized<size_t>(i-j+1);
					if (!IndexMap)
						{
						FREE_AND_NULL(MirrorArgList);
						return false;
						}
					MirrorArgList[k]=ArgArray[j];
					IndexMap[k++]=j;
					while(i>=++j)
						if (WantThis(*ArgArray[j]))
							{
							MirrorArgList[k]=ArgArray[j];
							IndexMap[k++]=j;
							}
					MirrorArgList = REALLOC(MirrorArgList,k*sizeof(MetaConcept*));
					IndexMap = REALLOC(IndexMap,k*sizeof(size_t));
					return true;
					}
			while(i>= ++j);
			}
	while(0<i);
	return true;
}
#endif

bool GrepArgList(MetaConcept**& MirrorArgList,LowLevelUnaryProperty& WantThis, MetaConcept** const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/2/2001
	assert(NULL==MirrorArgList);
	assert(NULL!=ArgArray);
	size_t i = ArraySize(ArgArray);
	do	if (WantThis(*ArgArray[--i]))
			{
			size_t j = 0;
			do	if (WantThis(*ArgArray[j]))
					{
					size_t k = 0;
					MirrorArgList = _new_buffer_uninitialized<MetaConcept*>(i-j+1);
					if (!MirrorArgList) return false;
					MirrorArgList[k++]=ArgArray[j];
					while(i>= ++j)
						if (WantThis(*ArgArray[j]))
							MirrorArgList[k++]=ArgArray[j];
					MirrorArgList = REALLOC(MirrorArgList,k*sizeof(MetaConcept*));
					return true;
					}
			while(i>= ++j);
			}
	while(0<i);
	return true;
}

bool GrepArgList(size_t*& IndexMap,LowLevelUnaryProperty& WantThis, MetaConcept** const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/2/2001
	assert(!IndexMap);
	assert(ArgArray);
	size_t i = ArraySize(ArgArray);
	do	if (WantThis(*ArgArray[--i]))
			{
			size_t j = 0;
			do	if (WantThis(*ArgArray[j]))
					{
					size_t k = 0;
					IndexMap = _new_buffer_uninitialized<size_t>(i-j+1);
					if (!IndexMap) return false;
					IndexMap[k++]=j;
					while(i>= ++j)
						if (WantThis(*ArgArray[j]))
							IndexMap[k++]=j;
					IndexMap = REALLOC(IndexMap,k*sizeof(size_t));
					return true;
					}
			while(i>= ++j);
			}
	while(0<i);
	return true;
}

bool DualGrepArgList(size_t*& IndexMap, size_t*& InverseIndexMap, LowLevelUnaryProperty& WantThis, MetaConcept** const ArgArray)
{
	assert(!IndexMap);
	assert(!InverseIndexMap);
	assert(ArgArray);
	IndexMap = _new_buffer_uninitialized<size_t>(ArraySize(ArgArray));
	if (!IndexMap)
		// TODO: recovery code.  Determine the size of both lists, then see if those can be
		// allocated
		return false;

	InverseIndexMap = _new_buffer_uninitialized<size_t>(ArraySize(ArgArray));
	if (!InverseIndexMap)
		{	// TODO: recovery code.  Determine the larger list, then see if the smaller one 
			// can be allocated
		free(IndexMap);
		IndexMap = NULL;
		return false;
		};

	size_t Idx3 = 0;
	size_t Idx4 = 0;
	size_t i = 0;
	do	if (WantThis(*ArgArray[i]))
			{
			IndexMap[Idx3++] = i;
			}
		else{
			InverseIndexMap[Idx4++] = i;
			}
	while(ArraySize(ArgArray)> ++i);

	// Be nice to the memory manager.  Either one needs deleting, or both need reallocating.
	if (0==Idx3)
		{
		free(IndexMap);
		IndexMap = NULL;
		}
	else if (0==Idx4)
		{
		free(InverseIndexMap);
		InverseIndexMap = NULL;
		}
	else{
		IndexMap = REALLOC(IndexMap,Idx3*sizeof(size_t));
		InverseIndexMap = REALLOC(InverseIndexMap,Idx4*sizeof(size_t));
		}
		
	return true;
}

#if 0
void SelfGrepArgList(LowLevelUnaryProperty& WantThis, MetaConcept**& ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/22/2005
	assert(NULL!=ArgArray);
	size_t Idx = 0;
	do	if (!WantThis(*ArgArray[Idx]))
			{
			delete ArgArray[Idx];
			size_t Offset = 1;
			while(ArraySize(ArgArray)>++Idx)
				if (!WantThis(*ArgArray[Idx]))
					{
					DELETE(ArgArray[Idx]);
					Offset++;
					}
				else
					ArgArray[Idx-Offset] = ArgArray[Idx];
			if (Offset==ArraySize(ArgArray))
				FREE_AND_NULL(ArgArray);
			else{
				ArgArray = REALLOC(ArgArray,_msize(ArgArray)-sizeof(MetaConcept*)*Offset);
				}
			return;
			}
	while(ArraySize(ArgArray)>++Idx);
}

void SelfGrepArgListNoOwnership(LowLevelBinaryRelation& WantThis, const MetaConcept& RHS, MetaConcept**& ArgArray, size_t*& IndexArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/22/2005
	assert(NULL!=ArgArray);
	assert(NULL!=IndexArray);
	size_t Idx = 0;
	do	if (!WantThis(*ArgArray[Idx],RHS))
			{
			size_t Offset = 1;
			while(ArraySize(ArgArray)>++Idx)
				if (!WantThis(*ArgArray[Idx],RHS))
					Offset++;
				else{
					ArgArray[Idx-Offset] = ArgArray[Idx];
					IndexArray[Idx-Offset] = IndexArray[Idx];
					}
			if (Offset==ArraySize(ArgArray))
				{
				FREE_AND_NULL(ArgArray);
				FREE_AND_NULL(IndexArray);
				}
			else{
				ArgArray = REALLOC(ArgArray,_msize(ArgArray)-sizeof(MetaConcept*)*Offset);
				IndexArray = REALLOC(IndexArray,_msize(IndexArray)-sizeof(size_t)*Offset);
				}
			return;
			}
	while(ArraySize(ArgArray)>++Idx);
}
#endif

void SelfGrepArgListNoOwnership(LowLevelBinaryRelation& WantThis, const MetaConcept& rhs, MetaConcept**& ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/22/2005
	assert(ArgArray);
	size_t i = 0;
	do	if (!WantThis(*ArgArray[i],rhs))
			{
			size_t Offset = 1;
			while(ArraySize(ArgArray)> ++i)
				if (!WantThis(*ArgArray[i],rhs))
					Offset++;
				else
					ArgArray[i-Offset] = ArgArray[i];
			if (Offset==ArraySize(ArgArray))
				FREE_AND_NULL(ArgArray);
			else
				ArgArray = REALLOC(ArgArray,_msize(ArgArray)-sizeof(MetaConcept*)*Offset);
			return;
			}
	while(ArraySize(ArgArray)> ++i);
}

bool MirrorArgArrayNoOwnership(size_t LocalUB,MetaConcept** LocalArgArray,MetaConcept**& MirrorArgArray,size_t MinArgCount,LowLevelBinaryRelation& TargetRelation)
{	// TODO: VERIFY
	assert(LocalArgArray);
	assert(!MirrorArgArray);

	do	{
		size_t i = LocalUB;
		while(--i>0 && !TargetRelation(*LocalArgArray[i],*LocalArgArray[LocalUB]));
		if (i>0 || TargetRelation(*LocalArgArray[0],*LocalArgArray[LocalUB]))
			break;
		}
	while(--LocalUB>0);
	size_t lb = 0;
	do	{
		size_t i = lb;
		while(++i<LocalUB && !TargetRelation(*LocalArgArray[i],*LocalArgArray[lb]));
		if (i<LocalUB || TargetRelation(*LocalArgArray[LocalUB],*LocalArgArray[lb]))
			break;
		}
	while(++lb<LocalUB);
	if (LocalUB-lb+1<MinArgCount) return false;

	MirrorArgArray = _new_buffer_uninitialized<MetaConcept*>(LocalUB-lb+1);
	if (!MirrorArgArray) return false;
	memmove(MirrorArgArray,LocalArgArray+lb,(LocalUB-lb+1)*sizeof(MetaConcept*));
	return true;
}

