// MetaCon4.hxx
// header for MetaConceptWith2Args

#ifndef METACONCEPT_2ARGS_DEF
#define METACONCEPT_2ARGS_DEF

#include "MetaCon1.hxx"
#include "Zaimoni.STL/AutoPtr.hpp"

class MetaConceptWith2Args : public MetaConcept
{
public:
	enum EvalRuleIdx_ER	{
						None_ER = 0,
						ForceLHSArg1_ER,
						ForceRHSArg2_ER,
						MaxEvalRuleIdx_ER = ForceRHSArg2_ER
						};
	enum SelfEvalRuleIdx_SER{
							SelfEvalSyntaxBadNoRules_SER = -2,	// flag: this is stable
							SelfEvalSyntaxOKNoRules_SER,	// flag: this is stable
							None_SER,
							EvaluateLHSArg1_SER,
							EvaluateRHSArg2_SER,
							EvaluateBothArgs_SER,
							MaxSelfEvalRuleIdx_SER = EvaluateBothArgs_SER
							};
protected:
	typedef bool (MetaConceptWith2Args::*EvaluateToOtherRule)(MetaConcept*& dest);
	typedef bool (MetaConceptWith2Args::*SelfEvaluateRule)(void);

	autoval_ptr<MetaConcept> LHS_Arg1;
	autoval_ptr<MetaConcept> RHS_Arg2;
	mutable unsigned short IdxCurrentEvalRule;
	mutable signed short IdxCurrentSelfEvalRule;

	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];

	MetaConceptWith2Args() : IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWith2Args(ExactType_MC NewType) : MetaConcept(NewType),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWith2Args(ExactType_MC NewType,unsigned char NewBitmap) : MetaConcept(NewType,NewBitmap),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWith2Args(ExactType_MC NewType,MetaConcept*& lhs,MetaConcept*& rhs) : MetaConcept(NewType),LHS_Arg1(lhs),RHS_Arg2(rhs),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWith2Args(ExactType_MC NewType,unsigned char NewBitmap,MetaConcept*& lhs,MetaConcept*& rhs) : MetaConcept(NewType,NewBitmap),LHS_Arg1(lhs),RHS_Arg2(rhs),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	MetaConceptWith2Args(const MetaConceptWith2Args& src);
	void operator=(const MetaConceptWith2Args& src);
public:
	virtual ~MetaConceptWith2Args();
	virtual void CopyInto(MetaConcept*& dest) const = 0;	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.
	inline void ForceCheckForEvaluation() const {IdxCurrentSelfEvalRule=None_SER;};
//  Type ID functions
	virtual const AbstractClass* UltimateType() const = 0;
	virtual size_t size() const {return 2;};
	virtual const MetaConcept* ArgN(size_t n) const;
	virtual MetaConcept* ArgN(size_t n);
// Syntactical equality and inequality
//	Override this for non-commutative types, or complicated internal structure
	virtual bool IsAbstractClassDomain() const;
//  Evaluation functions
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const = 0;
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true
// NOTE: we may need this further down
// Formal manipulation functions
//	virtual bool SelfLogicalNOT(void);	// instantiate when above is true
//	virtual bool DetectAntiIdempotent(const MetaConcept& Arg2) const;
	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src);
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const;
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const;
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation);
	// Next three routines really also need transitivity
	bool OrderIndependentPairwiseRelation(const MetaConceptWith2Args& rhs, LowLevelBinaryRelation& TargetRelation) const;
	bool ArgsPairwiseUnrelated(LowLevelBinaryRelation* TargetRelation) const {return !TargetRelation(*LHS_Arg1,*RHS_Arg2);};
	bool ExactOrderPairwiseRelation(const MetaConceptWith2Args& rhs, LowLevelBinaryRelation* TargetRelation) const;
	// type-specific routines
	void ForceTotalLexicalArgOrder();
	void TransferOutLHSAndNULL(MetaConcept*& dest) {LHS_Arg1.TransferOutAndNULL(dest);};
	void TransferOutRHSAndNULL(MetaConcept*& dest) {RHS_Arg2.TransferOutAndNULL(dest);};
protected:
	void MoveIntoAux(MetaConceptWith2Args& dest);
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual void _forceStdForm();
	virtual bool _IsExplicitConstant() const;

	virtual void DiagnoseInferenceRules(void) = 0;	// This is *not* the Interface!
	// implemented in MetaCon4.cxx
	bool EvaluateLHSArg1();
	bool EvaluateRHSArg2();
	bool EvaluateBothArgs();
	bool ForceLHSArg1(MetaConcept*& dest);
	bool ForceRHSArg2(MetaConcept*& dest);
	// implemented in Clause2.cxx
	void ExtractInfixArglist(MetaConcept**& Target, size_t& KeywordIdx);
	void ExtractPrefixArglist(MetaConcept**& Target, size_t KeywordIdx);
private:
	virtual bool DelegateEvaluate(MetaConcept*& dest) {return false;};		// same, or different type
};

namespace zaimoni {

template<>
struct is_polymorphic_base<MetaConceptWith2Args> : public std::true_type {};

}

#endif
