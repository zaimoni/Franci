// StdMult.cxx
// implementation of StdMultiplication, an n-ary operation defined for a variety of sets
// Commutativity is optional (different mode).

#include "Class.hxx"
#include "Integer1.hxx"
#include "StdMult.hxx"
#include "LowRel.hxx"

#include <boost/functional.hpp>

StdMultiplication::SelfEvaluateRule StdMultiplication::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&StdMultiplication::AddIntegerToIntegerFraction,
	&StdMultiplication::AddIntegerFractionToIntegerFraction,
	&StdMultiplication::MultDistributesOverAdd_CondenseProductsOnNonConstArgs,
	&StdMultiplication::CleanIntegerNumeralBlock
	};

// NOTE: a 0-ary StdMultiplication is an "omnione": a one that matches its context
StdMultiplication::StdMultiplication(MetaConcept**& NewArgList)
:	MetaConceptWithArgArray(StdMultiplication_MC,NewArgList)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	if (SyntaxOK()) _forceStdForm();
}

const StdMultiplication&
StdMultiplication::operator=(const StdMultiplication& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	autoval_ptr<AbstractClass> Tmp1(_DesiredType);
	autoval_ptr<AbstractClass> Tmp2(_DynamicType);

	MetaConceptWithArgArray::operator=(src);
	Tmp1.MoveInto(_DesiredType);
	Tmp2.MoveInto(_DynamicType);
	return *this;
}

void StdMultiplication::MoveInto(StdMultiplication*& dest)		// can throw memory failure.  If it succeeds, it destroys the source.
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	if (!dest) dest = new StdMultiplication();
	MoveIntoAux(*dest);
	_DesiredType.MoveInto(dest->_DesiredType);
	_DynamicType.MoveInto(dest->_DynamicType);
}

//  Type ID functions
// NOTE: StdMultiplication UltimateType is non-NULL
const AbstractClass* StdMultiplication::UltimateType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2003
	if (!_DesiredType.empty()) return _DesiredType;
	if (_DynamicType.empty())
		{
		DetermineDynamicType();
		if (_DynamicType.empty()) return &ClassMultiplicationDefined;
		};
	return _DynamicType;
}

//! \bug review this for try-catch [CopyInto]
bool StdMultiplication::ForceUltimateType(const AbstractClass* const rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/2005
	if (MetaConcept::ForceUltimateType(rhs)) return true;
	if (   !rhs->SupportsThisOperation(StdMultiplication_MC)
		&& !rhs->SupportsThisOperation(StdAddition_MC))	// module action escape
		return false;
	if (ArgArray.empty())	// omnione; type is already compatible, this is a request to add information content.
		{
		MetaConcept* Tmp = NULL;
		if (rhs->CreateIdentityForOperation(Tmp,StdMultiplication_MC))
			{
			if (!FastInsertSlotAt(0,Tmp))
				{
				delete Tmp;
				return false;
				}
			InvokeEvalForceArg(0);
			return true;
			};
		if (rhs->CanCreateIdentityForOperation(StdMultiplication_MC))
			return false;
		}

	if (_DesiredType.empty())
		{
		try	{
			rhs->CopyInto(_DesiredType);
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		}
	else if (    _DesiredType->IntersectionWithIsNULLSet(*rhs)
			 || !_DesiredType->IntersectWith(*rhs))				// non-null desired type, *not* subclass
		return false;
	SUCCEED_OR_DIE(NULLSet!=*_DesiredType);

	if (!DetermineDynamicType())
		{
		_DesiredType.reset();
		return false;
		};
	return true;
}

void StdMultiplication::_forceStdForm()
{	//! \todo IMPLEMENT
	if (!ArgArray.empty() && !IdxCurrentSelfEvalRule && !IdxCurrentEvalRule)
		{
		ForceStdFormAux();
		if (StdMultInv_VF & MultiPurposeBitmap)
			{	// MultInv: try to clear this
			size_t i = fast_size();
			while(!ArgArray[--i]->IsUltimateType(NULL) && ArgArray[i]->UltimateType()->SupportsThisOperation(StdMultiplication_MC))
				if (0==i)
					{
					i = fast_size();
					do	{SUCCEED_OR_DIE(ArgArray[--i]->SelfInverse(StdMultiplication_MC));}
					while(0<i);
					while(i<fast_size()-i-1)
						{
						swap(ArgArray[i],ArgArray[fast_size()-i-1]);
						++i;
						};
					MultiPurposeBitmap ^= StdMultInv_VF;
					};
			}
		// use commutativity where reasonable (this is dependent on domain support)
		// This is insert sort; cannot do better due to noncommutativity issues
		size_t i = 1;
		while(i<fast_size())
			{
			if (   ArgArray[i-1]->CanCommuteUnderStdMultiplicationWith(*ArgArray[i])
				&& ArgArray[i]->InternalDataLT(*ArgArray[i-1]))
				{
				size_t j = i;
				do	swap(ArgArray[j-1],ArgArray[j]);
				while(    0< --j
					  &&  ArgArray[j-1]->CanCommuteUnderStdMultiplicationWith(*ArgArray[j])
					  &&  ArgArray[j]->InternalDataLT(*ArgArray[j-1]));
				}				
			++i;
			}
		CleanOnes();
		};
}

bool StdMultiplication::_IsExplicitConstant() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/24/2001
	if (ArgArray.empty()) return true;
	if (0>IdxCurrentSelfEvalRule)
		return and_range_n(boost::mem_fun(&MetaConcept::IsExplicitConstant),ArgArray.begin(),ArgArray.ArraySize());
	return false;
}

