// MetaCon5.hxx
// header for MetaConceptWith1Args

#if !defined(METACONCEPT_1ARG_DEF)
#define METACONCEPT_1ARG_DEF

#include "MetaCon1.hxx"
#include "Zaimoni.STL/AutoPtr.hpp"

template<class T=MetaConcept>
class MetaConceptWith1Arg : public MetaConcept
{
	static_assert(std::is_base_of_v<MetaConcept,T>);
protected:
	autoval_ptr<T> Arg1;
	MetaConceptWith1Arg() = default;
	MetaConceptWith1Arg(const MetaConceptWith1Arg& src) = default;
	MetaConceptWith1Arg(MetaConceptWith1Arg&& src) = default;
	explicit MetaConceptWith1Arg(ExactType_MC NewType) noexcept : MetaConcept(NewType) {};
	explicit MetaConceptWith1Arg(ExactType_MC NewType,unsigned char NewBitmap) noexcept : MetaConcept(NewType,NewBitmap) {};
	explicit MetaConceptWith1Arg(ExactType_MC NewType,MetaConcept*& NewArg1) noexcept : MetaConcept(NewType),Arg1(NewArg1) {};
	explicit MetaConceptWith1Arg(ExactType_MC NewType,unsigned char NewBitmap,MetaConcept*& NewArg1) noexcept : MetaConcept(NewType,NewBitmap),Arg1(NewArg1) {};
	void operator=(const MetaConceptWith1Arg& src)
		{	Arg1=src.Arg1;
			MetaConcept::operator=(src);
		};
	MetaConceptWith1Arg& operator=(MetaConceptWith1Arg&& src) = default;
public:
	virtual ~MetaConceptWith1Arg() = default;
//	virtual void CopyInto(MetaConcept*& dest) const = 0;	// can throw memory failure
//	virtual void MoveInto(MetaConcept*& dest) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
//	virtual const AbstractClass* UltimateType() const = 0;
	size_t size() const override { return 1; }
	const T* ArgN(size_t n) const override { return (0 == n) ? Arg1 : 0; }
// Syntactical equality and inequality
	bool IsAbstractClassDomain() const override { return IsExplicitConstant(); }
//  Evaluation functions
	virtual bool CanEvaluate() const = 0;
	virtual bool CanEvaluateToSameType() const = 0;
	virtual bool SyntaxOK() const = 0;
	virtual bool Evaluate(MetaConcept*& dest) = 0;		// same, or different type
	virtual bool DestructiveEvaluateToSameType() = 0;	// overwrites itself iff returns true
	void ConvertVariableToCurrentQuantification(MetaQuantifier& src) override
	{
		if (!Arg1.empty()) Arg1->ConvertVariableToCurrentQuantification(src);
	}

	bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const override
	{
		return TargetRelation(Target, *Arg1) || Arg1->HasArgRelatedToThisConceptBy(Target, TargetRelation);
	}

	bool UsesQuantifierAux(const MetaQuantifier& x) const override;

	bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) override
	{
		try {
			if (TargetRelation(lhs, *Arg1)) RHSInducedActionOnArg(Arg1, rhs);
			return Arg1->ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(lhs, rhs, RHSInducedActionOnArg, TargetRelation);
		} catch (const bad_alloc&) {
			return false;
		};
	}

// NOTE: we may need this further down
// Formal manipulation functions
//	virtual bool SelfLogicalNOT(void);	// instantiate when above is true
//	virtual bool DetectAntiIdempotent(const MetaConcept& Arg2) const;
protected:
	bool InternalDataLTAux(const MetaConcept& rhs) const override
	{
		if (Arg1.empty()) return true;
		const MetaConceptWith1Arg& VR_rhs = static_cast<const MetaConceptWith1Arg&>(rhs);
		return !VR_rhs.Arg1.empty() && Arg1->InternalDataLT(*VR_rhs.Arg1);
	}

	bool EqualAux2(const MetaConcept& rhs) const override
	{
		const MetaConceptWith1Arg& VR_rhs = static_cast<const MetaConceptWith1Arg&>(rhs);
		if (Arg1.empty()) return VR_rhs.Arg1.empty();
		return !VR_rhs.Arg1.empty() && *Arg1 == *VR_rhs.Arg1;
	}

	void _forceStdForm() override { if (!Arg1.empty()) Arg1->ForceStdForm(); }
	bool _IsExplicitConstant() const override { return Arg1.empty() || Arg1->IsExplicitConstant(); }

};

#endif
