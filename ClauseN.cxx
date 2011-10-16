// ClauseN.cxx
// implementation for ClauseNArg, the metatype for n-ary clauses

#include "ClauseN.hxx"

#include "Class.hxx"
#include "Unparsed.hxx"
#include "InParse.hxx"
#include "Interval.hxx"
#include "Keyword1.hxx"
#include "LowRel.hxx"
#include "MetaCon3.hxx"
#include "Equal.hxx"

#include "Zaimoni.STL/except/syntax_error.hpp"
#include "Zaimoni.STL/limits"

// defined in LexParse.cxx
bool ImproviseVar(MetaConcept*& dest, const AbstractClass* Domain);

ClauseNArg::EvaluateToOtherRule ClauseNArg::EvaluateRuleLookup[ClauseNArg::MaxEvalRuleIdx_ER]
  =	{
	&ClauseNArg::ConvertToMetaConnective,
	&ClauseNArg::ConvertToEqualRelation
	};

#define LengthOfSelfName_LogicalAND LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_LogicalOR LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_LogicalIFF LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_LogicalXOR LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_LogicalNXOR LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_LogicalNIFF LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_LogicalNOR LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_LogicalNAND LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_ALLEQUAL LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_ALLDISTINCT LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_DISTINCTFROMALLOF LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_EQUALTOONEOF LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_NOTALLDISTINCT LengthOfSelfNamePrefixArglist
#define LengthOfSelfName_NOTALLEQUAL LengthOfSelfNamePrefixArglist

#define ConstructSelfName_LogicalAND ConstructSelfNamePrefixArglist
#define ConstructSelfName_LogicalOR ConstructSelfNamePrefixArglist
#define ConstructSelfName_LogicalIFF ConstructSelfNamePrefixArglist
#define ConstructSelfName_LogicalXOR ConstructSelfNamePrefixArglist
#define ConstructSelfName_LogicalNXOR ConstructSelfNamePrefixArglist
#define ConstructSelfName_LogicalNIFF ConstructSelfNamePrefixArglist
#define ConstructSelfName_LogicalNOR ConstructSelfNamePrefixArglist
#define ConstructSelfName_LogicalNAND ConstructSelfNamePrefixArglist
#define ConstructSelfName_ALLEQUAL ConstructSelfNamePrefixArglist
#define ConstructSelfName_ALLDISTINCT ConstructSelfNamePrefixArglist
#define ConstructSelfName_DISTINCTFROMALLOF ConstructSelfNamePrefixArglist
#define ConstructSelfName_EQUALTOONEOF ConstructSelfNamePrefixArglist
#define ConstructSelfName_NOTALLDISTINCT ConstructSelfNamePrefixArglist
#define ConstructSelfName_NOTALLEQUAL ConstructSelfNamePrefixArglist

#define SyntaxOK_LogicalAND SyntaxOKArglistTVal
#define SyntaxOK_LogicalOR SyntaxOKArglistTVal
#define SyntaxOK_LogicalIFF SyntaxOKArglistTVal
#define SyntaxOK_LogicalXOR SyntaxOKArglistTVal
#define SyntaxOK_LogicalNXOR SyntaxOKArglistTVal
#define SyntaxOK_LogicalNIFF SyntaxOKArglistTVal
#define SyntaxOK_LogicalNOR SyntaxOKArglistTVal
#define SyntaxOK_LogicalNAND SyntaxOKArglistTVal
#define SyntaxOK_ALLEQUAL SyntaxOKNoExtraInfo
#define SyntaxOK_ALLDISTINCT SyntaxOKNoExtraInfo
#define SyntaxOK_DISTINCTFROMALLOF SyntaxOKNoExtraInfo
#define SyntaxOK_EQUALTOONEOF SyntaxOKNoExtraInfo
#define SyntaxOK_NOTALLDISTINCT SyntaxOKNoExtraInfo
#define SyntaxOK_NOTALLEQUAL SyntaxOKNoExtraInfo

