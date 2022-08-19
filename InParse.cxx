// InParse.cxx
// former implementation for InParse, the type for an incompletely parsed object
// this implements the parsing for FranciScript

#include "InParse.hxx"
#include "SymConst.hxx"
#include "Quantify.hxx"
#include "Unparsed.hxx"
#include "Interval.hxx"
#include "StdAdd.hxx"
#include "StdMult.hxx"
#include "Phrase1.hxx"
#include "PhraseN.hxx"
#include "ParseNode.hxx"
#include "LowRel.hxx"
#include "Keyword1.hxx"
#include "LexParse.hxx"
#include "Integer1.hxx"
#include "Combin1.hxx"
#include "MetaCon3.hxx"
#include "Equal.hxx"
#include "InParse2.hxx"

#include "Zaimoni.STL/lite_alg.hpp"
#include "Zaimoni.STL/LexParse/Parser.hpp"
#include "Zaimoni.STL/Pure.C/logging.h"

#include <stdexcept>

extern Parser<MetaConcept> FranciScriptParser;

std::string ConstructPrefixArgList(const MetaConcept* const* const ArgArray); // defined in LenName.cxx
void NiceFormat80Cols(std::string& x); // defined in MetaCon.cxx

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
	if (is_logfile_active()) {
		auto Tmp(ConstructPrefixArgList(ArgArray));
		if (!Tmp.empty()) {
			NiceFormat80Cols(Tmp);
			LOG(Tmp.data());
		}
		else LOG("Attempt to log concept suffered RAM failure.\n");
	};
}

