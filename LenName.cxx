// LenName.cxx
// implementation for LengthOfSelfName virtual functions

#include "MetaCon3.hxx"
#include "Equal.hxx"
#include "StdAdd.hxx"
#include "StdMult.hxx"
#include "GCF.hxx"
#include "ClauseN.hxx"
#include "PhraseN.hxx"
#include "QState.hxx"

#include "Clause2.hxx"
#include "Phrase2.hxx"
#include "Interval.hxx"

#include "Phrase1.hxx"
#include "Class.hxx"
#include "Quantify.hxx"
#include "Variable.hxx"

#include "TruthVal.hxx"
#include "Unparsed.hxx"

#include "Keyword1.hxx"

#include "Zaimoni.STL/string.h"

// AbstractClass
#define UNNAMED_CLASS "(Unnamed class)"
#define UNNAMED_SET "(Unnamed set)"
#define UNNAMED_PROPER_CLASS "(Unnamed proper class)"
#define EMPTY_STATEMENT "(Empty Statement)"
#define BAD_VARIABLE_SYNTAX "(Variable with syntax error)"

// multiplicative inverse
#define MULT_INV_TEXT "<sup>-1</sup>"

// MetaConnective
const char* const MetaConnectiveNames[8] =	{
											LogicKeyword_AND,	// AND
											LogicKeyword_OR,	// OR
											LogicKeyword_IFF,	// IFF
											LogicKeyword_XOR,	// XOR
											LogicKeyword_NXOR,	// NXOR
											LogicKeyword_NIFF,	// NIFF
											LogicKeyword_NOR,	// NOR
											LogicKeyword_NAND	// NAND
											};

static_assert(sizeof(const char*)*(LogicalNAND_MC-LogicalAND_MC+1)==sizeof(MetaConnectiveNames));

// MetaQuantifier
const char* const QuantificationNames[5] =	{
											PredCalcKeyword_FORALL,
											PredCalcKeyword_THEREIS,
											PredCalcKeyword_FREE,
											PredCalcKeyword_NOTFORALL,
											PredCalcKeyword_THEREISNO
											};

static_assert(sizeof(const char*)*(ThereIsNot_MC-ForAll_MC+1)==sizeof(QuantificationNames));

// Actual implementations of ConstructSelfLengthNameAux()
// NOTE: Name, in the next two routines, relies on implicit null termination.
void SnipText2(char*& Name, size_t nonstrict_lb, size_t strict_ub)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/2000
	assert(Name);
	assert(nonstrict_lb<strict_ub);
	size_t NameLength = ArraySize(Name);
	assert(NameLength>=strict_ub);
	assert(NameLength>strict_ub-nonstrict_lb);
	if (strict_ub<NameLength)
		memmove(Name+nonstrict_lb,Name+strict_ub,NameLength-strict_ub);
	NameLength-=(strict_ub-nonstrict_lb);
	Name = REALLOC(Name,NameLength);
}

// This has to tolerate NULL pointers: it may be called from a SyntaxOKAux failure
std::string ConstructPrefixArgList(const MetaConcept* const* const ArgArray)
{
	std::string ret("(");
	if (ArgArray) {
		const size_t ub = ArraySize(ArgArray);
		size_t i = 0;
		do {
			if (ArgArray[i]) ret += ArgArray[i]->to_s();
			else ret += "\"\"";
			if (++i >= ub) break;
			ret += ',';
		} while (true);
	};
	ret += ')';
	return ret;
}

std::string MetaConcept::to_s() const {
	std::string ret(to_s_aux());

	if (MultiPurposeBitmap & LogicalNegated_VF) ret = "NOT " + ret;
	else if (MultiPurposeBitmap & StdAdditionInv_VF) ret = "-" + ret;
	if (MultiPurposeBitmap & StdMultInv_VF) ret += MULT_INV_TEXT;

	return ret;
}

