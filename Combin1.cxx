// Combin1.cxx
// Implementation of CombinatorialLike class, which implements
// * Factorial(K): factorial function.  UltimateType 1...infinity
// * Gamma(K): Gamma function,  UltimateType R\{-infinity...0}.  May do this one later.
// * Perm(N,K): counts permutations.  UltimateType 1...infinity
// * Comb(N,K): counts combinations.  UltimateType 1...infinity
// * Multi(N,K,J,...): multinomial combinations.  UltimateType 1...infinity

//! \todo I/O: 2-ary StdMult fractions should be a/b, not a&middot;b<sup>-1</sup> when
//! domain permits.  [Problem is any abstract object that admits a quotient interpretation.]

//! \todo IMPLEMENT ALGEBRAIC REDUCTIONS
//! <br>StdAddition support
//!	<br>virtual bool StdAddCanRAMConserveDestructiveInteract(void) const;
//!	<br>virtual bool StdAddCanRAMConserveDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const;
//!	<br>virtual bool StdAddCanDestructiveInteract(void) const;
//!	<br>virtual bool StdAddCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const;

//! <br>StdMultiplication support
//!	<br>virtual bool StdMultCanRAMConserveDestructiveInteract(void) const;
//!	<br>virtual bool StdMultCanRAMConserveDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const;
//!	<br>virtual bool StdMultCanDestructiveInteract(void) const;
//!	<br>virtual bool StdMultCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const;

//! <br>At some point, we'll want to store Unary Predicates with the affected variables 
//! in a dedicated AND clause.  We will then need EqualRelation, etc. to apply these constraints
//! to themselves.  This may actually be a top-level UnaryConstraints object that acts like 
//! a not-so-manipulatable AND.  QuantifiedStatement may have to do some hairy things to the output.

//! <br>Should augment "clearly not equal" to handle FACTORIAL(M),PERM(M,N)
//! <br>New Interface: IsMultiplicativeUnit(),IsNotMultiplicativeUnit(); allow IsMultiplicativeUnit when appropriate
//! <br>* if the type is a subset of the real numbers, this is a test for +-1 i.e. absolute value 1.
//! <br>following is auto from augmented type system that can handle inequalities
//! <br>ALLEQUAL(0,FACTORIAL(M)) => FALSE
//! <br>ALLEQUAL(0,PERM(M,N)) => FALSE
//! <br>in general, should be able to do fast checks without evaluating...

//! \todo Use AbstractClass' ConstructUpwardTopologicalRay to implement proper typechecking for: FACTORIAL, COMB, PERM, GCF

#include "Combin1.hxx"

#include "Class.hxx"
#include "TruthVal.hxx"
#include "Integer1.hxx"
#include "StdMult.hxx"
#include "LowRel.hxx"
#include "Keyword1.hxx"

#include "Zaimoni.STL/lite_alg.hpp"

CombinatorialLike::EvaluateToOtherRule CombinatorialLike::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
  =	{
	&CombinatorialLike::FactorialPartialEvaluate,
	&CombinatorialLike::PermutationCountPartialEvaluate,
	&CombinatorialLike::CombinationCountPartialEvaluate
	};

CombinatorialLike::SelfEvaluateRule CombinatorialLike::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&CombinatorialLike::RetypePermutationCountAsFactorial
	};


CombinatorialLike::CombinatorialLike(MetaConcept**& NewArgList, CombinatorialModes LinkageType)
:	MetaConceptWithArgArray((ExactType_MC)(LinkageType+Factorial_MC),NewArgList)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/12/2003
	if (SyntaxOK()) ForceStdForm();
}

void CombinatorialLike::MoveInto(CombinatorialLike*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/12/2003
	if (!dest)
		dest = new CombinatorialLike((CombinatorialModes)(ExactType()-Factorial_MC));

	MoveIntoAux(*dest);
}

void CombinatorialLike::_forceStdForm()
{	//! \todo IMPLEMENT
}

//  Type ID functions
const AbstractClass* CombinatorialLike::UltimateType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/25/2003
#if 0
	if (IsExactType(GammaFunction_MC))
		{
		return &Real;	// should be _R_\{0} (check references)
		}
