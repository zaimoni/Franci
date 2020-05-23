// Variable.hxx
// header for class Variable

#ifndef VARIABLE_DEF
#define VARIABLE_DEF

#include "MetaCon6.hxx"
#include "Quantify.hxx"

class Variable;
namespace zaimoni {

template<>
struct is_polymorphic_final<Variable> : public std::true_type {};

}

class AbstractClass;

// NOTE: Variable does *not* own its domain!
class Variable final : public MetaConceptZeroArgs
{
protected:
	MetaQuantifier* Arg1;	// cannot be const MetaQuantifier* because of ForceUltimateType
public:
	Variable(MetaQuantifier* NewName) noexcept : MetaConceptZeroArgs(Variable_MC),Arg1(NewName) {}	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2006
	Variable(const Variable& src) = default;
	Variable(Variable&& src) = default;
	Variable& operator=(const Variable& src) = default;
	Variable& operator=(Variable&& src) = default;
	~Variable() = default;

// Inherited from MetaConcept
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);}	// can throw memory failure
	void CopyInto(Variable*& dest) const;	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(Variable*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
	bool IsAbstractClassDomain() const override {return false;};
//  Type ID functions
	const AbstractClass* UltimateType() const override { return Arg1->UltimateType(); };
	bool ForceUltimateType(const AbstractClass* const rhs) override { return Arg1->ForceUltimateType(rhs); }
//  Evaluation functions
	bool CanEvaluate() const override { return false; };
	bool CanEvaluateToSameType() const override { return false; };
	bool SyntaxOK() const override;
	bool Evaluate(MetaConcept*& dest) override { return false; }
	bool DestructiveEvaluateToSameType() override { return false; }
	bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) override;
// text I/O functions
	size_t LengthOfSelfName() const override;
	const char* ViewKeyword() const override {return Arg1->ViewKeyword();};
// Formal manipulation functions
	// These two must be conditionally implemented: their overrides work only when the
	// variable is of type TruthValue
	void SelfLogicalNOT() override;	// instantiate when UltimateType is TruthValues
	bool LogicalANDOrthogonalClause() const override { return true; }
	bool StrictlyImplies(const MetaConcept& rhs) const override;
	void ConvertVariableToCurrentQuantification(MetaQuantifier& src) override;
	bool UsesQuantifierAux(const MetaQuantifier& x) const override { return x.MetaConceptPtrUsesThisQuantifier(Arg1); }

// type-specific functions
	bool IsLogicalNegatedVar(void) const {return ((unsigned char)(LogicalNegated_VF) & MultiPurposeBitmap) ? true : false;};
protected:
	bool EqualAux2(const MetaConcept& rhs) const override;
	bool InternalDataLTAux(const MetaConcept& rhs) const override;
	void ConstructSelfNameAux(char* Name) const override; // overwrites what is already there
	void _forceStdForm() override {};
	bool _IsExplicitConstant() const override {return false;};
private:
	bool isAntiIdempotentTo(const MetaConcept& rhs) const override;
};

#endif
