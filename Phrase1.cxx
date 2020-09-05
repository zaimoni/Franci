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
#define DECLARE_VFT(A) { &Phrase1Arg::SyntaxOK_##A }

const Phrase1Arg::Phrase1ArgVFT Phrase1Arg::VFTable2Lookup[(MaxPhrase1Idx_MC-MinPhrase1Idx_MC)+1]
	=	{	DECLARE_VFT(IN)
		};

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

ExactType_MC Phrase1Arg::CanConstruct(const MetaConcept * const * TargetArray, size_t KeywordIdx)
{	// FORMALLY CORRECT: 2020-07-29
	assert(TargetArray);
	assert(ArraySize(TargetArray)>KeywordIdx);
	assert(TargetArray[KeywordIdx]);
	const auto TargetArg = up_cast<UnparsedText>(TargetArray[KeywordIdx]);
	if (!TargetArg) return Unknown_MC;

	// This is Franci's IN ___ recognizer
	// META: It has a theoretical problem that isn't critical right now: it cannot cope
	// with functors that return classes or sets.  These functors are legitimate
	// args for IN, but this recognizer cannot deal with them.
	if (TargetArg->IsPredCalcKeyword(PredCalcKeyword_IN))
		{
		if (auto Arg = TargetArray[KeywordIdx + 1])
			return (Arg->IsExactType(AbstractClass_MC)) ? IN_Phrase1_MC : Unknown_MC;
		else
			return (KeywordIdx + 1 == ArraySize(TargetArray)) ? IN_Phrase1_MC : Unknown_MC;
		};
	return Unknown_MC;
}

