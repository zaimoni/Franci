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
#include "Zaimoni.STL/LexParse/Kuroda.hpp"

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
	CombinatorialLike(CombinatorialModes LinkageType) noexcept : MetaConceptWithArgArray((ExactType_MC)(LinkageType+Factorial_MC)) {};
	CombinatorialLike(MetaConcept**& NewArgList, CombinatorialModes LinkageType);
	CombinatorialLike(const CombinatorialLike& src) = default;
	CombinatorialLike(CombinatorialLike&& src) = default;
	CombinatorialLike& operator=(const CombinatorialLike & src) = default;
	CombinatorialLike& operator=(CombinatorialLike&& src) = default;
	virtual ~CombinatorialLike() = default;

	static std::vector<size_t> parse(kuroda::parser<MetaConcept>::sequence& symbols, size_t n);
	static ExactType_MC prefix_keyword(const MetaConcept* x);

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(CombinatorialLike*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move( *this), dest); }
	void MoveInto(CombinatorialLike*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual bool IsPositive() const;
	virtual bool IsNegative() const;

protected:
	std::string to_s_aux() const override;
	void _forceStdForm() override;
	virtual bool _IsOne() const;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;

private:
	void _ForceArgSameImplementation(size_t n) override;

	virtual bool DelegateEvaluate(MetaConcept*& dest);		// different type
	virtual bool DelegateSelfEvaluate();		// same type

	bool RetypePermutationCountAsFactorial();
	bool FactorialPartialEvaluate(MetaConcept*& dest);
	bool PermutationCountPartialEvaluate(MetaConcept*& dest);
	bool CombinationCountPartialEvaluate(MetaConcept*& dest);
};

#endif
