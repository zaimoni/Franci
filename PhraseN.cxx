// PhraseN.cxx
// implementation for PhraseNArg, the metatype for n-ary phrases

#include "PhraseN.hxx"
#include "InParse.hxx"
#include "Unparsed.hxx"
#include "Keyword1.hxx"

#include "Zaimoni.STL/except/syntax_error.hpp"
#include <functional>

// PHRASES IMPLEMENTED
// FORALL|THEREIS|FORALLNOT|THEREISNO __ [, ___]...; ___ is a potential variable name
// ....[___,] ___ FREE ; ___ is a potential variable name

// reference tables
#define SyntaxOK_FORALL SyntaxOKAuxCommaListVarNames
#define SyntaxOK_THEREIS SyntaxOKAuxCommaListVarNames
#define SyntaxOK_FREE SyntaxOKAuxCommaListVarNames
#define SyntaxOK_NOTFORALL SyntaxOKAuxCommaListVarNames
#define SyntaxOK_THEREISNO SyntaxOKAuxCommaListVarNames

#define LengthOfSelfName_FORALL LengthOfSelfNamePrefixOrPostfixCommaListVarNames
#define LengthOfSelfName_THEREIS LengthOfSelfNamePrefixOrPostfixCommaListVarNames
#define LengthOfSelfName_FREE LengthOfSelfNamePrefixOrPostfixCommaListVarNames
#define LengthOfSelfName_NOTFORALL LengthOfSelfNamePrefixOrPostfixCommaListVarNames
#define LengthOfSelfName_THEREISNO LengthOfSelfNamePrefixOrPostfixCommaListVarNames

#define ConstructSelfName_FORALL ConstructSelfNamePrefixCommaListVarNames
#define ConstructSelfName_THEREIS ConstructSelfNamePrefixCommaListVarNames
#define ConstructSelfName_FREE ConstructSelfNamePostfixCommaListVarNames
#define ConstructSelfName_NOTFORALL ConstructSelfNamePrefixCommaListVarNames
#define ConstructSelfName_THEREISNO ConstructSelfNamePrefixCommaListVarNames

PhraseNArg::LengthOfSelfNameAuxFunc PhraseNArg::LengthOfSelfNameAuxArray[(MaxPhraseNIdx_MC-MinPhraseNIdx_MC)+1]
	=	{
		&PhraseNArg::LengthOfSelfName_FORALL,
		&PhraseNArg::LengthOfSelfName_THEREIS,
		&PhraseNArg::LengthOfSelfName_FREE,
		&PhraseNArg::LengthOfSelfName_NOTFORALL,
		&PhraseNArg::LengthOfSelfName_THEREISNO
		};

PhraseNArg::ConstructSelfNameAuxFunc PhraseNArg::ConstructSelfNameAuxArray[(MaxPhraseNIdx_MC-MinPhraseNIdx_MC)+1]
	=	{
		&PhraseNArg::ConstructSelfName_FORALL,
		&PhraseNArg::ConstructSelfName_THEREIS,
		&PhraseNArg::ConstructSelfName_FREE,
		&PhraseNArg::ConstructSelfName_NOTFORALL,
		&PhraseNArg::ConstructSelfName_THEREISNO
		};

PhraseNArg::SyntaxOKAuxFunc PhraseNArg::SyntaxOKAuxArray[(MaxPhraseNIdx_MC-MinPhraseNIdx_MC)+1]
	=	{
		&PhraseNArg::SyntaxOK_FORALL,
		&PhraseNArg::SyntaxOK_THEREIS,
		&PhraseNArg::SyntaxOK_FREE,
		&PhraseNArg::SyntaxOK_NOTFORALL,
		&PhraseNArg::SyntaxOK_THEREISNO
		};

#undef ConstructSelfName_FORALL
#undef ConstructSelfName_THEREIS
#undef ConstructSelfName_FREE
#undef ConstructSelfName_NOTFORALL
#undef ConstructSelfName_THEREISNO

#undef AddLengthOfSelfName_FORALL
#undef AddLengthOfSelfName_THEREIS
#undef AddLengthOfSelfName_FREE
#undef AddLengthOfSelfName_NOTFORALL
#undef AddLengthOfSelfName_THEREISNO

#undef SyntaxOK_FORALL
#undef SyntaxOK_THEREIS
#undef SyntaxOK_FREE
#undef SyntaxOK_NOTFORALL
#undef SyntaxOK_THEREISNO

//! \todo support:
//! <br>SERIES(+,Quantifier,ExplicitConstant,Expression)
//! <br>SERIES(+,Quantifier,LinearInterval with domain _Z_,Expression)
//! <br>SERIES(+,Quantifier,NULLSet,Expression)
//! <br>SERIES(+,Var=ExplicitConstant,Expression)
//! <br>SERIES(+,Var=LinearInterval with domain _Z_,Expression)
//! <br>ditto for &middot; and relatives
//! <br>When we have finite sets:
//! <br>SERIES(+,Quantifier,FiniteSet,Expression)

// rest of implementation
PhraseNArg::PhraseNArg(MetaConcept**& src, size_t& KeywordIdx)
:	MetaConceptWithArgArray(CanConstruct(src,KeywordIdx))
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/1999
	assert(FORALL_PhraseN_MC<=ExactType() && THEREISNO_PhraseN_MC>=ExactType());
	PhraseKeyword = QuantificationNames[ExactType()-FORALL_PhraseN_MC];
	if (FREE_PhraseN_MC==ExactType())
		ExtractPostfixCommaListVarNames(src,KeywordIdx);
	else
		ExtractPrefixCommaListVarNames(src,KeywordIdx);
	return;
}

