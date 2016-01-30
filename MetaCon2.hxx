// MetaCon2.hxx
// declaration of MetaConceptWithArgArray

#ifndef METACONCEPTARGARRAY_DEF
#define METACONCEPTARGARRAY_DEF

#include "MetaCon1.hxx"
#include "Zaimoni.STL/AutoPtr.hpp"

// ASSUMPTION: the memory manager does not overallocate by more than sizeof(MetaConcept*)-1.
// failing this breaks void StrictBoundIdxArray(void) const;

class MetaQuantifier;

// ArgArray validation
bool ValidateArgArray(const MetaConcept* const * const ArgArray);

// ArgArray** manipulation functions
bool FindExplicitConstantArg(const MetaConcept* const * const ArgArray, size_t i, size_t& Param1);
// NOTE: if successful, found argIdx is passed back in Param1

void TotalSortByLexicalOrder(MetaConcept** const ArgArray, size_t StrictMaxIdx);

// The GrepArgList functions should have their output array pointers NULL when invoked.
bool GrepArgList(MetaConcept**& MirrorArgList,LowLevelUnaryProperty& WantThis, MetaConcept** const ArgArray);
bool GrepArgList(size_t*& IndexMap,LowLevelUnaryProperty& WantThis, MetaConcept** const ArgArray);
bool DualGrepArgList(size_t*& IndexMap, size_t*& InverseIndexMap, LowLevelUnaryProperty& WantThis, MetaConcept** const ArgArray);
void SelfGrepArgListNoOwnership(LowLevelBinaryRelation& WantThis, const MetaConcept& rhs, MetaConcept**& ArgArray);
bool MirrorArgArrayNoOwnership(size_t LocalUB,MetaConcept** LocalArgArray,MetaConcept**& MirrorArgArray,size_t MinArgCount,LowLevelBinaryRelation& TargetRelation);

bool FindArgRelatedToLHS(const MetaConcept& lhs, LowLevelBinaryRelation& TargetRelation, const MetaConcept* const * const ArgArray, size_t& Result);

class MetaConceptWithArgArray: public MetaConcept
{
public:
	enum EvalRuleIdx_ER	{
		None_ER = 0,
		EvalForceArg_ER,
		EvalForceNotArg_ER,
		EvalForceTrue_ER,			// MetaConnective wants these four
		EvalForceFalse_ER,
		EvalForceContradiction_ER,
		EvalForceUnknown_ER,
		ForceValue_ER,
		RetypeToMetaConnective_ER,
		MaxEvalRuleIdx_ER = RetypeToMetaConnective_ER
		};
	enum SelfEvalRuleIdx_SER{
		// In general, 'negative' values are used as mnemonics
		SelfEvalSyntaxBadNoRules_SER = -2,	// flag: this is stable
		SelfEvalSyntaxOKNoRules_SER,	// flag: this is stable
		None_SER,
		SelfEvalRuleEvaluateArgs_SER,
		SelfEvalRuleEvaluateArg_SER,
		SelfEvalRuleEvaluateArgOneStage_SER,
		SelfEvalStrictlyModify_SER,
		VirtualStrictlyModify_SER,
		SelfEvalRuleForceArgSameImplementation_SER,
		SelfEvalAddArgAtEndAndForceCorrectForm__SER,
		// NOTE: above is highest index allowed to pass through 2-ary AND unfiltered
		SelfEvalRuleCleanArg_SER,
		SelfEvalRuleCleanTrailingArg_SER,
		SelfEvalRuleCleanLeadingArg_SER,
		SelfEvalRuleNIFFXORCleanArg_SER,
		SelfEvalRuleNIFFXORCleanTrailingArg_SER,
		SelfEvalRuleAry2CorrectedCleanArg_SER,
		SelfEvalRuleAry2CorrectedCleanTrailingArg_SER,
		SelfEvalRuleUnrollGeneralizedAssociativity_SER,
		CompatibleRetypeOtherArgs_SER,
		CompatibleRetype_SER,
		LogicalANDSpliceNAryEqualArg_SER,
		LogicalANDSpliceALLEQUALAddInvArg_SER,
		ReplaceThisArgWithLeadArg_SER,
		LogicalORXORCompactANDArgHyperCube_SER,
		LogicalANDORXORCondenseORANDANDArgHyperCube_SER,
		TranslateInterval_SER,
		MaxSelfEvalRuleIdx_SER = TranslateInterval_SER
		};
protected:
	typedef bool (MetaConceptWithArgArray::*EvaluateToOtherRule)(MetaConcept*& dest);
	typedef bool (MetaConceptWithArgArray::*SelfEvaluateRule)();
	typedef void (MetaConceptWithArgArray::*VertexPairSelfProcess)(size_t Idx, size_t Idx2);