ClauseNArg::LengthOfSelfNameAuxFunc ClauseNArg::LengthOfSelfNameAuxArray[(MaxClauseNIdx_MC-MinClauseNIdx_MC)+1]
	=	{
		&ClauseNArg::LengthOfSelfName_LogicalAND,
		&ClauseNArg::LengthOfSelfName_LogicalOR,
		&ClauseNArg::LengthOfSelfName_LogicalIFF,
		&ClauseNArg::LengthOfSelfName_LogicalXOR,
		&ClauseNArg::LengthOfSelfName_LogicalNXOR,
		&ClauseNArg::LengthOfSelfName_LogicalNIFF,
		&ClauseNArg::LengthOfSelfName_LogicalNOR,
		&ClauseNArg::LengthOfSelfName_LogicalNAND,
		&ClauseNArg::LengthOfSelfName_ALLEQUAL,
		&ClauseNArg::LengthOfSelfName_ALLDISTINCT,
		&ClauseNArg::LengthOfSelfName_DISTINCTFROMALLOF,
		&ClauseNArg::LengthOfSelfName_EQUALTOONEOF,
		&ClauseNArg::LengthOfSelfName_NOTALLDISTINCT,
		&ClauseNArg::LengthOfSelfName_NOTALLEQUAL
		};

ClauseNArg::ConstructSelfNameAuxFunc ClauseNArg::ConstructSelfNameAuxArray[(MaxClauseNIdx_MC-MinClauseNIdx_MC)+1]
	=	{
		&ClauseNArg::ConstructSelfName_LogicalAND,
		&ClauseNArg::ConstructSelfName_LogicalOR,
		&ClauseNArg::ConstructSelfName_LogicalIFF,
		&ClauseNArg::ConstructSelfName_LogicalXOR,
		&ClauseNArg::ConstructSelfName_LogicalNXOR,
		&ClauseNArg::ConstructSelfName_LogicalNIFF,
		&ClauseNArg::ConstructSelfName_LogicalNOR,
		&ClauseNArg::ConstructSelfName_LogicalNAND,
		&ClauseNArg::ConstructSelfName_ALLEQUAL,
		&ClauseNArg::ConstructSelfName_ALLDISTINCT,
		&ClauseNArg::ConstructSelfName_DISTINCTFROMALLOF,
		&ClauseNArg::ConstructSelfName_EQUALTOONEOF,
		&ClauseNArg::ConstructSelfName_NOTALLDISTINCT,
		&ClauseNArg::ConstructSelfName_NOTALLEQUAL
		};

ClauseNArg::SyntaxOKAuxFunc ClauseNArg::SyntaxOKAuxArray[(MaxClauseNIdx_MC-MinClauseNIdx_MC)+1]
	=	{
		&ClauseNArg::SyntaxOK_LogicalAND,
		&ClauseNArg::SyntaxOK_LogicalOR,
		&ClauseNArg::SyntaxOK_LogicalIFF,
		&ClauseNArg::SyntaxOK_LogicalXOR,
		&ClauseNArg::SyntaxOK_LogicalNXOR,
		&ClauseNArg::SyntaxOK_LogicalNIFF,
		&ClauseNArg::SyntaxOK_LogicalNOR,
		&ClauseNArg::SyntaxOK_LogicalNAND,
		&ClauseNArg::SyntaxOK_ALLEQUAL,
		&ClauseNArg::SyntaxOK_ALLDISTINCT,
		&ClauseNArg::SyntaxOK_DISTINCTFROMALLOF,
		&ClauseNArg::SyntaxOK_EQUALTOONEOF,
		&ClauseNArg::SyntaxOK_NOTALLDISTINCT,
		&ClauseNArg::SyntaxOK_NOTALLEQUAL
		};

