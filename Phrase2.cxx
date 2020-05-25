// Phrase2.cxx
// Implementation for Phrase2Arg, the metatype for n-ary clauses

#include "Phrase2.hxx"
#include "Class.hxx"
#include "Unparsed.hxx"
#include "StdAdd.hxx"
#include "StdMult.hxx"
#include "Combin1.hxx"
#include "Integer1.hxx"
#include "LowRel.hxx"
#include "Keyword1.hxx"

#include "Zaimoni.STL/except/syntax_error.hpp"
#include "Zaimoni.STL/lite_alg.hpp"

// defined in LexParse.cxx
bool ImproviseVar(MetaConcept*& Target, const AbstractClass* Domain);
bool CoerceArgType(MetaConcept* const& Arg, const AbstractClass& ForceType);

Phrase2Arg::EvaluateToOtherRule Phrase2Arg::EvaluateRuleLookup[MaxEvalRuleIdx_ER]
	=	{
		&Phrase2Arg::ConvertToStdAddition,
		&Phrase2Arg::ConvertToStdMultiplication,
		&Phrase2Arg::ConvertToPermutation,
		&Phrase2Arg::ConvertToCombination
		};

// Infix keywords AND, OR, IFF, XOR, IMPLIES
// Duals: NAND, NOR, NIFF, NXOR, NIMPLIES
// NIFF==XOR, NXOR==IFF [but not for associativity/transitivity purposes!]

static const char* const Phrase2AryKeywordLookup[MaxPhrase2Idx_MC-MinPhrase2Idx_MC+1]
  =	{
	SymbolPlusSign,
	SymbolMultSign,
	PrefixKeyword_PERMUTATION,
	PrefixKeyword_COMBINATION
	};


// rest of implementation
Phrase2Arg::Phrase2Arg(MetaConcept**& src, size_t& KeywordIdx)
:	MetaConceptWith2Args(CanConstruct(src,KeywordIdx))
{	// FORMALLY CORRECT: Kenneth Boyd, 7/6/2001
	assert(MinPhrase2Idx_MC<=ExactType() && MaxPhrase2Idx_MC>=ExactType());
	PhraseKeyword = Phrase2AryKeywordLookup[ExactType()-MinPhrase2Idx_MC];
	switch(ExactType())
	{
	case SUM_Phrase2_MC:
		ExtractInfixArglist(src,KeywordIdx);
		return;
	case MULT_Phrase2_MC:
		ExtractInfixArglist(src,KeywordIdx);
		return;
	case PERMUTATION_Phrase2_MC:
		ExtractPrefixArglist(src,KeywordIdx);
		return;
	case COMBINATION_Phrase2_MC:
		ExtractPrefixArglist(src,KeywordIdx);
		return;
	default:
		FATAL(AlphaRetValAssumption);
		return;
	};
}

// Syntactical equality and inequality
//  Evaluation functions

