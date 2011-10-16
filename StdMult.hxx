// StdMult.hxx
// declaration of StdMultiplication, an n-ary operation defined for a variety of sets
// Commutativity is optional (different mode).

#ifndef STD_MULTIPLICATION_DEF
#define STD_MULTIPLICATION_DEF

#include "MetaCon2.hxx"
#include "Class.hxx"

class StdMultiplication;
namespace zaimoni {

template<>
struct is_polymorphic_final<StdMultiplication> : public boost::true_type {};

}

// NOTE: a 0-ary StdMultiplication is an "omnione": a one that matches its context

class StdMultiplication : public MetaConceptWithArgArray
{
	enum SelfEvalRuleIdx_SER{
		AddIntegerToIntegerFraction_SER = MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1,
		AddIntegerFractionToIntegerFraction_SER,
		MultDistributesOverAdd_CondenseProductsOnNonConstArgs_SER,
		CleanIntegerNumeralBlock_SER,
		MaxSelfEvalRuleIdx_SER = CleanIntegerNumeralBlock_SER-MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER
		};

	typedef bool (StdMultiplication::*SelfEvaluateRule)();
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];

	mutable autoval_ptr<AbstractClass> _DynamicType;
	mutable autoval_ptr<AbstractClass> _DesiredType;
public:
	StdMultiplication() : MetaConceptWithArgArray(StdMultiplication_MC) {};
	StdMultiplication(MetaConcept**& NewArgList);
//	StdMultiplication(const StdMultiplication& src);	// default ok
	virtual ~StdMultiplication();

	const StdMultiplication& operator=(const StdMultiplication& src);
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(StdMultiplication*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(StdMultiplication*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
	virtual bool ForceUltimateType(const AbstractClass* const rhs);
//  Evaluation functions
	virtual bool SyntaxOK() const;
	virtual bool StdAddCanDestructiveInteract() const;
	virtual bool StdAddCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual bool SelfInverse(const ExactType_MC Operation);
	virtual bool SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const;
	virtual bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;
//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
	bool DetermineDynamicType(void) const;
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	virtual void _forceStdForm();
	virtual bool _IsExplicitConstant() const;
	virtual bool _IsOne() const;
	virtual bool _IsZero() const;
private:
	void CleanOnes();
	virtual bool DelegateSelfEvaluate();

	bool CleanIntegerNumeralBlock();
	// Following block defined in AddMult.cxx
	bool AddIntegerToIntegerFraction();
	bool AddIntegerFractionToIntegerFraction();
	bool MultDistributesOverAdd_CondenseProductsOnNonConstArgs();
};

#endif
