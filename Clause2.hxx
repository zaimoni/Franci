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

class Clause2Arg: public MetaConceptWith2Args
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

	Clause2Arg() {};
public:
	Clause2Arg(MetaConcept**& src, size_t& KeywordIdx);
//	Clause2Arg(const Clause2Arg& src);	// default OK
	virtual ~Clause2Arg();
//	const Clause2Arg& operator=(const Clause2Arg& src);	// default OK
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Clause2Arg*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure; success destroys integrity of source
	void MoveInto(Clause2Arg*& dest);	// can throw memory failure; success destroys integrity of source

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual const char* ViewKeyword() const;
// Formal manipulation functions
	virtual void SelfLogicalNOT();	// instantiate when UltimateType is TruthValues
// type-specific functions
	static ExactType_MC CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx);
	static ExactType_MC CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx);
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there

	virtual void DiagnoseInferenceRules();
	virtual bool InvokeEqualArgRule();
private:
	virtual bool DelegateEvaluate(MetaConcept*& dest);		// same, or different type
	bool ConvertToMetaConnective(MetaConcept*& dest);
	static ExactType_MC CanConstruct(const MetaConcept* const * src, size_t KeywordIdx);
	size_t array_index() const {return ExactType()-MinClause2Idx_MC;};
	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;
};

#endif