#endif
	return (IsMetaMultInverted()) ? &Rational : &Integer;
}

bool CombinatorialLike::SyntaxOK() const
{	//! \todo IMPLEMENT
	if (SyntaxOKAux())
		{
		if (IsExactType(Factorial_MC))
			{
			if (1!=fast_size() || ArgArray[0]->IsNegative())
				return false;

			if (ArgArray[0]->IsExactType(IntegerNumeral_MC))
				return ArgArray[0]->IsUltimateType(&Integer);

			// FACTORIAL(&infin;) *does* make sense: it's a short-circuit limit!
			if (ArgArray[0]->IsExactType(LinearInfinity_MC))
				return true;

			return ArgArray[0]->ForceUltimateType(&Integer);
			};
		if (IsExactType(PermutationCount_MC) || IsExactType(CombinationCount_MC))
			{
			if (	2!=fast_size()
				||	ArgArray[0]->IsNegative()
				|| !ArgArray[0]->ForceUltimateType(&Integer)
				|| 	ArgArray[1]->IsNegative()
				||  ArgArray[1]->IsExactType(LinearInfinity_MC)
				|| !ArgArray[1]->ForceUltimateType(&Integer))
				return false;

			// Limit behavior OK...
			if (ArgArray[0]->IsExactType(LinearInfinity_MC))
				return true;

			// Algebraic constraint: first arg>=second arg
			// TODO: implement above when partial ordering implemented
			if (   ArgArray[0]->IsExactType(IntegerNumeral_MC)
				&& ArgArray[1]->IsExactType(IntegerNumeral_MC))
				return *static_cast<IntegerNumeral*>(ArgArray[0])>=*static_cast<IntegerNumeral*>(ArgArray[1]);

			// TODO: expensive...use infimum as proxy for whether to do adjustment
			if (ArgArray[1]->IsExactType(IntegerNumeral_MC))
				{	// ForceUltimateType ArgArray[1]...infinity on ArgArray[0]
				AbstractClass* Domain = NULL;
				Integer.ConstructUpwardTopologicalRay(*ArgArray[1],false,Domain);
				const bool result = ArgArray[0]->ForceUltimateType(Domain);
				delete Domain;
//				Domain = NULL;
				return result;
				}
			return true;
			}
		}
	return false;
}

// text I/O functions
size_t
CombinatorialLike::LengthOfSelfName(void) const
{	//! \todo implement other modes
	//! \todo move to LenName.cxx
	if (IsExactType(Factorial_MC))
		{
		const size_t BaseLen = strlen(PrefixKeyword_FACTORIAL)+2;
		if (NULL==ArgArray || NULL==ArgArray[0]) return BaseLen;
		return BaseLen+ArgArray[0]->LengthOfSelfName();
		}
	else if (IsExactType(PermutationCount_MC))
		{
		const size_t BaseLen = strlen(PrefixKeyword_PERMUTATION)+3;
		if (ArgArray.empty()) return BaseLen;
		if (1>=fast_size() || NULL==ArgArray[1])
			{
			if (NULL==ArgArray[0]) return BaseLen;
			return BaseLen+ArgArray[0]->LengthOfSelfName();
			}
		else if (NULL==ArgArray[0])
			{
			return BaseLen+ArgArray[1]->LengthOfSelfName();
			};
		return BaseLen+ArgArray[0]->LengthOfSelfName()+ArgArray[1]->LengthOfSelfName();
		}
	else if (IsExactType(CombinationCount_MC))
		{
		const size_t BaseLen = strlen(PrefixKeyword_COMBINATION)+3;
		if (ArgArray.empty()) return BaseLen;
		if (1>=fast_size() || NULL==ArgArray[1])
			{
			if (NULL==ArgArray[0]) return BaseLen;
			return BaseLen+ArgArray[0]->LengthOfSelfName();
			}
		else if (NULL==ArgArray[0])
			{
			return BaseLen+ArgArray[1]->LengthOfSelfName();
			};
		return BaseLen+ArgArray[0]->LengthOfSelfName()+ArgArray[1]->LengthOfSelfName();
		}
	UnconditionalDataIntegrityFailure();
}