	autovalarray_ptr_throws<MetaConcept*> ArgArray;
	mutable size_t InferenceParameter1;
	mutable size_t InferenceParameter2;
	mutable autoval_ptr<MetaConcept> InferenceParameterMC;
	mutable unsigned short IdxCurrentEvalRule;
	mutable signed short IdxCurrentSelfEvalRule;
	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];
	static SelfEvaluateRule SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER];

	MetaConceptWithArgArray() : IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWithArgArray(ExactType_MC NewType) : MetaConcept(NewType),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWithArgArray(ExactType_MC NewType,unsigned char NewBitmap) : MetaConcept(NewType,NewBitmap),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWithArgArray(ExactType_MC NewType,MetaConcept**& NewArgArray) : MetaConcept(NewType),ArgArray(NewArgArray),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	explicit MetaConceptWithArgArray(ExactType_MC NewType,unsigned char NewBitmap,MetaConcept**& NewArgArray) : MetaConcept(NewType,NewBitmap),ArgArray(NewArgArray),IdxCurrentEvalRule(None_ER),IdxCurrentSelfEvalRule(None_SER) {};
	MetaConceptWithArgArray(const MetaConceptWithArgArray& src);
	void operator=(const MetaConceptWithArgArray& src);
public:
	virtual ~MetaConceptWithArgArray();
	virtual void CopyInto(MetaConcept*& Target) const = 0;	// can throw memory failure
	virtual void MoveInto(MetaConcept*& Target) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.
	void ForceCheckForEvaluation() const {IdxCurrentSelfEvalRule=None_SER;};
	bool SelfEvalCleanEnd() const {IdxCurrentSelfEvalRule = None_SER; return true;};
//  Type ID functions
	virtual const AbstractClass* UltimateType() const = 0;
//	Arity functions
	virtual size_t size() const {return ArgArray.size();};
	size_t fast_size() const {assert(!ArgArray.empty()); return ArgArray.ArraySize();};
	virtual const MetaConcept* ArgN(size_t n) const;
	virtual MetaConcept* ArgN(size_t n);
// Syntactical equality and inequality
	virtual bool IsAbstractClassDomain() const;
