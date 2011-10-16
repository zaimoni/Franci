// Integer1.cxx
// Implementation of IntegerNumeral
// This is an abusive name; direct multiplicative inverses of integers are also subsumed by this type.

#include "Class.hxx"
#include "Integer1.hxx"
#include "GCF.hxx"

#include "Zaimoni.STL/limits"

// NOTE: 1/0 is garbage [NOT SyntaxOK]
// NOTE: 1/1=1, 1/-1=-1; catch these immediately

///////////
// Divisibility tests in Base 10^9
//	*	2, 5: direct modulo on lowest significant digit.  IF the numeral ends is 0, we have
//		2^9 and 5^9 as factors, and may continue the exercise.
//	*	10^9-1: take sum of digits, reducing by 10^9-1 as necessary to prevent overflows.
//		This will diagnose:
//			3^4
//			37
//		333667	[this is a prime]
//		[Erasthones sieve: check up to 577, starting at 7]
//	*	10^9+1: take sum of alternating digits, reducing by 10^9+1 as necessary; subtract sums
//		This will diagnose:
//			7
//			11
//			13
//			19
//			52579 [this is a prime]
//		[Erasthones sieve: check up to 999, starting at 17]

//	NOTE: RemainderOfLargeDivideByN is a RAM-safe operation.  This allows
//	testing for divisibility by any prime < 10^9 in a RAM-safe fashion.
//	This would be an exhaustive primality test for any number < 10^18.
//	There exists (if difficult-to-find) a minimal spanning set of products of 
//	primes to first powers allowing a linear-time test for all of these primes.  
//	More useful might be a condensed series in order (this would permit bailing 
//	out at a sufficiently early point)

const IntegerNumeral& IntegerNumeral::operator=(const IntegerNumeral& src)
{	// FORMALLY CORRECT: 7/12/2007, Kenneth Boyd
	_IntegerNumeral::operator=(src);
	MetaConceptZeroArgs::operator=(src);
	return *this;
}

const IntegerNumeral& IntegerNumeral::operator=(signed short src)
{	// FORMALLY CORRECT: 7/12/2007, Kenneth Boyd
	MultiPurposeBitmap = NoModifiers_VF;
	_IntegerNumeral::operator=(src);
	return *this;
}

const IntegerNumeral& IntegerNumeral::operator=(unsigned short src)
{	// FORMALLY CORRECT: 7/12/2007, Kenneth Boyd
	MultiPurposeBitmap = NoModifiers_VF;
	_IntegerNumeral::operator=(src);
	return *this;
}

const IntegerNumeral& IntegerNumeral::operator=(signed long src)
{	// FORMALLY CORRECT: 7/12/2007, Kenneth Boyd
	MultiPurposeBitmap = NoModifiers_VF;
	_IntegerNumeral::operator=(src);
	return *this;
}

const IntegerNumeral& IntegerNumeral::operator=(unsigned long src)
{	// FORMALLY CORRECT: 7/12/2007, Kenneth Boyd
	MultiPurposeBitmap = NoModifiers_VF;
	_IntegerNumeral::operator=(src);
	return *this;
}

void IntegerNumeral::MoveInto(IntegerNumeral*& dest)	// can throw memory failure.  If it succeeds, it destroys the source.
{	// FORMALLY CORRECT: 12/9/2004, Kenneth Boyd
	if (NULL==dest) dest = new IntegerNumeral();
	MoveInto(*dest);
}

void IntegerNumeral::MoveInto(IntegerNumeral& dest)	// destroys the source.
{	// FORMALLY CORRECT: 7/12/2007, Kenneth Boyd
	_IntegerNumeral::MoveInto(dest);
	dest.MultiPurposeBitmap = MultiPurposeBitmap;
}

bool IntegerNumeral::SyntacticalStandardLTAux(const MetaConcept& rhs) const
{	// MUTABLE
	if 		(rhs.IsExactType(IntegerNumeral_MC))
		// handle here:
		return _IntegerNumeral::operator<(static_cast<const IntegerNumeral&>(rhs));
	return false;
}

bool IntegerNumeral::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2007
	return _IntegerNumeral::operator==(static_cast<const IntegerNumeral&>(rhs));
}

bool IntegerNumeral::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: 7/12/2007
	return _IntegerNumeral::operator<(static_cast<const IntegerNumeral&>(rhs));
}

//  Type ID functions
const AbstractClass* IntegerNumeral::UltimateType() const
{	// FORMALLY CORRECT: 7/12/2007
	return (in_Z() ? &Integer : &Rational);
}

//  Evaluation functions
bool IntegerNumeral::CanEvaluate() const {return false;}
bool IntegerNumeral::CanEvaluateToSameType() const {return false;}
bool IntegerNumeral::Evaluate(MetaConcept*& dest) {return false;}
bool IntegerNumeral::DestructiveEvaluateToSameType() {return false;}

bool IntegerNumeral::SelfInverse(const ExactType_MC Operation)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2007
	if (StdAddition_MC==Operation) return _IntegerNumeral::SelfInverse(_IntegerNumeral::Addition);
	if (StdMultiplication_MC==Operation) return _IntegerNumeral::SelfInverse(_IntegerNumeral::Multiplication);
	return false;
}

