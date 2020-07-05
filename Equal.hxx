// Equal.hxx
// declaration of StdAddition, an n-ary commutative operation defined for a variety of sets
// StdAddition is assumed to endow the set it operates on with an abelian group structure

#ifndef EQUAL_DEF
#define EQUAL_DEF

#include "MetaCon2.hxx"

// LOW-LEVEL DEPENDENCY: this is a shifted image of the corresponding MetaConcept IDs
enum EqualRelationModes	{
						ALLEQUAL_EM = ALLEQUAL_MC-ALLEQUAL_MC,				// 0
						ALLDISTINCT_EM = ALLDISTINCT_MC-ALLEQUAL_MC,		// 1
						EQUALTOONEOF_EM = EQUALTOONEOF_MC-ALLEQUAL_MC,		// 2
						DISTINCTFROMALLOF_EM = DISTINCTFROMALLOF_MC-ALLEQUAL_MC,		// 3
						NOTALLDISTINCT_EM = NOTALLDISTINCT_MC-ALLEQUAL_MC,	// 4
						NOTALLEQUAL_EM = NOTALLEQUAL_MC-ALLEQUAL_MC,		// 5
						StrictBound_EM=NOTALLEQUAL_EM+1						// 6
						};

class EqualRelation;
namespace zaimoni {

template<>
struct is_polymorphic_final<EqualRelation> : public std::true_type{};

}

//! \todo implement AND-Factor support
//! \todo implement hypothesis augmentation
class EqualRelation final : public MetaConceptWithArgArray
{
private:
	enum EvalRuleIdx_ER	{
		NAryEQUALSpawn2AryEQUAL_ER = MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1,
		NAryEqRewriteZeroEq2ArySum_ER,
		ReduceIntervalForNOTALLEQUAL_ER,
		MaxEvalRuleIdx_ER = ReduceIntervalForNOTALLEQUAL_ER-MetaConceptWithArgArray::MaxEvalRuleIdx_ER
		};
	enum SelfEvalRuleIdx_SER{
		AddInvOutStdAddArg_SER = MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1,
		MultInvOutStdMultArg_SER,
		Ary2EqRewriteZeroEq2ArySum_SER, 
		MergeIntervals_SER,
		DismantleLinearIntervalToEndpoints_SER,
		DISTINCTFROMALLOFExtractALLDISTINCT_SER,
		ExtendLinearInterval_SER,
		MaxSelfEvalRuleIdx_SER = ExtendLinearInterval_SER-MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER
		};


	typedef bool (EqualRelation::*EvaluateToOtherRule)(MetaConcept*& dest);
	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];

	typedef bool (EqualRelation::*SelfEvaluateRule)();
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];

	typedef size_t (EqualRelation::*LengthAux)(void) const;
	typedef void (EqualRelation::*ConstructNameAux)(char* Name) const;
	typedef bool (EqualRelation::*InvokeEqualArgAux)() const;
	typedef bool (EqualRelation::*ImpliesAux)(const MetaConcept& rhs) const;
	typedef void (EqualRelation::*ModifiesAux)(MetaConcept*& rhs) const;
	typedef bool (EqualRelation::*FindDetailedRuleAux)(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);

	static LengthAux LengthOfSelfNameAux[StrictBound_EM];
	static ConstructNameAux SelfNameAux[StrictBound_EM];
	static InvokeEqualArgAux EqualArgAux[StrictBound_EM];
	static InvokeEqualArgAux UseConstantsAux[StrictBound_EM];
	static InvokeEqualArgAux UseStdAdditionAux[StrictBound_EM];
	static InvokeEqualArgAux UseStdMultiplicationAux[StrictBound_EM];
	static InvokeEqualArgAux UseDomainsAux[StrictBound_EM];
	static ImpliesAux StrictlyImpliesAux[StrictBound_EM];
	static FindDetailedRuleAux LogicalANDDetailedRuleAux[StrictBound_EM];
	static ImpliesAux CanStrictlyModifyAux[StrictBound_EM];
	static ModifiesAux StrictlyModifiesAux[StrictBound_EM];
