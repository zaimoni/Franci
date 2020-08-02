// SymConst.hxx
// header for SymbolicConstant, the class that implements symbolic constants.
// TruthValues are not handled here: that is the TruthValue class

#ifndef SYMBOLIC_CONSTANT_DEF
#define SYMBOLIC_CONSTANT_DEF

#include "MetaCon6.hxx"

// NOTE: second enumerated value requires implementation of HasSameImplementationAs
enum SymConstantIndex	{
						LinearInfinity_SC = 0	// +-inf for real numbers
												// Supports AddInv; inf-inf fails for StdAddition, however
												// Automatically fails MultInv
												// NOT ALLOWED IN COMPLEX-VALUED EXPRESSIONS
						};

class SymbolicConstant;
namespace zaimoni {

template<>
struct is_polymorphic_final<SymbolicConstant> : public std::true_type {};

}

class SymbolicConstant final : public MetaConceptZeroArgs
{
public:
	SymbolicConstant(SymConstantIndex ExactSymConstant) noexcept : MetaConceptZeroArgs((ExactType_MC)(LinearInfinity_MC + ExactSymConstant)) {}
	SymbolicConstant(const SymbolicConstant& src) = default;
	SymbolicConstant(SymbolicConstant&& src) = default;
	SymbolicConstant& operator=(const SymbolicConstant& src) = default;
	SymbolicConstant& operator=(SymbolicConstant&& src) = default;
	virtual ~SymbolicConstant() = default;

	void CopyInto(MetaConcept*& dest) const override {zaimoni::CopyInto(*this,dest);};	// can throw memory failure
	void CopyInto(SymbolicConstant*& dest) const {zaimoni::CopyInto(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(SymbolicConstant*& dest) {zaimoni::MoveIntoV2(std::move(*this),dest);}

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;

//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override { return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >(); }
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;				
	virtual bool SyntaxOK() const;							
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true

// Formal manipulation functions
	virtual bool SelfInverse(const ExactType_MC Operation);
	virtual bool SelfInverseTo(const MetaConcept& RHS, const ExactType_MC Operation) const;

	virtual bool IsPositive() const;	// needs total order *and* IsZero
	virtual bool IsNegative() const;	// needs total order *and* IsZero

protected:
	bool EqualAux2(const MetaConcept& rhs) const override { return true; }
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual bool SyntacticalStandardLTAux(const MetaConcept& rhs) const;
	std::string to_s_aux() const override;

private:
	bool SymbolicConstantSyntacticalLTReal(const MetaConcept& rhs) const;
	bool SymbolicConstantSyntacticalGTReal(const MetaConcept& rhs) const;
};

#endif
