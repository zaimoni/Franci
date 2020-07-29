// Clause2.cxx
// Implementation for Clause2Arg, the metatype for n-ary clauses

#include "Clause2.hxx"
#include "Class.hxx"
#include "MetaCon3.hxx"
#include "Unparsed.hxx"
#include "LowRel.hxx"
#include "Keyword1.hxx"

#include "Zaimoni.STL/except/syntax_error.hpp"
#include "Zaimoni.STL/lite_alg.hpp"

Clause2Arg::EvaluateToOtherRule Clause2Arg::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
	=	{
		&Clause2Arg::ConvertToMetaConnective
		};

// aux functions
static bool
InfixArglist2ArgsRecognize(const MetaConcept* const * ArgArray,size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/11/2000
	if (   ArgArray[KeywordIdx-1]->IsPotentialArg()
		&& ArgArray[KeywordIdx+1]->IsPotentialArg()
		&& ArgArray[KeywordIdx]->IsExactType(UnparsedText_MC)
		&& !static_cast<const UnparsedText*>(ArgArray[KeywordIdx])->IsSemanticChar())
		return true;
	return false;
}


// Infix keywords AND, OR, IFF, XOR, IMPLIES
// Duals: NAND, NOR, NIFF, NXOR, NIMPLIES
// NIFF==XOR, NXOR==IFF [but not for associativity/transitivity purposes!]

static const char* const Clause2AryKeywordLookup[MaxClause2Idx_MC-MinClause2Idx_MC+1]
  =	{
	LogicKeyword_AND,
	LogicKeyword_OR,
	LogicKeyword_IFF,
	LogicKeyword_XOR,
	LogicKeyword_IMPLIES,
	LogicKeyword_NIMPLIES,
	LogicKeyword_NOR,
	LogicKeyword_NAND
	};


// rest of implementation
Clause2Arg::Clause2Arg(MetaConcept**& src, size_t& KeywordIdx)
:	MetaConceptWith2Args(CanConstruct(src,KeywordIdx))
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/1999
	assert(MinClause2Idx_MC<=ExactType() && MaxClause2Idx_MC>=ExactType());
	ClauseKeyword = Clause2AryKeywordLookup[array_index()];
	ExtractInfixArglist(src,KeywordIdx);
}

//  Type ID functions
// FORMALLY CORRECT: 8/21/1999
const AbstractClass* Clause2Arg::UltimateType() const {return &TruthValues;}

// Syntactical equality and inequality
//  Evaluation functions

//! \todo FIX: This hard-codes infix 2-ary clauses with two args of UltimateType TruthValue
//! other paradigms will require moving the hard-coding to virtual functions.
bool Clause2Arg::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2000
	if (   (MetaConcept*)NULL!=LHS_Arg1
		&& LHS_Arg1->SyntaxOK()
		&& NULL!=ClauseKeyword
		&& (MetaConcept*)NULL!=RHS_Arg2
		&& RHS_Arg2->SyntaxOK()
		&& ForceUltimateTypeTruthValues(const_cast<MetaConcept*&>((const MetaConcept*&)LHS_Arg1))
		&& ForceUltimateTypeTruthValues(const_cast<MetaConcept*&>((const MetaConcept*&)RHS_Arg2)))
		return true;
	return false;
}

static ExactType_MC SelfLogicalNOTLookup_Clause2[(MaxClause2Idx_MC-MinClause2Idx_MC)+1]	=
	{	LogicalNAND_Clause2_MC,
		LogicalNOR_Clause2_MC,
		LogicalXOR_Clause2_MC,
		LogicalIFF_Clause2_MC,
		LogicalNIMPLIES_Clause2_MC,
		LogicalIMPLIES_Clause2_MC,
		LogicalOR_Clause2_MC,
		LogicalAND_Clause2_MC	};

// Formal manipulation functions
void Clause2Arg::SelfLogicalNOT()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/1999
	SetExactType(SelfLogicalNOTLookup_Clause2[array_index()]);
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > Clause2Arg::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

void Clause2Arg::DiagnoseInferenceRules()
{	// FORMALLY CORRECT: Kenneth Boyd, 9/11/1999
	if (!IdxCurrentEvalRule && !IdxCurrentSelfEvalRule)
		{
		SUCCEED_OR_DIE((in_range<LogicalAND_Clause2_MC,LogicalNAND_Clause2_MC>((unsigned long)ExactType())));
		IdxCurrentEvalRule = ConvertToMetaConnective_ER;
		}
}