bool
IntegerNumeral::SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2007
	if (rhs.IsExactType(IntegerNumeral_MC))
		{
		const IntegerNumeral& VR_RHS = static_cast<const IntegerNumeral&>(rhs);
		if (StdAddition_MC==Operation) return _IntegerNumeral::SelfInverseTo(VR_RHS,_IntegerNumeral::Addition);
		if (StdMultiplication_MC==Operation) return _IntegerNumeral::SelfInverseTo(VR_RHS,_IntegerNumeral::Multiplication);
		}
	return false;
}

bool
IntegerNumeral::SmallDifference(const MetaConcept& rhs, signed long& Result) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2007
	if (rhs.IsExactType(IntegerNumeral_MC))
		return _IntegerNumeral::SmallDifference(static_cast<const IntegerNumeral&>(rhs),Result);
	return MetaConcept::SmallDifference(rhs,Result);
}

// called only from Unparsed::Evaluate(...); guard is IntegerNumeral::IsLegalIntegerString
bool
IntegerNumeral::ConvertToIntegerNumeral(MetaConcept*& dest,const char* Text)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/4/2006
	assert(!dest);
	assert(IsLegalIntegerString(Text));
	try	{
		dest = new IntegerNumeral(Text);
		return true;
		}
	catch(const bad_alloc&)
		{
		return false;
		};
}

bool
IntegerNumeral::DestructiveAddABBothZ(IntegerNumeral*& A, IntegerNumeral*& B)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This function adds the product of A and B to the caller.  If it fails, it still does a partial reduction.
	// MingW doesn't really benefit in code size from explicit goto
	assert(A);
	assert(B);
	if (   A->IsZero()
		|| B->IsZero())
		{
		DELETE_AND_NULL(A);
		DELETE_AND_NULL(B);
		return true;
		}
	if (IsZero())
		{
		if (A->ForceRearrangeProduct(*B))	//! \todo standard order for product?
			{
			A->DestructiveNormalProductFormWithRHS(*B);
			if(A->IsOne()) B->MoveInto(*this);
			assert(B->IsOne());
			A->MoveInto(*this);
			DELETE_AND_NULL(A);
			DELETE_AND_NULL(B);
			return true;
			};
		// local recovery not possible
		return false;
		};
	// more complicated cases
	if (A->ForceRearrangeProduct(*B))
		{
		A->DestructiveNormalProductFormWithRHS(*B);
		if 		(A->IsOne())
			{
			if (ForceRearrangeSum(*B))
				{
				if (IsZero()) B->MoveInto(*this);
				DELETE_AND_NULL(A);
				DELETE_AND_NULL(B);
				return true;
				}
			return false;
			}

		assert(B->IsOne());

		if (ForceRearrangeSum(*A))
			{
			if (IsZero()) A->MoveInto(*this);
			DELETE_AND_NULL(A);
			DELETE_AND_NULL(B);
			return true;
			}
		return false;
		};
	// More complicated -- ForceRearrangeProduct wasn't effective, so we're in RAM trouble
	//! \todo RAM conservation techniques
	//! \todo may want to use loop to control this
	//! <br>basically, we have to reduce the RAM loading enough to permit evaluation
	//! <br>possibilities:
	//! <br>this oppsign A*B: if |this|>|A| or |this|>|B|, can reduce as follows:
	//! <br>c+a*b=(c+a)+a*(b-1)=(c-a)+a*(b+1)
	//! <br>- + + : totally safe
	//! <br>- - - : totally safe (but won't happen: DestructiveNormalForm)
	//! <br>+ + - : totally safe
	//! <br>For same-sign, object is to reduce c in an attempt to free up RAM, or to make further partial evaluations 
	//! more feasible
	//! <br>further partial evaluation: leading base 10^9 digit must be less than 500,000,000; then adjustment by 1
	//! <br>will obtain a factor of 2 to shift upwards.  Larger factors are more effective.
	//! <br>Smaller factor must not be a string of 999,999,999 in base 10^9
	//! <br>readjustment safe through at least |this|<2*max(|A|,|B|)
	//! <br>to make this work:
	//! <br>need fast estimator of 1..9*power-of-ten multiple that can be added/subtracted safely
	//! <br>need destructive addition/subtraction of that 1..9*power-of-ten multiple
	//! <br>need fast estimator of 1..9*power-of-ten that can be added/subtracted safely [alternate comparision interface]
	//! <br>need destructive addition/subtraction of 1..9*power-of-ten
	//! <br>_IntegerNumeral::ResetLHSRHSToGCF will also be interested in these
#if 0
	if (IsNegative())
		{
		if (A->IsPositive() && B->IsPositive())
			{	// c+a*b=(c+a)+a*(b-1); use larger of *A,*B as a against *this, provided absolute value OK
			}
		}
	else	// if (IsPositive())
		{
		if (A->IsPositive())
			{
			if (B->IsNegative())
				{	// c+a*b=(c+a)+a*(b-1); use *B as a against *this, provided absolute value OK
				}
			}
		else{	// if (A->IsNegative())
			if (B->IsPositive())
				{	// c+a*b=(c+a)+a*(b-1); use *A as a against *this, provided absolute value OK
				}
			}
		}	
#endif
	//! <br>+ + + : 
	return false;
}