void SayThisNormally(const MetaConcept * const * const ArgArray)
{
	auto Tmp(ConstructPrefixArgList(ArgArray));
	if (!Tmp.empty()) {
		NiceFormat80Cols(Tmp);
		INFORM(Tmp.data());
	} else INFORM("Attempt to state concept suffered RAM failure.\n");
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

static bool ResolveUnparsedText(MetaConcept**& ArgArray,size_t i)
{
	assert(ArgArray);
	assert(i<ArraySize(ArgArray));
	if (auto pivot = up_cast<UnparsedText>(ArgArray[i]))
		{
		// META: This is the complete reaction set for unclassified UnparsedText
		// if an UnparsedText item is the ONLY entry, check to see if it's a
		// variable name that has already been declared.  This is an inplace-rewrite.
		if (   1==ArraySize(ArgArray)	// implies 0==i
			&& pivot->IsQuasiEnglishOrVarName())
			{
			LookUpVar(ArgArray[0],0);	// requests a free variable
			return true;
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

//! following notes are for the legacy parser
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
Franci_o_n_rules[] =		{	&ResolveUnparsedText,
								&ConstructQuantifierList,
								&Construct1AryPhrase, // catches IN ___
								&ConstructNAryPhrasePostfix
							};

static Parser<MetaConcept>::ParseFunc*
Franci_o_n_2_rules[] =	{	&ConstructNAryPhraseNonPostfix	// catches other quantifier phrases
						};

#undef FRANCI_WARY

Parser<MetaConcept> FranciScriptParser(NULL,Franci_o_n_rules,sizeof(Franci_o_n_rules)/sizeof(Parser<MetaConcept>::ParseFunc*),
												Franci_o_n_2_rules,sizeof(Franci_o_n_2_rules)/sizeof(Parser<MetaConcept>::ParseFunc*));

static bool kuroda_ResolveUnparsedText(MetaConcept*& target)
{
	assert(target);
	auto arg = up_cast<UnparsedText>(target);
	if (!arg) return false;
	if (_PostfilterUnparsedText(target)) return true;

	if (arg->IsLogicalInfinity()) {
		// Infinity symbol (easy form)
		// Let autotyping promote LinearInfinity_MC to ComplexInfinity_MC
		// then always interpreting as LinearInfinity_MC isn't a problem
		MetaConcept* Tmp = new SymbolicConstant(LinearInfinity_SC);
		DELETE(target);
		target = Tmp;
		return true;
	}
	return false;
}

static bool _SplitTextInTwo(kuroda::parser<MetaConcept>::sequence& symbols, size_t i)
{
	assert(symbols.size() > i);
#ifndef NDEBUG
	{
	auto arg = up_cast<UnparsedText>(symbols[i]);
	assert(arg);
	assert(arg->CanSplitIntoTwoTexts());
	}
#endif
	if (symbols.InsertSlotAt(i + 1, nullptr)) {
		if (static_cast<UnparsedText*>(symbols[i])->SplitIntoTwoTexts(symbols[i + 1])) {
			kuroda_ResolveUnparsedText(symbols[i]);
			kuroda_ResolveUnparsedText(symbols[i + 1]);
			assert(ValidateArgArray(symbols));
			return true;
		}
		symbols.DeleteIdx(i + 1);
		assert(ValidateArgArray(symbols));
	}
	return false;
}

static std::vector<size_t> kuroda_ResolveUnparsedText2(kuroda::parser<MetaConcept>::sequence& symbols, size_t n) {
	assert(symbols.size() > n);
	std::vector<size_t> ret;
	if (auto arg = up_cast<UnparsedText>(symbols[n])) {
		// appears to handle both historical cases (arg->IsUnclassified() and arg->IsLeadingIntegerNumeral())
		if (arg->CanSplitIntoTwoTexts() && _SplitTextInTwo(symbols, n)) {
			ret.push_back(n);
			ret.push_back(n + 1);
		}
	}
	return ret;
}

static bool flush_ClausePhrase(MetaConcept*& target) {
	if (in_range<MinClausePhraseIdx_MC, MaxClausePhraseIdx_MC>(target->ExactType())
		&& target->CanEvaluate()) {
		DestructiveSyntacticallyEvaluateOnce(target);
		return true;
	}
	return false;
}

static std::pair<unsigned int, size_t> get_operator(const std::vector<unsigned int>& precedence_stack, 
	kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	std::pair<unsigned int, size_t> ret(0, 0);
	size_t i = 0;
	for (auto x : precedence_stack) {
		SUCCEED_OR_DIE(++i <= n);	// require effective index to be within bounds
		if (MetaConcept::Precedence::Comma >= x) continue;
		if (x < ret.first) continue;
		const size_t offset = n - i;
		switch(x)
		{
		// suffix unary operations: priority is right to left
		case MetaConcept::Precedence::Power:
			if (x == ret.first) continue;
			ret.first = x;
			ret.second = n - i;
			break;
		// prefix unary operations: ok to handle as left-to-right
		case MetaConcept::Precedence::UnaryAddition:
		// no priority for this...pretend left-to-right for now
		case MetaConcept::Precedence::Ellipsis:
		// priority for these is left to right
		case MetaConcept::Precedence::Addition:	// priority is left to right
		case MetaConcept::Precedence::Multiplication:	// priority is left to right
			ret.first = x;
			ret.second = n - i;
			break;
		default: SUCCEED_OR_DIE(0 && "invariant violation");
		}
	}
	return ret;
}

static bool CanCoerceArgType(const MetaConcept* const Arg, const AbstractClass& ForceType)
{
	decltype(auto) ult_type = Arg->UltimateType();
	if (ult_type) return !ult_type->IntersectionWithIsNULLSet(ForceType);
	return Arg->IsPotentialVarName();
}

static bool interpret_operator(const std::pair<unsigned int, size_t>& opcode, kuroda::parser<MetaConcept>::sequence& symbols, size_t lb, size_t ub)
{
	assert(0 <= lb);
	assert(symbols.size() > ub);
	assert(lb <= opcode.second);
	assert(ub >= opcode.second);
	switch (opcode.first) {
	case MetaConcept::Precedence::Ellipsis:
		// gets interesting once we start trying to parse English
		// for now, handle closed linear interval within the integers Z
		if (lb >= opcode.second || ub <= opcode.second) return false;
		if (symbols[opcode.second - 1]->IsUltimateType(&Integer) && symbols[opcode.second + 1]->IsUltimateType(&Integer)) {
			zaimoni::autoval_ptr<AbstractClass> TargetType;
			TargetType = new AbstractClass(Integer);
			symbols[opcode.second - 1] = new LinearInterval(symbols[opcode.second - 1], symbols[opcode.second + 1], TargetType, false, false);
			symbols.DeleteNSlotsAt(2, opcode.second);
			return true;
		}
		break;
	case MetaConcept::Precedence::Power:
		if (lb >= opcode.second) return false;	// doesn't work for contravariant Einstein notation
		{	// Heavily overloaded syntax.  This is the generic "superscripted to right", a postfix operator
		decltype(auto) L_proxy = UnwrapParentheses(symbols[opcode.second - 1]);
		decltype(auto) L_test = L_proxy.first ? L_proxy.first : symbols[opcode.second - 1];
		if (RejectTextToVar(L_test)) return false;
		// this only works if the corresponding phrase is of a reasonable ultimate type
		if (!CanCoerceArgType(L_test, ClassMultiplicationDefined)) return false;	// fails for chemical elements
		const auto test = up_cast<ParseNode>(symbols[opcode.second]);
		assert(test);	// invariant violation
		// in general we'd want a power operator data representation
		unsigned int code = 0;
		if (1 == test->size_infix()) {
			if (const auto test_int = up_cast<IntegerNumeral>(test->c_infix_N(0))) {
				if (*test_int == (unsigned short)0) {
					code = 1;
				} else if (*test_int == (unsigned short)1) {
					code = 2;
				} else if (*test_int == (signed short)-1) {
					code = 3;
				} // \todo missing case: power operator
			}
		}
		if (!code) return false;
		if (L_proxy.second) {
			L_proxy.second->infix_reset(0);
			delete symbols[opcode.second - 1];
			symbols[opcode.second - 1] = L_proxy.first;
		}
		if (!CoerceArgType(symbols[opcode.second - 1], ClassMultiplicationDefined)) return false;	// \todo would like these to throw instead
		switch (code)
		{
		case 1:
			// target becomes multiplicative identity if it is not zero
			if (symbols[opcode.second - 1]->IsZero()) return false;	// \todo handle this as 0^0 (errors if evaluated directly but can be context-salvaged)
			{
			zaimoni::autoval_ptr<MetaConcept> staging;
			if (!symbols[opcode.second - 1]->UltimateType()->CreateIdentityForOperation(staging, StdMultiplication_MC)) return false;
			if (staging.empty()) staging = new StdMultiplication();	// omnione
			staging.TransferOutAndNULL(symbols[opcode.second - 1]);
			}
			return true;
		case 2:
			// raising to first power is a value no-op
			symbols.DeleteNSlotsAt(1, opcode.second);
			return true;
		case 3:
			// raising to -1 is multiplicative inverse
			SUCCEED_OR_DIE(symbols[opcode.second - 1]->SelfInverse(StdMultiplication_MC));
			symbols.DeleteNSlotsAt(1, opcode.second);
			return true;
		default: SUCCEED_OR_DIE(0 && "power x<sup>y</sup> not yet implemented");	// \todo implement this
		}
		}
		return false;
	case MetaConcept::Precedence::UnaryAddition:
		if (ub <= opcode.second) return false;
		{	// check for unary -
		decltype(auto) R_proxy = UnwrapParentheses(symbols[opcode.second + 1]);
		decltype(auto) R_test = R_proxy.first ? R_proxy.first : symbols[opcode.second + 1];
		if (RejectTextToVar(R_test)) return false;
		if (!CanCoerceArgType(R_test, ClassAdditionDefined)) return false;
		const auto test = up_cast<UnparsedText>(symbols[opcode.second]);
		assert(test);	// invariant violation
		const bool is_subtraction = test->EndsWith('-');	// STL idiom would be '=' == test->back();
		if (R_proxy.second) {
			R_proxy.second->infix_reset(0);
			delete symbols[opcode.second + 1];
			symbols[opcode.second + 1] = R_proxy.first;
		}
		if (!CoerceArgType(symbols[opcode.second + 1], ClassAdditionDefined)) return false;
		if (is_subtraction) symbols[opcode.second + 1]->SelfInverse(StdAddition_MC);
		if (1 == test->text_size()) symbols.DeleteNSlotsAt(1, opcode.second);	// possibly incorrect for chemical ions (+ discriminates between positive-ion and isotope number)
		else test->TruncateByN(1);
		}
		return true;
	case MetaConcept::Precedence::Addition:
		// we only handle binary infix addition here
		if (lb >= opcode.second || ub <= opcode.second) return false;
		{
		decltype(auto) L_proxy = UnwrapParentheses(symbols[opcode.second - 1]);
		decltype(auto) R_proxy = UnwrapParentheses(symbols[opcode.second + 1]);
		decltype(auto) L_test = L_proxy.first ? L_proxy.first : symbols[opcode.second - 1];
		decltype(auto) R_test = R_proxy.first ? R_proxy.first : symbols[opcode.second + 1];
		if (RejectTextToVar(L_test)) return false;
		if (RejectTextToVar(R_test)) return false;
		// this only works if the corresponding phrase is of a reasonable ultimate type
		if (!CanCoerceArgType(L_test, ClassAdditionDefined)) return false;
		if (!CanCoerceArgType(R_test, ClassAdditionDefined)) return false;
		const auto test = up_cast<UnparsedText>(symbols[opcode.second]);
		assert(test);	// invariant violation
		const bool is_subtraction = test->EndsWith('-');	// STL idiom would be '=' == test->back();
		if (L_proxy.second) {
			L_proxy.second->infix_reset(0);
			delete symbols[opcode.second - 1];
			symbols[opcode.second - 1] = L_proxy.first;
		}
		if (R_proxy.second) {
			R_proxy.second->infix_reset(0);
			delete symbols[opcode.second + 1];
			symbols[opcode.second + 1] = R_proxy.first;
		}
		if (!CoerceArgType(symbols[opcode.second - 1], ClassAdditionDefined)) return false;	// \todo would like these to throw instead
		if (!CoerceArgType(symbols[opcode.second + 1], ClassAdditionDefined)) return false;
		if (is_subtraction) symbols[opcode.second + 1]->SelfInverse(StdAddition_MC);

		zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(2);
		args[0] = symbols[opcode.second - 1];
		args[1] = symbols[opcode.second + 1];
		auto staging = new StdAddition(args);
		symbols[opcode.second + 1] = 0;
		symbols[opcode.second - 1] = staging;
		}
		symbols.DeleteNSlotsAt(2, opcode.second);
		return true;
	case MetaConcept::Precedence::Multiplication:
		if (lb >= opcode.second || ub <= opcode.second) return false;
		{
		decltype(auto) L_proxy = UnwrapParentheses(symbols[opcode.second - 1]);
		decltype(auto) R_proxy = UnwrapParentheses(symbols[opcode.second + 1]);
		decltype(auto) L_test = L_proxy.first ? L_proxy.first : symbols[opcode.second - 1];
		decltype(auto) R_test = R_proxy.first ? R_proxy.first : symbols[opcode.second + 1];
		if (RejectTextToVar(L_test)) return false;
		if (RejectTextToVar(R_test)) return false;

		// this only works if the corresponding phrase is of a reasonable ultimate type
		// abstract-algebra modules over the integers Z only need +
		const bool right_module_ok = Integer.Superclass(R_test->UltimateType()) && CanCoerceArgType(L_test, ClassAdditionDefined);
		if (!right_module_ok && !CanCoerceArgType(L_test, ClassMultiplicationDefined)) return false;
		const bool left_module_ok = Integer.Superclass(L_test->UltimateType()) && CanCoerceArgType(R_test, ClassAdditionDefined);
		if (!left_module_ok && !CanCoerceArgType(R_test, ClassMultiplicationDefined)) return false;
		if (L_proxy.second) {
			L_proxy.second->infix_reset(0);
			delete symbols[opcode.second - 1];
			symbols[opcode.second - 1] = L_proxy.first;
		}
		if (R_proxy.second) {
			R_proxy.second->infix_reset(0);
			delete symbols[opcode.second + 1];
			symbols[opcode.second + 1] = R_proxy.first;
		}

		if (!CoerceArgType(symbols[opcode.second - 1], right_module_ok ? ClassAdditionDefined : ClassMultiplicationDefined)) return false;	// \todo would like these to throw instead
		if (!symbols[opcode.second - 1]->SyntaxOK()) throw std::runtime_error("error created");
		if (!CoerceArgType(symbols[opcode.second + 1], left_module_ok ? ClassAdditionDefined : ClassMultiplicationDefined)) return false;
		if (!symbols[opcode.second + 1]->SyntaxOK()) throw std::runtime_error("error created");
		}
		{
		zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(2);
		args[0] = symbols[opcode.second - 1];
		args[1] = symbols[opcode.second + 1];
		auto staging = new StdMultiplication(args);
		symbols[opcode.second + 1] = 0;
		symbols[opcode.second - 1] = staging;
		}
		symbols.DeleteNSlotsAt(2, opcode.second);
		return true;
	default: SUCCEED_OR_DIE(0 && "invariant violation");
	}
	return false;
}

static auto make_precedence_stack(const kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	std::vector<unsigned int> precedence_stack;
	size_t lb = n;
	while (0 < lb) {
		decltype(auto) text = up_cast<UnparsedText>(symbols[--lb]);
		decltype(auto) node = up_cast<ParseNode>(symbols[lb]);
		if (!text && !node) {
			precedence_stack.push_back(0);	// already parsed
			continue;
		}
		auto prec = text ? text->OpPrecedence() : node->OpPrecedence();
		if (MetaConcept::Precedence::Comma == prec) break;
		if (MetaConcept::Precedence::LParenthesis == prec) break;
		if (MetaConcept::Precedence::Addition < prec && !precedence_stack.empty() && MetaConcept::Precedence::Addition == precedence_stack.back()) precedence_stack.back() = MetaConcept::Precedence::UnaryAddition;
		precedence_stack.push_back(prec);
	}
	if (MetaConcept::Precedence::Addition == precedence_stack.back()) precedence_stack.back() = MetaConcept::Precedence::UnaryAddition;
	return precedence_stack;
}

void operator_bulk_parse(kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	do {
		std::vector<unsigned int> precedence_stack(make_precedence_stack(symbols, n));
		if (precedence_stack.empty()) return;
		auto opcode = get_operator(precedence_stack, symbols, n);
		if (!opcode.first) return;
		const size_t co_n = symbols.size() - n;
		if (!interpret_operator(opcode, symbols, n - precedence_stack.size(), n - 1)) return;
		n = symbols.size() - co_n;
	} while(true);
}

static std::vector<size_t> handle_Comma(kuroda::parser<MetaConcept>::sequence& symbols, size_t n) {
	assert(symbols.size() > n);
	std::vector<size_t> ret;
	if (!IsSemanticChar<','>(symbols[n])) return ret;
	operator_bulk_parse(symbols, n);
	return ret;
}

bool force_parse(kuroda::parser<MetaConcept>::sequence& symbols)
{
	Franci_parser().finite_parse(symbols);
	operator_bulk_parse(symbols, symbols.size());
	return true;
}

static std::vector<size_t> close_RightParenthesis(kuroda::parser<MetaConcept>::sequence& symbols, size_t n) {
	assert(symbols.size() > n);
	std::vector<size_t> ret;
	if (!IsSemanticChar<')'>(symbols[n])) return ret;
	// scan-down
	size_t lb = n;
	while (0 < lb) {
		if (IsSemanticChar<'('>(symbols[--lb])) {
			const auto working = new ParseNode(symbols, lb, n, ParseNode::CLOSED);
			ret.push_back(lb);
			if (1 == working->size_infix()) working->apply_all_infix(&flush_ClausePhrase);
			working->syntax_check_infix(force_parse);
			break;
		} else if (IsSemanticChar<')'>(symbols[lb])) {
			n = lb;
		}
		// \todo: half-open ray syntax
	}
	return ret;
}

static std::vector<size_t> close_HTMLterminal(kuroda::parser<MetaConcept>::sequence& symbols, size_t n) {
	assert(symbols.size() > n);
	std::vector<size_t> ret;
	decltype(auto) kw = IsHTMLTerminalTag(symbols[n]);
	if (!kw) return ret;

	// scan-down
	size_t lb = n;
	while (0 < lb) {
		if (IsHTMLStartTag(symbols[--lb], kw)) {
			const auto working = new ParseNode(symbols, lb, n, ParseNode::CLOSED);
			ret.push_back(lb);
			if (1 == working->size_infix()) working->apply_all_infix(&flush_ClausePhrase);
			working->syntax_check_infix(force_parse);
			break;
		} else if (IsHTMLTerminalTag(symbols[lb], kw)) {
			n = lb;
		}
	}
	return ret;
}

static std::vector<size_t> NOT_ForcesTruthValueVariable_kuroda(kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	assert(symbols.size() > n);
	std::vector<size_t> ret;
	if (1 <= n && IsLogicKeyword(symbols[n - 1], LogicKeyword_NOT) && CanCoerceArgType(symbols[n], TruthValues)) {
		ret.push_back(n - 1);
		SUCCEED_OR_DIE(CoerceArgType(symbols[n], TruthValues));
		symbols[n]->SelfLogicalNOT();
		symbols.DeleteIdx(n - 1);
		return ret;
	}
	return ret;
}

static std::vector<size_t> close_RightBracket(kuroda::parser<MetaConcept>::sequence& symbols, size_t n) {
	assert(symbols.size() > n);
	std::vector<size_t> ret;
	if (!IsSemanticChar<']'>(symbols[n])) return ret;
	// scan-down
	size_t lb = n;
	while (0 < lb) {
		if (IsSemanticChar<'['>(symbols[--lb])) {
			const auto working = new ParseNode(symbols, lb, n, ParseNode::CLOSED);
			ret.push_back(lb);
			if (1 == working->size_infix()) working->apply_all_infix(&flush_ClausePhrase);
			working->syntax_check_infix(force_parse);
		} else if (IsSemanticChar<']'>(symbols[lb])) {
			n = lb;
		}
		// \todo: half-open ray syntax
	}

	return ret;
}

ExactType_MC CombinatorialLike::prefix_keyword(const MetaConcept* x)
{
	if (const auto text = up_cast<UnparsedText>(x))
		{
		if (text->IsPrefixKeyword(PrefixKeyword_FACTORIAL)) return Factorial_MC;
		if (   text->IsPrefixKeyword(PrefixKeyword_PERMUTATION)
			|| text->IsPrefixKeyword(PrefixKeyword_PERMUTATION_ALT))
			return PermutationCount_MC;
		if (   text->IsPrefixKeyword(PrefixKeyword_COMBINATION)
			|| text->IsPrefixKeyword(PrefixKeyword_COMBINATION_ALT))
			return CombinationCount_MC;
		}
	return Unknown_MC;
}

std::vector<size_t> CombinatorialLike::parse(kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	assert(symbols.size() > n);
	static const auto int_coerce = [](MetaConcept*& x) { return CoerceArgType(x, Integer); };

	std::vector<size_t> ret;
	if (0 >= n) return ret;
	if (const auto node = IsArglist(symbols[n])) {
		const auto raw_arglen = node->arglist_is_comma_separated();
		int is_malformed = !raw_arglen;
		switch (const auto kw = prefix_keyword(symbols[n - 1])) {
		case Factorial_MC:
			if (1 != raw_arglen) {
				is_malformed = 1;
				break;
			}
			node->action_at_infix(0, UnwrapAllParentheses);
			if (!CanCoerceArgType(node->c_infix_N(0), Integer)) {
				is_malformed = 1;
				break;
			}
			SUCCEED_OR_DIE(node->apply_at_infix(0, int_coerce));
			{
			zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(1);
			ret.push_back(n - 1);
			args[0] = node->infix_N(0);
			std::unique_ptr<CombinatorialLike> staging(new CombinatorialLike(args, FACTORIAL_CM));
			node->infix_reset(0);
			delete symbols[n - 1];
			symbols[n - 1] = staging.release();
			}
			symbols.DeleteNSlotsAt(1, n);
			return ret;
		case CombinationCount_MC:
		case PermutationCount_MC:
			if (3 != raw_arglen) {
				is_malformed = 1;
				break;
			}
			// \todo: std::initializer_list<size_t> looks useful here
			node->action_at_infix(0, UnwrapAllParentheses);
			node->action_at_infix(2, UnwrapAllParentheses);
			if (!CanCoerceArgType(node->c_infix_N(0), Integer) || !CanCoerceArgType(node->c_infix_N(2), Integer)) {
				is_malformed = 1;
				break;
			}
			SUCCEED_OR_DIE(node->apply_at_infix(0, int_coerce));
			SUCCEED_OR_DIE(node->apply_at_infix(2, int_coerce));
			{
			zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(2);
			ret.push_back(n - 1);
			args[0] = node->infix_N(0);
			args[1] = node->infix_N(2);
			std::unique_ptr<CombinatorialLike> staging(new CombinatorialLike(args, (CombinatorialModes)(kw - Factorial_MC)));
			node->infix_reset(0);
			node->infix_reset(2);
			delete symbols[n - 1];
			symbols[n - 1] = staging.release();
			}
			symbols.DeleteNSlotsAt(1, n);
			return ret;
		default: return ret;	// something else entirely
		}
		if (is_malformed) {
			ret.push_back(n - 1);
			node->push_prefix(symbols[n - 1]);	// i.e., no longer arglist
			symbols.DeleteNSlotsAt(1, n - 1);
			INC_INFORM("malformed: ");
			INFORM(*node);
			// \todo would be useful to set ultimate type to Integer, etc. (gamma function would be reals, or infinity-extended reals)
			return ret;
		}
	}
	return ret;
}

ExactType_MC EqualRelation::prefix_keyword(const MetaConcept* x)
{
	if (const auto text = up_cast<UnparsedText>(x)) {
		if (text->IsPrefixKeyword(EqualRelation_ALLEQUAL)) return ALLEQUAL_MC;
		if (text->IsPrefixKeyword(EqualRelation_ALLDISTINCT)) return ALLDISTINCT_MC;
		if (text->IsPrefixKeyword(EqualRelation_NOTALLDISTINCT)) return NOTALLDISTINCT_MC;
		if (text->IsPrefixKeyword(EqualRelation_NOTALLEQUAL)) return NOTALLEQUAL_MC;
	}
	return Unknown_MC;
}

std::vector<size_t> EqualRelation::parse(kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	assert(symbols.size() > n);
	static const auto free_coerce = [](MetaConcept*& x) { _improviseVar(x, 0); };

	std::vector<size_t> ret;
	if (0 >= n) return ret;
	if (const auto node = IsArglist(symbols[n])) {
		const auto kw = prefix_keyword(symbols[n - 1]);
		if (!kw) return ret;
		const auto raw_arglen = node->arglist_is_comma_separated();
		bool is_malformed = !raw_arglen;
		// type coercion doesn't work cleanly for equality...variables go to free
		if (!is_malformed) {
			size_t i = -2;
			while (raw_arglen > (i += 2)) {
				node->action_at_infix(i, UnwrapAllParentheses);
				node->action_at_infix(i, free_coerce);
			}
		}
		if (!is_malformed) {
			zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(raw_arglen / 2 + 1);
			ret.push_back(n - 1);
			size_t i = -2;
			while (raw_arglen > (i += 2)) args[i / 2] = node->infix_N(i);
			std::unique_ptr<EqualRelation> staging(new EqualRelation(args, (EqualRelationModes)(kw - ALLEQUAL_MC)));
			i = -2;
			while (raw_arglen > (i += 2)) node->infix_reset(i);
			delete symbols[n - 1];
			symbols[n - 1] = staging.release();
			symbols.DeleteNSlotsAt(1, n);
			return ret;
		}
		/* if (is_malformed) */ {
			ret.push_back(n - 1);
			node->push_prefix(symbols[n - 1]);	// i.e., no longer arglist
			symbols.DeleteNSlotsAt(1, n - 1);
			INC_INFORM("malformed: ");
			INFORM(*node);
			// \todo would be useful to set ultimate type to TruthValues
			return ret;
		}
	}
	return ret;
}

ExactType_MC EqualRelation::infix_keyword(const MetaConcept* x)
{
	if (const auto text = up_cast<UnparsedText>(x)) {
		if (text->IsPrefixKeyword(EqualRelation_EQUALTOONEOF)) return EQUALTOONEOF_MC;
		if (text->IsPrefixKeyword(EqualRelation_DISTINCTFROMALLOF)) return DISTINCTFROMALLOF_MC;
	}
	return Unknown_MC;
}

std::vector<size_t> EqualRelation::rightmost_parse(kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	assert(symbols.size() > n);
	static const auto free_coerce = [](MetaConcept*& x) { _improviseVar(x, 0); };

	std::vector<size_t> ret;
	if (IsSemanticChar<','>(symbols[n]) || infix_keyword(symbols[n])) return ret;	// \todo harden callers of _initMetaConceptParserArray to be syntax error competent
	size_t scandown = n;
	ExactType_MC kw = Unknown_MC;
	while (2 <= scandown) {
		scandown -= 2;
		if (IsSemanticChar<','>(symbols[scandown]) || infix_keyword(symbols[scandown])) return ret;
		if (IsSemanticChar<','>(symbols[scandown + 1])) continue;
		if (kw = infix_keyword(symbols[scandown + 1])) break;
		return ret;
	}
	if (!kw) return ret;
	zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args((n - scandown) / 2 + 1);
	ret.push_back(scandown);
	size_t i = 0;
	size_t sweep = scandown;
	while (sweep <= n) {
		UnwrapAllParentheses(symbols[sweep]);
		free_coerce(symbols[sweep]);
		args[i++] = symbols[sweep];
		sweep += 2;
	}
	std::unique_ptr<EqualRelation> staging(new EqualRelation(args, (EqualRelationModes)(kw - ALLEQUAL_MC)));
	sweep = scandown;
	while (sweep < n) symbols[sweep += 2] = 0;
	symbols[scandown] = staging.release();
	symbols.DeleteNSlotsAt(n - scandown, scandown+1);
	return ret;
}

ExactType_MC MetaConnective::prefix_keyword(const MetaConcept* x)
{
	if (const auto text = up_cast<UnparsedText>(x)) {
		if (text->IsLogicKeyword(LogicKeyword_AND)) return LogicalAND_MC;
		if (text->IsLogicKeyword(LogicKeyword_OR)) return LogicalOR_MC;
		if (text->IsLogicKeyword(LogicKeyword_IFF)) return LogicalIFF_MC;
		if (text->IsLogicKeyword(LogicKeyword_XOR)) return LogicalXOR_MC;
		if (text->IsLogicKeyword(LogicKeyword_NXOR)) return LogicalNXOR_MC;
		if (text->IsLogicKeyword(LogicKeyword_NIFF)) return LogicalNIFF_MC;
		if (text->IsLogicKeyword(LogicKeyword_NOR)) return LogicalNOR_MC;
		if (text->IsLogicKeyword(LogicKeyword_NAND)) return LogicalNAND_MC;
	}
	return Unknown_MC;
}

ExactType_MC MetaConnective::infix_keyword(const MetaConcept* x)
{
	if (const auto text = up_cast<UnparsedText>(x)) {
		if (text->IsLogicKeyword(LogicKeyword_AND)) return LogicalAND_MC;
		if (text->IsLogicKeyword(LogicKeyword_OR)) return LogicalOR_MC;
		if (text->IsLogicKeyword(LogicKeyword_IFF)) return LogicalIFF_MC;
		if (text->IsLogicKeyword(LogicKeyword_XOR)) return LogicalXOR_MC;
//		if (text->IsLogicKeyword(LogicKeyword_NXOR)) return LogicalNXOR_MC;	// these 2 are 3-ary minimum
//		if (text->IsLogicKeyword(LogicKeyword_NIFF)) return LogicalNIFF_MC;
		if (text->IsLogicKeyword(LogicKeyword_NOR)) return LogicalNOR_MC;
		if (text->IsLogicKeyword(LogicKeyword_NAND)) return LogicalNAND_MC;
	}
	return Unknown_MC;
}

ExactType_MC MetaConnective::parse_infix_ok(const kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	if (2 >= symbols.size()) return Unknown_MC;
	const auto kw = infix_keyword(symbols[n - 1]);
	if (!kw) return Unknown_MC;
	if (LogicalIFF_MC == kw) return Unknown_MC; // handle IFF as right-most parse, not here
	if (symbols.size() - 1 > n) {
		if (const auto kw2 = infix_keyword(symbols[n + 1])) {
			if (kw != kw2) return Unknown_MC;	// hold off, don't want to mess with precedence
			// associative ok
			// transitive would be ok but we want to back-propagate that
			if (!(MetaConceptLookUp[kw].Bitmap1 & SelfAssociative_LITBMP1MC)) return Unknown_MC;
		}
	}
	UnwrapAllParentheses(symbols[n]);
	if (!CanCoerceArgType(symbols[n], TruthValues)) return Unknown_MC;
	UnwrapAllParentheses(symbols[n - 2]);
	if (!CanCoerceArgType(symbols[n - 2], TruthValues)) return Unknown_MC;
	return kw;
}

ExactType_MC MetaConnective::macro_keyword(const MetaConcept* x)
{
	if (const auto text = up_cast<UnparsedText>(x)) {
		if (text->IsLogicKeyword(LogicKeyword_IMPLIES)) return LogicalOR_MC;
	}
	return Unknown_MC;
}


ExactType_MC MetaConnective::parse_macro_ok(const kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	// IMPLIES is the deductive logic version, not entailment or a Kripke semantics statement
	// the previous parser also supported the integrated logical negation NIMPLIES, but this has divergent meaning between deductive logic and entailment.
	if (2 >= symbols.size()) return Unknown_MC;
	const auto kw = macro_keyword(symbols[n - 1]);
	if (!kw) return Unknown_MC;
	if (symbols.size() - 1 > n) {
		// technically IMPLIES is transitive
		if (infix_keyword(symbols[n + 1]) || macro_keyword(symbols[n + 1])) return Unknown_MC;
	}

	UnwrapAllParentheses(symbols[n]);
	if (!CanCoerceArgType(symbols[n], TruthValues)) return Unknown_MC;
	UnwrapAllParentheses(symbols[n - 2]);
	if (!CanCoerceArgType(symbols[n - 2], TruthValues)) return Unknown_MC;
	return kw;
}

std::vector<size_t> MetaConnective::rightmost_parse(kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	assert(symbols.size() > n);
	static const auto tval_coerce = [](MetaConcept*& x) { return CoerceArgType(x, TruthValues); };
	std::vector<size_t> ret;
	if (2 > n) return ret;

	const auto kw = infix_keyword(symbols[n - 1]);
	// IFF is an equivalence relation for classical logic, so gets n-ary infix notation
	if (kw != LogicalIFF_MC) return ret;

	size_t scan_down = n - 1;
	while (3 <= scan_down) {
		if (const auto kw2 = infix_keyword(symbols[scan_down - 2])) {
			if (kw != kw2) return ret;	// hold off, don't want to mess with precedence
			scan_down -= 2;
		}
		break;
	}

	const size_t nonstrict_lb = scan_down - 1;
	size_t nonstrict_ub = n;

	size_t i = 0;
	zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args((n - nonstrict_lb) / 2 + 1);

	size_t sweep = nonstrict_lb;
	while (sweep <= n) {
		UnwrapAllParentheses(symbols[sweep]);
		if (!CanCoerceArgType(symbols[sweep], TruthValues)) return ret;
		args[i++] = symbols[sweep];
		sweep += 2;
	}
	for(decltype(auto) x : args) SUCCEED_OR_DIE(tval_coerce(x));

	std::unique_ptr<MetaConnective> staging(new MetaConnective(args, (MetaConnectiveModes)(kw - LogicalAND_MC)));
	sweep = nonstrict_lb;
	while(sweep < n) symbols[sweep += 2] = nullptr;
	symbols[nonstrict_lb] = staging.release();
	symbols.DeleteNSlotsAt(n - nonstrict_lb, nonstrict_lb + 1);
	if (0 < nonstrict_lb) ret.push_back(nonstrict_lb);

	return ret;
}

std::vector<size_t> MetaConnective::parse(kuroda::parser<MetaConcept>::sequence& symbols, size_t n)
{
	assert(symbols.size() > n);
	static const auto tval_coerce = [](MetaConcept*& x) { return CoerceArgType(x, TruthValues); };

	std::vector<size_t> ret;
	if (0 >= n) return ret;
	if (const auto kw = parse_infix_ok(symbols, n)) {
		SUCCEED_OR_DIE(tval_coerce(symbols[n]));
		SUCCEED_OR_DIE(tval_coerce(symbols[n - 2]));
		// 2-ary; relying on associativity to auto-cleanup in n-ary case
		const size_t nonstrict_lb = n - 2;
		zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(2);
		ret.push_back(nonstrict_lb);
		args[0] = symbols[n - 2];
		args[1] = symbols[n];

		std::unique_ptr<MetaConnective> staging(new MetaConnective(args, (MetaConnectiveModes)(kw - LogicalAND_MC)));
		symbols[n] = nullptr;
		symbols[nonstrict_lb] = staging.release();
		symbols.DeleteNSlotsAt(2, n - 1);
		return ret;
	}

	// IMPLIES is the deductive logic version, not entailment or a Kripke semantics statement
	// the previous parser also supported the integrated logical negation NIMPLIES, but this has divergent meaning between deductive logic and entailment.
	if (const auto kw = parse_macro_ok(symbols, n)) {
		SUCCEED_OR_DIE(LogicalOR_MC == kw);	// IMPLIES; don't handle anything else (yet)
		SUCCEED_OR_DIE(tval_coerce(symbols[n]));
		SUCCEED_OR_DIE(tval_coerce(symbols[n - 2]));
		const size_t nonstrict_lb = n - 2;
		// technically IMPLIES is transitive; result is an AND of 2-ary OR clauses

		zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(2);
		ret.push_back(nonstrict_lb);
		args[0] = symbols[nonstrict_lb];
		args[1] = symbols[n];
		args[0]->SelfLogicalNOT();

		std::unique_ptr<MetaConnective> staging(new MetaConnective(args, (MetaConnectiveModes)(kw - LogicalAND_MC)));
		symbols[n] = nullptr;
		symbols[nonstrict_lb] = staging.release();
		symbols.DeleteNSlotsAt(2, n - 1);
		return ret;
	}

	if (const auto node = IsArglist(symbols[n])) {
		const auto kw = prefix_keyword(symbols[n - 1]);
		if (!kw) return ret;
		if (5 > node->size()) return ret; // all of the MetaConnective types have minimum arity 2
		const auto raw_arglen = node->arglist_is_comma_separated();
		bool is_malformed = !raw_arglen;
		if (!is_malformed) {
			size_t i = -2;
			while (raw_arglen > (i += 2)) {
				node->action_at_infix(i, UnwrapAllParentheses);
				if (!CanCoerceArgType(node->c_infix_N(i), TruthValues)) {
					INC_INFORM("non-coercable");
					INFORM(*node->c_infix_N(i));
					INFORM(i);
					is_malformed = true;
					break;
				}
				SUCCEED_OR_DIE(node->apply_at_infix(i, tval_coerce));
			}
		}
		if (!is_malformed) {
			zaimoni::weakautovalarray_ptr_throws<MetaConcept*> args(raw_arglen/2+1);
			ret.push_back(n - 1);
			size_t i = -2;
			while (raw_arglen > (i += 2)) args[i/2] = node->infix_N(i);
			std::unique_ptr<MetaConnective> staging(new MetaConnective(args, (MetaConnectiveModes)(kw - LogicalAND_MC)));
			i = -2;
			while (raw_arglen > (i += 2)) node->infix_reset(i);
			delete symbols[n - 1];
			symbols[n - 1] = staging.release();
			symbols.DeleteNSlotsAt(1, n);
			return ret;
		}
		/* if (is_malformed) */ {
			ret.push_back(n - 1);
			node->push_prefix(symbols[n - 1]);	// i.e., no longer arglist
			symbols.DeleteNSlotsAt(1, n - 1);
			INC_INFORM("malformed: ");
			INFORM(*node);
			// \todo would be useful to set ultimate type to TruthValues
			return ret;
		}
	}
	return ret;
}

kuroda::parser<MetaConcept>& Franci_parser()
{
	static zaimoni::autoval_ptr<kuroda::parser<MetaConcept> > ooao;
	if (!ooao) {
		ooao = new kuroda::parser<MetaConcept>();
		ooao->register_terminal(&kuroda_ResolveUnparsedText);
		ooao->register_build_nonterminal(&kuroda_ResolveUnparsedText2);
		ooao->register_build_nonterminal(&close_RightBracket);
		ooao->register_build_nonterminal(close_RightParenthesis);
		ooao->register_build_nonterminal(CombinatorialLike::parse);
		ooao->register_build_nonterminal(NOT_ForcesTruthValueVariable_kuroda);
		ooao->register_build_nonterminal(MetaConnective::parse);
		ooao->register_build_nonterminal(EqualRelation::parse);
		ooao->register_build_nonterminal(close_HTMLterminal);
		ooao->register_build_nonterminal(handle_Comma);

		ooao->register_right_edge_build_nonterminal(EqualRelation::rightmost_parse);
		ooao->register_right_edge_build_nonterminal(MetaConnective::rightmost_parse);
	}
	return *ooao;
}

bool MetaConcept::IsPotentialArg() const
{	// Detection metaphor: we want to allow phrases that eventually evaluate to acceptable args.
	// this does *not* include IN __, or any of the quantifier phrases
	return !IsUltimateType(NULL) || IsPotentialVarName();
}

// aux pattern matching
//! \todo META: FIX: need a more careful understanding of what may (and may not be) a valid
//! argument for purposes of pattern recognition.  These are too ad-hoc.
bool ArglistAry2PlusRecognize(const MetaConcept* const * ArgArray,size_t i)
{	// FORMALLY CORRECT: 2020-07-29
	assert(ArgArray);	//! \pre NULL!=ArgArray
	const size_t ARITY = ArraySize(ArgArray);
	assert(i < ARITY);
	// Style error: StartIdx is abused as a upward counter.  Its name suggests
	// it should be constant
	// this routine must recognize arglists, of at least length 2.
	// the leading ( is StartIdx
	if (   i+4<ARITY
		&& IsSemanticChar<'('>(ArgArray[i])
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
				if (static_cast<const UnparsedText*>(ArgArray[i])->IsSemanticChar(')')) return true;
				if (i+2>=ARITY || !ArgArray[i+1]->IsPotentialArg()) return false;
				PriorIsEllipsis = ThisIsEllipsis;
				ThisIsEllipsis = static_cast<const UnparsedText*>(ArgArray[i])->IsLogicalEllipsis() && ArgArray[i-1]->IsUltimateType(&Integer) && ArgArray[i+1]->IsUltimateType(&Integer);
				if (ThisIsEllipsis && PriorIsEllipsis) return false;
				if (!ThisIsEllipsis && !static_cast<const UnparsedText*>(ArgArray[i])->IsSemanticChar(',')) return false;
				i+=2;
				}
			while(ArgArray[i]->IsExactType(UnparsedText_MC));
			}
		};
	return false;
}

bool CommalistAry2PlusRecognize(const MetaConcept* const * ArgArray,size_t i)
{	// FORMALLY CORRECT: 2020-07-30
	assert(ArgArray);
	const size_t ARITY = ArraySize(ArgArray);
	assert(i < ARITY);
	// looking for A,B[,...]
	// StartIdx is A
	if (   2+i<ARITY
	    && ArgArray[i]->IsPotentialArg()
	    && ArgArray[i+2]->IsPotentialArg())
		{
		const auto pivot = up_cast<UnparsedText>(ArgArray[i + 1]);
		if (!pivot) return false;
		bool ThisIsEllipsis = pivot->IsLogicalEllipsis() && ArgArray[i]->IsUltimateType(&Integer) && ArgArray[i+2]->IsUltimateType(&Integer);
		if (ThisIsEllipsis || pivot->IsSemanticChar(','))
			{
			bool PriorIsEllipsis = false;
			do	{
				i +=2;
				if (ARITY==i+1) return true;
				const auto arg = up_cast<UnparsedText>(ArgArray[i + 1]);
				if (!arg || !arg->ArgCannotExtendRightThroughThis()) return false;
				PriorIsEllipsis = ThisIsEllipsis;
				ThisIsEllipsis = arg->IsLogicalEllipsis() && ArgArray[i]->IsUltimateType(&Integer) && ArgArray[i+2]->IsUltimateType(&Integer);
				if (ThisIsEllipsis) {
					if (PriorIsEllipsis) return false;
				} else {
					if (!arg->IsSemanticChar(',')) return true;
				}
				}
			while(ARITY>i+2 && ArgArray[i+2]->IsPotentialArg());
			}
		}
	return false;
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
size_t PrefixCommaListVarNamesRecognize(const MetaConcept* const * ArgArray,size_t KeywordIdx)
{	// FORMALLY CORRECT: 2020-07-29
	// Style error: KeywordIdx is abused as an upward counter.  Its name suggests it should be constant.
	assert(ArgArray);
	const size_t ARITY = ArraySize(ArgArray);
	assert(KeywordIdx < ARITY);
	KeywordIdx++;
	if (ARITY>KeywordIdx && ArgArray[KeywordIdx]->IsPotentialVarName()) {
		size_t IdentifiedArgs = 1;
		while(   2+KeywordIdx<ARITY
			  && ArgArray[KeywordIdx+2]->IsPotentialVarName()
			  && IsSemanticChar<','>(ArgArray[KeywordIdx+1])) {
			IdentifiedArgs++;
			KeywordIdx+=2;
		};
		return IdentifiedArgs;
	}
	return 0;
}

// Franci assumes, here, that a postfix keyword is not the initial symbol
size_t PostfixCommaListVarNamesRecognize(const MetaConcept* const * ArgArray,size_t KeywordIdx)
{	// FORMALLY CORRECT: 2020-07-29
	// Style error: KeywordIdx is abused as a downward counter.  Its name suggests it should be constant.
	assert(ArgArray);
	assert(KeywordIdx<ArraySize(ArgArray));
	if (0<KeywordIdx) {
		if (ArgArray[--KeywordIdx]->IsPotentialVarName()) {
			size_t IdentifiedArgs = 1;
			while(   1<KeywordIdx
				  && ArgArray[KeywordIdx-2]->IsPotentialVarName()
				  && IsSemanticChar<','>(ArgArray[KeywordIdx-1])) {
				IdentifiedArgs++;
				KeywordIdx-=2;
			};
			return IdentifiedArgs;
		}
	}
	return 0;
}

