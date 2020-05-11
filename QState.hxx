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
	const QuantifiedStatement& operator=(const QuantifiedStatement& src);
public:
	QuantifiedStatement() :	MetaConceptWithArgArray(QuantifiedStatement_MC),QuantifiersExplicitlySorted('\x00') {};
	QuantifiedStatement(const QuantifiedStatement& src);
	virtual ~QuantifiedStatement();
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(QuantifiedStatement*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(QuantifiedStatement*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	void SetExplicitSort() {QuantifiersExplicitlySorted = '\x01';};
	void ResetExplicitSort() {QuantifiersExplicitlySorted = '\x00';};
	bool Explore(const clock_t EvalTime0, bool DoNotExplain, MetaConcept**& PlausibleVarList);
	bool WantStateDump() const;
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	void _forceStdForm() override;

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
//  functions specific to QuantifiedStatement
	void AllVarsNotInThisStatement(unsigned long*& VectorBuffer) const;
private:
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