#undef SyntaxOK_LogicalAND
#undef SyntaxOK_LogicalOR
#undef SyntaxOK_LogicalIFF
#undef SyntaxOK_LogicalXOR
#undef SyntaxOK_LogicalNXOR
#undef SyntaxOK_LogicalNIFF
#undef SyntaxOK_LogicalNOR
#undef SyntaxOK_LogicalNAND
#undef SyntaxOK_ALLEQUAL
#undef SyntaxOK_ALLDISTINCT
#undef SyntaxOK_DISTINCTFROMALLOF
#undef SyntaxOK_EQUALTOONEOF
#undef SyntaxOK_NOTALLDISTINCT
#undef SyntaxOK_NOTALLEQUAL

#undef ConstructSelfName_LogicalAND
#undef ConstructSelfName_LogicalOR
#undef ConstructSelfName_LogicalIFF
#undef ConstructSelfName_LogicalXOR
#undef ConstructSelfName_LogicalNXOR
#undef ConstructSelfName_LogicalNIFF
#undef ConstructSelfName_LogicalNOR
#undef ConstructSelfName_LogicalNAND
#undef ConstructSelfName_ALLEQUAL
#undef ConstructSelfName_ALLDISTINCT
#undef ConstructSelfName_DISTINCTFROMALLOF
#undef ConstructSelfName_EQUALTOONEOF
#undef ConstructSelfName_NOTALLDISTINCT
#undef ConstructSelfName_NOTALLEQUAL

#undef LengthOfSelfName_LogicalAND
#undef LengthOfSelfName_LogicalOR
#undef LengthOfSelfName_LogicalIFF
#undef LengthOfSelfName_LogicalXOR
#undef LengthOfSelfName_LogicalNXOR
#undef LengthOfSelfName_LogicalNIFF
#undef LengthOfSelfName_LogicalNOR
#undef LengthOfSelfName_LogicalNAND
#undef LengthOfSelfName_ALLEQUAL
#undef LengthOfSelfName_ALLDISTINCT
#undef LengthOfSelfName_DISTINCTFROMALLOF
#undef LengthOfSelfName_EQUALTOONEOF
#undef LengthOfSelfName_NOTALLDISTINCT
#undef LengthOfSelfName_NOTALLEQUAL

//! \todo infix operator support: ==, !=

static const char* const ClauseNAryKeywordLookup[MaxClauseNIdx_MC-MinClauseNIdx_MC+1]
  =	{
	LogicKeyword_AND,
	LogicKeyword_OR,
	LogicKeyword_IFF,
	LogicKeyword_XOR,
	LogicKeyword_NXOR,
	LogicKeyword_NIFF,
	LogicKeyword_NOR,
	LogicKeyword_NAND,
	EqualRelation_ALLEQUAL,
	EqualRelation_ALLDISTINCT,
	EqualRelation_EQUALTOONEOF,
	EqualRelation_DISTINCTFROMALLOF,
	EqualRelation_NOTALLDISTINCT,
	EqualRelation_NOTALLEQUAL
	};

// rest of implementation
ClauseNArg::ClauseNArg(MetaConcept**& src, size_t& KeywordIdx)
:	MetaConceptWithArgArray(CanConstruct(src,KeywordIdx))
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/1999
	assert(MinClauseNIdx_MC<=ExactType() && MaxClauseNIdx_MC>=ExactType());
	ClauseKeyword = ClauseNAryKeywordLookup[array_index()];
	if (IsExactType(EQUALTOONEOF_ClauseN_MC) || IsExactType(DISTINCTFROMALLOF_ClauseN_MC))
		Extract1ArgBeforePrefixCommalist(src,KeywordIdx);
	else
		ExtractPrefixArglist(src,KeywordIdx);
	// If the syntax is already bad, then we don't need to proceed further
	if (!SyntaxOK()) return;
	ForceStdForm();
}

