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
	MetaConceptZeroArgs(const MetaConceptZeroArgs& src) : MetaConcept(src) {};
	virtual ~MetaConceptZeroArgs();
	void operator=(const MetaConceptZeroArgs& src) {MetaConcept::operator=(src);};
public:
//	virtual void CopyInto(MetaConcept*& dest) const = 0;	// can throw memory failure
//	virtual void MoveInto(MetaConcept*& dest) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
//	virtual const AbstractClass* UltimateType() const = 0;
//	Arity functions
	virtual size_t size() const {return 0;};
	virtual const MetaConcept* ArgN(size_t n) const {return 0;};
	virtual MetaConcept* ArgN(size_t n) {return 0;};
// Syntactical equality and inequality
	virtual bool IsAbstractClassDomain() const {return true;};
//  Evaluation functions
//	virtual bool CanEvaluate() const = 0;
//	virtual bool CanEvaluateToSameType() const = 0;
//	virtual bool SyntaxOK() const = 0;
//	virtual bool Evaluate(MetaConcept*& dest) = 0;		// same, or different type
//	virtual bool DestructiveEvaluateToSameType() = 0;	// overwrites itself iff returns true
// Formal manipulation functions
	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src) {};
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const;
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const {return false;};
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation);
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const = 0;
	void _forceStdForm() override {};
	virtual bool _IsExplicitConstant() const {return true;};
};

namespace zaimoni {

template<>
struct is_polymorphic_base<MetaConceptZeroArgs> : public std::true_type {};

}

#endif