bool Phrase2Arg::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/13/2000
	if (   !LHS_Arg1.empty() && LHS_Arg1->SyntaxOK()
		&& NULL!=PhraseKeyword
		&& !RHS_Arg2.empty() && RHS_Arg2->SyntaxOK())
		{
		if (IsExactType(SUM_Phrase2_MC))
			{
			DEBUG_LOG(LHS_Arg1->name());
			DEBUG_LOG(RHS_Arg2->name());
			if (   !LHS_Arg1->IsExactType(SUM_Phrase2_MC)
				&& !CoerceArgType(LHS_Arg1,ClassAdditionDefined))
				return false;
			if (   !RHS_Arg2->IsExactType(SUM_Phrase2_MC)
				&& !CoerceArgType(RHS_Arg2,ClassAdditionDefined))
				return false;
			return true;
			};
		if (IsExactType(MULT_Phrase2_MC))
			{	//! \bug autotyper is flawed.  We'll be fried at _Z_*S^1 domain signature,
				//! which is legitimate for trigonometry.
				//! We must permit integers to multiply anything that has + defined (this is a standard module action)
				//! Also, rings/fields will have actions over modules/vector spaces.
				//! Try AbstractClass member functions:
				//! * EnforceLeftModuleMultiplicationDomain
				//! * EnforceRightModuleMultiplicationDomain
			// Atomic case: we can proceed if
			// #1: both args have &middot; defined, or
			// #2: 1 arg is a ring under (+,&middot;) and the other has + defined.  [Actually, 
			// the chances of (+,&middot;) forming a ring structure is fairly high.]
			// For recursion purposes, Mult_Phrase2_MC always works, and proper classes always work.
			const bool LHSFree = LHS_Arg1->IsUltimateType(NULL) && !LHS_Arg1->IsExactType(MULT_Phrase2_MC);
			const bool RHSFree = RHS_Arg2->IsUltimateType(NULL) && !RHS_Arg2->IsExactType(MULT_Phrase2_MC);
			const bool LHSAutomatic = LHS_Arg1->IsExactType(MULT_Phrase2_MC) || (!LHSFree && LHS_Arg1->UltimateType()->IsProperClass());
			const bool RHSAutomatic = RHS_Arg2->IsExactType(MULT_Phrase2_MC) || (!RHSFree && RHS_Arg2->UltimateType()->IsProperClass());

			const bool LHSMultiplicationDefined = LHSAutomatic || (!LHSFree && LHS_Arg1->UltimateType()->Subclass(ClassMultiplicationDefined));
			const bool RHSMultiplicationDefined = RHSAutomatic || (!RHSFree && RHS_Arg2->UltimateType()->Subclass(ClassMultiplicationDefined));
			// &middot;/&middot; check
			if (LHSMultiplicationDefined && RHSMultiplicationDefined)
				return true;
			const bool LHSAdditionDefined = LHSAutomatic || (!LHSFree && LHS_Arg1->UltimateType()->Subclass(ClassAdditionDefined));
			const bool RHSAdditionDefined = RHSAutomatic || (!RHSFree && RHS_Arg2->UltimateType()->Subclass(ClassAdditionDefined));
			const bool LHSRing = LHSAutomatic || (!LHSFree && LHS_Arg1->UltimateType()->IsRingUnderOperationPair(StdAddition_MC,StdMultiplication_MC));
			const bool RHSRing = RHSAutomatic || (!RHSFree && RHS_Arg2->UltimateType()->IsRingUnderOperationPair(StdAddition_MC,StdMultiplication_MC));
			// ring/+ check
			if (   (LHSRing && (RHSAdditionDefined || RHSMultiplicationDefined))
				|| (RHSRing && (LHSAdditionDefined || LHSMultiplicationDefined))
				|| (LHSMultiplicationDefined && RHSMultiplicationDefined))
				return true;
			// improviser code.  This *is* tougher than it looks.
			if (LHSFree && RHSFree)
				{	// Both free...assume simplest case.
					//! \todo this is a nondeterministic parsing target.  Technically, all three
					//! cases should be parsed and interpreted in parallel.
				if (   !CoerceArgType(LHS_Arg1,ClassMultiplicationDefined)
					|| !CoerceArgType(RHS_Arg2,ClassMultiplicationDefined))
					return false;
				return true;
				}
			if (LHSFree)
				{
				if      (RHSRing)
					{
					if (	   !LHS_Arg1->ForceUltimateType(&ClassAdditionDefined)
						&&     !LHS_Arg1->ForceUltimateType(&ClassMultiplicationDefined)
						&& (   !LHS_Arg1->IsPotentialVarName()
							|| (   !ImproviseVar(const_cast<MetaConcept*&>((const MetaConcept*&)LHS_Arg1),&ClassAdditionDefined)
								&& !ImproviseVar(const_cast<MetaConcept*&>((const MetaConcept*&)LHS_Arg1),&ClassMultiplicationDefined))))
						return false;
					return true;
					}
				else if (RHSMultiplicationDefined)
					{
					return CoerceArgType(LHS_Arg1,ClassMultiplicationDefined);
					}
				else if (RHSAdditionDefined)
					{
					return CoerceArgType(LHS_Arg1,ClassAdditionMultiplicationDefined);
					}
				return false;
				};
			if (RHSFree)
				{
				if      (LHSRing)
					{
					if (	   !RHS_Arg2->ForceUltimateType(&ClassAdditionDefined)
						&&     !RHS_Arg2->ForceUltimateType(&ClassMultiplicationDefined)
						&& (   !RHS_Arg2->IsPotentialVarName()
							|| (   !ImproviseVar(const_cast<MetaConcept*&>((const MetaConcept*&)RHS_Arg2),&ClassAdditionDefined)
								&& !ImproviseVar(const_cast<MetaConcept*&>((const MetaConcept*&)RHS_Arg2),&ClassMultiplicationDefined))))
						return false;
					return true;
					}
				else if (LHSMultiplicationDefined)
					{
					return CoerceArgType(RHS_Arg2,ClassMultiplicationDefined);
					}
				else if (LHSAdditionDefined)
					{
					return CoerceArgType(RHS_Arg2,ClassAdditionMultiplicationDefined);
					}
				return false;
				};
			return false;
			};
		if (IsExactType(PERMUTATION_Phrase2_MC) || IsExactType(COMBINATION_Phrase2_MC))
			{	// require two args of type _Z_; infinite is bad for RHS
			if (   !CoerceArgType(RHS_Arg2,Integer)
				|| RHS_Arg2->IsNegative())
				return false;
			if (   LHS_Arg1->IsExactType(LinearInfinity_MC)
				&& LHS_Arg1->IsPositive())
				return true;
			return (CoerceArgType(LHS_Arg1,Integer) && !LHS_Arg1->IsNegative());
			}
		}
	return false;
}

