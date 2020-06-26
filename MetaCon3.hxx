// MetaCon3.hxx
// header for MetaConnective, which subsumes the logical connectives
// AND, OR, NAND, NOR, IFF, and XOR

#ifndef METACONNECTIVE_DEF
#define METACONNECTIVE_DEF

#include "MetaCon2.hxx"

// LOW-LEVEL DEPENDENCY: this is a shifted image of the corresponding MetaConcept IDs
enum MetaConnectiveModes	{
							AND_MCM	 = LogicalAND_MC-LogicalAND_MC,	/* 0 */
							OR_MCM   = LogicalOR_MC-LogicalAND_MC,	/* 1 */
							IFF_MCM	 = LogicalIFF_MC-LogicalAND_MC,	/* 2 */
							XOR_MCM  = LogicalXOR_MC-LogicalAND_MC,	/* 3 */
							NXOR_MCM = LogicalNXOR_MC-LogicalAND_MC,	/* 4 */
							NIFF_MCM = LogicalNIFF_MC-LogicalAND_MC,	/* 5 */
							NOR_MCM  = LogicalNOR_MC-LogicalAND_MC,	/* 6 */
							NAND_MCM = LogicalNAND_MC-LogicalAND_MC,/* 7 */
							StrictBound_MCM=NAND_MCM+1				/* 8 */
							};

class MetaConnective;
namespace zaimoni {

template<>
struct is_polymorphic_final<MetaConnective> : public std::true_type {};

}

class Variable;

class MetaConnective final : public MetaConceptWithArgArray
{
private:
	enum EvalRuleIdx_ER	{
		LogicalANDAry2NArySpliceEqualArg_ER = MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1,
		LogicalANDAry2ALLEQUALSpliceAddInvArg_ER,
		MaxEvalRuleIdx_ER = LogicalANDAry2ALLEQUALSpliceAddInvArg_ER-MetaConceptWithArgArray::MaxEvalRuleIdx_ER
		};
	enum SelfEvalRuleIdx_SER{
		LogicalANDSpliceIFFAntiIdempotentArg_SER = MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1,
		LogicalANDAry2IFFSpliceAntiIdempotentArg_SER,
		toNOR_SER,
		Ary2IFFToOR_SER,
		Ary2IFFToORV2_SER,
		Ary2IFFToORV3_SER,
		Ary2IFFToORSelfModify_SER,
		IFFSpawn2AryIFF_SER,
		LogicalANDReplaceTheseNAry2ORWithNIFF_SER,
		RemoveIrrelevantArgFromOR_SER,		
		LogicalANDSpawnClauseForDetailedRule_SER,
		IFF_ReplaceANDORWithOwnArgs_SER,
		LogicalANDStrictlyModify_SER,
		LogicalANDStrictlyImpliesClean_SER,
		VirtualDeepStrictlyModify_SER,
		VirtualDeepLogicallyImplies_SER,
		LogicalORStrictlyImpliesClean_SER,
		ExtractTrueArgFromXOR_SER,
		LogicalANDReplaceThisArgWithTRUE_SER,
		LogicalXORExtractANDFactor_SER,
		LogicalXORExtractIFFFactor_SER,
		LogicalANDReplaceORAndXORWithXORAndNORArg_SER,
		ConvertToNANDOtherArgs_SER,
		ConvertToNOROtherArgs_SER,
		ReplaceArgsWithTrue_SER,
		TargetVariableFalse_SER,
		TargetVariableTrue_SER,
		LogicalANDAry2MetaConnectiveSpliceEqualArg_SER,
		LogicalANDReplaceORAndXORWithXORAndNOTArg_SER,
		AB_ToIFF_AnotB_SER,
		MaxSelfEvalRuleIdx_SER = AB_ToIFF_AnotB_SER-MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER
		};		
		
	typedef bool (MetaConnective::*EvaluateToOtherRule)(MetaConcept*& dest);
	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];

	typedef bool (MetaConnective::*SelfEvaluateRule)();
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];

	typedef void (MetaConnective::*DiagnoseIntermediateRulesFunc)(void) const;
	typedef	void (MetaConnective::*StrictModifyAuxFunc)(MetaConcept*& Arg2) const;
	typedef bool (MetaConnective::*DiagnoseIntermediateRulesFunc2)(void) const;
	typedef bool (MetaConnective::*LogicalANDFindDetailedRuleAux)(MetaConcept& RHS, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);
	typedef void (MetaConnective::*UseThisAsMakeImplyAux)(const MetaConcept& Target);
	typedef	bool (MetaConnective::*BinaryRelationAuxFunc)(const MetaConnective& Arg2) const;
	typedef	bool (MetaConnective::*BinaryRelationAuxFunc2)(const MetaConcept& Arg2) const;

	static const BinaryRelationAuxFunc2 StrictlyImpliesAux[StrictBound_MCM];
	static const BinaryRelationAuxFunc2 CanStrictlyModifyAux[StrictBound_MCM];
	static const LogicalANDFindDetailedRuleAux ANDDetailedRuleAux[NIFF_MCM-AND_MCM+1];
	static const StrictModifyAuxFunc StrictlyModifiesAux[NIFF_MCM-AND_MCM+1];
	static const UseThisAsMakeImplyAux UseThisAsMakeImply2AryTable[IFF_MCM+1];
	static const UseThisAsMakeImplyAux UseThisAsMakeImplyNAryTable[NXOR_MCM+1];
