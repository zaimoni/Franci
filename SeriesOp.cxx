// SeriesOp.cxx

// declaration of SeriesOperation, which represents (will represent) finite and infinite 
// summation/product/etc. series notation.

// SeriesOperation will initially support + and *: SeriesAddition_MC and SeriesMultiplication_MC
// The argument order is important:
// Arg 0: the index variable.  We may have re-quantification issues from 
// this.
// Arg 1: the range.  We currently support single integers as a degenerate case, and 
// integer-domain intervals.  We choose this formalism to simplify the introduction of 
// generic indexing sets.
// Arg 2: the expression to be series-operated over.
// The arity must be exactly 3, or else.

// Note that the following constraints are necessary to avoid problems:
// * ensure that var-substitution dies if variable used by SeriesOperation is applied from the outside
// * ensure that said variable is not detected in use
// ** above two can be handled by making Variable test its quantifier for ptr equality, 
//	  not just semantic equality...but ptr equality doesn't work on Intel (even Win95, 
//    let alone DOS)
// * ensure that equality respects index variable replacement
// * TODO: ensure that when nesting series expressions, that variables don't clash 
// (OK as long as auto-generation used; script-spec more interesting)


// NOTE: the third operation to be supported will motivate a transition to 
// member function pointer arrays.
// NOTE: we already want a 'translation array' from Series___MC to Std___MC

//! \todo implement augmented SeriesExpandIntegerNumeralRange that simply relies on MetaConcept's SmallDifference to trigger

#include "Class.hxx"
#include "Variable.hxx"
#include "Integer1.hxx"
#include "StdAdd.hxx"
#include "StdMult.hxx"
#include "SeriesOp.hxx"
#include "Keyword1.hxx"
#include "LowRel.hxx"
#include <memory>

SeriesOperation::EvaluateToOtherRule SeriesOperation::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
  =	{
	&SeriesOperation::ExpandZeroAry,
	&SeriesOperation::ExpandUnary,
	&SeriesOperation::ExpandIntegerNumeralRange
	};

static_assert(SeriesAddition_MC - StdAddition_MC == SeriesMultiplication_MC - StdMultiplication_MC);

static constexpr ExactType_MC toCoreOp(ExactType_MC op) {	// \todo if CPU is a problem, rewrite to subtraction
	switch (op)
	{
	default: UnconditionalCallAssumptionFailure();
	case SeriesAddition_MC: return StdAddition_MC;
	case SeriesMultiplication_MC: return StdMultiplication_MC;
	}
}

static_assert(StdAddition_MC == toCoreOp(SeriesAddition_MC));
static_assert(StdMultiplication_MC == toCoreOp(SeriesMultiplication_MC));

static constexpr ExactType_MC toSeriesOp(ExactType_MC op) noexcept {
	switch (op)
	{
	default: UnconditionalCallAssumptionFailure();
	case StdAddition_MC: return SeriesAddition_MC;
	case StdMultiplication_MC: return SeriesMultiplication_MC;
	}
}

static_assert(SeriesAddition_MC == toSeriesOp(StdAddition_MC));
static_assert(SeriesMultiplication_MC == toSeriesOp(StdMultiplication_MC));

SeriesOperation::SeriesOperation(ExactType_MC Operation) noexcept
:	MetaConceptWithArgArray(toSeriesOp(Operation))	// NOTE: Uses a low-level dependency
{	// FORMALLY CORRECT: Kenneth Boyd, 1/6/2003
}


SeriesOperation::SeriesOperation(MetaConcept**& NewArgList,ExactType_MC Operation)
:	MetaConceptWithArgArray(toSeriesOp(Operation),NewArgList)	// NOTE: Uses a low-level dependency
{	// FORMALLY CORRECT: Kenneth Boyd, 10/28/2002
	if (!SyntaxOK()) _forceStdForm();
}

// Generic notations:
// SERIES(+|*,var IN Domain,Range description,expression)
// [Range description is any of Explicit Constant, LinearInterval with domain _Z_, or NULLSet]
// SERIES(+|*,var=ExplicitConstant,expression)
// SERIES(+|*,var=A...B,expression)
// sigma, pi notations for linear-interval form
// FACTORIAL(n), n a positive integer: SERIES(*,var=1...n,var)
// FACTORIAL(0), SERIES(*,var in _Z_,NULLSet,var)
// n!
// 0!

