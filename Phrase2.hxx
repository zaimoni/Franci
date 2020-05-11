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

	Phrase2Arg() {};
public:
	Phrase2Arg(MetaConcept**& src, size_t& KeywordIdx);
//	Phrase2Arg(const Phrase2Arg& src);	// default OK
	virtual ~Phrase2Arg() = default;
//	const Phrase2Arg& operator=(const Phrase2Arg& src); //	default ok
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Phrase2Arg*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure; success destroys integrity of source
	void MoveInto(Phrase2Arg*& dest);	// can throw memory failure; success destroys integrity of source

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;

//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual const char* ViewKeyword() const;
// type-specific functions
	static ExactType_MC CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx);
	static ExactType_MC CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx);
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there

	virtual void DiagnoseInferenceRules();
	virtual bool InvokeEqualArgRule();
private:
	virtual bool DelegateEvaluate(MetaConcept*& dest);		// same, or different type
	static ExactType_MC CanConstruct(const MetaConcept* const * src, size_t KeywordIdx);
	bool ConvertToStdAddition(MetaConcept*& dest);
	bool ConvertToStdMultiplication(MetaConcept*& dest);
	bool ConvertToPermutation(MetaConcept*& dest);
	bool ConvertToCombination(MetaConcept*& dest);
};

#endif