//  Evaluation functions
bool StdMultiplication::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	if (ArgArray.empty()) return true;
	// NOTE: this catches NULL entries
	if (SyntaxOKAux()) return DetermineDynamicType();		
	return false;
}

bool StdMultiplication::_IsOne() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	if (ArgArray.empty()) return true;
	if (!and_range_n(boost::mem_fun(&MetaConcept::IsOne),ArgArray.begin(),ArgArray.ArraySize()))
		return false;
	return DetermineDynamicType();
}

bool StdMultiplication::_IsZero() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	if (ArgArray.empty()) return false;
	return or_range_n(boost::mem_fun(&MetaConcept::IsZero),ArgArray.begin(),ArgArray.ArraySize());
}

// text I/O functions
bool StdMultiplication::SelfInverse(const ExactType_MC Operation)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/15/2004
	if (UltimateType()->SupportsThisOperation(Operation))
		{
		if      (StdAddition_MC==Operation)
			{	// whether it be a field or a module type (with leading coefficients), arg0 
				// should support StdAddition
			SUCCEED_OR_DIE(!ArgArray[0]->IsUltimateType(NULL) && UltimateType()->SupportsThisOperation(StdAddition_MC));
			if (ArgArray[0]->SelfInverse(StdAddition_MC))
				{
				_forceStdForm();
				return true;
				}
			return false;
			}
		else if (StdMultiplication_MC==Operation)
			{	// Remember, to properly invert we need *all* elements to support MultInv,
				// flip the order as we invert each one, then do a ForceStdForm.  This handles
				// non-commutative multiplications.
			size_t i = fast_size();
			do	if (ArgArray[--i]->IsUltimateType(NULL) || !ArgArray[i]->UltimateType()->SupportsThisOperation(StdMultiplication_MC))
					{
					MultiPurposeBitmap ^= StdMultInv_VF;
					return true;
					}
			while(0<i);
			i = fast_size();
			do	{SUCCEED_OR_DIE(ArgArray[--i]->SelfInverse(StdMultiplication_MC));}
			while(0<i);
			while(i<fast_size()-i-1)
				{
				swap(ArgArray[i],ArgArray[fast_size()-i-1]);
				++i;
				};
			_forceStdForm();
			return true;
			}
		}
	return MetaConcept::SelfInverse(Operation);
}

bool
StdMultiplication::SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const
{	//! \todo IMPLEMENT +-inv detection
	//! \todo IMPLEMENT *-inv detection
	return MetaConcept::SelfInverseTo(rhs,Operation);
}