// FACTORIAL() is appropriate when:
// operation is *
// Index variable is subclass of _Z_ containing range
// expression is the sole index variable
// FACTORIAL(0) is NULLSet range.
// FACTORIAL(1) is range {1}
// FACTORIAL(K) is range {1...K}, 1<K
// Domain contains range spec
bool SeriesOperation::FactorialIsAppropriateRepresentation() const
{	// FORMALLY CORRECT: 2020-08-02
//! \todo? move this to LenName.cxx
	if (!IsExactType(SeriesMultiplication_MC)) return false;
	const Variable* const var_index = up_cast<Variable>(ArgArray[EXPRESSION_IDX]);
	if (    var_index &&  var_index->NoMetaModifications()
		&& !var_index->IsUltimateType(NULL) && var_index->UltimateType()->Subclass(Complex)
		&&  static_cast<MetaQuantifier*>(ArgArray[INDEXVAR_IDX])->MetaConceptPtrUsesThisQuantifier(var_index))
		{
		if (*ArgArray[DOMAIN_IDX]==NULLSet || ArgArray[DOMAIN_IDX]->IsOne()) return true;
		if (    ArgArray[DOMAIN_IDX]->IsTypeMatch(LinearInterval_MC,&Integer)
			&&  ArgArray[DOMAIN_IDX]->ArgN(0)->IsOne()	// \todo might want to allow 2..n as an index range
			&& !ArgArray[DOMAIN_IDX]->ArgN(1)->IsExactType(LinearInfinity_MC))
			return true;	// historically "non-empty" but that should be tautological
		}
	return false;
}

std::string SeriesOperation::to_s_aux() const
{
	std::string ret;

	if (FactorialIsAppropriateRepresentation()) {
		ret = PrefixKeyword_FACTORIAL;
		ret += '(';
		if (*ArgArray[DOMAIN_IDX] == NULLSet) ret += '0';
		else if (ArgArray[DOMAIN_IDX]->IsOne()) ret += '1';
		else ret += ArgArray[DOMAIN_IDX]->ArgN(1)->to_s();
		ret += ')';
		return ret;
	};

	// normal form
	ret += "SERIES(";
	if (IsExactType(SeriesAddition_MC)) ret += '+';
	else if (IsExactType(SeriesMultiplication_MC)) ret += SymbolMultSign;
	else ret += '?';
	ret += ',';

	if (const auto kw = ArgArray[INDEXVAR_IDX]->ViewKeyword()) ret += kw;
	ret += (ArgArray[DOMAIN_IDX]->IsExplicitConstant() || ArgArray[DOMAIN_IDX]->IsTypeMatch(LinearInterval_MC, &Integer)) ? "=" : ",";
	ret += ArgArray[DOMAIN_IDX]->to_s();
	ret += ',';
	ret += ArgArray[EXPRESSION_IDX]->to_s();
	ret += ')';
	return ret;
}

SeriesOperation& SeriesOperation::operator=(const SeriesOperation& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	decltype(src.DesiredType) tmp(src.DesiredType);
	MetaConceptWithArgArray::operator=(src);
	DesiredType = std::move(tmp);
	return *this;
}

//  Type ID functions
// NOTE: SeriesOperation UltimateType is always non-NULL
const AbstractClass* SeriesOperation::UltimateType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/28/2002
	if 		(!DesiredType.empty())
		return DesiredType;
	else if (!ArgArray[EXPRESSION_IDX]->IsUltimateType(NULL))
		return ArgArray[EXPRESSION_IDX]->UltimateType();
	else if (IsExactType(SeriesAddition_MC))
		return &ClassAdditionDefined;
	else	// if (IsExactType(SeriesMultiplication_MC))
		return &ClassMultiplicationDefined;		
}

