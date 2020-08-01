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
struct is_polymorphic_final<StdMultiplication> : public std::true_type {};

}

// NOTE: a 0-ary StdMultiplication is an "omnione": a one that matches its context

class StdMultiplication final : public MetaConceptWithArgArray
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
	StdMultiplication() noexcept : MetaConceptWithArgArray(StdMultiplication_MC) {};
	StdMultiplication(MetaConcept**& NewArgList);
	StdMultiplication(const StdMultiplication& src) = default;
	StdMultiplication(StdMultiplication&& src) = default;
	StdMultiplication& operator=(const StdMultiplication& src);
	StdMultiplication& operator=(StdMultiplication&& src) = default;
	virtual ~StdMultiplication() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(StdMultiplication*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(StdMultiplication*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
	virtual bool ForceUltimateType(const AbstractClass* const rhs);
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
	virtual bool StdAddCanDestructiveInteract() const;
	virtual bool StdAddCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const;
// text I/O functions
#ifndef USE_TO_S
	virtual size_t LengthOfSelfName() const;
#endif
	virtual bool SelfInverse(const ExactType_MC Operation);
	virtual bool SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const;
	virtual bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;
//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
	bool DetermineDynamicType(void) const;
protected:
#ifndef USE_TO_S
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
#endif
	void _forceStdForm() override;
	virtual bool _IsExplicitConstant() const;
	virtual bool _IsOne() const;
	virtual bool _IsZero() const;
private:
	void _ForceArgSameImplementation(size_t n) override;

	void CleanOnes();
	virtual bool DelegateSelfEvaluate();

	bool CleanIntegerNumeralBlock();
	// Following block defined in AddMult.cxx
	bool AddIntegerToIntegerFraction();
	bool AddIntegerFractionToIntegerFraction();
	bool MultDistributesOverAdd_CondenseProductsOnNonConstArgs();
};

#endif
