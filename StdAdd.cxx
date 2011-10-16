// StdAdd.cxx
// definition of StdAddition, an n-ary commutative operation defined for a variety of sets
// StdAddition is assumed to endow the set it operates on with an abelian group structure

#include "Class.hxx"
#include "Integer1.hxx"
#include "StdAdd.hxx"
#include "LowRel.hxx"

#include <boost/functional.hpp>

StdAddition::SelfEvaluateRule StdAddition::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&StdAddition::CleanAddInv,
	&StdAddition::CleanIntegerNumeralBlock,
	&StdAddition::EqualArgsToIntegerProduct
	};

//! \todo AbstractClass must:	know what a (commutative) ring is (not yet);
//!								return the standard addition for a set as a ring(?) (not yet);
//!								know if a presented operation is compatible with a known ring structure (not yet)
//! \todo StdAddition compatible types must tolerate + to finite integer powers, both unsigned long and Integer

StdAddition::StdAddition(MetaConcept**& NewArgList)
:	MetaConceptWithArgArray(StdAddition_MC,NewArgList)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/10/2000
	if (SyntaxOK()) _forceStdForm();
}

const StdAddition& StdAddition::operator=(const StdAddition& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	autoval_ptr<AbstractClass> Tmp1(src.DesiredType);
	autoval_ptr<AbstractClass> Tmp2(src.DynamicType);

	MetaConceptWithArgArray::operator=(src);
	Tmp1.MoveInto(DesiredType);
	Tmp2.MoveInto(DynamicType);
	return *this;
}

void StdAddition::MoveInto(StdAddition*& dest)		// can throw memory failure.  If it succeeds, it destroys the source.
{	// FORMALLY CORRECT: Kenneth Boyd, 3/28/2002
	if (!dest) dest = new StdAddition();

	MoveIntoAux(*dest);
	DesiredType.MoveInto(dest->DesiredType);
	DynamicType.MoveInto(dest->DynamicType);
}

//  Type ID functions
// NOTE: StdAddition UltimateType is non-NULL
const AbstractClass* StdAddition::UltimateType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2003
	if (!DesiredType.empty()) return DesiredType;
	if (DynamicType.empty())
		{
		DetermineDynamicType();
		if (DynamicType.empty()) return &ClassAdditionDefined;
		};
	return DynamicType;
}

bool StdAddition::ForceUltimateType(const AbstractClass* const rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/2005
	if (MetaConcept::ForceUltimateType(rhs)) return true;
	if (!rhs->SupportsThisOperation(StdAddition_MC)) return false;
	if (ArgArray.empty())	// omnizero; type is already compatible, this is a request to add information content.
		{
		MetaConcept* Tmp = NULL;
		if (rhs->CreateIdentityForOperation(Tmp,StdAddition_MC))
			{
			if (!FastInsertSlotAt(0,Tmp))
				{
				delete Tmp;
				return false;
				}
			InvokeEvalForceArg(0);
			return true;
			};
		if (rhs->CanCreateIdentityForOperation(StdAddition_MC))
			return false;
		}

	if (DesiredType.empty())
		{
		try	{
			rhs->CopyInto(DesiredType);
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		}
	else if (    DesiredType->IntersectionWithIsNULLSet(*rhs)
			 || !DesiredType->IntersectWith(*rhs))				// non-null desired type, *not* subclass
		return false;
	SUCCEED_OR_DIE(NULLSet!=*DesiredType);

	// want ALLEQUAL(1,2+X) to autotype X as _Z_
	if (!DesiredType.empty() && DesiredType->IsProperSet())
		{
		size_t ImproperDomainCount = 0;
		size_t LowestFailure = 0;
		size_t i = fast_size();
		do	if (    ArgArray[--i]->IsUltimateType(NULL)
				|| !ArgArray[i]->UltimateType()->Subclass(*DesiredType))
				{
				ImproperDomainCount++;
				LowestFailure = i;
				}
		while(0<i);
		if (1==ImproperDomainCount)
			{
			if (!ArgArray[LowestFailure]->ForceUltimateType(DesiredType))
				return false;
			};
		}

	if (!DetermineDynamicType())
		{
		DesiredType.reset();
		return false;
		};
	return true;
}

void StdAddition::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/25/2001
	if (!ArgArray.empty() && !IdxCurrentSelfEvalRule && !IdxCurrentEvalRule)
		{
		ForceStdFormAux();
		ForceTotalLexicalArgOrder();
		CleanZeros();
		};
}