bool SeriesOperation::ForceUltimateType(const AbstractClass* const rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	if (MetaConcept::ForceUltimateType(rhs)) return true;
	if (!rhs->SupportsThisOperation(toCoreOp(ExactType()))) return false;

/*! This
	if (NULL==RHS)
		return true;
	if (NULL==UltimateType())
		return false;
	if (UltimateType()->Subclass(*RHS))
		return true;
	return false;

has failed with this:

	if 		(NULL!=DesiredType)
		return DesiredType;
	else if (!ArgArray[EXPRESSION_IDX]->IsUltimateType(NULL))
		return ArgArray[EXPRESSION_IDX]->UltimateType();
	else if (IsExactType(SeriesAddition_MC))
		return &ClassAdditionDefined;
	else	// if (IsExactType(SeriesMultiplication_MC))
		return &ClassMultiplicationDefined;

so we already know that:
* NULL!=RHS
* analytically, UltimateType() cannot return NULL (second default clause passed).
** TODO(?): Optimize for speed
* if NULL!=DesiredType, that DesiredType->Subclass(*RHS) failed
* if NULL==DesiredType and NULL!=ArgArray[EXPRESSION_IDX]->UltimateType(), that subclass failed
* if NULL==DesiredType and NULL==ArgArray[EXPRESSION_IDX]->UltimateType(), that the subclass 
* against the generic proper class failed.  But we weeded this out immediately afterwards.

We automatically fail against intersection NULLSet.  Otherwise, it's theoretically possible, 
but requires cancellations to work...Proceed.
*/

	if (!DesiredType.empty())
		{
		if (    DesiredType->IntersectionWithIsNULLSet(*rhs)
			|| !DesiredType->IntersectWith(*rhs))
			return false;
		}
	else{
		assert(!ArgArray[EXPRESSION_IDX]->IsUltimateType(NULL));
		if (ArgArray[EXPRESSION_IDX]->UltimateType()->IntersectionWithIsNULLSet(*rhs))
			return false;
		try	{
			ArgArray[EXPRESSION_IDX]->UltimateType()->CopyInto(DesiredType);
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		if (!DesiredType->IntersectWith(*rhs))
			{
			DesiredType.reset();
			return false;
			}
		}

	assert(NULLSet!=*DesiredType);
	return true;
}

#if OBSOLETE
// while this does interact with +/*, it should be treated as within its own parentheses block i.e. "atomic"
unsigned int SeriesOperation::OpPrecedence() const
{
	switch(ExactType())
	{
	case SeriesAddition_MC: return Precedence::Addition;
	case SeriesMultiplication_MC: return Precedence::Multiplication;
	default: SUCCEED_OR_DIE(0 && "invariant violation"); return Precedence::None;
	}
}
#endif

// Syntactical equality and inequality
bool SeriesOperation::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	const SeriesOperation& VR_rhs = static_cast<const SeriesOperation&>(rhs);
	if (*ArgArray[DOMAIN_IDX]==*VR_rhs.ArgArray[DOMAIN_IDX])
		{
		if (*ArgArray[INDEXVAR_IDX]==*VR_rhs.ArgArray[INDEXVAR_IDX])
			return *ArgArray[EXPRESSION_IDX]==*VR_rhs.ArgArray[EXPRESSION_IDX];

		//! \todo IMPLEMENT: want an 'equal-up-to-variable' test -- non-virtual MetaConcept member
		if (   ArgArray[EXPRESSION_IDX]->IsExactType(VR_rhs.ArgArray[EXPRESSION_IDX]->ExactType())
			&& ArgArray[EXPRESSION_IDX]->size()==VR_rhs.ArgArray[EXPRESSION_IDX]->size())
			{
			autoval_ptr<MetaConcept> VRExpression;
			try	{
				VR_rhs.ArgArray[EXPRESSION_IDX]->CopyInto(VRExpression);
				}
			catch(const bad_alloc&)
				{	// Not really that much recovery at this level
				UnconditionalRAMFailure();
				}
			SUCCEED_OR_DIE(VRExpression->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*VR_rhs.ArgArray[INDEXVAR_IDX],*ArgArray[INDEXVAR_IDX],SetLHSToRHS,AreSyntacticallyEqual));
			return *ArgArray[EXPRESSION_IDX]==*VRExpression;
			}
		}
	return false;
}


void SeriesOperation::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	if (!ArgArray.empty() && !IdxCurrentSelfEvalRule && !IdxCurrentEvalRule)
		{
		ForceStdFormAux();
		// We prefer additive inverse to be on the outside, rather than the inside
		if (   IsExactType(SeriesAddition_MC)
			&& ArgArray[EXPRESSION_IDX]->IsMetaAddInverted())
			{
			SelfInverse(StdAddition_MC);
			ArgArray[EXPRESSION_IDX]->SelfInverse(StdAddition_MC);
			}
		// We prefer multiplicative inverse to be on the outside, rather than the inside,
		// IF it is a finite product AND the domain has commutative multiplication
		if (   IsExactType(SeriesMultiplication_MC)
			&& ArgArray[EXPRESSION_IDX]->IsMetaMultInverted()
			&& IsFiniteSeries()	//! \todo IMPLEMENT IsFiniteSeries()
			&& (   *ArgArray[DOMAIN_IDX]==NULLSet
				||  ArgArray[DOMAIN_IDX]->IsExplicitConstant()
				|| static_cast<AbstractClass*>(ArgArray[DOMAIN_IDX])->StdMultiplicationCommutativeWithDomain(*static_cast<AbstractClass*>(ArgArray[DOMAIN_IDX]))))
			{
			SelfInverse(StdMultiplication_MC);
			ArgArray[EXPRESSION_IDX]->SelfInverse(StdMultiplication_MC);
			}
		}
}

