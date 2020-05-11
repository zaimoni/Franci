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
	Variable(MetaQuantifier* NewName) :	MetaConceptZeroArgs(Variable_MC),Arg1(NewName) {};	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2006
//	Variable(const Variable& src); default ok
	~Variable();
//	const Variable& operator=(const Variable& src);	// default ok

// Inherited from MetaConcept
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Variable*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(Variable*& dest) {zaimoni::CopyInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	virtual bool IsAbstractClassDomain() const {return false;};
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
	virtual bool ForceUltimateType(const AbstractClass* const rhs);
//  Evaluation functions
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const;							
	virtual bool Evaluate(MetaConcept*& dest);
	virtual bool DestructiveEvaluateToSameType();
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation);
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual const char* ViewKeyword() const {return Arg1->ViewKeyword();};
// Formal manipulation functions
	// These two must be conditionally implemented: their overrides work only when the
	// variable is of type TruthValue
	virtual void SelfLogicalNOT();	// instantiate when UltimateType is TruthValues
	virtual bool LogicalANDOrthogonalClause() const;
	virtual bool StrictlyImplies(const MetaConcept& rhs) const;
	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src);
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const;

// type-specific functions
	bool IsLogicalNegatedVar(void) const {return ((unsigned char)(LogicalNegated_VF) & MultiPurposeBitmap) ? true : false;};
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	void _forceStdForm() override;
	virtual bool _IsExplicitConstant() const {return false;};
private:
	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;
};

#endif