static bool TestInfinity(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/24/2004
	if (rhs.IsInfinite())	// uninlined test for maintainability
		return true;
	return false;
}

static bool TestZeroAbsorbSign(MetaConcept& lhs,MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/13/2003
	if (rhs.IsZero()) return true;
	if (rhs.IsExplicitConstant())
		{
		if (rhs.IsNegative())
			{
			rhs.SelfInverse(StdAddition_MC);
			lhs.SelfInverse(StdAddition_MC);
			}
		return false;
		}
	//! \todo other absorbable arguments: definitely not zero, known sign
	return true;
}

bool
StdMultiplication::ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const
{
	if (ArgArray[ArgIdx]->IsZero())
		// Zero clears everything except Infinity
		return BlockableAnnihilatorScan(ArgIdx,SelfEvalRule,EvalRule,TestInfinity);
	else if (ArgArray[ArgIdx]->IsExactType(LinearInfinity_MC))
		// LinearInfinity clears everything except zero; reacts to sign of what it clears
		return BlockableAnnihilatorScan(ArgIdx,SelfEvalRule,EvalRule,TestZeroAbsorbSign);
	return false;
}

//  Helper functions for CanEvaluate... routines
void StdMultiplication::DiagnoseInferenceRules() const
{	//! \todo IMPLEMENT
	// #1: [Lang] a 0-ary product evaluates to a one (compatible with its context).  Franci uses
	// this to represent an 'omnione'.
	if (!ArgArray.empty())
		{	// #2: a 1-ary product evaluates to its sole argument
		if (1==fast_size())
			{
			InvokeEvalForceArg(0);
			return;
			}

		// Annihilator propagation.
		size_t i = fast_size();
		do	if (ThisIsAnnihilatorKey(--i,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
				{
				InferenceParameter1 = i;
				// if it's a zero but not reduced, use ForceZero code
				if (     ArgArray[i]->IsZero()
					//! \todo this test should be controlled by UltimateType()->RHSIsReducedZero(*ArgArray[i]);
					//! otherwise, matrix multiplication loses
#if 0
					&& (!ArgArray[i]->IsExplicitConstant() || ArgArray[i]->IsExactType(StdAddition_MC))
#endif
					&&  EvalForceArg_ER==IdxCurrentEvalRule)
					InvokeForceZero();
				return;
				}
		while(0<i);

		// IntegerNumeral block processing
		i = fast_size();
		do	if (   ArgArray[--i]->IsExactType(IntegerNumeral_MC)
				&& ArgArray[--i]->IsExactType(IntegerNumeral_MC)
				&& (    ArgArray[i]->IsUltimateType(ArgArray[i+1]->UltimateType())
					|| (0<i && ArgArray[i-1]->IsExactType(IntegerNumeral_MC))
					||  static_cast<IntegerNumeral*>(ArgArray[i])->NonTrivialGCF(*static_cast<IntegerNumeral*>(ArgArray[i+1]))))
				{
				InferenceParameter1 = i+2;	// strict upper bound for integer numeral block
				IdxCurrentSelfEvalRule = CleanIntegerNumeralBlock_SER;
				return;
				}
		while(1<i);

		//! \todo type-conserving scalar multiplication of dense intervals
		//! \todo condensation of multiplication into powers
		//! \todo MultInv cancellation
		//! \todo 'almost-MultInv' cancellation

		// MetaCode
		//! \todo modify this to reduce RAM faster rather than slower
		//! \todo want dual form to handle asymmetric cases where vertex is lower than target
		i = fast_size();
		do	if (ArgArray[--i]->StdMultCanRAMConserveDestructiveInteract())
				{
				size_t j = i;
				do	if (ArgArray[i]->StdMultCanRAMConserveDestructiveInteract(*ArgArray[--j],IdxCurrentSelfEvalRule))
					{
					InferenceParameter1 = i;
					InferenceParameter2 = j;
					return;
					}
				while(0<j);
				}
		while(1<i);

		//! \todo modify this to use RAM slower rather than faster
		//! \todo want dual form to handle asymmetric cases where vertex is lower than target
		i = fast_size();
		do	if (ArgArray[--i]->StdMultCanDestructiveInteract())
				{
				size_t j = i;
				do	if (ArgArray[i]->StdMultCanDestructiveInteract(*ArgArray[--j],IdxCurrentSelfEvalRule))
						{
						InferenceParameter1 = i;
						InferenceParameter2 = j;
						return;
						}
				while(0<j);
				}
		while(0<i);

		if (DiagnoseStandardEvalRules()) return;
		};
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

#undef FRANCI_WARY

bool StdMultiplication::InvokeEqualArgRule() const
{	//! \todo IMPLEMENT
	// This invokes powers processing.
	return false;
}

//! \todo related rule: if module action is driving the type, then it is possible for DesiredType
//! to affect the sole module-target variable.
bool StdMultiplication::DetermineDynamicType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2003
	DEBUG_LOG(__PRETTY_FUNCTION__);
	autodel_ptr<AbstractClass> TmpDynamicType;
	bool NULLTypesFound = false;
	size_t i = fast_size();
	// NOTE: Integers can be used to define + via their multiplication.  Even one arg in _C_
	// will require StdAddition support.  If n-1 args are known to be in _C_, then the nth arg 
	// need only support +!
	size_t ComplexCount = 0;
	size_t AddOnlyCount = 0;
	size_t MultiplyOnlyCount = 0;
	size_t BothAddMultiplyCount = 0;
	do	if (!ArgArray[--i]->IsUltimateType(NULL))
			{	// NOTE: need *does not support*, which is *not* the logical negation for proper classes!
			if (ArgArray[i]->UltimateType()->Subclass(Complex))
				ComplexCount++;
			else if (ArgArray[i]->UltimateType()->CompletelyFailsToSupportThisOperation(StdMultiplication_MC))
				{
				AUDIT_IF_RETURN(ArgArray[i]->UltimateType()->CompletelyFailsToSupportThisOperation(StdAddition_MC),false);
				AddOnlyCount++;
				}
			else if (ArgArray[i]->UltimateType()->CompletelyFailsToSupportThisOperation(StdAddition_MC))
				MultiplyOnlyCount++;
			else
				BothAddMultiplyCount++;
			}
		else{
			if (!ArgArray[i]->IsExactType(StdMultiplication_MC) || !static_cast<StdMultiplication*>(ArgArray[i])->ArgArray.empty())
				NULLTypesFound = true;
			else	// omnione
				BothAddMultiplyCount++;
			}
	while(0<i);
	DEBUG_LOG("past classification loop");
	// Here's how it works:
	// ComplexCount > 0: all args must support addition.
	// AddOnlyCount = 1: all other args must be Ring [at least support both + and *]
    // AddOnlyCount > 1: trouble!
    // MultiplyOnlyCount 1+: cannot allow any args within _C_ (or anything in which Z embeds 
	// 							injectively).  Finite fields OK [e.g., permutation groups]
	AUDIT_IF_RETURN(0<ComplexCount && 0<MultiplyOnlyCount,false);	// one _C_ arg forces all others to support +
	AUDIT_IF_RETURN(2<=AddOnlyCount,false);							// can't multiply two of these

	// Desired type crunch: if all args except one have domain in the desired type, force it
	i = fast_size();
	try	{
		do	if (!ArgArray[--i]->IsUltimateType(NULL))
				{
				ArgArray[i]->UltimateType()->CopyInto(TmpDynamicType);
				}
		while(0<i && TmpDynamicType.empty());
		DEBUG_LOG("past domain forcing");
		}
	catch(const bad_alloc&)
		{
		AUDIT_STATEMENT(return false);
		};

	while(0<i && !TmpDynamicType.empty())
		if (   !ArgArray[--i]->IsUltimateType(NULL)
			&& !TmpDynamicType->UnionWith(*ArgArray[i]->UltimateType()))
			TmpDynamicType=NULL;
	DEBUG_LOG("past dynamic type downgrade");

	AUDIT_IF_RETURN(TmpDynamicType.empty(),true);							// no information
	AUDIT_IF_RETURN(ClassMultiplicationDefined==*TmpDynamicType,true);	// no information
	if (!NULLTypesFound)
		{	// TmpDynamicType is an *upper* bound [under subset] on the type required.
		if (    _DesiredType.empty()
			||  _DesiredType->Subclass(*TmpDynamicType)	//! \todo FIX: this really should be "proper subobject under StdMultiplication"
			|| (   !TmpDynamicType->IntersectionWithIsNULLSet(*_DesiredType)
				&&  TmpDynamicType->IntersectWith(*_DesiredType)))		//! \todo FIX: should "construct object containing both types as subobject with StdMultiplication"
			{	// TmpDynamicType dictates DynamicType
			DEBUG_LOG("Known type");
			TmpDynamicType.MoveInto(_DynamicType);
			AUDIT_STATEMENT(return true);
			}
		AUDIT_STATEMENT(return false);
		}
	else{	// Type compatibility problems.  Franci doesn't have a decent upper bound, due to
			// extreme extensibility of her type system.
			//! \todo FIX: when the type system is sufficiently advanced, check for skeletal types
			//! (e.g., "n-d monoid/vector space with ill-specified domain").
		AUDIT_STATEMENT(return true);
		}
}

void StdMultiplication::CleanOnes()
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	// We can assume lexical order only for commutative multiplication.
	// Fortunately, 0 and 1 commute under multiplication, regardless.
	// First, remove omniones (which lack type information)
	size_t i = fast_size();
	size_t LastHit = fast_size();
	do	if (   ArgArray[--i]->IsExactType(StdMultiplication_MC)
			&& NULL==static_cast<StdMultiplication*>(ArgArray[i])->ArgArray)
			{
			DELETE_AND_NULL(ArgArray[i]);
			LastHit = i;
			}
	while(0<i);
	FlushNULLFromArray((MetaConcept**&)ArgArray,LastHit);

	if (1>=size()) return;

	i = fast_size();
	while(ArgArray[--i]->IsOne())
		if (0==i)
			{	// First, blot all ones of identical type
			if (*ArgArray[0]==*ArgArray[fast_size()-1])
				{
				InferenceParameter1 = 1;
				SelfEvalRuleCleanTrailingArg();
				return;
				};
			// at least two distinct types of one here.  Not likely to work.
			LastHit = fast_size();
			i = fast_size()-1;
			do	{
				--i;
				if (*ArgArray[i]==*ArgArray[i+1])
					{
					DELETE_AND_NULL(ArgArray[i+1]);
					LastHit = i+1;
					}
				}
			while(0<i);
			FlushNULLFromArray((MetaConcept**&)ArgArray,LastHit);
			if (DetermineDynamicType())
				{	// there's a viable dynamic type anyway(??)
				i = fast_size();
				do	if (   NULL!=ArgArray[--i]->UltimateType()
						&& *_DynamicType==*ArgArray[i]->UltimateType())
						{	// Dynamic type has matching 0 type, use it
						SwapArgs(0,i);
						InferenceParameter1 = 1;
						SelfEvalRuleCleanTrailingArg();
						return;
						}
				while(0<i);
				//! \todo FIX: more clever algorithms
				};
			return;
			};

	if (1+i<fast_size())
		{
		InferenceParameter1 = i+1;
		SelfEvalRuleCleanTrailingArg();
		};
	// At this point, ArgArray[Idx] is non-zero
	while(0<i)
		if (ArgArray[--i]->IsOne())
			FastDeleteIdx(i);
}

// StdAddition support
static bool ArgsAreIntegerNumeralPair(MetaConcept** ArgArray)
{
	if (   2==ArraySize(ArgArray)
		&& ArgArray[0]->IsExactType(IntegerNumeral_MC)
		&& ArgArray[1]->IsExactType(IntegerNumeral_MC))
		return true;
	return false;
}

static bool ArgsAreIntegerNumeralFraction(MetaConcept** ArgArray)
{
	if (   2==ArraySize(ArgArray)
		&& ArgArray[0]->IsExactType(IntegerNumeral_MC)
		&& ArgArray[1]->IsExactType(IntegerNumeral_MC)
		&& !ArgArray[0]->IsUltimateType(ArgArray[1]->UltimateType()))
		return true;
	return false;
}

static bool BothConstantAndNonConstantArgsForFactoring(MetaConcept** ArgArray)
{
	if (   ArgArray[0]->IsExplicitConstant()
		|| ArgArray[ArraySize(ArgArray)-1]->IsExplicitConstant())
		return !and_range_n(boost::mem_fun(&MetaConcept::IsExplicitConstant),ArgArray,ArraySize(ArgArray));
	return false;
}

//! \todo addition of (integer) fractions
//! check for: 2 2-ary products:
//!  a*(b^-1)+c*(d^-1)=(a*d+b*c)*(b^-1*d^-1)
//! start with: b,d IntegerNumerals of UltimateType _Q_, a, c vars or IntegerNumeral of type _Z_.
//! then use b or d hard-coded 1 (different patterns)
//! TODO: Integer math stunt: 0<a<c<d, 0<a<b
//! * a*b-c*d = (a*d+a*(b-d))-(a*d+(c-a)*d) = a*(b-d)-(c-a)*d
//! IF b,d are same sign with |b|>|d| and a,c are same sign with |c|>|a|, this is a 
//! RAM-safe manipulation.

//! \todo #1: reduce improper fractions against improper fractions or integers (ignore small addition results)
//! \todo #2: reduce products of integers against each other (ignore small products)
//! \todo #3: reduce products of integers against integers (ignore small products)

#if 0
bool
StdMultiplication::StdAddCanRAMConserveDestructiveInteract(void) const
{return false;}

bool
StdMultiplication::StdAddCanRAMConserveDestructiveInteract(const MetaConcept& Target, size_t& ActOnThisRule) const
{return false;}
#endif

bool StdMultiplication::StdAddCanDestructiveInteract() const
{
	// product of IntegerNumerals (1/Z form fine)
	if (   ArgsAreIntegerNumeralPair(ArgArray)
	// at least one constant, at least one non-constant arg
		|| BothConstantAndNonConstantArgsForFactoring(ArgArray))
		return true;
	return false;
}

bool
StdMultiplication::StdAddCanDestructiveInteract(const MetaConcept& Target, size_t& ActOnThisRule) const
{
	if 		(Target.IsExactType(StdMultiplication_MC))
		{
		const StdMultiplication& VR_Target = *reinterpret_cast<const StdMultiplication*>(&Target);
		if (	ArgsAreIntegerNumeralPair(ArgArray)
			&&	ArgsAreIntegerNumeralPair(VR_Target.ArgArray))
			{
			//! \todo #1: reduce improper fractions against improper fractions or integers
			//! \todo #2: reduce products of integers against each other
			//! \todo #3: reduce products of integers against integers

			// #4: if two fractions, add them
			if (	!ArgArray[0]->IsUltimateType(ArgArray[1]->UltimateType())
				&& 	!CanEvaluate()
				&&	!VR_Target.ArgArray[0]->IsUltimateType(VR_Target.ArgArray[1]->UltimateType())
				&& 	!VR_Target.CanEvaluate())
				{	// pair of stalled fractions
				ActOnThisRule = AddIntegerFractionToIntegerFraction_SER;
				return true;
				}
			}
		if (	BothConstantAndNonConstantArgsForFactoring(ArgArray)
			&&	BothConstantAndNonConstantArgsForFactoring(VR_Target.ArgArray))
			{
			if (AllNonConstArgsEqualUpToAddInv(VR_Target))
				{	// NOTE: above will be tripped by +-1 as implicit constant.
				ActOnThisRule = MultDistributesOverAdd_CondenseProductsOnNonConstArgs_SER;
				return true;
				}
			}
		}
	else if (Target.IsExactType(IntegerNumeral_MC))
		{
		if (   ArgsAreIntegerNumeralFraction(ArgArray)
			&& !CanEvaluate())
			{
			ActOnThisRule = AddIntegerToIntegerFraction_SER;
			return true;
			}
		}
	return false;
}

bool StdMultiplication::DelegateSelfEvaluate()
{
	assert(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER<IdxSelfCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER>=IdxCurrentSelfEvalRule);
	return (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1)])();
}