bool StdAddition::_IsExplicitConstant() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2000
	if (ArgArray.empty()) return true;
	if (0>IdxCurrentSelfEvalRule)
		return and_range_n(boost::mem_fun(&MetaConcept::IsExplicitConstant),ArgArray.begin(),ArgArray.ArraySize());
	return false;
}

//  Evaluation functions
bool StdAddition::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/8/2000
	// [Lang] a 0-ary sum evaluates to 0 ['omnizero'], so it *is* defined.
	// This must be caught before SyntaxOKAux.
	if (ArgArray.empty()) return true;
	// NOTE: this catches NULL entries
	if (SyntaxOKAux()) return DetermineDynamicType();
	return false;
}

bool StdAddition::_IsZero() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/12/2000
	if (ArgArray.empty()) return true;
	if (!and_range_n(boost::mem_fun(&MetaConcept::IsZero),ArgArray.begin(),ArgArray.ArraySize()))
		return false;
	return DetermineDynamicType();
}

// text I/O functions
bool
StdAddition::SelfInverse(const ExactType_MC Operation)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
//! \todo the text-formatter routine is the one that clips __+-__ to __-__
//! \todo IMPLEMENT *-inv detection
	if (StdAddition_MC==Operation)
		{
		if (!ArgArray.empty())
			{
			size_t i = fast_size();
			do	if (!ArgArray[--i]->SelfInverse(StdAddition_MC))
					{
					while(fast_size()> ++i)
						ArgArray[i]->SelfInverse(StdAddition_MC);
					return false;
					}
			while(0<i);
			_forceStdForm();
			};
		return true;
		};
	return MetaConcept::SelfInverse(Operation);
}

bool StdAddition::SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/16/2000
//! \todo IMPLEMENT *-inv detection
	if (    rhs.IsExactType(StdAddition_MC)
		&&  size()==rhs.size()
		&& *UltimateType()==*rhs.UltimateType())
		{
		if (StdAddition_MC==Operation)
			{
			if (   NULL==ArgArray	// omnizero is additive inverse of omnizero
				|| OrderIndependentPairwiseRelation(static_cast<const StdAddition&>(rhs),IsStdAdditionInverseTo))
				return true;
			}
		return MetaConcept::SelfInverseTo(rhs,Operation);
		};
	return false;
}

static bool TestOppSignLinearInfinity(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/13/2003
	if (   rhs.IsExactType(LinearInfinity_MC)
		&& lhs.IsPositive()!=rhs.IsPositive())
		return true;
	return false;
}

bool
StdAddition::ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/13/2003
	if (ArgArray[ArgIdx]->IsExactType(LinearInfinity_MC))
		// LinearInfinity clears everything except the AddInv version of itself
		return BlockableAnnihilatorScan(ArgIdx,SelfEvalRule,EvalRule,TestOppSignLinearInfinity);
	return false;
}