// text I/O functions
const char* Phrase2Arg::ViewKeyword() const {return PhraseKeyword;}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > Phrase2Arg::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

// we have some constraints to make the diagnosis efficient
static_assert(1==MULT_Phrase2_MC-SUM_Phrase2_MC);
static_assert(2==PERMUTATION_Phrase2_MC-SUM_Phrase2_MC);
static_assert(3==COMBINATION_Phrase2_MC-SUM_Phrase2_MC);

// Formal manipulation functions
void Phrase2Arg::DiagnoseInferenceRules()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/6/2001
	// more constraints
	static_assert(Phrase2Arg::ConvertToCombination_ER-Phrase2Arg::ConvertToStdAddition_ER==COMBINATION_Phrase2_MC-SUM_Phrase2_MC);
	static_assert(Phrase2Arg::ConvertToPermutation_ER-Phrase2Arg::ConvertToStdAddition_ER==PERMUTATION_Phrase2_MC-SUM_Phrase2_MC);
	static_assert(Phrase2Arg::ConvertToStdMultiplication_ER-Phrase2Arg::ConvertToStdAddition_ER==MULT_Phrase2_MC-SUM_Phrase2_MC);
	if (!IdxCurrentEvalRule && !IdxCurrentSelfEvalRule)
		{
		if (in_range<SUM_Phrase2_MC,COMBINATION_Phrase2_MC>((unsigned long)ExactType()))
			{
			IdxCurrentEvalRule = (EvalRuleIdx_ER)((ExactType()-SUM_Phrase2_MC)+ConvertToStdAddition_ER);
			return;
			}
		UnconditionalDataIntegrityFailure();
		}
}

bool Phrase2Arg::InvokeEqualArgRule() {return false;}

bool Phrase2Arg::DelegateEvaluate(MetaConcept*& dest)
{
	assert(MetaConceptWith2Args::MaxEvalRuleIdx_ER<IdxCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWith2Args::MaxEvalRuleIdx_ER>=IdxCurrentEvalRule);
	return (this->*EvaluateRuleLookup[IdxCurrentEvalRule-(MetaConceptWith2Args::MaxEvalRuleIdx_ER+1)])(dest);
}		// same, or different type

