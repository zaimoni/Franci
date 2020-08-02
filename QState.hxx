// QState.hxx
// header for QuantifiedStatement

#ifndef QUANTIFIED_STATEMENT_DEF
#define QUANTIFIED_STATEMENT_DEF

#include "MetaCon2.hxx"

class QuantifiedStatement;
namespace zaimoni {

template<>
struct is_polymorphic_final<QuantifiedStatement> : public std::true_type {};

}

class QuantifiedStatement final : public MetaConceptWithArgArray
{
	enum SelfEvalRuleIdx_SER{
		SortQuantifiers_SER = MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1,
		MaxSelfEvalRuleIdx_SER = SortQuantifiers_SER-MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER
		};

	typedef bool (QuantifiedStatement::*SelfEvaluateRule)();
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];

	unsigned char QuantifiersExplicitlySorted;
	QuantifiedStatement& operator=(const QuantifiedStatement& src);	// ACID

public:
	QuantifiedStatement() noexcept : MetaConceptWithArgArray(QuantifiedStatement_MC),QuantifiersExplicitlySorted('\x00') {};
	QuantifiedStatement(const QuantifiedStatement& src);
	QuantifiedStatement(QuantifiedStatement&& src) = default;
	QuantifiedStatement& operator=(QuantifiedStatement&& src) = default;
	virtual ~QuantifiedStatement();
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(QuantifiedStatement*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(QuantifiedStatement*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	const AbstractClass* UltimateType() const override;
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	void SetExplicitSort() {QuantifiersExplicitlySorted = '\x01';};
	void ResetExplicitSort() {QuantifiersExplicitlySorted = '\x00';};
	bool Explore(const clock_t EvalTime0, bool DoNotExplain, MetaConcept**& PlausibleVarList);
	bool WantStateDump() const;

protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	std::string to_s_aux() const override;
	void _forceStdForm() override;

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
//  functions specific to QuantifiedStatement
	void AllVarsNotInThisStatement(unsigned long*& VectorBuffer) const;

private:
	void _ForceArgSameImplementation(size_t n) override;

	// if these three succeed i.e. return true, ExperimentalArg0 is cleaned
	bool ScreenVarList(const clock_t EvalTime0, bool DoNotExplain, MetaConcept**& VarList, const char* const FailureMessage, MetaConnective*& ExperimentalArg0);
	virtual bool DelegateSelfEvaluate();		// same type
	bool SortQuantifiers();
};

namespace zaimoni
{

template<>
struct has_invalid_assignment_but_copyconstructable<QuantifiedStatement> : public std::true_type {};

}

#endif