std::string AbstractClass::to_s_aux() const
{
	if (!ClassName.empty()) return ClassName;
	if (Arg1.empty()) return UNNAMED_CLASS;

	const bool WantSetBraces = Arg1->IsExplicitConstant() || Arg1->IsTypeMatch(LinearInterval_MC, &Integer);
	std::string ret;

	if (WantSetBraces) ret += '{';
	ret += Arg1->to_s();
	if (WantSetBraces) ret += '}';
	return ret;
}

// technically, this is just for infix 2-ary clauses
std::string Clause2Arg::to_s_aux() const
{
	std::string ret(LHS_Arg1->to_s());
	ret += ' ';
	ret += ClauseKeyword ? ClauseKeyword : "\"\"";
	ret += ' ';
	ret += RHS_Arg2->to_s();
	return ret;
}

std::string ClauseNArg::to_s_aux() const
{
	return (ClauseKeyword ? ClauseKeyword : "\"\"") + ConstructPrefixArgList();
}

static std::string ConstructCommaListVarNames(const zaimoni::_meta_autoarray_ptr<MetaConcept*>& args, const size_t origin = 0)
{
	if (args.size() <= origin) return "\"\"";

	std::string ret;
	bool first = true;
	size_t i = -1;
	for (decltype(auto) x : args) {
		if (++i < origin) continue;
		if (!first) ret += ',';
		if (x) ret += x->to_s();
		else ret += "\"\"";
		first = false;
	}
	return ret;
}

static std::string do_not_chain_equals(MetaConcept& x)
{
	const bool need_parens = x.IsExactType(ALLEQUAL_MC) || x.IsExactType(ALLDISTINCT_MC);
	auto staging(x.to_s());
	if (need_parens) {
		staging = "(" + staging;
		staging += ")";
	}
	return staging;
}

static std::string to_s_ALLEQUAL(const zaimoni::_meta_autoarray_ptr<MetaConcept*>& args)
{
	bool first = true;
	std::string ret;
	for (decltype(auto) x : args) {
		if (!first) ret += "==";
		ret += do_not_chain_equals(*x);
		first = false;
	}
	return ret;
}

static std::string to_s_ALLDISTINCT(const zaimoni::_meta_autoarray_ptr<MetaConcept*>& args)
{
	if (2 < args.size()) return EqualRelation_ALLDISTINCT + ConstructPrefixArgList(args);
	bool first = true;
	std::string ret;
	for (decltype(auto) x : args) {
		if (!first) ret += "!=";
		ret += do_not_chain_equals(*x);
		first = false;
	}
	return ret;
}

static std::string to_s_DISTINCTFROMALLOF(const zaimoni::_meta_autoarray_ptr<MetaConcept*>& args)
{
	std::string ret;

	if (3 == args.size()) {
		bool first = true;
		ret += do_not_chain_equals(*args[1]);
		ret += "!=";
		ret += do_not_chain_equals(*args[0]);
		ret += "!=";
		ret += do_not_chain_equals(*args[2]);
	} else {
		ret += '(';
		ret += args[0]->to_s();
		ret += ' ';
		ret += EqualRelation_DISTINCTFROMALLOF;
		ret += ' ';
		ret += ConstructCommaListVarNames(args, 1);
	};
	return ret;
}

static std::string to_s_EQUALTOONEOF(const zaimoni::_meta_autoarray_ptr<MetaConcept*>& args)
{
	std::string ret("(");
	ret += args[0]->to_s();
	ret += ' ';
	ret += EqualRelation_EQUALTOONEOF;
	ret += ' ';
	ret += ConstructCommaListVarNames(args, 1);
	ret += ')';
	return ret;
}

static std::string to_s_NOTALLDISTINCT(const zaimoni::_meta_autoarray_ptr<MetaConcept*>& args)
{
	return EqualRelation_NOTALLDISTINCT + ConstructPrefixArgList(args);
}