//  Helper functions for CanEvaluate... routines
void StdAddition::DiagnoseInferenceRules() const
{	// FORMALLY CORRECT: 1/17/2000
	// #1: [Lang] a 0-ary sum evaluates to a zero (compatible with its context).  Franci uses
	// this to represent an 'omnizero'.
	if (!ArgArray.empty())
		{
		// #2: a 1-ary sum evaluates to its sole argument
		if (1==fast_size())
			{
			if (StdMultInv_VF & MultiPurposeBitmap) 
				{
				SUCCEED_OR_DIE(ArgArray[0]->SelfInverse(StdMultiplication_MC));
				*const_cast<unsigned char*>(&MultiPurposeBitmap) ^= StdMultInv_VF;
				}
			InvokeEvalForceArg(0);
			return;
			}

		{
		//! \todo +- LinearInfinity_MC cancels everything in reals except opp-sign version
		//! this calls for an abstracted annihilator handling
		size_t i = fast_size();
		do	if (ThisIsAnnihilatorKey(--i,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
				{
				InferenceParameter1 = i;
				return;
				}
		while(0<i);
		}

		// IntegerNumeral block processing.
		// Standard form: IntegerNumeral is the *lowest* type that is syntactically OK in 
		// StdAddition, and there are no explicit zeros.  Arity is at least two now.
		if (ArgArray[1]->IsExactType(IntegerNumeral_MC))
			{
			if (    ArgArray[0]->IsUltimateType(ArgArray[1]->UltimateType())
				|| (3<=fast_size() && ArgArray[2]->IsExactType(IntegerNumeral_MC))
				|| (   ArgArray[0]->IsPositive()!=ArgArray[1]->IsPositive()
					&& (   static_cast<IntegerNumeral*>(ArgArray[0])->AbsValIsOneOverN(2)
						|| static_cast<IntegerNumeral*>(ArgArray[1])->AbsValIsOneOverN(2))))
				{
				IdxCurrentSelfEvalRule = CleanIntegerNumeralBlock_SER;
				return;
				}
			};

		// AddInv cancellation
		if (FindTwoRelatedArgs(IsStdAdditionInverseTo))
			{	// NOTE: InferenceParameter1, InferenceParameter2 already set
			IdxCurrentSelfEvalRule = CleanAddInv_SER;
			return;
			};

		// Equal args converts to integer product
		if (DiagnoseEqualArgs()) return;

		//! \todo type-conserving scalar addition of intervals

		//! \todo distributivity of * over +
		//! <br>important case here: a*b+a*c |-> a*(b+c)
		//! <br>note: this is non-deterministic (there may be more than one useful viewpoint var) at arity 3+
		//! <br>If a is explicit constant, handled in metacode

		// MetaCode
		//! \todo modify this to reduce RAM faster rather than slower
		//! \todo want dual form to handle asymmetric cases where vertex is lower than target
		size_t i = fast_size();
		do	if (ArgArray[--i]->StdAddCanRAMConserveDestructiveInteract())
				{
				size_t j = i;
				do	if (ArgArray[i]->StdAddCanRAMConserveDestructiveInteract(*ArgArray[--j],IdxCurrentSelfEvalRule))
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
		do	if (ArgArray[--i]->StdAddCanDestructiveInteract())
				{
				size_t j = i;
				do	if (ArgArray[i]->StdAddCanDestructiveInteract(*ArgArray[--j],IdxCurrentSelfEvalRule))
						{
						InferenceParameter1 = i;
						InferenceParameter2 = j;
						return;
						}
				while(0<j);
				}
		while(0<i);

		if (   DiagnoseEvaluatableArgs()
			|| DiagnoseSelfAssociativeArgs())
			return;
		};
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

bool StdAddition::DetermineDynamicType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/23/2003
//! \todo IMPLEMENT: New AbstractClass variant of Subclass, etc.: specifies an operation that must extend....
	autodel_ptr<AbstractClass> TmpDynamicType;
	bool NULLTypesFound = false;
	size_t i = fast_size();
	do	if (NULL!=ArgArray[--i]->UltimateType())
			{
			if (!ArgArray[i]->UltimateType()->SupportsThisOperation(StdAddition_MC))
				return false;
			if (   NULL==TmpDynamicType
				|| TmpDynamicType->ProperSubclass(*ArgArray[i]->UltimateType()))	//! \todo this really should be "proper subobject under StdAddition"
				ArgArray[i]->UltimateType()->CopyInto(TmpDynamicType);
			else if (!TmpDynamicType->Superclass(*ArgArray[i]->UltimateType()))
				{	//! \todo should "construct object containing both types as subobject with StdAddition"
				return false;
				}
			}
		else{	// we must ignore omnizeros
			if (!ArgArray[i]->IsExactType(StdAddition_MC) || !static_cast<StdAddition*>(ArgArray[i])->ArgArray.empty())
				NULLTypesFound = true;
			}
	while(0<i);

	// Desired type crunch: if all args except one have domain in the desired type, force it
	i = fast_size();
	try	{
		do	if (!ArgArray[--i]->IsUltimateType(NULL))
				{
				ArgArray[i]->UltimateType()->CopyInto(TmpDynamicType);
				}
		while(0<i && TmpDynamicType.empty());
		}
	catch(const bad_alloc&)
		{
		return false;
		};

	while(0<i && !TmpDynamicType.empty())
		if (   !ArgArray[--i]->IsUltimateType(NULL)
			&& !TmpDynamicType->UnionWith(*ArgArray[i]->UltimateType()))
			TmpDynamicType.reset();

	if (TmpDynamicType.empty()) return true;
	if (ClassAdditionDefined==*TmpDynamicType) return true;	// no information
	if (!NULLTypesFound)
		{	// TmpDynamicType is an *upper* bound [under subset] on the type required.
		if (   DesiredType.empty()
			|| DesiredType->Subclass(*TmpDynamicType)		//! \todo this really should be "proper subobject under StdAddition"
			|| (   !TmpDynamicType->IntersectionWithIsNULLSet(*DesiredType)
				&&  TmpDynamicType->IntersectWith(*DesiredType)))		//! \todo should "construct object containing both types as subobject with StdAddition"
			{	// TmpDynamicType dictates DynamicType
			TmpDynamicType.MoveInto(DynamicType);
			return true;
			}
		return false;
		}
	else{	// Type compatibility problems.  Franci doesn't have a decent upper bound, due to
			// extreme extensibility of her type system.
			//! \todo when the type system is sufficiently advanced, check for skeletal types
			//! (e.g., "n-d monoid/vector space with ill-specified domain").
		return true;
		}
}

bool StdAddition::InvokeEqualArgRule() const
{	// FORMALLY CORRECT: 10/13/2004
	// This can be interpreted as (unsigned long)*__: there should be specialized processing.
	// something for unsigned long powers of + -- along with a member that reports the RAM-safe
	// limit for unsigned long powers, and "risk level 1" for unsigned long powers.
	// NOTE: InferenceParameter1, InferenceParameter2 point to args known to be equal
	size_t EqualCount = 0;
	size_t i = fast_size();
	size_t LastHit = InferenceParameter1;
	do	if (   InferenceParameter1== --i
			|| *ArgArray[InferenceParameter1]==*ArgArray[i])
			{
			EqualCount++;
			LastHit = i;
			}
	while(0<i);
	assert(2<=EqualCount);
	InferenceParameter1 = LastHit;
	InferenceParameter2 = EqualCount;
	IdxCurrentSelfEvalRule = EqualArgsToIntegerProduct_SER;
	return true;
}

void StdAddition::CleanZeros()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/7/2001
	// Franci has already forced total lexical order
	// First, remove omnizeros (which lack type information)
	size_t i = fast_size();
	size_t LastHit = fast_size();
	do	if (   ArgArray[--i]->IsExactType(StdAddition_MC)
			&& NULL==static_cast<StdAddition*>(ArgArray[i])->ArgArray)
			{
			DELETE_AND_NULL(ArgArray[i]);
			LastHit = i;
			}
	while(0<i && StdAddition_MC<=ArgArray[i-1]->ExactType());
	FlushNULLFromArray((MetaConcept**&)ArgArray,LastHit);

	if (1>=size()) return;

	i = fast_size();
	while(ArgArray[--i]->IsZero())
		if (0==i)
			{
			// First, blot all zeros of identical type
			if (*ArgArray[0]==*ArgArray[fast_size()-1])
				{
				InferenceParameter1 = 1;
				SelfEvalRuleCleanTrailingArg();
				return;
				};
			// at least two distinct types of zero here.  Not likely to work.
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
						&& *DynamicType==*ArgArray[i]->UltimateType())
						{	// Dynamic type has matching 0 type, use it
						SwapArgs(0,i);
						InferenceParameter1 = 1;
						SelfEvalRuleCleanTrailingArg();
						return;
						}
				while(0<i);
				//! \todo more clever algorithms
				};
			return;
			};

	if (1+i<fast_size())
		{
		InferenceParameter1 = i+1;
		SelfEvalRuleCleanTrailingArg();
		};
	// At this point, ArgArray[i] is non-zero
	while(0<i) if (ArgArray[--i]->IsZero()) FastDeleteIdx(i);
}

