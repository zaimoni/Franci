// Phrase1.cxx
// implementation for Phrase1Arg, the metatype for 1-ary phrases

#include "Class.hxx"
#include "Unparsed.hxx"
#include "Integer1.hxx"
#include "Variable.hxx"
#include "Interval.hxx"
#include "Combin1.hxx"
#include "Phrase1.hxx"
#include "Keyword1.hxx"

// Currently supported phrases
//	IN ___, ___ an AbstractClass	[IN-phrase]
//	** the arg is accessible to infix functions that use the IN-phrase as the 1st arg,
//	** the arg is accessible to unary-postfix functions that do not parenthesize the arg

// reference tables
#define DECLARE_VFT(A)	\
  { &Phrase1Arg::SyntaxOK_##A, &Phrase1Arg::LengthOfSelfName_##A, &Phrase1Arg::ConstructSelfName_##A }

#define LengthOfSelfName_IN LengthOfSelfName_Prefix
#define ConstructSelfName_IN ConstructSelfName_Prefix
#define LengthOfSelfName_FACTORIAL LengthOfSelfName_FunctionLike
#define ConstructSelfName_FACTORIAL ConstructSelfName_FunctionLike

const Phrase1Arg::Phrase1ArgVFT Phrase1Arg::VFTable2Lookup[(MaxPhrase1Idx_MC-MinPhrase1Idx_MC)+1]
	=	{	DECLARE_VFT(IN),
			DECLARE_VFT(FACTORIAL)
		};

#undef ConstructSelfName_FACTORIAL
#undef LengthOfSelfName_FACTORIAL
#undef ConstructSelfName_IN
#undef LengthOfSelfName_IN

#undef DECLARE_VFT

// rest of implementation
Phrase1Arg::Phrase1Arg(MetaConcept**& src, size_t KeywordIdx)
:	MetaConceptWith1Arg(CanConstruct(src,KeywordIdx)),
	PhraseKeyword(NULL)
{	// NOTE: constructor fails by terminating the program with prejudice
	// FORMALLY CORRECT: Kenneth Boyd, 7/22/1999
	// MUTABLE
	switch(ExactType())
	{
	case IN_Phrase1_MC:
		PhraseKeyword = PredCalcKeyword_IN;
		if (KeywordIdx+1<ArraySize(src))
			{
			Arg1 = src[KeywordIdx+1];
			src[KeywordIdx+1] = NULL;
			SUCCEED_OR_DIE(SyntaxOK());
			};		
		DELETE(src[KeywordIdx]);
		src[KeywordIdx] = this;
		_delete_idx(src,KeywordIdx+1);
		return;
	case FACTORIAL_Phrase1_MC:
		PhraseKeyword = PrefixKeyword_FACTORIAL;
		Arg1 = src[KeywordIdx+2];
		src[KeywordIdx+2] = NULL;
		DELETE(src[KeywordIdx]);
		src[KeywordIdx] = this;
		_delete_n_slots_at(src,3,KeywordIdx+1);
		return;
	default:
		FATAL(AlphaRetValAssumption);
		return;
	};
}

bool Phrase1Arg::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	if (   MetaConceptWith1Arg::EqualAux2(rhs)
		&& 0==strcmp(PhraseKeyword,static_cast<const Phrase1Arg&>(rhs).PhraseKeyword))
		return true;
	return false;
}

bool Phrase1Arg::CanEvaluate() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	if (IsExactType(FACTORIAL_Phrase1_MC)) return true;
	return false;
}

// FORMALLY CORRECT: Kenneth Boyd, 7/12/1999
bool Phrase1Arg::CanEvaluateToSameType() const {return false;}

bool Phrase1Arg::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/22/1999
	if (   NULL==PhraseKeyword || NULL==Arg1 || !Arg1->SyntaxOK())
		return false;
	return (this->*VFTable2Lookup[ExactType()-MinPhrase1Idx_MC].SyntaxOK)();
}

bool Phrase1Arg::SyntaxOK_IN() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/22/1999
	return Arg1->IsExactType(AbstractClass_MC);
}

bool Phrase1Arg::SyntaxOK_FACTORIAL() const
{	//! \todo SyntaxOK_FACTORIAL should compare against 0...&infin;
	return Integer.HasAsElement(*Arg1);
//	return NonnegativeInteger().HasAsElement(*Arg1);
}

