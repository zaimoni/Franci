// MetaCon5.hxx
// header for MetaConceptWith1Args

#if !defined(METACONCEPT_1ARG_DEF)
#define METACONCEPT_1ARG_DEF

#include "MetaCon1.hxx"
#include "Zaimoni.STL/AutoPtr.hpp"

class MetaConceptWith1Arg : public MetaConcept
{
protected:
	autoval_ptr<MetaConcept> Arg1;
	MetaConceptWith1Arg() {};
	MetaConceptWith1Arg(const MetaConceptWith1Arg& src) : MetaConcept(src),Arg1(src.Arg1) {};
	explicit MetaConceptWith1Arg(ExactType_MC NewType) : MetaConcept(NewType) {};
	explicit MetaConceptWith1Arg(ExactType_MC NewType,unsigned char NewBitmap) : MetaConcept(NewType,NewBitmap) {};
	explicit MetaConceptWith1Arg(ExactType_MC NewType,MetaConcept*& NewArg1) : MetaConcept(NewType),Arg1(NewArg1) {};
	explicit MetaConceptWith1Arg(ExactType_MC NewType,unsigned char NewBitmap,MetaConcept*& NewArg1) : MetaConcept(NewType,NewBitmap),Arg1(NewArg1) {};
	void operator=(const MetaConceptWith1Arg& src)
		{	Arg1=src.Arg1;
			MetaConcept::operator=(src);
		};
public:
	virtual ~MetaConceptWith1Arg() = default;
//	virtual void CopyInto(MetaConcept*& dest) const = 0;	// can throw memory failure
//	virtual void MoveInto(MetaConcept*& dest) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveIntoAux(MetaConceptWith1Arg& dest);

//  Type ID functions
//	virtual const AbstractClass* UltimateType() const = 0;
	virtual size_t size() const {return 1;};
	virtual const MetaConcept* ArgN(size_t n) const;
	virtual MetaConcept* ArgN(size_t n);
// Syntactical equality and inequality
	bool IsAbstractClassDomain() const override { return IsExplicitConstant(); }
//  Evaluation functions
	virtual bool CanEvaluate() const = 0;
	virtual bool CanEvaluateToSameType() const = 0;
	virtual bool SyntaxOK() const = 0;
	virtual bool Evaluate(MetaConcept*& dest) = 0;		// same, or different type
	virtual bool DestructiveEvaluateToSameType() = 0;	// overwrites itself iff returns true
	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src);
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const;
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const;
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation);
// NOTE: we may need this further down
// Formal manipulation functions
//	virtual bool SelfLogicalNOT(void);	// instantiate when above is true
//	virtual bool DetectAntiIdempotent(const MetaConcept& Arg2) const;
protected:
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	void _forceStdForm() override;
	virtual bool _IsExplicitConstant() const;
};

namespace zaimoni {

template<>
struct is_polymorphic_base<MetaConceptWith1Arg> : public std::true_type {};

}

#endif