static std::string to_s_NOTALLEQUAL(const zaimoni::_meta_autoarray_ptr<MetaConcept*>& args)
{
	return EqualRelation_NOTALLEQUAL + ConstructPrefixArgList(args);
}

std::string EqualRelation::to_s_aux() const
{
	typedef std::string(*aux)(const zaimoni::_meta_autoarray_ptr<MetaConcept*>&);
	static const constexpr aux _s_aux[] = {
		&to_s_ALLEQUAL,
		&to_s_ALLDISTINCT,
		&to_s_EQUALTOONEOF,
		&to_s_DISTINCTFROMALLOF,
		&to_s_NOTALLDISTINCT,
		&to_s_NOTALLEQUAL,
	};
	return (*_s_aux[array_index()])(ArgArray);
}

std::string LinearInterval::to_s_aux() const
{
	std::string ret;

	if (Integer == *IntervalDomain) {
		ret += LHS_Arg1->to_s();
		if (LeftPointOpen && !LHS_Arg1->IsExactType(LinearInfinity_MC)) ret += "+1";
		ret += "...";
		ret += RHS_Arg2->to_s();
		if (RightPointOpen && !RHS_Arg2->IsExactType(LinearInfinity_MC)) ret += "-1";
	} else {	// normal-looking interval
		if (LeftPointOpen)
			ret += (KarlStrombergIntervals) ? ']' : '(';
		else
			ret += '[';
		ret += LHS_Arg1->to_s();

		ret += ',';

		ret += RHS_Arg2->to_s();
		if (RightPointOpen)
			ret += (KarlStrombergIntervals) ? '[' : ')';
		else
			ret += ']';
	}
}

std::string MetaConceptWithArgArray::ConstructPrefixArgList() const
{
	return ::ConstructPrefixArgList(ArgArray);
}

std::string MetaConnective::to_s_aux() const
{
	return MetaConnectiveNames[array_index()] + ConstructPrefixArgList();
}

static std::string Complete_UnaryPrefix(const MetaConcept* src)
{
	std::string ret(" ");

	if (src) ret += src->to_s();
	else ret += "\"\"";

	return ret;
}

static std::string Complete_UnaryFunctionLike(const MetaConcept* src)
{
	std::string ret("(");

	if (src) ret += src->to_s();
	else ret += "\"\"";
	ret += ')';

	return ret;
}

std::string Phrase1Arg::to_s_aux() const
{
	typedef std::string(*aux)(const MetaConcept*);
	static const constexpr aux _s_aux[] = {
		&Complete_UnaryPrefix,
		&Complete_UnaryFunctionLike
	};
	return (PhraseKeyword ? PhraseKeyword : "\"\"")+(*_s_aux[ExactType() - MinPhrase1Idx_MC])(Arg1);
}

// technically, this is just for infix 2-ary phrases
std::string Phrase2Arg::to_s_aux() const
{
	std::string ret(LHS_Arg1->to_s());

	ret += ' ';
	ret += PhraseKeyword ? PhraseKeyword : "\"\"";
	ret += ' ';

	ret += RHS_Arg2->to_s();
	return ret;
}

std::string PhraseNArg::to_s_aux() const
{
	std::string ret;
	if (IsExactType(FREE_PhraseN_MC)) { // postfix
		ret += ConstructCommaListVarNames(ArgArray);
		ret += PhraseKeyword ? PhraseKeyword : "\"\"";
	} else { // prefix
		ret += PhraseKeyword ? PhraseKeyword : "\"\"";
		ret += ConstructCommaListVarNames(ArgArray);
	}
	return ret;
}

bool MetaQuantifier::IsLayoutCompatible(const MetaQuantifier* rhs) const
{
	if (!rhs) return false;
	const auto rtype = rhs->ExactType();
	if (!IsExactType(rtype)) return false;
	if (Free_MC == rtype) return true;
	return *Arg1 == *rhs->Arg1;
}

