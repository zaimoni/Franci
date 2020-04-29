// InParse.cxx
// former implementation for InParse, the type for an incompletely parsed object
// this implements the parsing for FranciScript

#include "SymConst.hxx"
#include "Class.hxx"
#include "InParse.hxx"
#include "Quantify.hxx"
#include "Unparsed.hxx"
#include "Integer1.hxx"
#include "Phrase1.hxx"
#include "Phrase2.hxx"
#include "PhraseN.hxx"
#include "Clause2.hxx"
#include "ClauseN.hxx"
#include "LowRel.hxx"
#include "Keyword1.hxx"

#include "Zaimoni.STL/lite_alg.hpp"
#include "Zaimoni.STL/LexParse/Parser.hpp"
#include "Zaimoni.STL/Pure.C/logging.h"

#include <functional>

extern Parser<MetaConcept> FranciScriptParser;

// defined in LenName.cxx
size_t LengthOfPrefixArgList(const MetaConcept * const * const ArgArray);
void ConstructPrefixArgList(char* const PrefixArgListStart,const MetaConcept* const * const ArgArray);

// defined in MetaCon.cxx
void NiceFormat80Cols(char*& Name);


// NOTE: Parsing extension
// Want A...B, where A,B "will evaluate" to explicit constants of type IntegerNumeral, 
// to expand in arglist context to the integers from A to B inclusive.
// ... must be any of : 3 consecutive periods, ellipsis character, HTML entities.
// Dually, n-ary types faced with 3+ consecutive IntegerNumerals should convert to the 
// A...B notation when printing out.
// Well-formed is A<B, but Franci will understand A>=B and take appropriate action.
// NOTE: the A...B construction may show up in other places as well.  It's probably worth its 
// own 2-ary type: MathInterval.  [Note that UltimateType, and strict inclusion of both 
// endpoints must be specified; we have *very* different interpretations for A...B, [A,B],
// ]A,B[ i.e. (A,B), [A,B) i.e. [A,B[, and (A,B] i.e. ]A,B].  We also need domain-forcing
// options.
// Whether Franci prints intervals as conventional or Stromberg should be a controllable 
// option.  NOTE: f:(A,B) |-> _C_ will cause parsing problems: is (A,B) an open interval, or 
// a point?
//! \todo command that sets/resets LinearInterval::KarlStrombergIntervals (LexParse.cxx)
//! \todo LinearInterval with UltimateType _Z_ should be understood by various EqualRelation 
//! types (memory conservation).  This poses problems with arity calculations.
//! \todo GCF also should understand LinearInterval with UltimateType _Z_.  However, this is 
//! normally a trivialization: it should happen only with raw data input.

// defined in LexParse.cxx
bool ImproviseVar(MetaConcept*& Target, const AbstractClass* Domain);
bool LookUpVar(MetaConcept*& Target, const AbstractClass* Domain);

// temporary forward declares

// #define FRANCI_WARY 1

//! \bug META: Parser<MetaConcept> currently does not support nondeterministic
//! parsing.  This is not a problem right now, but could cause serious limitations.
//! ** Franci should be aware of 'reflex' and 'nondetermistic' parsing rules
//! ** 'Reflex' rules can be used immediately without endangering semantics.  E.g., 0-ary keywords.
//!    these could even be viewed as "semantically const" operations (and thus go off in the
//!    CanEvaluate routine).
//! ** 'Nondeterministic' rules must be used in parallel.  The results must be unified
//!    after each iteration.  Analyses that have no further parsings are parked.
//!    ** A list of minimal parsings is maintained.
//!    ** If this list of minimal parsings has one entry, the input is considered to be
//!       unambiguous.  Otherwise, the input is considered to be ambiguous.
//!    ** If the list consists of a single MetaConcept, the input is considered to be
//!       fully understandable.  Otherwise, the input is not fully understood.

//		*	0-ary Keywords: TRUE, FALSE, UNKNOWN, CONTRADICTION [TruthValue]
//		*	n-ary prefix Keywords: AND(), OR(), NAND(), NOR() IFF(), XOR() [MetaConnective]
//		*	Infix Keywords: ___ AND ___, ___ OR ___, ___ IFF ___ [MetaConnective]
//		*	Keywords: IF ___ THEN ___, NOT ___, ____ ONLY IF ___, ____ IF AND ONLY IF ____
//			[define in terms of software operations]
// this is the target for parsing global:
//	... FREE: n FREE variables
//  FORALL|THEREIS|NOTFORALL|THEREISNO .. IN ..: n bound variables
// probably should mediate concatenation of identical quantifier, identical domain
// We must allow general ID (0-ary or n-ary) followed by :=
// At preliminary stages, Franci is interested in:
//	*	Balanced (), [], {}								// ready
//	*	Balanced space-' '-space, space-" "-space		// not ready
//	*	PreEnglishNumeric tokens (also pre-variable)	// ready
//	*	PreSymbolic tokens								// not ready
//	*	PreVariable tokens (include all PreEnglish tokens)	// ready
//	*	index variable specifications: (PreEnglish)=(PreEnglish)...(PreEnglish)
//	*									(PreEnglish)<=(Prevariable),...<=(PreEnglish)
//	*									(PreEnglish)<=(Prevariable)<=...<=(PreEnglish)
//	*	, [syntactical information]
//	*	:=	(defines operator)