void CombinatorialLike::ConstructSelfNameAux(char* Name) const
{	//! \todo implement other modes
	//! \todo move to LenName.cxx
	if (IsExactType(Factorial_MC))
		{
		strcpy(Name,PrefixKeyword_FACTORIAL);
		Name += strlen(PrefixKeyword_FACTORIAL);
		*Name++='(';

		if (!ArgArray.empty() && NULL!=ArgArray[0])
			{
			ArgArray[0]->ConstructSelfName(Name);
			Name += ArgArray[0]->LengthOfSelfName();
			}

		Name[0]=')';
		return;
		}
	else if (IsExactType(PermutationCount_MC))
		{
		strcpy(Name,PrefixKeyword_PERMUTATION);
		Name += strlen(PrefixKeyword_PERMUTATION);
		*Name++='(';

		if (!ArgArray.empty() && NULL!=ArgArray[0])
			{
			ArgArray[0]->ConstructSelfName(Name);
			Name += ArgArray[0]->LengthOfSelfName();
			}

		*Name++=',';

		if (1<size() && NULL!=ArgArray[1])
			{
			ArgArray[1]->ConstructSelfName(Name);
			Name += ArgArray[1]->LengthOfSelfName();
			}

		Name[0]=')';
		return;
		}
	else if (IsExactType(CombinationCount_MC))
		{
		strcpy(Name,PrefixKeyword_COMBINATION);
		Name += strlen(PrefixKeyword_COMBINATION);
		*Name++='(';

		if (!ArgArray.empty() && NULL!=ArgArray[0])
			{
			ArgArray[0]->ConstructSelfName(Name);
			Name += ArgArray[0]->LengthOfSelfName();
			}

		*Name++=',';

		if (1<size() && NULL!=ArgArray[1])
			{
			ArgArray[1]->ConstructSelfName(Name);
			Name += ArgArray[1]->LengthOfSelfName();
			}

		Name[0]=')';
		return;
		}
}

bool CombinatorialLike::_IsOne() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/26/2003
	if (IsPositive())
		{
		if (EvalForceArg_ER == IdxCurrentEvalRule)
			return ArgArray[InferenceParameter1]->IsOne();
		else if (IsExactType(Factorial_MC))
			return ArgArray[0]->IsZero() || ArgArray[0]->IsOne();
		else if (IsExactType(PermutationCount_MC))
			return ArgArray[1]->IsZero();
		else if (IsExactType(CombinationCount_MC))
			return ArgArray[1]->IsZero() || *ArgArray[0]==*ArgArray[1];
		}
	return false;
}


bool CombinatorialLike::IsPositive() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/25/2003
#if 0
	if (IsExactType(GammaFunction_MC))
		{
		}
#endif
	return !IsMetaAddInverted();
}

bool CombinatorialLike::IsNegative() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/25/2003
#if 0
	if (IsExactType(GammaFunction_MC))
		{
		}
#endif
	return IsMetaAddInverted();
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > CombinatorialLike::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