void ClauseNArg::MoveInto(ClauseNArg*& dest)		// can throw memory failure.  If it succeeds, it destroys the source.
{	// FORMALLY CORRECT: Kenneth Boyd, 3/17/2000
	//! \todo retrofit most other types
	if (!dest) dest = new ClauseNArg();
	dest->VFTable1=VFTable1;
	dest->ClauseKeyword=ClauseKeyword;
	MoveIntoAux(*dest);
}

//  Type ID functions
// FORMALLY CORRECT: 8/8/1999
const AbstractClass* ClauseNArg::UltimateType() const {return &TruthValues;}

// FORMALLY CORRECT: Kenneth Boyd, 4/23/2000
void ClauseNArg::_forceStdForm() {ForceStdFormAux();}

//  Evaluation functions
bool ClauseNArg::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/11/1999
	if (SyntaxOKAux() && NULL!=ClauseKeyword)
		return (this->*SyntaxOKAuxArray[array_index()])();
	return false;
}

bool ClauseNArg::SyntaxOKArglistTVal() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2000
	// we accept an argument as legal if:
	// 1) UltimateType TruthValues, or
	// 2) Interpretable as a variable name of type TruthValues
	return and_range_n(ForceUltimateTypeTruthValues,(MetaConcept**&)ArgArray,fast_size());
}

bool ClauseNArg::SyntaxOKNoExtraInfo() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2000
	size_t i = fast_size();
	do	if (   ArgArray[--i]->IsExactType(UnparsedText_MC)
	        && ArgArray[i]->IsPotentialVarName()
			&& !ImproviseVar(ArgArray[i],NULL))
			return false;
	while(0<i);
	return true;
}

const char* ClauseNArg::ViewKeyword() const {return ClauseKeyword;}

static ExactType_MC SelfLogicalNOTLookup_ClauseN[(MaxClauseNIdx_MC-MinClauseNIdx_MC)+1]	=
	{	LogicalNAND_ClauseN_MC,
		LogicalNOR_ClauseN_MC,
		LogicalNIFF_ClauseN_MC,
		LogicalNXOR_ClauseN_MC,
		LogicalXOR_ClauseN_MC,
		LogicalIFF_ClauseN_MC,
		LogicalOR_ClauseN_MC,
		LogicalAND_ClauseN_MC,
		NOTALLEQUAL_ClauseN_MC,
		NOTALLDISTINCT_ClauseN_MC,
		DISTINCTFROMALLOF_ClauseN_MC,
		EQUALTOONEOF_ClauseN_MC,
		ALLDISTINCT_ClauseN_MC,
		ALLEQUAL_ClauseN_MC	};

void ClauseNArg::SelfLogicalNOT()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/16/1999
	SetExactType(SelfLogicalNOTLookup_ClauseN[array_index()]);
}

void ClauseNArg::DiagnoseInferenceRules() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/11/1999
	if (LogicalAND_ClauseN_MC<=ExactType() && LogicalNAND_ClauseN_MC>=ExactType())
		{
		IdxCurrentEvalRule=ConvertToMetaConnective_ER;
		return;
		};
	if (ALLEQUAL_ClauseN_MC<=ExactType() && NOTALLEQUAL_ClauseN_MC>=ExactType())
		{
		IdxCurrentEvalRule=ConvertToEqualRelation_ER;
		return;
		};
	UnconditionalDataIntegrityFailure();
}

bool ClauseNArg::InvokeEqualArgRule() const {return false;}

// #define FRANCI_WARY 1

