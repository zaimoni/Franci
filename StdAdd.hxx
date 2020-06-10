// StdAdd.hxx
// declaration of StdAddition, an n-ary commutative operation defined for a variety of sets
// StdAddition is assumed to endow the set it operates on with an abelian group structure

#ifndef STD_ADDITION_DEF
#define STD_ADDITION_DEF

#include "MetaCon2.hxx"
#include "Class.hxx"

class StdAddition;
namespace zaimoni {

template<>
struct is_polymorphic_final<StdAddition> : public std::true_type {};

}

// NOTE: a 0-ary StdAddition is an "omnizero": a zero that matches its context

class StdAddition final : public MetaConceptWithArgArray
{
	enum SelfEvalRuleIdx_SER {
		CleanAddInv_SER = MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1,
		CleanIntegerNumeralBlock_SER,
		EqualArgsToIntegerProduct_SER,
		MaxSelfEvalRuleIdx_SER = EqualArgsToIntegerProduct_SER-MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER
		};

	typedef bool (StdAddition::*SelfEvaluateRule)();
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];

	mutable autoval_ptr<AbstractClass> DynamicType;
	mutable autoval_ptr<AbstractClass> DesiredType;
public:
	StdAddition() noexcept : MetaConceptWithArgArray(StdAddition_MC) {};
	StdAddition(MetaConcept**& NewArgList);
	StdAddition(const StdAddition& src) = default;
	StdAddition(StdAddition&& src) = default;
	StdAddition& operator=(const StdAddition & src); // ACID
	StdAddition& operator=(StdAddition&& src) = default;
	virtual ~StdAddition() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(StdAddition*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(StdAddition*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
	virtual bool ForceUltimateType(const AbstractClass* const rhs);
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	void _forceStdForm() override;
	virtual bool _IsExplicitConstant() const;
	virtual bool _IsZero() const;
	virtual bool SelfInverse(const ExactType_MC Operation);
	virtual bool SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const;
	virtual bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
	bool DetermineDynamicType() const;
private:
	void CleanZeros();

	virtual bool DelegateSelfEvaluate();		// same type
	bool CleanAddInv();
	// Following block defined in AddMult.cxx
	bool CleanIntegerNumeralBlock();
	bool EqualArgsToIntegerProduct();
	void AddTwoOneOverIntegers();
	void AddThreeOneOverIntegers();
};

#endif