//  Helper functions for CanEvaluate... routines
void CombinatorialLike::DiagnoseInferenceRules() const
{	//! \todo IMPLEMENT
	if (IsExactType(Factorial_MC))
		{
		if (DiagnoseEvaluatableArgs()) return;
		if (ArgArray[0]->IsExactType(IntegerNumeral_MC))
			{	// split off the largest integer, and decrement
				// do it faster if we can
			IdxCurrentEvalRule=FactorialPartialEvaluate_ER;	// interacts with StdMultiplication
			return;
			}
		if (ArgArray[0]->IsExactType(LinearInfinity_MC))
			{	// limit: FACTORIAL(&infin;) is &infin;
			InvokeEvalForceArg(0);
			return;
			}
		};

	if (DiagnoseEqualArgs()) return;

	if (IsExactType(PermutationCount_MC))
		{
		if (   ArgArray[0]->IsExactType(LinearInfinity_MC)
			|| ArgArray[1]->IsOne())
			{	// limit: PERM(&infin;;,m) is &infin; for any integer m
				// 1-ary product.
			InvokeEvalForceArg(0);
			return;
			}

		if (DiagnoseEvaluatableArgs()) return;

		signed long TestVal = 0;
		// Take small-difference.  If it exists and is 1, we do a factorial conversion
		if (   ArgArray[0]->SmallDifference(*ArgArray[1],TestVal)
			&& 1==TestVal)
			{
			IdxCurrentSelfEvalRule = RetypePermutationCountAsFactorial_SER;
			return;
			}

		if (ArgArray[1]->IsExactType(IntegerNumeral_MC))
			{
			if (ArgArray[1]->IsZero())
				{	// 0-ary product.  Force 1 in _Z_
				*static_cast<IntegerNumeral*>(ArgArray[1]) = (unsigned short)(1);
				InvokeEvalForceArg(1);
				return;
				};
			if (ArgArray[0]->IsExactType(IntegerNumeral_MC))
				{
				// Technically, this rule is valid elsewhere...but 
				// an explicit decision must be made to head into that
				// algebraic morass.
				IdxCurrentEvalRule=PermutationCountPartialEvaluate_ER;	// interacts with StdMultiplication
				return;
				}
			}
		}
	else if (IsExactType(CombinationCount_MC))
		{
		signed long TestVal = 0;
		if (ArgArray[1]->IsZero())
			{	// TODO: COMB(n,0) = 1, even for n=infinity
			if (ArgArray[1]->IsExactType(IntegerNumeral_MC))
				{	// Force 1 in _Z_
				*static_cast<IntegerNumeral*>(ArgArray[1]) = (unsigned short)(1);
				InvokeEvalForceArg(1);
				return;
				}
			}
		else if (   ArgArray[0]->IsExactType(LinearInfinity_MC)
				 || ArgArray[1]->IsOne()
				 || (ArgArray[0]->SmallDifference(*ArgArray[1],TestVal) && 1==TestVal))
			{	// COMB(infinity,_) = infinity
				// COMB(n,1) = n
				// COMB(n,n-1) = n
			InvokeEvalForceArg(0);
			return;
			};

		if (DiagnoseEvaluatableArgs()) return;

		if (ArgArray[1]->IsExactType(IntegerNumeral_MC))
			{
			if (ArgArray[0]->IsExactType(IntegerNumeral_MC))
				{
				IdxCurrentEvalRule=CombinationCountPartialEvaluate_ER;	// interacts with StdMultiplication
				return;
				}
			}
		}
	IdxCurrentSelfEvalRule=SelfEvalSyntaxOKNoRules_SER;	
}


bool CombinatorialLike::InvokeEqualArgRule() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2003
	if (IsExactType(PermutationCount_MC))
		{	// retype as Factorial of Arg 0
		IdxCurrentSelfEvalRule = RetypePermutationCountAsFactorial_SER;
		return true;
		}
	else if (IsExactType(CombinationCount_MC))
		{	// set to IntegerNumeral 1
		if (ArgArray[1]->IsExactType(IntegerNumeral_MC))
			{
			*static_cast<IntegerNumeral*>(ArgArray[1]) = (unsigned short)(1);
			InvokeEvalForceArg(1);
			return true;
			};
		try	{
			IntegerNumeral* Target = new IntegerNumeral((unsigned short)1);
			delete ArgArray[1];
			ArgArray[1] = Target;
			InvokeEvalForceArg(1);
			return true;
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		}
	return false;
}

#if 0
bool CombinatorialLike::StdMultCanDestructiveInteract() const
{
#if 0
	if 		(IsExactType(FACTORIAL_MC))
		{
		signed long TestVal = 0;
		if (   static_cast<IntegerNumeral*>(ArgArray[0])->AsSignedLong(TestVal)
			&& 12>=TestVal)
			return false;
		return false;
		}
	else if (IsExactType(PERMUTATION_MC))
		{
		if (   *ArgArray[0]==*ArgArray[1]
			||  ArgArray[0]->IsExactType(LinearInfinity_MC)
			|| (   ArgArray[1]->IsExactType(IntegerNumeral_MC) 
				&& (   ArgArray[1]->IsZero()
					|| ArgArray[1]->IsOne())))
			return false;

		signed long TestVal = 0;
		// Take small-difference.  If it exists and is 1, we do a factorial conversion
		if (   ArgArray[0]->SmallDifference(*ArgArray[1],TestVal)
			&& 1==TestVal)
			return false;

		if (ArgArray[1]->IsExactType(IntegerNumeral_MC))
			{
			if (ArgArray[0]->IsExactType(IntegerNumeral_MC))
				{
				}
			}
		return false;
		}
#endif
	return false;
}