ExactType_MC
ClauseNArg::CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: 2/8/2000
	// AND(..), OR(...), IFF(...), XOR(...), NXOR(...), NIFF(...), NOR(...), NAND(...)
	// all of these have minarity 2, and arglists that must be ultimately TruthValues
	// ALLEQUAL(...), ALLDISTINCT(...), NOTALLEQUAL(...), NOTALLDISTINCT(...)
	// all of these have minarity 2
	assert(src);
	assert(ArraySize(src)>KeywordIdx);
	assert(src[KeywordIdx]);
	const UnparsedText* const VRKeyword = UnparsedText::fast_up_cast(src[KeywordIdx]);
	if (NULL==VRKeyword) return Unknown_MC;

	const ExactType_MC GuessType = VRKeyword->TypeForNAryClauseKeyword();
	if (Unknown_MC!=GuessType)
		{
		if (   DISTINCTFROMALLOF_ClauseN_MC==GuessType
			|| EQUALTOONEOF_ClauseN_MC==GuessType)
			{
			if (	0==KeywordIdx
				||	KeywordIdx+1==ArraySize(src))
				{
				std::string error_msg("Malformed infix clause: ");
				error_msg += VRKeyword->ViewKeyword();
				throw syntax_error(error_msg);
				}
			if (   1<=KeywordIdx
				&& CommalistAry2PlusRecognize(src,KeywordIdx+1))
				return GuessType;
			}
		else{
			if (ArglistAry2PlusRecognize(src,KeywordIdx+1))
				return GuessType;
			if (	ALLEQUAL_ClauseN_MC<=GuessType
				&& 	KeywordIdx+2<ArraySize(src)
				&& (	!src[KeywordIdx+1]->IsExactType(UnparsedText_MC)
					|| 	!static_cast<const UnparsedText*>(src[KeywordIdx+1])->IsSemanticChar('(')))
				{
				std::string error_msg("Malformed prefix clause: ");
				error_msg += VRKeyword->ViewKeyword();
				throw syntax_error(error_msg);
				}
			}
		};
	return Unknown_MC;
}

#undef FRANCI_WARY

ExactType_MC
ClauseNArg::CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: 8/11/1999
	return Unknown_MC;
}