//  Type ID functions
void PhraseNArg::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/2000
	ForceStdFormAux();
	if (IsSymmetric()) ForceTotalLexicalArgOrder();
}

//  Evaluation functions
bool PhraseNArg::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	if (SyntaxOKAux() && NULL!=PhraseKeyword)
		return (this->*SyntaxOKAuxArray[ExactType()-MinPhraseNIdx_MC])();
	return false;
}

bool PhraseNArg::SyntaxOKAuxCommaListVarNames() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/27/2007
	return and_range_n([](const MetaConcept* x) { return x->IsPotentialVarName(); }, ArgArray.begin(), fast_size());
}

const char* PhraseNArg::ViewKeyword() const {return PhraseKeyword;}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > PhraseNArg::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

// type-specific functions
void PhraseNArg::DiagnoseInferenceRules() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/21/2000
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

bool PhraseNArg::InvokeEqualArgRule() const {return false;}

ExactType_MC
PhraseNArg::CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/16/2006
	assert(NULL!=src);
	assert(ArraySize(src)>KeywordIdx);
	assert(NULL!=src[KeywordIdx]);
	const UnparsedText* TargetArg = UnparsedText::fast_up_cast(src[KeywordIdx]);
	if (NULL==TargetArg) return Unknown_MC;

	// these are Franci's recognizers for
	//	FORALL, THEREIS, NOTFORALL, THEREISNO
	//  all of these are comma-ized, and prefix
	// low-level optimization: FREE is at index 2
	unsigned int i = 5;
	do	if (2!=--i && TargetArg->IsPredCalcKeyword(QuantificationNames[i]))
			{
			if (KeywordIdx+1==ArraySize(src))
				{
				std::string error_msg("Quantifier applies to no variables: ");
				error_msg += TargetArg->ViewKeyword();
				throw syntax_error(error_msg);
				}

			if (PrefixCommaListVarNamesRecognize(src,KeywordIdx))
				return (ExactType_MC)(FORALL_PhraseN_MC+i);
			}
	while(0<i);
	return Unknown_MC;
}

// This is just 'dominantly postfix' phrases.
ExactType_MC
PhraseNArg::CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/16/2006
	assert(NULL!=src);
	assert(ArraySize(src)>KeywordIdx);
	assert(NULL!=src[KeywordIdx]);
	const UnparsedText* TargetArg = UnparsedText::fast_up_cast(src[KeywordIdx]);
	if (NULL!=TargetArg)
		{
		// these are Franci's recognizers for
		//  FREE: comma-ized
		if (TargetArg->IsPredCalcKeyword(PredCalcKeyword_FREE))
			{
			if (0==KeywordIdx)
				{
				std::string error_msg("Quantifier applies to no variables: ");
				error_msg += TargetArg->ViewKeyword();
				throw syntax_error(error_msg);
				}
				
			if (PostfixCommaListVarNamesRecognize(src,KeywordIdx))
				return FREE_PhraseN_MC;
			};
		}
	return Unknown_MC;
}

// must not throw, only used in PhraseNArg constructor
ExactType_MC
PhraseNArg::CanConstruct(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/27/2006
	assert(NULL!=src);
	assert(ArraySize(src)>KeywordIdx);
	assert(NULL!=src[KeywordIdx]);
	try	{
		ExactType_MC Tmp = CanConstructNonPostfix(src,KeywordIdx);
		if (Unknown_MC!=Tmp) return Tmp;
		Tmp = CanConstructPostfix(src,KeywordIdx);
		SUCCEED_OR_DIE(Unknown_MC!=Tmp);
		return Tmp;
		}
	catch(const syntax_error&)
		{
		FATAL(AlphaRetValAssumption);
		}
}

void
PhraseNArg::ExtractPrefixCommaListVarNames(MetaConcept**& Target, size_t& KeywordIdx)
{	// FORMALLY CORRECT: 10/16/2000
	size_t NewSize = PrefixCommaListVarNamesRecognize(Target,KeywordIdx);
	if (0<NewSize)
		{
		if (!InsertNSlotsAtV2(NewSize,0)) UnconditionalRAMFailure();
		size_t i = 0;
		do	{
			ArgArray[i] = Target[KeywordIdx+2*i+1];
			Target[KeywordIdx+2*i+1] = NULL;
			}
		while(NewSize> ++i);
		SUCCEED_OR_DIE(SyntaxOK());
		};	
	delete Target[KeywordIdx];
	Target[KeywordIdx] = this;
	if (0<NewSize)
		_delete_n_slots_at(Target,2*NewSize-1,KeywordIdx+1);
	_forceStdForm();
}

void
PhraseNArg::ExtractPostfixCommaListVarNames(MetaConcept**& Target, size_t& KeywordIdx)
{	// FORMALLY CORRECT: 7/22/1999
	size_t NewSize = PostfixCommaListVarNamesRecognize(Target,KeywordIdx);
	if (0<NewSize)
		{
		if (!InsertNSlotsAtV2(NewSize,0)) UnconditionalRAMFailure();
		size_t i = 0;
		//! \bug raw order is flipped.  This is not critical as long as only
		//! Symmetric keywords use this routine.
		do	{
			ArgArray[i] = Target[KeywordIdx-2*i-1];
			Target[KeywordIdx-2*i-1] = NULL;
			}
		while(NewSize> ++i);
		SUCCEED_OR_DIE(SyntaxOK());
		};
	delete Target[KeywordIdx];
	Target[KeywordIdx] = this;
	if (0<NewSize)
		_delete_n_slots_at(Target,2*NewSize-1,KeywordIdx-(2*NewSize-1));
	_forceStdForm();

	KeywordIdx -= 2*NewSize-1;
}