bool Clause2Arg::DelegateEvaluate(MetaConcept*& dest)
{
	assert(MetaConceptWith2Args::MaxEvalRuleIdx_ER<IdxCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWith2Args::MaxEvalRuleIdx_ER>=IdxCurrentEvalRule);
	return (this->*EvaluateRuleLookup[IdxCurrentEvalRule-(MetaConceptWith2Args::MaxEvalRuleIdx_ER+1)])(dest);
}		// same, or different type

// type-specific functions
ExactType_MC Clause2Arg::CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: 2020-07-26
	// infix keywords
	assert(src);
	assert(ArraySize(src)>KeywordIdx);
	assert(src[KeywordIdx]);
	const auto VRKeyword = fast_up_cast<UnparsedText>(src[KeywordIdx]);
	if (!VRKeyword) return Unknown_MC;

	if (const auto GuessType = VRKeyword->TypeFor2AryClauseInfixKeyword())
		{
		if (ArraySize(src)<=KeywordIdx+1)
			{
			std::string error_msg("Clause has no space for arguments: ");
			error_msg += static_cast<const UnparsedText*>(src[KeywordIdx])->ViewKeyword();
			throw syntax_error(error_msg);
			};
		if (1<=KeywordIdx)
			{	// Keywords (N)AND, (N)OR, (N)IFF, (N)XOR, (N)IMPLIES
			if (InfixArglist2ArgsRecognize(src,KeywordIdx))
				{	// Franci needs to rule out ambiguities in both directions
				ExactType_MC ForwardGuess = Unknown_MC;
				ExactType_MC BackwardGuess = Unknown_MC;
				if (   ArraySize(src)>KeywordIdx+3
					&& InfixArglist2ArgsRecognize(src,KeywordIdx+2))
					ForwardGuess = static_cast<const UnparsedText*>(src[KeywordIdx+2])->TypeFor2AryClauseInfixKeyword();
				if (   3<=KeywordIdx
					&& InfixArglist2ArgsRecognize(src,KeywordIdx-2))
					BackwardGuess = static_cast<const UnparsedText*>(src[KeywordIdx-2])->TypeFor2AryClauseInfixKeyword();
				// META: generally, to be returned, (thiskeyword) must either be
				// higher-precedence, or equal and either self-associative or transitive.
				if		(Unknown_MC!=ForwardGuess)
					{
					if (Unknown_MC!=BackwardGuess)
						{	// __ (keyword) __ (thiskeyword) ___ (keyword) ___
						if (   GuessType!=BackwardGuess
							|| GuessType!=ForwardGuess)
							return Unknown_MC;	//! \todo precedence handling of __ (keyword) __ (thiskeyword) ___ (keyword) ___
						// Self-associative keywords will automatically clean up.
						if (SelfAssociative_LITBMP1MC & MetaConceptLookUp[GuessType].Bitmap1)
							return GuessType;
						// Transitive keywords can clean up.
						if (!(Transitive_LITBMP1MC & MetaConceptLookUp[GuessType].Bitmap1))
							return Unknown_MC;
						// handler code: multiple logic keywords, but rewrite forms () don't count
						// this is a problem for NXOR (which rewrites to IFF)
						if (   LogicalIFF_Clause2_MC==GuessType
							&& (   static_cast<const UnparsedText*>(src[KeywordIdx])->IsLogicKeyword(LogicKeyword_NXOR)
								|| static_cast<const UnparsedText*>(src[KeywordIdx-2])->IsLogicKeyword(LogicKeyword_NXOR)
								|| static_cast<const UnparsedText*>(src[KeywordIdx+2])->IsLogicKeyword(LogicKeyword_NXOR)))
							return Unknown_MC;
						}
					else{	// __ (thiskeyword) __ (keyword) ___
						if (GuessType!=BackwardGuess)
							return Unknown_MC;	//! \todo precedence handling of __ (thiskeyword) __ (keyword) ___
						// Self-associative keywords will automatically clean up.
						if (SelfAssociative_LITBMP1MC & MetaConceptLookUp[GuessType].Bitmap1)
							return GuessType;
						// Transitive keywords can clean up.
						if (!(Transitive_LITBMP1MC & MetaConceptLookUp[GuessType].Bitmap1))
							return Unknown_MC;
						// handler code: multiple logic keywords, but rewrite forms () don't count
						// this is a problem for NXOR (which rewrites to IFF)
						if (   LogicalIFF_Clause2_MC==GuessType
							&& (   static_cast<const UnparsedText*>(src[KeywordIdx])->IsLogicKeyword(LogicKeyword_NXOR)
								|| static_cast<const UnparsedText*>(src[KeywordIdx+2])->IsLogicKeyword(LogicKeyword_NXOR)))
							return Unknown_MC;
						}
					}
				else if (Unknown_MC!=BackwardGuess)
					{	// __ (keyword) __ (thiskeyword) ___
					if (GuessType!=BackwardGuess)
						return Unknown_MC;	//! \todo precedence handling of __ (keyword) __ (thiskeyword) ___
					// Self-associative keywords will automatically clean up.
					if (SelfAssociative_LITBMP1MC & MetaConceptLookUp[GuessType].Bitmap1)
						return GuessType;
					// Transitive keywords can clean up.
					if (!(Transitive_LITBMP1MC & MetaConceptLookUp[GuessType].Bitmap1))
						return Unknown_MC;
					// handler code: multiple logic keywords, but rewrite forms () don't count
					// this is a problem for NXOR (which rewrites to IFF)
					if (   LogicalIFF_Clause2_MC==GuessType
						&& (   static_cast<const UnparsedText*>(src[KeywordIdx])->IsLogicKeyword(LogicKeyword_NXOR)
							|| static_cast<const UnparsedText*>(src[KeywordIdx-2])->IsLogicKeyword(LogicKeyword_NXOR)))
						return Unknown_MC;
					};
				return GuessType;
				};
			}
		else{
			if (   LogicalIMPLIES_Clause2_MC==GuessType
				|| LogicalNIMPLIES_Clause2_MC==GuessType)
				{
				std::string error_msg("Clause has no space for arguments: ");
				error_msg += static_cast<const UnparsedText*>(src[KeywordIdx])->ViewKeyword();
				throw syntax_error(error_msg);
				};
			}
		};
	return Unknown_MC;
}