bool StdAddition::DelegateSelfEvaluate()
{
	assert(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER<IdxSelfCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER>=IdxCurrentSelfEvalRule);
	return (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1)])();
}

bool StdAddition::CleanAddInv()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/15/2000
	// InferenceParameter1, InferenceParameter2 point to additive inverses; 
	// InferenceParameter1>InferenceParameter2
	if (2==fast_size())
		{	// must guarantee correct zero type; get this from the UltimateType
		MetaConcept* Tmp = NULL;
		// NULL==UltimateType doesn't happen [Franci should default to a suitable proper class 
		// instead]
		if (!ArgArray[1]->UltimateType()->CreateIdentityForOperation(Tmp,StdAddition_MC))
			return false;
		FastDeleteIdx(1);
		if (!Tmp)	// intended zero is an omnizero; this is a StdAddition object with no args
			{
			FastDeleteIdx(0);
			// It's an omnizero: nothing to evaluate.  Short-circuit the next rule determination.
			IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
			return true;
			};
		delete ArgArray[0];
		ArgArray[0]=Tmp;
		// It's arity 1...short-circuit the next rule determination
		InvokeEvalForceArg(0);
		return true;
		};
	DeleteIdx(InferenceParameter1);
	DeleteIdx(InferenceParameter2);

	//! \todo OPTIMIZE: SPEED accleration consideration: the general case of the rule doesn't
	//! invalidate the preconditions (except for arity)
	//! \todo OPTIMIZE: SPEED in practice, does Franci want to clear out *lots* of additive 
	//! inverse pairs?

	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