// type-specific functions
ExactType_MC
Phrase2Arg::CanConstructNonPostfix(const MetaConcept* const * dest, size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// SUM_Phrase2_MC: 
	assert(dest);
	assert(ArraySize(dest)>KeywordIdx);
	assert(dest[KeywordIdx]);
	const UnparsedText* const Keyword = UnparsedText::fast_up_cast(dest[KeywordIdx]);
	if (!Keyword) return Unknown_MC;

	const ExactType_MC GuessType = Keyword->TypeFor2AryPhraseKeyword();
	if (Unknown_MC!=GuessType)
		{
		if (KeywordIdx+1>=ArraySize(dest))
			{
			std::string error_msg("Phrase has no space for arguments: ");
			error_msg += Keyword->ViewKeyword();
			throw syntax_error(error_msg);
			}
		// infix-handlers: sum, multiplication
		if (	SUM_Phrase2_MC==GuessType
			||	MULT_Phrase2_MC==GuessType)
			{
			if (1<=KeywordIdx)
				{
				if (   dest[KeywordIdx-1]->IsExactType(UnparsedText_MC)
					&& !static_cast<const UnparsedText*>(dest[KeywordIdx-1])->IsQuasiEnglishOrVarName())
					return Unknown_MC;
				if (   dest[KeywordIdx+1]->IsExactType(UnparsedText_MC)
					&& !static_cast<const UnparsedText*>(dest[KeywordIdx+1])->IsQuasiEnglishOrVarName())
					return Unknown_MC;

				if (SUM_Phrase2_MC==GuessType)
					{	// want __ + __, where __ is an expression suitable for +
					// commit if: both cannot left-extend and cannot right-extend
					// commit if: __ + __ + [left-associative], cannot left-extend
					if (   2<=KeywordIdx
						&& (   !dest[KeywordIdx-2]->IsExactType(UnparsedText_MC)
							|| !static_cast<const UnparsedText*>(dest[KeywordIdx-2])->ArgCannotExtendLeftThroughThis()))
						return Unknown_MC;			
					if (   KeywordIdx+2<ArraySize(dest)
						&& (   !dest[KeywordIdx+2]->IsExactType(UnparsedText_MC)  
							|| (   !static_cast<const UnparsedText*>(dest[KeywordIdx+2])->IsLogicalPlusSign()
								&& !static_cast<const UnparsedText*>(dest[KeywordIdx+2])->ArgCannotExtendRightThroughThis())))
						return Unknown_MC;
					return SUM_Phrase2_MC;
					};
				// MULT_Phrase2_MC:
				// do not let &middot; extend through + in either direction (precedence)
				if (MULT_Phrase2_MC==GuessType)
					{	// want __ &middot; __, where __ is an expression suitable for &middot;
					// commit if: both cannot left-extend and cannot right-extend
					// commit if: __ &middot; __ &middot; [left-associative], cannot left-extend
					if (   2<=KeywordIdx
						&& (   !dest[KeywordIdx-2]->IsExactType(UnparsedText_MC)
							|| (   !static_cast<const UnparsedText*>(dest[KeywordIdx-2])->IsLogicalPlusSign()
								&& !static_cast<const UnparsedText*>(dest[KeywordIdx-2])->ArgCannotExtendLeftThroughThis())))
						return Unknown_MC;			
					if (   KeywordIdx+2<ArraySize(dest)
						&& (   !dest[KeywordIdx+2]->IsExactType(UnparsedText_MC)  
							|| (   !static_cast<const UnparsedText*>(dest[KeywordIdx+2])->IsLogicalMultiplicationSign()
								&& !static_cast<const UnparsedText*>(dest[KeywordIdx+2])->IsLogicalPlusSign()
								&& !static_cast<const UnparsedText*>(dest[KeywordIdx+2])->ArgCannotExtendRightThroughThis())))
						return Unknown_MC;
					return MULT_Phrase2_MC;
					};
				}
			return Unknown_MC;
			}

		// interception of PERM(M,N) and Perm(M,N)
		// interception of COMB(M,N) and Comb(M,N)
		if (	PERMUTATION_Phrase2_MC==GuessType
			||	COMBINATION_Phrase2_MC==GuessType)
			{
			if (   KeywordIdx+5<ArraySize(dest)
				&& dest[KeywordIdx+1]->IsExactType(UnparsedText_MC)
				&& static_cast<const UnparsedText*>(dest[KeywordIdx+1])->IsSemanticChar('(')
				&& dest[KeywordIdx+3]->IsExactType(UnparsedText_MC)
				&& static_cast<const UnparsedText*>(dest[KeywordIdx+3])->IsSemanticChar(',')
				&& dest[KeywordIdx+5]->IsExactType(UnparsedText_MC)
				&& static_cast<const UnparsedText*>(dest[KeywordIdx+5])->IsSemanticChar(')'))
				{	// KeywordIdx+2, KeywordIdx+4 must pass
				if (   dest[KeywordIdx+2]->IsExactType(UnparsedText_MC)
					&& !static_cast<const UnparsedText*>(dest[KeywordIdx+2])->IsQuasiEnglishOrVarName())
					return Unknown_MC;
				if (   dest[KeywordIdx+4]->IsExactType(UnparsedText_MC)
					&& !static_cast<const UnparsedText*>(dest[KeywordIdx+4])->IsQuasiEnglishOrVarName())
					return Unknown_MC;
				return GuessType;			
				}
			if (   KeywordIdx+2<ArraySize(dest)
				&& (	!dest[KeywordIdx+1]->IsExactType(UnparsedText_MC)
					|| 	!static_cast<const UnparsedText*>(dest[KeywordIdx+1])->IsSemanticChar('(')))
				{
				std::string error_msg("Malformed prefix phrase: ");
				error_msg += Keyword->ViewKeyword();
				throw syntax_error(error_msg);
				}
			return Unknown_MC;
			}
		}
	return Unknown_MC;
}

