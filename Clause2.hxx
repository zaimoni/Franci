// Clause2.hxx
// header for Clause2Arg, the metatype for n-ary clauses

#ifndef CLAUSE_2ARG_DEF
#define CLAUSE_2ARG_DEF

#include "MetaCon4.hxx"

class Clause2Arg;
namespace zaimoni {

template<>
struct is_polymorphic_final<Clause2Arg> : public std::true_type {};

}

class Clause2Arg final : public MetaConceptWith2Args
{
private:
	enum EvalRuleIdx_ER	{
						ConvertToMetaConnective_ER = 1+MetaConceptWith2Args::MaxEvalRuleIdx_ER,
						MaxEvalRuleIdx_ER = ConvertToMetaConnective_ER-MetaConceptWith2Args::MaxEvalRuleIdx_ER
						};

	typedef bool (Clause2Arg::*EvaluateToOtherRule)(MetaConcept*& dest);
	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];
	const char* ClauseKeyword;	// this controls the intended semantics
								// Clause2Arg does *NOT* own this!

public:
	Clause2Arg() = delete;
	Clause2Arg(MetaConcept**& src, size_t& KeywordIdx);
	Clause2Arg(const Clause2Arg& src) = default;
	Clause2Arg(Clause2Arg&& src) = default;
	Clause2Arg& operator=(const Clause2Arg & src) = default;
	Clause2Arg& operator=(Clause2Arg&& src) = default;
	virtual ~Clause2Arg() = default;
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Clause2Arg*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this),dest); }
	void MoveInto(Clause2Arg*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	const AbstractClass* UltimateType() const override;
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	const char* ViewKeyword() const override { return ClauseKeyword; }
// Formal manipulation functions
	virtual void SelfLogicalNOT();	// instantiate when UltimateType is TruthValues
// type-specific functions
	static ExactType_MC CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx);
	constexpr static ExactType_MC CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx) { return Unknown_MC; }
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there

	virtual void DiagnoseInferenceRules();
	bool InvokeEqualArgRule() { return false; }
private:
	virtual bool DelegateEvaluate(MetaConcept*& dest);		// same, or different type
	bool ConvertToMetaConnective(MetaConcept*& dest);
	static ExactType_MC CanConstruct(const MetaConcept* const * src, size_t KeywordIdx);
	size_t array_index() const {return ExactType()-MinClause2Idx_MC;};
	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;
};

#endif