// Reduces computational loading (don't do for simple evaluations)
// (remember: FACTORIAL<PERM; do mixed tests from PERM viewpoint)
// FACTORIAL(M)/FACTORIAL(N) = PERM(M,M-N) when M>N
// following should check for N=0, but N=0 is simple
// PERM(M,N)/(M-N+1)=PERM(M,N-1)
// PERM(M,N)/M = PERM(M-1,N-1)

// Convenience
// PERM(M,N)*FACTORIAL(M-N) = FACTORIAL(M)
// PERM(M,N)*PERM(M-N,K) = PERM(M,N+K)

bool
CombinatorialLike::StdMultCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const
{
#if 0
	if 		(IsExactType(FACTORIAL_MC))
		{
		if (Target.IsExactType(FACTORIAL_MC))
			{
			if (   IsMetaMultInverted()!=Target.IsMetaMultInverted()
				&& *ArgArray[0]!=*Target.ArgN(0)
				&& ArgArray[0]->IsExactType(IntegerNumeral_MC)
				&& Target.ArgN(0)->IsExactType(IntegerNumeral_MC))
				//! \todo some test that considers whether M-N reduces "cleanly"
				{	// FACTORIAL(M)/FACTORIAL(N) = PERM(M,M-N) when M>N
				ActOnThisRule = CancelMultInvFactorial_SER;	// TODO: IMPLEMENT
				return true;
				}
			}
		}
	else if (IsExactType(PERMUTATION_MC))
		{
		if 		(Target.IsExactType(PERMUTATION_MC))
			{
			}
		else if	(Target.IsExactType(FACTORIAL_MC))
			{
			}
		}
#endif
	return false;
}
#endif

bool CombinatorialLike::DelegateEvaluate(MetaConcept*& dest)
{
	assert(MetaConceptWithArgArray::MaxEvalRuleIdx_ER<IdxCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxEvalRuleIdx_ER>=IdxCurrentEvalRule);
	return (this->*EvaluateRuleLookup[IdxCurrentEvalRule-(MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1)])(dest);
}

bool CombinatorialLike::DelegateSelfEvaluate()
{
	assert(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER<IdxCurrentSelfEvalRule);
	assert(MaxSelfEvalRuleIdx_SER+MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER>=IdxCurrentSelfEvalRule);
	return (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1)])();
}

enum ConsecutiveDecreasingLimits
	{
	Ary2Limit_CDL = 31623,
	Ary3Limit_CDL = 1001,
	Ary4Limit_CDL = 179,
	Ary5Limit_CDL = 65,
	Ary6Limit_CDL = 34
	};

static const signed long CDL_Bounds[5]
	=	{
		Ary2Limit_CDL,
		Ary3Limit_CDL,
		Ary4Limit_CDL,
		Ary5Limit_CDL,
		Ary6Limit_CDL
		};

// This *doesn't* handle overflow
// it probably belongs in a machine integer math library
static signed long
HornerPolynomialEvaluateLeadingCoeffOneConstantZero(signed long X,size_t DegreeSub1,const signed long* Coefficients)
{
	assert(1<=DegreeSub1 && Coefficients);
	signed long NewVal = X;
	do	{
		NewVal += Coefficients[--DegreeSub1];
		NewVal *= X;
		}
	while(0<DegreeSub1);
	return NewVal;
}

static const signed long CoeffAry3ConsecutiveDecreasingProduct[2] = {2,-3};
static const signed long CoeffAry4ConsecutiveDecreasingProduct[3] = {-6,11,-6};
static const signed long CoeffAry5ConsecutiveDecreasingProduct[4] = {24,-50,35,-10};
static const signed long CoeffAry6ConsecutiveDecreasingProduct[5] = {-120,274,-225,85,-15};