bool Phrase1Arg::Evaluate(MetaConcept*& dest)	// same, or different type
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	assert(NULL==dest);
	if (IsExactType(FACTORIAL_Phrase1_MC))
		{	
		weakautoarray_ptr<MetaConcept*> NewArgArray(1);
		if (NewArgArray.empty()) return false;
		NewArgArray[0] = Arg1;
		dest = new CombinatorialLike(NewArgArray,FACTORIAL_CM);
		Arg1.NULLPtr();
		assert(dest->SyntaxOK());
		return true;
		}
	return false;
}

// FORMALLY CORRECT: Kenneth Boyd, 7/12/1999
bool Phrase1Arg::DestructiveEvaluateToSameType() {return false;}

const char* Phrase1Arg::ViewKeyword() const {return PhraseKeyword;}

ExactType_MC
Phrase1Arg::CanConstruct(const MetaConcept * const * TargetArray, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2003
	assert(NULL!=TargetArray);
	assert(ArraySize(TargetArray)>KeywordIdx);
	assert(NULL!=TargetArray[KeywordIdx]);
	const UnparsedText* const TargetArg = UnparsedText::up_cast(TargetArray[KeywordIdx]);
	if (NULL==TargetArg) return Unknown_MC;

	// This is Franci's IN ___ recognizer
	// META: It has a theoretical problem that isn't critical right now: it cannot cope
	// with functors that return classes or sets.  These functors are legitimate
	// args for IN, but this recognizer cannot deal with them.
	if (TargetArg->IsPredCalcKeyword(PredCalcKeyword_IN))
		{
		const MetaConcept* Arg = TargetArray[KeywordIdx+1];
		if (NULL==Arg)
			return (KeywordIdx+1==ArraySize(TargetArray)) ? IN_Phrase1_MC : Unknown_MC;
		else
			return (Arg->IsExactType(AbstractClass_MC)) ? IN_Phrase1_MC : Unknown_MC;
		};
	// recognize FACTORIAL
	if (   KeywordIdx+3<ArraySize(TargetArray)
		&& TargetArg->IsPrefixKeyword(PrefixKeyword_FACTORIAL))
		{	// FACTORIAL is a 1-ary function that accepts non-negative integers.
		const MetaConcept* LParenArg = TargetArray[KeywordIdx+1];
		const MetaConcept* RParenArg = TargetArray[KeywordIdx+3];
		if (    LParenArg->IsExactType(UnparsedText_MC)
			&&  static_cast<const UnparsedText*>(LParenArg)->IsSemanticChar('(')
			&&  RParenArg->IsExactType(UnparsedText_MC)
			&&  static_cast<const UnparsedText*>(RParenArg)->IsSemanticChar(')'))
			{	//! \todo argument tolerance
				//! We (will) accept: variable-like text
				//!  (turn it into correct domain; this requires support of intervals as abstract classes)
				//!  (We'll probably have to boost StdAddition, StdMultiplication to handle
				//!   algebraic operations on classes first)
				//!  [Factorial(X) [X in 0...&infin;]] : OK
				//!  [Factorial(-X) [X in -&infin...0]] : need support from Variable's ForceUltimateType, AbstractClass
				//!  [Factorial(X+const) [X in -const...&infin;]] : need support from StdAddition's ForceUltimateType, AbstractClass
				//!  [Factorial(-X+const) [X in -&infin...const]] : .....
				//!   currently do not handle multiplication, multinv, etc.
				//!   To do this right for single variables, we would support 
				//!   elementwise mappings as abstract functions on sets
				//!   Multiple variables will be trickier
				//! Anything whose domain is a subclass of 0...&infin;
				//! Integer constants in 0...&infin;
				//! We'd rather not force domain of +,*, etc.
			const MetaConcept* ParameterArg = TargetArray[KeywordIdx+2];
			if (!ParameterArg->IsNegative())
				{
				if (ParameterArg->IsExactType(LinearInfinity_MC))
					return FACTORIAL_Phrase1_MC;
				else if (ParameterArg->IsExactType(IntegerNumeral_MC))
					{
					if (ParameterArg->IsUltimateType(&Integer))
						return FACTORIAL_Phrase1_MC;
					}
				// TODO: give variables a chance...after we can do something with them.
				}
			// TODO: construct system that informs Target of syntax errors.
			if (!ParameterArg->IsExactType(UnparsedText_MC))
				WARNING("Syntax Error: FACTORIAL requires a non-negative integer type for its argument.");

			return Unknown_MC;	// negative isn't acceptable
			}
		};
	return Unknown_MC;
}