//  Evaluation functions
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const = 0;
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true
	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src);
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const;
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const;
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation);
// type-specific functions
	// QuantifiedStatement, MetaConnective are having access problems with these.
	inline size_t ImageInferenceParameter1() const {return InferenceParameter1;};
	inline size_t ImageInferenceParameter2() const {return InferenceParameter2;};
	// CombinatorialLike is having access problems
	inline void ReplaceArgArray(MetaConcept** src) {assert(src); ArgArray=src;};
	inline void ReplaceArgArray(_meta_autoarray_ptr<MetaConcept*>& src) {assert(!src.empty()); src.MoveInto(ArgArray);};
	inline void OverwriteAndNULL(MetaConcept**& dest) {dest = ArgArray.release();};
	
	bool ThisConceptUsesNon1stArg(const MetaConcept& Target) const;
	bool ReplaceNon1stArgWith1stArgInThisConcept(MetaConcept*& dest) const;
	// resume implementation in MetaCon2.cxx
	inline void FastDeleteIdx(size_t i) {ArgArray.FastDeleteIdx(i);};
	inline void DeleteIdx(size_t i) {ArgArray.DeleteIdx(i);};
	void DeleteNSlotsAt(size_t n, size_t i);
	void insertNSlotsAt(size_t n, size_t i);
	bool InsertNSlotsAtV2(size_t n, size_t i);
	bool InsertSlotAt(size_t i,MetaConcept* __default)
	{
		assert(size()>=i);
		return ArgArray.InsertSlotAt(i,__default);
	}

	void TransferOutAndNULL(size_t i, MetaConcept*& dest);		// NOTE: dest is simply overwritten
	void CleanTransferOutAndNULL(size_t i, MetaConcept*& dest);
	void TransferInAndOverwriteRaw(size_t i, MetaConcept* src);
	template <class T> void TransferInAndOverwrite(size_t i, _meta_auto_ptr<T>& src)
		{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
		assert(size()>i);
		src.TransferOutAndNULL(ArgArray[i]);
		}
	// this one copies all args, except the one pointed to by InferenceParameter1.
	void CopyAllArgsButOneIntoArgList(MetaConcept** const dest); // can throw bad_alloc
	bool AddArgAtEndAndForceCorrectForm(autoval_ptr<MetaConcept>& src);
 	template<class T> bool AddArgAtEndAndForceCorrectForm(T*& src)
 	{
 		assert(src);
 		bool ret = _AddArgAtEndAndForceCorrectForm(src);
 		src = 0;
 		return ret;
 	}
	bool AddArgAtStartAndForceCorrectForm(MetaConcept*& src);
	// Next four routines need symmetric, transitivity
	bool OrderIndependentPairwiseRelation(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const;
	bool ArgsPairwiseUnrelated(LowLevelBinaryRelation& TargetRelation) const;
	bool ArgRangePairwiseUnrelated(LowLevelBinaryRelation& TargetRelation, size_t lb, size_t nonstrict_ub) const;
	bool ExactOrderPairwiseRelation(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const;
	// This one assumes the args all have ultimate type TruthValue
	bool OneEqualOneIdempotentFirstTwoArgs(const MetaConceptWithArgArray& rhs) const;
	// next two: args found are pointed to by InferenceParameter1, InferenceParameter2
	// These return the *lowest* matching pair.
	// InferenceParameter1>InferenceParameter2: asymmetric relations should also call dual
	bool FindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation) const;
	bool FindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation, size_t StartIdx) const;
	// InferenceParameter1<InferenceParameter2: asymmetric relations should also call normal
	bool DualFindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation) const;
	bool DualFindTwoRelatedArgs(LowLevelBinaryRelation& TargetRelation, size_t StartIdx) const;
	// next: args found are pointed to by InferenceParameter1
	bool FindTwoRelatedArgs(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const;
	bool FindTwoRelatedArgs(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation, size_t IgnoreLHSArg, size_t IgnoreRHSArg) const;
	bool FindTwoEqualArgs() const;
	bool FindTwoEqualArgsLHSRHSLexicalOrderedArgs(const MetaConceptWithArgArray& rhs) const;
	bool FindTwoAntiIdempotentArgsSymmetric() const;
	// LHS and RHS of TargetRelation, respectively
	// Arg is pointed to by InferenceParameter1
	bool FindArgRelatedToLHS(const MetaConcept& lhs, LowLevelBinaryRelation& TargetRelation) const;
	bool FindArgRelatedToLHSViewPoint(size_t ViewPoint, LowLevelBinaryRelation& TargetRelation) const;
	bool FindArgRelatedToRHSViewPoint(size_t ViewPoint, LowLevelBinaryRelation& TargetRelation) const;
	bool FindArgRelatedToLHS(const MetaConcept& lhs, LowLevelBinaryRelation& TargetRelation, const size_t NonStrictLB, const size_t NonStrictUB) const;
	bool FindArgRelatedToRHS(const MetaConcept& rhs, LowLevelBinaryRelation& TargetRelation) const;
	bool FindArgRelatedToRHS(const MetaConcept& rhs, LowLevelBinaryRelation& TargetRelation, const size_t NonStrictLB, const size_t NonStrictUB) const;
	// both of these set InferenceParameter1 to the highest index for a match, if successful
	bool FindArgOfExactType(ExactType_MC Target) const;
	bool FindArgTypeMatch(ExactType_MC Target, const AbstractClass* UltimateTarget) const;
	bool ArgListHasInjectionIntoRHSArgListRelatedThisWay(const MetaConceptWithArgArray& rhs, LowLevelBinaryRelation& TargetRelation) const;
	// Unary property detectors.  May have to create metacode eventually.
	bool VerifyArgsExplicitConstant(size_t lb, size_t strict_ub) const;
	bool FindArgWithUnaryProperty(LowLevelUnaryProperty& Property) const;
	bool FindArgWithUnaryProperty(LowLevelUnaryProperty& Property, size_t StrictUB) const;
	// next: if found, arg is pointed to by InferenceParameter1.  This one searches top-down.
	bool FindExplicitConstantArgInArgArray() const {return FindExplicitConstantArg(ArgArray,fast_size(),InferenceParameter1);};
	bool FindLRMultInvCompetentExplicitConstArgInArgArray() const;

	bool BlockableAnnihilatorScan(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule, LowLevelBinaryRelation& TargetRelation) const;
	bool BlockableAnnihilatorScan(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule, LowLevelSideEffectBinaryRelation& TargetRelation) const;

	// Next is implemented in StdAdd.cxx
	bool AllNonConstArgsEqualUpToAddInv(const MetaConceptWithArgArray& RHS) const;
	// Resume implementing in MetaCon2.cxx
	// this is because of Borland access problems
	inline void SwapArgs(size_t Idx1, size_t Idx2) const {swap(ArgArray[Idx1],ArgArray[Idx2]);};
	bool EvalForceArg(MetaConcept*& dest);
	void InvokeEvalForceArg(size_t Target) const {	IdxCurrentEvalRule = EvalForceArg_ER;IdxCurrentSelfEvalRule = None_SER;InferenceParameter1 = Target;};
	bool EvalForceNotArg(MetaConcept*& dest);
	bool RetypeToMetaConnective(MetaConcept*& dest);
	// Following block defined in ConEqual.cxx [not a class file: for rules that interact between MetaConnective and EqualRelation]
	void Init2AryEqualForZeroEq2ArySum(MetaConcept*& TmpEqual);	// can throw bad_alloc
	// Next four should be static, except that they're used in the evalrule lookup table
	bool EvalForceTrue(MetaConcept*& dest);
	bool EvalForceFalse(MetaConcept*& dest);
	void InvokeEvalForceFalse() const {	IdxCurrentEvalRule = EvalForceFalse_ER;IdxCurrentSelfEvalRule = None_SER;};
	bool EvalForceContradiction(MetaConcept*& dest);
	void InvokeEvalForceContradiction() const {	IdxCurrentEvalRule = EvalForceContradiction_ER;IdxCurrentSelfEvalRule = None_SER;};
	bool EvalForceUnknown(MetaConcept*& dest);
	// next members are implemented in QState.cxx
	void BuildInfluenceCounts(MetaQuantifier**& TVarList) const;

	// This routine returns true iff the arglist of the LHS occurs, in the same order,
	// as a subarglist of RHS.
	bool SubvectorArgList(const MetaConceptWithArgArray& rhs) const;
	bool GrepArgList(MetaConcept**& MirrorArgList,LowLevelUnaryProperty& WantThis) {return ::GrepArgList(MirrorArgList,WantThis,ArgArray);};
protected:
	void MoveIntoAux(MetaConceptWithArgArray& dest);
//	Override this for non-commutative types, or complicated internal structure
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual bool _IsExplicitConstant() const {return false;};

	void FastTransferOutAndNULL(size_t i, MetaConcept*& dest) {dest = ArgArray[i]; ArgArray[i]=NULL;};
	void FastCleanTransferOutAndNULL(size_t i, MetaConcept*& dest) {delete dest; dest = ArgArray[i]; ArgArray[i]=NULL;};
	void FastTransferInAndOverwrite(size_t i, MetaConcept* src) {delete ArgArray[i]; ArgArray[i]=src;};

	virtual void DiagnoseInferenceRules(void) const = 0;	// This is *not* the Interface!
	virtual bool InvokeEqualArgRule() const = 0;		// This is *not* the Interface!
	// this is because of Borland access problems from MetaConnective.
	void SetArgNInfParam1(size_t n, size_t NewVal) const {static_cast<MetaConceptWithArgArray*>(ArgArray[n])->InferenceParameter1 = NewVal;};
	void ForceStdFormAux() const;
	bool SyntaxOKAux() const {return ValidateArgArray(ArgArray);};
	size_t LengthOfPrefixArgList() const;
	void ConstructPrefixArgList(char* const PrefixArgListStart) const;
	size_t LengthOfCommaListVarNames() const;
	void ConstructCommaListVarNames(char*& CommaListStart) const;
	size_t LengthOfCommaListVarNames(size_t MinIdx, size_t StrictMaxIdx) const;
	void ConstructCommaListVarNames(char*& CommaListStart, size_t MinIdx, size_t StrictMaxIdx) const;

	inline void SetExactTypeV2(ExactType_MC NewType) {SetExactType(NewType); IdxCurrentSelfEvalRule = None_SER;};
	
	// Following block defined in MetaCon2.cxx
	void InvokeForceValue(MetaConcept* src) const {assert(src);assert(src->SyntaxOK());InferenceParameterMC=src;IdxCurrentEvalRule = ForceValue_ER;};
	bool ForceValue(MetaConcept*& dest);

	bool SelfEvalRuleEvaluateArgs();
	bool SelfEvalRuleEvaluateArg();
	bool SelfEvalRuleEvaluateArgOneStage();
	bool SelfEvalStrictlyModify();
	bool VirtualStrictlyModify();
	bool SelfEvalRuleForceArgSameImplementation();
	bool SelfEvalRuleCleanArg();
	bool SelfEvalRuleCleanTrailingArg();
	bool SelfEvalRuleCleanLeadingArg();
	bool SelfEvalRuleNIFFXORCleanArg();
	bool SelfEvalRuleNIFFXORCleanTrailingArg();
	bool SelfEvalRuleAry2CorrectedCleanArg();
	bool SelfEvalRuleAry2CorrectedCleanTrailingArg();
	bool SelfEvalRuleUnrollGeneralizedAssociativity();
	bool SelfEvalAddArgAtEndAndForceCorrectForm();
	bool CompatibleRetype();
	bool CompatibleRetypeOtherArgs();
	// Following block defined in MetaCon3.cxx
	bool LogicalANDSpliceNAryEqualArg();
	bool LogicalANDSpliceALLEQUALAddInvArg();
	bool ReplaceThisArgWithLeadArg();
	bool LogicalORXORCompactANDArgHyperCube();
	bool LogicalANDORXORCondenseORANDANDArgHyperCube();

	// Following block defined in StdAdd.cxx
	void InvokeForceZero() const;
	// Following defined in AddInterval.cxx
	bool TranslateInterval();
	// resume normal
	bool DiagnoseStandardEvalRules() const;
	bool DiagnoseEqualArgs() const;
	bool DiagnoseEvaluatableArgs() const;
	bool DiagnoseSelfAssociativeArgs() const;
	bool SilentDiagnoseSelfAssociativeArgs() const;
	void ForceTotalLexicalArgOrder() const;
	void ForceLexicalArgOrder(size_t lb, size_t strict_ub) const;
	// this routine invalidates IndexArray
	void CompactLowLevelArrayVectorToDeleteTrailingArgs(unsigned long* IndexArray) const;
	// this routine nondestructively blots the InferenceParameter1 entry in the ArgArray.
	bool CheckForTrailingCleanArg(LowLevelBinaryRelation& TargetRelation, SelfEvalRuleIdx_SER ChangeToRule, size_t ForceArity) const;
	void UseArg0ForPropagateAddMultInv(MetaConcept*& dest);	// leaves ArgArray[0] NULL
private:
	bool _AddArgAtEndAndForceCorrectForm(MetaConcept* src);
	void LogicalANDORCondenseORANDArgHyperCubeAuxDim1(size_t i, size_t i2);
	void LogicalORXORCompactANDArgHyperCubeAuxDim1(size_t Idx, size_t Idx2);
	void HypercubeArgRelationProcessor(VertexPairSelfProcess CleanupAction);

	virtual bool DelegateEvaluate(MetaConcept*& dest) {return false;};		// different type
	virtual bool DelegateSelfEvaluate() {return false;};		// same type
};

namespace zaimoni {

template<>
struct is_polymorphic_base<MetaConceptWithArgArray> : public std::true_type {};

}

#endif