ExactType_MC
Clause2Arg::CanConstruct(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/21/1999
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

bool Clause2Arg::isAntiIdempotentTo(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/25/1999
	if (rhs.IsExactType(SelfLogicalNOTLookup_Clause2[array_index()]))
		return ExactOrderPairwiseRelation(static_cast<const Clause2Arg&>(rhs),AreSyntacticallyEqual);
	return false;
}

void
MetaConceptWith2Args::ExtractInfixArglist(MetaConcept**& Target, size_t& KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/19/2001
	// This is only supposed to be called from the constructor.
	RHS_Arg2 = Target[KeywordIdx+1];
	Target[KeywordIdx+1] = NULL;
	_delete_n_slots_at(Target,2,KeywordIdx);
	LHS_Arg1 = Target[KeywordIdx-1];
	Target[KeywordIdx-1] = this;

	if (LHS_Arg1->HasSimpleTransition() && !LHS_Arg1->IsExactType(ExactType()))
		DestructiveSyntacticallyEvaluateOnce(LHS_Arg1);
	if (RHS_Arg2->HasSimpleTransition() && !RHS_Arg2->IsExactType(ExactType()))
		DestructiveSyntacticallyEvaluateOnce(RHS_Arg2);

	SUCCEED_OR_DIE(SyntaxOK());

	ForceStdForm();

	--KeywordIdx;
}

void
MetaConceptWith2Args::ExtractPrefixArglist(MetaConcept**& Target, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/22/2003
	// This is only supposed to be called from the constructor.
	RHS_Arg2 = Target[KeywordIdx+4];
	Target[KeywordIdx+4] = NULL;
	LHS_Arg1 = Target[KeywordIdx+2];
	Target[KeywordIdx+2] = NULL;
	_delete_n_slots_at(Target,5,KeywordIdx+1);
	DELETE(Target[KeywordIdx]);
	Target[KeywordIdx] = this;
	if (LHS_Arg1->HasSimpleTransition() && !LHS_Arg1->IsExactType(ExactType()))
		DestructiveSyntacticallyEvaluateOnce(LHS_Arg1);
	if (RHS_Arg2->HasSimpleTransition() && !RHS_Arg2->IsExactType(ExactType()))
		DestructiveSyntacticallyEvaluateOnce(RHS_Arg2);

	SUCCEED_OR_DIE(SyntaxOK());

	ForceStdForm();
}

bool Clause2Arg::ConvertToMetaConnective(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	// This routine is called only from Evaluate, so it may destroy data in Clause2Arg.
	assert(!dest);
	SUCCEED_OR_DIE(LogicalAND_Clause2_MC<=ExactType() && LogicalNAND_Clause2_MC>=ExactType());
	// handler code
	// (N)AND, (N)OR, IFF, XOR: one set of code
	// (N)IMPLIES: two more sets.
	//		  [A IMPLIES B] iff [~A OR B]
	//		  [A NIMPLIES B] iff [~A NOR B]
	// inplace-rewrite is risky...
	weakautoarray_ptr_throws<MetaConcept*> NewArgArray(2);

	NewArgArray[0]=LHS_Arg1;
	NewArgArray[1]=RHS_Arg2;

	// MetaConnective constructor does not use dynamic RAM, so only the
	// actual MetaConnective RAM is at risk.
	if (IsExactType(LogicalIMPLIES_Clause2_MC) || IsExactType(LogicalNIMPLIES_Clause2_MC))
		{
		NewArgArray[0]->SelfLogicalNOT();
		dest = new MetaConnective(NewArgArray,(IsExactType(LogicalIMPLIES_Clause2_MC)) ? OR_MCM : NOR_MCM);
		}
	else{	//! \todo META: low-level typecast here.  We may want to explicitly define it, so it
			//! crashes if misused (this would be a call assumption violation)
		if (   IsExactType(LogicalIFF_Clause2_MC)
			&& (NewArgArray[0]->IsExactType(LogicalIFF_Clause2_MC) || NewArgArray[1]->IsExactType(LogicalIFF_Clause2_MC)))
			{
			while(NewArgArray[0]->IsExactType(LogicalIFF_Clause2_MC))
				{	// Clearing out left-based IFF transitivity
				if (!NewArgArray.InsertNSlotsAt(1,0))
					UnconditionalRAMFailure();

				{
				Clause2Arg* Tmp = static_cast<Clause2Arg*>(NewArgArray[1]);
				NewArgArray[0] = Tmp->LHS_Arg1.release();
				NewArgArray[1] = Tmp->RHS_Arg2.release();
				delete Tmp;
				};
				if (NewArgArray[1]->IsExactType(LogicalIFF_Clause2_MC))
					swap(NewArgArray[0],NewArgArray[1]);
				};
			while(NewArgArray[NewArgArray.ArraySize()-1]->IsExactType(LogicalIFF_Clause2_MC))
				{
				if (!NewArgArray.InsertNSlotsAt(1,NewArgArray.ArraySize()))
					UnconditionalRAMFailure();

				{
				Clause2Arg* Tmp = static_cast<Clause2Arg*>(NewArgArray[NewArgArray.ArraySize()-2]);
				NewArgArray[NewArgArray.ArraySize()-2] = Tmp->LHS_Arg1.release();
				NewArgArray[NewArgArray.ArraySize()-1] = Tmp->RHS_Arg2.release();
				delete Tmp;
				};
				if (NewArgArray[NewArgArray.ArraySize()-2]->IsExactType(LogicalIFF_Clause2_MC))
					swap(NewArgArray[NewArgArray.ArraySize()-2],NewArgArray[NewArgArray.ArraySize()-1]);
				};

			dest = new(nothrow) MetaConnective(NewArgArray,(MetaConnectiveModes)(LogicalIFF_Clause2_MC-LogicalAND_Clause2_MC));
			if (!dest) UnconditionalRAMFailure();

			LHS_Arg1.NULLPtr();
			RHS_Arg2.NULLPtr();
			assert(dest->SyntaxOK());
			return true;
			}
		dest = new MetaConnective(NewArgArray,(MetaConnectiveModes)(ExactType()-LogicalAND_Clause2_MC));
		};
	LHS_Arg1.NULLPtr();
	RHS_Arg2.NULLPtr();
	assert(dest->SyntaxOK());
	return true;
}

ExactType_MC
UnparsedText::TypeFor2AryClauseInfixKeyword(void) const
{	// FORMALLY CORRECT: 1/11/2000
	// Logic keywords section
	if (IsLogicKeyword())
		{
		unsigned int Idx = MaxClause2Idx_MC-MinClause2Idx_MC+1;
		do	if (!strcmp(Text,Clause2AryKeywordLookup[--Idx]))
				return (ExactType_MC)(MinClause2Idx_MC+Idx);
		while(0<Idx);
		};
	return Unknown_MC;
}

