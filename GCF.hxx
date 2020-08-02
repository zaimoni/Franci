// GCF.hxx
// header for the class GCF, which handles Greatest Common Factor computations.
// Direct representation of GCF as a function would require a different class.

#ifndef GCF_DEF
#define GCF_DEF

#include "MetaCon2.hxx"

class GCF;
namespace zaimoni {

template<>
struct is_polymorphic_final<GCF> : public std::true_type {};

}

class GCF final : public MetaConceptWithArgArray
{
public:
	GCF() noexcept : MetaConceptWithArgArray(GCF_MC) {};
	GCF(MetaConcept**& NewArgList);
	GCF(const GCF& src) = default;
	GCF(GCF&& src) = default;
	GCF& operator=(const GCF & src) = default;
	GCF& operator=(GCF&& src) = default;
	virtual ~GCF() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(GCF*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(GCF*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;

//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
	bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;

protected:
	void _ForceArgSameImplementation(size_t n) override;

	std::string to_s_aux() const override { return ConstructPrefixArgList(); }
	void _forceStdForm() override;

	virtual bool _IsOne() const;
	virtual bool _IsZero() const;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
};

#endif
