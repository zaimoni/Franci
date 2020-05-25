// ClauseN.hxx
// header for ClauseNArg, the metatype for n-ary clauses

#ifndef CLAUSE_NARG_DEF
#define CLAUSE_NARG_DEF

#include "MetaCon2.hxx"

class ClauseNArg;
namespace zaimoni {

template<>
struct is_polymorphic_final<ClauseNArg> : public std::true_type {};

}

class ClauseNArg final : public MetaConceptWithArgArray
{
private:
enum EvalRuleIdx_ER	{
					ConvertToMetaConnective_ER = MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1,
					ConvertToEqualRelation_ER,
					MaxEvalRuleIdx_ER = ConvertToEqualRelation_ER-MetaConceptWithArgArray::MaxEvalRuleIdx_ER
					};

	typedef bool (ClauseNArg::*EvaluateToOtherRule)(MetaConcept*& dest);
	typedef size_t (ClauseNArg::*LengthOfSelfNameAuxFunc)(void) const;
	typedef void (ClauseNArg::*ConstructSelfNameAuxFunc)(char* Name) const;
	typedef bool (ClauseNArg::*SyntaxOKAuxFunc)(void) const;

	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];

	const char* ClauseKeyword;	// this controls the intended semantics
								// ClauseNArg does *NOT* own this!
	static LengthOfSelfNameAuxFunc LengthOfSelfNameAuxArray[(MaxClauseNIdx_MC-MinClauseNIdx_MC)+1];
	static ConstructSelfNameAuxFunc ConstructSelfNameAuxArray[(MaxClauseNIdx_MC-MinClauseNIdx_MC)+1];
	static SyntaxOKAuxFunc SyntaxOKAuxArray[(MaxClauseNIdx_MC-MinClauseNIdx_MC)+1];

	ClauseNArg(void) {};
public:
	ClauseNArg(MetaConcept**& src, size_t& KeywordIdx);
//	ClauseNArg(const ClauseNArg& src);	// default OK
	virtual ~ClauseNArg() = default;
//	const ClauseNArg& operator=(const ClauseNArg& src);	// default OK
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(ClauseNArg*& dest) {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure; success destroys integrity of source
	void MoveInto(ClauseNArg*& dest);	// can throw memory failure; success destroys integrity of source

//  Type ID functions
	const AbstractClass* UltimateType() const override;

//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
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
	virtual void ConstructSelfNameAux(char* Name) const;	// overwrites what is already there
	void _forceStdForm() override;

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
private:
	bool SyntaxOKArglistTVal() const;
	bool SyntaxOKNoExtraInfo() const;

	size_t LengthOfSelfNamePrefixArglist() const;
	void ConstructSelfNamePrefixArglist(char* Name) const;	// overwrites what is already there

	static ExactType_MC CanConstruct(const MetaConcept* const * src, size_t KeywordIdx);
	void ExtractPrefixArglist(MetaConcept**& src, size_t KeywordIdx);
	void Extract1ArgBeforePrefixCommalist(MetaConcept**& src, size_t& KeywordIdx);
	size_t array_index() const {return ExactType()-MinClauseNIdx_MC;};
	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;

	virtual bool DelegateEvaluate(MetaConcept*& dest);		// same, or different type
	bool ConvertToMetaConnective(MetaConcept*& dest);
	bool ConvertToEqualRelation(MetaConcept*& dest);
};
#endif