bool InterpretOneStage(MetaConcept**& ArgArray)
{
	autotransform_n(ArgArray, SafeArraySize(ArgArray), [](MetaConcept* x) {return x->ForceStdForm(); });
	return FranciScriptParser.ParseOneStep(ArgArray);
}

void Franci_SyntaxError(void)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/26/2005
	//! \todo FIX: need a quasi-random selector of phrases.  This selector should "want" to avoid repeating itself too often.
	//! \todo FIX: Franci needs to explain the syntactical errors.  This requires
	//! modifying the Interface.  Alternatively, SyntaxOK could log the errors in real-time.
	WARNING("I'm sorry, but that has syntactical errors.");
}

void Franci_IncompleteUnderstanding(void)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/26/2005
	//! \todo FIX: tell parser to check for stalled rules
	//! \todo FIX: need a quasi-random selector of phrases.  This selector should "want" to avoid repeating itself too often.
	WARNING("I don't completely understand that.");
}

#if 0
bool
Franci_IsSyntacticalSymmetry(const MetaConcept* const * const ArgArray)
{	//! \todo IMPLEMENT
	// Symmetry has to have the form Hypothesis,Conclusion where
	// Hypothesis is an AND of OR clauses
	// Conclusion is an AND of IFF clauses (should extend to ALLEQUAL)
	// Properly programmed, the hypothesis evaluates to FALSE when anded with a what-if
	// that breaks its symmetry.
	if (   3==SafeArraySize(ArgArray)
		&& ArgArray[0]->IsExactType(LogicalAND_MC)
		&& ArgArray[1]->IsExactType(UnparsedText_MC)
		&& ArgArray[2]->IsExactType(LogicalAND_MC))
//! \todo IMPLEMENT MetaConnective::LogicalANDSymmetryHypothesis(), MetaConnective::LogicalANDSymmetryConclusion()
//		&& static_cast<MetaConnective*>(ArgArray[0])->LogicalANDSymmetryHypothesis()
//		&& static_cast<MetaConnective*>(ArgArray[2])->LogicalANDSymmetryConclusion())
		return true;
	return false;
}
#endif

void LogThis(const MetaConcept * const * const ArgArray)
{
	if (is_logfile_active())
		{
		autoarray_ptr<char> Tmp(LengthOfPrefixArgList(ArgArray));
		if (NULL!=Tmp)
			{
			ConstructPrefixArgList(Tmp,ArgArray);
			NiceFormat80Cols(Tmp);
			LOG(Tmp);
			}
		else
			LOG("Attempt to log concept suffered RAM failure.\n");
		};
}

void SayThisNormally(const MetaConcept * const * const ArgArray)
{
	autoarray_ptr<char> Tmp(LengthOfPrefixArgList(ArgArray));
	if (NULL!=Tmp)
		{
		ConstructPrefixArgList(Tmp,ArgArray);
		NiceFormat80Cols(Tmp);
		INFORM(Tmp);
		}
	else
		INFORM("Attempt to state concept suffered RAM failure.\n");
}


// start specific definitions (shove into include eventually)
//#define FRANCI_WARY 1

static void _PostFilterPhraseClause(MetaConcept*& dest)
{
	assert(dest);
	assert((in_range<MinClausePhraseIdx_MC,MaxClausePhraseIdx_MC>((unsigned long)dest->ExactType())));
	if (dest->HasSimpleTransition() && dest->CanEvaluate())
		{
#ifdef FRANCI_WARY
		LOG("Attempting DestructiveSyntacticallyEvaluateOnce");
		LOG(*dest);		
#endif
		SUCCEED_OR_DIE(DestructiveSyntacticallyEvaluateOnce(dest));
#ifdef FRANCI_WARY
		LOG("DestructiveSyntacticallyEvaluateOnce OK");
#endif
		};
}

static bool NOT_ForcesTruthValueVariable(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (   1<=i
		&& ArgArray[i-1]->IsExactType(UnparsedText_MC)
		&& static_cast<UnparsedText*>(ArgArray[i-1])->IsLogicKeyword(LogicKeyword_NOT))
		{	// NOT ___
		//! \todo FIX: needs to respond to NOT a whole lot of ... [NOT of a sentence]
		if (   ArgArray[i]->IsPotentialVarName()
			&& ArgArray[i]->IsUltimateType(NULL))
			ImproviseVar(ArgArray[i],&TruthValues);
		if (ArgArray[i]->SelfLogicalNOTWorks())
			{
			ArgArray[i]->SelfLogicalNOT();
			_delete_idx(ArgArray,i-1);
			assert(ValidateArgArray(ArgArray));
			return true;
			};
		};
	return false;
}