ExactType_MC
Phrase2Arg::CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx)
{	// FORMALLY CORRECT: 7/20/2003
	return Unknown_MC;
}

ExactType_MC
Phrase2Arg::CanConstruct(const MetaConcept* const * src, size_t KeywordIdx)
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

bool Phrase2Arg::ConvertToStdAddition(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	// This routine is called only from Evaluate, so it may destroy data in Phrase2Arg.
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	assert(!dest);
	assert(IsExactType(SUM_Phrase2_MC));
	weakautoarray_ptr_throws<MetaConcept*> NewArgArray(2);

	if (LHS_Arg1->IsPotentialVarName())
		ImproviseVar(LHS_Arg1,&ClassAdditionDefined);
	if (RHS_Arg2->IsPotentialVarName())
		ImproviseVar(RHS_Arg2,&ClassAdditionDefined);

	NewArgArray[0]=LHS_Arg1;
	NewArgArray[1]=RHS_Arg2;
	if (   !LHS_Arg1->IsExactType(SUM_Phrase2_MC)
		&& !RHS_Arg2->IsExactType(SUM_Phrase2_MC)
		&& !LHS_Arg1->IsExactType(StdAddition_MC)
		&& !RHS_Arg2->IsExactType(StdAddition_MC))
		{
		DEBUG_LOG("Nonrecursive exit");
		dest = new StdAddition(NewArgArray);
		LHS_Arg1.NULLPtr();
		RHS_Arg2.NULLPtr();
		assert(dest->SyntaxOK());
		return true;
		};

	// embedded SUM_Phrase2_MC types
	LHS_Arg1.NULLPtr();
	RHS_Arg2.NULLPtr();
	size_t i = 2;
	do	{
		--i;
		if (NewArgArray[i]->IsExactType(SUM_Phrase2_MC))
			{
			if (!NewArgArray.InsertNSlotsAt(1,i+1))
				UnconditionalRAMFailure();

			Phrase2Arg* TmpArg = static_cast<Phrase2Arg*>(NewArgArray[i]);
			NewArgArray[i++] = TmpArg->LHS_Arg1.release();
			NewArgArray[i++] = TmpArg->RHS_Arg2.release();
			delete TmpArg;
			}
		else if (NewArgArray[i]->IsExactType(StdAddition_MC) && NewArgArray[i]->NoMetaModifications())
			{
			MetaConceptWithArgArray* TmpArg = static_cast<MetaConceptWithArgArray*>(NewArgArray[i]);
			const size_t ArgArity = TmpArg->size();
			if (!NewArgArray.InsertNSlotsAt(ArgArity-1,i+1))
				UnconditionalRAMFailure();

			size_t j = ArgArity;
			do	{
				j--;
				TmpArg->TransferOutAndNULL(j,NewArgArray[i+j]);
				}
			while(0<j);
			delete TmpArg;
			i += ArgArity;
			}
		}
	while(0<i);

	try	{
		dest = new StdAddition(NewArgArray);
		}
	catch(const bad_alloc&)
		{
		NewArgArray.clear();
		UnconditionalRAMFailure();
		};
	assert(dest->SyntaxOK());
	return true;
}