bool SeriesOperation::_IsExplicitConstant() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	if (   *ArgArray[DOMAIN_IDX]==NULLSet
		||  ArgArray[EXPRESSION_IDX]->IsExplicitConstant())
		return true;
	return false;
}

//  Evaluation functions
bool SeriesOperation::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	// [Lang] a 0-ary sum evaluates to 0 ['omnizero'], so it *is* defined.
	// This must be caught before SyntaxOKAux.
	if (	SyntaxOKAux()							// NOTE: this catches NULL entries
		&& 	LEGAL_ARITY==fast_size()
		&&	ArgArray[INDEXVAR_IDX]->IsExactType(ThereIs_MC)
		&& !static_cast<MetaQuantifier*>(ArgArray[DOMAIN_IDX])->MetaConceptPtrUsesThisQuantifier(ArgArray[INDEXVAR_IDX]))
		{
		if (*ArgArray[DOMAIN_IDX]==NULLSet) return true;
		if (   ArgArray[DOMAIN_IDX]->IsExplicitConstant()
			|| ArgArray[DOMAIN_IDX]->IsTypeMatch(LinearInterval_MC,&Integer))
			{
			if (IsExactType(SeriesAddition_MC))
				return ArgArray[EXPRESSION_IDX]->ForceUltimateType(&ClassAdditionDefined);
			else	// if (IsExactType(SeriesMultiplication_MC))
				return ArgArray[EXPRESSION_IDX]->ForceUltimateType(&ClassMultiplicationDefined);
			}
		}
	return false;
}

void SeriesOperation::_ForceArgSameImplementation(size_t n) { NARY_FORCEARGSAMEIMPLEMENTATION_BODY; }

bool SeriesOperation::_IsOne() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	if (   IsExactType(SeriesMultiplication_MC)
		&& *ArgArray[DOMAIN_IDX]==NULLSet)
		return true;
	return false;
}

bool SeriesOperation::_IsZero() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	if (   IsExactType(SeriesAddition_MC)
		&& *ArgArray[DOMAIN_IDX]==NULLSet)
		return true;
	if (    IsExactType(SeriesMultiplication_MC)
		&&  *ArgArray[DOMAIN_IDX]!=NULLSet
		&&  ArgArray[EXPRESSION_IDX]->IsZero())
		return true;
	return false;
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > SeriesOperation::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

//  Helper functions for CanEvaluate... routines
void SeriesOperation::DiagnoseInferenceRules() const
{	//! \todo IMPLEMENT
	//!\todo NULLSet index set turns to omnizero(+) or omnione(*).  Retain DesiredType information, though.
	if (   *ArgArray[DOMAIN_IDX]==NULLSet
		&& UltimateType()->CanCreateIdentityForOperation(toCoreOp(ExactType())))
		{
		IdxCurrentEvalRule = ExpandZeroAry_ER;
		return;
		}

	if (DiagnoseEvaluatableArgs()) return;

#if 0
	if (FactorialIsAppropriateRepresentation())
		{	//! \todo IMPLEMENT SeriesProductToFactorial
		IdxCurrentEvalRule = SeriesProductToFactorial_ER;
		return;
		}
#endif

	//! \todo finite index sets may be directly converted to an explicit expansion.
	//! Retain DesiredType information, though
	//! So far, two cases: single constant element and Integer-domain interval
	if (   ArgArray[DOMAIN_IDX]->IsTypeMatch(LinearInterval_MC,&Integer)
		&& ArgArray[DOMAIN_IDX]->ArgN(0)->IsExactType(IntegerNumeral_MC)
		&& ArgArray[DOMAIN_IDX]->ArgN(1)->IsExactType(IntegerNumeral_MC))
		{
		IdxCurrentEvalRule = ExpandIntegerNumeralRange_ER;
		return;
		}
	if (ArgArray[DOMAIN_IDX]->IsExplicitConstant())
		{
		IdxCurrentEvalRule = ExpandUnary_ER;
		return;
		}
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
bool SeriesOperation::InvokeEqualArgRule() const {return false;}

//! \todo when explicit test for infinite series comes up, change SeriesOperation::IsFiniteSeries to TruthValue-based 
// meta-code
bool SeriesOperation::IsFiniteSeries() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/15/2004
	// NULLSet: 0 iterations
	if (*ArgArray[DOMAIN_IDX]==NULLSet) return true;
	else if (ArgArray[DOMAIN_IDX]->IsExplicitConstant())	// explicit constant: 1 iteration
		return true;
	else if (ArgArray[DOMAIN_IDX]->IsExactType(LinearInterval_MC))
		{
		if (ArgArray[DOMAIN_IDX]->IsUltimateType(&Integer))
			{
			if (   !ArgArray[DOMAIN_IDX]->ArgN(0)->IsExactType(LinearInfinity_MC)
				&& !ArgArray[DOMAIN_IDX]->ArgN(1)->IsExactType(LinearInfinity_MC))
				return true;	// all integer-types other than infinity are finite
			return false;
			}
		else if (*ArgArray[DOMAIN_IDX]->ArgN(0)==*ArgArray[DOMAIN_IDX]->ArgN(1))
			return true;	// masked singleton...finite
		else if (ArgArray[DOMAIN_IDX]->UltimateType()->IsDenseUnderStandardTopology())
			return false;	// dense non-singleton intervals are never finite
		// default: clueless, so don't commit to anything
		return false;
		};
	// default: clueless, so don't commit to anything
	return false;
}

