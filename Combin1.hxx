// Combin1.hxx
// Header for CombinatorialLike class, which implements
// * Factorial(K): factorial function.  UltimateType 1...infinity
// * Gamma(K): Gamma function,  UltimateType R\{-infinity...0}.  May do this one later.
// * Perm(N,K): counts permutations.  UltimateType 1...infinity
// * Comb(N,K): counts combinations.  UltimateType 1...infinity
// * Multi(N,K,J,...): multinomial combinations.  UltimateType 1...infinity

#ifndef COMBINATORIAL_DEF
#define COMBINATORIAL_DEF

#include "MetaCon2.hxx"

class CombinatorialLike;
namespace zaimoni {

template<>
struct is_polymorphic_final<CombinatorialLike> : public std::true_type {};

}

enum CombinatorialModes	{
						FACTORIAL_CM		= Factorial_MC-Factorial_MC,	/* 0 */
//						GAMMAFUNCTION_CM	= GammaFunction_MC-Factorial_MC,	/* 1 */
						PERMUTATIONCOUNT_CM	= PermutationCount_MC-Factorial_MC,	/* 2 */
						COMBINATIONCOUNT_CM	= CombinationCount_MC-Factorial_MC,	/* 3 */
//						MULTINOMIALCOUNT_CM	= MultinomialCount_MC-Factorial_MC,	/* 4 */
						StrictBound_CM		= PERMUTATIONCOUNT_CM+1				/* 5 */
						};

class CombinatorialLike final : public MetaConceptWithArgArray
{
private:
	enum EvalRuleIdx_ER	{
		FactorialPartialEvaluate_ER = MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1,
		PermutationCountPartialEvaluate_ER,
		CombinationCountPartialEvaluate_ER,
		MaxEvalRuleIdx_ER = CombinationCountPartialEvaluate_ER-MetaConceptWithArgArray::MaxEvalRuleIdx_ER
		};
	enum SelfEvalRuleIdx_SER{
		RetypePermutationCountAsFactorial_SER = MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1,
		MaxSelfEvalRuleIdx_SER = RetypePermutationCountAsFactorial_SER-MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER
		};

	typedef bool (CombinatorialLike::*EvaluateToOtherRule)(MetaConcept*& dest);
	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];

	typedef bool (CombinatorialLike::*SelfEvaluateRule)();
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];
public:
	CombinatorialLike(CombinatorialModes LinkageType) : MetaConceptWithArgArray((ExactType_MC)(LinkageType+Factorial_MC)) {};
	CombinatorialLike(MetaConcept**& NewArgList, CombinatorialModes LinkageType);
//	CombinatorialLike(const CombinatorialLike& src);	// default OK
	virtual ~CombinatorialLike() = default;

//	const CombinatorialLike& operator=(const CombinatorialLike& src) // default OK
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(CombinatorialLike*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(CombinatorialLike*& dest);		// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual bool IsPositive() const;
	virtual bool IsNegative() const;
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	void _forceStdForm() override;
	virtual bool _IsOne() const;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
private:
	virtual bool DelegateEvaluate(MetaConcept*& dest);		// different type
	virtual bool DelegateSelfEvaluate();		// same type

	bool RetypePermutationCountAsFactorial();
	bool FactorialPartialEvaluate(MetaConcept*& dest);
	bool PermutationCountPartialEvaluate(MetaConcept*& dest);
	bool CombinationCountPartialEvaluate(MetaConcept*& dest);
};

#endif
