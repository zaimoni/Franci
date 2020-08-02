// Phrase2.hxx
// header for Phrase2Arg, the metatype for n-ary clauses

#ifndef PHRASE_2ARG_DEF
#define PHRASE_2ARG_DEF

#include "MetaCon4.hxx"

class Phrase2Arg;
namespace zaimoni {

template<>
struct is_polymorphic_final<Phrase2Arg> : public std::true_type {};

}

class Phrase2Arg final : public MetaConceptWith2Args
{
private:
	enum EvalRuleIdx_ER	{
						ConvertToStdAddition_ER= 1+MetaConceptWith2Args::MaxEvalRuleIdx_ER,
						ConvertToStdMultiplication_ER,
						ConvertToPermutation_ER,
						ConvertToCombination_ER,
						MaxEvalRuleIdx_ER = ConvertToCombination_ER-MetaConceptWith2Args::MaxEvalRuleIdx_ER
						};

	typedef bool (Phrase2Arg::*EvaluateToOtherRule)(MetaConcept*& dest);
	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];
	const char* PhraseKeyword;	// this controls the intended semantics
								// Phrase2Arg does *NOT* own this!

public:
	Phrase2Arg() = delete;
	Phrase2Arg(MetaConcept**& src, size_t& KeywordIdx);
	Phrase2Arg(const Phrase2Arg& src) = default;
	Phrase2Arg(Phrase2Arg&& src) = default;
	Phrase2Arg& operator=(const Phrase2Arg & src) = default;
	Phrase2Arg& operator=(Phrase2Arg&& src) = default;
	virtual ~Phrase2Arg() = default;
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Phrase2Arg*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(Phrase2Arg*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	const AbstractClass* UltimateType() const override { return 0; }

//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	const char* ViewKeyword() const override { return PhraseKeyword; }
// type-specific functions
	static ExactType_MC CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx);
	constexpr static ExactType_MC CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx) { return Unknown_MC; }

protected:
	std::string to_s_aux() const override;
	virtual void DiagnoseInferenceRules();
	bool InvokeEqualArgRule() { return false; }

private:
	virtual bool DelegateEvaluate(MetaConcept*& dest);		// same, or different type
	static ExactType_MC CanConstruct(const MetaConcept* const * src, size_t KeywordIdx);
	bool ConvertToStdAddition(MetaConcept*& dest);
	bool ConvertToStdMultiplication(MetaConcept*& dest);
	bool ConvertToPermutation(MetaConcept*& dest);
	bool ConvertToCombination(MetaConcept*& dest);
};

#endif