public:
	MetaConnective(MetaConnectiveModes LinkageType) noexcept : MetaConceptWithArgArray((ExactType_MC)(LinkageType+LogicalAND_MC)) {};
	MetaConnective(MetaConcept**& NewArgList, MetaConnectiveModes LinkageType);
	MetaConnective(const MetaConnective& src) = default;
	MetaConnective(MetaConnective&& src) = default;
	MetaConnective& operator=(const MetaConnective & src) = default;
	MetaConnective& operator=(MetaConnective&& src) = default;
	virtual ~MetaConnective() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(MetaConnective*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(MetaConnective*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	const AbstractClass* UltimateType() const override;
//  Evaluation functions
	evalspec canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName(void) const;
// Formal manipulation functions
	virtual bool LogicalANDFindDetailedRule(MetaConcept& RHS, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);
	virtual bool LogicalANDNonTrivialFindDetailedRule() const;
	bool LogicalANDOrthogonalClause() const override;
	// Logical Amplification support
	bool WantToBeAmplified() const override;
	bool CanAmplifyClause() const override;
	bool CanAmplifyThisClause(const MetaConcept& rhs) const override;
	bool AmplifyThisClause(MetaConcept*& rhs) const override;
	// Basis clause support
	virtual size_t BasisClauseCount() const;
	virtual bool DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const;
	// "AND-factor" support
	virtual size_t ANDFactorCount() const;
	virtual bool DirectCreateANDFactorIdx(size_t Idx, MetaConcept*& dest) const;
	// Interaction clause support
	ExactType_MC CanMakeLHSImplyRHS() const override;
	virtual bool MakesLHSImplyRHS(const MetaConcept& lhs, const MetaConcept& rhs) const;
	virtual bool ValidLHSForMakesLHSImplyRHS(const MetaConcept& lhs) const;
	virtual bool ValidRHSForMakesLHSImplyRHS(const MetaConcept& rhs) const;
	evalspec _CanUseThisAsMakeImply(const MetaConcept& Target) override;
	virtual bool CanUseThisAsMakeImply(const MetaConcept& Target) const;
	virtual void UseThisAsMakeImply(const MetaConcept& Target);
	// Logical operation support
	virtual void SelfLogicalNOT();
	virtual bool StrictlyImplies(const MetaConcept& RHS) const;
	// QState.cxx support: Hypothesis augmentation
	virtual bool CouldAugmentHypothesis(void) const;
	virtual bool CanAugmentHypothesis(const MetaConcept& Hypothesis) const;
	virtual bool AugmentHypothesis(MetaConcept*& Hypothesis) const;
	virtual void StrictlyModifies(MetaConcept*& RHS) const;
	virtual bool CanStrictlyModify(const MetaConcept& RHS) const;
	bool VertexDiagnoseIntermediateRulesANDAux(void) const;
	virtual bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;
	bool LogicalANDCanUseSpeculativeTarget(MetaConcept*& SpeculativeTarget, size_t NonStrictLB, size_t StrictUB) const;
	//	type-specific functions
	bool HyperNonStrictlyImpliesReplacement(const MetaConcept& LHS, const MetaConcept& RHS);
	bool HyperNonStrictlyImpliesLogicalNOTOfReplacement(const MetaConcept& LHS, const MetaConcept& RHS);
	// next members are implemented in QState.cxx
	bool LogicalANDDoNotExplore(void) const;
	void LogicalANDCleanOrthogonalClauses(const clock_t EvalTime0);
	void LogicalANDCleanRenamedVariables(const clock_t EvalTime0);
	// this, if it doesn't crash, clears out TVarList
	void LogicalANDCreateVarListV2(MetaQuantifier**& TVarList, MetaConcept**& VarList, MetaConcept**& PlausibleVarList);
	void LogicalANDImproviseDomains(void);
	void LogicalANDCleanConsistentVars(size_t& Idx, size_t& NonRedundantStrictUB, MetaConcept**& VarList);
	void ScreenVarList_IFFClean(void);
	bool ImprovisedUsesSpeculativeOR(const MetaConnective& Target) const;
	bool ANDValidConclusionForSymmetry() const;
	bool WantStateDump() const;	// for LogicalAND only; inference engine helper

	template<ExactType_MC dest> void set() {
		static_assert(LogicalAND_MC <= dest && LogicalNAND_MC >= dest);
		SetExactType(dest);
		if constexpr (LogicalNOR_MC <= dest) DoSelfDeMorgan();
		ForceCheckForEvaluation();
	}

	// thin forwarders to support lambda functions
	bool TargetVariableFalse(size_t i) {
		InferenceParameter1 = i;
		return TargetVariableFalse();
	}
	bool TargetVariableTrue(size_t i) {
		InferenceParameter1 = i;
		return TargetVariableTrue();
	}

protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	void _forceStdForm() override;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;

private:
	void _ForceArgSameImplementation(size_t n) override;

	void DoSelfDeMorgan();
	void SetNANDNOR(ExactType_MC NewType);

	bool LogicalANDFindDetailedRuleForOR(MetaConcept& RHS, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);
	bool LogicalANDFindDetailedRuleForIFF(MetaConcept& RHS, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx);

	bool Prefilter_CanAmplifyThisClause(const MetaConcept& rhs) const;
	bool OR_CanAmplifyThisClause(const MetaConnective& rhs) const;
	bool XOR_CanAmplifyThisClause(const MetaConnective& rhs) const;

	bool OR_AmplifyThisClause(MetaConnective& rhs) const;
	bool IFF_AmplifyThisClauseV1(MetaConnective& rhs) const;
	bool IFF_AmplifyThisClauseV2(MetaConnective& rhs) const;
	bool XOR_AmplifyThisClause(MetaConnective& rhs) const;

	evalspec _CanUseThisAsMakeImplyAND(const MetaConcept& Target);
	evalspec _CanUseThisAsMakeImplyOR(const MetaConcept& Target);
	evalspec _CanUseThisAsMakeImplyIFF(const MetaConcept& Target);
	evalspec _CanUseThisAsMakeImplyXOR(const MetaConcept& Target);
	evalspec _CanUseThisAsMakeImplyNXOR(const MetaConcept& Target);

	void UseThisAsMakeImply2AryAND(const MetaConcept& Target);
	void UseThisAsMakeImply2AryOR(const MetaConcept& Target);
	void UseThisAsMakeImply2AryIFF(const MetaConcept& Target);
	void UseThisAsMakeImplyNAryAND(const MetaConcept& Target);
	void UseThisAsMakeImplyNAryOR(const MetaConcept& Target);
	void UseThisAsMakeImplyNAryIFF(const MetaConcept& Target);
	void UseThisAsMakeImplyNAryXOR(const MetaConcept& Target);
	void UseThisAsMakeImplyNAryNXOR(const MetaConcept& Target);

	void SelfLogicalNOT_Normal();
	void SelfLogicalNOT_IFF();
	void SelfLogicalNOT_XOR();
	void SelfLogicalNOT_ANDOR();

	bool StrictlyImplies_AND(const MetaConcept& rhs) const;
	bool StrictlyImplies_OR(const MetaConcept& rhs) const;
	bool StrictlyImplies_IFF(const MetaConcept& rhs) const;
	bool StrictlyImplies_XOR(const MetaConcept& rhs) const;
	bool StrictlyImplies_NXOR(const MetaConcept& rhs) const;
	bool StrictlyImplies_NIFF(const MetaConcept& rhs) const;
	bool NORNANDFatal(const MetaConcept& rhs) const;

	void StrictlyModifies_AND(MetaConcept*& RHS) const;
	void StrictlyModifies_OR(MetaConcept*& RHS) const;
	void StrictlyModifies_IFF(MetaConcept*& RHS) const;
	bool StrictlyModifies_IFFAux(MetaConcept*& RHS,LowLevelBinaryRelation* TargetRelation) const;
	void StrictlyModifies_XOR(MetaConcept*& RHS) const;
	void StrictlyModifies_NXOR(MetaConcept*& RHS) const;
	void StrictlyModifies_NIFF(MetaConcept*& RHS) const;

	bool CanStrictlyModify_AND(const MetaConcept& RHS) const;
	bool CanStrictlyModify_OR(const MetaConcept& RHS) const;
	bool CanStrictlyModify_IFF(const MetaConcept& RHS) const;
	bool CanStrictlyModify_XOR(const MetaConcept& RHS) const;
	bool CanStrictlyModify_NXOR(const MetaConcept& RHS) const;
	bool CanStrictlyModify_NIFF(const MetaConcept& RHS) const;

	void DiagnoseInferenceRulesContradiction2Ary() const;
	void DiagnoseInferenceRulesTrue2Ary() const;
	void DiagnoseInferenceRulesFalse2Ary() const;
	void DiagnoseInferenceRulesUnknown2Ary() const;
	bool DiagnoseInferenceRulesContradictionNAry() const;
	bool DiagnoseInferenceRulesTrueNAry() const;
	bool DiagnoseInferenceRulesFalseNAry() const;
	bool DiagnoseInferenceRulesUnknownNAry() const;
	bool WouldDiagnoseInferenceRulesContradictionNAry() const;
	bool WouldDiagnoseInferenceRulesTrueNAry() const;
	bool WouldDiagnoseInferenceRulesFalseNAry() const;
	bool WouldDiagnoseInferenceRulesUnknownNAry() const;

	bool DiagnoseStrictlyImplies2AryAND() const;
	bool DiagnoseStrictlyImplies2AryOR() const;
	bool DiagnoseStrictlyImplies2AryIFF() const;
	bool DiagnoseStrictlyImpliesNAryAND() const;
	bool DiagnoseStrictlyImpliesNAryOR() const;
	bool DiagnoseStrictlyImpliesNAryIFF() const;
	bool DiagnoseStrictlyImpliesNAryXOR() const;
	bool DiagnoseStrictlyImpliesNAryNXOR() const;
	void DiagnoseIntermediateRulesANDAux(void) const;

	bool DiagnoseCommonIntermediateRulesORXORAux(void) const;
	void DiagnoseIntermediateRulesORAux(void) const;
	void DiagnoseIntermediateRulesIFFAux(void) const;
	void DiagnoseIntermediateRulesXORAux() const;
	void DiagnoseIntermediateRulesNXORAux(void) const;
	void DiagnoseIntermediateRulesNIFFAux(void) const;
	void DiagnoseIntermediateRulesAND2AryAux(void) const;
	void DiagnoseIntermediateRulesOR2AryAux(void) const;
	void DiagnoseIntermediateRulesIFF2AryAux(void) const;

	size_t array_index() const {return ExactType()-LogicalAND_MC;};
	void AddBasisClausesToVarList(MetaConcept**& VarList) const;
	void LogicalIFFRemoveRedundantVariables(bool& HaveResponded, bool& LastRemoveRewriteVar);
	// resume implementation in MetaCon3.cxx
	bool LogicalANDCreateSpeculativeORAux(MetaConnective*& SpeculativeOR, size_t MaxArityWanted, size_t HighXORIdx, size_t LowXORIdx, size_t HighORIdx, size_t LowORIdx) const;
	bool LogicalANDCreateSpeculativeOR(void) const;
	bool ExploreImprovisedUsesSpeculativeORAux(MetaConnective*& SpeculativeTarget, const MetaConnective& Target, size_t MaxArityWanted, size_t HighXORIdx, size_t LowXORIdx, size_t HighORIdx, size_t LowORIdx) const;
	bool LogicalANDBoostORWithIFF(MetaConnective*& SpeculativeTarget, size_t LowXORIdx, size_t HighORIdx) const;
	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;
	bool LogicalANDTargetArgCritical(size_t TestIdx) const;

	virtual bool DelegateEvaluate(MetaConcept*& dest);		// different type
	virtual bool DelegateSelfEvaluate();		// same type

	bool LogicalANDAry2NArySpliceEqualArg(MetaConcept*& dest);	
	bool LogicalANDAry2ALLEQUALSpliceAddInvArg(MetaConcept*& dest);

	bool LogicalANDSpliceIFFAntiIdempotentArg();
	bool LogicalANDAry2IFFSpliceAntiIdempotentArg();
	bool toNOR();
	bool Ary2IFFToOR();
	bool Ary2IFFToORV2();
	bool Ary2IFFToORV3();
	bool Ary2IFFToORSelfModify();
	bool IFFSpawn2AryIFF();
	bool LogicalANDReplaceTheseNAry2ORWithNIFF();
	bool RemoveIrrelevantArgFromOR();
	bool LogicalANDSpawnClauseForDetailedRule();
	bool IFF_ReplaceANDORWithOwnArgs();
	bool LogicalANDStrictlyModify();
	bool LogicalANDStrictlyImpliesClean();
	bool VirtualDeepStrictlyModify();
	bool VirtualDeepLogicallyImplies();
	bool LogicalORStrictlyImpliesClean();
	bool ExtractTrueArgFromXOR();
	bool LogicalANDReplaceThisArgWithTRUE();
	bool LogicalXORExtractANDFactor();
	bool LogicalXORExtractIFFFactor();
	bool LogicalANDReplaceORAndXORWithXORAndNORArg();
	bool ConvertToNANDOtherArgs();
	bool ConvertToNOROtherArgs();
	bool ReplaceArgsWithTrue();
	bool TargetVariableFalse();
	bool TargetVariableTrue();
	bool LogicalANDAry2MetaConnectiveSpliceEqualArg();
	bool LogicalANDReplaceORAndXORWithXORAndNOTArg();
	bool AB_ToIFF_AnotB();	
};
#endif