bool SeriesOperation::DelegateEvaluate(MetaConcept*& dest)
{
	assert(MetaConceptWithArgArray::MaxEvalRuleIdx_ER<IdxCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxEvalRuleIdx_ER>=IdxCurrentEvalRule);
	return (this->*EvaluateRuleLookup[IdxCurrentEvalRule-(MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1)])(dest);
}

static MetaConceptWithArgArray* createIdentity(ExactType_MC op) {
	switch (op)
	{
	default: UnconditionalCallAssumptionFailure();
	case StdAddition_MC: return new StdAddition();
	case StdMultiplication_MC: return new StdMultiplication();
	}
}

bool SeriesOperation::ExpandZeroAry(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	assert(!dest);
	if (UltimateType()->CreateIdentityForOperation(dest,toCoreOp(ExactType())))
		{
		if (!dest) dest = createIdentity(ExactType());
		LOG("Replacing zero-ary series with corresponding identity:");
		LOG(*this);
		LOG(*dest);
		assert(dest->SyntaxOK());
		return true;		
		}
	return false;
}

bool SeriesOperation::ExpandUnary(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/28/2002
	// This essentially takes the expression arg, and substitutes the sole expression for
	// the target
	assert(!dest);
	autoval_ptr<MetaConcept> TmpVar;
	TmpVar = new Variable(static_cast<MetaQuantifier*>(ArgArray[SeriesOperation::INDEXVAR_IDX]));

	LOG("Replacing 1-ary series with its sole term:");
	LOG(*this);

	if (   DestructiveStrongVarSubstitute(ArgArray[SeriesOperation::EXPRESSION_IDX],*TmpVar,ArgArray[SeriesOperation::DOMAIN_IDX])
		|| StrongVarSubstitute(ArgArray[SeriesOperation::EXPRESSION_IDX],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]))
		{
		dest = ArgArray[SeriesOperation::EXPRESSION_IDX];
		ArgArray[SeriesOperation::EXPRESSION_IDX] = NULL;

		LOG(*dest);
		assert(dest->SyntaxOK());
		return true;
		}
	return false;
}