// The self-name is of the form <reverse-order quantification list><statement>
// the estimator is somewhat unreliable: variables with the same quantification and domain
// should be compacted into a list of var-names later
std::string QuantifiedStatement::to_s_aux() const
{
	if (ArgArray.empty()) return EMPTY_STATEMENT;

	string ret;
	size_t ub = ArgArray.size();
	while (0 < --ub) {
		if (!ArgArray[ub]) {
			ret += "()";
			continue;
		}
		if (1 < ub) {
			const MetaQuantifier* anchor = static_cast<MetaQuantifier*>(ArgArray[ub]);
			size_t lb = ub;
			while (ArgArray[lb - 1] && anchor->IsLayoutCompatible(static_cast<MetaQuantifier*>(ArgArray[lb-1])) && 1< --lb);
			if (lb < ub) {
				ret += static_cast<MetaQuantifier*>(ArgArray[lb])->to_s_start();
				while (lb < ub) {
					ret += ArgArray[ub--]->ViewKeyword();
					ret += ',';
				}
				ret += ArgArray[ub]->ViewKeyword();
				ret += anchor->to_s_end();
				continue;
			}
		}
		ret += ArgArray[ub]->to_s();
	}

	ret += '(';
	if (ArgArray[0]) ret += ArgArray[0]->to_s();
	else ret += "\"\"";
	ret += ')';

	return ret;
}

std::string MetaQuantifier::to_s_start() const
{
	std::string ret("(");
	if (!IsExactType(Free_MC)) {
		ret += QuantificationNames[ExactType() - ForAll_MC];
		ret += ' ';
	}
	return ret;
}

std::string MetaQuantifier::to_s_end() const
{
	std::string ret;
	if (IsExactType(Free_MC)) {
		ret += " IS ";
		ret += QuantificationNames[ExactType() - ForAll_MC];
	} else {
		ret += " IN ";
		ret += Arg1->to_s();
	}
	ret += ')';
	return ret;
}

// The self-name is of the form ([Quantification-type] Var-name [IN <class-name>]|[IS FREE])
std::string MetaQuantifier::to_s_aux() const
{
	std::string ret(to_s_start());
	ret += VariableName;
	ret += to_s_end();
	return ret;
}

// basic version: n-1 + signs, n args
// each arg that isn't higher precedence gets a pair of parentheses
// NOTE: a 0-ary sum returns the universal result 0
// NOTE: a 1-ary sum returns the single arg
std::string StdAddition::to_s_aux() const
{
	if (_IsZero()) return "0";

	bool first = true;
	std::string ret;

	for (decltype(auto) x : ArgArray) {
		if (!first) ret += '+';
		auto staging(x->to_s());
		if (!x->NeverNeedsParentheses()) { // \todo actually...argument has lower precedence than us
			staging = "(" + staging;
			staging += ')';
		};
		ret += staging;
		first = false;
	}
	return ret;
}

std::string StdMultiplication::to_s_aux() const
{
	if (_IsOne()) return "1";

	bool first = true;
	std::string ret;

	for (decltype(auto) x : ArgArray) {
		if (!first) ret += "&middot;";
		auto staging(x->to_s());
		if (!x->NeverNeedsParentheses()) { // \todo actually...argument has lower precedence than us
			staging = "(" + staging;
			staging += ")";
		};
		ret += staging;
		first = false;
	}
	return ret;
}

std::string UnparsedText::to_s_aux() const
{
	std::string ret(Text);
	if (IsUninterpreted()) return ret;
	if (IsHTMLStartTag()) return "<" + ret + ">";
	if (IsHTMLTerminalTag()) return "</" + ret + ">";
	if (IsJSEntity()) return "&" + ret + ";";
	if (IsLogicKeyword()) return "'" + ret + "'";
	if (IsPredCalcKeyword()) return "!" + ret + "!";
	return "\"" + ret + "\"";
}

std::string Variable::to_s_aux() const { return SyntaxOK() ? ViewKeyword() : BAD_VARIABLE_SYNTAX; }