// must not throw, called only from constructor
ExactType_MC
ClauseNArg::CanConstruct(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/27/2006
	assert(src);
	assert(ArraySize(src)>KeywordIdx);
	assert(src[KeywordIdx]);
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

void ClauseNArg::ExtractPrefixArglist(MetaConcept**& src, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/25/2002
	// This is only supposed to be called from the constructor.
	// This may belong in MetaConceptWithArgArray
	// 1) find )
	size_t i = KeywordIdx+3;
	bool ThisIsLogicalEllipsis = false;
	bool PriorIsLogicalEllipsis = false;
	size_t ArrayCorrection = 0;

	do	{
		const UnparsedText& VRArgN = UnparsedText::fast_up_reference(src[i]);
		if (VRArgN.IsSemanticChar(')')) break;
		PriorIsLogicalEllipsis = ThisIsLogicalEllipsis;
		ThisIsLogicalEllipsis = VRArgN.IsLogicalEllipsis();
		if (ThisIsLogicalEllipsis)
			{
			SUCCEED_OR_DIE(!PriorIsLogicalEllipsis);
			SUCCEED_OR_DIE(src[i-1]->IsUltimateType(&Integer) && src[i+1]->IsUltimateType(&Integer));
			ArrayCorrection++;
			}
		else{
			SUCCEED_OR_DIE(VRArgN.IsSemanticChar(','));
			}
		}
	while(ArraySize(src)>(i+=2));
	SUCCEED_OR_DIE(ArraySize(src)>i);
	// 2) create space for args
	if (!InsertNSlotsAtV2((i-KeywordIdx-1)/2-ArrayCorrection,0))
		UnconditionalRAMFailure();
	// 3) Move args in
	{
	size_t j = KeywordIdx+2;
	size_t ArrayIdx = 0;
	// Review args to see which were connected by ellipsis.  Create LinearIntervals of 
	// UltimateType _Z_ when indicated.
	do	{
		if (static_cast<UnparsedText*>(src[j+1])->IsLogicalEllipsis())
			LinearIntervalInit(src,j,ArgArray[ArrayIdx++]);
		else{
			ArgArray[ArrayIdx++] = src[j];
			src[j] = NULL;
			}
		}
	while(i>(j+=2));
	}
	// 4) Move *this into Target
	delete src[KeywordIdx];
	src[KeywordIdx] = this;
	// 5) Clean Arglist space of target
	_delete_n_slots_at(src,i-KeywordIdx,KeywordIdx+1);
	// 6) do simple transitions
	i = fast_size();
	do	if (ArgArray[--i]->HasSimpleTransition())
			DestructiveSyntacticallyEvaluateOnce(ArgArray[i]);
	while(0<i);
}	

void
ClauseNArg::Extract1ArgBeforePrefixCommalist(MetaConcept**& src, size_t& KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/25/2002
	// This is only supposed to be called from the constructor.
	// This may belong in MetaConceptWithArgArray
	// 1) find end of comma-list
	size_t FinalCommaIdx = KeywordIdx+2;
	SUCCEED_OR_DIE(ArraySize(src)>KeywordIdx+3);
	const UnparsedText& FinalCommaProxy = UnparsedText::fast_up_reference(src[FinalCommaIdx]);
	SUCCEED_OR_DIE(FinalCommaProxy.IsSemanticChar(',') || FinalCommaProxy.IsLogicalEllipsis());
	bool ThisIsLogicalEllipsis = false;
	bool PriorIsLogicalEllipsis = false;
	size_t ArrayCorrection = 0;

	while(ArraySize(src)>(FinalCommaIdx+=2))
		{
		const UnparsedText& FinalCommaProxy = UnparsedText::fast_up_reference(src[FinalCommaIdx]);
		SUCCEED_OR_DIE(FinalCommaProxy.ArgCannotExtendRightThroughThis());
		PriorIsLogicalEllipsis = ThisIsLogicalEllipsis;
		ThisIsLogicalEllipsis = FinalCommaProxy.IsLogicalEllipsis();
		if (ThisIsLogicalEllipsis)
			{
			SUCCEED_OR_DIE(!PriorIsLogicalEllipsis);
			SUCCEED_OR_DIE(src[FinalCommaIdx-1]->IsUltimateType(&Integer) && (FinalCommaIdx+1>=ArraySize(src) || src[FinalCommaIdx+1]->IsUltimateType(&Integer)));
			ArrayCorrection++;
			continue;
			};
		if (!FinalCommaProxy.IsSemanticChar(',')) break;
		};
	FinalCommaIdx-=2;

	// 2) create space for args
	if (!InsertNSlotsAtV2(2+(FinalCommaIdx-KeywordIdx)/2-ArrayCorrection,0))
		UnconditionalRAMFailure();

	// 3) Move args in
	ArgArray[0] = src[KeywordIdx-1];
	src[KeywordIdx-1] = NULL;
	size_t i = KeywordIdx+1;
	size_t RatchetIdx = 1;

	// Review args to see which were connected by ellipsis.  Create LinearIntervals of 
	// UltimateType _Z_ when indicated.
	do	{
		if (static_cast<UnparsedText*>(src[i+1])->IsLogicalEllipsis())
			LinearIntervalInit(src,i,ArgArray[RatchetIdx++]);
		else{
			ArgArray[RatchetIdx++] = src[i];
			src[i] = NULL;
			}
		}
	while(FinalCommaIdx>(i+=2));
	if (NULL!=src[i])
		{
		ArgArray[RatchetIdx++] = src[i];
		src[i] = NULL;
		}

	// 4) Move *this into Target
	delete src[KeywordIdx-1];
	src[KeywordIdx-1] = this;

	// 5) Clean Arglist space of target
	_delete_n_slots_at(src,2+FinalCommaIdx-KeywordIdx,KeywordIdx);
	if (RatchetIdx<fast_size())
		DeleteNSlotsAt(fast_size()-RatchetIdx,RatchetIdx);

	// 6) do simple transitions
	i = fast_size();	// HACK: reusing variable
	do	if (ArgArray[--i]->HasSimpleTransition())
			DestructiveSyntacticallyEvaluateOnce(ArgArray[i]);
	while(0<i);

	// 7) adjust KeywordIdx to relay location of new clause
	--KeywordIdx;
}

bool ClauseNArg::isAntiIdempotentTo(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/16/1999
	if (rhs.IsExactType(SelfLogicalNOTLookup_ClauseN[array_index()]))
		return ExactOrderPairwiseRelation(static_cast<const ClauseNArg&>(rhs),AreSyntacticallyEqual);
	return false;
}

bool ClauseNArg::DelegateEvaluate(MetaConcept*& dest)
{
	assert(MetaConceptWithArgArray::MaxEvalRuleIdx_ER<IdxCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxEvalRuleIdx_ER>=IdxCurrentEvalRule);
	return (this->*EvaluateRuleLookup[IdxCurrentEvalRule-(MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1)])(dest);
}

ExactType_MC UnparsedText::TypeForNAryClauseKeyword() const
{	// FORMALLY CORRECT: 1/11/2000
	unsigned int i = MaxClauseNIdx_MC-MinClauseNIdx_MC+1;
	// Prefix keywords section
	if (IsPrefixKeyword())
		{
		do	if (!strcmp(Text,ClauseNAryKeywordLookup[--i]))
				return (ExactType_MC)(MinClauseNIdx_MC+i);
		while((ALLEQUAL_ClauseN_MC-MinClauseNIdx_MC)<i);
		}
	else
		i = ALLEQUAL_ClauseN_MC-MinClauseNIdx_MC;

	// Logic keywords section
	if (IsLogicKeyword())
		{
		do	if (!strcmp(Text,ClauseNAryKeywordLookup[--i]))
				return (ExactType_MC)(MinClauseNIdx_MC+i);
		while(0<i);
		};
	return Unknown_MC;
}

void LinearIntervalInit(MetaConcept**& ArgArray,size_t& i,MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/27/2002
	//! \todo OPTIMIZE: A...A+1 doesn't create LinearInterval.  Obvious cases will be 
	// IntegerNumerals and StdAddition.
	assert(ArgArray);
	assert(!dest);
	if (*ArgArray[i]==*ArgArray[i+2])
		{	// Ellipsis, but endpoints are same: 1 physical argument
		dest = ArgArray[i];
		ArgArray[i] = NULL;
		DELETE_AND_NULL(ArgArray[i+2]);
		}
	else{
		AbstractClass* TargetType = NULL;
		try	{
			Integer.CopyInto(TargetType);
			dest = new LinearInterval(ArgArray[i],ArgArray[i+2],TargetType,false,false);
			}
		catch(const bad_alloc&)
			{
			delete TargetType;
			UnconditionalRAMFailure();
			};
		}
	i += 2;
}

bool ClauseNArg::ConvertToMetaConnective(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	// only called from Evaluate(); can destroy own integrity
	assert(!dest);
	assert(LogicalAND_ClauseN_MC<=ExactType() || LogicalNAND_ClauseN_MC>=ExactType());

	dest = new MetaConnective(ArgArray,(MetaConnectiveModes)(ExactType()-LogicalAND_ClauseN_MC));
	assert(dest->SyntaxOK());
	return true;
}

bool ClauseNArg::ConvertToEqualRelation(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	// only called from Evaluate(): can destroy own integrity
	assert(!dest);
	assert(ALLEQUAL_ClauseN_MC<=ExactType() && NOTALLEQUAL_ClauseN_MC>=ExactType());

	dest = new EqualRelation(ArgArray,(EqualRelationModes)(ExactType()-ALLEQUAL_ClauseN_MC));
	assert(dest->SyntaxOK());
	return true;
}