static signed long ConsecutiveDecreasingProductAry2(signed long X)
{	// TestVal*(TestVal-1) fits (also Horner algorithm)
	return X*(X-1);
}

static signed long ConsecutiveDecreasingProductAry3(signed long X)
{	// TestVal*(TestVal-1)*(TestVal-2) fits
	// Expanded: TestVal^3-3*TestVal^2+2*TestVal
	return HornerPolynomialEvaluateLeadingCoeffOneConstantZero(X,2,CoeffAry3ConsecutiveDecreasingProduct);
}

static signed long ConsecutiveDecreasingProductAry4(signed long X)
{	// TestVal*(TestVal-1)*(TestVal-2)*(TestVal-3) fits
	// Expanded: TestVal^4-6*TestVal^3+11*TestVal^2-6*TestVal
	return HornerPolynomialEvaluateLeadingCoeffOneConstantZero(X,3,CoeffAry4ConsecutiveDecreasingProduct);
}

static signed long ConsecutiveDecreasingProductAry5(signed long X)
{	// TestVal*(TestVal-1)*(TestVal-2)*(TestVal-3)*(TestVal-4) fits
	// Expanded: TestVal^5-10*TestVal^4+35*TestVal^3-50*TestVal^2+24*TestVal
	return HornerPolynomialEvaluateLeadingCoeffOneConstantZero(X,4,CoeffAry5ConsecutiveDecreasingProduct);
}

static signed long ConsecutiveDecreasingProductAry6(signed long X)
{	// TestVal*(TestVal-1)*(TestVal-2)*(TestVal-3)*(TestVal-4)*(TestVal-5) fits
	// Expanded: T^6-15*T^5+85*T^4-225*T^3+274*T^2-120*T^1
	return HornerPolynomialEvaluateLeadingCoeffOneConstantZero(X,5,CoeffAry6ConsecutiveDecreasingProduct);
}

typedef signed long SignedLongToSignedLongMap(signed long X);

static SignedLongToSignedLongMap* ConsecutiveDecreasingProduct[5]
	=	{
		ConsecutiveDecreasingProductAry2,
		ConsecutiveDecreasingProductAry3,
		ConsecutiveDecreasingProductAry4,
		ConsecutiveDecreasingProductAry5,
		ConsecutiveDecreasingProductAry6
		};

static unsigned int SafeArityForConsecutiveProduct(signed long TestVal)
{
	if (Ary2Limit_CDL>=TestVal)
		{
		if (Ary6Limit_CDL>=TestVal)
			return 6;
		else if (Ary5Limit_CDL>=TestVal)
			return 5;
		else if (Ary4Limit_CDL>=TestVal)
			return 4;
		else if (Ary3Limit_CDL>=TestVal)
			return 3;
		else
			return 2;
		}
	return 0;
}

bool CombinatorialLike::FactorialPartialEvaluate(MetaConcept*& dest)
{	// Base version: 2-ary StdMultiplication
	// Arg 0: Decremented Factorial
	// Arg 1: IntegerNumeral of interest
	// Note: FACTORIAL(0)..FACTORIAL(12) will fit in a small IntegerNumeral
	assert(!dest);
	signed long TestVal = 0;
	unsigned long DecrementBy = 1;
	bool RepresentableAsSignedLong = static_cast<IntegerNumeral*>(ArgArray[0])->AsSignedLong(TestVal);
	if (RepresentableAsSignedLong && 12>=TestVal)
		{
		*static_cast<IntegerNumeral*>(ArgArray[0]) = ReferenceData<void>::FactorialTable[TestVal];
		FastTransferOutAndNULL(0,dest);

		// propagate Add/MultInv to Target
		if (IsMetaAddInverted()) dest->SelfInverse(StdAddition_MC);
		if (IsMetaMultInverted())
			dest->SelfInverse(StdMultiplication_MC);
		return true;
		}
	StdMultiplication* NewTarget = NULL;
	MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(2);
	if (!NewArgArray) return false;
	
	try	{
		NewArgArray[1] = new CombinatorialLike(FACTORIAL_CM);
		ArgArray[0]->CopyInto(NewArgArray[0]);

		if (RepresentableAsSignedLong)
			{
			DecrementBy = SafeArityForConsecutiveProduct(TestVal);
			if (0==DecrementBy)
				DecrementBy = 1;
			else
				*static_cast<IntegerNumeral*>(NewArgArray[0]) = (ConsecutiveDecreasingProduct[DecrementBy-2])(TestVal);
			}

		NewTarget = new StdMultiplication();
		}
	catch(const bad_alloc&)
		{
		BLOCKDELETEARRAY(NewArgArray);
		}

	// RAM-safe now
	static_cast<IntegerNumeral*>(ArgArray[0])->ReduceAbsValByN(DecrementBy);	
	if (IsMetaMultInverted())
		NewArgArray[0]->SelfInverse(StdMultiplication_MC);
	MoveInto(NewArgArray[1]);
	NewTarget->ReplaceArgArray(NewArgArray);
	NewTarget->ForceCheckForEvaluation();
	dest = NewTarget;
	assert(dest->SyntaxOK());
	return true;
}

