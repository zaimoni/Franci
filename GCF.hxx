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
	GCF() : MetaConceptWithArgArray(GCF_MC) {};
	GCF(MetaConcept**& NewArgList);
//	GCF(const GCF& src);	// default OK
	virtual ~GCF() = default;

//	const GCF& operator=(const GCF& src);	// default ok
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(GCF*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(GCF*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;

//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	void _forceStdForm() override;

	virtual bool _IsOne() const;
	virtual bool _IsZero() const;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
};

#endif