public:
	EqualRelation(EqualRelationModes LinkageType) noexcept : MetaConceptWithArgArray((ExactType_MC)(LinkageType+ALLEQUAL_MC)) {};
	EqualRelation(MetaConcept**& NewArgList, EqualRelationModes LinkageType);
	EqualRelation(const EqualRelation& src) = default;
	EqualRelation(EqualRelation&& src) = default;
	EqualRelation& operator=(const EqualRelation & src) = default;
	EqualRelation& operator=(EqualRelation&& src) = default;
	virtual ~EqualRelation() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(EqualRelation*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(EqualRelation*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
//  Type ID functions
	const AbstractClass* UltimateType() const override;
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;

	virtual void SelfLogicalNOT();
	int _strictlyImplies(const MetaConcept& rhs) const override;
	virtual bool StrictlyImplies(const MetaConcept& rhs) const;
	virtual void StrictlyModifies(MetaConcept*& rhs) const;
	virtual bool CanStrictlyModify(const MetaConcept& rhs) const;
	virtual bool LogicalANDFindDetailedRule(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);
	virtual bool LogicalANDNonTrivialFindDetailedRule() const;
	bool LogicalANDOrthogonalClause() const override;
	// Basis clause support
	virtual size_t BasisClauseCount() const;
	virtual bool DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const;
// Assistants to QStatement
	void ImproviseDomainsALLEQUAL(bool& Target);
	bool ImproviseDomainsEQUALTOONEOF(bool& Target);
protected:
	virtual void ConstructSelfNameAux(char* Name) const;	// overwrites what is already there
	void _forceStdForm() override;

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
private:
	void _ForceArgSameImplementation(size_t n) override;

	size_t array_index() const {return ExactType()-ALLEQUAL_MC;};

	size_t LengthOfSelfNameALLEQUAL() const;
	size_t LengthOfSelfNameALLDISTINCT() const;
	size_t LengthOfSelfNameDISTINCTFROMALLOF() const;
	size_t LengthOfSelfNameEQUALTOONEOF() const;
	size_t LengthOfSelfNameNOTALLDISTINCT() const;
	size_t LengthOfSelfNameNOTALLEQUAL() const;

	void ConstructSelfNameAuxALLEQUAL(char* Name) const;
	void ConstructSelfNameAuxALLDISTINCT(char* Name) const;
	void ConstructSelfNameAuxDISTINCTFROMALLOF(char* Name) const;
	void ConstructSelfNameAuxEQUALTOONEOF(char* Name) const;
	void ConstructSelfNameAuxNOTALLDISTINCT(char* Name) const;
	void ConstructSelfNameAuxNOTALLEQUAL(char* Name) const;

	bool StrictlyImpliesALLEQUAL(const MetaConcept& rhs) const;
	bool StrictlyImpliesALLDISTINCT(const MetaConcept& rhs) const;
	bool StrictlyImpliesEQUALTOONEOF(const MetaConcept& rhs) const;
	bool StrictlyImpliesDISTINCTFROMALLOF(const MetaConcept& rhs) const;
	bool StrictlyImpliesNOTALLDISTINCT(const MetaConcept& rhs) const;
	bool StrictlyImpliesNOTALLEQUAL(const MetaConcept& rhs) const;

	void StrictlyModifiesALLEQUAL(MetaConcept*& rhs) const;
	void StrictlyModifiesALLDISTINCT(MetaConcept*& rhs) const;
	void StrictlyModifiesEQUALTOONEOF(MetaConcept*& rhs) const;
	void StrictlyModifiesDISTINCTFROMALLOF(MetaConcept*& rhs) const;
	void StrictlyModifiesNOTALLDISTINCT(MetaConcept*& rhs) const;
	void StrictlyModifiesNOTALLEQUAL(MetaConcept*& rhs) const;

	bool CanStrictlyModifyALLEQUAL(const MetaConcept& rhs) const;
	bool CanStrictlyModifyALLDISTINCT(const MetaConcept& rhs) const;
	bool CanStrictlyModifyEQUALTOONEOF(const MetaConcept& rhs) const;
	bool CanStrictlyModifyDISTINCTFROMALLOF(const MetaConcept& rhs) const;
	bool CanStrictlyModifyNOTALLDISTINCT(const MetaConcept& rhs) const;
	bool CanStrictlyModifyNOTALLEQUAL(const MetaConcept& rhs) const;

	bool LogicalANDFindDetailedRuleALLEQUAL(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);
	bool LogicalANDFindDetailedRuleBothDISTINCTFROMALLOFAux(EqualRelation& rhs, size_t LHSIdx, size_t RHSIdx, size_t& VRParam1, size_t& VRParam2);
	bool LogicalANDFindDetailedRuleDISTINCTFROMALLOF(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);

	bool InvokeEqualArgRuleALLEQUAL() const;
	bool InvokeEqualArgRuleALLDISTINCT() const;
	bool InvokeEqualArgRuleEQUALTOONEOF() const;
	bool InvokeEqualArgRuleDISTINCTFROMALLOF() const;
	bool InvokeEqualArgRuleNOTALLDISTINCT() const;
	bool InvokeEqualArgRuleNOTALLEQUAL() const;

	bool UseConstantsALLEQUAL() const;
	bool UseConstantsALLDISTINCT() const;
	bool UseConstantsEQUALTOONEOForDISTINCTFROMALLOF() const;
	bool UseConstantsNOTALLDISTINCT() const;
	bool UseConstantsNOTALLEQUAL() const;	

	bool UseStdAdditionALLEQUAL() const;
	bool UseStdAdditionALLDISTINCT() const;
	bool UseStdAdditionEQUALTOONEOForDISTINCTFROMALLOF() const;
	bool UseStdAdditionNOTALLDISTINCT() const;
	bool UseStdAdditionNOTALLEQUAL() const;	

	bool UseStdMultiplicationALLEQUAL() const;
	bool UseStdMultiplicationALLDISTINCT() const;
	bool UseStdMultiplicationEQUALTOONEOForDISTINCTFROMALLOF() const;
	bool UseStdMultiplicationNOTALLDISTINCT() const;
	bool UseStdMultiplicationNOTALLEQUAL() const;	

	bool UseDomainsALLEQUAL() const;
	bool UseDomainsALLDISTINCT() const;
	bool UseDomainsEQUALTOONEOForDISTINCTFROMALLOF() const;
	bool UseDomainsNOTALLDISTINCT() const;
	bool UseDomainsNOTALLEQUAL() const;	

	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;
	
	virtual bool DelegateEvaluate(MetaConcept*& dest);		// different type
	virtual bool DelegateSelfEvaluate();		// same type	

	// Following block defined in ConEqual.cxx [not a class file: for rules that interact between MetaConnective and EqualRelation]
	bool NAryEQUALSpawn2AryEQUAL(MetaConcept*& dest);
	bool NAryEqRewriteZeroEq2ArySum(MetaConcept*& dest);
	bool ReduceIntervalForNOTALLEQUAL(MetaConcept*& dest);	

	// resume Equal.cxx
	bool Ary2EqRewriteZeroEq2ArySum();
	bool MergeIntervals();
	bool DismantleLinearIntervalToEndpoints();
	bool ExtendLinearInterval();
	bool DISTINCTFROMALLOFExtractALLDISTINCT();
	// Following block defined in StdAdd.cxx
	bool AddInvOutStdAddArg();
	// Following block defined in StdMult.cxx
	bool MultInvOutStdMultArg();	
};

#endif