bool Phrase2Arg::ConvertToStdMultiplication(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	// This routine is called only from Evaluate, so it may destroy data in Phrase2Arg.
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	assert(!dest);
	assert(IsExactType(MULT_Phrase2_MC));
	weakautoarray_ptr_throws<MetaConcept*> NewArgArray(2);

	if (LHS_Arg1->IsPotentialVarName())
		ImproviseVar(LHS_Arg1,&ClassMultiplicationDefined);
	if (RHS_Arg2->IsPotentialVarName())
		ImproviseVar(RHS_Arg2,&ClassMultiplicationDefined);

	NewArgArray[0]=LHS_Arg1;
	NewArgArray[1]=RHS_Arg2;
	if (   !LHS_Arg1->IsExactType(MULT_Phrase2_MC)
		&& !RHS_Arg2->IsExactType(MULT_Phrase2_MC))
		{
		DEBUG_LOG("Nonrecursive exit");
		dest = new StdMultiplication(NewArgArray);
		LHS_Arg1.NULLPtr();
		RHS_Arg2.NULLPtr();
		assert(dest->SyntaxOK());
		return true;
		};

	// embedded MULT_Phrase2_MC types
	LHS_Arg1.NULLPtr();
	RHS_Arg2.NULLPtr();
	size_t i = 2;
	do	{
		--i;
		if (NewArgArray[i]->IsExactType(MULT_Phrase2_MC))
			{
			if (!NewArgArray.InsertNSlotsAt(1,i+1))
				UnconditionalRAMFailure();

			Phrase2Arg* TmpArg = static_cast<Phrase2Arg*>(NewArgArray[i]);
			NewArgArray[i++] = TmpArg->LHS_Arg1.release();
			NewArgArray[i++] = TmpArg->RHS_Arg2.release();
			delete TmpArg;
			};
		}
	while(0<i);

	dest = new(nothrow) StdMultiplication(NewArgArray);
	if (!dest)
		{
		NewArgArray.clear();
		UnconditionalRAMFailure();
		};
	assert(dest->SyntaxOK());
	return true;
}

bool Phrase2Arg::ConvertToPermutation(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	// This routine is called only from Evaluate, so it may destroy data in Phrase2Arg.
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	assert(!dest);
	assert(IsExactType(PERMUTATION_Phrase2_MC));
	weakautoarray_ptr_throws<MetaConcept*> NewArgArray(2);

	if (LHS_Arg1->IsPotentialVarName()) ImproviseVar(LHS_Arg1,&Integer);
	if (RHS_Arg2->IsPotentialVarName()) ImproviseVar(RHS_Arg2,&Integer);
	NewArgArray[0]=LHS_Arg1;
	NewArgArray[1]=RHS_Arg2;

	dest = new CombinatorialLike(NewArgArray,PERMUTATIONCOUNT_CM);
	LHS_Arg1.NULLPtr();
	RHS_Arg2.NULLPtr();
	assert(dest->SyntaxOK());
	return true;
}

bool Phrase2Arg::ConvertToCombination(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	// This routine is called only from Evaluate, so it may destroy data in Phrase2Arg.
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	assert(!dest);
	assert(IsExactType(COMBINATION_Phrase2_MC));
	weakautoarray_ptr_throws<MetaConcept*> NewArgArray(2);

	if (LHS_Arg1->IsPotentialVarName()) ImproviseVar(LHS_Arg1,&Integer);
	if (RHS_Arg2->IsPotentialVarName()) ImproviseVar(RHS_Arg2,&Integer);
	NewArgArray[0]=LHS_Arg1;
	NewArgArray[1]=RHS_Arg2;

	dest = new CombinatorialLike(NewArgArray,COMBINATIONCOUNT_CM);
	LHS_Arg1.NULLPtr();
	RHS_Arg2.NULLPtr();
	assert(dest->SyntaxOK());
	return true;
}

ExactType_MC
UnparsedText::TypeFor2AryPhraseKeyword(void) const
{	// FORMALLY CORRECT: 1/11/2000
	// Logic keywords section
	if (IsLogicalPlusSign()) return SUM_Phrase2_MC;
	if (IsLogicalMultiplicationSign()) return MULT_Phrase2_MC;
	if (	IsPrefixKeyword(PrefixKeyword_PERMUTATION)
		||	IsPrefixKeyword(PrefixKeyword_PERMUTATION_ALT))
		return PERMUTATION_Phrase2_MC;
	if (	IsPrefixKeyword(PrefixKeyword_COMBINATION)
		||	IsPrefixKeyword(PrefixKeyword_COMBINATION_ALT))
		return COMBINATION_Phrase2_MC;
	return Unknown_MC;
}
