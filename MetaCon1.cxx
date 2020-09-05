// MetaCon1.cxx
// implementation of MetaConcept

#include "MetaCon1.hxx"
#include "Class.hxx"
#include "LowRel.hxx"

#include "Zaimoni.STL/Pure.C/logging.h"

#include <stdexcept>

#define DECLARE_METACONCEPT(MINA,MAXA,NAME,FLAGS,GRAMMAR)	\
   { MINA, MAXA, NAME##_MC, FLAGS, #NAME, (char)(GRAMMAR) }

//! \todo retrofit BOOST_STATIC_ASSERT tests; e.g., only AND, OR have AnnihilatorKey set
const MetaConceptVFT MetaConceptLookUp[]
  = { DECLARE_METACONCEPT(0,0,Unknown,0,0),
	  DECLARE_METACONCEPT(0,0,TruthValue,0,Adjective_LT),
	  DECLARE_METACONCEPT(0,0,IntegerNumeral,0,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(2,2,LinearInterval,Symmetric_LITBMP1MC,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(0,0,LinearInfinity,0,CommonNoun_LT),
	  DECLARE_METACONCEPT(0,0,AbstractClass,0,CommonNoun_LT),
	  DECLARE_METACONCEPT(0,0,Variable,0,CommonNoun_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalAND,Symmetric_LITBMP1MC | SelfAssociative_LITBMP1MC | HasAnnihilatorKey_LITBMP1MC | AND_of_BasisClauses_LITBMP1MC,Connective_LT),	// begin MetaConnective block
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalOR,Symmetric_LITBMP1MC | SelfAssociative_LITBMP1MC | HasAnnihilatorKey_LITBMP1MC,Connective_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalIFF,Symmetric_LITBMP1MC | Transitive_LITBMP1MC | Reflexive_LITBMP1MC,Connective_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalXOR,Symmetric_LITBMP1MC,Connective_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalNXOR,Symmetric_LITBMP1MC,Connective_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalNIFF,Symmetric_LITBMP1MC,Connective_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalNOR,Symmetric_LITBMP1MC,Connective_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,LogicalNAND,Symmetric_LITBMP1MC,Connective_LT),	// end MetaConnectiveBlock
	  DECLARE_METACONCEPT(2,ULONG_MAX,ALLEQUAL,Symmetric_LITBMP1MC | Transitive_LITBMP1MC | Reflexive_LITBMP1MC | SimplePhraseClauseTransition_LITBMP1MC | AND_of_BasisClauses_LITBMP1MC,Verb_LT),	// begin EqualityRelation block
	  DECLARE_METACONCEPT(2,ULONG_MAX,ALLDISTINCT,Symmetric_LITBMP1MC | SimplePhraseClauseTransition_LITBMP1MC | ProcessesIntegerIntervals_LITBMP1MC | AND_of_BasisClauses_LITBMP1MC,Verb_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,EQUALTOONEOF,SimplePhraseClauseTransition_LITBMP1MC | ProcessesIntegerIntervals_LITBMP1MC,Verb_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,DISTINCTFROMALLOF,SimplePhraseClauseTransition_LITBMP1MC | ProcessesIntegerIntervals_LITBMP1MC | AND_of_BasisClauses_LITBMP1MC,Verb_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,NOTALLDISTINCT,Symmetric_LITBMP1MC | SimplePhraseClauseTransition_LITBMP1MC | ProcessesIntegerIntervals_LITBMP1MC,Verb_LT),
	  DECLARE_METACONCEPT(2,ULONG_MAX,NOTALLEQUAL,Symmetric_LITBMP1MC | SimplePhraseClauseTransition_LITBMP1MC | ProcessesIntegerIntervals_LITBMP1MC,Verb_LT),	// end EqualityRelation block
	  DECLARE_METACONCEPT(1,1,ForAll,0,Adjective_LT | Adverb_LT),	// begin MetaQuantifier block
	  DECLARE_METACONCEPT(1,1,ThereIs,0,Adjective_LT | Adverb_LT),
	  DECLARE_METACONCEPT(1,1,Free,0,Adjective_LT | Adverb_LT),
	  DECLARE_METACONCEPT(1,1,ForAllNot,0,Adjective_LT | Adverb_LT),
	  DECLARE_METACONCEPT(1,1,ThereIsNot,0,Adjective_LT | Adverb_LT),	// end MetaQuantifier clock
	  DECLARE_METACONCEPT(0,0,UnparsedText,0,0),
	  DECLARE_METACONCEPT(2,ULONG_MAX,QuantifiedStatement,0,CommonNoun_LT),
	  DECLARE_METACONCEPT(1,ULONG_MAX,Inparse,0,0),
	  DECLARE_METACONCEPT(1,ULONG_MAX,StdAddition,Symmetric_LITBMP1MC | SelfAssociative_LITBMP1MC | HasAnnihilatorKey_LITBMP1MC,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(1,ULONG_MAX,StdMultiplication,SelfAssociative_LITBMP1MC | HasAnnihilatorKey_LITBMP1MC,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(3,3,SeriesAddition,0,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(3,3,SeriesMultiplication,0,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(1,ULONG_MAX,GCF,Symmetric_LITBMP1MC | SelfAssociative_LITBMP1MC | HasAnnihilatorKey_LITBMP1MC,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(1,1,Factorial,0,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(2,2,PermutationCount,0,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(2,2,CombinationCount,0,ProperNoun_LT | Adjective_LT),
	  DECLARE_METACONCEPT(1,1,IN_Phrase1,0,Adjective_LT),		// begin/end 1-ary phrase block
	  DECLARE_METACONCEPT(1,ULONG_MAX,FORALL_PhraseN,Symmetric_LITBMP1MC,CommonNoun_LT),		// begin n-ary phrase block
	  DECLARE_METACONCEPT(1,ULONG_MAX,THEREIS_PhraseN,Symmetric_LITBMP1MC,CommonNoun_LT),
	  DECLARE_METACONCEPT(1,ULONG_MAX,FREE_PhraseN,Symmetric_LITBMP1MC,CommonNoun_LT),
	  DECLARE_METACONCEPT(1,ULONG_MAX,NOTFORALL_PhraseN,Symmetric_LITBMP1MC,CommonNoun_LT),
	  DECLARE_METACONCEPT(1,ULONG_MAX,THEREISNO_PhraseN,Symmetric_LITBMP1MC,CommonNoun_LT),	// end n-ary phrase block
	  DECLARE_METACONCEPT(0,ULONG_MAX,ParseNode,0,0)
};
static_assert(sizeof(MetaConceptLookUp) / sizeof(*MetaConceptLookUp) == UB_MC);

#undef DECLARE_METACONCEPT

// #define TRAP_UNDEF_VFUNC 1

// testable low-level dependencies on internal type enumeration

// TruthValue_MC is the smallest type with ultimatetype TruthValues
// 1 == TruthValue_MC
static_assert(1==TruthValue_MC);

// MetaConnective:
// * MetaConnective IDs are a block
static_assert(7==LogicalNAND_MC-LogicalAND_MC);
// * Lowest MetaConnective ID is LogicalAND_MC
static_assert(LogicalAND_MC<LogicalOR_MC);
static_assert(LogicalAND_MC<LogicalIFF_MC);
static_assert(LogicalAND_MC<LogicalXOR_MC);
static_assert(LogicalAND_MC<LogicalNXOR_MC);
static_assert(LogicalAND_MC<LogicalNIFF_MC);
static_assert(LogicalAND_MC<LogicalNOR_MC);
static_assert(LogicalAND_MC<LogicalNAND_MC);
// * Paired s.t. logical-negations are on opposite ends, similar offset
static_assert(LogicalAND_MC+LogicalNAND_MC==LogicalOR_MC+LogicalNOR_MC);
static_assert(LogicalAND_MC+LogicalNAND_MC==LogicalIFF_MC+LogicalNIFF_MC);
static_assert(LogicalAND_MC+LogicalNAND_MC==LogicalXOR_MC+LogicalNXOR_MC);
// * -2 converts NIFF to XOR, NXOR to IFF
static_assert(LogicalNIFF_MC-2==LogicalXOR_MC);
static_assert(LogicalNXOR_MC-2==LogicalIFF_MC);
// * -6 converts NOR to AND, NAND to OR
static_assert(LogicalNAND_MC-6==LogicalOR_MC);
static_assert(LogicalNOR_MC-6==LogicalAND_MC);
// * NOR, NAND are terminal block, NOR<NAND
//	BOOST_STATIC_ASSERT(LogicalAND_MC<LogicalNOR_MC);	// already done above
static_assert(LogicalOR_MC<LogicalNOR_MC);
static_assert(LogicalIFF_MC<LogicalNOR_MC);
static_assert(LogicalXOR_MC<LogicalNOR_MC);
static_assert(LogicalNXOR_MC<LogicalNOR_MC);
static_assert(LogicalNIFF_MC<LogicalNOR_MC);
static_assert(LogicalNOR_MC<LogicalNAND_MC);

// * EqualRelation IDs are a block
static_assert(5==NOTALLEQUAL_MC-ALLEQUAL_MC);
// * Lowest EqualRelation ID is ALLEQUAL_MC
static_assert(ALLEQUAL_MC<ALLDISTINCT_MC);
static_assert(ALLEQUAL_MC<DISTINCTFROMALLOF_MC);
static_assert(ALLEQUAL_MC<EQUALTOONEOF_MC);
static_assert(ALLEQUAL_MC<NOTALLDISTINCT_MC);
static_assert(ALLEQUAL_MC<NOTALLEQUAL_MC);
// * Paired s.t. logical-negations are on opposite ends, similar offset
static_assert(ALLEQUAL_MC+NOTALLEQUAL_MC==ALLDISTINCT_MC+NOTALLDISTINCT_MC);
static_assert(ALLEQUAL_MC+NOTALLEQUAL_MC==DISTINCTFROMALLOF_MC+EQUALTOONEOF_MC);
// * -2 maps EQUALTOONEOF to ALLEQUAL, DISTINCTFROMALLOF to ALLDISTINCT
static_assert(EQUALTOONEOF_MC-2==ALLEQUAL_MC);
static_assert(DISTINCTFROMALLOF_MC-2==ALLDISTINCT_MC);

// * MetaQuantifier IDs are a block
// * There are 5 identifiers in the block
static_assert(4==ThereIsNot_MC-ForAll_MC);
// * Lowest MetaQuantifier ID is ForAll_MC
static_assert(ForAll_MC<ThereIs_MC);
static_assert(ForAll_MC<Free_MC);
static_assert(ForAll_MC<ForAllNot_MC);
static_assert(ForAll_MC<ThereIsNot_MC);
// * Paired s.t. logical-negations are on opposite ends, similar offset
static_assert(ForAll_MC+ThereIsNot_MC==ThereIs_MC+ForAllNot_MC);
static_assert(ForAll_MC+ThereIsNot_MC==Free_MC+Free_MC);
// * 6*[highest ID in block] does not cause an arithmetic overflow
static_assert(UCHAR_MAX/6>=ThereIsNot_MC);
// FREE is at offset 2 (PhraseNArg wants this)
static_assert(2==(Free_MC-ForAll_MC));

void DiagnoseMetaConceptVFT()
{	// FORMALLY CORRECT: Kenneth Boyd, 6/17/1999
	int i = 0;
	do	{
		// check: index matches ID
		SUCCEED_OR_DIE(MetaConceptLookUp[i].ExactType==i);
		// check: Minarity <= MaxArity
		SUCCEED_OR_DIE(MetaConceptLookUp[i].MinArity<=MetaConceptLookUp[i].MaxArity);
		// check: Minarity for Commutative >=2
		SUCCEED_OR_DIE(2<=MetaConceptLookUp[i].MaxArity || !(Symmetric_LITBMP1MC & MetaConceptLookUp[i].Bitmap1));
		}
	while(MaxSemanticIdx_MC>= ++i);
}

void MetaConcept::_syntax_ok() const
{
	int fail = 0;
	if ((LogicalNegated_VF |StdAdditionInv_VF | StdMultInv_VF) < MultiPurposeBitmap) fail |= 1;
	if (VFTable1 && (MetaConceptLookUp + TruthValue_MC > VFTable1 || MetaConceptLookUp + MaxSemanticIdx_MC < VFTable1)) fail |= 2;
	if (!_memory_block_start_valid(this)) fail |= 4;
	if (fail) {
		std::string err("C invalid memory error: ");
		err += std::to_string(fail) + "; " + std::to_string(VFTable1-MetaConceptLookUp);
		INFORM(err.c_str());
		throw std::logic_error(err);
	}
}


bool MetaConcept::IsInfinite() const
{	//! \todo: augment when other infinities introduced
	return IsExactType(LinearInfinity_MC);
}

bool MetaConcept::IsFinite() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/24/2004
	const AbstractClass* UltType = UltimateType();
	if (   NULL!=UltType
		&& UltType->ToleratesInfinity()
		&& UltType->HasAsElement(*this))
		return true;
	return false;
}

bool MetaConcept::IsUltimateType(const AbstractClass* rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	const AbstractClass* UltType = UltimateType();
	if (!UltType) return !rhs;
	if (!rhs) return false;
	return *UltType==*rhs;
}

//! \todo compose implementation matrix for for StdAddition, other dynamic n-ary types
bool MetaConcept::ForceUltimateType(const AbstractClass* const rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/27/2000
	//! \return true iff the specified UltimateType was forced
	//! The default implementation must be overridden for any type whose UltimateType is intended to be changed.
	// Base version, suitable for constant types
	if (!rhs) return true;
	if (!UltimateType()) return false;
	if (UltimateType()->Subclass(*rhs)) return true;
	return false;
}

bool MetaConcept::CanUseThisAsMakeImply(const MetaConcept& Target) const
{
	auto rules = const_cast<MetaConcept*>(this)->_CanUseThisAsMakeImply(Target);
	return rules.first || rules.second;
}

bool operator==(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/15/2005
	if (lhs.IsExactType(rhs.ExactType()) && lhs.MultiPurposeBitmap==rhs.MultiPurposeBitmap)
		return lhs.EqualAux2(rhs);
	return false;
}

// MetaConcept::SyntacticalStandardLT is in SymConst.cxx

bool MetaConcept::ValidateFunctionArgList(MetaConcept** const& ArgValList, unsigned long*& ArgList) const
{	//! \todo IMPLEMENT further when function UltimateTypes are in place.
	const unsigned long FunctionArityImage = FunctionArity();
	const unsigned long ArgListSizeImage = ArraySize(ArgList);
	if (   !ArgValList
		|| !ArgList
		|| FunctionArityImage<ArgListSizeImage
		|| ArraySize(ArgValList)!=ArgListSizeImage)
		return false;
	if (!and_range([&](unsigned long x) { return x < FunctionArityImage; }, ArgList, ArgList + ArgListSizeImage))
		return false;
	return pairwise_distinct(std::less<unsigned long>(),ArgList,ArgList+ArgListSizeImage);
	//! \todo type-checking on args when pairwise distinct (needs function UltimateTypes to work)
}

bool
MetaConcept::CanCommuteUnderStdMultiplicationWith(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/15/2004
	if (IsUltimateType(NULL) || rhs.IsUltimateType(NULL)) return false;
	if (UltimateType()->StdMultiplicationCommutativeWithDomain(*rhs.UltimateType()))
		return true;
	if (   !UltimateType()->SupportsThisOperation(StdMultiplication_MC)
		&& !rhs.UltimateType()->SupportsThisOperation(StdMultiplication_MC))
		return false;
	return CanCommuteUnderStdMultiplicationWithAux(rhs);
}

//! \todo decide on how to use MultiPurposeBitmap for this
//! \return true iff the internal data type is "less than" the RHS's internal data type
//! Arbitrary ordering to enable standard form for symmetric types
bool MetaConcept::InternalDataLT(const MetaConcept& rhs) const
{
	if (IsExactType(rhs.ExactType())) return InternalDataLTAux(rhs);
	return ExactType()<rhs.ExactType();
}

static size_t FindRParens(const char* x, const size_t x_len)
{
	assert(x && *x);
	size_t i = 0;
	size_t LParenCount=1;
	while(++i<x_len)
		{
		if		('('==x[i]) LParenCount++;
		else if (')'==x[i])
			{
			if (0==--LParenCount) return i;
			}
		};
	return 0;
}		

static size_t FindExtraOffsetFromRParens(const char* x)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/27/2007
	assert(x && *x);
	size_t n = strspn(x+1,")")+1;
	if (','!=x[n]) --n;
	return n;
}		

static char* PutCharAtIdx(char*& x, size_t& x_len, char NewChar, const size_t i, size_t& BreakOutOfThisIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/1999
	assert(x_len>=i);
	char* tmp = REALLOC(x,ZAIMONI_LEN_WITH_NULL(x_len+1));
	if (tmp)
		{	// NULL Tmp: No memory to format with, so must be desperate: 
			// don't worry about it.
		if (i<x_len) memmove(tmp+i+1,tmp+i,x_len-i);
		tmp[i]=NewChar;
		x_len++;
		BreakOutOfThisIdx++;
		x = tmp;
		ZAIMONI_NULL_TERMINATE(x[x_len+1]);
		};
	return tmp;
}

static char* PutNCharAtIdx(char*& x, size_t& x_len, char src, size_t n, size_t i, size_t& BreakOutOfThisIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/27/2007
	assert(0<n);
	assert(strlen(x)==x_len);
	assert(i<=x_len);
	char* Tmp = REALLOC(x,x_len+n);
	if (Tmp)
		{	// NULL Tmp: No memory to format with, so must be desperate: don't worry about it.
		if (i<x_len) memmove(Tmp+i+n,Tmp+i,x_len-i);
		x_len += n;
		BreakOutOfThisIdx += n;
		memset(Tmp+i,src,n);
		x = Tmp;
		};
	return Tmp;
}

void PutNCharAtIdx(std::string& x, char src, size_t n, size_t i, size_t& BreakOutOfThisIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/27/2007
	assert(0 < n);
	assert(i <= x.size());
	x.replace(i, 0, n, src);
	BreakOutOfThisIdx += n;
}

static void NiceFormatWithLogicalTab(std::string& Name, size_t& LogicalLineOrigin, size_t BreakOutOfThisIdx, const size_t LogicalTab)
{	//! \todo OPTIMIZE: TIME
Restart:
	while (79 + LogicalLineOrigin < BreakOutOfThisIdx + LogicalTab)
	{
		if ('\n' == Name[LogicalLineOrigin]) {
			if (0 < LogicalTab) PutNCharAtIdx(Name, ' ', LogicalTab, LogicalLineOrigin + 1, BreakOutOfThisIdx);
			LogicalLineOrigin += 1 + LogicalTab;
			goto Restart;
		};
		size_t i = 1;
		do	if ('\n' == Name[LogicalLineOrigin + i]) {
			LogicalLineOrigin += i;
			goto Restart;
		} while (79 > ++i + LogicalTab);
		{
			i = 0;
			size_t NiceBreakPoint = 80 - LogicalTab;
			do {
				if (' ' == Name[i + LogicalLineOrigin]
					|| ',' == Name[i + LogicalLineOrigin]
					|| (')' == Name[i + LogicalLineOrigin] && ')' != Name[i + LogicalLineOrigin + 1] && ',' != Name[i + LogicalLineOrigin + 1]))
					NiceBreakPoint = i;
				else if ('(' == Name[i + LogicalLineOrigin])
				{	// parentheses processing
					size_t RParensOffset = FindRParens(Name.data() + LogicalLineOrigin + i, Name.size() - LogicalLineOrigin - i);
					if (RParensOffset)
					{
						size_t ExtraOffsetFromRParens = FindExtraOffsetFromRParens(Name.data() + LogicalLineOrigin + i + RParensOffset);
						if (79 > i + RParensOffset + ExtraOffsetFromRParens + LogicalTab)
						{
							i += RParensOffset + ExtraOffsetFromRParens;
							NiceBreakPoint = i;
						}
						else {	// FORMAT RECURSION
							if (80 - LogicalTab > NiceBreakPoint)
								break;	// Current, valid NiceBreakPoint will work
							if (' ' != Name[LogicalLineOrigin + i + RParensOffset + ExtraOffsetFromRParens + 1])
							{	//! \todo this is *probably* math notation.  We'll have to refine this later.
								if (LogicalLineOrigin + i + RParensOffset + ExtraOffsetFromRParens + 1 < BreakOutOfThisIdx
									// this clause prevents unwanted blank lines
									&& '\n' != Name[LogicalLineOrigin + i + RParensOffset + ExtraOffsetFromRParens + 1])
									PutNCharAtIdx(Name, '\n', 1, LogicalLineOrigin + i + RParensOffset + ExtraOffsetFromRParens + 1, BreakOutOfThisIdx);
								LogicalLineOrigin += i + 1;
								const size_t OldNameLen = Name.size();
								NiceFormatWithLogicalTab(Name, LogicalLineOrigin, LogicalLineOrigin + RParensOffset + ExtraOffsetFromRParens, LogicalTab + i + 1);
								if (OldNameLen < Name.size()) BreakOutOfThisIdx += (Name.size() - OldNameLen);
								goto Restart;
							};
						}
					}
				}
			} while (79 > ++i + LogicalTab);

			PutNCharAtIdx(Name, '\n', 1, NiceBreakPoint + LogicalLineOrigin + 1, BreakOutOfThisIdx);
			LogicalLineOrigin += NiceBreakPoint + 1;
		}
	};
	if ('\n' == Name[LogicalLineOrigin]) {
		if (0 < LogicalTab) PutNCharAtIdx(Name, ' ', LogicalTab, LogicalLineOrigin + 1, BreakOutOfThisIdx);
		LogicalLineOrigin += 1 + LogicalTab;
	};
}

void NiceFormat80Cols(std::string& x)
{	// This routine formats the text representation of a Franci object for her logfile.
	// The intent is to provide enough whitespace for readability.
	if (!x.empty()) {
		size_t x_len = x.size();
		if (79 < x_len)
		{
			size_t LogicalLineOrigin = 0;
			NiceFormatWithLogicalTab(x, LogicalLineOrigin, x_len, 0);
		};
	};
}


void LOG(const MetaConcept& B)
{	// FORMALLY CORRECT: 5/19/2006
	if (is_logfile_active())
		{
		auto Tmp(B.to_s());
		NiceFormat80Cols(Tmp);
		if (!Tmp.empty()) LOG(Tmp.data());
		else LOG("Attempt to log concept suffered RAM failure.\n");
		};
}

void INFORM(const MetaConcept& B)
{	// FORMALLY CORRECT: 5/19/2006
	auto Tmp(B.to_s());
	NiceFormat80Cols(Tmp);
	if (!Tmp.empty()) LOG(Tmp.data());
	else INFORM("Attempt to state concept suffered RAM failure.\n");
}


//! \todo OPTIMIZE: SPEED (?)
//! this could use a guard clause: FranciDialog.IsLogFileActive() [cf. above]
void MetaConcept::LogThis(const char* const Header) const
{	// FORMALLY CORRECT: 10/7/1999
	LOG(Header);
	LOG(*this);
	LOG("");
}

// The SelfLogicalNOT required can damage the internal state of the target.  Unfortunately, 
// cloning the target is a performance killer.
// NOTE: consider having EqualRelation SelfLogicalNOT() call ForceStdForm()
bool StrictlyImpliesLogicalNOTOf(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/27/2002
	if (rhs.SelfLogicalNOTWorks())
		{
		const_cast<MetaConcept&>(rhs).SelfLogicalNOT();
		const bool Result = StrictlyImplies(lhs,rhs);
		const_cast<MetaConcept&>(rhs).SelfLogicalNOT();
		return Result;
		};
	return false;
}

bool LogicalNOTOfStrictlyImplies(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/27/2002
	if (lhs.SelfLogicalNOTWorks())
		{
		const_cast<MetaConcept&>(lhs).SelfLogicalNOT();
		const bool Result = StrictlyImplies(lhs,rhs);
		const_cast<MetaConcept&>(lhs).SelfLogicalNOT();
		return Result;
		};
	return false;
}

bool MetaConcept::SelfInverse(const ExactType_MC Operation)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/31/2001
	if (   !IsUltimateType(NULL)
	    &&  UltimateType()->SupportsThisOperation(Operation))
		{
		if (StdAddition_MC==Operation)
			{
			MultiPurposeBitmap ^= StdAdditionInv_VF;
			return true;
			};
		if (StdMultiplication_MC==Operation)
			{
			MultiPurposeBitmap ^= StdMultInv_VF;
			return true;
			};
		};
	return false;
}

bool
MetaConcept::SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const
{	// FORMALLY CORRECT: 7/31/2001
	if (    rhs.IsExactType(ExactType())
		&& !IsUltimateType(NULL)
		&& !rhs.IsUltimateType(NULL)
	    &&  UltimateType()->SupportsThisOperation(Operation)
		&&  rhs.UltimateType()->SupportsThisOperation(Operation))
		{
		if (StdAddition_MC==Operation || StdMultiplication_MC==Operation)
			{
			const_cast<MetaConcept&>(rhs).SelfInverse(Operation);
			if (*this==rhs)
				{
				const_cast<MetaConcept&>(rhs).SelfInverse(Operation);
				return true;
				}
			const_cast<MetaConcept&>(rhs).SelfInverse(Operation);
			return false;
			}
		};
	return false;
}

bool
MetaConcept::MakesLHSImplyLogicalNOTOfRHS(const MetaConcept& lhs, const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/3/2000
	SUCCEED_OR_DIE(rhs.SelfLogicalNOTWorks());
	const_cast<MetaConcept&>(rhs).SelfLogicalNOT();
	const bool Result = MakesLHSImplyRHS(lhs,rhs);
	const_cast<MetaConcept&>(rhs).SelfLogicalNOT();
	return Result;
}

bool
MetaConcept::ValidRHSForMakesLHSImplyLogicalNOTOfRHS(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/3/2000
	SUCCEED_OR_DIE(rhs.SelfLogicalNOTWorks());
	const_cast<MetaConcept&>(rhs).SelfLogicalNOT();
	const bool Result = ValidRHSForMakesLHSImplyRHS(rhs);
	const_cast<MetaConcept&>(rhs).SelfLogicalNOT();
	return Result;
}

size_t
MetaConcept::RadixSortIdxSourceListByRHSCompatible(MetaConcept** const IdxSourceList) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/5/2000
	SUCCEED_OR_DIE(IdxSourceList);
	size_t StrictUB = 0;
	size_t SweepDown = ArraySize(IdxSourceList);
	do	{
		while(ValidRHSForMakesLHSImplyLogicalNOTOfRHS(*IdxSourceList[StrictUB]))
			if (SweepDown== ++StrictUB) return StrictUB;
		while(!ValidRHSForMakesLHSImplyLogicalNOTOfRHS(*IdxSourceList[--SweepDown]))
			if (SweepDown==StrictUB) return StrictUB;
		swap(IdxSourceList[StrictUB],IdxSourceList[SweepDown]);
		}
	while(--SweepDown> ++StrictUB);
	return StrictUB;	
}

//! \todo IMPLEMENT: metacase == yields 0 when + supported
//! \todo IMPLEMENT: metacase ___ vs. 2-sum
//! \todo IMPLEMENT: 2-sum vs. __
//! \todo IMPLEMENT: n-sum vs. n-1-sum, or vice versa [subvector test]
bool
MetaConcept::SmallDifference(const MetaConcept& RHS, signed long& Result) const
{
#if 0
	if (NULL!=UltimateType() && UltimateType()->SupportsThisOperation(StdAddition_MC))
		{
		if (   *this==RHS
			&& UltimateType()->HasAsElement(0))	//! \todo IMPLEMENT AbstractClass' HasAsElement family for (un)signed long
			{
			Result = 0;
			return true;
			}
		if (   RHS.IsExactType(StdAddition_MC)
			&& 2==RHS.ActualArity())
			{
			if 		(*this==*RHS.ArgN(0))
				{	//! \todo IMPLEMENT ConvertToMachineInt(signed long& Result)
				if (   RHS.ArgN(1)->IsExactType(IntegerNumeral_MC)
					&& static_cast<IntegerNumeral*>(RHS.ArgN(1))->ConvertToMachineInt(Result))
					return true;
				}
			else if (*this==*RHS.ArgN(1))
				{
				if (   RHS.ArgN(0)->IsExactType(IntegerNumeral_MC)
					&& static_cast<IntegerNumeral*>(RHS.ArgN(0))->ConvertToMachineInt(Result))
					return true;
				}
			}
		}
#endif
	return false;
}

/* NOTE: RHS is NULL afterwards if this goes off */
/* use this as an optional RAM-conserving frontend for StrongVarSubstitute */
bool DestructiveStrongVarSubstitute(MetaConcept*& dest, const MetaConcept& lhs, MetaConcept*& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/26/2003
	//! \todo AUGMENT
	assert(dest);
	assert(rhs);
	if (*dest==lhs)
		{
		delete dest;
		dest = rhs;
		rhs = NULL;
		return true;
		}

	if (   lhs.SelfLogicalNOTWorks() && rhs->SelfLogicalNOTWorks()
		&& dest->SelfLogicalNOTWorks()
		&& IsAntiIdempotentTo(*dest,lhs))
		{
		rhs->SelfLogicalNOT();
		delete dest;
		dest = rhs;
		rhs = NULL;
		return true;
		}

	// AddInv test
	if (   !lhs.IsUltimateType(NULL) && lhs.UltimateType()->SupportsThisOperation(StdAddition_MC)
		&& !rhs->IsUltimateType(NULL) && rhs->UltimateType()->SupportsThisOperation(StdAddition_MC)
		&& !dest->IsUltimateType(NULL) && dest->UltimateType()->SupportsThisOperation(StdAddition_MC)
		&&  dest->SelfInverseTo(lhs,StdAddition_MC))
		{
		if (!rhs->SelfInverse(StdAddition_MC)) return false;
		delete dest;
		dest = rhs;
		rhs = NULL;
		return true;
		}
		
	// MultInv test
	if (   !lhs.IsUltimateType(NULL) && lhs.UltimateType()->SupportsThisOperation(StdMultiplication_MC)
		&& !rhs->IsUltimateType(NULL) && rhs->UltimateType()->SupportsThisOperation(StdMultiplication_MC)
		&& !dest->IsUltimateType(NULL) && dest->UltimateType()->SupportsThisOperation(StdMultiplication_MC)
		&&  dest->SelfInverseTo(lhs,StdMultiplication_MC))
		{
		if (!rhs->SelfInverse(StdMultiplication_MC)) return false;
		delete dest;
		dest = rhs;
		rhs = NULL;
		return true;
		}

	return false;
}

bool StrongVarSubstitute(MetaConcept*& dest, const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/26/2003
	// == section
	assert(dest);
	if (*dest==lhs)
		{
		try	{
			SetLHSToRHS(dest,rhs);
			return true;
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		}
	if (!dest->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,SetLHSToRHS,AreSyntacticallyEqual))
		return false;

	// AntiIdempotent test
	if (   lhs.SelfLogicalNOTWorks()
		&& rhs.SelfLogicalNOTWorks())
		{
		if (dest->SelfLogicalNOTWorks() && IsAntiIdempotentTo(*dest,lhs))
			{
			try	{
				SetLHSToLogicalNOTOfRHS(dest,rhs);
				return true;
				}
			catch(const bad_alloc&)
				{
				return false;
				}
			}

		if (!dest->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,SetLHSToLogicalNOTOfRHS,IsAntiIdempotentTo))
			return false;
		}

	// AddInv test
	if (   !lhs.IsUltimateType(NULL) && lhs.UltimateType()->SupportsThisOperation(StdAddition_MC)
		&& !rhs.IsUltimateType(NULL) && rhs.UltimateType()->SupportsThisOperation(StdAddition_MC))
		{
		if (   !dest->IsUltimateType(NULL) && dest->UltimateType()->SupportsThisOperation(StdAddition_MC)
			&&  dest->SelfInverseTo(lhs,StdAddition_MC))
			{
			try	{
				SetLHSToStdAdditionInverseOfRHS(dest,rhs);
				return true;
				}
			catch(const bad_alloc&)
				{
				return false;
				}
			}

		if (!dest->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,SetLHSToStdAdditionInverseOfRHS,IsStdAdditionInverseTo))
			return false;
		}
		
	// MultInv test
	if (   !lhs.IsUltimateType(NULL) && lhs.UltimateType()->SupportsThisOperation(StdMultiplication_MC)
		&& !rhs.IsUltimateType(NULL) && rhs.UltimateType()->SupportsThisOperation(StdMultiplication_MC))
		{
		if (   !dest->IsUltimateType(NULL) && dest->UltimateType()->SupportsThisOperation(StdMultiplication_MC)
			&&  dest->SelfInverseTo(lhs,StdMultiplication_MC))
			{
			try	{
				SetLHSToStdMultiplicationInverseOfRHS(dest,rhs);
				return true;
				}
			catch(const bad_alloc&)
				{
				return false;
				}
			}

		if (!dest->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs,rhs,SetLHSToStdMultiplicationInverseOfRHS,IsStdMultiplicationInverseTo))
			return false;
		}
	return true;
}

void CopyOrThrow(MetaConcept*& dest, MetaConcept*& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	if (src)
		src->CopyInto(dest);
	else
		DELETE_AND_NULL(dest);
}

void CopyOrThrow(AbstractClass*& dest, AbstractClass*& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2002
	if (src)
		src->CopyInto(dest);
	else
		DELETE_AND_NULL(dest);
}