bool StdMultiplication::CleanIntegerNumeralBlock()
{	// TODO: IMPLEMENT
	// InferenceParameter1: StrictUB of highest block of IntegerNumerals
	size_t IntegerLB = InferenceParameter1-2;
	while(0<IntegerLB && ArgArray[IntegerLB-1]->IsExactType(IntegerNumeral_MC)) IntegerLB--;

	//! \todo may have to drastically rewrite this (currently inefficient)
	//! Four classes of interesting #'s:
	//!			_Z_	_Q_
	//!	Large
	//!	Small
	//!  Small/Small mismatch is O(1)
	//!  Small/Large mismatch is O(n) in digits of large
	//!  Large/Large mismatch is slow (GCF invokes), intermediate is nonconservative in general.
	//!	Small/Small same is O(1), may transform to Large
	//!  Small/Large same is O(n) in digits of large
	//!  Large/Large same is nominally O(n^2), but can be recursed to a more efficient O.

	do	{	// first stage: conservative evaluation
		size_t i = InferenceParameter1;
		do	{
			size_t j = --i;
			do	{
				static_cast<IntegerNumeral*>(ArgArray[--j])->RearrangeProduct(*static_cast<IntegerNumeral*>(ArgArray[i]));
				if (ArgArray[j]->IsOne())
					{
					DeleteIdx(j);
					if (1== --InferenceParameter1)
						return SelfEvalCleanEnd();
					--i;
					}
				else if (*static_cast<IntegerNumeral*>(ArgArray[j])==(signed short)(-1))
					{	// clear out -1, which would happen only as the first entry
					DeleteIdx(j);
					ArgArray[j]->SelfInverse(StdAddition_MC);
					if (1== --InferenceParameter1)
						return SelfEvalCleanEnd();
					--i;
					};
				if (ArgArray[i]->IsOne())
					{
					DeleteIdx(i);
					if (1== --InferenceParameter1)
						return SelfEvalCleanEnd();
					j = --i;
					};
				}
			while(IntegerLB<j);
			}
		while(IntegerLB+1<i);
		// second stage: non-conservative.
		i = InferenceParameter1;
		do	{
			size_t j = --i;
			do	if (static_cast<IntegerNumeral*>(ArgArray[--j])->ForceRearrangeProduct(*static_cast<IntegerNumeral*>(ArgArray[i])))
					{
					if (ArgArray[j]->IsOne())
						{
						DeleteIdx(j);
						if (1== --InferenceParameter1)
							return SelfEvalCleanEnd();
						--i;
						}
					else if (*static_cast<IntegerNumeral*>(ArgArray[j])==(signed short)(-1))
						{	// clear out -1, which would happen only as the first entry
						DeleteIdx(j);
						ArgArray[j]->SelfInverse(StdAddition_MC);
						if (1== --InferenceParameter1)
							return SelfEvalCleanEnd();
						--i;
						};
					if (ArgArray[i]->IsOne())
						{
						DeleteIdx(i);
						if (1== --InferenceParameter1)
							return SelfEvalCleanEnd();
						j = --i;
						};
					}
			while(IntegerLB<j);
			}
		while(IntegerLB+1<i);
		if (2<IntegerLB)
			{
			i = IntegerLB-1;
			do	if (   ArgArray[--i]->IsExactType(IntegerNumeral_MC)
					&& ArgArray[--i]->IsExactType(IntegerNumeral_MC))
					{
					InferenceParameter1 = i+2;	// strict upper bound for integer numeral block
					IntegerLB = i;
					while(0<IntegerLB && ArgArray[IntegerLB-1]->IsExactType(IntegerNumeral_MC)) IntegerLB--;
					goto Restart;
					}
			while(1<i);
			};
		InferenceParameter1 = 1;
Restart:;
		}
	while(1<InferenceParameter1);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}