void MetaConceptWithArgArray::InvokeForceZero() const
{
	assert(UltimateType()->SupportsThisOperation(StdAddition_MC));
	SUCCEED_OR_DIE(UltimateType()->CreateIdentityForOperation(InferenceParameterMC,StdAddition_MC));
	
	//! \todo IMPROVED ERROR HANDLING
	//! <br>CreateIdentityForOperation returns false when RAM fails.
	//! There should be pro-active reduction rules involved (somewhere).  These might 
	//! make the autotyping sharper.

	if (NULL==InferenceParameterMC)
		{	// need explicit omnizero
		InferenceParameterMC = new(nothrow) StdAddition();
		if (NULL==InferenceParameterMC) UnconditionalRAMFailure();
		}

	assert(InferenceParameterMC->SyntaxOK());
	IdxCurrentEvalRule = ForceValue_ER;
}

bool
MetaConceptWithArgArray::AllNonConstArgsEqualUpToAddInv(const MetaConceptWithArgArray& rhs) const
{	// FORMALLY CORRECT: 10/13/2004
	const size_t ArityArg1 = fast_size();
	const size_t ArityArg2 = rhs.fast_size();
	size_t TestIdxArg1 = 0;
	size_t TestIdxArg2 = 0;
	size_t Offset=0;
	signed long OppSignToggle = 1;	// range: 1, -1
	// NOTE: due to issues with commutativity of multiplication, Franci relies on multiplicands 
	// commuting to a standard order.  The controlling function AllNonConstArgsEqualUpToAddInv
	// for the rule MultDistributesOverAdd_CondenseProductsOnNonConstArgs
	// must insure that the constant args to be distributed against are all at one side or 
	// the other.
	// For now, the algorithm is just interested in products over the complex numbers.  This
	// means we only need to worry about constants piling up on the left. [This may mean
	// rerigging the standard form code at some point.]
	// The testing code should probably pass the useful parameters to this function:
	// length of the block to be factored
	// OppSignToggle

	while(ArityArg1>TestIdxArg1 && ArgArray[TestIdxArg1]->IsExplicitConstant()) TestIdxArg1++;
	while(ArityArg2>TestIdxArg2 && rhs.ArgArray[TestIdxArg2]->IsExplicitConstant()) TestIdxArg2++;
	if (   ArityArg1-TestIdxArg1!=ArityArg2-TestIdxArg2
		|| ArityArg1==TestIdxArg1)
		return false;
	while(ArityArg1>TestIdxArg1+Offset)
		{
		if      (*ArgArray[TestIdxArg1+Offset]==*rhs.ArgArray[TestIdxArg2+Offset])
			Offset++;
		else if (ArgArray[TestIdxArg1+Offset]->SelfInverseTo(*rhs.ArgArray[TestIdxArg2+Offset], StdAddition_MC))
			{
			Offset++;
			OppSignToggle = -OppSignToggle;
			}
		else
			return false;
		}
	InferenceParameter1 = ArityArg1-TestIdxArg1;
	InferenceParameter2 = OppSignToggle;
	rhs.InferenceParameter1 = ArityArg1-TestIdxArg1;
	rhs.InferenceParameter2 = OppSignToggle;
	return true;

	//! \todo if we need to apply distributivity from the right, it will have to be 
	//! signaled with a negative CommonBlockArity.
}