bool SeriesOperation::ExpandIntegerNumeralRange(MetaConcept*& dest)
{	// NOTE: if expression doesn't use index var:
	// + reduces to * [put integer on left, in case of module action escape]
	// * reduces to Power with operation *
	// NOTE: if expression uses index var, we need to be careful about excessive expansion
	// this function doesn't like automatic memory management
	assert(!dest);
	std::unique_ptr<MetaConceptWithArgArray> Tmp(createIdentity(ExactType()));
	signed long Range;
	bool IsSmallDifference = static_cast<IntegerNumeral*>(ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1))->SmallDifference(*static_cast<IntegerNumeral*>(ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(0)),Range);

	zaimoni::autovalarray_ptr_throws<MetaConcept*> NewArgArray((IsSmallDifference && 1 == Range) ? 2 : 3);

	LOG("Expanding this series:");
	LOG(*this);

	// Now: to initialize the Argarray "safely"
	// we rely on our only caller being LexParse's evaluate processor (which has a std::bad_alloc handler)
	// We know that for the LinearInterval, ArgN(0)<ArgN(1) and are IntegerNumerals.
	// The instant-expansion range is ArgN(1)-ArgN(0)+1.  If we can't do that, we should 
	// strip at least two arguments...to put things mildly, "everything is risky".
	// This suggests that 2-3 args would always be immediately expanded.
	// We can always avoid incrementing positive integers/decrementing negative integers to
	// a larger-representation value.
	ArgArray[SeriesOperation::EXPRESSION_IDX]->CopyInto(NewArgArray[1]);
	if (IsSmallDifference && 2>=Range)
		{
		ArgArray[SeriesOperation::EXPRESSION_IDX]->CopyInto(NewArgArray[0]);
		if (2==Range)	// 3 args.
			ArgArray[SeriesOperation::EXPRESSION_IDX]->CopyInto(NewArgArray[2]);
		}
	else{
		if (ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1)->IsPositive())
			{	// decrement upper bound is safe
			ArgArray[SeriesOperation::EXPRESSION_IDX]->CopyInto(NewArgArray[2]);
			}
		else{	// increment lower bound is safe
			ArgArray[SeriesOperation::EXPRESSION_IDX]->CopyInto(NewArgArray[0]);
			};
		}

	zaimoni::autoval_ptr<Variable> TmpVar;
	TmpVar = new Variable(static_cast<MetaQuantifier*>(ArgArray[SeriesOperation::INDEXVAR_IDX]));

	if (IsSmallDifference && 2>=Range)
		{
		//! \todo IMPLEMENT: OverwriteArgNWith(Idx,MetaConcept*&) and TakeArgNAndNULL(Idx,&MetaConcept*&),
		//! MetaConcept members with the semantics of MetaConceptWithArgArray's
		//! TransferInAndOverwrite and TransferOutAndNULL.  This lets us safely use
		//! DestructiveStrongVarSubstitute here.  (We *need* both.  TakeArgNAndNULL lets us
		//! invoke DestructiveStrongVarSubstitute, while OverwriteArgNWith lets us
		//! correctly recover from failure.)
		if (2==Range)
			{	// 3 args
			if (   !StrongVarSubstitute(NewArgArray[0],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(0))
				|| !StrongVarSubstitute(NewArgArray[2],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1)))
				goto ExitFailCleanAll;

			// decrement upper bound/increment lower bound is safe
			unsigned int MiddleIdx = (ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1)->IsPositive()) ? 1 : 0;
			static_cast<IntegerNumeral*>(ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(MiddleIdx))->ReduceAbsValByN(1);
			if (!StrongVarSubstitute(NewArgArray[1],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(MiddleIdx)))
				goto ExitFailCleanAll;
			}
		else{	// 2 args
			if (   !StrongVarSubstitute(NewArgArray[0],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(0))
				|| !StrongVarSubstitute(NewArgArray[1],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1)))
				goto ExitFailCleanAll;
			}
		}
	else{
		if (ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1)->IsPositive())
			{	// decrement upper bound is safe
			if (!StrongVarSubstitute(NewArgArray[2],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1)))
				goto ExitFailCleanAll;
			static_cast<IntegerNumeral*>(ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1))->ReduceAbsValByN(1);
			if (!StrongVarSubstitute(NewArgArray[1],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1)))
				goto ExitFailCleanAll;
			static_cast<IntegerNumeral*>(ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(1))->ReduceAbsValByN(1);
			MoveInto(NewArgArray[0]);
			}
		else{	// increment lower bound is safe
			if (!StrongVarSubstitute(NewArgArray[0],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(0)))
				goto ExitFailCleanAll;
			static_cast<IntegerNumeral*>(ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(0))->ReduceAbsValByN(1);
			if (!StrongVarSubstitute(NewArgArray[1],*TmpVar,*ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(0)))
				goto ExitFailCleanAll;
			static_cast<IntegerNumeral*>(ArgArray[SeriesOperation::DOMAIN_IDX]->ArgN(0))->ReduceAbsValByN(1);
			MoveInto(NewArgArray[2]);
			};
		}

	// NewArgArray is now initialized
	Tmp->ReplaceArgArray(NewArgArray);
	Tmp->ForceCheckForEvaluation();
	dest = Tmp.release();

	LOG("to:");
	LOG(*dest);

	assert(dest->SyntaxOK());
	return true;
ExitFailCleanAll:
	return false;
}