static bool _PostfilterUnparsedText(MetaConcept*& Target)
{
	assert(Target);
	assert(Target->IsExactType(UnparsedText_MC));
	while(Target->DestructiveEvaluateToSameType());
	if (Target->CanEvaluate())
		{
		SUCCEED_OR_DIE(DestructiveSyntacticallyEvaluateOnce(Target));
		Target->ForceStdForm();
		return true;
		}
	return false;
}

static bool _SplitTextInTwo(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (_insert_slot_at(ArgArray,i+1,(MetaConcept*)NULL))
		{
		if (static_cast<UnparsedText*>(ArgArray[i])->SplitIntoTwoTexts(ArgArray[i+1]))
			{
			_PostfilterUnparsedText(ArgArray[i]);
			_PostfilterUnparsedText(ArgArray[i+1]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		_delete_idx(ArgArray,i+1);
		assert(ValidateArgArray(ArgArray));
		}
	return false;
}

static bool ResolveUnparsedText(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (ArgArray[i]->IsExactType(UnparsedText_MC))
		{
		// META: This is the complete reaction set for 0-ary UnparsedText
		if (_PostfilterUnparsedText(ArgArray[i])) return true;

		UnparsedText& VR_ArgArrayIdx = *static_cast<UnparsedText*>(ArgArray[i]);
		// META: This is the complete reaction set for unclassified UnparsedText
		// if an UnparsedText item is the ONLY entry, check to see if it's a
		// variable name that has already been declared.  This is an inplace-rewrite.
		if (   1==ArraySize(ArgArray)	// implies 0==Idx
			&& VR_ArgArrayIdx.IsQuasiEnglishOrVarName())
			{
			LookUpVar(ArgArray[0],NULL);	// requests a free variable
			return true;
			};
		if (VR_ArgArrayIdx.IsUnclassified())	// for currently-unclassified text
			return _SplitTextInTwo(ArgArray,i);

		// META: This is the complete reaction set for LeadingIntegerNumeral UnparsedText
		if (VR_ArgArrayIdx.IsLeadingIntegerNumeral())
			{	// if QuasiEnglishNumeric starts with a legal IntegerNumeral string,
				// then has a mere ID character character afterwards, then split off the numeral
				// [this is a shorthand in math, and a grammar-fixer in English]
			size_t SplitLength = VR_ArgArrayIdx.LengthOfNumericIntegerToSplitOff();
			SUCCEED_OR_DIE(0<SplitLength);	// data integrity error otherwise
			return _SplitTextInTwo(ArgArray,i);
			};
		if (VR_ArgArrayIdx.IsLogicalInfinity())
			{	// Infinity symbol (easy form)
			// Let autotyping promote LinearInfinity_MC to ComplexInfinity_MC
			// then always interpreting as LinearInfinity_MC isn't a problem
			try	{
				MetaConcept* Tmp = new SymbolicConstant(LinearInfinity_SC);
				DELETE(ArgArray[i]);
				ArgArray[i] = Tmp;
				assert(ValidateArgArray(ArgArray));
				return true;
				}
			catch(const bad_alloc&)
				{
				return false;
				}
			};
		// ], ) key stripper rules
		// these rules are *not* exhaustive; they need enough space behind them to work
		if (2<=i && ArgArray[i-2]->IsExactType(UnparsedText_MC))
			{
			// [ ] strip
			// ( ) strip
			const UnparsedText& VR_ArgArrayIdxMinus2 = *static_cast<UnparsedText*>(ArgArray[i-2]);
			if (   (VR_ArgArrayIdx.IsCharacter(']') && VR_ArgArrayIdxMinus2.IsCharacter('['))
				|| (VR_ArgArrayIdx.IsCharacter(')') && VR_ArgArrayIdxMinus2.IsCharacter('(')))
				{	// This cleans up those clauses/phrases that can evaluate;
				if (in_range<MinClausePhraseIdx_MC,MaxClausePhraseIdx_MC>((unsigned long)ArgArray[i-1]->ExactType()))
					{	// if this clause/phrase *doesn't* evaluate, stall this section
					if (!ArgArray[i-1]->CanEvaluate())
						return false;

					DestructiveSyntacticallyEvaluateOnce(ArgArray[i-1]);
					}

				// What's inside is *not* a clause/phrase, it has an interpretation
				bool NoLeftExtend = 	2==i
									|| (   ArgArray[i-3]->IsExactType(UnparsedText_MC)
				    					&& static_cast<UnparsedText*>(ArgArray[i-3])->ArgCannotExtendLeftThroughThis());
				bool NoRightExtend =	i+1==ArraySize(ArgArray)
									|| (   ArgArray[i+1]->IsExactType(UnparsedText_MC)
										&& static_cast<UnparsedText*>(ArgArray[i+1])->ArgCannotExtendRightThroughThis());
				//! \todo FIX: deconflict this with integer floor function; matrices will behave via isomorphism
				if (NoLeftExtend && NoRightExtend)
					{
SaveParenBracketStrip:
					_delete_idx(ArgArray,i);
					_delete_idx(ArgArray,i-2);
					assert(ValidateArgArray(ArgArray));
					return true;
					};
				// MUTABLE
				// +/&middot;-compatible is legitimate to strip when both sides are 
				// bounded by '+'-signs, &middot;-signs, or no-extends
				if (   NULL!=ArgArray[i-1]->UltimateType()
					&& (	ArgArray[i-1]->UltimateType()->Subclass(ClassAdditionDefined)
						||	ArgArray[i-1]->UltimateType()->Subclass(ClassMultiplicationDefined)))
					{
					if 		(NoLeftExtend)
						{
						if (   ArgArray[i+1]->IsExactType(UnparsedText_MC)
				    		&& (	static_cast<UnparsedText*>(ArgArray[i+1])->IsLogicalPlusSign()
								|| 	static_cast<UnparsedText*>(ArgArray[i+1])->IsLogicalMultiplicationSign()))
							goto SaveParenBracketStrip;
						}
					else if (NoRightExtend)
						{
						if (   ArgArray[i-3]->IsExactType(UnparsedText_MC)
				    		&& (	static_cast<UnparsedText*>(ArgArray[i-3])->IsLogicalPlusSign()
								||	static_cast<UnparsedText*>(ArgArray[i-3])->IsLogicalMultiplicationSign()))
							goto SaveParenBracketStrip;
						}
					else{
						if (   ArgArray[i-3]->IsExactType(UnparsedText_MC)
				    		&& (	static_cast<UnparsedText*>(ArgArray[i-3])->IsLogicalPlusSign()
								||	static_cast<UnparsedText*>(ArgArray[i-3])->IsLogicalMultiplicationSign())
							&& ArgArray[i+1]->IsExactType(UnparsedText_MC)
				    		&& (	static_cast<UnparsedText*>(ArgArray[i+1])->IsLogicalPlusSign()
								||	static_cast<UnparsedText*>(ArgArray[i+1])->IsLogicalMultiplicationSign()))
							goto SaveParenBracketStrip;									
						};
					};
				};
			};
		};
	return false;
}

//! \todo several modalities should be supported for quantifiers<br>
//! (_)(_)...(_)(statement) : those in front quantify statement (anywhere)<br>
//! (_)(_)...(_): statement : those in front quantify statement (top-level)<br>
//! _;_;...;_ : statement : those in front quantify statement (top-level)<br>
//! _,_,...,_ : statement : those in front quantify statement (top-level)<br>
//! statement: (_)(_)...(_) : those in back quantify statement (top-level)<br>
//! statement: _;_;...;_ : those in back quantify statement (top-level)<br>
//! statement: _,_,...,_ : those in back quantify statement (top-level)<br>
//! Q: where should this be relative to Parentheses-strip??
static bool _ConstructQuantifierListAux(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	const bool IN_Phrase = ArgArray[i]->IsExactType(IN_Phrase1_MC);
	MetaConcept* Tmp2 = (IN_Phrase) ? ArgArray[i--] : NULL;
	MetaConcept* Tmp = ArgArray[i];
	ArgArray[i] = NULL;
	if (IN_Phrase) ArgArray[i+1] = NULL;

	// Tmp now points to a quantifier-phrase.
	// Insert vars at InferenceParameter1
	// if not FREE-phrase, Tmp2 points to domain via IN-clause.
	const MetaQuantifierMode QuantifierType = (MetaQuantifierMode)(Tmp->ExactType()-FORALL_PhraseN_MC);
	const size_t VarCount = Tmp->size(); 
	const size_t BaseArity = (IN_Phrase) ? 2 : 1;
	if		(VarCount>BaseArity)
		{
		if (!_insert_n_slots_at(ArgArray,VarCount-BaseArity,i))
			{
			ArgArray[i] = Tmp;
			if (IN_Phrase) ArgArray[i+1] = Tmp2;
			return false;
			}
		}
	else if (VarCount<BaseArity) _delete_idx(ArgArray,i);

	// Now the # of slots = the # of args in Tmp
	// Tmp is type PhraseNArg
	try	{
		size_t j = VarCount;
		do	{
			MetaConcept* Tmp3 = NULL;
			static_cast<MetaConceptWithArgArray*>(Tmp)->TransferOutAndNULL(--j,Tmp3);
			assert(Tmp3->IsExactType(UnparsedText_MC));
			ArgArray[i+j] = new MetaQuantifier(static_cast<UnparsedText*>(Tmp3)->ViewKeyword(),
												(IN_Phrase) ? static_cast<AbstractClass*>(Tmp2->ArgN(0)) :  NULL,
												QuantifierType);
			delete Tmp3;
			}
		while(0<j);
		}
	catch(const bad_alloc&)
		{	//! \todo Franci can recover iff # of args exceeds or equals # of args replaced
		UnconditionalRAMFailure();
		};
	DELETE(Tmp2);
	delete Tmp;
	assert(ValidateArgArray(ArgArray));
	return true;
}


static bool ConstructQuantifierList(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (   ArgArray[i]->IsExactType(IN_Phrase1_MC) && 1<=i
		&& 0<ArgArray[i-1]->size())
		{
		if (   ArgArray[i-1]->IsExactType(THEREIS_PhraseN_MC)
		    || ArgArray[i-1]->IsExactType(FORALL_PhraseN_MC)
			|| ArgArray[i-1]->IsExactType(THEREISNO_PhraseN_MC)
			|| ArgArray[i-1]->IsExactType(NOTFORALL_PhraseN_MC))
			return _ConstructQuantifierListAux(ArgArray,i);
		};
	if (ArgArray[i]->IsExactType(FREE_PhraseN_MC) && 0<ArgArray[i]->size())
		return _ConstructQuantifierListAux(ArgArray,i);
	return false;
}

static bool Construct1AryPhrase(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (Phrase1Arg::CanConstruct(ArgArray,i))
		{
		try	{
			new Phrase1Arg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool Construct2AryPhrasePostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (Phrase2Arg::CanConstructPostfix(ArgArray,i))
		{
		try	{
			new Phrase2Arg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool Construct2AryPhraseNonPostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (Phrase2Arg::CanConstructNonPostfix(ArgArray,i))
		{
		try	{
			new Phrase2Arg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool Construct2AryClausePostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (Clause2Arg::CanConstructPostfix(ArgArray,i))
		{
		try	{
			new Clause2Arg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool Construct2AryClauseNonPostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (Clause2Arg::CanConstructNonPostfix(ArgArray,i))
		{
		try	{
			new Clause2Arg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool ConstructNAryPhrasePostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (PhraseNArg::CanConstructPostfix(ArgArray,i))
		{
		try	{
			new PhraseNArg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool ConstructNAryPhraseNonPostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (PhraseNArg::CanConstructNonPostfix(ArgArray,i))
		{
		try	{
			new PhraseNArg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool ConstructNAryClausePostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (ClauseNArg::CanConstructPostfix(ArgArray,i))
		{
		try	{
			new ClauseNArg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static bool ConstructNAryClauseNonPostfix(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (ClauseNArg::CanConstructNonPostfix(ArgArray,i))
		{
		try	{
			new ClauseNArg(ArgArray,i);
			_PostFilterPhraseClause(ArgArray[i]);
			assert(ValidateArgArray(ArgArray));
			return true;
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			}
		}
	return false;
}

static void _StripAddInvSymbolOffOntoArgument(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	// InferenceParameter1 is the index of the AddInv-compatible argument triggering this.
	// InferenceParameter2 is the length of the UnparsedText symbol indexed by InferenceParameter-1
	// ___+-X |-> ___+AddInv(X)
	// ___-X |-> ___+AddInv(X)
	// +-X |-> AddInv(X)
	// -X |-> AddInv(X)
	SUCCEED_OR_DIE(ArgArray[i--]->SelfInverse(StdAddition_MC));

	// Now Idx points to the symbol ending in -.
	if (1==static_cast<UnparsedText*>(ArgArray[i])->LengthOfQuasiSymbolID())
		{
		if (    0==i
			|| (   ArgArray[i-1]->IsExactType(UnparsedText_MC)
		    	&& (   static_cast<UnparsedText*>(ArgArray[i-1])->ArgCannotExtendLeftThroughThis()
					|| static_cast<UnparsedText*>(ArgArray[i-1])->IsLogicalPlusSign()
					|| static_cast<UnparsedText*>(ArgArray[i-1])->IsLogicalMultiplicationSign()
					|| static_cast<UnparsedText*>(ArgArray[i-1])->IsQuasiSymbolEndingInPlusSign())))
			{
			_delete_idx(ArgArray,i);
			assert(ValidateArgArray(ArgArray));
			return;
			};
		static_cast<UnparsedText*>(ArgArray[i])->OverwriteNthPlaceWith(0,'+');
		assert(ValidateArgArray(ArgArray));
		return;
		};
	// general code: truncate symbol by 1.
	static_cast<UnparsedText*>(ArgArray[i])->TruncateByN(1);
	assert(ValidateArgArray(ArgArray));
}

static bool StripAddInvSymbol(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (1<=i && ArgArray[i-1]->IsExactType(UnparsedText_MC))
		{
		// .... X, X AddInvCompatible
		// ___+-X |-> ___+AddInv(X)
		// ___-X |-> ___+AddInv(X)
		// +-X |-> AddInv(X)
		// +X |-> X
		// -X |-> AddInv(X)
		// accept inability to extend leftwards as termination
		unsigned int sign_code = (static_cast<UnparsedText*>(ArgArray[i-1])->IsQuasiSymbolEndingInMinusSign()) ? 1 : 0;
		if (	0==sign_code
			&&	static_cast<UnparsedText*>(ArgArray[i-1])->IsInfixSymbol(SymbolPlusSign)	// if + is definitely 1-ary, clean it
			&& (   1==i
				|| (   ArgArray[i-2]->IsExactType(UnparsedText_MC)
					&& static_cast<UnparsedText*>(ArgArray[i-2])->ArgCannotExtendLeftThroughThis())))
			sign_code = 2;
		if (0==sign_code) return false;

		//! \todo FIX: Franci must be aware of shorthands like "- a dash ... "
		//! *real* idiom clash: 9-5! [this is why parser doesn't evaluate....]
		//! PotentialVarname followed by another PotentialVarname is suspicious
		if (   ArgArray[i]->IsPotentialVarName()
			&& ArgArray[i]->IsUltimateType(NULL))
			ImproviseVar(ArgArray[i],&ClassAdditionDefined);	// this is a potential varname that is probably safe to improvise

		if (   NULL!=ArgArray[i]->UltimateType()
			&& ArgArray[i]->UltimateType()->SupportsThisOperation(StdAddition_MC))
			{	// support for AddInv-compatible types
			if		(1==sign_code)
				{	// AddInv target; clean - to + or null
					// need a safe rewrite method for UnparsedText symbols
				_StripAddInvSymbolOffOntoArgument(ArgArray,i);
				}
			// +X |-> X
			else	// if (2==sign_code)
				{
				_delete_idx(ArgArray,i-1);
				};				    
			assert(ValidateArgArray(ArgArray));
			return true;
			};
		}
	return false;
}

static bool HTMLSuperScript(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (   i+3<ArraySize(ArgArray)
		&& ArgArray[i+1]->IsExactType(UnparsedText_MC)
		&& ArgArray[i+2]->IsExactType(IntegerNumeral_MC)
		&& ArgArray[i+3]->IsExactType(UnparsedText_MC)
		&& static_cast<UnparsedText*>(ArgArray[i+1])->IsHTMLStartTag("sup")
		&& static_cast<UnparsedText*>(ArgArray[i+3])->IsHTMLTerminalTag("sup"))
		{	//! \todo check for varname/MultCompetent object <sup>-1</sup>
			//! this provokes parsing as Multiplicative Inverse
			//! this will also catch powers (eventually)
			//! GENERAL DIFFICULTY: this notation also shows up in nuclear chemistry...as prefix.
			//! Franci may need to learn chemistry before we take on this 
			//! complexity.  We *cannot* reserve element abbreviations outright, since
			//! we may use them as math variables...usage will depend on context.
			//! GENERAL DIFFICULTY: also ionic charge (postfix)
		// At least three types: power under multiplication (arbitrary)
		// Composition Inverse (-1 only)
		// Contravariant index to n-forms (positive only), 
		// or other superscript index thereof (arbitrary)
		if (   ArgArray[i]->IsPotentialVarName()
			&& ArgArray[i]->IsUltimateType(NULL))
			LookUpVar(ArgArray[i],NULL);
		if (   !ArgArray[i]->IsUltimateType(NULL)
			&&  ArgArray[i]->UltimateType()->SupportsThisOperation(StdMultiplication_MC))
			{	// Target supports multiplication.  It matters whether target is a 
				// function, or a scalar...think about this once Franci knows about
				// function composition.
				//! \bug protect against 0<sup>-Integer</sup>
			if (*static_cast<IntegerNumeral*>(ArgArray[i+2])==(signed short)(-1))
				{
				// InferenceParameter1 is the index of the MultInv-compatible argument triggering this.
				// X<sup>-1</sup> |-> MultInv(X)
				// NOTE: <sup>, -1, </sup> are all single MetaConcept entries
				SUCCEED_OR_DIE(ArgArray[i]->SelfInverse(StdMultiplication_MC));

				// clean <sup>-1</sup>
				_delete_n_slots_at(ArgArray,3,i+1);
				assert(ValidateArgArray(ArgArray));
				return true;
				};
#if 0
			else{	//! \todo construct a multiplicative power ArgArray[Idx]^Argarray[Idx+2]
					// note: HTML entities &sup1;, &sup2; also trip this
				// ....
				return true;
				}
#endif
			}
		}
	return false;
}

//! \todo comma-list recognizer [cannot be in recognizer-1: it could inflict O(n^2) performance there]
//!		Comma-lists are n-ary types.  They are semantically sublists of InParse,
//!		with inferred comma suppression.  If they are in an arg-like context,
//!		they also store their bounding semantic-chars.
//!		While Franci will tolerate any of ( ) [ ] { } as bounds, she definitely
//!		prefers certain versions to others.
//!		certain keywords, etc. may request comma-list recognition (that is,
//!		these keywords will not be syntactically correct if the comma-list isn't
//!		there, bounded as specified.  These specify where to start, which direction,
//!		and the desired bounding characters.
//! \todo various comma-ized arg-list recognizers [chain with comma-list, otherwise recognizer-1 will backfire]

// recognition of variables must *not* proceed until all real quantifications are
// complete.  [i.e., no quantification keywords are allowed.]
//		quantifiers do not tolerate NOT; this is a syntax error.
//		Franci cannot improvise abstract class names.

static Parser<MetaConcept>::ParseFunc*
Franci_o_n_rules[] =		{	&NOT_ForcesTruthValueVariable,
								&ResolveUnparsedText,
								&ConstructQuantifierList,
								&Construct1AryPhrase,
								&Construct2AryPhrasePostfix,
								&ConstructNAryPhrasePostfix,
								&Construct2AryClausePostfix,
								&ConstructNAryClausePostfix,
								&StripAddInvSymbol,
								&HTMLSuperScript
							};

static Parser<MetaConcept>::ParseFunc*
Franci_o_n_2_rules[] =	{	&Construct2AryPhraseNonPostfix,	// catches IN ___
							&ConstructNAryPhraseNonPostfix,	// others other quantifier phrases
							&Construct2AryClauseNonPostfix,
							&ConstructNAryClauseNonPostfix
						};

#undef FRANCI_WARY

Parser<MetaConcept> FranciScriptParser(NULL,Franci_o_n_rules,sizeof(Franci_o_n_rules)/sizeof(Parser<MetaConcept>::ParseFunc*),
												Franci_o_n_2_rules,sizeof(Franci_o_n_2_rules)/sizeof(Parser<MetaConcept>::ParseFunc*));

// aux pattern matching
//! \todo META: FIX: need a more careful understanding of what may (and may not be) a valid
//! argument for purposes of pattern recognition.  These are too ad-hoc.
bool ArglistAry2PlusRecognize(const MetaConcept* const * ArgArray,size_t i)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/26/2002
	assert(NULL!=ArgArray);	//! \pre NULL!=ArgArray
	assert(i<ArraySize(ArgArray));
#define ARITY ArraySize(ArgArray)
	// Style error: StartIdx is abused as a upward counter.  Its name suggests
	// it should be constant
	// this routine must recognize arglists, of at least length 2.
	// the leading ( is StartIdx
	if (   i+4<ARITY
		&& ArgArray[i]->IsExactType(UnparsedText_MC)
		&& static_cast<const UnparsedText*>(ArgArray[i])->IsSemanticChar('(')
		&& ArgArray[i+2]->IsExactType(UnparsedText_MC)
		&& ArgArray[i+4]->IsExactType(UnparsedText_MC)
		&& ArgArray[i+1]->IsPotentialArg()
		&& ArgArray[i+3]->IsPotentialArg())
		{
		bool PriorIsEllipsis = false;
		bool ThisIsEllipsis = static_cast<const UnparsedText*>(ArgArray[i+2])->IsLogicalEllipsis() && ArgArray[i+1]->IsUltimateType(&Integer) && ArgArray[i+3]->IsUltimateType(&Integer);
		if (ThisIsEllipsis || static_cast<const UnparsedText*>(ArgArray[i+2])->IsSemanticChar(','))
			{
			i+=4;
			do	{
				if (static_cast<const UnparsedText*>(ArgArray[i])->IsSemanticChar(')'))
					return true;
				if (    i+2>=ARITY
					|| !ArgArray[i+1]->IsPotentialArg())
					return false;
				PriorIsEllipsis = ThisIsEllipsis;
				ThisIsEllipsis = static_cast<const UnparsedText*>(ArgArray[i])->IsLogicalEllipsis() && ArgArray[i-1]->IsUltimateType(&Integer) && ArgArray[i+1]->IsUltimateType(&Integer);
				if (ThisIsEllipsis && PriorIsEllipsis)
					return false;
				if (!ThisIsEllipsis && !static_cast<const UnparsedText*>(ArgArray[i])->IsSemanticChar(','))
					return false;
				i+=2;
				}
			while(ArgArray[i]->IsExactType(UnparsedText_MC));
			}
		};
	return false;
#undef ARITY
}

bool CommalistAry2PlusRecognize(const MetaConcept* const * ArgArray,size_t i)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/26/2002
	assert(NULL!=ArgArray);	//! \pre NULL!=ArgArray
	assert(i<ArraySize(ArgArray));
#define ARITY ArraySize(ArgArray)
	// looking for A,B[,...]
	// StartIdx is A
	if (   2+i<ARITY
	    && ArgArray[i]->IsPotentialArg()
	    && ArgArray[i+2]->IsPotentialArg()
		&& ArgArray[i+1]->IsExactType(UnparsedText_MC))
		{
		bool PriorIsEllipsis = false;
		bool ThisIsEllipsis = static_cast<const UnparsedText*>(ArgArray[i+1])->IsLogicalEllipsis() && ArgArray[i]->IsUltimateType(&Integer) && ArgArray[i+2]->IsUltimateType(&Integer);
		if (ThisIsEllipsis || static_cast<const UnparsedText*>(ArgArray[i+1])->IsSemanticChar(','))
			{
			do	{
				i +=2;
				if (ARITY==i+1) return true;
				if (   !ArgArray[i+1]->IsExactType(UnparsedText_MC)
				    || !static_cast<const UnparsedText*>(ArgArray[i+1])->ArgCannotExtendRightThroughThis())
					return false;
				PriorIsEllipsis = ThisIsEllipsis;
				ThisIsEllipsis = static_cast<const UnparsedText*>(ArgArray[i+1])->IsLogicalEllipsis() && ArgArray[i]->IsUltimateType(&Integer) && ArgArray[i+2]->IsUltimateType(&Integer);
				if (ThisIsEllipsis && PriorIsEllipsis)
					return false;
				if (!ThisIsEllipsis && !static_cast<const UnparsedText*>(ArgArray[i+1])->IsSemanticChar(','))
					return true;
				}
			while(ARITY>i+2 && ArgArray[i+2]->IsPotentialArg());
			}
		}
	return false;
#undef ARITY
}

bool ArgThatCannotExtendLeftRecognize(const MetaConcept* const * ArgArray,size_t i)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/11/2000
	assert(NULL!=ArgArray);	//! \pre NULL!=ArgArray
	assert(i<ArraySize(ArgArray));
	if (   ArgArray[i]->IsPotentialArg()
		&& (   0==i
		    || (   ArgArray[i-1]->IsExactType(UnparsedText_MC)
			    && static_cast<const UnparsedText*>(ArgArray[i-1])->ArgCannotExtendLeftThroughThis())))
		return true;
	return false;
}


// Franci assumes, here, that a prefix keyword is not the final symbol
size_t
PrefixCommaListVarNamesRecognize(const MetaConcept* const * ArgArray,size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/4/2006
	// Style error: KeywordIdx is abused as an upward counter.  Its name suggests
	// it should be constant.
	assert(NULL!=ArgArray);	//! \pre NULL!=ArgArray
	assert(KeywordIdx<ArraySize(ArgArray));
#define ARITY ArraySize(ArgArray)
	KeywordIdx++;
	if (ARITY>KeywordIdx && ArgArray[KeywordIdx]->IsPotentialVarName())
		{
		size_t IdentifiedArgs = 1;
		while(   2+KeywordIdx<ARITY
			  && ArgArray[KeywordIdx+2]->IsPotentialVarName()
			  && ArgArray[KeywordIdx+1]->IsExactType(UnparsedText_MC)
			  && static_cast<const UnparsedText*>(ArgArray[KeywordIdx+1])->IsSemanticChar(','))
			{
			IdentifiedArgs++;
			KeywordIdx+=2;
			};
		return IdentifiedArgs;
		}
	return 0;
#undef ARITY
}

// Franci assumes, here, that a postfix keyword is not the initial symbol
size_t
PostfixCommaListVarNamesRecognize(const MetaConcept* const * ArgArray,size_t KeywordIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/4/2006
	// Style error: KeywordIdx is abused as a downward counter.  Its name suggests
	// it should be constant.
	assert(NULL!=ArgArray);	//! \pre NULL!=ArgArray
	assert(KeywordIdx<ArraySize(ArgArray));
	if (0<KeywordIdx)
		{
		KeywordIdx--;
		if (ArgArray[KeywordIdx]->IsPotentialVarName())
			{
			size_t IdentifiedArgs = 1;
			while(   1<KeywordIdx
				  && ArgArray[KeywordIdx-2]->IsPotentialVarName()
				  && ArgArray[KeywordIdx-1]->IsExactType(UnparsedText_MC)
				  && static_cast<const UnparsedText*>(ArgArray[KeywordIdx-1])->IsSemanticChar(','))
				{
				IdentifiedArgs++;
				KeywordIdx-=2;
				};
			return IdentifiedArgs;
			}
		}
	return 0;
}

