// MetaCon6.hxx
// header for MetaConceptZeroArgs

#ifndef METACONCEPT_ZERO
#define METACONCEPT_ZERO

#include "MetaCon1.hxx"

class MetaConceptZeroArgs : public MetaConcept
{
private:
	MetaConceptZeroArgs() {};
protected:
	explicit MetaConceptZeroArgs(ExactType_MC NewType) : MetaConcept(NewType) {};
	explicit MetaConceptZeroArgs(ExactType_MC NewType,unsigned char NewBitmap) : MetaConcept(NewType,NewBitmap) {};
	MetaConceptZeroArgs(const MetaConceptZeroArgs& src) = default;
	MetaConceptZeroArgs(MetaConceptZeroArgs&& src) = default;
	MetaConceptZeroArgs& operator=(const MetaConceptZeroArgs & src) = default;
	MetaConceptZeroArgs& operator=(MetaConceptZeroArgs&& src) = default;
	virtual ~MetaConceptZeroArgs() = default;
public:
//	virtual void CopyInto(MetaConcept*& dest) const = 0;	// can throw memory failure
//	virtual void MoveInto(MetaConcept*& dest) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
//	virtual const AbstractClass* UltimateType() const = 0;
//	Arity functions
	size_t size() const override final {return 0;};
	const MetaConcept* ArgN(size_t n) const override final {return 0;};
	MetaConcept* ArgN(size_t n) override final {return 0;};
// Syntactical equality and inequality
	bool IsAbstractClassDomain() const override {return true;};
//  Evaluation functions
//	virtual bool CanEvaluate() const = 0;
//	virtual bool CanEvaluateToSameType() const = 0;
//	virtual bool SyntaxOK() const = 0;
//	virtual bool Evaluate(MetaConcept*& dest) = 0;		// same, or different type
//	virtual bool DestructiveEvaluateToSameType() = 0;	// overwrites itself iff returns true
// Formal manipulation functions
	void ConvertVariableToCurrentQuantification(MetaQuantifier& src) override {};
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const;
	bool UsesQuantifierAux(const MetaQuantifier& x) const override {return false;};
	bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) override { return true; }
protected:
//	virtual bool EqualAux2(const MetaConcept& rhs) const = 0;
	void _forceStdForm() override {};
	bool _IsExplicitConstant() const override {return true;};
};

namespace zaimoni {

template<>
struct is_polymorphic_base<MetaConceptZeroArgs> : public std::true_type {};

}

#endif