bool CombinatorialLike::RetypePermutationCountAsFactorial()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/15/2004
	SetExactType(Factorial_MC);
	DeleteIdx(1);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool
CombinatorialLike::PermutationCountPartialEvaluate(MetaConcept*& dest)
{	//! \todo IMPLEMENT
	// Note: We are guaranteed *only* that ArgArray[1] is an IntegerNumeral
	// greater than or equal to 2!  However, it would be unusual to want to
	// expand this.
	assert(!dest);
	if (ArgArray[0]->IsExactType(IntegerNumeral_MC))
		{
		//! \todo direct computations to small IntegerNumerals;
		//! use goto to control exit code bloat
		signed long TestVal = 0;
		unsigned long DecrementBy = 1;
		bool RepresentableAsSignedLong = static_cast<IntegerNumeral*>(ArgArray[0])->AsSignedLong(TestVal);
		if (RepresentableAsSignedLong)
			{
			signed long RHSVal = 0;
			SUCCEED_OR_DIE(static_cast<IntegerNumeral*>(ArgArray[1])->AsSignedLong(RHSVal));
			SUCCEED_OR_DIE(TestVal>=RHSVal && 2<=RHSVal && 1<TestVal-RHSVal);
			if (12>=TestVal)
				{	// Direct computation from the definition
				*static_cast<IntegerNumeral*>(ArgArray[0]) = ReferenceData<void>::FactorialTable[TestVal]/ReferenceData<void>::FactorialTable[TestVal-RHSVal];
DirectCalcOK:
				TransferOutAndNULL(0,dest);

				// propagate Add/MultInv to Target
				if (IsMetaAddInverted())
					dest->SelfInverse(StdAddition_MC);
				if (IsMetaMultInverted())
					dest->SelfInverse(StdMultiplication_MC);
				assert(dest->SyntaxOK());
				return true;
				}

			if (7>RHSVal && CDL_Bounds[RHSVal]>=TestVal)
				{
				*static_cast<IntegerNumeral*>(ArgArray[0]) = (ConsecutiveDecreasingProduct[RHSVal-2])(TestVal);
				goto DirectCalcOK;
				}
			// TODO: more useful heuristics...up to 9==RHSVal
			}

		// general case:
		StdMultiplication* NewTarget = NULL;
		MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(2);
		if (!NewArgArray) return false;
	
		try	{
			NewArgArray[1] = new CombinatorialLike(PERMUTATIONCOUNT_CM);

			// TODO: optimize the following for iterations
			ArgArray[0]->CopyInto(NewArgArray[0]);

			NewTarget = new StdMultiplication();
			}
		catch(const bad_alloc&)
			{
			BLOCKDELETEARRAY(NewArgArray);
			}

		// RAM-safe now
		static_cast<IntegerNumeral*>(ArgArray[0])->ReduceAbsValByN(DecrementBy);	
		static_cast<IntegerNumeral*>(ArgArray[1])->ReduceAbsValByN(DecrementBy);
		if (IsMetaMultInverted())
			NewArgArray[0]->SelfInverse(StdMultiplication_MC);
		MoveInto(NewArgArray[1]);
		NewTarget->ReplaceArgArray(NewArgArray);
		NewTarget->ForceCheckForEvaluation();
		dest = NewTarget;
		assert(dest->SyntaxOK());
		return true;
		}
	return false;
}

