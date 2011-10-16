// GCF.hxx
// header for the class GCF, which handles Greatest Common Factor computations.
// Direct representation of GCF as a function would require a different class.

#ifndef GCF_DEF
#define GCF_DEF

#include "MetaCon2.hxx"

class GCF;
namespace zaimoni {

template<>
struct is_polymorphic_final<GCF> : public boost::true_type {};

}

class GCF : public MetaConceptWithArgArray
{
public:
	GCF() : MetaConceptWithArgArray(GCF_MC) {};
	GCF(MetaConcept**& NewArgList);
//	GCF(const GCF& src);	// default OK
	virtual ~GCF();

//	const GCF& operator=(const GCF& src);	// default ok
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(GCF*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(GCF*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;

//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	virtual void _forceStdForm();

	virtual bool _IsOne() const;
	virtual bool _IsZero() const;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
};

#endif
