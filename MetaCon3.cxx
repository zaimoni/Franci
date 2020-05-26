// MetaCon3.cxx
// implementation of MetaConnective

/*
QualInv2Form4Alt1.txt wants this
    OR(NOT A1B2_GT_0,NOT A2B1_LT_0),OR(A1B2_LT_0,A2B1_GT_0),
    OR(NOT A1B3_GT_0,NOT A3B1_LT_0),OR(A1B3_LT_0,A3B1_GT_0),
    OR(NOT A1B4_GT_0,NOT A4B1_LT_0),OR(A1B4_LT_0,A4B1_GT_0),
    OR(NOT A2B3_GT_0,NOT A3B2_LT_0),OR(A2B3_LT_0,A3B2_GT_0),
    OR(NOT A2B4_GT_0,NOT A4B2_LT_0),OR(A2B4_LT_0,A4B2_GT_0),
    OR(NOT A3B4_GT_0,NOT A4B3_LT_0),OR(A3B4_LT_0,A4B3_GT_0),
to eliminate the redundant OR's (left pair) by applying XOR to the primary OR's (right pair)

We could temporarily amplify the OR with XOR.  E.g.:
OR(A3B4_LT_0,A4B3_GT_0)
XOR(A3B4_EQ_0,A3B4_GT_0, A3B4_LT_0)
XOR(A4B3_EQ_0,A4B3_GT_0, A4B3_LT_0)
allows replacing the OR with
OR(AND(NOT A3B4_EQ_0,NOT A3B4_GT_0, A3B4_LT_0),AND(NOT A4B3_EQ_0,NOT A4B3_GT_0,A4B3_LT_0))

Temporary amplification also works with IFF, and also applies to other argument types.  Of 
course, amplification cannot delete (by implication) anything actually used in the amplification.

We record whether a given IFF/XOR can amplify another argtype in the digraph, and then 
process everything with incoming edges.

IFF amplifies if any arg is ==, or antiidempotent to, an arg in the target.
XOR amplifies if any arg is == to an arg in the target.

The use of anti-idempotent arg bridging.
OR(A,B)
OR(~A,C)
* boosted OR(B,C)

is also a form of amplification.

Interface functions:
* CanAmplifyClause(void) const
* CanAmplifyThisClause(const MetaConcept&) const
* AmplifyThisClause(MetaConcept*&) const

It might make sense to provide specialized rules for
"use VR statement as StrictlyImplies|StrictlyModifies|CanUseAsStrictlyImplies"
 */

/*
QualInv2Form5Alt3.txt want this
     IFF(A1B3_EQ_0,NOT A1B3_LT_0,NOT A1B4_EQ_0,A1B4_GT_0,A2B3_EQ_0,NOT 
         A2B3_LT_0,NOT A2B4_EQ_0,A2B4_GT_0,NOT A3B1_EQ_0,A3B1_GT_0,NOT 
         A3B2_EQ_0,A3B2_GT_0,NOT A3B4_EQ_0,A3B4_LT_0,NOT A3_EQ_0,A4B1_EQ_0,NOT 
         A4B1_LT_0,A4B2_EQ_0,NOT A4B2_LT_0,A4B3_EQ_0,NOT A4B3_GT_0,A4_EQ_0,
         B3_EQ_0,NOT B4_EQ_0,
         AND(IFF(A1_GT_0,A2_GT_0,NOT A3_GT_0,A3_LT_0,NOT B1_GT_0,NOT B2_GT_0,
                 B4_GT_0,NOT B4_LT_0),
         NIFF(A1_GT_0,A4_GT_0,NOT A4_LT_0)),
         NIFF(A1_GT_0,A2_GT_0,NOT A4_GT_0,A4_LT_0,B2_GT_0,NOT B3_GT_0,
              B3_LT_0)),
     XOR(A1B3_EQ_0,A4_GT_0,A4_LT_0),XOR(A1B3_EQ_0,B3_GT_0,B3_LT_0),
     XOR(NOT A1B3_EQ_0,A3_GT_0,A3_LT_0),XOR(NOT A1B3_EQ_0,B4_GT_0,B4_LT_0))

to trigger a "tensorification"

The IFF tensor basis is A1B3_EQ_0,NOT A1B3_EQ_0, and uses A4_GT_0,A4_LT_0,B3_GT_0,B3_LT_0,A3_GT_0,A3_LT_0,B4_GT_0,B4_LT_0.
The XORs tensor bases are:
	A1B3_EQ_0,A4_GT_0,A4_LT_0
	A1B3_EQ_0,B3_GT_0,B3_LT_0
	NOT A1B3_EQ_0,A3_GT_0,A3_LT_0
	NOT A1B3_EQ_0,B4_GT_0,B4_LT_0

Eliminate the A1B3_EQ_0,NOT A1B3_EQ_0 basis (and IFF) [it signals we want to do this].  
End result is a 8-ary XOR of AND in 9 variables.  evaluate this first.  If it 
remains a 17-ary XOR, use; otherwise, figure out if the reduction in arity signifies anything.

Alternatively, use this basis to create a "ring the changes" on AND clauses for another source of speculative ORs
 */

#include "MetaCon3.hxx"

#include "Class.hxx"
#include "TruthVal.hxx"
#include "Digraph.hxx"
#include "SrchTree.hxx"
#include "LowRel.hxx"
#include "Keyword1.hxx"
#include <memory>

// Define this if there's some reason to think amplifying tautologies is a useful heuristic
// #define AMPLIFY_TAUTOLOGIES 1

// no local gain from macroizing msz_Implies, msz_ImpliesNOT
const char* const msz_Using = "Using";
const char* const msz_UsingNOT = "Using the logical negation of";
const char* const msz_SyntaxImplies = "syntactically implies";
const char* const msz_Implies = "implies";
const char* const msz_SyntaxImpliesNOT = "syntactically implies the logical negation of";
const char* const msz_ImpliesNOT = "implies the logical negation of";
const char* const msz_RewriteAND = "which rewrites this AND";
const char* const msz_RewriteOR = "which rewrites this OR";
const char* const msz_InferThat = "Inferring that";
const char* const msz_ToThis = "to this";
const char* const msz_NotDefinitive = "RAM failure in speculative reasoning noticed.  This (sub)calculation may not be definitive.";

MetaConnective::EvaluateToOtherRule MetaConnective::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
  =	{
	&MetaConnective::LogicalANDAry2NArySpliceEqualArg,
	&MetaConnective::LogicalANDAry2ALLEQUALSpliceAddInvArg
	};

MetaConnective::SelfEvaluateRule MetaConnective::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&MetaConnective::LogicalANDSpliceIFFAntiIdempotentArg,
	&MetaConnective::LogicalANDAry2IFFSpliceAntiIdempotentArg,
	&MetaConnective::toNOR,
	&MetaConnective::Ary2IFFToOR,
	&MetaConnective::Ary2IFFToORV2,
	&MetaConnective::Ary2IFFToORV3,
	&MetaConnective::Ary2IFFToORSelfModify,
	&MetaConnective::IFFSpawn2AryIFF,
	&MetaConnective::LogicalANDReplaceTheseNAry2ORWithNIFF,
	&MetaConnective::RemoveIrrelevantArgFromOR,
	&MetaConnective::LogicalANDSpawnClauseForDetailedRule,
	&MetaConnective::IFF_ReplaceANDORWithOwnArgs,
	&MetaConnective::LogicalANDStrictlyModify,
	&MetaConnective::LogicalANDStrictlyImpliesClean,
	&MetaConnective::VirtualDeepStrictlyModify,
	&MetaConnective::VirtualDeepLogicallyImplies,
	&MetaConnective::LogicalORStrictlyImpliesClean,
	&MetaConnective::ExtractTrueArgFromXOR,
	&MetaConnective::LogicalANDReplaceThisArgWithTRUE,
	&MetaConnective::LogicalXORExtractANDFactor,
	&MetaConnective::LogicalXORExtractIFFFactor,
	&MetaConnective::LogicalANDReplaceORAndXORWithXORAndNORArg,
	&MetaConnective::ConvertToNANDOtherArgs,
	&MetaConnective::ConvertToNOROtherArgs,
	&MetaConnective::ReplaceArgsWithTrue,
	&MetaConnective::TargetVariableFalse,
	&MetaConnective::TargetVariableTrue,
	&MetaConnective::LogicalANDAry2MetaConnectiveSpliceEqualArg,
	&MetaConnective::LogicalANDReplaceORAndXORWithXORAndNOTArg,
	&MetaConnective::AB_ToIFF_AnotB
	};	
	
// reference tables V2
#define DECLARE_CONTRADICTION(A)	EvalForceContradiction_ER
#define DECLARE_TRUE(A)	EvalForceTrue_ER
#define DECLARE_FALSE(A)	EvalForceFalse_ER
#define DECLARE_UNKNOWN(A)	EvalForceUnknown_ER
#define DECLARE_ARG(A)	EvalForceArg_ER
#define DECLARE_NOTARG(A)	EvalForceNotArg_ER
#define DECLARE_CLEANARG(A)	SelfEvalRuleCleanArg_SER
#define DECLARE_CLEANARGNXOR(A)	SelfEvalRuleAry2CorrectedCleanArg_SER
#define DECLARE_CLEANARGNIFFXOR(A)	SelfEvalRuleNIFFXORCleanArg_SER
#define DECLARE_CONVERT_TO_NOR(A)	ConvertToNOROtherArgs_SER
#define DECLARE_CONVERT_TO_OR(A)	CompatibleRetypeOtherArgs_SER
#define DECLARE_CONVERT_TO_NAND(A)	ConvertToNANDOtherArgs_SER
#define DECLARE_CONVERT_TO_AND(A)	CompatibleRetypeOtherArgs_SER
#define DECLARE_REPLACEARGS_WITH_TRUE(A) ReplaceArgsWithTrue_SER
#define DECLARE_TARGETVARFALSE_AND_XOR(A)	TargetVariableFalse_SER
#define DECLARE_TARGETVARTRUE_OR_NXOR(A)	TargetVariableTrue_SER

const signed short MetaConnective::IdempotentNArySelfEvalRulesTable[NIFF_MCM+1]
  = {
	DECLARE_CLEANARG(AND),
	DECLARE_CLEANARG(OR),
	DECLARE_CLEANARG(IFF),
	DECLARE_TARGETVARFALSE_AND_XOR(XOR),
	DECLARE_TARGETVARTRUE_OR_NXOR(NXOR),
	DECLARE_CLEANARGNIFFXOR(NIFF)
	};

const MetaConceptWithArgArray::EvalRuleIdx_ER MetaConnective::Idempotent2AryRulesTable[NIFF_MCM+1]
  = { DECLARE_ARG(AND),
      DECLARE_ARG(OR),
	  DECLARE_TRUE(IFF),
	  DECLARE_CONTRADICTION(XOR),
	  DECLARE_TRUE(NXOR),
	  DECLARE_CONTRADICTION(NIFF)
	 };

// NOTE: MetaConnective::DiagnoseInferenceRulesTrueNAry assumes only LogicalAND entry is DECLARE_CLEANARG
const signed short MetaConnective::TruthValueTrueAryNSelfTable[NIFF_MCM+1]
  =	{	DECLARE_CLEANARG(AND),
		None_SER,
		DECLARE_CONVERT_TO_AND(IFF),
		DECLARE_CONVERT_TO_NOR(XOR),
		DECLARE_CONVERT_TO_OR(NXOR),
		DECLARE_CONVERT_TO_NAND(NIFF)
	};

// NOTE: MetaConnective::DiagnoseInferenceRulesFalseNAry assumes only LogicalOR entry is DECLARE_CLEANARG
const signed short MetaConnective::TruthValueFalseAryNSelfTable[NIFF_MCM]
  =	{	DECLARE_CLEANARG(OR),
		DECLARE_CONVERT_TO_NOR(IFF),
		DECLARE_CLEANARGNIFFXOR(XOR),
		DECLARE_CLEANARGNXOR(NXOR),
		DECLARE_CONVERT_TO_OR(NIFF)
	};

static constexpr const MetaConceptWithArgArray::EvalRuleIdx_ER truthValueFalseAry2Table[IFF_MCM+1]
  =	{	MetaConceptWithArgArray::DECLARE_FALSE(AND),
		MetaConceptWithArgArray::DECLARE_ARG(OR),
		MetaConceptWithArgArray::DECLARE_NOTARG(IFF)
	};

#undef DECLARE_ARG
#undef DECLARE_NOTARG
#undef DECLARE_CONTRADICTION
#undef DECLARE_TRUE
#undef DECLARE_FALSE
#undef DECLARE_UNKNOWN

const MetaConnective::UseThisAsMakeImplyAux MetaConnective::UseThisAsMakeImply2AryTable[IFF_MCM+1]
  =	{	&MetaConnective::UseThisAsMakeImply2AryAND,
		&MetaConnective::UseThisAsMakeImply2AryOR,
		&MetaConnective::UseThisAsMakeImply2AryIFF
	};

const MetaConnective::UseThisAsMakeImplyAux MetaConnective::UseThisAsMakeImplyNAryTable[NXOR_MCM+1]
  =	{	&MetaConnective::UseThisAsMakeImplyNAryAND,
		&MetaConnective::UseThisAsMakeImplyNAryOR,
		&MetaConnective::UseThisAsMakeImplyNAryIFF,
		&MetaConnective::UseThisAsMakeImplyNAryXOR,
		&MetaConnective::UseThisAsMakeImplyNAryNXOR
	};

// lookup tables
#define StrictlyImplies_NOR NORNANDFatal
#define StrictlyImplies_NAND NORNANDFatal
#define StrictlyImpliesLogicalNOTOf_NOR NORNANDFatal
#define StrictlyImpliesLogicalNOTOf_NAND NORNANDFatal
#define LogicalNOTOfStrictlyImplies_NOR NORNANDFatal
#define LogicalNOTOfStrictlyImplies_NAND NORNANDFatal
#define LogicalNOTOfStrictlyImplies_NXOR StrictlyImplies_XOR
#define LogicalNOTOfStrictlyImplies_XOR StrictlyImplies_NXOR
#define StrictlyModifies_NOR NORNANDFatal
#define StrictlyModifies_NAND NORNANDFatal
#define CanStrictlyModify_NOR NORNANDFatal
#define CanStrictlyModify_NAND NORNANDFatal

// NOTE: this lookup is now computable "on the fly" [index in is MetaConcept ID shifted
// to start at 0].  Technically, we don't have to store a pointer to this table now.

const MetaConnective::LogicalANDFindDetailedRuleAux MetaConnective::ANDDetailedRuleAux[NIFF_MCM-AND_MCM+1]
  =	{
    NULL,
	&MetaConnective::LogicalANDFindDetailedRuleForOR,
	&MetaConnective::LogicalANDFindDetailedRuleForIFF,
	NULL,
	NULL,
	NULL
	};

const MetaConnective::StrictModifyAuxFunc MetaConnective::StrictlyModifiesAux[NIFF_MCM-AND_MCM+1]
  =	{
	&MetaConnective::StrictlyModifies_AND,
	&MetaConnective::StrictlyModifies_OR,
	&MetaConnective::StrictlyModifies_IFF,
	&MetaConnective::StrictlyModifies_XOR,
	&MetaConnective::StrictlyModifies_NXOR,
	&MetaConnective::StrictlyModifies_NIFF
	};

const MetaConnective::BinaryRelationAuxFunc2 MetaConnective::StrictlyImpliesAux[StrictBound_MCM]
  =	{
	&MetaConnective::StrictlyImplies_AND,
	&MetaConnective::StrictlyImplies_OR,
	&MetaConnective::StrictlyImplies_IFF,
	&MetaConnective::StrictlyImplies_XOR,
	&MetaConnective::StrictlyImplies_NXOR,
	&MetaConnective::StrictlyImplies_NIFF,
	&MetaConnective::StrictlyImplies_NOR,
	&MetaConnective::StrictlyImplies_NAND
	};

const MetaConnective::BinaryRelationAuxFunc2 MetaConnective::CanStrictlyModifyAux[StrictBound_MCM]
  =	{
	&MetaConnective::CanStrictlyModify_AND,
	&MetaConnective::CanStrictlyModify_OR,
	&MetaConnective::CanStrictlyModify_IFF,
	&MetaConnective::CanStrictlyModify_XOR,
	&MetaConnective::CanStrictlyModify_NXOR,
	&MetaConnective::CanStrictlyModify_NIFF,
	&MetaConnective::CanStrictlyModify_NOR,
	&MetaConnective::CanStrictlyModify_NAND
	};

#undef StrictlyModifies_NOR
#undef StrictlyModifies_NAND
#undef CanStrictlyModify_NOR
#undef CanStrictlyModify_NAND
#undef LogicalNOTOfStrictlyImplies_NOR
#undef LogicalNOTOfStrictlyImplies_NAND
#undef LogicalNOTOfStrictlyImplies_NXOR
#undef LogicalNOTOfStrictlyImplies_XOR
#undef StrictlyImpliesLogicalNOTOf_NAND
#undef StrictlyImpliesLogicalNOTOf_NOR
#undef StrictlyImplies_NAND
#undef StrictlyImplies_NOR

#undef DECLARE_MCVFT

static bool NAryAllArgsEqualExceptOneAntiIdempotentPair(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2003
	if (lhs.size()!=rhs.size()) return false;

	// ???
	const MetaConceptWithArgArray& VR_lhs = static_cast<const MetaConceptWithArgArray&>(lhs);
	const MetaConceptWithArgArray& VR_rhs = static_cast<const MetaConceptWithArgArray&>(rhs);

	if (!VR_lhs.FindTwoRelatedArgs(VR_rhs,IsAntiIdempotentTo)) return false;

	size_t LHS_Idx1 = VR_lhs.ImageInferenceParameter1();
	size_t i = VR_lhs.size();
	do	if (   --i!=LHS_Idx1
			&& !VR_rhs.FindArgRelatedToLHS(*VR_lhs.ArgN(i),AreSyntacticallyEqual))
			return false;
	while(0<i);
	return true;
}

static bool NAryAllArgsEqualExceptOne(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/25/2003
	if (lhs.size()!=rhs.size()) return false;

	const MetaConceptWithArgArray& VR_lhs = static_cast<const MetaConceptWithArgArray&>(lhs);
	const MetaConceptWithArgArray& VR_rhs = static_cast<const MetaConceptWithArgArray&>(rhs);

	size_t LHSIdx = lhs.size();
	while(VR_rhs.FindArgRelatedToLHS(*lhs.ArgN(--LHSIdx),AreSyntacticallyEqual))
		if (0==LHSIdx)
			return false;

	if (0<LHSIdx)
		{
		size_t LHSIdx2 = LHSIdx;
		do	if (!VR_rhs.FindArgRelatedToLHS(*lhs.ArgN(--LHSIdx2),AreSyntacticallyEqual))
				return false;
		while(0<LHSIdx2);
		}

	size_t RHSIdx = rhs.size();
	while(VR_lhs.FindArgRelatedToLHS(*rhs.ArgN(--RHSIdx),AreSyntacticallyEqual))
		if (0==RHSIdx)
			return false;

	VR_lhs.FindArgRelatedToLHS(*lhs.ArgN(LHSIdx),AreSyntacticallyEqual);
	VR_rhs.FindArgRelatedToLHS(*rhs.ArgN(RHSIdx),AreSyntacticallyEqual);
	return true;
}

#if 0
bool NAryAllArgsEqualExceptTwoAntiIdempotentPairs(const MetaConcept& LHS, const MetaConcept& RHS)
{
	if (LHS.size()!=RHS.size())
		return false;

	MetaConceptWithArgArray& VR_LHS = *static_cast<const MetaConceptWithArgArray*>(&LHS);
	MetaConceptWithArgArray& VR_RHS = *static_cast<const MetaConceptWithArgArray*>(&RHS);

	if (!VR_LHS.FindTwoRelatedArgs(VR_RHS,IsAntiIdempotentTo))
		return false;

	size_t LHS_Idx1 = VR_LHS.ImageInferenceParameter1();
	size_t RHS_Idx1 = VR_RHS.ImageInferenceParameter1();

	if (!VR_LHS.FindTwoRelatedArgs(VR_RHS,IsAntiIdempotentTo,LHS_Idx1,RHS_Idx1))
		return false;

	size_t LHS_Idx2 = VR_LHS.ImageInferenceParameter1();
	size_t RHS_Idx2 = VR_RHS.ImageInferenceParameter1();

	size_t Idx = VR_LHS.fast_size();
	do	if (   --Idx!=LHS_Idx1
			&&   Idx!=LHS_Idx2
			&& !VR_RHS.FindArgRelatedToLHS(VR_LHS.ArgN(Idx),AreSyntacticallyEqual))
			return false;
	while(0<Idx);
	return true;
}
#endif


// OPTIMIZATION NOTES:
// Anything called from: SyntaxOK, [Can][DestructiveSelf]Evaluate will be used heavily

//	META: In general, Franci should 'prefer' to print out short names
//		**	Franci should prefer A to NOT A

// Some things that might be in LowRel.cxx, if they weren't so specific
static void StrictlyModify(MetaConcept*& Target, const MetaConcept& Inducer)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/30/2000
	Inducer.StrictlyModifies(Target);
}

// Main code
MetaConnective::MetaConnective(MetaConcept**& NewArgList, MetaConnectiveModes LinkageType)
:	MetaConceptWithArgArray((ExactType_MC)(LinkageType+LogicalAND_MC),NewArgList)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/18/1999
	// If the syntax is already bad, then we don't need to proceed further
	if (!SyntaxOK()) return;
	// Arity 2: NXOR(A,B) to IFF(A,B), NIFF(A,B) to XOR(A,B); XOR(A,B) to IFF(A,~B)
	if (2==fast_size())
		{
		if (IsExactType(LogicalNXOR_MC))
			SetExactType(LogicalIFF_MC);
		else if (   IsExactType(LogicalNIFF_MC)
				 || IsExactType(LogicalXOR_MC))
			{
			SetExactType(LogicalIFF_MC);
			ArgArray[1]->SelfLogicalNOT();
			}
		}
	_forceStdForm();
	// NAND, NOR: DeMorgan!
	if (LogicalNOR_MC<=ExactType()) DoSelfDeMorgan();
}

bool MetaConnective::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/2000
	if (MetaConceptWithArgArray::EqualAux2(rhs)) return true;
	if (IsExactType(LogicalIFF_MC))
		return OrderIndependentPairwiseRelation(static_cast<const MetaConceptWithArgArray&>(rhs),IsAntiIdempotentTo);
	return false;
}

void MetaConnective::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/2000
	if (!IdxCurrentSelfEvalRule && !IdxCurrentEvalRule)
		{
		ForceStdFormAux();
		ForceTotalLexicalArgOrder();
		};
}

//  Type ID functions
const AbstractClass* MetaConnective::UltimateType() const { return &TruthValues; }

//  Evaluation functions
bool MetaConnective::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	if (!SyntaxOKAux()) return false;	// NOTE: this routine catches NULL entries
	size_t i = fast_size();
	do	if (!ArgArray[--i]->IsUltimateType(&TruthValues)) return false;
	while(0<i);
	return true;
}

bool MetaConnective::LogicalANDOrthogonalClause() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/9/2000
	assert(2<=ArgArray.size());
	if (   IsExactType(LogicalIFF_MC)
		&& ArgArray[fast_size()-1]->IsExactType(Variable_MC)
		&& ArgArray[0]->IsExactType(Variable_MC))
		return true;
	return false;
}

// Logical Amplification support
bool MetaConnective::WantToBeAmplified() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/22/2003
	return IsExactType(LogicalAND_MC) || IsExactType(LogicalOR_MC);
}

bool MetaConnective::CanAmplifyClause() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/22/2003
	return IsExactType(LogicalIFF_MC) || IsExactType(LogicalXOR_MC)
		|| (IsExactType(LogicalOR_MC) && 2==fast_size());
}

bool MetaConnective::Prefilter_CanAmplifyThisClause(const MetaConcept& rhs) const
{
	if (!rhs.IsExactType(LogicalAND_MC) && !rhs.IsExactType(LogicalOR_MC))
		return false;
	if (NonStrictlyImplies(rhs,*this)) return false;
	if (   rhs.IsExactType(LogicalOR_MC)
		&& FindArgRelatedToLHS(rhs,::NonStrictlyImplies))
		return false;
#ifdef AMPLIFY_TAUTOLOGIES
	// checks for tautology base: we don't want IFF amplifying a tautology
	if (   rhs.IsExactType(LogicalOR_MC)
		&& 2==rhs.size()
		&& IsExactType(LogicalIFF_MC)
		&& (   rhs.ArgN(0)->IsAntiIdempotentTo(*rhs.ArgN(1))
			|| (   rhs.ArgN(0)->StrictlyImpliesLogicalNOTOf(*rhs.ArgN(1))
			    && rhs.ArgN(1)->StrictlyImpliesLogicalNOTOf(*rhs.ArgN(0)))))
		return false;
	// we don't want a tautology amplifying anything
	if (   IsExactType(LogicalOR_MC) && 2==fast_size()
		&& (   ArgArray[0]->IsAntiIdempotentTo(*ArgArray[1])
			|| (   ArgArray[0]->StrictlyImpliesLogicalNOTOf(*ArgArray[1])
			    && ArgArray[1]->StrictlyImpliesLogicalNOTOf(*ArgArray[0]))))
		return false;
#endif
	return true;
}

bool MetaConnective::OR_CanAmplifyThisClause(const MetaConnective& rhs) const
{	// FORMALLY CORRECT: 2020-05-15
	return rhs.FindTwoRelatedArgs(*this, NonStrictlyImpliesLogicalNOTOf, [&](const MetaConcept& l) {
		return !NonStrictlyImplies(l, *this);
	});
}

bool MetaConnective::XOR_CanAmplifyThisClause(const MetaConnective& rhs) const
{	// FORMALLY CORRECT: 2020-05-15
	return rhs.FindTwoRelatedArgs(*this, NonStrictlyImplies, [&](const MetaConcept& l) {
		return !NonStrictlyImplies(l, *this);
	});
}

bool MetaConnective::CanAmplifyThisClause(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: 2020-05-15
	if (!Prefilter_CanAmplifyThisClause(rhs)) return false;
	
	const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);
	// SEMANTIC ERROR: FindArgRelatedToLHS doesn't work.
	// Want FindTwoRelatedArgs for N-ary types
	if (IsExactType(LogicalOR_MC))
		{
		if (2==fast_size()) return OR_CanAmplifyThisClause(VR_rhs);
		}
	else if (IsExactType(LogicalIFF_MC))
		return VR_rhs.FindTwoRelatedArgs(*this, NonStrictlyImpliesThisOrLogicalNOTOf, [&](const MetaConcept& l) {
			return !NonStrictlyImplies(l, *this);
		});
	else if (IsExactType(LogicalXOR_MC)) return XOR_CanAmplifyThisClause(VR_rhs);
	return false;
}

bool MetaConnective::OR_AmplifyThisClause(MetaConnective& rhs) const
{
	const size_t IncomingParam1 = InferenceParameter1;
	const size_t IncomingRHSParam1 = rhs.InferenceParameter1;
	assert(rhs.ArgArray.size()>IncomingRHSParam1);
	assert(ArgArray.size()>IncomingParam1);

// backing up InferenceParameter1 because recursion test can overwrite them.  Disallow use temporarily.
#define InferenceParameter1

	if (   Prefilter_CanAmplifyThisClause(*rhs.ArgArray[IncomingRHSParam1])
		&& OR_CanAmplifyThisClause(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1])))
		return OR_AmplifyThisClause(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1]));

	std::unique_ptr<MetaConnective> NewArg;
	try	{
		NewArg = std::unique_ptr<MetaConnective>(new MetaConnective(*this));
		}
	catch(const bad_alloc&)
		{
		return false;
		}
			
	NewArg->ArgArray[IncomingParam1]->SelfLogicalNOT();
	if (IsAntiIdempotentTo(*ArgArray[IncomingParam1],*rhs.ArgArray[IncomingRHSParam1]))
		{	// Anti-idempotent filter
		delete rhs.ArgArray[IncomingRHSParam1];
		}
	else{	// StrictlyImpliesLogicalNOTOf
		if (!NewArg->AddArgAtEndAndForceCorrectForm(rhs.ArgArray[IncomingRHSParam1]))
			return false;
		}
	NewArg->SetExactTypeV2(LogicalAND_MC);
	rhs.ArgArray[IncomingRHSParam1] = NewArg.release();
	// immediately normalize AND
	if (rhs.IsExactType(LogicalAND_MC))
		{
		rhs.SilentDiagnoseSelfAssociativeArgs();
		if (!rhs.DestructiveEvaluateToSameType()) FATAL(RAMFailure);
		}

// Re-enable use of InferenceParameter1
#undef InferenceParameter1

	return true;
}

bool MetaConnective::IFF_AmplifyThisClauseV1(MetaConnective& rhs) const
{
	const size_t IncomingParam1 = InferenceParameter1;
	const size_t IncomingRHSParam1 = rhs.InferenceParameter1;
	assert(ArgArray.size()>IncomingParam1);
	assert(rhs.ArgArray.size()>IncomingRHSParam1);

// backing up InferenceParameter1 because recursion test can overwrite them.  Disallow use temporarily.
#define InferenceParameter1

	if (Prefilter_CanAmplifyThisClause(*rhs.ArgArray[IncomingRHSParam1]))
		{
		if (rhs.FindTwoRelatedArgs(*this,NonStrictlyImpliesLogicalNOTOf, [&](const MetaConcept& l) {
			return !NonStrictlyImplies(l, *this);
		}))
			return IFF_AmplifyThisClauseV1(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1]));
		else if (rhs.FindTwoRelatedArgs(*this,NonStrictlyImplies, [&](const MetaConcept& l) {
			return !NonStrictlyImplies(l, *this);
		}))
			return IFF_AmplifyThisClauseV2(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1]));
		}

	std::unique_ptr<MetaConnective> NewArg;
	try	{
		NewArg = std::unique_ptr<MetaConnective>(new MetaConnective(*this));
		}
	catch(const bad_alloc&)
		{
		return false;
		}
		
	if (IsAntiIdempotentTo(*ArgArray[IncomingParam1],*rhs.ArgArray[IncomingRHSParam1]))
		{	// Anti-idempotent filter
		delete rhs.ArgArray[IncomingRHSParam1];
		}
	else{	// StrictlyImpliesLogicalNOTOf
		rhs.ArgArray[IncomingRHSParam1]->SelfLogicalNOT();
		if (!NewArg->AddArgAtEndAndForceCorrectForm(rhs.ArgArray[IncomingRHSParam1]))
			return false;
		}
	NewArg->SetNANDNOR(LogicalNOR_MC);
	rhs.ArgArray[IncomingRHSParam1] = NewArg.release();
	// immediately normalize AND
	if (rhs.IsExactType(LogicalAND_MC))
		{
		rhs.SilentDiagnoseSelfAssociativeArgs();
		if (!rhs.DestructiveEvaluateToSameType()) FATAL(RAMFailure);
		}

// Re-enable use of InferenceParameter1
#undef InferenceParameter1

	return true;
}

bool MetaConnective::IFF_AmplifyThisClauseV2(MetaConnective& rhs) const
{
	const size_t IncomingParam1 = InferenceParameter1;
	const size_t IncomingRHSParam1 = rhs.InferenceParameter1;
	assert(ArgArray.size()>IncomingParam1);
	assert(rhs.ArgArray.size()>IncomingRHSParam1);

// backing up InferenceParameter1 because recursion test can overwrite them.  Disallow use temporarily.
#define InferenceParameter1

	if (Prefilter_CanAmplifyThisClause(*rhs.ArgArray[IncomingRHSParam1]))
		{
		if (rhs.FindTwoRelatedArgs(*this,NonStrictlyImpliesLogicalNOTOf, [&](const MetaConcept& l) {
			return !NonStrictlyImplies(l, *this);
		}))
			return IFF_AmplifyThisClauseV1(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1]));
		else if (rhs.FindTwoRelatedArgs(*this,NonStrictlyImplies, [&](const MetaConcept& l) {
			return !NonStrictlyImplies(l, *this);
		}))
			return IFF_AmplifyThisClauseV2(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1]));
		}

	std::unique_ptr<MetaConnective> NewArg;
	try	{
		NewArg = std::unique_ptr<MetaConnective>(new MetaConnective(*this));
		}
	catch(const bad_alloc&)
		{
		return false;
		}
		
	if (*ArgArray[IncomingParam1]==*rhs.ArgArray[IncomingRHSParam1])
		{	// Syntactically equal filter
		delete rhs.ArgArray[IncomingRHSParam1];
		}
	else{	// StrictlyImplies
		if (!NewArg->AddArgAtEndAndForceCorrectForm(rhs.ArgArray[IncomingRHSParam1]))
			return false;
		}			
	NewArg->SetExactTypeV2(LogicalAND_MC);
	rhs.ArgArray[IncomingRHSParam1] = NewArg.release();
	// immediately normalize AND
	if (rhs.IsExactType(LogicalAND_MC))
		{
		rhs.SilentDiagnoseSelfAssociativeArgs();
		if (!rhs.DestructiveEvaluateToSameType()) FATAL(RAMFailure);
		}

// Re-enable use of InferenceParameter1
#undef InferenceParameter1

	return true;
}

bool MetaConnective::XOR_AmplifyThisClause(MetaConnective& rhs) const
{
	const size_t IncomingParam1 = InferenceParameter1;
	const size_t IncomingRHSParam1 = rhs.InferenceParameter1;
	assert(ArgArray.size()>IncomingParam1);
	assert(rhs.ArgArray.size()>IncomingRHSParam1);

// backing up InferenceParameter1 because recursion test can overwrite them.  Disallow use temporarily.
#define InferenceParameter1

	if (   Prefilter_CanAmplifyThisClause(*rhs.ArgArray[IncomingRHSParam1])
		&& XOR_CanAmplifyThisClause(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1])))
		return XOR_AmplifyThisClause(*static_cast<MetaConnective*>(rhs.ArgArray[IncomingRHSParam1]));

	std::unique_ptr<MetaConnective> NewArg;
	try	{
		NewArg = std::unique_ptr<MetaConnective>(new MetaConnective(*this));
		}
	catch(const bad_alloc&)
		{
		return false;
		}
			
	NewArg->ArgArray[IncomingParam1]->SelfLogicalNOT();
	if (*ArgArray[IncomingParam1]==*rhs.ArgArray[IncomingRHSParam1])
		{	// Syntactically equal filter
		delete rhs.ArgArray[IncomingRHSParam1];
		}
	else{	// StrictlyImplies
		rhs.ArgArray[IncomingRHSParam1]->SelfLogicalNOT();
		if (NewArg->AddArgAtEndAndForceCorrectForm(rhs.ArgArray[IncomingRHSParam1]))
			return false;
		}
	NewArg->SetNANDNOR(LogicalNOR_MC);
	rhs.ArgArray[IncomingRHSParam1] = NewArg.release();
	// immediately normalize AND
	if (rhs.IsExactType(LogicalAND_MC))
		{
		rhs.SilentDiagnoseSelfAssociativeArgs();
		if (!rhs.DestructiveEvaluateToSameType()) FATAL(RAMFailure);
		}

// Re-enable use of InferenceParameter1
#undef InferenceParameter1

	return true;
}

bool MetaConnective::AmplifyThisClause(MetaConcept*& rhs) const
{	// FORMALLY CORRECT: 10/17/2004
	assert(rhs);
	assert(typeid(MetaConnective)==typeid(*rhs));
	MetaConnective& VR_rhs = *static_cast<MetaConnective*>(rhs);
	// General: must recurse AND, OR finds as appropriate
	if (IsExactType(LogicalOR_MC) && 2==fast_size())
		{
		if (VR_rhs.FindTwoRelatedArgs(*this,NonStrictlyImpliesLogicalNOTOf))
			return OR_AmplifyThisClause(VR_rhs);
		return false;
		}
	if (IsExactType(LogicalIFF_MC))
		{
		if (VR_rhs.FindTwoRelatedArgs(*this,NonStrictlyImpliesLogicalNOTOf))
			return IFF_AmplifyThisClauseV1(VR_rhs);
		if (VR_rhs.FindTwoRelatedArgs(*this,NonStrictlyImplies))
			return IFF_AmplifyThisClauseV2(VR_rhs);
		return false;
		}
	if (IsExactType(LogicalXOR_MC))
		{
		if (VR_rhs.FindTwoRelatedArgs(*this,NonStrictlyImplies))
			return XOR_AmplifyThisClause(VR_rhs);
		return false;
		}
	return false;
}


// Basis clause support
// OR: N+1: N arity-reductions, then NIFF-conversion.  Arity-reductions are normally boring, 
// however.
// IFF: two: AND and OR
//! \todo Basis clause support for XOR: 0 to N (vars are already accounted for)
size_t MetaConnective::BasisClauseCount() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if (IsExactType(LogicalIFF_MC)) return 2;
	else if (IsExactType(LogicalOR_MC))
		{
		if (2==fast_size())
			{	// special rule, can spawn naked variables (don't do that)
			if (ArgArray[0]->IsExactType(Variable_MC))
				return (ArgArray[1]->IsExactType(Variable_MC)) ? 1 : 2;
			else
				return (ArgArray[1]->IsExactType(Variable_MC)) ? 2 : 3;
			}
		return fast_size()+1;	// 1, plus n-1 arity reductions that use NIFF
		}
	else if (IsExactType(LogicalNIFF_MC))
		{
		if (   3==fast_size()
			&& ArgArray[0]->IsExactType(Variable_MC)
			&& ArgArray[1]->IsExactType(Variable_MC)
			&& ArgArray[2]->IsExactType(Variable_MC))
			return 0;					// special rule.  Don't pollute.
		const size_t Arity = fast_size();
		const size_t AritySub1 = Arity-1;

		if (LONG_MAX/Arity>=AritySub1)
			return Arity*AritySub1;
		else
			return LONG_MAX;
		}
	return 0;
}

bool MetaConnective::DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const
{	//! \todo Basis clause support for XOR 0 to N
	assert(BasisClauseCount()>Idx);
	assert(!dest);
	try	{
		if      (IsExactType(LogicalIFF_MC))
			{
			MetaConnective* tmp_mc = new MetaConnective(*this);
			if (0==Idx)
				tmp_mc->SetExactTypeV2(LogicalAND_MC);
			else	// if (1==Idx)
				tmp_mc->SetNANDNOR(LogicalNOR_MC);
			dest = tmp_mc;
			return true;
			}
		else if (IsExactType(LogicalOR_MC))
			{
			if (0==Idx)
				{
				MetaConnective* tmp_mc = new MetaConnective(*this);
				tmp_mc->SetExactTypeV2(LogicalAND_MC);
				dest = tmp_mc;
				return true;
				}
			if (fast_size()>=Idx)
				{
				if (2==fast_size())
					{
					if 		(ArgArray[0]->IsExactType(Variable_MC))
						Idx = 1;
					else if (ArgArray[1]->IsExactType(Variable_MC))
						Idx = 2;
					ArgArray[2-Idx]->CopyInto(dest);
					dest->SelfLogicalNOT();
					return true;
					}
				else{
					MetaConnective* tmp_mc = new MetaConnective(*this);
					tmp_mc->DeleteIdx(Idx-1);
					tmp_mc->SelfLogicalNOT();
					dest = tmp_mc;
					return true;
					}
				}
			}
		else if (IsExactType(LogicalNIFF_MC))
			{	// The logical negation of any 2-ary IFF using these args implies the NIFF
			// 3-ary case is simpler: just use all 3 args, both as normal and as logically negated.
			if (3==fast_size())
				{
				ArgArray[Idx%3]->CopyInto(dest);
				if (3<=Idx) dest->SelfLogicalNOT();
				return true;
				}

			autovalarray_ptr<MetaConcept*> NewArgArray(2);
			if (NewArgArray.empty()) return false;

			size_t Arity = fast_size()-1;
			size_t FirstArg = 0;
			bool incremental_data_mode = true;

			while(Idx>=Arity)
				{
				FirstArg++;
				Idx -= Arity;
				if (1==--Arity)
					{
					if (0==Idx) break;
					if (!incremental_data_mode) return false;
					incremental_data_mode = false;
					Idx -= 1;
					Arity = fast_size()-1;
					FirstArg = 0;
					}
				};

			ArgArray[FirstArg]->CopyInto(NewArgArray[0]);
			ArgArray[FirstArg+Idx+1]->CopyInto(NewArgArray[1]);

			if (incremental_data_mode) NewArgArray[1]->SelfLogicalNOT();

			dest = new MetaConnective(NewArgArray,IFF_MCM);
			return true;
			}
		}
	catch(const bad_alloc&)
		{
		return false;
		}
	FATAL(AlphaMiscallVFunction);
	return false;	
}

// IMPLEMENT FOR: AND, IFF, NXOR(?)
size_t MetaConnective::ANDFactorCount() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2003
	if 		(IsExactType(LogicalAND_MC))
		return fast_size();
	else if (IsExactType(LogicalIFF_MC))
		{
		size_t Arity = fast_size();
		size_t AritySub1 = Arity-1;

		if (0==Arity%2)
			Arity /= 2;
		else
			AritySub1 /= 2;

		if (SIZE_MAX/Arity>=AritySub1)
			return Arity*AritySub1;
		else
			return SIZE_MAX;
		}
	return 0;
}

bool MetaConnective::DirectCreateANDFactorIdx(size_t Idx, MetaConcept*& dest) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2003
	assert(ANDFactorCount()>Idx);
	assert(!dest);
	try	{
		if 		(IsExactType(LogicalAND_MC))
			{
			assert(ArgArray.size()>Idx);
			ArgArray[Idx]->CopyInto(dest);
			return true;
			}
		else if (IsExactType(LogicalIFF_MC))
			{
			autovalarray_ptr<MetaConcept*> NewArgArray(2);
			if (!NewArgArray) return false;

			size_t Arity = fast_size()-1;
			size_t FirstArg = 0;

			while(1<Arity && Idx>=Arity)
				{
				FirstArg++;
				Idx -= Arity--;
				};
			assert(Idx<Arity);
			assert(ArgArray.size()>FirstArg);
			assert(ArgArray.size()>FirstArg+Idx+1);

			ArgArray[FirstArg]->CopyInto(NewArgArray[0]);
			ArgArray[FirstArg+Idx+1]->CopyInto(NewArgArray[1]);
			dest = new MetaConnective(NewArgArray,IFF_MCM);
			return true;
			}
		}
	catch(const bad_alloc&)
		{
		return false;
		}
	FATAL(AlphaMiscallVFunction);
	return false;	
}

bool MetaConnective::CanMakeLHSImplyRHS() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	return (IsExactType(LogicalIFF_MC)) || IsExactType(LogicalXOR_MC) || (IsExactType(LogicalOR_MC) && 2==fast_size());
}

bool MetaConnective::MakesLHSImplyRHS(const MetaConcept& lhs, const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	if 		(IsExactType(LogicalOR_MC) && 2==fast_size())
		return (   (NonStrictlyImpliesLogicalNOTOf(lhs,*ArgArray[0]) && NonStrictlyImplies(*ArgArray[1],rhs))
				|| (NonStrictlyImpliesLogicalNOTOf(lhs,*ArgArray[1]) && NonStrictlyImplies(*ArgArray[0],rhs)));
	else if (IsExactType(LogicalIFF_MC))
		{	// LHS => OR and AND => RHS: OK
		// NOTE: this code can be spoofed by IFFs containing OR [LHS blinded] or AND [RHS blinded]
		const_cast<MetaConnective* const>(this)->SetExactTypeV2(LogicalOR_MC);
		const bool LHSImpliesORVariantOfIFF = NonStrictlyImplies(lhs,*this);
		const bool NORVariantOfIFFImpliesRHS = LogicalNOTOfNonStrictlyImplies(*this,rhs);
		const_cast<MetaConnective* const>(this)->SetExactTypeV2(LogicalAND_MC);
		const bool ANDVariantOfIFFImpliesRHS = NonStrictlyImplies(*this,rhs);
		const bool LHSImpliesNANDVariantOfIFF = NonStrictlyImpliesLogicalNOTOf(lhs,*this);
		const_cast<MetaConnective* const>(this)->SetExactTypeV2(LogicalIFF_MC);
		if (   (LHSImpliesORVariantOfIFF && ANDVariantOfIFFImpliesRHS)
			|| (NORVariantOfIFFImpliesRHS && LHSImpliesNANDVariantOfIFF))
			return true;
		return false;
		}
	else if (IsExactType(LogicalXOR_MC))
		{
		size_t Arg1;
		// XOR(A,B,C): if D=>A and ~C=>E, then XOR(A,B,C) enables D=>E
		if (   FindArgRelatedToLHS(lhs,NonStrictlyImplies)
			&& (Arg1 = InferenceParameter1, FindArgRelatedToRHS(rhs, LogicalNOTOfNonStrictlyImplies, [&](const MetaConcept& l) {
					return l != *ArgArray[Arg1];
				})))
			return true;
		// XOR(A,B,C): if D=>~B, D=>~C, and A=>E, then XOR(A,B,C) enables D=>E
		if (FindArgRelatedToRHS(rhs,NonStrictlyImplies))
			{
			size_t i = fast_size();
			do	if (   InferenceParameter1!= --i
					&& !NonStrictlyImpliesLogicalNOTOf(lhs,*ArgArray[i]))
					return false;
			while(0<i);
			return true;
			}
		return false;
		}
	FATAL(AlphaMiscallVFunction);
	return false;
}

bool MetaConnective::ValidLHSForMakesLHSImplyRHS(const MetaConcept& lhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	if 		(IsExactType(LogicalOR_MC) && 2==fast_size())
		return (   NonStrictlyImpliesLogicalNOTOf(lhs,*ArgArray[0])
				|| NonStrictlyImpliesLogicalNOTOf(lhs,*ArgArray[1]));
	else if (IsExactType(LogicalIFF_MC))
		return (   FindArgRelatedToLHS(lhs,NonStrictlyImplies)
				|| FindArgRelatedToLHS(lhs,LogicalNOTOfNonStrictlyImplies));
	else if (IsExactType(LogicalXOR_MC))
		return FindArgRelatedToLHS(lhs,NonStrictlyImplies);
	FATAL(AlphaMiscallVFunction);
	return false;
}

bool MetaConnective::ValidRHSForMakesLHSImplyRHS(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	if 		(IsExactType(LogicalOR_MC) && 2==fast_size())
		return (   NonStrictlyImplies(*ArgArray[1],rhs)
				|| NonStrictlyImplies(*ArgArray[0],rhs));
	else if (IsExactType(LogicalIFF_MC))
		return (   FindArgRelatedToRHS(rhs,NonStrictlyImplies)
				|| FindArgRelatedToRHS(rhs,LogicalNOTOfNonStrictlyImplies));
	else if (IsExactType(LogicalXOR_MC))
		return FindArgRelatedToRHS(rhs,LogicalNOTOfNonStrictlyImplies);
	FATAL(AlphaMiscallVFunction);
	return false;
}

#if 0
static void DismemberORInIdxSourceList(MetaConcept**& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/19/2001
	assert(NULL!=dest);
	size_t NewArgs = 0;
	size_t i = ArraySize(dest);
	do	if (dest[--i]->IsExactType(LogicalOR_MC))
			NewArgs += dest[i]->size()-1;
	while(0<i);
	if (0<NewArgs)
		{
		size_t NewLowBound = i = ArraySize(dest);
		if (!_resize(dest,NewLowBound+NewArgs)) return;

		do	if (Target[--i]->IsExactType(LogicalOR_MC))
				{	// dismember this OR's arglist.
				MetaConnective* TmpOR = static_cast<MetaConnective*>(dest[i]);
				size_t j = TmpOR->size();
				do	TmpOR->TransferOutAndNULL(--j,dest[NewLowBound++]);
				while(1<j);
				TmpOR->TransferOutAndNULL(0,dest[i]);
				delete TmpOR;
				}
		while(0<i);
		}
}

bool MetaConnective::InitIdxSourceList(MetaConcept**& dest) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/7/2000
	assert(NULL==dest);
	try	{
		if 		(IsExactType(LogicalOR_MC) && 2==fast_size())
			{	// Want logical negation of each arg: 2
			dest = _new_buffer<MetaConcept*>(2);
			if (NULL==dest) return false;
			size_t Idx = 2;
			do	{
				--Idx;
				ArgArray[Idx]->CopyInto(dest[Idx]);
				dest[Idx]->SelfLogicalNOT();
				}
			while(0<Idx);
			DismemberORInIdxSourceList(dest);
			return true;
			}
		else if (IsExactType(LogicalIFF_MC))
			{	// Want args, and logical negation: 2n
			//! \todo IMPLEMENT: smart algorithm that ignores substituted-out vars but not complex expressions beyond
			dest = _new_buffer<MetaConcept*>(fast_size()<<1);
			if (NULL==dest) return false;
			size_t Idx = fast_size();
			do	{
				--Idx;
				ArgArray[Idx]->CopyInto(dest[Idx]);
				ArgArray[Idx]->CopyInto(dest[Idx+fast_size()]);
				dest[Idx+fast_size()]->SelfLogicalNOT();
				}
			while(0<Idx);
			DismemberORInIdxSourceList(dest);
			return true;
			}
		else if (IsExactType(LogicalXOR_MC))
			{	// Want args: n
			dest = _new_buffer<MetaConcept*>(fast_size());
			if (NULL==dest) return false;
			size_t Idx = fast_size();
			do	{
				--Idx;
				ArgArray[Idx]->CopyInto(dest[Idx]);
				}
			while(0<Idx);
			DismemberORInIdxSourceList(dest);
			return true;
			};
		FATAL(AlphaMiscallVFunction);
		return false;
		}
	catch(const bad_alloc&)
		{
		BLOCKDELETEARRAY_AND_NULL(dest);
		return false;
		};
}
#endif

typedef bool _canUseAsMakeImply(const MetaConcept& Target,const MetaConcept* const * const ArgArray);

static bool canUseThisAsMakeImply2AryANDOR(const MetaConcept& Target,const MetaConcept* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2000
	assert(ArgArray);
	assert(2==ArraySize(ArgArray));
	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[0]))
		{
		if (   Target.MakesLHSImplyRHS(*ArgArray[0],*ArgArray[1])
			|| Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[0],*ArgArray[1]))
			return true;
		};
	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[1]))
		{
		if (   Target.MakesLHSImplyRHS(*ArgArray[1],*ArgArray[0])
			|| Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[1],*ArgArray[0]))
			return true;
		};
	return false;
}

static bool canUseThisAsMakeImply2AryIFF(const MetaConcept& Target,const MetaConcept* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/29/2000
	assert(ArgArray);
	assert(2==ArraySize(ArgArray));
	if (   Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[0],*ArgArray[1])
		|| Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[1],*ArgArray[0]))
		return true;
	return false;
}

static _canUseAsMakeImply* const canUseThisAsMakeImply2AryTable[]
  =	{	canUseThisAsMakeImply2AryANDOR,
		canUseThisAsMakeImply2AryANDOR,
		canUseThisAsMakeImply2AryIFF
	};
static_assert(STATIC_SIZE(canUseThisAsMakeImply2AryTable)==IFF_MCM+1);

static bool canUseThisAsMakeImplyNAryAND(const MetaConcept& Target,const MetaConcept* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/25/2001
	assert(ArgArray);
	assert(2<ArraySize(ArgArray));
	size_t i = ArraySize(ArgArray);
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			size_t j = ArraySize(ArgArray);
			do	if (    i!= --j
					&& (   Target.MakesLHSImplyRHS(*ArgArray[i],*ArgArray[j])
						|| Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[i],*ArgArray[j])))
				return true;
			while(0<j);
			}
	while(0<i);
	return false;
}

static bool canUseThisAsMakeImplyNAryORXORNXOR(const MetaConcept& Target,const MetaConcept* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/29/2000
	assert(ArgArray);
	assert(2<ArraySize(ArgArray));
	size_t i = ArraySize(ArgArray);
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			size_t j = ArraySize(ArgArray);
			do	if (   i!=--j
					&& Target.MakesLHSImplyRHS(*ArgArray[i],*ArgArray[j]))
					return true;
			while(0<j);
			}
	while(0<i);
	return false;
}

static bool canUseThisAsMakeImplyNAryIFF(const MetaConcept& Target,const MetaConcept* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/29/2000
	assert(ArgArray);
	assert(2<ArraySize(ArgArray));
	size_t i = ArraySize(ArgArray);
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			size_t j = ArraySize(ArgArray);
			do	if (   i!= --j
					&& Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[i],*ArgArray[j]))
					return true;
			while(0<j);
			}
	while(0<i);
	return false;
}

static _canUseAsMakeImply* const canUseThisAsMakeImplyNAryTable[]
  =	{	canUseThisAsMakeImplyNAryAND,
		canUseThisAsMakeImplyNAryORXORNXOR,
		canUseThisAsMakeImplyNAryIFF,
		canUseThisAsMakeImplyNAryORXORNXOR,
		canUseThisAsMakeImplyNAryORXORNXOR
	};
static_assert(STATIC_SIZE(canUseThisAsMakeImplyNAryTable)==NXOR_MCM+1);

bool MetaConnective::CanUseThisAsMakeImply(const MetaConcept& Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/21/2000
	// this returns true iff the Target generates an implies that can be used for reduction.
	// cf. StrictlyImplies, StrictlyImpliesLogicalNOTOf handlers
	if (2==fast_size())
		// AND, OR, IFF
		return (canUseThisAsMakeImply2AryTable[array_index()])(Target,ArgArray);
	else if (!IsExactType(LogicalNIFF_MC))
		// AND, OR, IFF, XOR, NXOR
		return (canUseThisAsMakeImplyNAryTable[array_index()])(Target,ArgArray);
	return false;
}

void MetaConnective::UseThisAsMakeImply(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/25/2001
	// this activates iff the Target generates an implies that can be used for reduction.
	// cf. StrictlyImplies, StrictlyImpliesLogicalNOTOf handlers
	(this->*((2==fast_size()) ? UseThisAsMakeImply2AryTable : UseThisAsMakeImplyNAryTable)[ExactType()-LogicalAND_MC])(Target);
}

void MetaConnective::UseThisAsMakeImply2AryAND(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2000
	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[0]))
		{
		if (Target.MakesLHSImplyRHS(*ArgArray[0],*ArgArray[1]))
			{
			InvokeEvalForceArg(0);
			return;
			};
		if (Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[0],*ArgArray[1]))
			{	// AND(A,B): CONTRADICTION [AND(A,B,OR(~A,~B))|->...|->FALSE]
			InvokeEvalForceFalse();
			return;
			}
		};
	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[1]))
		{
		if (Target.MakesLHSImplyRHS(*ArgArray[1],*ArgArray[0]))
			{
			InvokeEvalForceArg(1);
			return;
			};
		if (Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[1],*ArgArray[0]))
			{	// AND(A,B): CONTRADICTION [AND(A,B,OR(~A,~B))|->...|->FALSE]
			InvokeEvalForceFalse();
			return;
			};
		};
}

void MetaConnective::UseThisAsMakeImply2AryOR(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/27/2000
	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[0]))
		{
		if 		(Target.MakesLHSImplyRHS(*ArgArray[0],*ArgArray[1]))
			{
			InvokeEvalForceArg(1);
			return;
			}
		else if (Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[0],*ArgArray[1]))
			{
			AB_ToIFF_AnotB();
			return;
			};
		};
	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[1]))
		{
		if 		(Target.MakesLHSImplyRHS(*ArgArray[1],*ArgArray[0]))
			{
			InvokeEvalForceArg(0);
			return;
			}
		else if (Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[1],*ArgArray[0]))
			{
			AB_ToIFF_AnotB();
			return;
			};
		};
}

void MetaConnective::UseThisAsMakeImply2AryIFF(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	if (   Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[0],*ArgArray[1])
		|| Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[1],*ArgArray[0]))
		// IFF(A,B): boost to NOR(A,B), then DeMorgan
		toNOR();
}

void MetaConnective::UseThisAsMakeImplyNAryAND(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/26/2004
	size_t i = fast_size();
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			bool DidDeletion = false;
			size_t j = fast_size();
			do	if (   i!= --j
					&& Target.MakesLHSImplyRHS(*ArgArray[i],*ArgArray[j]))
					{
					FastDeleteIdx(j);
					if (j<i) --i;
					DidDeletion = true;
					}
			while(0<j);
			j = fast_size();
			do	if (   i!= --j
					&& Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[i],*ArgArray[j]))
					{	// AND(A,B): CONTRADICTION [AND(A,B,OR(~A,~B))|->...|->FALSE]
					InvokeEvalForceFalse();
					return;
					}
			while(0<j);
			if (DidDeletion) return;
			}
	while(0<i);
}

void MetaConnective::UseThisAsMakeImplyNAryOR(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/26/2004
	size_t i = fast_size();
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			bool DidDeletion = false;
			size_t j = fast_size();
			do	if (   i!= --j
					&& Target.MakesLHSImplyRHS(*ArgArray[i],*ArgArray[j]))
					{
					FastDeleteIdx(i);
					if (j<i) --i;
					DidDeletion = true;
					}
			while(0<j);
			if (DidDeletion) return;
			}
	while(0<i);
}

void MetaConnective::UseThisAsMakeImplyNAryIFF(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/29/2000
	size_t i = fast_size();
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			size_t j = fast_size();
			do	if (   i!= --j
					&& Target.MakesLHSImplyLogicalNOTOfRHS(*ArgArray[i],*ArgArray[j]))
					{	// IFF(A,B,...): boost to NOR(A,B,...) [AND(IFF(A,B,...),OR(~A,~B))]
					toNOR();
					return;
					}
			while(0<j);
			}
	while(0<i);
}

void MetaConnective::UseThisAsMakeImplyNAryXOR(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/29/2000
	size_t i = fast_size();
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			size_t j = fast_size();
			do	if (   i!= --j
					&& Target.MakesLHSImplyRHS(*ArgArray[i],*ArgArray[j]))
					{	// A=>B: force ~A AND XOR(...)
					InferenceParameter1 = i;
					TargetVariableFalse();
					return;
					}
			while(0<j);
			}
	while(0<i);
}

void MetaConnective::UseThisAsMakeImplyNAryNXOR(const MetaConcept& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/29/2000
	size_t i = fast_size();
	do	if (Target.ValidLHSForMakesLHSImplyRHS(*ArgArray[--i]))
			{
			size_t j = fast_size();
			do	if (   i!= --j
					&& Target.MakesLHSImplyRHS(*ArgArray[i],*ArgArray[j]))
					{	// A=>B: force A OR NXOR(...)
					InferenceParameter1 = i;
					TargetVariableTrue();
					return;
					}
			while(0<j);
			}
	while(0<i);
}

void MetaConnective::SelfLogicalNOT()
{	// FORMALLY CORRECT: Kenneth Boyd, 2/14/1999
	typedef void (MetaConnective::* SelfLogicalNOTFunc)();

	static const SelfLogicalNOTFunc SelfLogicalNOTAux[]
		= {
		  &MetaConnective::SelfLogicalNOT_ANDOR,
		  &MetaConnective::SelfLogicalNOT_ANDOR,
		  &MetaConnective::SelfLogicalNOT_IFF,
		  &MetaConnective::SelfLogicalNOT_XOR,
		  &MetaConnective::SelfLogicalNOT_Normal,
		  &MetaConnective::SelfLogicalNOT_Normal,
		  &MetaConnective::SelfLogicalNOT_Normal,
		  &MetaConnective::SelfLogicalNOT_Normal
	};
	static_assert(sizeof(SelfLogicalNOTAux)/sizeof(*SelfLogicalNOTAux) == StrictBound_MCM);

	(this->*SelfLogicalNOTAux[array_index()])();
}

void MetaConnective::SelfLogicalNOT_Normal()
{	// FORMALLY CORRECT: 6/4/1999
	SetExactTypeV2((ExactType_MC)(LogicalAND_MC+LogicalNAND_MC-VFTable1->ExactType));
}

void MetaConnective::SelfLogicalNOT_IFF()
{	// FORMALLY CORRECT: 6/22/1999
	if (2==fast_size())
		{
		ArgArray[1]->SelfLogicalNOT();
		ForceTotalLexicalArgOrder();
		}
	else
		SetExactTypeV2(LogicalNIFF_MC);
}

void MetaConnective::SelfLogicalNOT_XOR()
{	// FORMALLY CORRECT: 11/15/1999
	SetExactTypeV2((2==fast_size()) ? LogicalIFF_MC : LogicalNXOR_MC);
}

void MetaConnective::SelfLogicalNOT_ANDOR()
{	// FORMALLY CORRECT: 6/4/1999
	SetNANDNOR((ExactType_MC)(LogicalAND_MC+LogicalNAND_MC-VFTable1->ExactType));
}

bool MetaConnective::InvokeEqualArgRule() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/23/2000
	if (2==fast_size())
		{
		InferenceParameter1 = 0;
		IdxCurrentEvalRule=Idempotent2AryRulesTable[array_index()];
		return true;
		}
	else{
		IdxCurrentSelfEvalRule = IdempotentNArySelfEvalRulesTable[array_index()];
		if 		(SelfEvalRuleCleanArg_SER==IdxCurrentSelfEvalRule)
			return CheckForTrailingCleanArg(AreSyntacticallyEqual,SelfEvalRuleCleanTrailingArg_SER,2);
		else if (SelfEvalRuleNIFFXORCleanArg_SER==IdxCurrentSelfEvalRule)
			return CheckForTrailingCleanArg(AreSyntacticallyEqual,SelfEvalRuleNIFFXORCleanTrailingArg_SER,2);
		return true;
		}
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > MetaConnective::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

void MetaConnective::DiagnoseInferenceRules() const
{
	assert(LogicalNOR_MC>ExactType());
	assert(2<ArgArray.size() || LogicalXOR_MC>ExactType());

	// this will be mode-based; store intended flags to set in VFTable2
	// META: need information from detection rule when using the rule!
	// static variables aren't thread-safe; need two indices
	// 1) T/F/contradiction detection [3 versions]; some of these are instant-evaluation
	//    rules for certain cases
	// next rules want to know about T/F
	if (2==fast_size())
		{
		static const DiagnoseIntermediateRulesFunc diagnoseTVal2ary[] = {
			&MetaConnective::DiagnoseInferenceRulesContradiction2Ary,
			&MetaConnective::DiagnoseInferenceRulesTrue2Ary,
			&MetaConnective::DiagnoseInferenceRulesFalse2Ary,
			&MetaConnective::DiagnoseInferenceRulesUnknown2Ary
		};

Ary2Finish:
		if (ArgArray[0]->IsExactType(TruthValue_MC))
			{
			(this->*diagnoseTVal2ary[static_cast<TruthValue*>(ArgArray[0])->_x.array_index()])();
			return;
			};
		// antiidempotent scanner
		if (IsAntiIdempotentTo(*ArgArray[1],*ArgArray[0]))
			{
			static constexpr const MetaConceptWithArgArray::EvalRuleIdx_ER antiIdempotent2AryRulesTable[] = {
				EvalForceContradiction_ER, // AND
				EvalForceTrue_ER,          // OR
				EvalForceContradiction_ER  // IFF
			};

			LOG("(Using anti-idempotent arguments)");
			LOG(*ArgArray[0]);
			LOG(*ArgArray[1]);
			LOG(*this);
			IdxCurrentEvalRule= antiIdempotent2AryRulesTable[array_index()];
			return;
			};
		static constexpr const DiagnoseIntermediateRulesFunc2 DiagnoseRules2AryStrictlyImpliesAux[] = {
			&MetaConnective::DiagnoseStrictlyImplies2AryAND,
			&MetaConnective::DiagnoseStrictlyImplies2AryOR,
			&MetaConnective::DiagnoseStrictlyImplies2AryIFF
		};

		// A=>B, A=>~B scanners
		if (   DiagnoseStandardEvalRules()
			|| (this->*DiagnoseRules2AryStrictlyImpliesAux[array_index()])())
			return;

		static constexpr const DiagnoseIntermediateRulesFunc DiagnoseRules2AryAux[] = {
			&MetaConnective::DiagnoseIntermediateRulesAND2AryAux,
			&MetaConnective::DiagnoseIntermediateRulesOR2AryAux,
			&MetaConnective::DiagnoseIntermediateRulesIFF2AryAux
		};

		(this->*DiagnoseRules2AryAux[array_index()])();
		return;
		}
	else if (1==fast_size())
		{	// arity 1 -- handle by 
		assert(LogicalXOR_MC>=ExactType());
		InvokeEvalForceArg(0);
		return;
		}
	else if (ArgArray[0]->IsExactType(TruthValue_MC))
		{	// XXX fix \todo this architecture is hard to maintain...WouldDiagnose family should be returning member function pointers instead
		static constexpr const DiagnoseIntermediateRulesFunc2 DiagnoseRulesTValNAryAux[] = {
			&MetaConnective::DiagnoseInferenceRulesContradictionNAry,
			&MetaConnective::DiagnoseInferenceRulesTrueNAry,
			&MetaConnective::DiagnoseInferenceRulesFalseNAry,
			&MetaConnective::DiagnoseInferenceRulesUnknownNAry
		};

		static constexpr const DiagnoseIntermediateRulesFunc2 WouldDiagnoseRulesTValNAryAux[] = {
			&MetaConnective::WouldDiagnoseInferenceRulesContradictionNAry,
			&MetaConnective::WouldDiagnoseInferenceRulesTrueNAry,
			&MetaConnective::WouldDiagnoseInferenceRulesFalseNAry,
			&MetaConnective::WouldDiagnoseInferenceRulesUnknownNAry
		};

retryNAryTVal:
		if ((this->*DiagnoseRulesTValNAryAux[static_cast<TruthValue*>(ArgArray[0])->_x.array_index()])()) return;
		if (2 == fast_size()) goto Ary2Finish;
		if (ArgArray[0]->IsExactType(TruthValue_MC) && (this->*WouldDiagnoseRulesTValNAryAux[static_cast<TruthValue*>(ArgArray[0])->_x.array_index()])()) goto retryNAryTVal;
		};
		// Internal VFT
	// consider : ==, eval, Associative options
	// == : cleanup rules
	// Associative: evaluate to prevent issues with pattern matching on AND, OR
	// == goes before associative to prevent unnecessary associativity expansions.
	// AntiIdempotent goes before associative to prevent unnecessary associativity expansions
	// should expand associativity *before* =>-checks.
	// eval : indefinite (may want to defer this)
	if (DiagnoseEqualArgs()) return;
	// Above call can, for efficiency reasons, drop the arity down to 2...at which point
	// the following code will legitimately crash
	if (2==fast_size()) goto Ary2Finish;

	// 4) AntiIdempotence detection [A,~A][1 version]; several of these are instant-evaluation
	//    constructor will be interested in this one
	if (FindTwoAntiIdempotentArgsSymmetric())
		{
		static constexpr const unsigned short AntiIdempotentNAryEvalRulesTable[] = {
			EvalForceContradiction_ER, // AND
			EvalForceTrue_ER, // OR
			EvalForceContradiction_ER, // IFF
			None_ER, // XOR
			None_ER, // NXOR
			EvalForceTrue_ER // NIFF
		};

		static constexpr const signed short AntiIdempotentNArySelfEvalRulesTable[] = {
		  None_SER, // AND
		  None_SER, // OR
		  None_SER, // IFF
		  ReplaceArgsWithTrue_SER, // XOR
		  ReplaceArgsWithTrue_SER, // NXOR
		  None_SER // NIFF
		};

		LOG("(Using anti-idempotent arguments)");
		LOG(*ArgArray[InferenceParameter1]);
		LOG(*ArgArray[InferenceParameter2]);
		const size_t Idx = array_index();
		IdxCurrentEvalRule = AntiIdempotentNAryEvalRulesTable[Idx];
		IdxCurrentSelfEvalRule = AntiIdempotentNArySelfEvalRulesTable[Idx];
		if (IdxCurrentSelfEvalRule && 3==fast_size())
			{	// XOR or NXOR.  Missed arg of XOR is false; missed arg of NXOR is true.
			IdxCurrentEvalRule = (IsExactType(LogicalXOR_MC)) ? EvalForceNotArg_ER : EvalForceArg_ER;
			IdxCurrentSelfEvalRule = None_SER;
			// 0,1 |-> 2
			// 0,2 |-> 1
			// 1,2 |-> 0;
			InferenceParameter1 = 3-(InferenceParameter1+InferenceParameter2);
			}
		return;
		};
	if (DiagnoseSelfAssociativeArgs()) return;
	// NIFF(A,B,...): NO EFFECT
	// NOTE: swapped evaluation and StrictlyImplies heuristics to protect EqualRelation rules
	// if an argument can evaluate, it will; also covers generalized associativity
	if (DiagnoseEvaluatableArgs()) return;

	// recursed, so retry (QualInv2Form5Alt1.txt is having problems)
	if (DiagnoseEqualArgs()) return;
	// Above call can, for efficiency reasons, drop the arity down to 2...at which point
	// the following code will legitimately crash
	if (2==fast_size()) goto Ary2Finish;

	static constexpr const DiagnoseIntermediateRulesFunc2 DiagnoseRulesNAryStrictlyImpliesAux[] = {
		  &MetaConnective::DiagnoseStrictlyImpliesNAryAND,
		  &MetaConnective::DiagnoseStrictlyImpliesNAryOR,
		  &MetaConnective::DiagnoseStrictlyImpliesNAryIFF,
		  &MetaConnective::DiagnoseStrictlyImpliesNAryXOR,
		  &MetaConnective::DiagnoseStrictlyImpliesNAryNXOR
	};

	static constexpr const DiagnoseIntermediateRulesFunc DiagnoseRulesAux[] = {
		&MetaConnective::DiagnoseIntermediateRulesANDAux,
		&MetaConnective::DiagnoseIntermediateRulesORAux,
		&MetaConnective::DiagnoseIntermediateRulesIFFAux,
		&MetaConnective::DiagnoseIntermediateRulesXORAux,
		&MetaConnective::DiagnoseIntermediateRulesNXORAux,
		&MetaConnective::DiagnoseIntermediateRulesNIFFAux
	};

	if (   !IsExactType(LogicalNIFF_MC)
	    && (this->*DiagnoseRulesNAryStrictlyImpliesAux[array_index()])())
		return;
	// FORMALLY CORRECT: Kenneth Boyd, 3/2/1999
	(this->*DiagnoseRulesAux[array_index()])();
}

void MetaConnective::DiagnoseInferenceRulesContradiction2Ary() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2000
	//! \pre Arg0 is CONTRADICTION
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::Contradiction));
	if (IsExactType(LogicalOR_MC))
		{
		InferenceParameter1 = 1;
		IdxCurrentEvalRule=truthValueFalseAry2Table[LogicalOR_MC-LogicalAND_MC];
		return;
		};
	InvokeEvalForceArg(0);
}

void MetaConnective::DiagnoseInferenceRulesTrue2Ary() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2000
	//! \pre Arg0 is TRUE
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::True));
	InvokeEvalForceArg(1!=array_index());
}

void MetaConnective::DiagnoseInferenceRulesFalse2Ary() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2000
	//! \pre Arg0 is FALSE
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::False));
	InferenceParameter1 = 1;
	IdxCurrentEvalRule=truthValueFalseAry2Table[array_index()];
}

void MetaConnective::DiagnoseInferenceRulesUnknown2Ary() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2000
	//! \pre Arg0 is UNKNOWN 
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::Unknown));
	if (ArgArray[1]->IsExactType(TruthValue_MC))
		{	// it's sorted...so it's Unknown
		InvokeEvalForceArg(0);
		return;
		};
	DiagnoseStandardEvalRules();
}

bool MetaConnective::WouldDiagnoseInferenceRulesContradictionNAry() const
{
	if ("\x00\x01\x00\x01\x01"[array_index()])	// sixth entry is implicitly \x00
		return WouldDiagnoseInferenceRulesFalseNAry();
	return true;
}

bool MetaConnective::DiagnoseInferenceRulesContradictionNAry() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2000
	//! \pre Arg0 is CONTRADICTION
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::Contradiction));
	// this invokes the false rules for Logical: OR, XOR, NXOR.
	// otherwise, forces contradiction.

	if ("\x00\x01\x00\x01\x01"[array_index()])	// sixth entry is implicitly \x00
		return DiagnoseInferenceRulesFalseNAry();
	InvokeEvalForceArg(0);
	return true;
}

bool MetaConnective::WouldDiagnoseInferenceRulesTrueNAry() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/20/2000
	//! \pre Arg0 is TRUE
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::True));
	if (IsExactType(LogicalOR_MC)) return true;
	else if (IsExactType(LogicalAND_MC)) return true;
	else if (IsExactType(LogicalIFF_MC)) return true;
	else if (IsExactType(LogicalXOR_MC))
	{
		if (ArgArray[1]->IsExactType(TruthValue_MC))
		{
			if (static_cast<TruthValue*>(ArgArray[1])->_x.is(true)) return true;
			else if (static_cast<TruthValue*>(ArgArray[1])->_x.is(false))
			{
				if (ArgArray[fast_size() - 1]->IsExactType(TruthValue_MC)) return true;
				else if (ArgArray[fast_size() - 2]->IsExactType(TruthValue_MC))
				{
					if (static_cast<TruthValue*>(ArgArray[fast_size() - 2])->_x.is(false)) return true;
					// otherwise, next-to-last arg is UNKNOWN
				};
				return false;
			}
			else {	// second arg is UNKNOWN
				if (ArgArray[fast_size() - 1]->IsExactType(TruthValue_MC)) return true;
				return false;
			}
		};
		return false;
	}
	else if (IsExactType(LogicalNXOR_MC)) return true;
	return false;
}

bool MetaConnective::DiagnoseInferenceRulesTrueNAry() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/20/2000
	//! \pre Arg0 is TRUE
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::True));
	if 		(IsExactType(LogicalOR_MC))
		{
		InferenceParameter1 = 2;
		const_cast<MetaConnective*>(this)->SelfEvalRuleCleanTrailingArg();		
		InvokeEvalForceArg(0);
		return true;
		}
	else if (IsExactType(LogicalAND_MC))
		{
		if (ArgArray[1]->IsExactType(TruthValue_MC))
			{
			if 		(static_cast<TruthValue*>(ArgArray[1])->_x.is(true))
				{
				size_t i = 1;
				while(   fast_size()-1>i
					  && ArgArray[i+1]->IsExactType(TruthValue_MC)
					  && static_cast<TruthValue*>(ArgArray[i+1])->_x.is(true)) ++i;
				if (fast_size()-2<=i)
					{	// all but the last arg is TRUE; the last one is unspecified.
					InvokeEvalForceArg(fast_size()-1);
					return true;
					};

				if (   ArgArray[i+1]->IsExactType(TruthValue_MC)
					&& static_cast<TruthValue*>(ArgArray[i+1])->_x.is(false))
					{	// FALSE: trip off set-to-FALSE rule
					InvokeEvalForceArg(i+1);
					return true;
					};

				// SelfEvalRuleCleanLeadingArg clears a block of leading TRUE.  *TRIVIAL*
				InferenceParameter1 = i+1;
				const_cast<MetaConnective*>(this)->SelfEvalRuleCleanLeadingArg();
				return false;
				}
			else if (static_cast<TruthValue*>(ArgArray[1])->_x.is(false))
				{	// FALSE: trip off set-to-FALSE rule
				InvokeEvalForceArg(1);
				return true;
				};
			};
		// would clear a leading TRUE.  *TRIVIAL*.
		const_cast<MetaConnective*>(this)->FastDeleteIdx(0);
		return false;
		};
	IdxCurrentSelfEvalRule=TruthValueTrueAryNSelfTable[array_index()];
	InferenceParameter1 = 0;	// all the rules want this
	if 		(IsExactType(LogicalIFF_MC))
		{
		InferenceParameter2 = LogicalAND_MC;
		return true;
		}
	else if (IsExactType(LogicalXOR_MC))
		{
		if (ArgArray[1]->IsExactType(TruthValue_MC))
			{
			if		(static_cast<TruthValue*>(ArgArray[1])->_x.is(true))
				{	// XOR(TRUE,TRUE,...) |-> FALSE
				IdxCurrentSelfEvalRule = None_SER;
				InferenceParameter1 = 0;
				IdxCurrentEvalRule=EvalForceNotArg_ER;
				return true;			
				}
			else if (static_cast<TruthValue*>(ArgArray[1])->_x.is(false))
				{
				if 		(ArgArray[fast_size()-1]->IsExactType(TruthValue_MC))
					{
					IdxCurrentSelfEvalRule = None_SER;
					InvokeEvalForceArg(static_cast<TruthValue*>(ArgArray[fast_size()-1])->_x.is(false) ? 0 : fast_size()-1);
					return true;
					}
				else if (ArgArray[fast_size()-2]->IsExactType(TruthValue_MC))
					{
					if (static_cast<TruthValue*>(ArgArray[fast_size()-2])->_x.is(false))
						{
						IdxCurrentSelfEvalRule = None_SER;
						IdxCurrentEvalRule = EvalForceNotArg_ER;
						InferenceParameter1 = fast_size()-1;
						};
					// otherwise, next-to-last arg is UNKNOWN
					};
				return true;
				}
			else{	// second arg is UNKNOWN
				if (ArgArray[fast_size()-1]->IsExactType(TruthValue_MC))
					// last arg is TruthValue, thus UNKNOWN
					InvokeEvalForceArg(fast_size()-1);
				return true;			
				}
			};
		return true;
		}
	//! \todo dualize LogicalXOR code to LogicalNXOR
	else if (IsExactType(LogicalNXOR_MC))
		InferenceParameter2 = LogicalOR_MC;
	return true;
}

bool MetaConnective::WouldDiagnoseInferenceRulesFalseNAry() const
{
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(!static_cast<TruthValue*>(ArgArray[0])->_x.could_be(true));
	return true;
}

bool MetaConnective::DiagnoseInferenceRulesFalseNAry() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/3/2000
	//! \pre Arg0 is FALSE or CONTRADICTION (latter is recursion)
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(!static_cast<TruthValue*>(ArgArray[0])->_x.could_be(true));
	if 		(IsExactType(LogicalAND_MC))
		{
		InferenceParameter1 = 2;
		const_cast<MetaConnective*>(this)->SelfEvalRuleCleanTrailingArg();		
		InvokeEvalForceArg(0);
		return true;
		}
	else if (IsExactType(LogicalOR_MC))
		{
		if (   ArgArray[1]->IsExactType(TruthValue_MC)
			&& !static_cast<TruthValue*>(ArgArray[1])->_x.could_be(true))
			{
			size_t i = 1;
			while(   fast_size()-1>i
				  && ArgArray[i+1]->IsExactType(TruthValue_MC)
				  && !static_cast<TruthValue*>(ArgArray[i+1])->_x.could_be(true)) ++i;
			if (fast_size()-2<=i)
				{	// all but the last arg is FALSE; the last one is unspecified.
				InvokeEvalForceArg(fast_size()-1);
				return true;
				};
			// deleting leading block of FALSE.  *TRIVIAL*
			InferenceParameter1 = i+1;
			const_cast<MetaConnective*>(this)->SelfEvalRuleCleanLeadingArg();
			return false;
			};
		// deleting leading FALSE.  *TRIVIAL*
		const_cast<MetaConnective*>(this)->FastDeleteIdx(0);
		return false;
		};
	IdxCurrentSelfEvalRule=TruthValueFalseAryNSelfTable[ExactType()-LogicalOR_MC];
	InferenceParameter1 = 0;	// all the rules want this
	InferenceParameter2 = LogicalIFF_MC;	// NXOR version wants this
	//! \todo (?) IMPLEMENT vector delete: IFF, NXOR/NIFF versions
	// OR is already implemented
	// NOTE: only LogicalOR_MC sets up SelfEvalRuleCleanArg_SER==IdxCurrentSelfEvalRule
	if (IsExactType(LogicalNIFF_MC)) InferenceParameter2 = LogicalOR_MC;
	return true;
}

bool MetaConnective::WouldDiagnoseInferenceRulesUnknownNAry() const
{
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::Unknown));
	if (ArgArray[1]->IsExactType(TruthValue_MC)) return true;
	return false;
}

bool MetaConnective::DiagnoseInferenceRulesUnknownNAry() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/19/2000
	//! \pre Arg0 is UNKNOWN
	assert(ArgArray[0]->IsExactType(TruthValue_MC));
	assert(static_cast<TruthValue*>(ArgArray[0])->_x.is(TVal::Unknown));
	if (ArgArray[1]->IsExactType(TruthValue_MC))
		{	// sorted, thus Unknown
		// Quick unknown mergence: args 0, 1 both TruthValue Unknown => merge
		do	{
			const_cast<MetaConnective*>(this)->FastDeleteIdx(1);
			if (2==fast_size())
				{	// XOR,NIFF |-> IFF (~2nd arg)
					// NXOR |-> IFF
				if (IsExactType(LogicalNXOR_MC))
					{
					const_cast<MetaConnective*>(this)->SetExactTypeV2(LogicalIFF_MC);
					return false;
					}
				else if (LogicalXOR_MC<=ExactType())
					{	// Exact type is XOR or NIFF
					ArgArray[1]->SelfLogicalNOT();
					const_cast<MetaConnective*>(this)->SetExactTypeV2(LogicalIFF_MC);
					return false;
					};
				return false;
				}
			}
		while(ArgArray[1]->IsExactType(TruthValue_MC));
		};
	return false;
}

bool MetaConnective::DiagnoseStrictlyImplies2AryAND() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/7/2000
	// A=>B scanner
	//! \todo IMPLEMENT: mirror augmentations for DiagnoseStrictlyImpliesNAryAND
	if (ArgArray[0]->StrictlyImplies(*ArgArray[1]))
		{	// table
		// AND(A,B): ForceArg 0 [redundancy control: AND(A,B,OR(~A,B))|->AND(A,B,TRUE)|->AND(A,B)]
		LOG(msz_Using);
		LOG(*ArgArray[0]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[1]);
		InvokeEvalForceArg(0);
		return true;
		};
	if (ArgArray[1]->StrictlyImplies(*ArgArray[0]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[1]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[0]);
		// AND(A,B): ForceArg 1
		InvokeEvalForceArg(1);
		return true;
		};
	// A=>~B scanner
	if (StrictlyImpliesLogicalNOTOf(*ArgArray[0],*ArgArray[1]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[0]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[1]);
		// AND(A,B): CONTRADICTION [AND(A,B,OR(~A,~B))|->...|->FALSE]
		InvokeEvalForceContradiction();
		return true;
		};
	if (StrictlyImpliesLogicalNOTOf(*ArgArray[1],*ArgArray[0]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[1]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[0]);
		// AND(A,B): CONTRADICTION [AND(A,B,OR(~A,~B))|->...|->FALSE]
		InvokeEvalForceContradiction();
		return true;
		};
	return false;
}

bool MetaConnective::DiagnoseStrictlyImplies2AryOR() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/7/2000
	// A=>B scanner
	//! \todo IMPLEMENT: mirror augmentations for DiagnoseStrictlyImpliesNAryOR
	if (ArgArray[0]->StrictlyImplies(*ArgArray[1]))
		{	// table
		// OR(A,B): ForceArg 1 [AND(OR(A,B),OR(~A,B))|->OR(AND(A,~A),B)|->OR(CONTRADICTION,B)|->B]
		LOG(msz_Using);
		LOG(*ArgArray[0]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[1]);
		InvokeEvalForceArg(1);
		return true;
		};
	if (ArgArray[1]->StrictlyImplies(*ArgArray[0]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[1]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[0]);
		// OR(A,B): ForceArg 0
		InvokeEvalForceArg(0);
		return true;
		};
#if 0
	// A=>~B scanner
	// A StrictlyImpliesLogicalNOTOf B
	// generally, this means OR(~A,AND(A,B))
	// correct response is to rewrite as OR(~A,B)
	// e.g.: OR(~A,AND(A,B)) |-> OR(~A,AND(TRUE,B)) |-> OR(~A,B)
	// e.g.: OR(ALLEQUAL(A,B),ALLDISTINCT(A,B,C)) |-> OR(ALLEQUAL(A,B),(C DISTINCTFROMALLOF A,B))
	//! \todo fix code, below
	if (StrictlyImpliesLogicalNOTOf(*ArgArray[0],*ArgArray[1]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[0]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[1]);
		// OR(A,B): boost to IFF(A,~B) // SelfEval rule
		//! \todo EXCEPTION: OR(~A,AND(A,B)): this maps to OR(~A,B)
		IdxCurrentSelfEvalRule = AB_ToIFF_AnotB_SER;
		return true;
		};
	if (StrictlyImpliesLogicalNOTOf(*ArgArray[1],*ArgArray[0]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[1]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[0]);
		// OR(A,B): boost to IFF(A,~B) // SelfEval rule
		//! \todo EXCEPTION: OR(~A,AND(A,B)): this maps to OR(~A,B)
		IdxCurrentSelfEvalRule = AB_ToIFF_AnotB_SER;
		return true;
		};
#endif
	return false;
}


//! \todo Analog rule for EqualRelation:
//! <br>IFF(A==B,(A EQUALTOONEOF B,C)) |-> OR(A==B,A!=C)	[generalizes]
//! <br>IFF(A==B,NOTALLDISTINCT(A,B,C)) |-> OR(A==B,(A DISTINCTFROMALLOF B,C)) [clunky generalization]
//! <br>IFF(A!=B,NOTALLEQUAL(A,B,C) |-> OR(A!=B,A==B==C) [generalizes]
//! <br>General: IFF(A,OR(A,B)) |-> OR(A,[Action of ~A on AND(~A,~B)])
//! <br>dual: IFF(A,AND(A,B)) |-> OR(~A,[Action of A on AND(A,B)])
static bool CanGetORFromIFF(const MetaConcept& lhs, const MetaConcept& rhs)
{
//! IFF(A,AND(A,B)) |-> ... |-> OR(~A,B)
//! <br>Alternate: AND(A,B)=>A |-> ... |-> OR(~A,B)
//! <br>IFF(A,OR(A,B)) |-> ... |-> OR(A,~B)
//! <br>ALTERNATE: A=>OR(A,B) |-> ... |-> OR(A,~B)
//! <br>Rephrase these for DiagnoseStrictlyImplies
//! <br>AND(A,B)=>A; other direction is A=>AND(A,B) i.e OR(~A,AND(A,B))
//! <br>A=>OR(A,B); other direction is OR(A,B)=>A i.e. OR(AND(~A,~B),A)

//! <p>IFF(A,OR(A,B)) |-> OR(A,~B)
	if (   (rhs.IsExactType(LogicalOR_MC) || rhs.IsExactType(LogicalAND_MC))
	    && static_cast<const MetaConnective&>(rhs).FindArgRelatedToLHS(lhs,AreSyntacticallyEqual))
		return true;
	return false;
}

static bool CanGetORFromIFFV2(const MetaConcept& lhs, const MetaConcept& rhs)
{   //! Alt. form: IFF(AND(A,B),IFF(A,B)) |-> OR(AND(A,B),OR(A,B)) |-> OR(A,B)
	if (   rhs.IsExactType(LogicalIFF_MC)
        && lhs.IsExactType(LogicalAND_MC)
    	&& 2==rhs.size()
    	&& 2==lhs.size()
	    && static_cast<const MetaConnective&>(lhs).ExactOrderPairwiseRelation(static_cast<const MetaConnective&>(rhs),AreSyntacticallyEqual))
		return true;
	return false;
}

static bool CanGetORFromIFFV3(const MetaConcept& lhs, const MetaConcept& rhs)
{   //! Checks for subvector arglists of AND, OR
	if (   (rhs.IsExactType(LogicalAND_MC) || rhs.IsExactType(LogicalOR_MC))
        &&  lhs.IsExactType(rhs.ExactType())
	    && static_cast<const MetaConnective&>(lhs).SubvectorArgList(static_cast<const MetaConnective&>(rhs)))
		return true;
	return false;
}

// IFF(A,OR(~A,B)) |-> ... |-> AND(A,B)

// IFF(A,AND(~A,B)) |-> ... |-> AND(~A,~B)
bool MetaConnective::DiagnoseStrictlyImplies2AryIFF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/7/2000
	// A=>~B scanner
	if (StrictlyImpliesLogicalNOTOf(*ArgArray[0],*ArgArray[1]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[0]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[1]);

		// IFF(A,B): boost to NOR(A,B), then DeMorgan
		LOG("to rewrite this IFF as a NOR");
		LOG(*this);
		IdxCurrentSelfEvalRule = toNOR_SER;
		return true;
		};
	if (StrictlyImpliesLogicalNOTOf(*ArgArray[1],*ArgArray[0]))
		{
		LOG(msz_Using);
		LOG(*ArgArray[1]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[0]);

		// IFF(A,B): boost to NOR(A,B), then DeMorgan
		LOG("to rewrite this IFF as a NOR");
		LOG(*this);
		IdxCurrentSelfEvalRule = toNOR_SER;
		return true;
		};
	// ~A=>B scanner
	if (LogicalNOTOfStrictlyImplies(*ArgArray[0],*ArgArray[1]))
		{
		LOG(msz_UsingNOT);
		LOG(*ArgArray[0]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[1]);

		// IFF(A,B): boost to AND(A,B)
		LOG("to rewrite this IFF as an AND");
		LOG(*this);
		InferenceParameter1 = LogicalAND_MC;
		IdxCurrentSelfEvalRule = CompatibleRetype_SER;
		return true;
		};
	if (LogicalNOTOfStrictlyImplies(*ArgArray[1],*ArgArray[0]))
		{
		LOG(msz_UsingNOT);
		LOG(*ArgArray[1]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[0]);

		// IFF(A,B): boost to AND(A,B)
		LOG("to rewrite this IFF as an AND");
		LOG(*this);
		InferenceParameter1 = LogicalAND_MC;
		IdxCurrentSelfEvalRule = CompatibleRetype_SER;
		return true;
		};
	// IFF(A,OR(A,B)) |-> OR(A,~B)
	// General: IFF(A,OR(A,B)) |-> OR(A,[Action of ~A on AND(~A,~B)]
	// New LowRel entry for this function
	//! \todo: shove CanGetORFromIFF, CanGetORFromIFFV2, CanGetORFromIFFV3 into a "psuedoCanStrictlyModify" function specific to
	//! MetaConnective, of reverse-handedness.  The performance hit from moving this into MetaConcept's CanStrictlyModify is too much.
	if (CanGetORFromIFF(*ArgArray[0],*ArgArray[1]))
		{	// IFF(A,OR(A,B)) |-> OR(A,~B)
		InferenceParameter1 = 0;
		InferenceParameter2 = 1;
		IdxCurrentSelfEvalRule = Ary2IFFToOR_SER;		
		return true;
		};
	if (CanGetORFromIFF(*ArgArray[1],*ArgArray[0]))
		{
		InferenceParameter1 = 1;
		InferenceParameter2 = 0;
		IdxCurrentSelfEvalRule = Ary2IFFToOR_SER;		
		return true;
		};
	if (CanGetORFromIFFV2(*ArgArray[0],*ArgArray[1]))
		{	// IFF(AND(A,B),IFF(A,B)) |-> OR(A,B)
		InferenceParameter1 = 0;
		InferenceParameter2 = 1;
		IdxCurrentSelfEvalRule = Ary2IFFToORV2_SER;		
		return true;
		};
	if (CanGetORFromIFFV2(*ArgArray[1],*ArgArray[0]))
		{
		InferenceParameter1 = 1;
		InferenceParameter2 = 0;
		IdxCurrentSelfEvalRule = Ary2IFFToORV2_SER;		
		return true;
		};
	if (CanGetORFromIFFV3(*ArgArray[0],*ArgArray[1]))
		{	// IFF(AND(A,B),AND(A,B,C)) |-> OR(~A,~B,C)
		InferenceParameter1 = 0;
		InferenceParameter2 = 1;
		IdxCurrentSelfEvalRule = Ary2IFFToORV3_SER;		
		return true;
		};

	//! \todo metacode: IFF(A,B), A=>B, ~A can strictly modify ~B: remap to OR(A,~B strictly modified by ~A).
	//! note that forcing AND args TRUE, OR args false is a strictly-modify operation....
	// E.g., above handles analog rules for EqualRelation:
	// IFF(A==B,(A EQUALTOONEOF B,C)) |-> OR(A==B,A!=C)	[generalizes]
	// IFF(A==B,NOTALLDISTINCT(A,B,C)) |-> OR(A==B,(A DISTINCTFROMALLOF B,C)) [clunky generalization]
	// IFF(A!=B,NOTALLEQUAL(A,B,C) |-> OR(A!=B,A==B==C) [generalizes]
	if 		(ArgArray[0]->StrictlyImplies(*ArgArray[1]))
		{
		ArgArray[0]->SelfLogicalNOT();
		ArgArray[1]->SelfLogicalNOT();
		if (ArgArray[0]->CanStrictlyModify(*ArgArray[1]))
			{
			ArgArray[0]->SelfLogicalNOT();
			ArgArray[1]->SelfLogicalNOT();
			InferenceParameter1 = 0;
			InferenceParameter2 = 1;
			IdxCurrentSelfEvalRule = Ary2IFFToORSelfModify_SER;
			return true;
			}
		ArgArray[0]->SelfLogicalNOT();
		ArgArray[1]->SelfLogicalNOT();
		}
	else if (ArgArray[1]->StrictlyImplies(*ArgArray[0]))
		{
		ArgArray[0]->SelfLogicalNOT();
		ArgArray[1]->SelfLogicalNOT();
		if (ArgArray[1]->CanStrictlyModify(*ArgArray[0]))
			{
			ArgArray[0]->SelfLogicalNOT();
			ArgArray[1]->SelfLogicalNOT();
			InferenceParameter1 = 0;
			InferenceParameter2 = 1;
			IdxCurrentSelfEvalRule = Ary2IFFToORSelfModify_SER;
			return true;
			}
		ArgArray[0]->SelfLogicalNOT();
		ArgArray[1]->SelfLogicalNOT();
		}
	return false;
}

bool MetaConnective::DiagnoseStrictlyImpliesNAryAND() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/17/2001
	// A=>B scanner
	// IFF(A,B,...): NO EFFECT
	// change the body of this clause into a rule [that iterates itself]
	if (DualFindTwoRelatedArgs(::StrictlyImplies) || FindTwoRelatedArgs(::StrictlyImplies))
		{
		if (LogicalANDTargetArgCritical(InferenceParameter2))
			// LogicalANDTargetCritical outright changes the applicable evaluation rule
			return true;
		IdxCurrentSelfEvalRule = LogicalANDStrictlyImpliesClean_SER;
		return true;
		};
	// A=>~B scanner
	// OR(A,B,...): NO EFFECT
	// XOR(A,B,...): NO EFFECT
	// NXOR(A,B,...): NO EFFECT
	if (FindTwoRelatedArgs(::StrictlyImpliesLogicalNOTOf)|| DualFindTwoRelatedArgs(::StrictlyImpliesLogicalNOTOf))
		{
		LOG(msz_Using);
		LOG(*ArgArray[InferenceParameter1]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[InferenceParameter2]);
		// AND(A,B,...): CONTRADICTION
		InvokeEvalForceContradiction();
		return true;
		};
	return false;
}

bool MetaConnective::DiagnoseStrictlyImpliesNAryOR() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/23/2000
	// A=>B scanner
	// IFF(A,B,...): NO EFFECT
	//! \todo IMPLEMENT: mirror augmentations for DiagnoseStrictlyImpliesNAryAND
	if (DualFindTwoRelatedArgs(::StrictlyImplies) || FindTwoRelatedArgs(::StrictlyImplies))
		{
		IdxCurrentSelfEvalRule = LogicalORStrictlyImpliesClean_SER;
		return true;
		};
	// A=>~B scanner
	// OR(A,B,...): NO EFFECT
	// XOR(A,B,...): NO EFFECT
	// NXOR(A,B,...): NO EFFECT
	return false;
}

bool MetaConnective::DiagnoseStrictlyImpliesNAryIFF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2000
	// A=>B scanner
	// IFF(A,B,...): NO EFFECT without internal structure
	// IFF(A,OR(A,B)) |-> OR(A,~B)
	// A=>~B scanner
	// OR(A,B,...): NO EFFECT
	// XOR(A,B,...): NO EFFECT
	// NXOR(A,B,...): NO EFFECT
	if (FindTwoRelatedArgs(StrictlyImpliesLogicalNOTOf)|| DualFindTwoRelatedArgs(StrictlyImpliesLogicalNOTOf))
		{
		LOG(msz_Using);
		LOG(*ArgArray[InferenceParameter1]);
		LOG(msz_SyntaxImpliesNOT);
		LOG(*ArgArray[InferenceParameter2]);

		// IFF(A,B,...): boost to NOR(A,B,...) [AND(IFF(A,B,...),OR(~A,~B))]
		LOG("to rewrite this IFF as a NOR");
		LOG(*this);
		IdxCurrentSelfEvalRule = toNOR_SER;
		return true;
		};
	if (FindTwoRelatedArgs(LogicalNOTOfStrictlyImplies)|| DualFindTwoRelatedArgs(LogicalNOTOfStrictlyImplies))
		{
		LOG(msz_UsingNOT);
		LOG(*ArgArray[InferenceParameter1]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[InferenceParameter2]);

		// IFF(A,B,...): boost to NOR(A,B,...) [AND(IFF(A,B,...),OR(~A,~B))]
		LOG("to rewrite this IFF as an AND");
		LOG(*this);
		InferenceParameter1 = LogicalAND_MC;
		IdxCurrentSelfEvalRule = CompatibleRetype_SER;
		return true;
		};
	if (FindTwoRelatedArgs(CanGetORFromIFF) || DualFindTwoRelatedArgs(CanGetORFromIFF))
		{
		IdxCurrentSelfEvalRule = IFFSpawn2AryIFF_SER;
		return true;
		};
	if (FindTwoRelatedArgs(CanGetORFromIFFV2) || DualFindTwoRelatedArgs(CanGetORFromIFFV2))
		{
		IdxCurrentSelfEvalRule = IFFSpawn2AryIFF_SER;
		return true;
		};
	//! \todo OPTIMIZE: CanGetORFromIFFV3 relies on Subvector, so the sort order does matter
	// CanGetORFromIFFV3 always returns false when its first arg is lexically later than 
	// its second arg: only DualFindTwoRelatedArgs is necessary.
	if (DualFindTwoRelatedArgs(CanGetORFromIFFV3))
		{
		IdxCurrentSelfEvalRule = IFFSpawn2AryIFF_SER;
		return true;
		};
	return false;
}

bool MetaConnective::DiagnoseStrictlyImpliesNAryXOR() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2000
	// A=>B scanner
	// IFF(A,B,...): NO EFFECT
	if (DualFindTwoRelatedArgs(::StrictlyImplies) || FindTwoRelatedArgs(::StrictlyImplies))
		{
		// XOR(A,AND(A,B,..),AND(A,C,...),...) |-> AND(A,XOR(TRUE,AND(A,B,...),AND(A,C,...),...))
		// we already know that ArgN(InferenceParameter1) StrictlyImplies ArgN(InferenceParameter2)
		/// ...
		if (	InferenceParameter1+1==InferenceParameter2			// dual went off, no known implication failures
			||	(1==InferenceParameter1 && 0==InferenceParameter2))	// primary went off, no known implication failures
			{
			size_t SweepIdx = fast_size();
			do	if (   InferenceParameter1!=--SweepIdx
					&& InferenceParameter2!=SweepIdx
					&& !ArgArray[SweepIdx]->StrictlyImplies(*ArgArray[InferenceParameter2]))
					goto NormalXORImpliesHandler;
			while(0<SweepIdx);

			InferenceParameter1 = InferenceParameter2;
			IdxCurrentSelfEvalRule = ExtractTrueArgFromXOR_SER;
			return true;
			}
NormalXORImpliesHandler:
		LOG(msz_Using);
		LOG(*ArgArray[InferenceParameter1]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[InferenceParameter2]);
		// XOR(A,B,...): Force ~A; remove A from XOR: AND(~A,XOR(B,...))
		IdxCurrentSelfEvalRule = TargetVariableFalse_SER;
		return true;
		};
	// A=>~B scanner
	// OR(A,B,...): NO EFFECT
	// XOR(A,B,...): NO EFFECT
	// NXOR(A,B,...): NO EFFECT
	return false;
}

bool MetaConnective::DiagnoseStrictlyImpliesNAryNXOR() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2000
	// A=>B scanner
	// IFF(A,B,...): NO EFFECT
	if (DualFindTwoRelatedArgs(::StrictlyImplies) || FindTwoRelatedArgs(::StrictlyImplies))
		{
		LOG(msz_Using);
		LOG(*ArgArray[InferenceParameter1]);
		LOG(msz_SyntaxImplies);
		LOG(*ArgArray[InferenceParameter2]);

		// NXOR(A,B,...): rewrite to NAND(~A,XOR(B,...)):=OR(A,NXOR(B,...))
		IdxCurrentSelfEvalRule = TargetVariableTrue_SER;
		return true;
		};
	// A=>~B scanner
	// OR(A,B,...): NO EFFECT
	// XOR(A,B,...): NO EFFECT
	// NXOR(A,B,...): NO EFFECT
	return false;
}

//! \todo most of these rules have versions where implications generalize.
//! However, these implications may be complicated, and require justification in their own right.
//! The recursive version will be called SyntaticallyImplies, and will have a trace function
//! TraceSyntacticallyImplies.

bool MetaConnective::LogicalANDNonTrivialFindDetailedRule() const
{return NULL!=ANDDetailedRuleAux[array_index()];}

// Top-level for IFF-driven dynamically implies
// implemented deep versions: AND, OR
bool MetaConnective::LogicalANDFindDetailedRule(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx)
{	// FORMALLY CORRECT: 1/20/2000
	// NOTE: AND is caught by generalized associativity prior to the pattern hunt.
	return (this->*ANDDetailedRuleAux[array_index()])(rhs,LHSIdx,RHSIdx,Param1,Param2,SelfEvalIdx,EvalIdx);
}

bool MetaConnective::LogicalANDFindDetailedRuleForOR(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/20/2000
	// Generally, disassembling this rule is counterproductive.
	if		(rhs.IsExactType(LogicalOR_MC))
		{
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);
		// OR(A,B,C), OR(~A,~B,~C): either strictlymodifies other to NIFF(A,B,C)
		//! \todo OR(A,B,C), OR(~A,AND(~B,~C)): longer strictlymodifies shorter to NIFF
		switch(cmp(fast_size(),VR_rhs.fast_size()))
		{
		case -1:
			{	// LHS shorter than RHS
			size_t i = VR_rhs.fast_size();
			while(FindArgRelatedToRHS(*VR_rhs.ArgArray[--i],NonStrictlyImpliesLogicalNOTOf))
				if (0==i)
					{
					Param1 = LHSIdx;	// NIFF this arg
					Param2 = RHSIdx;	// Delete this arg
					SelfEvalIdx = LogicalANDReplaceTheseNAry2ORWithNIFF_SER;
					return true;
					};
			break;
			}
		case 0:
			{
			if (OrderIndependentPairwiseRelation(VR_rhs,IsAntiIdempotentTo))
				{
				Param1 = LHSIdx;	// NIFF this arg
				Param2 = RHSIdx;	// Delete this arg
				SelfEvalIdx = LogicalANDReplaceTheseNAry2ORWithNIFF_SER;
				return true;
				};
			break;
			}
		case 1:
			{	// RHS shorter than LHS
			size_t i = fast_size();
			while(VR_rhs.FindArgRelatedToRHS(*ArgArray[--i],NonStrictlyImpliesLogicalNOTOf))
				if (0==i)
					{
					Param1 = RHSIdx;	// NIFF this arg
					Param2 = LHSIdx;	// Delete this arg
					SelfEvalIdx = LogicalANDReplaceTheseNAry2ORWithNIFF_SER;
					return true;
					};
			break;
			}
		};
		}
	//! \todo (?) FIX: disassemble this rule and move the parts to XOR StrictlyModifies
	//! <br>XOR StrictlyModifies OR: OR subvector arglist XOR: change OR to XOR
	//! <br>XOR StrictlyModifies XOR: 1st XOR subvector 2nd XOR: change 2nd XOR to AND args not in 1st XOR [1 arg: arg-extraction]
	else if (rhs.IsExactType(LogicalXOR_MC))
		{
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);
		if (SubvectorArgList(VR_rhs))
			{
			Param1 = LHSIdx;
			Param2 = RHSIdx;
			if (fast_size()+1==VR_rhs.fast_size())
				{
				SelfEvalIdx = LogicalANDReplaceORAndXORWithXORAndNOTArg_SER;
				return true;
				}
			else{
				SelfEvalIdx = LogicalANDReplaceORAndXORWithXORAndNORArg_SER;
				return true;
				}
			};
		};
	return false;
}

bool MetaConnective::LogicalANDFindDetailedRuleForIFF(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/10/2000
	if (rhs.IsExactType(LogicalIFF_MC))
		{	// check for arg splicing on anti-idempotent args
		if (FindTwoRelatedArgs(static_cast<MetaConnective&>(rhs),IsAntiIdempotentTo))
			{
			Param1 = LHSIdx;
			Param2 = RHSIdx;
			SelfEvalIdx = LogicalANDSpliceIFFAntiIdempotentArg_SER;
			return true;
			}
		};
//! \todo IMPLEMENT: (elsewhere) XOR(A,B,C) defines: A IFF AND(~B,~C), etc.
	return false;
}

bool MetaConnective::StrictlyImplies(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/14/2000
	return (this->*StrictlyImpliesAux[array_index()])(rhs);
}

bool MetaConnective::StrictlyImplies_AND(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT: NIFF: conjugate StrictlyImpliesLogicalNOTOf for IFF
// AND(A1..An) =>
//	AND: RHS is vector subarglist of LHS
//	IFF: IFF arglist is vector subarglist of AND-args, or all args of IFF are anti-idempotent AND-args
//	XOR: 1 arg is AND-arg, other n-1 XOR-args are anti-idempotent to AND-args; XOR has at most as many args as AND
//	NIFF: conjugate StrictlyImpliesLogicalNOTOf for IFF
//	NXOR: conjugate StrictlyImpliesLogicalNOTOf for XOR
// DONE:
	// Variable, and other direct quotations
	if (FindArgRelatedToRHS(rhs,NonStrictlyImplies)) return true;
	if (rhs.IsExactType(LogicalAND_MC))
		{
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);
		size_t i = VR_rhs.fast_size();
		while(NonStrictlyImplies(*this,*VR_rhs.ArgArray[--i]))
			if (0==i) return *this!=rhs;
		return false;	// short-circuit default: requires RHS.IsExactType(LogicalOR_MC)
		}
	if (rhs.IsExactType(LogicalIFF_MC))
		{
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);
		size_t i = VR_rhs.fast_size();
		if 		(NonStrictlyImplies(*this,*VR_rhs.ArgArray[--i]))
			{
			while(NonStrictlyImplies(*this,*VR_rhs.ArgArray[--i]))
				if (0==i) return true;
			}
		else if (NonStrictlyImpliesLogicalNOTOf(*this,*VR_rhs.ArgArray[i]))
			{
			while(NonStrictlyImpliesLogicalNOTOf(*this,*VR_rhs.ArgArray[--i]))
				if (0==i) return true;
			}
		return false;	// short-circuit default: requires RHS.IsExactType(LogicalOR_MC)
		}
	if (rhs.IsExactType(LogicalXOR_MC))
		{
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);
		if (    FindTwoRelatedArgs(VR_rhs,NonStrictlyImplies)
			&& !FindTwoRelatedArgs(VR_rhs,NonStrictlyImplies,InferenceParameter1,VR_rhs.InferenceParameter1))
			{
			size_t i = VR_rhs.fast_size();
			do	if (   --i!=VR_rhs.InferenceParameter1
					&& !FindArgRelatedToRHS(*VR_rhs.ArgArray[i],NonStrictlyImpliesLogicalNOTOf))
					return false;	// short-circuit default: requires RHS.IsExactType(LogicalOR_MC)
			while(0<i);
			return true;
			}
		return false;	// short-circuit default: requires RHS.IsExactType(LogicalOR_MC)
		}
	if (rhs.IsExactType(LogicalNXOR_MC))
		{
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);
		if (FindTwoRelatedArgs(VR_rhs,NonStrictlyImplies))
			// short-circuit default: requires RHS.IsExactType(LogicalOR_MC)
			// implying two distinct args of a NXOR makes it true
			return FindTwoRelatedArgs(VR_rhs,NonStrictlyImplies,InferenceParameter1,VR_rhs.InferenceParameter1);
		else{
			size_t i = VR_rhs.fast_size();
			do	if (!FindArgRelatedToRHS(*VR_rhs.ArgArray[--i],NonStrictlyImpliesLogicalNOTOf))
					return false;	// short-circuit default: requires RHS.IsExactType(LogicalOR_MC)
			while(0<i);
			return true;
			}
		return false;	// short-circuit default: requires RHS.IsExactType(LogicalOR_MC)
		}
	return TValStrictlyImpliesDefault(rhs);
}

bool MetaConnective::StrictlyImplies_OR(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/24/2003
	// OR(A1..An) =>
	//	OR(A1..An,B1..Bm)
	//	AND, IFF, XOR: n-ary not visible
	//	NIFF, NXOR: conjugate StrictlyImpliesLogicalNOTOf for IFF, XOR RHS [n-ary not visible]
	if (rhs.IsExactType(LogicalOR_MC))
		{	// OR(A1..An)=>OR(A1..An,B1..Bm): vector subarglist
		// OR(A1..An), OR(B1..Bm): Each arg A1 implies some arg Bi
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);

		size_t i = fast_size();
		do	if (!VR_rhs.FindArgRelatedToLHS(*ArgArray[--i],NonStrictlyImplies))
				return false;
		while(0<i);
		return *this!=rhs;
// AllArgsImpliedBySomeArgFailed: ....
		}
	return false;
}

bool MetaConnective::StrictlyImplies_IFF(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/9/2003
// IFF(A1..An) =>
//	IFF: all RHS IFF-args are == LHS IFF args, or all RHS IFF-args are anti-idempotent to LHS IFF-args
//	OR: 1 OR-arg == to IFF-arg, and 1 OR-arg antiidempotent to IFF-arg
//	NIFF: conjugate StrictlyImpliesLogicalNOTOf for IFF RHS
//	NXOR: conjugate StrictlyImpliesLogicalNOTOf for XOR RHS
//  AND, XOR: n-ary not visible
	if (rhs.IsExactType(LogicalIFF_MC))
		{	// OR(A1..An)=>OR(A1..An,B1..Bm): vector subarglist
		// IFF(A1..An), IFF(B1..Bm): Each arg A1 implies some arg Bi, or implies logical not of some Bi
		const MetaConnective& VR_rhs = static_cast<const MetaConnective&>(rhs);

		size_t i = VR_rhs.fast_size();
		if (FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],NonStrictlyImplies))
			{
			do	if (!FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],NonStrictlyImplies))
					return false;
			while(0<i);
			return *this!=rhs;
			}
		else if (FindArgRelatedToLHS(*VR_rhs.ArgArray[i],NonStrictlyImpliesLogicalNOTOf))
			{
			do	if (!FindArgRelatedToLHS(*VR_rhs.ArgArray[--i],NonStrictlyImpliesLogicalNOTOf))
					return false;
			while(0<i);
			return *this!=rhs;
			}
// AllArgsImpliedBySomeArgFailed: ....
		};
	return TValStrictlyImpliesDefault(rhs);
}

bool MetaConnective::StrictlyImplies_XOR(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
// XOR(A1..An) =>
//	OR: XOR-args are a subset of OR-args
//	NIFF: conjugate StrictlyImpliesLogicalNOTOf for IFF RHS
//	NXOR: conjugate StrictlyImpliesLogicalNOTOf for XOR RHS
//	AND, IFF, XOR: n-ary not visible
	// 2 OR-args LogicalNOTOfNonStrictlyImplied by XOR-args
	//! \todo OPTIMIZE: generalize this.  RHS needs to be 'functionally' OR.
	if (   FindArgRelatedToRHS(rhs,LogicalNOTOfNonStrictlyImplies,1,fast_size()-1)
		&& FindArgRelatedToRHS(rhs,LogicalNOTOfNonStrictlyImplies,0,InferenceParameter1-1))
		return true;
	// If all n XOR args imply target, target is implied
	//! \todo ALTER: this and above to iteration over n AND collapses (comprehensive)
	size_t i = fast_size();
	do	if (!ArgArray[--i]->StrictlyImplies(rhs))
			goto TotalXORImplyFailed;
	while(0<i);
	return true;
TotalXORImplyFailed:
	return TValStrictlyImpliesDefault(rhs);
}

bool MetaConnective::StrictlyImplies_NXOR(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
// NXOR(A1..An) =>
//	OR: [complicated]
//	NIFF, NXOR: conjugate StrictlyImpliesLogicalNOTOf for IFF/XOR: n-ary not visible
//	AND, IFF, XOR: n-ary not visible
	return TValStrictlyImpliesDefault(rhs);
}

bool MetaConnective::StrictlyImplies_NIFF(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
// NIFF(A1..An) =>
//	OR: [complicated]
//	NIFF: conjugate StrictlyImpliesLogicalNOTOf for IFF RHS
//	NXOR: conjugate StrictlyImpliesLogicalNOTOf for XOR RHS
//	AND, IFF, XOR: n-ary not visible
	return TValStrictlyImpliesDefault(rhs);
}

bool MetaConnective::NORNANDFatal(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd 3/13/2000
	UnconditionalDataIntegrityFailure();
	return false;
}

// QState.cxx support
//! \todo (?): IMPLEMENT THESE FOR: LogicalIFF, LogicalXOR, LogicalNIFF
// NOTE: Franci wants NIFF: arg LogicalNOTOfNonStrictlyImplies => use AND of other args
bool MetaConnective::CouldAugmentHypothesis() const
{	//! \todo IMPLEMENT
	if (IsExactType(LogicalOR_MC) || IsExactType(LogicalIFF_MC) || IsExactType(LogicalXOR_MC) || IsExactType(LogicalNIFF_MC))
		return true;
	return false;
}

bool MetaConnective::CanAugmentHypothesis(const MetaConcept& Hypothesis) const
{	//! \todo IMPLEMENT: there are other structurally useful types that can occur as the first arg of an IFF.
	if      (IsExactType(LogicalOR_MC))
		return FindArgRelatedToRHS(Hypothesis,NonStrictlyImplies);
	else if (IsExactType(LogicalIFF_MC))
		{
		if (ArgArray[0]->IsExactType(Variable_MC))
			{
			if 		(IsAntiIdempotentTo(*ArgArray[0],Hypothesis))
				return FindArgRelatedToRHS(Hypothesis,LogicalNOTOfNonStrictlyImplies);		
			else if (*ArgArray[0]!=Hypothesis)
				return FindArgRelatedToRHS(Hypothesis,NonStrictlyImplies);
			}
		}
	else if (IsExactType(LogicalXOR_MC))
		return FindArgRelatedToRHS(Hypothesis,LogicalNOTOfNonStrictlyImplies);
	else if (IsExactType(LogicalNIFF_MC))
		return FindArgRelatedToRHS(Hypothesis,LogicalNOTOfNonStrictlyImplies) || FindArgRelatedToRHS(Hypothesis,NonStrictlyImplies);
	return false;
}

bool MetaConnective::AugmentHypothesis(MetaConcept*& Hypothesis) const
{	//! \todo IMPLEMENT
	// NOTE: the previous member function call was CanAugmentHypothesis: InferenceParameter1 is set
	MetaConcept* Tmp = NULL;
	try	{
		if (IsExactType(LogicalOR_MC))
			{
			if (2<fast_size())
				{
				CopyInto(Tmp);
				static_cast<MetaConnective*>(Tmp)->DeleteIdx(InferenceParameter1);
				}
			else{
				ArgArray[1-InferenceParameter1]->CopyInto(Tmp);
				};
			Tmp->SelfLogicalNOT();
			}
		else if (IsExactType(LogicalIFF_MC))
			{
			ArgArray[0]->CopyInto(Tmp);
			if (LogicalNOTOfNonStrictlyImplies(*ArgArray[InferenceParameter1],*Hypothesis))
				Tmp->SelfLogicalNOT();
#if 0		// other case: exhaustive listing.  This one does nothing.
			else if (NonStrictlyImplies(*ArgArray[InferenceParameter1],*Hypothesis))
#endif
			}
		else if (IsExactType(LogicalXOR_MC))
			{	// InferenceParameter1 points to the arg that *must* not be returned.  We have
				// some flexibility, otherwise.
			//! \todo n-ary XOR could simply tabulate which choice 'alters the most targets at once'
			size_t TestIdx = (0==InferenceParameter1) ? 1 : 0;
			if (ArgArray[TestIdx]->IsExactType(Variable_MC) || ArgArray[TestIdx]->IsExactType(LogicalIFF_MC))
				{
				ArgArray[TestIdx]->CopyInto(Tmp);
				}
			else{	// Don't have a nice arg in earliest slot.
				//! \todo AI sweep for nice arg to augment to.
				ArgArray[2]->CopyInto(Tmp);
				}
			}
		else{	// IsExactType(LogicalNIFF_MC)
			CopyInto(Tmp);
			if (LogicalNOTOfNonStrictlyImplies(*ArgArray[InferenceParameter1],*Hypothesis))
				static_cast<MetaConnective*>(Tmp)->SetExactTypeV2(LogicalAND_MC);
			else	// if (NonStrictlyImplies(*ArgArray[InferenceParameter1],*Hypothesis))
				static_cast<MetaConnective*>(Tmp)->SetNANDNOR(LogicalNOR_MC);
			static_cast<MetaConnective*>(Tmp)->DeleteIdx(InferenceParameter1);
			}
		}
	catch(const bad_alloc&)
		{
		delete Tmp;
		return false;
		}
	delete Hypothesis;
	Hypothesis = Tmp;
	return true;
}

void MetaConnective::StrictlyModifies(MetaConcept*& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/30/2000
	return (this->*StrictlyModifiesAux[array_index()])(rhs);
}

void MetaConnective::StrictlyModifies_AND(MetaConcept*& rhs) const
{	//! \todo IMPLEMENT
}

//! \todo augment
// if all args in OR NonStrictlyimply some arg in IFF, turn IFF into AND
// if all args in OR NonStrictlyimplyLogicalNotOF some arg in IFF, turn IFF into NAND.
void MetaConnective::StrictlyModifies_OR(MetaConcept*& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/2001
	if 		(2==fast_size() && rhs->CanUseThisAsMakeImply(*this))
		{
		if (   rhs->IsExactType(LogicalOR_MC)
			&& 2==static_cast<MetaConnective*>(rhs)->fast_size())
			{
			if 		(   ValidLHSForMakesLHSImplyRHS(*static_cast<MetaConnective*>(rhs)->ArgArray[0])
					 && MakesLHSImplyRHS(*static_cast<MetaConnective*>(rhs)->ArgArray[0],*static_cast<MetaConnective*>(rhs)->ArgArray[1]))
				{
				MetaConcept* Tmp = NULL;
				static_cast<MetaConnective*>(rhs)->TransferOutAndNULL(1,Tmp);
				delete rhs;
				rhs = Tmp;
				return;
				}
			else if (   ValidLHSForMakesLHSImplyRHS(*static_cast<MetaConnective*>(rhs)->ArgArray[1])
					 && MakesLHSImplyRHS(*static_cast<MetaConnective*>(rhs)->ArgArray[1],*static_cast<MetaConnective*>(rhs)->ArgArray[0]))
				{
				MetaConcept* Tmp = NULL;
				static_cast<MetaConnective*>(rhs)->TransferOutAndNULL(0,Tmp);
				delete rhs;
				rhs = Tmp;
				return;
				}
			//! \todo generalize the above:
			// OR(A_1,...,A_N,B),OR(A_1,...,A_N,~B): modify RHS to OR(A_1,...,A_N)
			// will be handled by hypercube code in SpeculativeOR
			}
		rhs->UseThisAsMakeImply(*this);
		}
	else if (LogicalAND_MC==static_cast<MetaConnective*>(rhs)->InferenceParameter1)
		static_cast<MetaConnective*>(rhs)->SetExactTypeV2(LogicalAND_MC);
	else	// if (LogicalNOR_MC==static_cast<MetaConnective*>(RHS)->InferenceParameter1)
		static_cast<MetaConnective*>(rhs)->SetNANDNOR(LogicalNOR_MC);
}

bool
MetaConnective::StrictlyModifies_IFFAux(MetaConcept*& rhs,LowLevelBinaryRelation* TargetRelation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	MetaConnective& VR_RHS = *static_cast<MetaConnective*>(rhs);
	if (FindTwoRelatedArgs(VR_RHS,*TargetRelation))
		{
		size_t LHSIdx1 = InferenceParameter1;
		size_t RHSIdx1 = VR_RHS.InferenceParameter1;
		if (FindTwoRelatedArgs(VR_RHS,*TargetRelation,LHSIdx1,RHSIdx1))
			{
			size_t KeepIdx = RHSIdx1;
			size_t DropIdx = VR_RHS.InferenceParameter1;

			// no other preference: keep the earlier arg in the IFF
			if (LHSIdx1>InferenceParameter1)
				{
				KeepIdx = VR_RHS.InferenceParameter1;
				DropIdx = RHSIdx1;
				}

			if (2==VR_RHS.fast_size())
				{
				MetaConcept* Tmp = NULL;
				VR_RHS.TransferOutAndNULL(KeepIdx,Tmp);
				delete rhs;
				rhs = Tmp;
				return true;
				}
			else{
				VR_RHS.DeleteIdx(DropIdx);
				if (   2==VR_RHS.fast_size()
					&& VR_RHS.IsExactType(LogicalNIFF_MC))
					VR_RHS.SetExactTypeV2(LogicalIFF_MC);
				return true;
				}
			}
		}
	return false;
}

void MetaConnective::StrictlyModifies_IFF(MetaConcept*& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/23/2003
	if (!ArgArray[0]->IsExactType(Variable_MC))
		{
		if (   rhs->IsExactType(LogicalAND_MC)
			|| rhs->IsExactType(LogicalOR_MC)
			|| rhs->IsExactType(LogicalNIFF_MC))
			{
			if (   StrictlyModifies_IFFAux(rhs,AreSyntacticallyEqual)
				|| StrictlyModifies_IFFAux(rhs,IsAntiIdempotentTo))
				return;
			}
		}
	rhs->UseThisAsMakeImply(*this);
}

#if 0
//! \todo FIX:
// IFF(A,AND(~B,~C)),
// XOR(A,B,C)
// 2-ary IFF is TRUE
// n-ary IFF : delete AND
// this is an irreversible implication
#endif
void MetaConnective::StrictlyModifies_XOR(MetaConcept*& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	// TODO: FIX: applying XOR(A,B,C) to AND(~A,...) should cause XOR(B,C) to be applied to AND(~A,...)
	rhs->UseThisAsMakeImply(*this);
}

void MetaConnective::StrictlyModifies_NXOR(MetaConcept*& rhs) const
{	//! \todo IMPLEMENT
}

void MetaConnective::StrictlyModifies_NIFF(MetaConcept*& rhs) const
{	//! \todo IMPLEMENT
}

bool MetaConnective::CanStrictlyModify(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/30/2000
	return (this->*CanStrictlyModifyAux[array_index()])(rhs);
}

bool MetaConnective::CanStrictlyModify_AND(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	return false;
}

bool MetaConnective::CanStrictlyModify_OR(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	if (2==fast_size() && rhs.CanUseThisAsMakeImply(*this))
		return true;
	//! \todo this is actually a specialized instance of a meta-target rule:
	//! X implies OR-version of IFF => IFF collapses to AND-version	
	if (rhs.IsExactType(LogicalIFF_MC))
		{	// NOTE: this code can be spoofed [OR or AND args in IFF, LHS OR has non-atomic args]
		// LHS => OR: convert to AND-form
		const MetaConnective& VR_RHS = static_cast<const MetaConnective&>(rhs);
		size_t i = fast_size();
		while(VR_RHS.FindArgRelatedToLHS(*ArgArray[--i],NonStrictlyImplies))
			if (0==i)
				{
				VR_RHS.InferenceParameter1 = LogicalAND_MC;
				return true;
				};
		i = fast_size();
		while(VR_RHS.FindArgRelatedToLHS(*ArgArray[--i],NonStrictlyImpliesLogicalNOTOf))
			if (0==i)
				{
				VR_RHS.InferenceParameter1 = LogicalNOR_MC;
				return true;
				};
		};
	return false;
}

//! \todo AUGMENT StrictlyModify for IFF
// The substitution rules, while they always simplify, do not always provide 
// the most effective applications.
// Currently, consider the following:
// AND, OR: prune arg; reduction to 1-ary has usual twist
// IFF: if some arg implies truth/falsity, and opposite form also implies truth/falsity: convert
// XOR: complicated
// NIFF: prune arg; reduction to 2-ary has usual twist
// NXOR: complicated
bool MetaConnective::CanStrictlyModify_IFF(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	if (!ArgArray[0]->IsExactType(Variable_MC))
		{
		if (   rhs.IsExactType(LogicalAND_MC)
			|| rhs.IsExactType(LogicalOR_MC)
			|| rhs.IsExactType(LogicalNIFF_MC))
			{
			const MetaConnective& VR_RHS = static_cast<const MetaConnective&>(rhs);
			if (FindTwoEqualArgsLHSRHSLexicalOrderedArgs(VR_RHS))
				return FindTwoRelatedArgs(VR_RHS,AreSyntacticallyEqual,InferenceParameter1,VR_RHS.InferenceParameter1);
			if (FindTwoRelatedArgs(VR_RHS,IsAntiIdempotentTo))
				return FindTwoRelatedArgs(VR_RHS,IsAntiIdempotentTo,InferenceParameter1,VR_RHS.InferenceParameter1);
			};
		};
	return rhs.CanUseThisAsMakeImply(*this);
}

bool MetaConnective::CanStrictlyModify_XOR(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2000
	return rhs.CanUseThisAsMakeImply(*this);
}

bool MetaConnective::CanStrictlyModify_NXOR(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	return false;
}

bool MetaConnective::CanStrictlyModify_NIFF(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	return false;
}

bool AmplifyOrBeAmplified(const MetaConcept& lhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2003
	if (lhs.WantToBeAmplified() || lhs.CanAmplifyClause())
		return true;
	return false;
}

bool NotAVariable(const MetaConcept& lhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2003
	return !lhs.IsExactType(Variable_MC);
}

bool LHSAmplifiesRHS(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2003
#if 1
	if (lhs.CanAmplifyThisClause(rhs))
		return true;
#else	// full version
	if (   lhs.CanAmplifyClause()
		&& rhs.WantToBeAmplified()
		&& lhs.CanAmplifyThisClause(rhs))
		return true;
#endif
	return false;
}

void AmplifyTarget(MetaConcept*& Target, const MetaConcept& Inducer)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2003
	Inducer.AmplifyThisClause(Target);
}

bool CanUseAmplification(const MetaConcept& lhs, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2003
	return rhs.CanAmplifyThisClause(lhs);
}

#define LOGICAL_IMPLIES_ACTION SC_ANALYZING+1
#define DEEP_LOGICAL_IMPLIES_ACTION SC_ANALYZING+2
#define DEEP_STRICTLY_MODIFIES_ACTION SC_ANALYZING+3
#define STRICTLY_MODIFIES_ACTION SC_ANALYZING+4

signed int
AmplifiedStatementIsUseful(const MetaConcept& Amplified, const MetaConcept& ApprovalTarget)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2003
	if (NonStrictlyImplies(Amplified,ApprovalTarget))
		return LOGICAL_IMPLIES_ACTION;
	if (Amplified.CanStrictlyModify(ApprovalTarget))
		return STRICTLY_MODIFIES_ACTION;
	if (DeepLogicallyImplies(Amplified,ApprovalTarget))
		return DEEP_LOGICAL_IMPLIES_ACTION;
	if (CanDeepStrictlyModify(Amplified,ApprovalTarget))
		return DEEP_STRICTLY_MODIFIES_ACTION;
	return SC_ANALYZING;
}

static bool
DiagnoseAmplificationCapability(size_t& TargetIdx, signed int& FullyAmplifiedAnalysisMode, MetaConcept**& AmplifySource, const MetaConcept& AmplifyMe, MetaConcept** ApprovalTargetsImage)
{
	MetaConcept* PreviewAmplified = NULL;
	try	{
		AmplifyMe.CopyInto(PreviewAmplified);
		}
	catch(const bad_alloc&)
		{
		return false;
		}

	size_t i = ArraySize(AmplifySource);
	do	if (    AmplifySource[--i]->CanAmplifyThisClause(*PreviewAmplified)
			&& !AmplifySource[i]->AmplifyThisClause(PreviewAmplified))
			{
			delete PreviewAmplified;
			return false;
			}
	while(0<i);

	if 		(::FindArgRelatedToLHS(*PreviewAmplified,::CanStrictlyModify,ApprovalTargetsImage,TargetIdx))
		FullyAmplifiedAnalysisMode = STRICTLY_MODIFIES_ACTION;
	else if (::FindArgRelatedToLHS(*PreviewAmplified,CanDeepStrictlyModify,ApprovalTargetsImage,TargetIdx))
		FullyAmplifiedAnalysisMode = DEEP_STRICTLY_MODIFIES_ACTION;
	else if (::FindArgRelatedToLHS(*PreviewAmplified,DeepLogicallyImplies,ApprovalTargetsImage,TargetIdx))
		FullyAmplifiedAnalysisMode = DEEP_LOGICAL_IMPLIES_ACTION;
	else if (::FindArgRelatedToLHS(*PreviewAmplified,NonStrictlyImplies,ApprovalTargetsImage,TargetIdx))
		FullyAmplifiedAnalysisMode = LOGICAL_IMPLIES_ACTION;

	delete PreviewAmplified;
	return true;
}

//! \todo A comprehensive overview of evaluation rules.
void MetaConnective::DiagnoseIntermediateRulesANDAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/22/2000
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(2<size());
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	// 3+ary case
	// NOTE: ==, anti-idempotent pairs have already been cleaned up.  Also,
	// AND-objects do not contain quantifiers.

	// Viewpoint scan approach.  This is amenable to functionalizing (which makes this code 
	// usable in other contexts, usually relating to imminent deletion of target
	// NOTE: this code needs two forms: one for imminent deletion, one for use here
	InferenceParameter1 = 0;
	do	if (VertexDiagnoseIntermediateRulesANDAux())
			return;
	while(fast_size()>++InferenceParameter1);
	DEBUG_LOG("VertexDiagnoseIntermediateRulesANDAux loop OK");

	// Franci may need to spawn OR-statements to trigger the OR/XOR reduction rule.
	// This has to fire after the standard OR-scan.  Franci should do other speculative
	// OR generation rules here.
	if (LogicalANDCreateSpeculativeOR())
		return;	
	DEBUG_LOG("LogicalANDCreateSpeculativeOR OK, false");

	//! \todo LogicalAND could use a StrictlyImplies family augmented by a list of MetaConcepts
	//! to use MakesStrictlyImply against.  Invocation time should be fairly late.  This would be
	//! a second tier LogicalANDFindDetailedRule.  This would affect the test-case.

	//! \todo start function target
	// Preview: should we even try to allocate RAM?
#ifdef AMPLIFY_TAUTOLOGIES
	MetaConcept** TautologyIndex = NULL;
#endif
	MetaConcept** MirrorArgList = NULL;
	size_t MayAmplify = 0;
	size_t ShouldBeAmplified = 0;
	size_t SweepIdx = fast_size();
	do	{
		if (ArgArray[--SweepIdx]->CanAmplifyClause())
			MayAmplify++;
		if (ArgArray[SweepIdx]->WantToBeAmplified())
			ShouldBeAmplified++;
		}
	while(0<SweepIdx);
	DEBUG_LOG("Amplify stats OK");

	//! \todo XOR(A,B,C),XOR(~A,D,E) => amplify OR(A,~A) [maybe check for these anyway..no,
	// worthless IFF hookins.  IFF should not amplify OR(A,~A).]
#ifdef AMPLIFY_TAUTOLOGIES
	if (0==MayAmplify)
		goto EndAmplification;

	if (   LogicalXOR_MC<=ArgArray[fast_size()-1]->ExactType()
		&& LogicalXOR_MC>=ArgArray[0]->ExactType())
		{
		SweepIdx = fast_size();
		do	{
			if (ArgArray[--SweepIdx]->IsExactType(LogicalXOR_MC))
				{
				size_t SweepIdx2 = SweepIdx;
				while(0<SweepIdx2)
					if      (ArgArray[--SweepIdx2]->IsExactType(LogicalXOR_MC))
						{
						if (static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[SweepIdx]),IsAntiIdempotentTo))
							{	// OK, have anti-idempotent args...are we clear to expand?
							bool AlreadyUsing = false;
							if (NULL!=TautologyIndex)
								{
								size_t AltIdx = ArraySize(TautologyIndex);
								do	if (NonStrictlyImpliesThisOrLogicalNOTOf(*TautologyIndex[--AltIdx]->ArgN(0),*static_cast<MetaConnective*>(ArgArray[SweepIdx])->ArgArray[static_cast<MetaConnective*>(ArgArray[SweepIdx])->InferenceParameter1]))
										{
										AlreadyUsing = true;
										break;
										}
								while(0<AltIdx);
								}
							if (!AlreadyUsing)
								{	// construct OR(A,~A)
								// realloc is malloc for NULL source pointer
								const OldTautologyLength = SafeArraySize(TautologyIndex);
								if (_resize(TautologyIndex,OldTautologyLength+1))
									{
									MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(2);
									if (NewArgArray)
										{
										try	{
											static_cast<MetaConnective*>(ArgArray[SweepIdx])->ArgArray[static_cast<MetaConnective*>(ArgArray[SweepIdx])->InferenceParameter1]->CopyInto(NewArgArray[0]);
											static_cast<MetaConnective*>(ArgArray[SweepIdx])->ArgArray[static_cast<MetaConnective*>(ArgArray[SweepIdx])->InferenceParameter1]->CopyInto(NewArgArray[1]);
											NewArgArray[1]->SelfLogicalNOT();
											TautologyIndex[ArraySize(TautologyIndex)-1] = new MetaConnective(NewArgArray,OR_MCM);
											ShouldBeAmplified++;
											}
										catch(const bad_alloc&)
											{
											BLOCKDELETEARRAY_AND_NULL(NewArgArray);
											_shrink(TautologyIndex,OldTautologyLength);
											continue;
											}
										}
									else
										_shrink(TautologyIndex,OldTautologyLength);
									}
								}
							}
						}
					else if (LogicalXOR_MC>ArgArray[SweepIdx2]->ExactType())
						break;
				}
			else if (LogicalXOR_MC>ArgArray[SweepIdx]->ExactType())
				break;
			}
		while(0<SweepIdx);
		}

	if (   0==ShouldBeAmplified
		|| !::GrepArgList(MirrorArgList,AmplifyOrBeAmplified,ArgArray))
		goto EndAmplification;

	if (NULL!=TautologyIndex)
		{	// Prepend the tautologies to the grep results in MirrorArgList
		size_t OriginalMirrorSize = _msize(MirrorArgList);
		MetaConcept** MirrorArgList2 = REALLOC(MirrorArgList,_msize(MirrorArgList)+_msize(TautologyIndex));
		if (NULL!=MirrorArgList2)
			{
			MirrorArgList = MirrorArgList2;
			memmove(&MirrorArgList[ArraySize(TautologyIndex)],MirrorArgList,OriginalMirrorSize);
			memmove(MirrorArgList,TautologyIndex,_msize(TautologyIndex));
			}
		else{
			ShouldBeAmplified -= ArraySize(TautologyIndex);
			BLOCKDELETEARRAY_AND_NULL(TautologyIndex);
			if (0==ShouldBeAmplified)
				DELETEARRAY_AND_NULL(MirrorArgList);
			}
		}
#else
	if (   0==MayAmplify
		|| 0==ShouldBeAmplified
		|| !::GrepArgList(MirrorArgList,AmplifyOrBeAmplified,ArgArray))
		{
		DEBUG_LOG("GrepArgList OK, false");
		goto EndAmplification;
		}
	DEBUG_LOG("GrepArgList OK, true");
#endif

	// #1: construct list of clauses that either want to be amplified, or can be amplified.
	if (NULL!=MirrorArgList)
		{
		DEBUG_LOG("Entering Amplification for AND");

		// #2: Stuff this into a Digraph.  AutoInit.  Flush uninvolved vertices.
		// #3: For each vertex that can be amplified: construct corresponding SearchTree.  Expand
		// until it stabilizes; if it finds something useful, destructive-extract the expanded 
		// clause.  BE SURE SEARCHTREE CAN DO THIS: Destructive extraction requires a unique leaf,
		// so some sort of conflict resolution is required for multiple leaves of the same final rating.
		// otherwise, remove the tested vertex, then any newly uninvolved vertices.

		Digraph* TraceAmplification = NULL;
		PROPERLY_INIT_DIGRAPH(TraceAmplification,MirrorArgList,false,LHSAmplifiesRHS,DELETE,DELETEARRAY,goto EndAmplification)

		TraceAmplification->ResetDiagonal();

		// Pre-init before Flush...
		TraceAmplification->InitForClauseAmplification();
		TraceAmplification->FlushVerticesWithDirectedEdgeCountsLTE(0,0);
		while(2<=TraceAmplification->size())
			{
			DEBUG_LOG("Amplication starting");

			// #1: grab a vertex that is being amplified (has to-edges)
			//! \todo function target?
			//! \todo do we want minimal non-zero arity?  It might make sense for this particular
			//! application.  (Nope...low-specificity targets tested first).
			//! what about maximal arity?
			unsigned long CumulativeComplexity = 1;

			size_t i = TraceAmplification->size();
			size_t ToEdgeCount = 0;
			while(0==(ToEdgeCount = TraceAmplification->VertexToEdgeCount(--i)))
				if (0==i)
					{
					delete TraceAmplification;
					UnconditionalDataIntegrityFailure();
					};

			// maximize arity of amplification list, but minimize arity of amplifyee
			TraceAmplification->MaxMinArityForAmplication(i,ToEdgeCount);

			DEBUG_LOG("Vertex OK");

			// #2: The amplifiers (sources of the to-edges) become the NewBranchingOperationSources
			MetaConcept** AmplifySource = NULL;
			TraceAmplification->MirrorSourceVerticesForIdxNoLoopsNoOwnership(i,ToEdgeCount,0,TraceAmplification->size(),AmplifySource);
			if (!AmplifySource)
				{
				DEBUG_LOG("Mirroring of amplifiers bad");
				delete TraceAmplification;
				goto EndAmplification;
				}
			DEBUG_LOG("Mirroring of amplifiers OK");
			size_t AmplifierCount = ArraySize(AmplifySource);

			// #3: branching operation: AmplifyTarget (defined above)
			// #4: NewCanUseBranchingOperation: CanUseAmplification (defined above)
			// #5: the list of clauses that are neither amplified, nor used to amplify, is 
			//     NewApprovalTargets.  We may wish to weed out naked variables.
			//     We may wish to weed out statements completely non-interacting with
			//     all of the reserved clauses.  The variables part is easier.
			MetaConcept** ApprovalTargetsImage = NULL;
			if (!::GrepArgList(ApprovalTargetsImage,NotAVariable,ArgArray))
				{
				DEBUG_LOG("Mirroring of targets bad");
				free(AmplifySource);
				delete TraceAmplification;
				goto EndAmplification;
				}
			DEBUG_LOG("Mirroring of targets OK");
			if (!ApprovalTargetsImage)
				{
				DEBUG_LOG("No targets found");
				free(AmplifySource);
				delete TraceAmplification;
				UnconditionalDataIntegrityFailure();
			}
			// eliminate statement amplified
			SelfGrepArgListNoOwnership(AreSyntacticallyUnequal,*TraceAmplification->ArgN(i),ApprovalTargetsImage);

			DEBUG_LOG("Self-amplification cancelled");

			// eliminate statements used to amplify
			size_t Idx2 = ArraySize(AmplifySource);
			do	SelfGrepArgListNoOwnership(AreSyntacticallyUnequal,*AmplifySource[--Idx2],ApprovalTargetsImage);
			while(0<Idx2);

			DEBUG_LOG("Amplifier-amplification cancelled");
			//! \todo (???): eliminate unrelated statements.

			// preview: see if full amplification actually does anything
			size_t TargetIdx = 0;
			signed int FullyAmplifiedAnalysisMode = 0;
			if (!DiagnoseAmplificationCapability(TargetIdx,FullyAmplifiedAnalysisMode,AmplifySource,*TraceAmplification->ArgN(i),ApprovalTargetsImage))
				{
				DEBUG_LOG("Diagnosis OK, false");
				free(ApprovalTargetsImage);
				free(AmplifySource);
				delete TraceAmplification;
				goto EndAmplification;
				}

			DEBUG_LOG("Diagnosis OK, true");

			if (0==FullyAmplifiedAnalysisMode)
				{	// does nothing...ignore
				free(ApprovalTargetsImage);
				free(AmplifySource);
				TraceAmplification->RemoveVertex(i);
				TraceAmplification->FlushVerticesWithDirectedEdgeCountsLTE(0,0);
				continue;
				}			
			//! \todo retain this as a default technique, and/or optimize by removing
			//! irrelevant data items.  An irrelevant data item is defined as one whose 
			//! removal does not reduce the effect of the corresponding fully amplified 
			//! statement. Do the reduction only for "large" examples.

			// #6: need a custom function for approval: AmplifiedStatementIsUseful, defined above
			// #7: construct SearchTree
			MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(1);
			if (!NewArgArray)
				{
				free(ApprovalTargetsImage);
				free(AmplifySource);
				delete TraceAmplification;
				goto EndAmplification;
				}			

			SearchTree* ExploreThis = NULL;
			try	{
				TraceAmplification->ArgN(i)->CopyInto(NewArgArray[0]);
				ExploreThis = new SearchTree(NewArgArray,
											AmplifyTarget,
											CanUseAmplification,
											AmplifiedStatementIsUseful,
											AmplifySource,
											ApprovalTargetsImage);
				}
			catch(const bad_alloc&)
				{
				BLOCKDELETEARRAY(NewArgArray);
				free(ApprovalTargetsImage);
				free(AmplifySource);
				delete TraceAmplification;
				goto EndAmplification;				
				}

			LOG("Testing amplification of");
			LOG(*TraceAmplification->ArgN(i));

			// #8: Execute SearchTree
			// This application will want 'cancel equal nodes'; make sure 
			// the Amplify interface functions can handle this
			// ...
			CumulativeComplexity *= AmplifierCount--;
			LOG("Apparent complexity of amplification iteration:");
			LOG(CumulativeComplexity);

			bool RAMStalled = false;
			signed int SearchResultCode = ExploreThis->BreadthSearchOneStage(RAMStalled);

			DEBUG_LOG("Bootstrap OK");
			size_t IterationCount = 1;
			while(SearchResultCode && !RAMStalled)
				{
				DEBUG_LOG(SearchResultCode);
				if (SC_ANALYZING<SearchResultCode)
					{
					DEBUG_LOG("Have diagnosis");
					size_t ApprovalIdx = 0;
					MetaConcept* Target = NULL;
					if (!ExploreThis->DestructiveExtractUniqueResult(Target))
						goto SearchFailed;
					delete ExploreThis;
					LOG("Amplified");
					LOG(*Target);
					LOG("from");
					LOG(*TraceAmplification->ArgN(i));
					delete TraceAmplification;
					DEBUG_LOG("Past delete TraceAmplification");

					if 		(LOGICAL_IMPLIES_ACTION==SearchResultCode)
						{	// Strictly implies
						DEBUG_LOG("LOGICAL_IMPLIES_ACTION branch");
						SUCCEED_OR_DIE(::FindArgRelatedToLHS(*Target,NonStrictlyImplies,ApprovalTargetsImage,ApprovalIdx));
						SUCCEED_OR_DIE(FindArgRelatedToLHS(*ApprovalTargetsImage[ApprovalIdx],AreSyntacticallyEqual));
						LOG(msz_Using);
						LOG(*Target);
						delete Target;
						LOG(msz_SyntaxImplies);
						LOG(*ArgArray[InferenceParameter1]);
						if (2==fast_size())
							{
							InvokeEvalForceArg(1-InferenceParameter1);
							return;
							}
						else{
							IdxCurrentSelfEvalRule = SelfEvalRuleCleanArg_SER;
							return;
							}
						}
					else if (DEEP_LOGICAL_IMPLIES_ACTION==SearchResultCode)
						{	// deep logically implies
						DEBUG_LOG("DEEP_LOGICAL_IMPLIES_ACTION branch");
						SUCCEED_OR_DIE(::FindArgRelatedToLHS(*Target,DeepLogicallyImplies,ApprovalTargetsImage,ApprovalIdx));
						SUCCEED_OR_DIE(FindArgRelatedToLHS(*ApprovalTargetsImage[ApprovalIdx],AreSyntacticallyEqual));
						InferenceParameterMC = Target;
						IdxCurrentSelfEvalRule = VirtualDeepLogicallyImplies_SER;
						return;
						}
					else if (DEEP_STRICTLY_MODIFIES_ACTION==SearchResultCode)
						{	// deep strictly modifies
						DEBUG_LOG("DEEP_STRICTLY_MODIFIES_ACTION branch");
						SUCCEED_OR_DIE(::FindArgRelatedToLHS(*Target,CanDeepStrictlyModify,ApprovalTargetsImage,ApprovalIdx));
						SUCCEED_OR_DIE(FindArgRelatedToLHS(*ApprovalTargetsImage[ApprovalIdx],AreSyntacticallyEqual));
						InferenceParameterMC = Target;
						IdxCurrentSelfEvalRule = VirtualDeepStrictlyModify_SER;
						return;
						}
					else // if (STRICTLY_MODIFIES_ACTION==SearchResultCode)
						{	// Strictly modifies
						DEBUG_LOG("STRICTLY_MODIFIES_ACTION branch");
						SUCCEED_OR_DIE(::FindArgRelatedToLHS(*Target,::CanStrictlyModify,ApprovalTargetsImage,ApprovalIdx));
						SUCCEED_OR_DIE(FindArgRelatedToLHS(*ApprovalTargetsImage[ApprovalIdx],AreSyntacticallyEqual));
						InferenceParameterMC = Target;
						IdxCurrentSelfEvalRule = VirtualStrictlyModify_SER;
						return;
						}

#undef LOGICAL_IMPLIES_ACTION
#undef DEEP_LOGICAL_IMPLIES_ACTION
#undef DEEP_STRICTLY_MODIFIES_ACTION
#undef STRICTLY_MODIFIES_ACTION

					return;
					};
				if (0==AmplifierCount) break;

				CumulativeComplexity /= IterationCount;
				CumulativeComplexity *= AmplifierCount--;
				IterationCount++;
				// arbitary heuristic: idea is not to chase the wild goose forever and ever
				//!! \todo allow SearchTree to recycle solo leaves as a config option
				if (2<IterationCount && 400<CumulativeComplexity) break;

				LOG("Apparent complexity of amplification iteration:");
				LOG(CumulativeComplexity);
				LOG("for");
				LOG(*TraceAmplification->ArgN(i));
				SearchResultCode = ExploreThis->BreadthSearchOneStage(RAMStalled);
				}

			DEBUG_LOG("Normal end to Amplification");

SearchFailed:
			// #9: If this SearchTree fizzles:
			delete ExploreThis;
			free(ApprovalTargetsImage);
			free(AmplifySource);
			TraceAmplification->RemoveVertex(i);
			TraceAmplification->FlushVerticesWithDirectedEdgeCountsLTE(0,0);
			DEBUG_LOG("Incremental TraceAmplificaton clean OK");
			}

		delete TraceAmplification;
		DEBUG_LOG("DELETE(TraceAmplification) OK");
		}

EndAmplification:
//! \todo end function target
#ifdef AMPLIFY_TAUTOLOGIES
	if (TautologyIndex) BLOCKDELETEARRAY(TautologyIndex);
#endif

	//! \todo MANY OTHER INFERENCE RULES
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;

	// DO NOT USE!: AND(XOR(A,B,C),XOR(D,E,F),XOR(AND(A,D),AND(B,E),AND(C,F)),Z):
	// rewrite to AND(XOR(AND(A,D),AND(B,E),AND(C,F)),Z) [this is an n-ary rule, and should
	// cope with partitions as well: but it is mathematically incorrect.  Try
	// [A B C D E F]:=[TRUE FALSE FALSE TRUE TRUE FALSE]: 1st fails, 2nd succeeds]
}

bool LHSUsesRHSAsArg(const MetaConcept& lhs, const MetaConcept& rhs)
{
	return static_cast<const MetaConceptWithArgArray&>(lhs).ThisConceptUsesNon1stArg(rhs);
}

bool MetaConnective::VertexDiagnoseIntermediateRulesANDAux() const
{	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	// 3+ary case
	// NOTE: ==, anti-idempotent pairs have already been cleaned up.  Also,
	// AND-objects do not contain quantifiers.

	// Viewpoint scan approach.  This is amenable to functionalizing (which makes this code 
	// usable in other contexts, usually relating to imminent deletion of target
	// NOTE: this code needs two forms: one for imminent deletion, one for use here
	// #1: if Arg N is == to a subargument elsewhere, replace it with TRUE; anti-idempotent
	// is replaced with FALSE
	if (FindArgRelatedToLHSViewPoint(InferenceParameter1,DeepLogicallyImplies))
		{
		IdxCurrentSelfEvalRule = LogicalANDReplaceThisArgWithTRUE_SER;
		return true;
		}

	if (ArgArray[InferenceParameter1]->IsSymmetricTransitive())
		{
		InferenceParameter2 = InferenceParameter1;
		while(   fast_size()>++InferenceParameter2
			  && ArgArray[InferenceParameter1]->IsExactType(ArgArray[InferenceParameter2]->ExactType()))
			if (static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])))
				{
				IdxCurrentSelfEvalRule = LogicalANDSpliceNAryEqualArg_SER;
				return true;
				}
// next block is general-case only (will have to recode)
#if 0
		while(0<InferenceParameter2)
			{
			InferenceParameter2--;
			if (   ArgArray[InferenceParameter1]->IsExactType(ArgArray[Idx2]->ExactType())
				&& static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[Idx2])))
				{
				IdxCurrentSelfEvalRule = LogicalANDSpliceNAryEqualArg_SER;
				return;
				}
			}
#endif
		}
	
	if (ArgArray[InferenceParameter1]->LogicalANDNonTrivialFindDetailedRule())
		{
		InferenceParameter2 = fast_size();
		while(--InferenceParameter2>InferenceParameter1)
			if (ArgArray[InferenceParameter1]->LogicalANDFindDetailedRule(*ArgArray[InferenceParameter2],InferenceParameter1,InferenceParameter2,InferenceParameter1,InferenceParameter2,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
				return true;
// next block is general-case only
#if 0
		while(0<InferenceParameter2)
			{
			InferenceParameter2--;
			if (ArgArray[InferenceParameter1]->LogicalANDFindDetailedRule(*ArgArray[InferenceParameter2],InferenceParameter1,InferenceParameter2,InferenceParameter1,InferenceParameter2,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
				return true;
			}
#endif
		}

	// an IFF constraint allows replacement of any arg by any other arg.
	// For now, restrict attention to the case where the 1st arg of the IFF is a variable.
	// we then scan the *other* args
	if 		(ArgArray[InferenceParameter1]->IsExactType(LogicalIFF_MC))
		{
		if (static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[0]->IsExactType(Variable_MC))
			{	// check to see if any other args are used.  If so, substitute them out.
			if (FindArgRelatedToLHSViewPoint(InferenceParameter1,LHSUsesRHSAsArg))
				{
				IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
				return true;
				}
			}
		}
	// an ALLEQUAL clause allows replacement of any arg in the clause with any
	// other arg in the clause.  A constant arg should be favored; after that, use lexical
	// order.
	else if (ArgArray[InferenceParameter1]->IsExactType(ALLEQUAL_MC))
		{
		//! \todo Interface function MetaConcept::ExactTypeOrExplicitConstant(...)
		if (   ArgArray[InferenceParameter1]->ArgN(0)->IsExactType(Variable_MC)
			|| ArgArray[InferenceParameter1]->ArgN(0)->IsExplicitConstant())
			{
			if (FindArgRelatedToLHSViewPoint(InferenceParameter1,LHSUsesRHSAsArg))
				{
				IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
				return true;
				}
			}
		}
		// this one spots transitive-symmetric clauses and tries to splice them on equal args.

	//! \todo INTERFACE: MetaConcept routine: virtual size_t DynamicSizeOf(void) const
	//! This routine reports the RAM cost of the given MetaConcept, which *should* influence
	//! Franci's reasoning methods!  E.g., the IFF-constraint should select the least-intensive
	//! target to substitute for.

	// shallow StrictlyModify
	if (FindArgRelatedToLHSViewPoint(InferenceParameter1,::CanStrictlyModify))
		{
		IdxCurrentSelfEvalRule = SelfEvalStrictlyModify_SER;
		return true;
		}

	// deep StrictlyModify
	if (FindArgRelatedToLHSViewPoint(InferenceParameter1,CanDeepStrictlyModify))
		{
		IdxCurrentSelfEvalRule = LogicalANDStrictlyModify_SER;
		return true;
		}

	//! \todo handle OR(A,B),OR(C,B), A=>~C by removing A
	//! citation from LogicalANDNonTrivialFindDetailedRule [needs more power]
	//! OR-ish clause, so don't mirror in LogicalANDTargetArgCritical
	if (ArgArray[InferenceParameter1]->IsExactType(LogicalOR_MC))
		{
		InferenceParameter2 = fast_size();
		do	if (   InferenceParameter1!=--InferenceParameter2
				&& ArgArray[InferenceParameter2]->IsExactType(LogicalOR_MC)
				&& static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size()==static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->fast_size())
				{
				size_t SweepIdx = static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size();
				while(0<--SweepIdx && *static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[SweepIdx]==*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[SweepIdx]);
				// SweepIdx is now stalled on the first argument pair that isn't equal, downwards
				size_t Idx2 = 0;
				while(Idx2<SweepIdx && *static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[Idx2]==*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[Idx2]) Idx2++;
				// Idx2 is now stalled on the first argument pair that isn't equal, upwards
				size_t LHSTarget = 0;
				size_t RHSTarget = 0;
				if (SweepIdx==Idx2)
					{
					LHSTarget = Idx2;
					RHSTarget = Idx2;
					}
				else{
					if (*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[Idx2]==*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[Idx2+1])
						{
						LHSTarget = SweepIdx;
						RHSTarget = Idx2;
						while(++Idx2<SweepIdx)
							if (*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[Idx2]!=*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[Idx2+1])
								goto NextIteration;								
						}
					else if (*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[Idx2+1]==*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[Idx2])
						{
						LHSTarget = Idx2;
						RHSTarget = SweepIdx;
						while(++Idx2<SweepIdx)
							if (*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[Idx2+1]!=*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[Idx2])
								goto NextIteration;								
						}
					else
						continue;
					}
				// verify that LHSTarget, RHSTarget indexes satisfy trigger
				if (IsAntiIdempotentTo(*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[LHSTarget],*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[RHSTarget]))
					{
					LOG("Using these");
					LOG(*ArgArray[InferenceParameter1]);
					LOG(*ArgArray[InferenceParameter2]);
					LOG("to identify irrelevant argument");
					LOG(*ArgArray[InferenceParameter1]->ArgN(LHSTarget));
					InferenceParameter2 = LHSTarget;
					IdxCurrentSelfEvalRule = RemoveIrrelevantArgFromOR_SER;
					return true;
					};
				// verify that CanUse...MakeLHSImplyLogicalNotOfRHS doesn't work
				// This code can trip off when an IFF substitution can be applied.
				// Quirked, but the net effect seems to be memory-efficient.
				{
				size_t Idx3 = fast_size();
				do	if (   --Idx3!=InferenceParameter1
						&&   Idx3!=InferenceParameter2
						&&   ArgArray[Idx3]->CanMakeLHSImplyRHS()
						&&   ArgArray[Idx3]->MakesLHSImplyLogicalNOTOfRHS(*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[LHSTarget],*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[RHSTarget]))
						{
						LOG("Using these");
						LOG(*ArgArray[InferenceParameter1]);
						LOG(*ArgArray[InferenceParameter2]);
						LOG(*ArgArray[Idx3]);
						LOG("to identify irrelevant argument");
						LOG(*ArgArray[InferenceParameter1]->ArgN(LHSTarget));
						InferenceParameter2 = LHSTarget;
						IdxCurrentSelfEvalRule = RemoveIrrelevantArgFromOR_SER;
						return true;
						}
				while(0<Idx3);
				}
NextIteration:;
				}
		while(0<InferenceParameter2);
		}

	return false;
}

bool MetaConnective::DelegateEvaluate(MetaConcept*& dest)
{
	assert(MetaConceptWithArgArray::MaxEvalRuleIdx_ER<IdxCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxEvalRuleIdx_ER>=IdxCurrentEvalRule);
	return (this->*EvaluateRuleLookup[IdxCurrentEvalRule-(MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1)])(dest);
}

bool MetaConnective::DelegateSelfEvaluate()
{
	assert(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER<IdxCurrentSelfEvalRule);
	assert(MaxSelfEvalRuleIdx_SER+MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER>=IdxCurrentSelfEvalRule);
	return (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1)])();
}

const char* const SpliceTransSymmClauseEqualClause = "Splicing these clauses on equal clause:";

bool MetaConnective::LogicalANDAry2NArySpliceEqualArg(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/2000
	assert(!dest);
	assert(2==size());
	assert(IsExactType(LogicalAND_MC));
	LOG(SpliceTransSymmClauseEqualClause);
	LOG(*ArgArray[0]);
	LOG(*ArgArray[1]);

	const int SwapDefaults = static_cast<MetaConceptWithArgArray*>(ArgArray[0])->fast_size()
							<static_cast<MetaConceptWithArgArray*>(ArgArray[1])->fast_size();
	MetaConceptWithArgArray& Parameter1 = *static_cast<MetaConceptWithArgArray*>(ArgArray[(SwapDefaults) ? 1 : 0]);
	MetaConceptWithArgArray& Parameter2 = *static_cast<MetaConceptWithArgArray*>(ArgArray[(SwapDefaults) ? 0 : 1]);

	const size_t NewSlotsOrigin = Parameter1.fast_size();
	if (!Parameter1.InsertNSlotsAtV2(Parameter2.fast_size()-1,NewSlotsOrigin))
		return false;

	{
	size_t i = Parameter2.fast_size();
	MetaConcept* Tmp = NULL;
	while(Parameter2.ImageInferenceParameter1()<--i)
		{
		Parameter2.TransferOutAndNULL(i,Tmp);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i-1,Tmp);
		};
	while(0<i)
		{
		Parameter2.TransferOutAndNULL(--i,Tmp);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i,Tmp);
		};
	Parameter1.ForceCheckForEvaluation();
	}

	if (SwapDefaults)
		{	// Arg1 has the active data
		dest=ArgArray[1];
		ArgArray[1]=NULL;
		}
	else{	// Arg0 has the active data
		dest=ArgArray[0];
		ArgArray[0]=NULL;
		};
	assert(dest->SyntaxOK());
	return true;
}

const char* const SpliceALLEQUALClauseAddInvClause = "Splicing these ALLEQUAL clauses on additive-inverse clause:";

bool MetaConnective::LogicalANDAry2ALLEQUALSpliceAddInvArg(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/2000
	assert(!dest);
	assert(2==size());
	assert(IsExactType(LogicalAND_MC));
	assert(ArgArray[0]->IsExactType(ALLEQUAL_MC));
	assert(ArgArray[1]->IsExactType(ALLEQUAL_MC));
	LOG(SpliceALLEQUALClauseAddInvClause);
	LOG(*ArgArray[0]);
	LOG(*ArgArray[1]);

	const int SwapDefaults = static_cast<MetaConceptWithArgArray*>(ArgArray[0])->fast_size()
							<static_cast<MetaConceptWithArgArray*>(ArgArray[1])->fast_size();
	MetaConceptWithArgArray& Parameter1 = *static_cast<MetaConceptWithArgArray*>(ArgArray[(SwapDefaults) ? 1 : 0]);
	MetaConceptWithArgArray& Parameter2 = *static_cast<MetaConceptWithArgArray*>(ArgArray[(SwapDefaults) ? 0 : 1]);

	const size_t NewSlotsOrigin = Parameter1.fast_size();
	if (!Parameter1.InsertNSlotsAtV2(Parameter2.fast_size()-1,NewSlotsOrigin))
		return false;

	{
	size_t i = Parameter2.fast_size();
	MetaConcept* Tmp = NULL;
	while(Parameter2.ImageInferenceParameter1()<--i)
		{
		Parameter2.TransferOutAndNULL(i,Tmp);
		Tmp->SelfInverse(StdAddition_MC);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i-1,Tmp);
		};
	while(0<i)
		{
		Parameter2.TransferOutAndNULL(--i,Tmp);
		Tmp->SelfInverse(StdAddition_MC);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i,Tmp);
		};
	Parameter1.ForceCheckForEvaluation();
	}

	if (SwapDefaults)
		{	// Arg1 has the active data
		dest=ArgArray[1];
		ArgArray[1]=NULL;
		}
	else{	// Arg0 has the active data
		dest=ArgArray[0];
		ArgArray[0]=NULL;
		};
	assert(dest->SyntaxOK());
	return true;
}

const char* const SpliceIFFClauseAntiIdempotentClause = "Splicing these IFF clauses on anti-idempotent clause:";

//! \todo analyze MetaConnective::LogicalANDSpliceIFFAntiIdempotentArg for parallel IO code, and splice it out
bool MetaConnective::LogicalANDSpliceIFFAntiIdempotentArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/9/2003
	// InferenceParameter1, InferenceParameter2 point to IFF statements
	// InferenceParameter1 of the two args point to the anti-idempotent arg
	// AND(IFF(A,B),IFF(~A,C),D) |-> AND(IFF(A,B,~C),D)
	assert(ArgArray.size()>InferenceParameter1);
	assert(ArgArray.size()>InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(LogicalIFF_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalIFF_MC));
	LOG(SpliceIFFClauseAntiIdempotentClause);
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);

	if ( static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->fast_size()
		<static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->fast_size())
		{
		size_t Tmp = InferenceParameter2;
		InferenceParameter2 = InferenceParameter1;
		InferenceParameter1 = Tmp;
		};

	MetaConnective& Parameter1 = *static_cast<MetaConnective*>(ArgArray[InferenceParameter1]);
	MetaConnective& Parameter2 = *static_cast<MetaConnective*>(ArgArray[InferenceParameter2]);
	const size_t NewSlotsOrigin = Parameter1.fast_size();

	if (!Parameter1.InsertNSlotsAtV2(Parameter2.fast_size()-1,NewSlotsOrigin))
		return false;

	{
	size_t i = Parameter2.fast_size();
	MetaConcept* Tmp = NULL;
	while(Parameter2.InferenceParameter1< --i)
		{
		Parameter2.TransferOutAndNULL(i,Tmp);
		Tmp->SelfLogicalNOT();
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i-1,Tmp);
		};
	while(0<i)
		{
		Parameter2.TransferOutAndNULL(--i,Tmp);
		Tmp->SelfLogicalNOT();
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i,Tmp);
		};
	Parameter1.ForceCheckForEvaluation();
	}

	LOG("to");
	LOG(*ArgArray[InferenceParameter1]);	

	DeleteIdx(InferenceParameter2);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

//! \todo analyze MetaConceptWithArgArray::LogicalANDAry2NArySpliceEqualArg for parallel IO code, and splice it out
bool MetaConnective::LogicalANDAry2IFFSpliceAntiIdempotentArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/2000
//	InferenceParameter1 = 0;
//	InferenceParameter2 = 1;
	// InferenceParameter1, InferenceParameter2 point to IFF statements
	// InferenceParameter1 of the two args point to the anti-idempotent arg
	// AND(IFF(A,B),IFF(~A,C),D) |-> AND(IFF(A,B,~C),D)
	assert(ArgArray[0]->IsExactType(LogicalIFF_MC));
	assert(ArgArray[1]->IsExactType(LogicalIFF_MC));
	LOG(SpliceIFFClauseAntiIdempotentClause);
	LOG(*ArgArray[0]);
	LOG(*ArgArray[1]);

	const int SwapDefaults = static_cast<MetaConnective*>(ArgArray[0])->fast_size()
							<static_cast<MetaConnective*>(ArgArray[1])->fast_size();
	MetaConnective& Parameter1 = *static_cast<MetaConnective*>(ArgArray[SwapDefaults ? 1 : 0]);
	MetaConnective& Parameter2 = *static_cast<MetaConnective*>(ArgArray[SwapDefaults ? 0 : 1]);

	const size_t NewSlotsOrigin = Parameter1.fast_size();
	if (!Parameter1.InsertNSlotsAtV2(Parameter2.fast_size()-1,NewSlotsOrigin))
		return false;

	{
	size_t i = Parameter2.fast_size();
	MetaConcept* Tmp = NULL;
	while(Parameter2.ImageInferenceParameter1()< --i)
		{
		Parameter2.TransferOutAndNULL(i,Tmp);
		Tmp->SelfLogicalNOT();
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i-1,Tmp);
		};
	while(0<i)
		{
		Parameter2.TransferOutAndNULL(--i,Tmp);
		Tmp->SelfLogicalNOT();
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i,Tmp);
		};
	}

	// InferenceParameter1 indexes the "live" IFF clause.  Copy it to the main clause.
	{
	MetaConcept** tmp = NULL;
	Parameter1.OverwriteAndNULL(tmp);
	ArgArray = tmp;
	}
	SetExactTypeV2(LogicalIFF_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::toNOR()
{	// FORMALLY CORRECT: Kenneth Boyd, 12/12/1999
	SetNANDNOR(LogicalNOR_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::Ary2IFFToOR()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// InferenceParameter1 points to A
	// InferenceParameter2 points to OR(A,....); A is pointed to by InferenceParameter1 of OR
	// want OR(A,~OR(A,....)) without A in negated OR
	// Dual version: ~A, AND(~A,...) maps to A, OR(A,....)
	assert(IsExactType(LogicalIFF_MC));
	assert(2==size());
	assert(size()>InferenceParameter2);
	assert(size()>InferenceParameter1);
	assert(InferenceParameter1!=InferenceParameter2);
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalAND_MC) || ArgArray[InferenceParameter2]->IsExactType(LogicalOR_MC));
	
	LOG("Converting");
	LOG(*this);
	if (2==static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->fast_size())
		{
		MetaConcept* Tmp = NULL;
		static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->TransferOutAndNULL(1-static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->InferenceParameter1,Tmp);
		DELETE(ArgArray[InferenceParameter2]);
		ArgArray[InferenceParameter2]=Tmp;
		}
	else{
		static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->DeleteIdx(static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->InferenceParameter1);
		}
	// Normal version at this point: OR(A,OR(....)): want OR(A,AND(...))
	// flipped version at this point: OR(A,AND(...)): want OR(~A, AND(...))
	if (ArgArray[InferenceParameter2]->IsExactType(LogicalOR_MC))
		{
		ArgArray[InferenceParameter2]->SelfLogicalNOT();
		}
	else{
		ArgArray[InferenceParameter1]->SelfLogicalNOT();
		}
	SetExactTypeV2(LogicalOR_MC);
	LOG("to");
	LOG(*this);
	assert(SyntaxOK());
	return true;	
}

// IFF(AND(A,B),IFF(A,B)) |-> OR(A,B)
bool MetaConnective::Ary2IFFToORV2()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// InferenceParameter1 points to AND(A,B)
	// InferenceParameter2 points to IFF(A,B)
	// this rewrites to OR(A,B)
	assert(IsExactType(LogicalIFF_MC));
	assert(2==size());
	assert(size()>InferenceParameter2);
	assert(size()>InferenceParameter1);
	assert(InferenceParameter1!=InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(LogicalAND_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalIFF_MC));
	assert(2==ArgArray[InferenceParameter1]->size());
	assert(2==ArgArray[InferenceParameter2]->size());

	LOG("Converting");
	LOG(*this);
	{
	MetaConcept** tmp = NULL;
	static_cast<MetaConnective*>(ArgArray[0])->OverwriteAndNULL(tmp);
	ArgArray = tmp;
	}
	SetExactTypeV2(LogicalOR_MC);
	LOG("to");
	LOG(*this);
	assert(SyntaxOK());
	return true;	
}

bool MetaConnective::Ary2IFFToORV3()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// V1:
	// InferenceParameter1 points to AND(A,B)
	// InferenceParameter2 points to AND(A,B,C)
	// This rewrites to OR(~A,~B,C)
	// V2:
	// InferenceParameter1 points to AND(A,B)
	// InferenceParameter2 points to AND(A,B,C,D)
	// This rewrites to OR(~A,~B,AND(C,D))
	// V3:
	// InferenceParameter1 points to OR(A,B)
	// InferenceParameter2 points to OR(A,B,C)
	// This rewrites to OR(A,B,~C)
	// V4:
	// InferenceParameter1 points to OR(A,B)
	// InferenceParameter2 points to OR(A,B,C,D)
	// This rewrites to OR(A,B,AND(~C,~D))
	// in all cases, the arglist in InferenceParameter1 is a subvector of the arglist in 
	// InferenceParameter2
	assert(IsExactType(LogicalIFF_MC));
	assert(2==size());
	assert(size()>InferenceParameter2);
	assert(size()>InferenceParameter1);
	assert(InferenceParameter1!=InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(ArgArray[InferenceParameter2]->ExactType()));
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalAND_MC) || ArgArray[InferenceParameter2]->IsExactType(LogicalOR_MC));
	assert(ArgArray[InferenceParameter1]->size()<ArgArray[InferenceParameter2]->size());
	
	// this rewrites to OR(A,....,B)
	// InferenceParameter2 points to OR(A,....); A is pointed to by InferenceParameter1 of OR
	// want OR(A,~OR(A,....)) without A in negated OR
	LOG("Converting");
	LOG(*this);

	if (static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->fast_size()+1==static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->fast_size())
		{	// exactly one arg longer: can recycle InferenceParameter2 ArgArray
		const size_t InfParam1Arity = static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size();
		if (ArgArray[InferenceParameter2]->IsExactType(LogicalAND_MC))
			{
			size_t i = 0;
			do	if (*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[i]==*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[i])
					{
					static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[i]->SelfLogicalNOT();
					}
				else{
					while(++i<=InfParam1Arity)	// Works by precondition for this block
						static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[i]->SelfLogicalNOT();	
					}
			while(++i<InfParam1Arity);
			}
		else{
			size_t i = 0;
			while(*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[i]==*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[i] && ++i<InfParam1Arity);
			static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[i]->SelfLogicalNOT();
			}
		{
		MetaConcept** tmp = NULL;
		static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->OverwriteAndNULL(tmp);
		ArgArray = tmp;
		}
		}
	else{	// cannot recycle InferenceParameter2 argarray.
		if (!static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->InsertSlotAt(0,NULL))
			return false;

		size_t i = static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size();
		// FindArgRelatedToLHS works "top-down"; this order should minimize linear search time.
		//! \todo OPTIMIZE: a dedicated implementation would be O(n), rather than O(n^2), 
		// for == tests.
		do	{
			SUCCEED_OR_DIE(static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->FindArgRelatedToLHS(*static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[--i],AreSyntacticallyEqual));
			static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->DeleteIdx(static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->InferenceParameter1);
			}
		while(1<i);

		// really don't want to do the negation twice in the LogicalAND_MC case, but
		// this is thought to be dictated by RAM conservatism [we need to know if the 
		// allocation will work before we start to discard arguments].  O(n) redundancy in 
		// SelfLogicalNOT().
		ArgArray[InferenceParameter2]->SelfLogicalNOT();
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->TransferInAndOverwriteRaw(0,ArgArray[InferenceParameter2]);
		ArgArray[InferenceParameter2]=NULL;
		if (ArgArray[InferenceParameter1]->IsExactType(LogicalAND_MC))
			ArgArray[InferenceParameter1]->SelfLogicalNOT();

		{
		MetaConcept** tmp = NULL;
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->OverwriteAndNULL(tmp);
		ArgArray = tmp;
		}
		};

	SetExactTypeV2(LogicalOR_MC);
	LOG("to");
	LOG(*this);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::Ary2IFFToORSelfModify()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/13/2004
	// IFF(A,B), A=>B, ~A strictly modifies ~B: replace with OR(A,~B strictly modified by ~A)
	assert(IsExactType(LogicalIFF_MC));
	assert(2==size());
	assert(size()>InferenceParameter2);
	assert(size()>InferenceParameter1);
	assert(InferenceParameter1!=InferenceParameter2);
	LOG("Converting");
	LOG(*this);

	ArgArray[InferenceParameter1]->SelfLogicalNOT();
	ArgArray[InferenceParameter2]->SelfLogicalNOT();
	assert(ArgArray[InferenceParameter1]->CanStrictlyModify(*ArgArray[InferenceParameter2]));
	ArgArray[InferenceParameter1]->StrictlyModifies(ArgArray[InferenceParameter2]);
	ArgArray[InferenceParameter1]->SelfLogicalNOT();

	SetExactTypeV2(LogicalOR_MC);
	LOG("to");
	LOG(*this);
	assert(SyntaxOK());
	return true;	
}

bool MetaConnective::IFFSpawn2AryIFF()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This rewrites the IFF as an AND of two IFFs
	// IFF(Arg1,Arg2)
	// IFF(all args except arg2 of original)
	assert(IsExactType(LogicalIFF_MC));
	assert(2<size());
	LOG("Found it pragmatic to rewrite");
	LOG(*this);
	LOG("as");
	MetaConnective* TmpIFFTarget2Ary = NULL;
	MetaConcept* TmpIFFTargetNMinus1Ary = NULL;
	MetaConcept** NilArgArray = _new_buffer<MetaConcept*>(2);
	if (!NilArgArray) return false;
	try	{
		TmpIFFTargetNMinus1Ary = new MetaConnective(IFF_MCM);
		TmpIFFTarget2Ary = new MetaConnective(IFF_MCM);
		TmpIFFTarget2Ary->insertNSlotsAt(2,0);
		ArgArray[InferenceParameter1]->CopyInto(TmpIFFTarget2Ary->ArgArray[0]);
		}
	catch(const bad_alloc&)
		{
		delete TmpIFFTarget2Ary;
		delete TmpIFFTargetNMinus1Ary;
		free(NilArgArray);
		return false;
		};
	TransferOutAndNULL(InferenceParameter2,TmpIFFTarget2Ary->ArgArray[1]);
	DeleteIdx(InferenceParameter2);
	TmpIFFTarget2Ary->ForceCheckForEvaluation();
	NilArgArray[0] = TmpIFFTarget2Ary;
	IdxCurrentSelfEvalRule = None_SER;	
	MoveInto(TmpIFFTargetNMinus1Ary);
	NilArgArray[1] = TmpIFFTargetNMinus1Ary;
	ArgArray = NilArgArray;
	SetExactTypeV2(LogicalAND_MC);
	LOG(*this);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::LogicalANDReplaceTheseNAry2ORWithNIFF()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/1999
	// arity>2
	// InferenceParameter1 points to OR(A,B,...)
	// InferenceParameter2 points to OR(~A,~B,...)
	// we wish to replace these with NIFF(A,B,...) [2-ary: IFF(A,~B)
	assert(IsExactType(LogicalAND_MC));
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(LogicalOR_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalOR_MC));
	assert(InferenceParameter1!=InferenceParameter2);
	LOG("Replacing AND of these:");
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);
	LOG("with:");
	InferenceParameter1 -= InferenceParameter1>InferenceParameter2; 
	DeleteIdx(InferenceParameter2);
	if (2==static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size())
		{
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[1]->SelfLogicalNOT();
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->SetExactType(LogicalIFF_MC);
		}
	else
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->SetExactType(LogicalNIFF_MC);
	static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ForceCheckForEvaluation();
	LOG(*ArgArray[InferenceParameter1]);
	if (1==size())
		{	// was 2-ary, now 1-ary
		const ExactType_MC tmp_type = ArgArray[InferenceParameter1]->ExactType();
		MetaConcept** tmp = NULL;
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->OverwriteAndNULL(tmp);
		ArgArray = tmp;
		SetExactType(tmp_type);
		}
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConnective::RemoveIrrelevantArgFromOR()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// InferenceParameter1 contains target
	// InferenceParameter2 is index of irrelevant arg
	assert(ArgArray.size()>InferenceParameter1);
	assert(ArgArray[InferenceParameter1]->IsExactType(LogicalOR_MC));
	assert(ArgArray[InferenceParameter1]->size()>InferenceParameter2);
	MetaConnective& VR_target = *static_cast<MetaConnective*>(ArgArray[InferenceParameter1]);
	LOG("Blotting");
	LOG(*VR_target.ArgArray[InferenceParameter2]);
	LOG("from");
	LOG(VR_target);
	if (2==VR_target.fast_size())
		{
		MetaConcept* Tmp = NULL;
		VR_target.TransferOutAndNULL(1-InferenceParameter2,Tmp);
		delete ArgArray[InferenceParameter1];	// reference dies
		ArgArray[InferenceParameter1] = Tmp;
		}
	else
		VR_target.DeleteIdx(InferenceParameter2);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConnective::LogicalANDSpawnClauseForDetailedRule()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
//	InferenceParameterMC = SpeculativeTarget;	// target
//	InferenceParameter1 = Idx3;	// trigger
	assert(IsExactType(LogicalAND_MC));
	assert(ArgArray.size()>InferenceParameter1);
	assert(!InferenceParameterMC.empty());
	if (!InsertSlotAt(fast_size(),InferenceParameterMC)) return false;

	InferenceParameterMC.NULLPtr();
	SUCCEED_OR_DIE(ArgArray[fast_size()-1]->LogicalANDFindDetailedRule(*ArgArray[InferenceParameter1],fast_size()-1,InferenceParameter1,InferenceParameter1,InferenceParameter2,IdxCurrentSelfEvalRule,IdxCurrentEvalRule));
	if (CanEvaluateToSameType()) return DestructiveEvaluateToSameType();
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::IFF_ReplaceANDORWithOwnArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	assert(IsExactType(LogicalAND_MC));
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(InferenceParameter2!=InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(LogicalAND_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalOR_MC));
	assert(ArgArray[InferenceParameter1]->size()==ArgArray[InferenceParameter2]->size());
	// InferenceParameter1: AND
	// InferenceParameter2: OR
	// they have the same args.
	LOG("Using");
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);
	LOG("to rewrite ");
	LOG(*this);
	if (   2<static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size()
		&& !InsertNSlotsAtV2(static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size(),InferenceParameter2+1))
		return false;

	size_t i = static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size()-1;
	do	static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->TransferOutAndNULL(i,ArgArray[InferenceParameter2+i-1]);
	while(0<--i);
	MetaConcept* Tmp = NULL;
	static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->TransferOutAndNULL(0,Tmp);
	delete ArgArray[InferenceParameter1];
	ArgArray[InferenceParameter1]=Tmp;
	LOG(" as ");
	LOG(*this);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConnective::LogicalANDStrictlyModify()
{	// FORMALLY CORRECT: 2/1/2003
	// InferenceParameter1 is the Idx of the target arg
	// *other* instances are to be replaced
	assert(IsExactType(LogicalAND_MC));
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(InferenceParameter1!=InferenceParameter2);
	LOG("(altering statements manipulated by this)");
	LOG(*ArgArray[InferenceParameter1]);
	SUCCEED_OR_DIE(ArgArray[InferenceParameter2]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],*ArgArray[InferenceParameter1],StrictlyModify,::CanStrictlyModify));
	if (0<InferenceParameter2)
		{
		size_t i = InferenceParameter2;
		if (i>InferenceParameter1)
			while(--i>InferenceParameter1)
				if (!ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],*ArgArray[InferenceParameter1],StrictlyModify,::CanStrictlyModify))
					return false;
		while(0<i)
			if (!ArgArray[--i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],*ArgArray[InferenceParameter1],StrictlyModify,::CanStrictlyModify))
				return false;
		};
	
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConnective::LogicalANDStrictlyImpliesClean()
{	// FORMALLY CORRECT: 11/17/2001
	// InferenceParameter1, InferenceParameter2 initialized by the FindTwoRelatedArgs
	// clause that triggered this
	// trigger line
//	if (DualFindTwoRelatedArgs(::StrictlyImplies) || FindTwoRelatedArgs(::StrictlyImplies))
//	NOTE: Dual form forces InferenceParameter1<InferenceParameter2
//  NOTE: standard form forces InferenceParameter1>InferenceParameter2
//! \todo FIX: in general, we have theoretical difficulties with A=>B(blot)=>C.  This requires
//!  Digraph to address properly, or a spot-diagnosis check
	assert(IsExactType(LogicalAND_MC));
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(InferenceParameter1!=InferenceParameter2);
	LOG("====");
Restart:
	LOG("Using [in AND clause]");
	LOG(*ArgArray[InferenceParameter1]);
	LOG(msz_SyntaxImplies);
	LOG(*ArgArray[InferenceParameter2]);

	// AND(A,B,...): Delete B: scan down to clean others using A
	DeleteIdx(InferenceParameter2);
	if (2==fast_size()) goto FinalExit;
	// ArgArray[InferenceParameter1] points to dominator.
	if (InferenceParameter1<InferenceParameter2)
		{	// dual tripped this off; resume pattern search
		size_t i = fast_size();
		while(InferenceParameter2<= --i)
			if (ArgArray[InferenceParameter1]->StrictlyImplies(*ArgArray[i]))
				{
				if (LogicalANDTargetArgCritical(i))
					goto FinalExit2;
				LOG(*ArgArray[i]);
				DeleteIdx(i);
				if (2==fast_size()) goto FinalExit;
				};
		if (0<InferenceParameter1)
			{
			i = InferenceParameter1;
			do	if (ArgArray[InferenceParameter1]->StrictlyImplies(*ArgArray[--i]))
					{
					if (LogicalANDTargetArgCritical(i))
						goto FinalExit2;
					LOG(*ArgArray[i]);
					DeleteIdx(i);
					if (2==fast_size()) goto FinalExit;
					InferenceParameter1--;
					InferenceParameter2--;
					}
			while(0<i);
			};
		if (    DualFindTwoRelatedArgs(::StrictlyImplies,InferenceParameter2)
			||  FindTwoRelatedArgs(::StrictlyImplies))
			{
			if (LogicalANDTargetArgCritical(InferenceParameter2))
				goto FinalExit2;
			goto Restart;
			}
		goto FinalExit;
		}
	else{	// standard tripped this off; resume pattern search
		// ALL implies tests with InferenceParameter1<InferenceParameter2 have been conducted.
		if (0<InferenceParameter2)
			{
			size_t i = InferenceParameter2;
			--InferenceParameter1;
			do	if (ArgArray[InferenceParameter1]->StrictlyImplies(*ArgArray[--i]))
					{
					if (LogicalANDTargetArgCritical(i))
						goto FinalExit2;
					LOG(*ArgArray[i]);
					DeleteIdx(i);
					if (2==fast_size()) goto FinalExit;
					InferenceParameter1--;
					}
			while(0<i);
			};
		if (FindTwoRelatedArgs(::StrictlyImplies,InferenceParameter1+1))
			{
			if (LogicalANDTargetArgCritical(InferenceParameter2))
				goto FinalExit2;
			goto Restart;
			}
		};
FinalExit:
	IdxCurrentSelfEvalRule = None_SER;	
FinalExit2:
	LOG("====");
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::VirtualDeepStrictlyModify()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2003
	LOG("Altering statements manipulated by this");
	LOG(*InferenceParameterMC);
	LOG("in");
	LOG(*ArgArray[InferenceParameter1]);

	SUCCEED_OR_DIE(ArgArray[InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,*InferenceParameterMC,StrictlyModify,::CanStrictlyModify));
	while(0<InferenceParameter1)
		if (!ArgArray[--InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,*InferenceParameterMC,StrictlyModify,::CanStrictlyModify))
			return false;
	InferenceParameterMC.reset();
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

static void CleanUpANDIFFAfterHyperNonStrictlyImpliesReplacement(MetaConcept*& Target, const MetaConcept& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if (    2==Target->size()
		&& (Target->IsExactType(LogicalAND_MC) || Target->IsExactType(LogicalIFF_MC)))
		{
		MetaConcept* Tmp2 = NULL;
		if      (rhs==*Target->ArgN(1))
			static_cast<MetaConnective*>(Target)->TransferOutAndNULL(0,Tmp2);
		else if (rhs==*Target->ArgN(0))
			static_cast<MetaConnective*>(Target)->TransferOutAndNULL(1,Tmp2);
		if (Tmp2)
			{
			delete Target;
			Target = Tmp2;
			}
		}
}

static void CleanUpORIFFAfterHyperNonStrictlyImpliesLogicalNOTOfReplacement(MetaConcept*& Target, const MetaConcept& rhs)
{
	if (   2==Target->size()
		&& (Target->IsExactType(LogicalOR_MC) || Target->IsExactType(LogicalIFF_MC)))
		{
		MetaConcept* Tmp2 = NULL;
		if      (IsAntiIdempotentTo(rhs,*Target->ArgN(1)))
			static_cast<MetaConnective*>(Target)->TransferOutAndNULL(0,Tmp2);
		else if (IsAntiIdempotentTo(rhs,*Target->ArgN(0)))
			static_cast<MetaConnective*>(Target)->TransferOutAndNULL(1,Tmp2);
		if (Tmp2)
			{
			if (Target->IsExactType(LogicalIFF_MC)) Tmp2->SelfLogicalNOT();
			delete Target;
			Target = Tmp2;
			}
		}
}

bool MetaConnective::VirtualDeepLogicallyImplies()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2003
	LOG("Replacing statements implied by this");
	LOG(*InferenceParameterMC);
	LOG("in");
	LOG(*ArgArray[InferenceParameter1]);
	TruthValue Tmp(true);

	// NOTE: for efficiency reasons, would be interested in something that didn't populate
	// with a lot of temporary TRUE/FALSE constants (effectively StrictlyModifies)
	// this would be a specialization of SetLHSToRHS, SetLHSToLogicalNOTOfRHS that relied 
	// on MetaConcept support [Direct]
	if (LogicalAND_MC<=ArgArray[InferenceParameter1]->ExactType() && LogicalNAND_MC>=ArgArray[InferenceParameter1]->ExactType())
		{
		if (!static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->HyperNonStrictlyImpliesReplacement(*InferenceParameterMC,Tmp))
			return false;
		CleanUpANDIFFAfterHyperNonStrictlyImpliesReplacement(ArgArray[InferenceParameter1],Tmp);
		if (LogicalAND_MC<=ArgArray[InferenceParameter1]->ExactType() && LogicalNAND_MC>=ArgArray[InferenceParameter1]->ExactType())
			{
			if (!static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->HyperNonStrictlyImpliesLogicalNOTOfReplacement(*InferenceParameterMC,Tmp))
				return false;
			CleanUpORIFFAfterHyperNonStrictlyImpliesLogicalNOTOfReplacement(ArgArray[InferenceParameter1],Tmp);
			}
		else{
			if (!ArgArray[InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,Tmp,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf))
				return false;
			}
		}
	else{
		if (   !ArgArray[InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,Tmp,SetLHSToRHS,::NonStrictlyImplies)
			|| !ArgArray[InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,Tmp,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf))
			return false;
		}

	while(0<InferenceParameter1)
		if (LogicalAND_MC<=ArgArray[--InferenceParameter1]->ExactType() && LogicalNAND_MC>=ArgArray[InferenceParameter1]->ExactType())
			{
			if (!static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->HyperNonStrictlyImpliesReplacement(*InferenceParameterMC,Tmp))
				return false;
			CleanUpANDIFFAfterHyperNonStrictlyImpliesReplacement(ArgArray[InferenceParameter1],Tmp);
			if (LogicalAND_MC<=ArgArray[InferenceParameter1]->ExactType() && LogicalNAND_MC>=ArgArray[InferenceParameter1]->ExactType())
				{
				if (!static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->HyperNonStrictlyImpliesLogicalNOTOfReplacement(*InferenceParameterMC,Tmp))
					return false;
				CleanUpORIFFAfterHyperNonStrictlyImpliesLogicalNOTOfReplacement(ArgArray[InferenceParameter1],Tmp);
				}
			else{
				if (!ArgArray[InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,Tmp,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf))
					return false;
				}
			}
		else{
			if (   !ArgArray[InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,Tmp,SetLHSToRHS,::NonStrictlyImplies)
				|| !ArgArray[InferenceParameter1]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*InferenceParameterMC,Tmp,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf))
				return false;
			};

	InferenceParameterMC.reset();
	// Anti-idempotency wins out:
	if (FindTwoAntiIdempotentArgsSymmetric())
		{
		IdxCurrentSelfEvalRule = None_SER;
		LOG("(Using anti-idempotent arguments)");
		LOG(*ArgArray[InferenceParameter1]);
		LOG(*ArgArray[InferenceParameter2]);
		LOG(*this);
		InvokeEvalForceContradiction();
		return true;
		};

	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConnective::LogicalORStrictlyImpliesClean()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/28/2000
	LOG(msz_Using);
	LOG(*ArgArray[InferenceParameter1]);
	LOG(msz_SyntaxImplies);
	LOG(*ArgArray[InferenceParameter2]);

	// OR(A,B,...): Delete A: scan down to clean others using B?
	// the find routines skim pairs s.t. the larger index is the largest index ever tested
	if (fast_size()>InferenceParameter1+1)
		{
		MetaConcept* const UseThisRHS = ArgArray[InferenceParameter2];
		InferenceParameter2 = InferenceParameter1;
		size_t i = 0;
		do	if (   i!=InferenceParameter2
				&& ArgArray[i]->StrictlyImplies(*UseThisRHS))
				{
				IdxCurrentSelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
				if (fast_size()-1!=InferenceParameter2)
					SwapArgs(InferenceParameter2,fast_size()-1);
				if (fast_size()-2!=i)
					SwapArgs(i,fast_size()-2);
				InferenceParameter1 = fast_size()-2;
				if (InferenceParameter2<i && InferenceParameter2<InferenceParameter1)
					while(   ArgArray[InferenceParameter2]->StrictlyImplies(*UseThisRHS)
						  && (LOG(*ArgArray[InferenceParameter2]),InferenceParameter2<--InferenceParameter1))
						SwapArgs(InferenceParameter2,InferenceParameter1);
				while(InferenceParameter1> ++i)
					if (   ArgArray[i]->StrictlyImplies(*UseThisRHS)
						&& --InferenceParameter1>i)
						SwapArgs(i--,InferenceParameter1);
				if (1==InferenceParameter1)
					InvokeEvalForceArg(0);
				return true;						
				}
		while(fast_size()> ++i);
		};
	IdxCurrentSelfEvalRule = SelfEvalRuleCleanArg_SER;
	assert(SyntaxOK());
	return true;
}

/*
XOR(L1,AND(L1,...),AND(L1,...))
|->
AND(L1,
	XOR(L1,AND(L1,...),AND(L1,...))
*/
bool MetaConnective::ExtractTrueArgFromXOR()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// InferenceParameter1: target
	autovalarray_ptr_throws<MetaConcept*> NewArgArray(2);
	autoval_ptr<MetaConcept> ReplaceParam1;

	ReplaceParam1 = new TruthValue(true);
	// no more throwing operations	
	{
	MetaConnective* const VR_arg1 = static_cast<MetaConnective*>(NewArgArray[1]);
	VR_arg1->TransferOutAndNULL(InferenceParameter1,NewArgArray[0]);
	VR_arg1->TransferInAndOverwriteRaw(InferenceParameter1,ReplaceParam1);
	VR_arg1->ForceCheckForEvaluation();
	}
	NewArgArray.MoveInto(ArgArray);
	SetExactTypeV2(LogicalAND_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::LogicalANDReplaceThisArgWithTRUE()
{	// FORMALLY CORRECT: 4/29/2002
	// InferenceParameter1 is the Idx of the target arg
	// *other* instances are to be replaced
	LOG("(replacing statements implied by this statement asserted in AND by TRUE or FALSE at a level below AND)");
	LOG(*ArgArray[InferenceParameter1]);
	LOG("starting with");
	LOG(*ArgArray[InferenceParameter2]);
	TruthValue Tmp(true);

	// NOTE: for efficiency reasons, would be interested in something that didn't populate
	// with a lot of temporary TRUE/FALSE constants (effectively StrictlyModifies)
	// this would be a specialization of SetLHSToRHS, SetLHSToLogicalNOTOfRHS that relied 
	// on MetaConcept support [Direct]
	if (LogicalAND_MC<=ArgArray[InferenceParameter2]->ExactType() && LogicalNAND_MC>=ArgArray[InferenceParameter2]->ExactType())
		{
		if (!static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->HyperNonStrictlyImpliesReplacement(*ArgArray[InferenceParameter1],Tmp))
			return false;

		CleanUpANDIFFAfterHyperNonStrictlyImpliesReplacement(ArgArray[InferenceParameter2],Tmp);
		if (LogicalAND_MC<=ArgArray[InferenceParameter2]->ExactType() && LogicalNAND_MC>=ArgArray[InferenceParameter2]->ExactType())
			{
			if (!static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->HyperNonStrictlyImpliesLogicalNOTOfReplacement(*ArgArray[InferenceParameter1],Tmp))
				return false;
			CleanUpORIFFAfterHyperNonStrictlyImpliesLogicalNOTOfReplacement(ArgArray[InferenceParameter2],Tmp);
			}
		else{
			if (!ArgArray[InferenceParameter2]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToLogicalNOTOfRHS,NonStrictlyImpliesLogicalNOTOf))
				return false;
			}
		}
	else{
		if (   !ArgArray[InferenceParameter2]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToRHS,NonStrictlyImplies)
			|| !ArgArray[InferenceParameter2]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToLogicalNOTOfRHS,NonStrictlyImpliesLogicalNOTOf))
			return false;
		}

	if (0<InferenceParameter2)
		{
		size_t i = InferenceParameter2;
		if (i>InferenceParameter1)
			while(--i>InferenceParameter1)
				if (LogicalAND_MC<=ArgArray[i]->ExactType() && LogicalNAND_MC>=ArgArray[i]->ExactType())
					{
					if (!static_cast<MetaConnective*>(ArgArray[i])->HyperNonStrictlyImpliesReplacement(*ArgArray[InferenceParameter1],Tmp))
						return false;
					CleanUpANDIFFAfterHyperNonStrictlyImpliesReplacement(ArgArray[i],Tmp);
					if (LogicalAND_MC<=ArgArray[i]->ExactType() && LogicalNAND_MC>=ArgArray[i]->ExactType())
						{
						if (!static_cast<MetaConnective*>(ArgArray[i])->HyperNonStrictlyImpliesLogicalNOTOfReplacement(*ArgArray[InferenceParameter1],Tmp))
							return false;
						CleanUpORIFFAfterHyperNonStrictlyImpliesLogicalNOTOfReplacement(ArgArray[i],Tmp);
						}
					else{
						if (!ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToLogicalNOTOfRHS,NonStrictlyImpliesLogicalNOTOf))
							return false;
						}
					}
				else{
					if (   !ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToRHS,NonStrictlyImplies)
						|| !ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToLogicalNOTOfRHS,NonStrictlyImpliesLogicalNOTOf))
						return false;
					}
		while(0<i)
			if (LogicalAND_MC<=ArgArray[--i]->ExactType() && LogicalNAND_MC>=ArgArray[i]->ExactType())
				{
				if (!static_cast<MetaConnective*>(ArgArray[i])->HyperNonStrictlyImpliesReplacement(*ArgArray[InferenceParameter1],Tmp))
					return false;
				CleanUpANDIFFAfterHyperNonStrictlyImpliesReplacement(ArgArray[i],Tmp);
				if (LogicalAND_MC<=ArgArray[i]->ExactType() && LogicalNAND_MC>=ArgArray[i]->ExactType())
					{
					if (!static_cast<MetaConnective*>(ArgArray[i])->HyperNonStrictlyImpliesLogicalNOTOfReplacement(*ArgArray[InferenceParameter1],Tmp))
						return false;
					CleanUpORIFFAfterHyperNonStrictlyImpliesLogicalNOTOfReplacement(ArgArray[i],Tmp);
					}
				else{
					if (!ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToLogicalNOTOfRHS,NonStrictlyImpliesLogicalNOTOf))
						return false;
					}
				}
			else{
				if (   !ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToRHS,NonStrictlyImplies)
					|| !ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToLogicalNOTOfRHS,NonStrictlyImpliesLogicalNOTOf))
					return false;
				}
		};

	// Anti-idempotency wins out:
	if (FindTwoAntiIdempotentArgsSymmetric())
		{
		IdxCurrentSelfEvalRule = None_SER;
		LOG("(Using anti-idempotent arguments)");
		LOG(*ArgArray[InferenceParameter1]);
		LOG(*ArgArray[InferenceParameter2]);
		LOG(*this);
		InvokeEvalForceContradiction();
		return true;
		};

	// 'memory' that another substitution is likely...but still be alert 
	// to total collapses [anti-idempotency, for now]
	// In general, interested in 'atomicity' of clauses
	while(++InferenceParameter1<fast_size() && ArgArray[InferenceParameter1]->IsExactType(Variable_MC))
		if (FindArgRelatedToLHSViewPoint(InferenceParameter1,DeepLogicallyImplies))
			return true;
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

/*
XOR(AND(L1,...),AND(L1,...),AND(L1,...))
|->
AND(L1,
    XOR(AND(L1,...),AND(L1,...),AND(L1,...))
*/
bool MetaConnective::LogicalXORExtractANDFactor()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// InferenceParameter1: argument providing the ANDFactor
	// SweepIdx: index of ANDFactor
	MetaConcept** TmpArgArray = _new_buffer<MetaConcept*>(2);
	if (!TmpArgArray) return false;

	// Try to do it the right way, first.  If that fails (RAM trouble), look for the tricky way.
	if (   !ArgArray[InferenceParameter1]->DirectCreateANDFactorIdx(InferenceParameter2,TmpArgArray[0])
		&&  ArgArray[InferenceParameter1]->IsExactType(LogicalAND_MC))
		{	// NOTE: the InferenceParameter2 AND-factor for an AND is simply the InferenceParameter2 arg.
		static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->TransferOutAndNULL(InferenceParameter2,TmpArgArray[0]);
		if (2==ArgArray[InferenceParameter1]->size())
			{
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->TransferOutAndNULL(1-InferenceParameter2,TmpArgArray[1]);
			FastTransferInAndOverwrite(InferenceParameter1,TmpArgArray[1]);
			TmpArgArray[1] = NULL;
			}
		else
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->DeleteIdx(InferenceParameter2);
		}

	if (TmpArgArray[0])
		{
		try	{
			MoveInto(TmpArgArray[1]);
			}
		catch(const bad_alloc&)
			{
			BLOCKDELETEARRAY(TmpArgArray);
			IdxCurrentSelfEvalRule = LogicalXORExtractANDFactor_SER;
			return false;	
			}		
		ArgArray = TmpArgArray;
		SetExactTypeV2(LogicalAND_MC);
		InferenceParameter1 = 0;

		LOG("Extracting");
		LOG(*ArgArray[0]);
		LOG("from");
		LOG(*ArgArray[1]);

		if (!VertexDiagnoseIntermediateRulesANDAux())
			IdxCurrentSelfEvalRule = None_SER;
		assert(SyntaxOK());
		return true;
		}
	BLOCKDELETEARRAY(TmpArgArray);
	return false;
}

bool MetaConnective::LogicalXORExtractIFFFactor()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// InferenceParameterMC contains the relevant Digraph object
	// #1) Isolate the relevant arglist for IFF
	Digraph* WorkingGraph = static_cast<Digraph*>((MetaConcept*)InferenceParameterMC);
	
	size_t MaxAmbiguousCount = 0;	// controls arity of extracted IFF
	size_t MaxAmbiguousCountIdx = 0;	// controls arity of extracted IFF
	size_t i = InferenceParameterMC->size();
	do	{
		size_t AmbiguousCount = WorkingGraph->VertexAmbiguousToEdgeCount(--i);
		if 		(AmbiguousCount>MaxAmbiguousCount)
			{
			MaxAmbiguousCount = AmbiguousCount;
			MaxAmbiguousCountIdx = i;
			while(i+1<InferenceParameterMC->size())
				WorkingGraph->RemoveVertex(InferenceParameterMC->size()-1);
			}
		else if (AmbiguousCount<MaxAmbiguousCount)
			{	// don't need this
			WorkingGraph->RemoveVertex(i);
			if (MaxAmbiguousCountIdx>i) MaxAmbiguousCountIdx--;
			}
		}
	while(0<i);
		
	// if this goes off, we need to upgrade this algorithm
	SUCCEED_OR_DIE(InferenceParameterMC->size()<MaxAmbiguousCount+1);
	if (InferenceParameterMC->size()>MaxAmbiguousCount+1)
		{
		i = 1;
		do	if (WorkingGraph->ExplicitNoEdge(0,i))
				{
				WorkingGraph->RemoveVertex(i);
				if (InferenceParameterMC->size()<=MaxAmbiguousCount+1) break;
				}
			else
				++i;
		while(i<InferenceParameterMC->size());
		}

	MetaConcept** TmpArgArray = _new_buffer<MetaConcept*>(2);
	if (!TmpArgArray) return false;

	// #2) construct the IFF in TmpArgArray[0]
	MetaConcept** NewArgArray = NULL;
	WorkingGraph->OverwriteAndNULL(NewArgArray);
	try	{
		TmpArgArray[0] = new MetaConnective(NewArgArray,IFF_MCM);
		}
	catch(const bad_alloc&)
		{
		WorkingGraph->ReplaceArgArray(NewArgArray);
		BLOCKDELETEARRAY(TmpArgArray);
		return false;			
		}
	
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
	try	{
		MoveInto(TmpArgArray[1]);
		}
	catch(const bad_alloc&)
		{
		MetaConcept** tmp = NULL;
		static_cast<MetaConceptWithArgArray*>(TmpArgArray[0])->OverwriteAndNULL(tmp);
		WorkingGraph->ReplaceArgArray(tmp);
		BLOCKDELETEARRAY(TmpArgArray);
		IdxCurrentSelfEvalRule = LogicalXORExtractIFFFactor_SER;
		return false;	
		}
	InferenceParameterMC.reset();
	ArgArray = TmpArgArray;
	SetExactTypeV2(LogicalAND_MC);
	IdxCurrentSelfEvalRule = None_SER;
	InferenceParameter1 = 0;

	LOG("Extracting");
	LOG(*ArgArray[0]);
	LOG("from");
	LOG(*ArgArray[1]);

	if (!static_cast<MetaConnective*>(this)->VertexDiagnoseIntermediateRulesANDAux())
		IdxCurrentSelfEvalRule = None_SER;
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::LogicalANDReplaceORAndXORWithXORAndNORArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 9/27/2002
	// InferenceParameter1 points to OR
	// InferenceParameter2 points to XOR
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(LogicalOR_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalXOR_MC));
	LOG("Replacing these:");
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);
	LOG("with:");
	// NOTE: START Bugfix here
	// first arg not-matched in XOR is pointed to in ArgArray[InferenceParameter2]->InferenceParameter1.
	// remove all matched-args, then convert XOR to NOR and DeMorgan it
	{
	size_t FirstNonMatchIdx = static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->InferenceParameter1;
	if (0<FirstNonMatchIdx)
		static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->DeleteNSlotsAt(FirstNonMatchIdx,0);
	if (static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size()>FirstNonMatchIdx)
		{
		size_t OffsetIdx = 1;
		do	{
			while(  *static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[FirstNonMatchIdx]
				  !=*static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->ArgArray[OffsetIdx]) OffsetIdx++;
			static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->DeleteIdx(OffsetIdx);
			}
		while(static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size()>++FirstNonMatchIdx);
		}
	}
	static_cast<MetaConnective*>(ArgArray[InferenceParameter2])->SetNANDNOR(LogicalNOR_MC);
	
	// Convert OR to XOR.  If arity-2, fix it to IFF
	if (2==static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size())
		{
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[1]->SelfLogicalNOT();
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->SetExactType(LogicalIFF_MC);
		}
	else
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->SetExactType(LogicalXOR_MC);
	static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ForceCheckForEvaluation();

	// NOTE: END Bugfix here
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);

	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConnective::ConvertToNANDOtherArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/15/1999
	DeleteIdx(InferenceParameter1);
	SetNANDNOR(LogicalNAND_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::ConvertToNOROtherArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/15/1999
	DeleteIdx(InferenceParameter1);
	SetNANDNOR(LogicalNOR_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::ReplaceArgsWithTrue()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2002
	// NOTE: MISNAMED
	// This is called only for XOR and NXOR, in response to a pair of args 
	// that are anti-idempotent.
	// 3-ary: missed arg in XOR is false; missed arg in NXOR is true [special case intercepted]
	// 4+-ary: missed args in XOR are NOR; missed args in NXOR are OR
	assert(IsExactType(LogicalXOR_MC) || IsExactType(LogicalNXOR_MC));
	DELETE_AND_NULL(ArgArray[InferenceParameter1]);
	DELETE_AND_NULL(ArgArray[InferenceParameter2]);
	FlushNULLFromArray((MetaConcept**&)ArgArray,(InferenceParameter1<InferenceParameter2) ? InferenceParameter1 : InferenceParameter2);
	if (IsExactType(LogicalXOR_MC))
		// XOR:
		SetNANDNOR(LogicalNOR_MC);	
	else	// NXOR:
		SetExactTypeV2(LogicalOR_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::TargetVariableFalse()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/12/2004
	// This is called from XOR
	// result: ~A AND XOR(...) [XOR(A,A,B): ~A AND B]
	assert(IsExactType(LogicalXOR_MC));
	MetaConnective* TmpXOR = new(nothrow) MetaConnective((3==fast_size()) ? IFF_MCM : XOR_MCM);
	if (!TmpXOR) return false;
	if (!TmpXOR->InsertNSlotsAtV2(2,0))
		{
		delete TmpXOR;
		return false;
		};

	TransferOutAndNULL(InferenceParameter1,TmpXOR->ArgArray[0]);
	TmpXOR->ArgArray[0]->SelfLogicalNOT();
	DeleteIdx(InferenceParameter1);
	if (2==fast_size()) ArgArray[1]->SelfLogicalNOT();
	{
	TruthValue Tmp(true);
	if (   !ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*TmpXOR->ArgArray[0],Tmp,SetLHSToRHS,::NonStrictlyImplies)
		|| !ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*TmpXOR->ArgArray[0],Tmp,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf))
		return false;
	}

	swap(ArgArray,TmpXOR->ArgArray);
	ArgArray[1] = TmpXOR;
	SetExactTypeV2(LogicalAND_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::TargetVariableTrue()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/12/2004
	// Called from NXOR for idempotent case
	// result: A OR NXOR(...) [NXOR(A,A,B): A OR ~B]
	assert(IsExactType(LogicalNXOR_MC));
	MetaConnective* TmpNXOR = new(nothrow) MetaConnective((3==fast_size()) ? IFF_MCM : NXOR_MCM);
	if (!TmpNXOR) return false;
	if (!TmpNXOR->InsertNSlotsAtV2(2,0))
		{
		delete TmpNXOR;
		return false;
		};

	TransferOutAndNULL(InferenceParameter1,TmpNXOR->ArgArray[0]);
	DeleteIdx(InferenceParameter1);
	// TmpNXOR arglist: A,NULL
	{
	TruthValue Tmp(false);
	if (   !ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*TmpNXOR->ArgArray[0],Tmp,SetLHSToRHS,::NonStrictlyImplies)
		|| !ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*TmpNXOR->ArgArray[0],Tmp,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf))
		return false;
	}

	swap(ArgArray,TmpNXOR->ArgArray);
	ArgArray[1] = TmpNXOR;	
	SetExactTypeV2(LogicalOR_MC);
	assert(SyntaxOK());
	return true;
}

//! \todo analyze MetaConnective::LogicalANDAry2MetaConnectiveSpliceEqualArg for parallel IO code, and splice it out
bool MetaConnective::LogicalANDAry2MetaConnectiveSpliceEqualArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/2000
//	InferenceParameter1 = 0;
//	InferenceParameter2 = 1;
	assert(IsExactType(LogicalAND_MC));
	assert(2==size());
	assert(typeid(MetaConnective)==typeid(*ArgArray[0]));
	assert(typeid(MetaConnective)==typeid(*ArgArray[1]));
	LOG(SpliceTransSymmClauseEqualClause);
	LOG(*ArgArray[0]);
	LOG(*ArgArray[1]);

	const int SwapDefaults = static_cast<MetaConnective*>(ArgArray[0])->fast_size()
							<static_cast<MetaConnective*>(ArgArray[1])->fast_size();
	MetaConnective& Parameter1 = *static_cast<MetaConnective*>(ArgArray[(SwapDefaults) ? 1 : 0]);
	MetaConnective& Parameter2 = *static_cast<MetaConnective*>(ArgArray[(SwapDefaults) ? 0 : 1]);

	const size_t NewSlotsOrigin = Parameter1.fast_size();
	if (!Parameter1.InsertNSlotsAtV2(Parameter2.fast_size()-1,NewSlotsOrigin))
		return false;

	{
	size_t i = Parameter2.fast_size();
	MetaConcept* Tmp = NULL;
	while(Parameter2.InferenceParameter1<--i)
		{
		Parameter2.TransferOutAndNULL(i,Tmp);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i-1,Tmp);
		};
	while(0<i)
		{
		Parameter2.TransferOutAndNULL(--i,Tmp);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i,Tmp);
		};
	}

	// InferenceParameter1 indexes the "live" IFF clause.  Copy it to the main clause.
	{
	MetaConcept** tmp = NULL;
	Parameter1.OverwriteAndNULL(tmp);
	ArgArray = tmp;
	}
	SetExactTypeV2(LogicalIFF_MC);
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::LogicalANDReplaceORAndXORWithXORAndNOTArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/2001
	// InferenceParameter1 points to OR
	// InferenceParameter2 points to XOR
	assert(IsExactType(LogicalAND_MC));
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(ArgArray[InferenceParameter1]->IsExactType(LogicalOR_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(LogicalXOR_MC));
	LOG("Replacing these:");
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);
	LOG("with:");
	// The one arg not-matched in XOR is pointed to by ArgArray[InferenceParameter2]->InferenceParameter1.
	// Extract it and self-logical-negate it.
	MetaConnective* Tmp = static_cast<MetaConnective*>(ArgArray[InferenceParameter2]);
	ArgArray[InferenceParameter2] = NULL;
	Tmp->TransferOutAndNULL(Tmp->InferenceParameter1,ArgArray[InferenceParameter2]);
	delete Tmp;
	ArgArray[InferenceParameter2]->SelfLogicalNOT();

	// Convert OR to XOR.  If arity-2, fix it to IFF
	if (2==static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->fast_size())
		{
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ArgArray[1]->SelfLogicalNOT();
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->SetExactType(LogicalIFF_MC);
		}
	else
		static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->SetExactType(LogicalXOR_MC);
	static_cast<MetaConnective*>(ArgArray[InferenceParameter1])->ForceCheckForEvaluation();

	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);
		
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConnective::AB_ToIFF_AnotB()
{	// FORMALLY CORRECT: Kenneth Boyd, 12/13/1999
	SetExactTypeV2(LogicalIFF_MC);
	ArgArray[1]->SelfLogicalNOT();
	assert(SyntaxOK());
	return true;
}

bool MetaConnective::LogicalANDTargetArgCritical(size_t TestIdx) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	assert(IsExactType(LogicalAND_MC));
	if (   ArgArray[TestIdx]->IsExactType(LogicalOR_MC)
		|| ArgArray[TestIdx]->IsExactType(EQUALTOONEOF_MC)
		|| ArgArray[TestIdx]->IsExactType(NOTALLEQUAL_MC)
		|| ArgArray[TestIdx]->IsExactType(NOTALLDISTINCT_MC))
		return false;	// OR-ish clauses always fail to process.
	//! \todo IMPLEMENT: MetaConcept's semantic knowledge should handle the type precondition

	// META: SYNC with VertexDiagnoseIntermediateRulesANDAux.  Note change in variable roles.
	// InferenceParameter1 reserved.
	// TestIdx contains "victim" argument that must do something or else.
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	// 3+ary case
	// NOTE: ==, anti-idempotent pairs have already been cleaned up.  Also,
	// AND-objects do not contain quantifiers.

	// Viewpoint scan approach.  This is amenable to functionalizing (which makes this code 
	// usable in other contexts, usually relating to imminent deletion of target
	// NOTE: this code needs two forms: one for imminent deletion, one for use here
	// #1: if Arg N is == to a subargument elsewhere, replace it with TRUE; anti-idempotent
	// is replaced with FALSE
	if (FindArgRelatedToLHSViewPoint(TestIdx,DeepLogicallyImplies))
		{
		IdxCurrentSelfEvalRule = LogicalANDReplaceThisArgWithTRUE_SER;
		return true;
		}
	
	// an IFF constraint allows replacement of any arg by any other arg.
	// For now, restrict attention to the case where the 1st arg of the IFF is a variable.
	// we then scan the *other* args
	if 		(ArgArray[TestIdx]->IsExactType(LogicalIFF_MC))
		{
		if (static_cast<MetaConnective*>(ArgArray[TestIdx])->ArgArray[0]->IsExactType(Variable_MC))
			{	// check to see if any other args are used.  If so, substitute them out.
			if (FindArgRelatedToLHSViewPoint(TestIdx,LHSUsesRHSAsArg))
				{
				IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
				return true;
				}
			}
		}
	// an ALLEQUAL clause allows replacement of any arg in the clause with any
	// other arg in the clause.  A constant arg should be favored; after that, use lexical
	// order.
	else if (ArgArray[TestIdx]->IsExactType(ALLEQUAL_MC))
		{
		if (   ArgArray[TestIdx]->ArgN(0)->IsExactType(Variable_MC)
			|| ArgArray[TestIdx]->ArgN(0)->IsExplicitConstant())
			{
			if (FindArgRelatedToLHSViewPoint(TestIdx,LHSUsesRHSAsArg))
				{
				IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
				return true;
				}
			}
		}
		// this one spots transitive-symmetric clauses and tries to splice them on equal args.
	if (ArgArray[TestIdx]->IsSymmetricTransitive())
		{
		size_t i = TestIdx;
		while(   fast_size()> ++i
			  && ArgArray[TestIdx]->IsExactType(ArgArray[i]->ExactType()))
			if (static_cast<MetaConceptWithArgArray*>(ArgArray[TestIdx])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[i])))
				{
				InferenceParameter1 = TestIdx;
				InferenceParameter2 = i;
				IdxCurrentSelfEvalRule = LogicalANDSpliceNAryEqualArg_SER;
				return true;
				}
		i = TestIdx;
		while(   0< --i
			  && ArgArray[TestIdx]->IsExactType(ArgArray[i]->ExactType()))
			if (static_cast<MetaConceptWithArgArray*>(ArgArray[TestIdx])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[i])))
				{
				InferenceParameter1 = TestIdx;
				InferenceParameter2 = i;
				IdxCurrentSelfEvalRule = LogicalANDSpliceNAryEqualArg_SER;
				return true;
				}
		}

	if (ArgArray[TestIdx]->LogicalANDNonTrivialFindDetailedRule())
		{
		size_t i = fast_size();
		while(--i>TestIdx)
			if (ArgArray[TestIdx]->LogicalANDFindDetailedRule(*ArgArray[i],TestIdx,i,InferenceParameter1,InferenceParameter2,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
				return true;
		while(0<i)
			{
			--i;
			if (ArgArray[TestIdx]->LogicalANDFindDetailedRule(*ArgArray[i],TestIdx,i,InferenceParameter1,InferenceParameter2,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
				return true;
			}
		}

	// shallow StrictlyModify
	if (FindArgRelatedToLHSViewPoint(TestIdx,::CanStrictlyModify))
		{
		IdxCurrentSelfEvalRule = SelfEvalStrictlyModify_SER;
		return true;
		}

	// deep StrictlyModify
	if (FindArgRelatedToLHSViewPoint(TestIdx,CanDeepStrictlyModify))
		{
		IdxCurrentSelfEvalRule = LogicalANDStrictlyModify_SER;
		return true;
		}

	return false;
}

bool MetaConnective::DiagnoseCommonIntermediateRulesORXORAux() const
{	//! \todo IMPLEMENT
	assert(2<=ArgArray.size());
	size_t MinANDFactorCount = ArgArray[fast_size()-1]->ANDFactorCount();
	if (0<MinANDFactorCount)
		{
		size_t SweepIdx = fast_size()-1;
		size_t MinANDFactorIdx = fast_size()-1;
		do	{
			size_t LocalANDFactorCount = ArgArray[--SweepIdx]->ANDFactorCount();
			if (LocalANDFactorCount<MinANDFactorCount)
				{
				MinANDFactorCount = LocalANDFactorCount;
				MinANDFactorIdx = SweepIdx;
				}
			// debug: OK here
			if (0==SweepIdx && 0<MinANDFactorCount)
				{	// ...
				// NOTE: failure to create this cache is non-lethal
				size_t* IFFCandidateArgs = _new_buffer<size_t>(MinANDFactorCount);
				if (NULL!=IFFCandidateArgs)
					{
					size_t CountIFFCandidateArgs = 0;
					do	{
// XOR(AND(L1,...),AND(L1,...),AND(L1,...))
// |->
// AND(L1,
//    XOR(AND(L1,...),AND(L1,...),AND(L1,...))
						MetaConcept* TmpArgPtr = NULL;
						if (ArgArray[MinANDFactorIdx]->DirectCreateANDFactorIdx(SweepIdx,TmpArgPtr))
							{
							size_t Idx2 = 0;
							while(Idx2==MinANDFactorIdx || ArgArray[Idx2]->StrictlyImplies(*TmpArgPtr))
								if (++Idx2==fast_size())
									{	// match!
									delete TmpArgPtr;
									free(IFFCandidateArgs);
									InferenceParameter1 = MinANDFactorIdx;
									InferenceParameter2 = SweepIdx;
									IdxCurrentSelfEvalRule = LogicalXORExtractANDFactor_SER;
									return true;
									};
							// stopped early...is it compatible for probing for IFF?
							if (   NULL!=IFFCandidateArgs
								&& StrictlyImpliesLogicalNOTOf(*ArgArray[Idx2],*TmpArgPtr))
								{
								while(++Idx2<fast_size()
									  && (   Idx2==MinANDFactorIdx
										  || ArgArray[Idx2]->StrictlyImplies(*TmpArgPtr)
										  || StrictlyImpliesLogicalNOTOf(*ArgArray[Idx2],*TmpArgPtr)));
								if (Idx2==fast_size())
									// ok, this arg can be used to probe for IFF extraction
									IFFCandidateArgs[CountIFFCandidateArgs++] = SweepIdx;
								}
							DELETE(TmpArgPtr);
							}
						}
					while(++SweepIdx<MinANDFactorCount);

					if (2<=CountIFFCandidateArgs)
						{	// It actually makes sense to look for an IFF
						MetaConcept** IFFCandidateArgList = _new_buffer<MetaConcept*>(CountIFFCandidateArgs);
						if (NULL!=IFFCandidateArgList)
							{
							SweepIdx = 0;
							do	if (!ArgArray[MinANDFactorIdx]->DirectCreateANDFactorIdx(IFFCandidateArgs[SweepIdx],IFFCandidateArgList[SweepIdx]))
									break;
							while(CountIFFCandidateArgs>++SweepIdx);

							if (2<=SweepIdx)
								{
								if (CountIFFCandidateArgs>SweepIdx)
									{
									// Stopped early, conserve
									IFFCandidateArgList = REALLOC(IFFCandidateArgList,sizeof(MetaConcept*)*SweepIdx);
									IFFCandidateArgs = REALLOC(IFFCandidateArgs,sizeof(MetaConcept*)*SweepIdx);
									}

								// We no longer need the storage for the dereferencing indexes.  However,
								// we *do* need storage for the cached results on 
								// StrictlyImplies/StrictlyImpliesLogicalNOTOf matches

								Digraph* IFFFinderGraph = NULL;
								PROPERLY_INIT_DIGRAPH(IFFFinderGraph,IFFCandidateArgList,true,NULL,DELETE,BLOCKDELETEARRAY,free(IFFCandidateArgs); return false);

								// IFFFinderGraph now owns IFFCandidateArgList, and has set the external
								// reference to NULL.  Graph is created with no known edges/not-edges
								IFFFinderGraph->ResetDiagonal();
								// isolate a possible IFF
								SweepIdx = fast_size();
								do	if (--SweepIdx!=MinANDFactorIdx)
										{
										size_t Idx2 = IFFFinderGraph->size();
										do	{
											const MetaConcept* const TmpArg = IFFFinderGraph->ArgN(--Idx2);
											if 		(ArgArray[SweepIdx]->StrictlyImplies(*TmpArg))
												{
												IFFCandidateArgs[Idx2] = 1;
												}
											else if (StrictlyImpliesLogicalNOTOf(*ArgArray[SweepIdx],*TmpArg))
												{
												IFFCandidateArgs[Idx2] = 0;
												}
											else{	// RAM failure
												delete IFFFinderGraph;
												free(IFFCandidateArgs);
												return false;
												}
											}
										while(0<Idx2);

										// Filter the graph
										// Any mismatch is a not-edge.
										Idx2 = IFFFinderGraph->size();
										do	{
											size_t Idx3 = --Idx2;
											do	{
												if (IFFCandidateArgs[--Idx3]!=IFFCandidateArgs[Idx2])
													IFFFinderGraph->ResetUndirectedEdge(Idx3,Idx2);
												}
											while(0<Idx3);
											}
										while(1<Idx2);

										// Any vertices that max out the not-edge count are deleted.
										// abort if reduced to 1 or 0 vertices
										Idx2 = IFFFinderGraph->size();
										do	if (0==IFFFinderGraph->VertexAmbiguousToEdgeCount(--Idx2))
												IFFFinderGraph->RemoveVertex(Idx2);
										while(0<Idx2);
										if (1>=IFFFinderGraph->size())
											SweepIdx=0;
										}
								while(0<SweepIdx);
								if (2<=IFFFinderGraph->size())
									{	// We have a legitimate IFF candidate, of arity whatever is here.
									free(IFFCandidateArgs);
									InferenceParameterMC = IFFFinderGraph;
									IdxCurrentSelfEvalRule = LogicalXORExtractIFFFactor_SER;
									return true;
									}
								delete IFFFinderGraph;
								//! \todo XOR-finder using Digraph
								}
							if (NULL!=IFFCandidateArgList)
								BLOCKDELETEARRAY(IFFCandidateArgList);
							}
						}
					free(IFFCandidateArgs);
					}
				break;
				}
			}
		while(0<MinANDFactorCount);
		}
/*
NOTE: ZaiBand_5Blocked_Stop.txt wants this

XOR(AND(L1,L2,R2),AND(L1,L2,NOT R2),AND(NOT L1,L2,R2),
         AND(NOT L1,NOT L2,R2))

To reduce to this in two stages
IFF(AND(L1,L2),OR(L1,NOT R2))

Idea: XOR(AND(...,R2),AND(...,NOT R2),...)
|->
XOR(AND(...),...)

when this all-args-equal-except-one-anti-idempotent match is unique for one of the two involved.
Use for both XOR and OR.

In basis terms (more complicated), all ANDFactors must be equal except for one anti-idempotent.
However, EqualRelation won't directly support that, so don't worry about it for now.

We also want:
XOR(AND(L1,L3,NOT R1),AND(L1,NOT L3,R1),AND(NOT L1,L3,R1))))
|->
XOR(NOT L1,NOT L3,NOT R1)

for both OR and XOR.  Again, EqualRelation won't support the ANDFactor form of this.

However, the EqualRelation limits are removed if we extended AND's definition of ANDFactor 
to recurse.  This change would invalidate the memory save in LogicalXORExtractANDFactor, and 
does not cooperate with with ALLEQUAL (including transitivity) or ALLDISTINCT.
*/
	if (   LogicalAND_MC>=ArgArray[0]->ExactType()
		&& LogicalAND_MC<=ArgArray[fast_size()-1]->ExactType())
		{
		DEBUG_LOG("more detailed AND scan");
		size_t LowBoundANDIdx = 0;
		while(LogicalAND_MC>ArgArray[LowBoundANDIdx]->ExactType())
			LowBoundANDIdx++;
		if (   ArgArray[LowBoundANDIdx]->IsExactType(LogicalAND_MC)
			&& LowBoundANDIdx+1<fast_size()
			&& ArgArray[LowBoundANDIdx+1]->IsExactType(LogicalAND_MC))
			{
			//! \todo this isn't immediately going to be multi-threaded.  Recode to minimize RAM loading.
			size_t HighBoundANDIdx = fast_size()-1;
			while(LogicalAND_MC>ArgArray[HighBoundANDIdx]->ExactType())
				HighBoundANDIdx--;

			// need two digraphs.  This probably could use multithreading, so set it up like
			// Franci's going to multithread.

			// AND(A,B,C), AND(A,B,~C) |-> AND(A,B)
			//! \todo setup hypercube code in AND for OR
			MetaConcept** NewArgArray1 = NULL;
			if (MirrorArgArrayNoOwnership(HighBoundANDIdx-LowBoundANDIdx,ArgArray+LowBoundANDIdx,
									NewArgArray1,2,NAryAllArgsEqualExceptOneAntiIdempotentPair))
				{
				Digraph* AntiIdempotentArgBlotter = NULL;
				PROPERLY_INIT_DIGRAPH(AntiIdempotentArgBlotter,NewArgArray1,false,&NAryAllArgsEqualExceptOneAntiIdempotentPair,DELETE,free,return false);

				AntiIdempotentArgBlotter->ResetDiagonal();
				const size_t HypercubeDimension = AntiIdempotentArgBlotter->FindStrictHypercubeGraphFlushIrrelevantVertices();
				if (0<HypercubeDimension)
					{
					InferenceParameter1 = HypercubeDimension;
					InferenceParameterMC = AntiIdempotentArgBlotter;
					IdxCurrentSelfEvalRule = LogicalORXORCompactANDArgHyperCube_SER;
					return true;
					};
				delete AntiIdempotentArgBlotter;
//				AntiIdempotentArgBlotter = NULL;
				}

#if 0
			if (MirrorArgArrayNoOwnership(HighBoundANDIdx-LowBoundANDIdx,ArgArray+LowBoundANDIdx,
									NewArgArray1,2,&NAryAllArgsEqualExceptOne))
				{
				Digraph* DifferentArgBlotter = NULL;
				PROPERLY_INIT_DIGRAPH(DifferentArgBlotter,NewArgArray1,false,&NAryAllArgsEqualExceptOne,DELETE,free,return false);

				DifferentArgBlotter->ResetDiagonal();
				const size_t HypercubeDimension = DifferentArgBlotter->FindStrictHypercubeGraphFlushIrrelevantVertices();
				if (0<HypercubeDimension)
					{
					InferenceParameter1 = HypercubeDimension;
					InferenceParameterMC = DifferentArgBlotter;
					IdxCurrentSelfEvalRule = LogicalANDORXORCondenseORANDANDArgHyperCube_SER;
					return true;				
					};
				delete DifferentArgBlotter;
//				DifferentArgBlotter = NULL;
				}
#endif

#if 0
			if (MirrorArgArrayNoOwnership(HighBoundANDIdx-LowBoundANDIdx,ArgArray+LowBoundANDIdx,
									NewArgArray1,2,&NAryAllArgsEqualExceptTwoAntiIdempotentPairs))
				{
				Digraph* XORIntegrator = NULL;
				PROPERLY_INIT_DIGRAPH(XORIntegrator,NewArgArray1,false,&NAryAllArgsEqualExceptTwoAntiIdempotentPairs,DELETE,free,return false);

				XORIntegrator->ResetDiagonal();
				const size_t CompleteGraphDimension = XORIntegrator->FindCompleteGraphFlushIrrelevantVertices();
				if (0<CompleteGraphDimension)
					{
					InferenceParameter1 = CompleteGraphDimension;
					InferenceParameterMC = XORIntegrator;
					//! \todo IMPLEMENT LogicalXORCompactANDArgsToXOR_SER
					IdxCurrentSelfEvalRule = LogicalXORCompactANDArgsToXOR_SER;
					return true;
					};
				delete XORIntegrator;
//				XORIntegrator = NULL;
				}
#endif
			}
		}
	return false;
}

void MetaConnective::DiagnoseIntermediateRulesORAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/5/1999
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalOR_MC));
	assert(2<size());
	// X EQUALTOONEOF ..., X EQUALTOONEOF ...: merge on arg0
	if (   EQUALTOONEOF_MC>=ArgArray[0]->ExactType()
		&& EQUALTOONEOF_MC<=ArgArray[fast_size()-1]->ExactType())
		{
		size_t lb = 0;
		while(fast_size()-2>lb && EQUALTOONEOF_MC>ArgArray[lb]->ExactType()) ++lb;
		if (   ArgArray[lb]->IsExactType(EQUALTOONEOF_MC)
			&& ArgArray[lb+1]->IsExactType(EQUALTOONEOF_MC))
			{
			size_t ub = lb+1;
			while(fast_size()-1>ub && ArgArray[ub+1]->IsExactType(EQUALTOONEOF_MC)) ++ub;
			size_t i = lb+1;
			do	{
				size_t j = i;
				const MetaConcept& TestLHS = *ArgArray[i]->ArgN(0);
				do	if (TestLHS==*ArgArray[--j]->ArgN(0))
						{
						IdxCurrentSelfEvalRule = LogicalANDSpliceNAryEqualArg_SER;
						SetArgNInfParam1(i,0);
						SetArgNInfParam1(j,0);
						InferenceParameter1 = i;
						InferenceParameter2 = j;
						return;
						}
				while(lb<j);
				}
			while(ub>= ++i);
			}
		}
	if (DiagnoseCommonIntermediateRulesORXORAux()) return;
	//! \todo MANY OTHER INFERENCE RULES
	//! \todo two more sophisticated rules are possible:
	//! 1) an OR-clause of two AND-terms, which have equal arity and which have
	//! an order s.t. the AND-clauses terms are all antiidempotent, is rewriteable as an
	//! IFF clause.
	//! 2) an OR-clause of n AND-clauses T1..Tn, all of which have arity n, s.t. an AND-term
	//! T-meta exists s.t. each T1..Tn differs from T-meta only in that term k of Tk is anti-idempotent
	//! to term k of T-meta (as opposed to syntactical equality) is rewriteable as a XOR clause.
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

void MetaConnective::DiagnoseIntermediateRulesIFFAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/14/1999
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalIFF_MC));
	assert(2<size());
	// IFF(AND(A,B),OR(A,B)) |-> IFF(A,B) [arglist replacement]
	// Also need 2-ary version [just in case]
	if (   LogicalOR_MC<=ArgArray[fast_size()-1]->ExactType()
		&& LogicalAND_MC>=ArgArray[0]->ExactType())
		{
		size_t OR_HighBound = fast_size();
		while(LogicalOR_MC<ArgArray[--OR_HighBound]->ExactType());
		if (ArgArray[OR_HighBound]->IsExactType(LogicalOR_MC))
			{
			size_t AND_LowBound = 0;
			while(LogicalAND_MC>ArgArray[AND_LowBound]->ExactType()) AND_LowBound++;
			while(ArgArray[AND_LowBound]->IsExactType(LogicalAND_MC))
				{
				size_t i = OR_HighBound;
				do	if (static_cast<MetaConnective*>(ArgArray[AND_LowBound])->ExactOrderPairwiseRelation(*static_cast<MetaConnective*>(ArgArray[i]),AreSyntacticallyEqual))
						{
						InferenceParameter1 = AND_LowBound;
						InferenceParameter2 = i;
						IdxCurrentSelfEvalRule = IFF_ReplaceANDORWithOwnArgs_SER;
						return;	
						}
				while(ArgArray[ --i]->IsExactType(LogicalOR_MC));
				AND_LowBound++;
				}
			}
		}
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

void MetaConnective::DiagnoseIntermediateRulesXORAux() const
{	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalXOR_MC));
	assert(2<size());

	if (DiagnoseCommonIntermediateRulesORXORAux()) return;
	//! \todo IMPLEMENT: XOR(AND(A,~B,~C),AND(~A,B,~C),AND(~A,~B,C)) |-> XOR(A,B,C)
	//! Probably should deal with "baggage" as factors.
	//! this is a SelfEval, regardless
#if 0
	if (   ArgArray[0]->IsExactType(LogicalAND_MC)
        && ArgArray[fast_size()-1]->IsExactType(LogicalAND_MC)
		&& static_cast<MetaConnective*>(ArgArray[0])->fast_size()>=fast_size())
		{	// check for whether each AND contains an arg that is anti-idempotent to 
			// an arg in all of the other ANDs.
		if (   fast_size()==ArgArray[0]->size()
			&& fast_size()==ArgArray[fast_size()-1]->size())
			{
			size_t* CandidateIndexesForXOR = _new_buffer<size_t>(fast_size());
			if (NULL!=CandidateIndexesForXOR)
				{	// pure XOR check
				size_t LocatedBound = 0;
				do	{	// find pairs of anti-idempotent args and proceed
					if (!static_cast<MetaConnective*>(ArgArray[LocatedBound])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[LocatedBound+1]),IsAntiIdempotentTo))
						goto XORANDDefFailed;
					size_t LHSTarget1 = static_cast<MetaConnective*>(ArgArray[LocatedBound])->InferenceParameter1;
					size_t RHSTarget1 = static_cast<MetaConnective*>(ArgArray[LocatedBound+1])->InferenceParameter1;
					if (!static_cast<MetaConnective*>(ArgArray[LocatedBound])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[LocatedBound+1]),IsAntiIdempotentTo,LHSTarget1,RHSTarget2))
						goto XORANDDefFailed;
					size_t LHSTarget2 = static_cast<MetaConnective*>(ArgArray[LocatedBound])->InferenceParameter1;
					size_t RHSTarget2 = static_cast<MetaConnective*>(ArgArray[LocatedBound+1])->InferenceParameter1;
					size_t SweepIdx = fast_size()-1;
					if      (static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound])->ArgArray[LHSTarget2],IsAntiIdempotentTo))
						{
						if (!static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound])->ArgArray[LHSTarget1],SyntacticallyEqual))
							goto XORANDDefFailed;
						size_t Tmp = LHSTarget2;
						LHSTarget2 = LHSTarget1;
						LHSTarget1 = Tmp;							
						}
					else if (   !static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound])->ArgArray[LHSTarget1],IsAntiIdempotentTo)
							 || !static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound])->ArgArray[LHSTarget2],SyntacticallyEqual))
						goto XORANDDefFailed;
					// LHSTarget1 now points to potential XOR target arg
					if      (static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound+1])->ArgArray[RHSTarget2],IsAntiIdempotentTo))
						{
						if (!static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound+1])->ArgArray[RHSTarget1],SyntacticallyEqual))
							goto XORANDDefFailed;
						size_t Tmp = RHSTarget2;
						RHSTarget2 = RHSTarget1;
						RHSTarget1 = Tmp;							
						}
					else if (   !static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound+1])->ArgArray[RHSTarget1],IsAntiIdempotentTo)
							 || !static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound+1])->ArgArray[RHSTarget2],SyntacticallyEqual))
						goto XORANDDefFailed;
					// RHSTarget1 now points to potential XOR target arg
					do	if (   --SweepIdx!=LocatedBound
							&&   SweepIdx!=LocatedBound+1)
							{
							if (   !static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound])->ArgArray[LHSTarget1],IsAntiIdempotentTo)
								|| !static_cast<MetaConnective*>(ArgArray[SweepIdx])->FindArgRelatedToLHS(static_cast<MetaConnective*>(ArgArray[LocatedBound+1])->ArgArray[RHSTarget1],IsAntiIdempotentTo))
								goto XORANDDefFailed;							
							}
					while(0<SweepIdx);
					CandidateIndexesForXOR[LocatedBound++]=LHSTarget1;
					CandidateIndexesForXOR[LocatedBound++]=RHSTarget1;
					}
				while(LocatedBound+1<fast_size());
				if (LocatedBound<fast_size())
					{	// one arg left to find
					size_t ArgIdx = ArgArray[LocatedBound]->size();
					do	if (*static_cast<MetaConnective*>(ArgArray[0])->FindArgRelatedToLHS(*static_cast<MetaConnective*>(ArgArray[LocatedBound])->ArgArray[--ArgIdx],IsAntiIdempotentTo))
							{
							size_t SweepIdx = LocatedBound;
							do	if (!*static_cast<MetaConnective*>(ArgArray[--SweepIdx])->FindArgRelatedToLHS(*static_cast<MetaConnective*>(ArgArray[0])->ArgArray[ArgIdx],IsAntiIdempotentTo))
									goto NextIteration2;
							while(1<SweepIdx);
							CandidateIndexesForXOR[LocatedBound++]=ArgIdx;
							goto XORANDDefSuccess;
NextIteration2:;
							}
					while(0<ArgIdx);
					goto XORANDDefFailed;
					};
XORANDDefSuccess:	// all-initialized: proceed to create XOR, possibly with augmented implications.
				InferenceParameter1 = CandidateIndexesForXOR;	// NOTE: dynamic retyping...affects destructor!
				IdxCurrentSelfEvalRule = XOROfANDDefinesXOR_SER;
				return;				
				};
XORANDDefFailed:
			DELETE_AND_NULL(CandidateIndexesForXOR);
			}
		// Distributivity of AND over XOR
		// related to: distributivity of AND over OR
		// related to: distributivity of OR over AND
		// actually, this entire loop is a function target
		size_t ArgIdx = ArgArray[0]->size();
		do	if (*static_cast<MetaConnective*>(ArgArray[1])->FindArgRelatedToLHS(*static_cast<MetaConnective*>(ArgArray[0])->ArgArray[--ArgIdx],SyntacticallyEqual))
				{
				size_t SweepIdx = fast_size();
				do	if (!*static_cast<MetaConnective*>(ArgArray[--SweepIdx])->FindArgRelatedToLHS(*static_cast<MetaConnective*>(ArgArray[0])->ArgArray[ArgIdx],SyntacticallyEqual))
						goto NextIteration;
				while(2<SweepIdx);
				// success!
				// ArgIdx points to Arg0 Idx to be factored.  InferenceParameter1 of other args points to same.
				static_cast<MetaConnective*>(ArgArray[0])->InferenceParameter1 = ArgIdx;
				IdxCurrentSelfEvalRule = DistributeLogicalArgThroughThisArg_SER;
				return;
NextIteration:;
				}
		while(0<ArgIdx);
		}
#endif
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

void MetaConnective::DiagnoseIntermediateRulesNIFFAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/14/1999
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalNIFF_MC));
	assert(2<size());
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

void MetaConnective::DiagnoseIntermediateRulesNXORAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/1999
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalNXOR_MC));
	assert(2<size());
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

void MetaConnective::DiagnoseIntermediateRulesAND2AryAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/31/2003
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalAND_MC));
	assert(2==size());

	// if Arg N is == to a subargument elsewhere, replace it with TRUE; anti-idempotent
	// is replaced with FALSE
	if (ArgArray[1]->HasArgRelatedToThisConceptBy(*ArgArray[0],NonStrictlyImpliesThisOrLogicalNOTOf))
		{
		InferenceParameter1 = 0;
		InferenceParameter2 = 1;
		IdxCurrentSelfEvalRule = LogicalANDReplaceThisArgWithTRUE_SER;
		return;
		};
	if (ArgArray[0]->HasArgRelatedToThisConceptBy(*ArgArray[1],NonStrictlyImpliesThisOrLogicalNOTOf))
		{
		InferenceParameter1 = 1;
		InferenceParameter2 = 0;
		IdxCurrentSelfEvalRule = LogicalANDReplaceThisArgWithTRUE_SER;
		return;
		};
	// an IFF constraint allows replacement of any arg by any other arg.
	// For now, restrict attention to the case where the 1st arg of the IFF is a variable.
	// we then scan the *other* args
	if (   ArgArray[0]->IsExactType(LogicalIFF_MC)
		&& ArgArray[0]->ArgN(0)->IsExactType(Variable_MC))
		{
		if (static_cast<MetaConceptWithArgArray*>(ArgArray[0])->ThisConceptUsesNon1stArg(*ArgArray[1]))
			{
			InferenceParameter1 = 0;
			InferenceParameter2 = 1;
			IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
			return;
			};
		};
	if (   ArgArray[1]->IsExactType(LogicalIFF_MC)
		&& ArgArray[1]->ArgN(0)->IsExactType(Variable_MC))
		{
		if (static_cast<MetaConceptWithArgArray*>(ArgArray[1])->ThisConceptUsesNon1stArg(*ArgArray[0]))
			{
			InferenceParameter1 = 1;
			InferenceParameter2 = 0;
			IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
			return;
			};
		};

	if (ArgArray[0]->IsExactType(ALLEQUAL_MC))
		{
		if (   ArgArray[0]->ArgN(0)->IsExactType(Variable_MC)
			|| ArgArray[0]->ArgN(0)->IsExplicitConstant())
			{	// check to see if any other args are used.  If so, substitute them out.
			if (static_cast<MetaConceptWithArgArray*>(ArgArray[0])->ThisConceptUsesNon1stArg(*ArgArray[1]))
				{
				InferenceParameter1 = 0;
				InferenceParameter2 = 1;
				IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
				return;
				}
			};
		};
	if (ArgArray[1]->IsExactType(ALLEQUAL_MC))
		{
		if (   ArgArray[1]->ArgN(0)->IsExactType(Variable_MC)
			|| ArgArray[1]->ArgN(0)->IsExplicitConstant())
			{	// check to see if any other args are used.  If so, substitute them out.
			if (static_cast<MetaConceptWithArgArray*>(ArgArray[1])->ThisConceptUsesNon1stArg(*ArgArray[0]))
				{
				InferenceParameter1 = 1;
				InferenceParameter2 = 0;
				IdxCurrentSelfEvalRule = ReplaceThisArgWithLeadArg_SER;
				return;
				}
			};
		};

	if (   ArgArray[0]->IsSymmetricTransitive()
		&& ArgArray[0]->IsExactType(ArgArray[1]->ExactType()))
		{	// actual pattern-finder code
		if (static_cast<MetaConnective*>(ArgArray[0])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[1])))
			{
			IdxCurrentEvalRule = LogicalANDAry2NArySpliceEqualArg_ER;
			return;
			};
		};

	if (   ArgArray[0]->LogicalANDNonTrivialFindDetailedRule()
		&& ArgArray[0]->LogicalANDFindDetailedRule(*ArgArray[1],0,1,InferenceParameter1,InferenceParameter2,IdxCurrentSelfEvalRule,IdxCurrentEvalRule))
		{
		if (LogicalANDReplaceORAndXORWithXORAndNORArg_SER>=IdxCurrentSelfEvalRule)
			return;

		if (SelfEvalRuleCleanArg_SER==IdxCurrentSelfEvalRule)
			{
			InvokeEvalForceArg(1-InferenceParameter1);
			return;
			};
		if (LogicalANDSpliceNAryEqualArg_SER==IdxCurrentSelfEvalRule)
			{
			if (typeid(MetaConnective)==typeid(*ArgArray[0]))
				{
				IdxCurrentSelfEvalRule=LogicalANDAry2MetaConnectiveSpliceEqualArg_SER;
				return;
				}
			else{
				IdxCurrentSelfEvalRule=None_SER;
				IdxCurrentEvalRule=LogicalANDAry2NArySpliceEqualArg_ER;
				return;
				};
			};
		if (LogicalANDSpliceIFFAntiIdempotentArg_SER==IdxCurrentSelfEvalRule)
			{
			IdxCurrentSelfEvalRule=LogicalANDAry2IFFSpliceAntiIdempotentArg_SER;
			return;
			};
		if (LogicalANDSpliceALLEQUALAddInvArg_SER==IdxCurrentSelfEvalRule)
			{
			IdxCurrentSelfEvalRule=None_SER;
			IdxCurrentEvalRule=LogicalANDAry2ALLEQUALSpliceAddInvArg_ER;
			return;
			}
		{
		_console->ResumeLogFile();
		char Buffer[10];
		ltoa(IdxCurrentSelfEvalRule,Buffer,10);
		WARNING(Buffer);
		ltoa(IdxCurrentEvalRule,Buffer,10);
		WARNING(Buffer);
		}
		FATAL("FATAL ERROR: reinterpreter code needs extension.");
		};
	// Shallow StrictlyModify
	if (ArgArray[0]->CanStrictlyModify(*ArgArray[1]))
		{
		IdxCurrentSelfEvalRule = SelfEvalStrictlyModify_SER;
		InferenceParameter1 = 0;
		InferenceParameter2 = 1;
		return;
		};
	if (ArgArray[1]->CanStrictlyModify(*ArgArray[0]))
		{
		IdxCurrentSelfEvalRule = SelfEvalStrictlyModify_SER;
		InferenceParameter1 = 1;
		InferenceParameter2 = 0;
		return;
		};
	// deep StrictlyModify
	if (ArgArray[0]->HasArgRelatedToThisConceptBy(*ArgArray[1],::CanStrictlyModify))
		{
		InferenceParameter1 = 1;
		InferenceParameter2 = 0;
		IdxCurrentSelfEvalRule = LogicalANDStrictlyModify_SER;
		return;
		};
	if (ArgArray[1]->HasArgRelatedToThisConceptBy(*ArgArray[0],::CanStrictlyModify))
		{
		InferenceParameter1 = 0;
		InferenceParameter2 = 1;
		IdxCurrentSelfEvalRule = LogicalANDStrictlyModify_SER;
		return;
		};
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
	return;
}

void MetaConnective::DiagnoseIntermediateRulesOR2AryAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/5/1999
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalOR_MC));
	assert(2==size());
	//! \todo MANY OTHER INFERENCE RULES
	// X EQUALTOONEOF ..., X EQUALTOONEOF ...: merge on arg0
	if (   ArgArray[0]->IsExactType(EQUALTOONEOF_MC)
		&& ArgArray[1]->IsExactType(EQUALTOONEOF_MC)
		&& *ArgArray[0]->ArgN(0)==*ArgArray[1]->ArgN(0))
		{
		IdxCurrentEvalRule = LogicalANDAry2NArySpliceEqualArg_ER;
		SetArgNInfParam1(0,0);
		SetArgNInfParam1(1,0);
		InferenceParameter1 = 1;
		InferenceParameter2 = 0;
		return;
		};
	if (DiagnoseCommonIntermediateRulesORXORAux()) return;

	//! \todo two more sophisticated rules are possible:
	//! 1) an OR-clause of two AND-terms, which have equal arity and which have
	//! an order s.t. the AND-clauses terms are all antiidempotent, is rewriteable as an
	//! IFF clause.
	//! 2) an OR-clause of n AND-clauses T1..Tn, all of which have arity n, s.t. an AND-term
	//! T-meta exists s.t. each T1..Tn differs from T-meta only in that term k of Tk is anti-idempotent
	//! to term k of T-meta (as opposed to syntactical equality) is rewriteable as a XOR clause.
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

void MetaConnective::DiagnoseIntermediateRulesIFF2AryAux() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/14/1999
	// ASSUMPTION: the args have already been sorted by DiagnoseIntermediateRules
	assert(IsExactType(LogicalIFF_MC));
	assert(2==fast_size());
	if (   ArgArray[0]->IsExactType(LogicalAND_MC)
		&& ArgArray[1]->IsExactType(LogicalOR_MC)
		&& static_cast<MetaConnective*>(ArgArray[0])->ExactOrderPairwiseRelation(*static_cast<MetaConnective*>(ArgArray[1]),AreSyntacticallyEqual))
		{
		InferenceParameter1 = 0;
		InferenceParameter2 = 1;
		IdxCurrentSelfEvalRule = IFF_ReplaceANDORWithOwnArgs_SER;
		return;	
		};
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

// AND using IFF for dynamic implies information
//	C=>A, B=>D: remote C=>D: blot C
//	C=>~A, ~B=>D: remote C=>D: blot C
//	C=>A, B=>~D: remote C=>~D: force contradiction
//	C=>~A, ~B=>~D: remote C=>~D: force contradiction
// OR using IFF for dynamic implies information
//	C=>A, B=>D: remote C=>D: blot D
//	C=>~A, ~B=>D: remote C=>D: blot D
//	C=>A, B=>~D: remote C=>~D: force true
//	C=>~A, ~B=>~D: remote C=>~D: force true

// These hardcodes cause StrictlyModifies-like behavior rather than substitutions, which 
// is usually useful for RAM purposes
bool
MetaConnective::HyperNonStrictlyImpliesReplacement(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// emulated call: ArgArray[Idx]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToRHS,::NonStrictlyImplies)
	// RHS is the TruthValue constant TRUE

	try	{
		size_t i = fast_size();
		do	if (NonStrictlyImplies(LHS,*ArgArray[--i]))
				{
				// LogicalAND: delete target arg instead
				// LogicalOR: should be caught by now
				// LogicalIFF: delete target arg and convert to AND instead
				// LogicalXOR: delete target arg and convert to NOR instead
				// LogicalNXOR: delete target arg and convert to OR instead
				// LogicalNIFF: delete target arg and convert to NAND instead
				if (IsExactType(LogicalOR_MC) || 2==fast_size())
					// IFF, AND could be 2-ary
					SetLHSToRHS(ArgArray[i],RHS);		//! \todo replace with intelligent actions
				else{
					FastDeleteIdx(i);
					if      (IsExactType(LogicalIFF_MC))
						SetExactType(LogicalAND_MC);
					else if (IsExactType(LogicalXOR_MC))
						SetNANDNOR(LogicalNOR_MC);
					else if (IsExactType(LogicalNXOR_MC))
						SetExactType(LogicalOR_MC);
					else if (IsExactType(LogicalNIFF_MC))
						SetNANDNOR(LogicalNAND_MC);
					}
				IdxCurrentSelfEvalRule = None_SER;      // resets syntax-immunity at this level
				}
		while(0<i);

		i = fast_size();
		do	if (ArgArray[--i]->HasArgRelatedToThisConceptBy(LHS,::NonStrictlyImplies))
				{
				if (LogicalAND_MC<=ArgArray[i]->ExactType() && LogicalNAND_MC>=ArgArray[i]->ExactType())
					{
					if (!static_cast<MetaConnective*>(ArgArray[i])->HyperNonStrictlyImpliesReplacement(LHS,RHS))
						return false;
					CleanUpANDIFFAfterHyperNonStrictlyImpliesReplacement(ArgArray[i],RHS);
					}
				else if (!ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(LHS,RHS,SetLHSToRHS,::NonStrictlyImplies))
					return false;
				IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
				}
		while(0<i);
		return true;
		}
	catch(const bad_alloc&)
		{
		return false;
		};
}

bool
MetaConnective::HyperNonStrictlyImpliesLogicalNOTOfReplacement(const MetaConcept& LHS, const MetaConcept& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// emulated call: ArgArray[Idx]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(*ArgArray[InferenceParameter1],Tmp,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf)
	// RHS is the TruthValue constant TRUE

	try	{
		size_t i = fast_size();
		do	if (NonStrictlyImpliesLogicalNOTOf(LHS,*ArgArray[--i]))
				{
				// LogicalAND: should be caught by now
				// LogicalOR: delete target arg instead
				// LogicalIFF: delete target arg and convert to NOR instead
				// LogicalXOR: delete target arg; 2-ary to IFF
				// LogicalNXOR: delete target arg; 2-ary to IFF
				// LogicalNIFF: delete target arg and convert to OR instead
				if (IsExactType(LogicalAND_MC) || 2==fast_size())
					// IFF, OR could be 2-ary
					SetLHSToLogicalNOTOfRHS(ArgArray[i],RHS);
				else{
					FastDeleteIdx(i);
					if      (IsExactType(LogicalIFF_MC))
						SetNANDNOR(LogicalNOR_MC);
					else if (IsExactType(LogicalXOR_MC))
						{
						if (2==fast_size())
							{
							ArgArray[1]->SelfLogicalNOT();
							SetExactType(LogicalIFF_MC);
							};
						}
					else if (IsExactType(LogicalNXOR_MC))
						{
						if (2==fast_size())
							SetExactType(LogicalIFF_MC);
						}
					else if (IsExactType(LogicalNIFF_MC))
						SetExactType(LogicalOR_MC);
					}
				IdxCurrentSelfEvalRule = None_SER;      // resets syntax-immunity at this level
				}
		while(0<i);

		i = fast_size();
		do	if (ArgArray[--i]->HasArgRelatedToThisConceptBy(LHS,::NonStrictlyImpliesLogicalNOTOf))
				{
				if (LogicalAND_MC<=ArgArray[i]->ExactType() && LogicalNAND_MC>=ArgArray[i]->ExactType())
					{
					if (!static_cast<MetaConnective*>(ArgArray[i])->HyperNonStrictlyImpliesLogicalNOTOfReplacement(LHS,RHS))
						return false;
					// catch 2-ary IFF/OR aftermath here
					CleanUpORIFFAfterHyperNonStrictlyImpliesLogicalNOTOfReplacement(ArgArray[i],RHS);
					}
				else if (!ArgArray[i]->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(LHS,RHS,SetLHSToLogicalNOTOfRHS,::NonStrictlyImpliesLogicalNOTOf))
					return false;
				IdxCurrentSelfEvalRule = None_SER;	// resets syntax-immunity at this level
				}
		while(0<i);
		return true;
		}
	catch(const bad_alloc&)
		{
		return false;
		};
}

//! \todo analyze MetaConceptWithArgArray::LogicalANDSpliceNAryEqualArg for parallel IO code, and splice it out
bool MetaConceptWithArgArray::LogicalANDSpliceNAryEqualArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/9/2003
	// InferenceParameter1, InferenceParameter2 point to compatible MetaConceptWithNArgs statements
	// InferenceParameter1 of the two args point to the == arg
	// AND(IFF(A,B),IFF(A,C),D) |-> AND(IFF(A,B,C),D)
	LOG(SpliceTransSymmClauseEqualClause);
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);

	if ( static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->fast_size()
		<static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->fast_size())
		{
		size_t Tmp = InferenceParameter2;
		InferenceParameter2 = InferenceParameter1;
		InferenceParameter1 = Tmp;
		};

	MetaConceptWithArgArray& Parameter1 = *static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1]);
	MetaConceptWithArgArray& Parameter2 = *static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2]);
	const size_t NewSlotsOrigin = Parameter1.fast_size();

	if (!Parameter1.InsertNSlotsAtV2(Parameter2.fast_size()-1,NewSlotsOrigin))
		return false;

	{
	size_t i = Parameter2.fast_size();
	MetaConcept* Tmp = NULL;
	while(Parameter2.InferenceParameter1< --i)
		{
		Parameter2.TransferOutAndNULL(i,Tmp);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i-1,Tmp);
		};
	while(0<i)
		{
		Parameter2.TransferOutAndNULL(--i,Tmp);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i,Tmp);
		};
	Parameter1.ForceCheckForEvaluation();
	}
	LOG("to");
	LOG(*ArgArray[InferenceParameter1]);	

	DeleteIdx(InferenceParameter2);

	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::LogicalANDSpliceALLEQUALAddInvArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/9/2003
	// InferenceParameter1, InferenceParameter2 point to ALLEQUAL statements
	// InferenceParameter1 of the two args point to the anti-idempotent arg
	// AND(IFF(A,B),IFF(~A,C),D) |-> AND(IFF(A,B,~C),D)
	LOG(SpliceALLEQUALClauseAddInvClause);
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter2]);

	if ( static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->fast_size()
		<static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->fast_size())
		{
		size_t Tmp = InferenceParameter2;
		InferenceParameter2 = InferenceParameter1;
		InferenceParameter1 = Tmp;
		};

	MetaConceptWithArgArray& Parameter1 = *static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1]);
	MetaConceptWithArgArray& Parameter2 = *static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2]);
	const size_t NewSlotsOrigin = Parameter1.fast_size();

	if (!Parameter1.InsertNSlotsAtV2(Parameter2.fast_size()-1,NewSlotsOrigin))
		return false;

	{
	size_t i = Parameter2.fast_size();
	MetaConcept* Tmp = NULL;
	while(Parameter2.InferenceParameter1< --i)
		{
		Parameter2.TransferOutAndNULL(i,Tmp);
		Tmp->SelfInverse(StdAddition_MC);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i-1,Tmp);
		};
	while(0<i)
		{
		Parameter2.TransferOutAndNULL(--i,Tmp);
		Tmp->SelfInverse(StdAddition_MC);
		Parameter1.TransferInAndOverwriteRaw(NewSlotsOrigin+i,Tmp);
		};
	Parameter1.ForceCheckForEvaluation();

	}
	LOG("to");
	LOG(*ArgArray[InferenceParameter1]);

	DeleteIdx(InferenceParameter2);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

#if 0
bool MetaConnective::LogicalANDSpawnIFF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/18/2000
	// DEMO CODE: 2-ary cycles are better handled via StrictlyModifies; activate calls when
		// nontrivial
	// need: effective sources 0..NonStrictLHSMax, 0..NonStrictRHSMax
	// if can find LHS, RHS source s.t. LHS makes LHS=>RHS and RHS makes RHS=>LHS, *and*
	// no corresponding IFF: spawn the IFF.  Should filter both lists by corresponding legitimate RHS targets.
	MetaConcept*** ImpliesSources = NULL;
	size_t LogicalOrigin = 0;
	size_t Idx = fast_size();
	do	if (ArgArray[--Idx]->CanMakeLHSImplyRHS())
			{
			do	if (ArgArray[LogicalOrigin]->CanMakeLHSImplyRHS())
					{
					ImpliesSources = _new_buffer<MetaConcept**>(Idx-LogicalOrigin+1);
					goto TriedToStartBuffers;
					}
			while(Idx> ++LogicalOrigin);
			}	
	while(1<Idx);

TriedToStartBuffers:
	if (NULL==ImpliesSources) return false;

	if (   ArgArray[LogicalOrigin]->InitIdxSourceList(ImpliesSources[0])
		&& ArgArray[Idx]->InitIdxSourceList(ImpliesSources[Idx-LogicalOrigin]))
		{	// try to fill cache, then complete scan with cache list
		while(LogicalOrigin<--Idx)
			if (    ArgArray[Idx]->CanMakeLHSImplyRHS()
				&& !ArgArray[Idx]->InitIdxSourceList(ImpliesSources[Idx-LogicalOrigin]))
				break;
		Idx = ArraySize(ImpliesSources);
		do	if (NULL!=ImpliesSources[--Idx])
				{
				size_t Idx2 = Idx;
				do	if (NULL!=ImpliesSources[--Idx2])
						{	// we have a scan possibility.  If ArgArray[Idx] can make an item
						// in ImpliesSources[Idx] imply one in ImpliesSources[Idx2], and this is dual,
						// then Franci should spawn a 2-ary IFF.  This IFF is *not* one that already exists,
						// provided the candidates are not equal.
						// Franci probably should in-place sort by whether the index members are an 
						// RHS [need not be order preserving.]
						size_t StrictIdxUB = ArgArray[LogicalOrigin+Idx]->RadixSortIdxSourceListByRHSCompatible(ImpliesSources[Idx2]);
						if (0==StrictIdxUB)
							break;
						size_t StrictIdx2UB = ArgArray[LogicalOrigin+Idx2]->RadixSortIdxSourceListByRHSCompatible(ImpliesSources[Idx]);
						if (0==StrictIdx2UB) break;
						// scan!
						size_t Idx3 = StrictIdxUB;
						do	{
							Idx3--;
							size_t Idx4 = StrictIdx2UB;
							do	if (   ArgArray[LogicalOrigin+Idx]->MakesLHSImplyRHS(*ImpliesSources[Idx][Idx3],*ImpliesSources[Idx2][--Idx4])
									&& ArgArray[LogicalOrigin+Idx2]->MakesLHSImplyRHS(*ImpliesSources[Idx2][Idx4],*ImpliesSources[Idx][Idx3])
									&& *ImpliesSources[Idx][Idx3]!=*ImpliesSources[Idx2][Idx4])
									{	// success! proceed to spawn IFF-clause
									MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(2);
									if (!NewArgArray)
										goto IFFCreationFailure;
									NewArgArray[0] = ImpliesSources[Idx][Idx3];
									NewArgArray[1] = ImpliesSources[Idx2][Idx4];
									try	{
										MetaConcept* NewIFF	= new MetaConnective(NewArgArray,IFF_MCM);
										ImpliesSources[Idx][Idx3]=NULL;
										ImpliesSources[Idx2][Idx4]=NULL;

										// ImpliesSources cache cleanup, success case
										Idx = ArraySize(ImpliesSources);
										do	if (ImpliesSources[--Idx])
											BLOCKDELETEARRAY(ImpliesSources[Idx]);
										while(0<Idx);
										DELETEARRAY(ImpliesSources);
										
										InferenceParameter1 = reinterpret_cast<size_t>(NewIFF);
										IdxCurrentSelfEvalRule = SelfEvalAddArgAtEndAndForceCorrectForm_SER;
										return true;
										}
									catch(const bad_alloc&)
										{
										DELETEARRAY(NewArgArray);
										goto IFFCreationFailure;
										}
									}
							while(0<Idx3);
							}
						while(0<Idx3);
						}
				while(0<Idx2);
				}
		while(1<Idx);
		}; 

IFFCreationFailure:
	// ImpliesSources cache cleanup, failed case
	Idx = ArraySize(ImpliesSources);
	do	if (NULL!=ImpliesSources[--Idx])
			BLOCKDELETEARRAY(ImpliesSources[Idx]);
	while(0<Idx);
	DELETEARRAY(ImpliesSources);
	return false;
}
#endif

bool
MetaConnective::LogicalANDBoostORWithIFF(MetaConnective*& SpeculativeTarget, size_t LowXORIdx, size_t HighORIdx) const
{
	bool Tweaked = false;
	size_t Idx4 = LowXORIdx;
	while(--Idx4>HighORIdx)
		if (static_cast<MetaConnective*>(ArgArray[Idx4])->FindTwoRelatedArgs(*SpeculativeTarget,AreSyntacticallyEqual))
			{
			try	{
				autoval_ptr<TruthValue> TmpTruthValue;
				TmpTruthValue = new TruthValue(true);
				MetaConnective* Target = NULL;
				static_cast<MetaConnective*>(ArgArray[Idx4])->CopyInto(Target);
				Target->TransferInAndOverwriteRaw(Target->InferenceParameter1,TmpTruthValue.release());
				Target->SetExactTypeV2(LogicalAND_MC);
				SpeculativeTarget->TransferInAndOverwriteRaw(SpeculativeTarget->InferenceParameter1,Target);
				Tweaked = true;
				}
			catch(const bad_alloc&)
				{
				WARNING(msz_NotDefinitive);
				continue;
				}
			}
		else if (static_cast<MetaConnective*>(ArgArray[Idx4])->FindTwoRelatedArgs(*SpeculativeTarget,IsAntiIdempotentTo))
			{
			try	{
				autoval_ptr<TruthValue> TmpTruthValue;
				TmpTruthValue = new TruthValue(false);
				MetaConnective* Target = NULL;
				static_cast<MetaConnective*>(ArgArray[Idx4])->CopyInto(Target);
				Target->TransferInAndOverwriteRaw(Target->InferenceParameter1,TmpTruthValue.release());
				Target->SetNANDNOR(LogicalNOR_MC);
				SpeculativeTarget->TransferInAndOverwriteRaw(SpeculativeTarget->InferenceParameter1,Target);
				Tweaked = true;
				}
			catch(const bad_alloc&)
				{
				WARNING(msz_NotDefinitive);
				continue;
				}
			};
	return Tweaked;
}

bool
MetaConnective::ExploreImprovisedUsesSpeculativeORAux(MetaConnective*& SpeculativeTarget, const MetaConnective& Target, size_t MaxArityWanted, size_t HighXORIdx, size_t LowXORIdx, size_t HighORIdx, size_t LowORIdx) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/12/2005
	//	**	evaluate OR(B,C) until it stabilizes.
	if (SpeculativeTarget->ForceStdForm(),SpeculativeTarget->CanEvaluateToSameType())
		{
		LOG("====");
		while(SpeculativeTarget->ForceStdForm(),SpeculativeTarget->DestructiveEvaluateToSameType());
		LOG("====");
		};
RestartSpeculativeOR:
	// If it's already drifted from OR, return false:
	if (!SpeculativeTarget->IsExactType(LogicalOR_MC)) return false;

	if (SpeculativeTarget->ForceStdForm(),SpeculativeTarget->CanEvaluate())
		return false;

	//	**	if arity of OR(B,C)>N-1 (or is no longer OR), drop out;
	if (MaxArityWanted>=SpeculativeTarget->fast_size())
		{
		if (   Target.IsExactType(LogicalXOR_MC)
			&& SpeculativeTarget->SubvectorArgList(Target))
			{
			DELETE(SpeculativeTarget);
			return true;	//	**	if XOR plus this OR would trigger OR/XOR reduction, 
			}

		// OR antiidempotent arglist: construct NIFF.
		//! \todo OR subarglist: comment on deleting superfluous args
		if (Target.IsExactType(LogicalOR_MC))
			{	// check suitable ORs for NIFF-triggering
RetryBoostedOR:
			size_t Idx4 = Target.fast_size();
			while(SpeculativeTarget->FindArgRelatedToRHS(*Target.ArgArray[--Idx4],NonStrictlyImpliesLogicalNOTOf))
				if (0==Idx4)
					{
					DELETE(SpeculativeTarget);
					return true;
					}
//	OR(NOT A1B3_EQ_0,NOT A4B1_EQ_0)
//	note: IFF(A1B3_EQ_0,OR(A3B4_EQ_0,A4B1_EQ_0)) makes NOT A1B3_EQ_0 imply NOT A4B1_EQ_0
//	so this can be auxilliarily reduced to A4B1_EQ_0
//	do this *before* boosting OR with IFF (or do the IFF-part first, at least)
//====
//	Test OR for ability to use AND-args as implies
//	Arity3+: this is arg-reduction.  Do it, then reinvoke
//	Arity2: take over.  This is either IFF-creation or arg-isolation.  Both are interesting
			if (   fast_size()>LowXORIdx	// IFF-implies use for OR; IFF-booster for OR
				&& 1<LowXORIdx-HighORIdx)			// low-level dependency: OR<IFF<XOR)
				{	// IFF-implies use check
				size_t Idx4 = LowXORIdx;	// IFF is an imply-inducing type
				while(--Idx4>HighORIdx)
					// IFF is an imply-inducing type
					if (SpeculativeTarget->CanUseThisAsMakeImply(*ArgArray[Idx4]))
						{	// Arity 3+: use it!
						if (!SpeculativeTarget->IsExactType(LogicalOR_MC))
							return false;
						SpeculativeTarget->ForceCheckForEvaluation();
						goto RestartSpeculativeOR;
						}
				// IFF-booster for OR check
				if (LogicalANDBoostORWithIFF(SpeculativeTarget,LowXORIdx,HighORIdx))	// Destructive!
					goto RetryBoostedOR;
				}
			}
		}
	return false;
}

// auxilliary functions for MetaConnective::LogicalANDCreateSpeculativeOR
// NOTE: false means failure; true means success, SpeculativeTarget
// has been consumed.
bool
MetaConnective::LogicalANDCreateSpeculativeORAux(MetaConnective*& SpeculativeTarget, size_t MaxArityWanted, size_t HighXORIdx, size_t LowXORIdx, size_t HighORIdx, size_t LowORIdx) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	//	**	evaluate OR(B,C) until it stabilizes.
	if (SpeculativeTarget->ForceStdForm(),SpeculativeTarget->CanEvaluateToSameType())
		{
		LOG("====");
		while(SpeculativeTarget->ForceStdForm(),SpeculativeTarget->DestructiveEvaluateToSameType());
		LOG("====");
		};
RestartSpeculativeOR:
	// If it's already drifted from OR, return false:
	if (!SpeculativeTarget->IsExactType(LogicalOR_MC)) return false;

	if (SpeculativeTarget->ForceStdForm(),SpeculativeTarget->CanEvaluate())
		{
		MetaConcept* SpeculativeTarget2 = SpeculativeTarget;
		SpeculativeTarget = NULL;
		while(DestructiveSyntacticallyEvaluateOnce(SpeculativeTarget2));
		if (!SpeculativeTarget2->IsExactType(TruthValue_MC))
			{
			InferenceParameterMC = SpeculativeTarget2;
			IdxCurrentSelfEvalRule = SelfEvalAddArgAtEndAndForceCorrectForm__SER;
			LOG("Spawning this from SpeculativeOR screen");
			LOG(*SpeculativeTarget2);
			return true;
			}
		DELETE(SpeculativeTarget2);
		return false;
		};
	//	**	if arity of OR(B,C)>N-1 (or is no longer OR), drop out;
	if (MaxArityWanted>=SpeculativeTarget->fast_size())
		{
RetryBoostedOR:
		MetaConcept* SpeculativeTargetMirror = SpeculativeTarget;
		if (   (fast_size()>LowXORIdx && LogicalANDCanUseSpeculativeTarget(SpeculativeTargetMirror,LowXORIdx,HighXORIdx+1))
			|| (fast_size()>LowORIdx  && LogicalANDCanUseSpeculativeTarget(SpeculativeTargetMirror,LowORIdx,HighORIdx+1)))
			{
			if (NULL==SpeculativeTargetMirror)
				SpeculativeTarget = NULL;
			return true;
			}
		if (   fast_size()>LowXORIdx	// IFF-implies use for OR
			&& 1<LowXORIdx-HighORIdx)			// low-level dependency: OR<IFF<XOR
			{
			size_t Idx4 = LowXORIdx;	// IFF is an imply-inducing type
			while(--Idx4>HighORIdx)
				// IFF is an imply-inducing type
				if (SpeculativeTarget->CanUseThisAsMakeImply(*ArgArray[Idx4]))
					{	// Arity 3+: use it!
					LOG("Using this");
					LOG(*ArgArray[Idx4]);
					LOG("to reduce speculative OR");
					LOG(*SpeculativeTarget);
					LOG("to");
					SpeculativeTarget->UseThisAsMakeImply(*ArgArray[Idx4]);
					LOG(*SpeculativeTarget);
					if (   !SpeculativeTarget->IsExactType(LogicalOR_MC)
						&& !FindArgRelatedToRHS(*SpeculativeTarget,NonStrictlyImplies))
						{	// spawn IFF, probably
						InferenceParameterMC = SpeculativeTarget;
						IdxCurrentSelfEvalRule = SelfEvalAddArgAtEndAndForceCorrectForm__SER;
						LOG("Spawning this");
						LOG(*SpeculativeTarget);
						return true;
						};
					SpeculativeTarget->ForceCheckForEvaluation();
					goto RestartSpeculativeOR;
					}

			if (   fast_size()>HighORIdx
				&& LogicalANDBoostORWithIFF(SpeculativeTarget,LowXORIdx,HighORIdx))	// Destructive!
				goto RetryBoostedOR;
			}
		}
	return false;
}

bool
MetaConnective::LogicalANDCanUseSpeculativeTarget(MetaConcept*& SpeculativeTarget, size_t NonStrictLB, size_t StrictUB) const
{
//	LHSIdx = real index
//	RHSIdx = fast_size() [safe, it's a relay]
	size_t Detailed_Param1 = 0;
	size_t Detailed_Param2 = 0;
	signed short Detailed_SelfEvalIdx = 0;
	unsigned short Detailed_EvalIdx = 0;		

	size_t Idx3 = StrictUB;
	do	{
		Idx3--;
		if (SpeculativeTarget->LogicalANDFindDetailedRule(*ArgArray[Idx3],fast_size(),Idx3,Detailed_Param1,Detailed_Param2,Detailed_SelfEvalIdx,Detailed_EvalIdx))
			{
			if (LogicalANDReplaceTheseNAry2ORWithNIFF_SER==Detailed_SelfEvalIdx)
				{
				// analyze! only need SpeculativeTarget if Detailed_Param1 = fast_size()
				if (fast_size()==Detailed_Param1)
					{
					InferenceParameter1 = Idx3;
					IdxCurrentSelfEvalRule = SelfEvalRuleEvaluateArg_SER;

					LOG("Using");
					LOG(*ArgArray[Idx3]);
					LOG((2==static_cast<MetaConnective*>(ArgArray[Idx3])->fast_size())
												? "to make this spawned OR-clause an IFF-clause:"
												: "to make this spawned OR-clause a NIFF-clause:");
					LOG(*SpeculativeTarget);
					delete ArgArray[Idx3];
					ArgArray[Idx3] = SpeculativeTarget;
					SpeculativeTarget = NULL;
					}
				else{
					InferenceParameter1 = Idx3;
					IdxCurrentSelfEvalRule = SelfEvalRuleEvaluateArg_SER;
					LOG("Using virtual OR-clause");
					LOG(*SpeculativeTarget);
					DELETE_AND_NULL(SpeculativeTarget);
					LOG((2==static_cast<MetaConnective*>(ArgArray[Idx3])->fast_size())
												? "to make this OR-clause an IFF-clause:"
												: "to make this OR-clause a NIFF-clause:");
					LOG(*ArgArray[Idx3]);
					}

				//! \todo this is a function target: PrepareRetypeToNIFF
				if (2==static_cast<MetaConnective*>(ArgArray[Idx3])->fast_size())
					{
					static_cast<MetaConnective*>(ArgArray[Idx3])->ArgArray[1]->SelfLogicalNOT();
					static_cast<MetaConnective*>(ArgArray[Idx3])->InferenceParameter1 = LogicalIFF_MC;
					}
				else
					static_cast<MetaConnective*>(ArgArray[Idx3])->InferenceParameter1 = LogicalNIFF_MC;
				static_cast<MetaConnective*>(ArgArray[Idx3])->IdxCurrentSelfEvalRule = CompatibleRetype_SER;

				return true;
				}

			InferenceParameterMC = SpeculativeTarget;	// target
			InferenceParameter1 = Idx3;	// trigger
			IdxCurrentSelfEvalRule = LogicalANDSpawnClauseForDetailedRule_SER;

			LOG("Spawning");
			LOG(*SpeculativeTarget);
			LOG("to reduce this");					
			LOG(*ArgArray[Idx3]);
			return true;
			}
		}
	while(NonStrictLB<Idx3);
	return false;
}

// NOTE: current system creates the speculative targets dynamically.
// However, QState's improviser needs guidance to know whether a proposed AND's logical 
// negation interacts with the speculative OR code...that is, it wants to test a proposed 
// OR against the LogicalAND's vector of speculative OR's.
// Right now, the NIFF/XOR bridge is more important.

// The main difference between the main code, and the QState analyzer, is that the QState
// analyzer must be silent.
bool
MetaConnective::ImprovisedUsesSpeculativeOR(const MetaConnective& Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/12/2000
	// Idx is first OR-arg in this (LogicalAND)
	// New spec!
	// We may want an OR-arg two ways:
	// *	XOR wants a subarg OR so it can dismantle itself
	// *	OR wants an all-antiidempotent OR so that it can NIFF itself
	// We may derive a k-ary OR-arg two ways:
	// *	Two ORs with an antiidempotent clause may be merged, omitting the antiidempotent args
	// *	2 (k+1)-ary NIFFs with antiidempotent arg may mesh with k 3-ary XORs so that the omitted
	//      args of the XORS are anti-idempotent to the target OR args.
	if (   LogicalXOR_MC<ArgArray[0]->ExactType()
		|| LogicalOR_MC>ArgArray[fast_size()-1]->ExactType())
		return false;

	// OR-span
	size_t LowORIdx = 0;
	size_t HighXORIdx = fast_size()-1;
	while(LogicalOR_MC>ArgArray[LowORIdx]->ExactType()) LowORIdx++;
	while(LogicalXOR_MC<ArgArray[HighXORIdx]->ExactType()) HighXORIdx--;
	size_t HighORIdx = HighXORIdx;
	size_t LowXORIdx = LowORIdx;
	if (LogicalOR_MC<ArgArray[LowORIdx]->ExactType())
		{
		LowORIdx = HighORIdx = fast_size();
		}
	else{
		while(LogicalOR_MC<ArgArray[HighORIdx]->ExactType()) HighORIdx--;
		};
	if (LogicalXOR_MC>ArgArray[HighXORIdx]->ExactType())
		{
		LowXORIdx = HighXORIdx = fast_size();
		}
	else{
		while(LogicalXOR_MC>ArgArray[LowXORIdx]->ExactType()) LowXORIdx++;
		};

	if (LowORIdx<fast_size() || LowXORIdx<fast_size())
		{	// something wants OR clauses
		const size_t MaxXORArityWanted = (fast_size()>HighXORIdx) ? static_cast<MetaConnective*>(ArgArray[HighXORIdx])->fast_size()-1 : 0;
		const size_t MaxORArityWanted = (fast_size()>HighORIdx) ? static_cast<MetaConnective*>(ArgArray[HighORIdx])->fast_size() : 0;
		const size_t MaxArityWanted = (MaxORArityWanted<MaxXORArityWanted) ? MaxXORArityWanted : MaxORArityWanted;
		// NIFF, OR: think in ordered triples: index-index-length created.  Length-created is dynamically computable "quickly"
		// XOR, OR: think in length wanted: XOR: 2 to arity-1, OR wants exact
		// generate OR triple list
		if (   LowORIdx<HighORIdx
			&& MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[LowORIdx])->fast_size()+static_cast<MetaConnective*>(ArgArray[LowORIdx+1])->fast_size()-2)
			{
			size_t SweepIdx1 = LowORIdx;
			do	{
				size_t SweepIdx2 = SweepIdx1+1;
				do	if (static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[SweepIdx2]),IsAntiIdempotentTo))
						{
						MetaConnective* ORArg1 = static_cast<MetaConnective*>(ArgArray[SweepIdx1]);
						MetaConnective* ORArg2 = static_cast<MetaConnective*>(ArgArray[SweepIdx2]);
						MetaConcept** SpeculativeArgArray = _new_buffer<MetaConcept*>(ORArg1->fast_size()+ORArg2->fast_size()-2);
						if (NULL==SpeculativeArgArray)
							{
							WARNING(msz_NotDefinitive);
							return false;
							}
						MetaConnective* SpeculativeTarget = NULL;
						try	{	// copy args from OR(A,B),OR(~A,C) into SpeculativeArgArray, then create OR
							ORArg1->CopyAllArgsButOneIntoArgList(SpeculativeArgArray);
							ORArg2->CopyAllArgsButOneIntoArgList(SpeculativeArgArray+ORArg1->fast_size()-1);
							SpeculativeTarget = new MetaConnective(SpeculativeArgArray,OR_MCM);
							}
						catch(const bad_alloc&)
							{
							BLOCKFREEARRAY(SpeculativeArgArray);
							WARNING(msz_NotDefinitive);
							return false;
							}
						if (ExploreImprovisedUsesSpeculativeORAux(SpeculativeTarget,Target,MaxArityWanted,HighXORIdx,LowXORIdx,HighORIdx,LowORIdx))
							return true;

						DELETE(SpeculativeTarget);
						}
				while(++SweepIdx2<=HighORIdx && MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()+static_cast<MetaConnective*>(ArgArray[SweepIdx2])->fast_size()-2);
				}
			while(++SweepIdx1<HighORIdx && MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()+static_cast<MetaConnective*>(ArgArray[SweepIdx1+1])->fast_size()-2);
			};
		// NIFF-span: non-trivial leads to cross-referencing against XOR,
		// then another list of OR-clauses
		if (   LowXORIdx<HighXORIdx
			&& 3==static_cast<MetaConnective*>(ArgArray[LowXORIdx])->fast_size()
			&& 3==static_cast<MetaConnective*>(ArgArray[LowXORIdx+1])->fast_size()
			&& 2<fast_size()-HighXORIdx)
			{
			size_t LowNIFFIdx = HighXORIdx+1;
			size_t HighNIFFIdx = fast_size()-1;
			size_t High3AryXORIdx = HighXORIdx;
			while(3<static_cast<MetaConnective*>(ArgArray[High3AryXORIdx])->fast_size()) High3AryXORIdx--;
			while(LogicalNIFF_MC<ArgArray[HighNIFFIdx]->ExactType()) HighNIFFIdx--;
			while(   ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC)
				  && static_cast<MetaConnective*>(ArgArray[HighNIFFIdx])->fast_size()>(High3AryXORIdx-LowXORIdx+2))
				HighNIFFIdx--;
			while(   ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC)
				  && static_cast<MetaConnective*>(ArgArray[HighNIFFIdx])->fast_size()>MaxArityWanted+1)
				HighNIFFIdx--;
			if (ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC))
				while(   ArgArray[HighNIFFIdx-1]->IsExactType(LogicalNIFF_MC)
					  && static_cast<MetaConnective*>(ArgArray[HighNIFFIdx])->fast_size()>static_cast<MetaConnective*>(ArgArray[HighNIFFIdx-1])->fast_size())
					HighNIFFIdx--;
			if (   ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC)
				&& ArgArray[HighNIFFIdx-1]->IsExactType(LogicalNIFF_MC))
				{
				while(LogicalNIFF_MC>ArgArray[LowNIFFIdx]->ExactType()) LowNIFFIdx++;
				while(static_cast<MetaConnective*>(ArgArray[LowNIFFIdx])->fast_size()<static_cast<MetaConnective*>(ArgArray[LowNIFFIdx+1])->fast_size())
					LowNIFFIdx++;
				// generate NIFF triple list
				size_t* NIFFPairs = NULL;
				unsigned long* XORBridgeIdxSuite = NULL;
				size_t SweepIdx1 = LowNIFFIdx;
				do	{
					size_t SweepIdx2 = SweepIdx1;
					while(++SweepIdx2<=HighNIFFIdx && static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()==static_cast<MetaConnective*>(ArgArray[SweepIdx2])->fast_size())
						if (static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[SweepIdx2]),IsAntiIdempotentTo))
							{
							if (!ExtendByN(NIFFPairs,2))
								return false;
							NIFFPairs[ArraySize(NIFFPairs)-2] = SweepIdx1;
							NIFFPairs[ArraySize(NIFFPairs)-1] = SweepIdx2;
							};
					}
				while(++SweepIdx1<HighNIFFIdx && MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()-1);
				while(NULL!=NIFFPairs)
					{	// use NIFF triple list
					size_t CurrentXORBridgeIdx = 0;
					// realloc is malloc for NULL source
					{
					unsigned long* Tmp = REALLOC(XORBridgeIdxSuite,(static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->fast_size()-1)*sizeof(unsigned long));
					if (NULL==Tmp)
						{
						free(XORBridgeIdxSuite);
						return false;
						};
					XORBridgeIdxSuite = Tmp;
					};
					// find XOR-bridge
					size_t SweepIdx1 = High3AryXORIdx;
					static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]]),IsAntiIdempotentTo);
					const size_t NIFF1IdxAvoid = static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->InferenceParameter1;
					const size_t NIFF2IdxAvoid = static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]])->InferenceParameter1;
					size_t XORIdx1Match;
					size_t XORIdx2Match;
								// fetch XOR match NIFF1
					do	if (    static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]]))
								// don't use NIFF1 C/~C as match
							&& (XORIdx1Match=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->InferenceParameter1,static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->InferenceParameter1!=NIFF1IdxAvoid)
								// XOR doesn't match two args in NIFF1
							&& !static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]]),AreSyntacticallyEqual,XORIdx1Match,static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->InferenceParameter1)
								// fetch XOR match NIFF2
							&&  static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]]))
								// don't use same XOR arg for both matches
							&& (XORIdx2Match=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->InferenceParameter1,XORIdx1Match!=XORIdx2Match)
								// don't use NIFF2 C/~C as match
							&&  static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]])->InferenceParameter1!=NIFF2IdxAvoid
								// XOR doesn't match two args in NIFF2
							&& !static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]]),AreSyntacticallyEqual,XORIdx2Match,static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]])->InferenceParameter1))
							{
							static_cast<MetaConnective*>(ArgArray[SweepIdx1])->InferenceParameter1 
								= (0!=XORIdx1Match && 0!=XORIdx2Match) ? 0
								: (1!=XORIdx1Match && 1!=XORIdx2Match) ? 1
								: 2;
							XORBridgeIdxSuite[CurrentXORBridgeIdx++] = SweepIdx1;
							}
					while(0<SweepIdx1 && --SweepIdx1>=LowXORIdx);
					if (ArraySize(XORBridgeIdxSuite)==CurrentXORBridgeIdx)
						{	// if we have XOR bridge, test OR-clause against target cases
						MetaConcept** SpeculativeArgArray = _new_buffer<MetaConcept*>(static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->fast_size()-1);
						if (NULL==SpeculativeArgArray)
							{
							WARNING(msz_NotDefinitive);
							return false;
							}
						MetaConnective* SpeculativeTarget = NULL;
						try	{
							size_t InsertIdx = 0;
							do	{	// construct OR clause from XOR arg *not* in either NIFF
								static_cast<MetaConnective*>(ArgArray[XORBridgeIdxSuite[CurrentXORBridgeIdx-1]])->ArgArray[static_cast<MetaConnective*>(ArgArray[XORBridgeIdxSuite[CurrentXORBridgeIdx-1]])->InferenceParameter1]->CopyInto(SpeculativeArgArray[InsertIdx]);
								SpeculativeArgArray[InsertIdx++]->SelfLogicalNOT();
								}
							while(0<--CurrentXORBridgeIdx);
							SpeculativeTarget = new MetaConnective(SpeculativeArgArray,OR_MCM);					
							}
						catch(const bad_alloc&)
							{	// Franci: not fatal.  Could cause an avoidable stall.
							BLOCKFREEARRAY(SpeculativeArgArray);
							WARNING(msz_NotDefinitive);
							return false;
							};
						if (ExploreImprovisedUsesSpeculativeORAux(SpeculativeTarget,Target,MaxArityWanted,HighXORIdx,LowXORIdx,HighORIdx,LowORIdx))
							{
							free(NIFFPairs);
							free(XORBridgeIdxSuite);
							return true;
							};
						DELETE(SpeculativeTarget);
						};
					_shrink(NIFFPairs,ArraySize(NIFFPairs)-2);
					};
				free(XORBridgeIdxSuite);
				}
			}
		}
	return false;
}

bool
MetaConnective::LogicalANDCreateSpeculativeOR(void) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/12/2000
	// Idx is first OR-arg in this (LogicalAND)
	// New spec!
	// We may want an OR-arg two ways:
	// *	XOR wants a subarg OR so it can dismantle itself
	// *	OR wants an all-antiidempotent OR so that it can NIFF itself
	// We may derive a k-ary OR-arg two ways:
	// *	Two ORs with an antiidempotent clause may be merged, omitting the antiidempotent args
	// *	2 (k+1)-ary NIFFs with antiidempotent arg may mesh with k 3-ary XORs so that the omitted
	//      args of the XORS are anti-idempotent to the target OR args.
	if (   LogicalXOR_MC<ArgArray[0]->ExactType()
		|| LogicalOR_MC>ArgArray[fast_size()-1]->ExactType())
		return false;

	DEBUG_LOG(ZAIMONI_FUNCNAME);
	//! \todo META: GREP Target?
	// OR-span
	size_t LowORIdx = 0;
	size_t HighXORIdx = fast_size()-1;
	while(LogicalOR_MC>ArgArray[LowORIdx]->ExactType()) LowORIdx++;
	while(LogicalXOR_MC<ArgArray[HighXORIdx]->ExactType()) HighXORIdx--;
	size_t HighORIdx = HighXORIdx;
	size_t LowXORIdx = LowORIdx;
	if (LogicalOR_MC<ArgArray[LowORIdx]->ExactType())
		{
		LowORIdx = fast_size();
		HighORIdx = fast_size();
		}
	else{
		while(LogicalOR_MC<ArgArray[HighORIdx]->ExactType()) HighORIdx--;
		};
	if (LogicalXOR_MC>ArgArray[HighXORIdx]->ExactType())
		{
		LowXORIdx = fast_size();
		HighXORIdx = fast_size();
		}
	else{
		while(LogicalXOR_MC>ArgArray[LowXORIdx]->ExactType()) LowXORIdx++;
		};
	DEBUG_LOG("spans OK");

	if (LowORIdx<HighORIdx)
		{	// hypercube code for AND of OR
		DEBUG_LOG("AND of OR hypercube");
		MetaConcept** NewArgArray1 = NULL;
		if (MirrorArgArrayNoOwnership(HighORIdx-LowORIdx,ArgArray+LowORIdx,
								NewArgArray1,2,NAryAllArgsEqualExceptOneAntiIdempotentPair))
			{
			Digraph* AntiIdempotentArgBlotter = NULL;
			PROPERLY_INIT_DIGRAPH(AntiIdempotentArgBlotter,NewArgArray1,false,&NAryAllArgsEqualExceptOneAntiIdempotentPair,DELETE,free,return false);

			AntiIdempotentArgBlotter->ResetDiagonal();
			const size_t HypercubeDimension = AntiIdempotentArgBlotter->FindStrictHypercubeGraphFlushIrrelevantVertices();
			if (0<HypercubeDimension)
				{
				InferenceParameter1 = HypercubeDimension;
				InferenceParameterMC = AntiIdempotentArgBlotter;
				IdxCurrentSelfEvalRule = LogicalORXORCompactANDArgHyperCube_SER;
				return true;				
				};
			delete AntiIdempotentArgBlotter;
//			AntiIdempotentArgBlotter = NULL;
			}

		NewArgArray1 = NULL;
		// compaction of OR by using ANDs
		// A number of the QualInv tests want this
		if (MirrorArgArrayNoOwnership(HighORIdx-LowORIdx,ArgArray+LowORIdx,
								NewArgArray1,2,NAryAllArgsEqualExceptOne))
			{
			Digraph* SingleNoMatchArgBlotter = NULL;
			PROPERLY_INIT_DIGRAPH(SingleNoMatchArgBlotter,NewArgArray1,false,&NAryAllArgsEqualExceptOne,DELETE,free,return false);

			SingleNoMatchArgBlotter->ResetDiagonal();
			const size_t HypercubeDimension = SingleNoMatchArgBlotter->FindStrictHypercubeGraphFlushIrrelevantVertices();
			if (0<HypercubeDimension)
				{
				InferenceParameter1 = HypercubeDimension;
				InferenceParameterMC = SingleNoMatchArgBlotter;
				IdxCurrentSelfEvalRule = LogicalANDORXORCondenseORANDANDArgHyperCube_SER;
				return true;				
				};
			delete SingleNoMatchArgBlotter;
//			SingleNoMatchArgBlotter = NULL;
			}
		}

	if (LowORIdx<fast_size() || LowXORIdx<fast_size())
		{	// something wants OR clauses
		DEBUG_LOG("NIFF/XOR looking for OR");
		const size_t MaxXORArityWanted = (fast_size()>HighXORIdx) ? static_cast<MetaConnective*>(ArgArray[HighXORIdx])->fast_size()-1 : 0;
		const size_t MaxORArityWanted = (fast_size()>HighORIdx) ? static_cast<MetaConnective*>(ArgArray[HighORIdx])->fast_size() : 0;
		const size_t MaxArityWanted = (MaxORArityWanted<MaxXORArityWanted) ? MaxXORArityWanted : MaxORArityWanted;
		// NIFF, OR: think in ordered triples: index-index-length created.  Length-created is dynamically computable "quickly"
		// XOR, OR: think in length wanted: XOR: 2 to arity-1, OR wants exact
		// generate OR triple list
		if (   LowORIdx<HighORIdx
			&& MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[LowORIdx])->fast_size()+static_cast<MetaConnective*>(ArgArray[LowORIdx+1])->fast_size()-2)
			{
			DEBUG_LOG("OR-triple scan");
			size_t SweepIdx1 = LowORIdx;
			do	{
				size_t SweepIdx2 = SweepIdx1+1;
				do	if (static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[SweepIdx2]),IsAntiIdempotentTo))
						{
						DEBUG_LOG("FindTwoRelatedArgs OK, true");
						MetaConnective* SpeculativeTarget = NULL;
						MetaConnective* ORArg1 = static_cast<MetaConnective*>(ArgArray[SweepIdx1]);
						MetaConnective* ORArg2 = static_cast<MetaConnective*>(ArgArray[SweepIdx2]);
						MetaConcept** SpeculativeArgArray = _new_buffer<MetaConcept*>(ORArg1->fast_size()+ORArg2->fast_size()-2);
						DEBUG_LOG("SpeculativeArgArray OK");
						if (NULL==SpeculativeArgArray)
							{
							WARNING(msz_NotDefinitive);
							return false;
							}
						try	{
							// copy args from OR(A,B),OR(~A,C) into SpeculativeArgArray, then create OR
							DEBUG_LOG("Attempting ORArg1->CopyAllArgsButOneIntoArgList(SpeculativeArgArray);");
							ORArg1->CopyAllArgsButOneIntoArgList(SpeculativeArgArray);
							DEBUG_LOG("Attempting ORArg2->CopyAllArgsButOneIntoArgList(SpeculativeArgArray+ORArg1->fast_size()-1);");
							ORArg2->CopyAllArgsButOneIntoArgList(SpeculativeArgArray+ORArg1->fast_size()-1);
							DEBUG_LOG("Attempting SpeculativeTarget = new MetaConnective(SpeculativeArgArray,OR_MCM);");
							SpeculativeTarget = new MetaConnective(SpeculativeArgArray,OR_MCM);
							}
						catch(const bad_alloc&)
							{
							BLOCKDELETEARRAY(SpeculativeArgArray);
							WARNING(msz_NotDefinitive);
							return false;					
							};
						DEBUG_LOG("Attempting LogicalANDCreateSpeculativeORAux");
						if (LogicalANDCreateSpeculativeORAux(SpeculativeTarget,MaxArityWanted,HighXORIdx,LowXORIdx,HighORIdx,LowORIdx))
							{
							DEBUG_LOG("LogicalANDCreateSpeculativeORAux OK, true");
							LOG("Derived speculative OR by anti-idempotent elimination from:");
							LOG(*ORArg1);
							LOG(*ORArg2);
							return true;
							};
						DEBUG_LOG("LogicalANDCreateSpeculativeORAux OK, false");
						DELETE(SpeculativeTarget);
						}
				while(++SweepIdx2<=HighORIdx && MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()+static_cast<MetaConnective*>(ArgArray[SweepIdx2])->fast_size()-2);
				}
			while(++SweepIdx1<HighORIdx && MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()+static_cast<MetaConnective*>(ArgArray[SweepIdx1+1])->fast_size()-2);
			};
		// NIFF-span: non-trivial leads to cross-referencing against XOR,
		// then another list of OR-clauses
		if (   LowXORIdx<HighXORIdx
			&& 3==static_cast<MetaConnective*>(ArgArray[LowXORIdx])->fast_size()
			&& 3==static_cast<MetaConnective*>(ArgArray[LowXORIdx+1])->fast_size()
			&& 2<fast_size()-HighXORIdx)
			{
			DEBUG_LOG("NIFF-span scan");
			size_t LowNIFFIdx = HighXORIdx+1;
			size_t HighNIFFIdx = fast_size()-1;
			size_t High3AryXORIdx = HighXORIdx;
			while(3<static_cast<MetaConnective*>(ArgArray[High3AryXORIdx])->fast_size()) High3AryXORIdx--;
			while(LogicalNIFF_MC<ArgArray[HighNIFFIdx]->ExactType()) HighNIFFIdx--;
			while(   ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC)
				  && static_cast<MetaConnective*>(ArgArray[HighNIFFIdx])->fast_size()>(High3AryXORIdx-LowXORIdx+2))
				HighNIFFIdx--;
			while(   ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC)
				  && static_cast<MetaConnective*>(ArgArray[HighNIFFIdx])->fast_size()>MaxArityWanted+1)
				HighNIFFIdx--;
			if (ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC))
				while(   ArgArray[HighNIFFIdx-1]->IsExactType(LogicalNIFF_MC)
					  && static_cast<MetaConnective*>(ArgArray[HighNIFFIdx])->fast_size()>static_cast<MetaConnective*>(ArgArray[HighNIFFIdx-1])->fast_size())
					HighNIFFIdx--;
			if (   ArgArray[HighNIFFIdx]->IsExactType(LogicalNIFF_MC)
				&& ArgArray[HighNIFFIdx-1]->IsExactType(LogicalNIFF_MC))
				{
				while(LogicalNIFF_MC>ArgArray[LowNIFFIdx]->ExactType()) LowNIFFIdx++;
				while(static_cast<MetaConnective*>(ArgArray[LowNIFFIdx])->fast_size()<static_cast<MetaConnective*>(ArgArray[LowNIFFIdx+1])->fast_size())
					LowNIFFIdx++;
				// generate NIFF triple list
				size_t* NIFFPairs = NULL;
				unsigned long* XORBridgeIdxSuite = NULL;
				size_t SweepIdx1 = LowNIFFIdx;
				do	{
					size_t SweepIdx2 = SweepIdx1;
					while(++SweepIdx2<=HighNIFFIdx && static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()==static_cast<MetaConnective*>(ArgArray[SweepIdx2])->fast_size())
						if (static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[SweepIdx2]),IsAntiIdempotentTo))
							{
							if (!ExtendByN(NIFFPairs,2))
								return false;
							NIFFPairs[ArraySize(NIFFPairs)-2] = SweepIdx1;
							NIFFPairs[ArraySize(NIFFPairs)-1] = SweepIdx2;
							};
					}
				while(++SweepIdx1<HighNIFFIdx && MaxArityWanted>=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->fast_size()-1);
				while(NULL!=NIFFPairs)
					{	// use NIFF triple list
					size_t CurrentXORBridgeIdx = 0;
					// realloc is malloc for NULL source
					{
					unsigned long* Tmp = REALLOC(XORBridgeIdxSuite,(static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->fast_size()-1)*sizeof(unsigned long));
					if (NULL==Tmp)
						{
						free(XORBridgeIdxSuite);
						return false;
						};
					XORBridgeIdxSuite = Tmp;
					};

					// find XOR-bridge
					size_t SweepIdx1 = High3AryXORIdx;
					static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]]),IsAntiIdempotentTo);
					const size_t NIFF1IdxAvoid = static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->InferenceParameter1;
					const size_t NIFF2IdxAvoid = static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]])->InferenceParameter1;
					size_t XORIdx1Match;
					size_t XORIdx2Match;
								// fetch XOR match NIFF1
					do	if (    static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]]))
								// don't use NIFF1 C/~C as match
							&& (XORIdx1Match=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->InferenceParameter1,static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->InferenceParameter1!=NIFF1IdxAvoid)
								// XOR doesn't match two args in NIFF1
							&& !static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]]),AreSyntacticallyEqual,XORIdx1Match,static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->InferenceParameter1)
								// fetch XOR match NIFF2
							&&  static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoEqualArgsLHSRHSLexicalOrderedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]]))
								// don't use same XOR arg for both matches
							&& (XORIdx2Match=static_cast<MetaConnective*>(ArgArray[SweepIdx1])->InferenceParameter1,XORIdx1Match!=XORIdx2Match)
								// don't use NIFF2 C/~C as match
							&&  static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]])->InferenceParameter1!=NIFF2IdxAvoid
								// XOR doesn't match two args in NIFF2
							&& !static_cast<MetaConnective*>(ArgArray[SweepIdx1])->FindTwoRelatedArgs(*static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]]),AreSyntacticallyEqual,XORIdx2Match,static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]])->InferenceParameter1))
							{
							static_cast<MetaConnective*>(ArgArray[SweepIdx1])->InferenceParameter1 
								= (0!=XORIdx1Match && 0!=XORIdx2Match) ? 0
								: (1!=XORIdx1Match && 1!=XORIdx2Match) ? 1
								: 2;
							XORBridgeIdxSuite[CurrentXORBridgeIdx++] = SweepIdx1;
							}
					while(0<SweepIdx1 && --SweepIdx1>=LowXORIdx);
					if (ArraySize(XORBridgeIdxSuite)==CurrentXORBridgeIdx)
						{	// if we have XOR bridge, test OR-clause against target cases
						MetaConnective* SpeculativeTarget = NULL;
						MetaConcept** SpeculativeArgArray = _new_buffer<MetaConcept*>(static_cast<MetaConnective*>(ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]])->fast_size()-1);
						if (NULL==SpeculativeArgArray)
							{
							WARNING(msz_NotDefinitive);
							return false;					
							};
						try	{
							size_t InsertIdx = 0;
							do	{	// construct OR clause from XOR arg *not* in either NIFF
								static_cast<MetaConnective*>(ArgArray[XORBridgeIdxSuite[CurrentXORBridgeIdx-1]])->ArgArray[static_cast<MetaConnective*>(ArgArray[XORBridgeIdxSuite[CurrentXORBridgeIdx-1]])->InferenceParameter1]->CopyInto(SpeculativeArgArray[InsertIdx]);
								SpeculativeArgArray[InsertIdx++]->SelfLogicalNOT();
								}
							while(0<--CurrentXORBridgeIdx);
							SpeculativeTarget = new MetaConnective(SpeculativeArgArray,OR_MCM);					
							}
						catch(const bad_alloc&)
							{
							BLOCKDELETEARRAY(SpeculativeArgArray);
							WARNING(msz_NotDefinitive);
							return false;					
							};
						if (LogicalANDCreateSpeculativeORAux(SpeculativeTarget,MaxArityWanted,HighXORIdx,LowXORIdx,HighORIdx,LowORIdx))
							{
							LOG("Derived speculative OR by NIFF/XOR-bridge construction:");
							LOG(*ArgArray[NIFFPairs[ArraySize(NIFFPairs)-2]]);
							LOG(*ArgArray[NIFFPairs[ArraySize(NIFFPairs)-1]]);
							free(NIFFPairs);
							size_t Idx = ArraySize(XORBridgeIdxSuite);
							do	LOG(*ArgArray[XORBridgeIdxSuite[--Idx]]);
							while(0<Idx);
							free(XORBridgeIdxSuite);
							return true;
							};
						DELETE(SpeculativeTarget);
						};
					_shrink(NIFFPairs,ArraySize(NIFFPairs)-2);
					};
				free(XORBridgeIdxSuite);
				}
			}
		}
	return false;
}

bool
MetaConnective::ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/2/2003
	if (ArgArray[ArgIdx]->IsExactType(TruthValue_MC))
		{
		TruthValue& Target = *static_cast<TruthValue*>(ArgArray[ArgIdx]);
		if (IsExactType(LogicalAND_MC))
			{
			if (!Target._x.could_be(true))
				{
				SelfEvalRule = None_SER;
				EvalRule = EvalForceArg_ER;
				return true;
				}
			}
		else if (Target._x.is(true))
			{
			SelfEvalRule = None_SER;
			EvalRule = EvalForceArg_ER;
			return true;
			}
		};
	return false;
}

void MetaConnective::DoSelfDeMorgan()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	// De Morgan's Law: NAND to OR, negate all arguments
	// or NOR to AND, negate all arguments
	assert(IsExactType(LogicalNOR_MC) || IsExactType(LogicalNAND_MC));
	static_assert(LogicalAND_MC==LogicalNOR_MC-6);
	static_assert(LogicalOR_MC==LogicalNAND_MC-6);

	SetExactTypeV2((ExactType_MC)(ExactType()-6));

	// Arg can be LogicalSelfNegated iff Metaconnective is syntactically correct:
	// LogicalSelfNegate it
	size_t i = fast_size();
	do	ArgArray[--i]->SelfLogicalNOT();
	while(0<i);
	ForceTotalLexicalArgOrder();
}

//! \todo OPTIMIZE: TIME
bool MetaConceptWithArgArray::ReplaceThisArgWithLeadArg()
{	// FORMALLY CORRECT: 10/22/1999
	// InferenceParameter1 is the Idx of the target arg
	// *other* instances are to be replaced
	LOG("(replacing later args in this by)");
	LOG(*ArgArray[InferenceParameter1]);
	LOG(*ArgArray[InferenceParameter1]->ArgN(0));
	LOG("(starting at)");
	LOG(*ArgArray[InferenceParameter2]);
	const MetaConceptWithArgArray& ArgArrayInfParam1 = *static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1]);

	InferenceParameter2++;
	do	if (    --InferenceParameter2!=InferenceParameter1
			&& !ArgArrayInfParam1.ReplaceNon1stArgWith1stArgInThisConcept(ArgArray[InferenceParameter2]))
			return false;
	while(0<InferenceParameter2);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

void
MetaConceptWithArgArray::LogicalANDORCondenseORANDArgHyperCubeAuxDim1(size_t Idx, size_t Idx2)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This may be called for AND, OR, or XOR
	// AND wants to merge OR clauses.  The merged arg is 2-ary AND.
	// OR wants to merge AND clauses.  The merged arg is 2-ary OR
	// XOR wants to merge AND clauses.  The merged arg is 2-ary IFF
	assert(Idx>Idx2);
	Digraph* const WorkingGraph = static_cast<Digraph*>((MetaConcept*)InferenceParameterMC);
	MetaConcept* Arg1 = WorkingGraph->ArgN(Idx);
	MetaConcept* Arg2 = WorkingGraph->ArgN(Idx2);
	WorkingGraph->RemoveVertex(Idx);
	WorkingGraph->RemoveVertex(Idx2);
	FindArgRelatedToLHS(*Arg1,AreSyntacticallyEqual);
	size_t TargetIdx1 = InferenceParameter1;
	FindArgRelatedToLHS(*Arg2,AreSyntacticallyEqual);
	size_t TargetIdx2 = InferenceParameter1;
	VERIFY(!NAryAllArgsEqualExceptOne(*ArgArray[TargetIdx1],*ArgArray[TargetIdx2]),AlphaCallAssumption);

	MetaConcept** NewArgArray = _new_buffer<MetaConcept*>(2);
	SUCCEED_OR_DIE(NewArgArray);

	LOG("Condensing");
	LOG(*ArgArray[TargetIdx1]);
	LOG(*ArgArray[TargetIdx2]);
	if 		(IsExactType(LogicalAND_MC))
		LOG("in AND-clause to");
	else if (IsExactType(LogicalOR_MC))
		LOG("in OR-clause to");
	else if (IsExactType(LogicalXOR_MC))
		LOG("in XOR-clause to");
	else
		UnconditionalCallAssumptionFailure();
	// ....
	static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->FastTransferOutAndNULL(static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->InferenceParameter1,NewArgArray[0]);
	static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx2])->FastTransferOutAndNULL(static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx2])->InferenceParameter1,NewArgArray[1]);
	MetaConcept* TempArg = NULL;
	try	{
		if 		(IsExactType(LogicalAND_MC))
			TempArg = new MetaConnective(NewArgArray,AND_MCM);
		else if (IsExactType(LogicalOR_MC))
			TempArg = new MetaConnective(NewArgArray,OR_MCM);
		else if (IsExactType(LogicalXOR_MC))
			{
			NewArgArray[1]->SelfLogicalNOT();
			TempArg = new MetaConnective(NewArgArray,IFF_MCM);
			}
		}
	catch(const bad_alloc&)
		{
		UnconditionalRAMFailure();
		}
	static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->TransferInAndOverwriteRaw(static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->InferenceParameter1,TempArg);
	LOG(*ArgArray[TargetIdx1]);

	if 		(2<fast_size())
		DeleteIdx(TargetIdx2);
	else // if (HasSameImplementationAs(*ArgArray[TargetIdx1]))	// certain
		{
		MetaConceptWithArgArray* Tmp = static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1]);
		ArgArray[TargetIdx1] = NULL;
		SetExactType(Tmp->ExactType());

		ArgArray = Tmp->ArgArray;

		InferenceParameterMC = Tmp->InferenceParameterMC;
		InferenceParameter1 = Tmp->InferenceParameter1;
		InferenceParameter2 = Tmp->InferenceParameter2;
		
		DELETE(Tmp);
		}
}

void MetaConceptWithArgArray::LogicalORXORCompactANDArgHyperCubeAuxDim1(size_t i, size_t i2)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	SUCCEED_OR_DIE(i>i2);
	Digraph* const WorkingGraph = static_cast<Digraph*>((MetaConcept*)InferenceParameterMC);
	MetaConcept* Arg1 = WorkingGraph->ArgN(i);
	MetaConcept* Arg2 = WorkingGraph->ArgN(i2);
	WorkingGraph->RemoveVertex(i);
	WorkingGraph->RemoveVertex(i2);
	FindArgRelatedToLHS(*Arg1,AreSyntacticallyEqual);
	size_t TargetIdx1 = InferenceParameter1;
	FindArgRelatedToLHS(*Arg2,AreSyntacticallyEqual);
	size_t TargetIdx2 = InferenceParameter1;
	SUCCEED_OR_DIE(static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->FindTwoRelatedArgs(*static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx2]),IsAntiIdempotentTo));

	LOG("Compacting");
	LOG(*ArgArray[TargetIdx1]);
	LOG(*ArgArray[TargetIdx2]);
	if 		(IsExactType(LogicalAND_MC))
		LOG("in AND-clause to");
	else if (IsExactType(LogicalOR_MC))
		LOG("in OR-clause to");
	else if (IsExactType(LogicalXOR_MC))
		LOG("in XOR-clause to");
	else
		UnconditionalCallAssumptionFailure();

	// AND: target is OR
	// OR, XOR: target is AND
	// same logic
	if (2==ArgArray[TargetIdx1]->size())
		{
		MetaConcept* Tmp = NULL;
		static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->FastTransferOutAndNULL(1-static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->InferenceParameter1,Tmp);
		delete ArgArray[TargetIdx1];
		ArgArray[TargetIdx1] = Tmp;
		}
	else{
		static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->DeleteIdx(static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1])->InferenceParameter1);
		}
	LOG(*ArgArray[TargetIdx1]);
	if 		(2<fast_size())
		DeleteIdx(TargetIdx2);
	else if (HasSameImplementationAs(*ArgArray[TargetIdx1]))
		{
		MetaConceptWithArgArray* Tmp = static_cast<MetaConceptWithArgArray*>(ArgArray[TargetIdx1]);
		ArgArray[TargetIdx1] = NULL;
		SetExactType(Tmp->ExactType());

		ArgArray = Tmp->ArgArray;

		InferenceParameterMC = Tmp->InferenceParameterMC;
		InferenceParameter1 = Tmp->InferenceParameter1;
		InferenceParameter2 = Tmp->InferenceParameter2;
		
		delete Tmp;
		}
}

void MetaConceptWithArgArray::HypercubeArgRelationProcessor(VertexPairSelfProcess CleanupAction)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
RestartCase1:
	Digraph* const WorkingGraph = static_cast<Digraph*>((MetaConcept*)InferenceParameterMC);
	size_t i = InferenceParameterMC->size();
	do	if (1==WorkingGraph->VertexUndirectedEdgeCount(--i))
			{	// good target: arity 1
			size_t j = InferenceParameterMC->size();
			do	if (--j!=i && WorkingGraph->ExplicitEdge(i,j))
					{	// match:
					if (i<j)
						(this->*CleanupAction)(j,i);
					else
						(this->*CleanupAction)(i,j);
					if (NULL==InferenceParameterMC) return;
					if (1>=InferenceParameterMC->size()) goto Terminate;
					goto RestartCase1;
					}
			while(0<j);
			}
	while(0<i);

	WorkingGraph->FlushVerticesWithUndirectedEdgeCountLTE(0);

	if (2<=InferenceParameterMC->size())
		{	// Pick something!
			// Below may malfunction at 4+ d; OK below that
		i = InferenceParameterMC->size();
		size_t ApparentMinimumEdgeCount = 0;
		do	{
			size_t CurrentEdgeCount = WorkingGraph->VertexUndirectedEdgeCount(--i);
			if (2==CurrentEdgeCount)
				{
				size_t j = i;
				do	if (   WorkingGraph->ExplicitEdge(i,--j)
						&& 2==WorkingGraph->VertexUndirectedEdgeCount(j))
						{	// belongs to a strict square, or connects two hypercubes.  Not dangerous.
						(this->*CleanupAction)(i,j);
						if (NULL==InferenceParameterMC) return;
						if (1>=InferenceParameterMC->size()) goto Terminate;
						goto RestartCase1;
						}
				while(0<j);
				};
			if (0==ApparentMinimumEdgeCount || ApparentMinimumEdgeCount>CurrentEdgeCount)
				ApparentMinimumEdgeCount = CurrentEdgeCount;
			}
		while(1<i);

		i = InferenceParameterMC->size();
		do	{
			if (ApparentMinimumEdgeCount==WorkingGraph->VertexUndirectedEdgeCount(--i))
				{
				size_t j = i;
				do	if (   WorkingGraph->ExplicitEdge(i,--j)
						&& ApparentMinimumEdgeCount==WorkingGraph->VertexUndirectedEdgeCount(j))
						{	// Connects two edges of minimum arity.  This is within an n-cube.  Probably safe.
						(this->*CleanupAction)(i,j);
						if (NULL==InferenceParameterMC) return;
						if (1>=InferenceParameterMC->size()) goto Terminate;
						goto RestartCase1;
						}
				while(0<j);
				}
			}
		while(1<i);

		// emergency algorithm
		i = InferenceParameterMC->size()-1;
		size_t j = i;
		do	if (WorkingGraph->ExplicitEdge(i,--j))
				{	// match:
				(this->*CleanupAction)(i,j);
				if (NULL==InferenceParameterMC) return;
				if (1>=InferenceParameterMC->size()) goto Terminate;
				i = 0;
				}
		while(0<j);
		goto RestartCase1;
		};
Terminate:
	InferenceParameterMC.reset();
}

bool MetaConceptWithArgArray::LogicalORXORCompactANDArgHyperCube()
{	// FORMALLY CORRECT: Kenneth Boyd, 2/24/2003
	// InferenceParameterMC contains the relevant graph
	// InferenceParameter1 is the highest dimension we can crunch
	assert(1<=InferenceParameter1 && 2>=InferenceParameter1);
	assert(IsExactType(LogicalAND_MC) || IsExactType(LogicalOR_MC) || IsExactType(LogicalXOR_MC));

	HypercubeArgRelationProcessor(&MetaConceptWithArgArray::LogicalORXORCompactANDArgHyperCubeAuxDim1);

	if (IsExactType(LogicalXOR_MC) && 2==fast_size())
		{
		ArgArray[1]->SelfLogicalNOT();
		SetExactTypeV2(LogicalIFF_MC);
		}
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool MetaConceptWithArgArray::LogicalANDORXORCondenseORANDANDArgHyperCube()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// InferenceParameterMC contains the relevant graph
	// InferenceParameter1 is the highest dimension we can crunch
	assert(1<=InferenceParameter1 && 2>=InferenceParameter1);
	assert(IsExactType(LogicalAND_MC) || IsExactType(LogicalOR_MC) || IsExactType(LogicalXOR_MC));

	HypercubeArgRelationProcessor(&MetaConceptWithArgArray::LogicalANDORCondenseORANDArgHyperCubeAuxDim1);

	if (IsExactType(LogicalXOR_MC) && 2==fast_size())
		{
		ArgArray[1]->SelfLogicalNOT();
		SetExactTypeV2(LogicalIFF_MC);
		}

	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

#if 0
bool
MetaConceptWithArgArray::XOROfANDDefinesXOR(void)
{	// TODO: VERIFY
	// InferenceParameter1 contains the cached results of which args to 
	// use.
	size_t* TargetIndexes = (size_t*)(InferenceParameter);
	size_t Idx = ArraySize(TargetIndexes);
	do	{
		MetaConcept* Tmp = NULL;
		Idx--;
		static_cast<MetaConceptWithArgArray*>(ArgArray[Idx])->TransferOutAndNULL(TargetIndexes[Idx],Tmp);
		DELETE(ArgArray[Idx]);
		ArgArray[Idx]=Tmp;
		}
	while(0<Idx);
	DELETE(TargetIndexes);
	return SelfEvalCleanEnd();
}

bool
MetaConceptWithArgArray::DistributeLogicalArgThroughThisArg(void)
{	// TODO: VERIFY
	// InferenceParameter1 args contain target argument
	// This may be invoked for distributivity of 
	// AND over XOR	[XOR(AND(A,..),AND(A,...),AND(A,...))
	// AND over OR [OR(AND(A,...),AND(A,...))
	// OR over AND [AND(OR(A,..),OR(A,...))
	// in any case, the top-level type is the type to be distributed over
	// the types of the next-lower-level are all the type to be distributed
	// result is probably 2-ary top-level type distributed
	MetaConceptWithArgArray* RHS = NULL;
	MetaConcept** TmpArgArray = _new_buffer<MetaConcept*>(2);
	if (NULL==TmpArgArray) return false;
	try	{
		RHS = new MetaConnective(LogicalAND_MCM);
		}
	catch(const bad_alloc&)
		{
		free(TmpArgArray);
		return false;
		}
	const ExactType_MC TopLevelType = ArgArray[0]->ExactType();
	static_cast<MetaConceptWithArgArray*>(ArgArray[0])->TransferOutAndNULL(static_cast<MetaConceptWithArgArray*>(ArgArray[0])->InferenceParameter1,TmpArgArray[0]);
	size_t Idx = fast_size();
	do	if (2==static_cast<MetaConceptWithArgArray*>(ArgArray[--Idx])->fast_size())
			{
			MetaConcept* Tmp = NULL;
			static_cast<MetaConceptWithArgArray*>(ArgArray[Idx])->TransferOutAndNULL(1-static_cast<MetaConceptWithArgArray*>(ArgArray[Idx])->InferenceParameter1,Tmp);
			DELETE(ArgArray[Idx]);
			ArgArray[Idx] = Tmp;
			}
		else{
			static_cast<MetaConceptWithArgArray*>(ArgArray[Idx])->DeleteIdx(static_cast<MetaConceptWithArgArray*>(ArgArray[Idx])->InferenceParameter1);
			}
	while(0<Idx);
	TmpArgArray[1]=RHS;
	RHS->ArgArray = ArgArray;
	RHS->SetExactTypeV2(ExactType());
	ArgArray = TmpArgArray;
	SetExactTypeV2(TopLevelType);
	return true;
}
#endif

bool MetaConceptWithArgArray::RetypeToMetaConnective(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	assert(!dest);
	dest = new MetaConnective(ArgArray,(MetaConnectiveModes)(InferenceParameter1-LogicalAND_MC));
	return true;
}

bool MetaConcept::TValStrictlyImpliesDefault(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/6/2000
	if (rhs.IsExactType(LogicalOR_MC))
		return static_cast<const MetaConnective&>(rhs).FindArgRelatedToLHS(*this,NonStrictlyImplies);
	return false;
}

bool
MetaConcept::TValStrictlyImpliesLogicalNOTOfDefault(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/6/2000
	if (rhs.IsExactType(LogicalAND_MC))
		return static_cast<const MetaConnective&>(rhs).FindArgRelatedToLHS(*this,NonStrictlyImpliesLogicalNOTOf);
	return false;
}

bool
MetaConcept::TValLogicalNOTOfStrictlyImpliesDefault(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/6/2000
	if (rhs.IsExactType(LogicalOR_MC))
		return static_cast<const MetaConnective&>(rhs).FindArgRelatedToLHS(*this,LogicalNOTOfNonStrictlyImplies);
	return false;
}