bool CombinatorialLike::CombinationCountPartialEvaluate(MetaConcept*& dest)
{	//! \todo IMPLEMENT
	// Note: We are guaranteed *only* that ArgArray[1] is an IntegerNumeral
	// greater than or equal to 2!  However, it would be unusual to want to
	// expand this.
	assert(!dest);
	if (ArgArray[0]->IsExactType(IntegerNumeral_MC))
		{
		// TODO: direct computations to small IntegerNumerals
		// use goto to control exit code bloat
		signed long TestVal = 0;
		bool RepresentableAsSignedLong = static_cast<IntegerNumeral*>(ArgArray[0])->AsSignedLong(TestVal);
		if (RepresentableAsSignedLong)
			{
			signed long RHSVal = 0;
			SUCCEED_OR_DIE(static_cast<IntegerNumeral*>(ArgArray[1])->AsSignedLong(RHSVal));
			SUCCEED_OR_DIE(TestVal>=RHSVal || 2<=RHSVal || 1<TestVal-RHSVal);
			if (12>=TestVal)
				{	// Direct computation from the definition
				signed long TempVal = ReferenceData<void>::FactorialTable[TestVal]/ReferenceData<void>::FactorialTable[TestVal-RHSVal];
				*static_cast<IntegerNumeral*>(ArgArray[0]) = TempVal/ReferenceData<void>::FactorialTable[RHSVal];
				UseArg0ForPropagateAddMultInv(dest);
				return true;
				}

			// Symmetry exploit
			if (   7<=RHSVal
				&& 7>TestVal-RHSVal
				&& CDL_Bounds[TestVal-RHSVal]>=TestVal)
				RHSVal = TestVal-RHSVal;

			if (7>RHSVal && CDL_Bounds[RHSVal]>=TestVal)
				{
				signed long TempVal = (ConsecutiveDecreasingProduct[RHSVal-2])(TestVal);
				*static_cast<IntegerNumeral*>(ArgArray[0]) = TempVal/ReferenceData<void>::FactorialTable[RHSVal];
				UseArg0ForPropagateAddMultInv(dest);
				return true;
				}
			// TODO: more useful heuristics...up to 9==RHSVal
			}

		// general case:
		StdMultiplication* NewTarget = NULL;
		MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(3);
		if (!NewArgArray) return false;
	
		try	{
			NewArgArray[2] = new CombinatorialLike(COMBINATIONCOUNT_CM);

			// TODO: optimize the following for iterations
			ArgArray[0]->CopyInto(NewArgArray[0]);
			ArgArray[1]->CopyInto(NewArgArray[1]);

			NewTarget = new StdMultiplication();
			}
		catch(const bad_alloc&)
			{
			BLOCKDELETEARRAY(NewArgArray);
			}

		// RAM-safe now
		// Basic recursions:
		// COMB(N,K) = (N/K)*COMB(N-1,K-1)
		// COMB(N,K) = (N/(N-K))*COMB(N-1,K) [TODO: IMPLEMENT this to minimize iterations]
		// To be efficient, needs a low-level X>=2*Y test
		//! \todo: implement IntegerNumeral low-level X>=2*Y test

		static_cast<IntegerNumeral*>(ArgArray[0])->ReduceAbsValByN(1);	
		static_cast<IntegerNumeral*>(ArgArray[1])->ReduceAbsValByN(1);
		NewArgArray[(IsMetaMultInverted()) ? 0 : 1]->SelfInverse(StdMultiplication_MC);

		MoveInto(NewArgArray[2]);
		NewTarget->ReplaceArgArray(NewArgArray);
		NewTarget->ForceCheckForEvaluation();
		dest = NewTarget;
		assert(dest->SyntaxOK());
		return true;
		}
	return false;
}

