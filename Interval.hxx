// Interval.hxx
// header for class LinearInterval, which represents linear intervals over various mathematical 
// domains.
// A linear interval is considered to be one of the following:
// * The range of a linear map f: I |-> Range, where I is one of [0,1], [0,1) i.e. [0,1[,
//   (0,1] i.e. ]0,1], and (0,1) i.e. ]0,1[
// * The points in a totally ordered topological space X that are between A,B in the topological 
//   space.
// In either case, we have open/closedness on each endpoint, depending on whether the endpoint 
// is actually included.
// We must specify the domain explicitly.

// The preliminary type for an Interval is a 2-ary Phrase.

#ifndef LINEARINTERVAL_DEF
#define LINEARINTERVAL_DEF

#include "MetaCon4.hxx"
#include "TVal.hxx"

// Interpretation for IntersectionUnionStatus
enum IntersectionUnion_LI
{
	NoInfo_LI = 0,
	Intersect_NULLset_LI = 1,

	// Union Evaluation action: replace [A,B],[C,D] with [A,D]
	Union_BC_LI = 2,
	Union_AD_LI = 4,

	// TODO: decide whether this section and the other group of 4 are duals (and thus can 
	// use the same bits

	// Union Evaluation action: replace [A,B],[C,D] with [A,C[,[C,D]
	Union_ACD_LI = 8,
	Union_ABD_LI = 16,
	Union_CAB_LI = 32,
	Union_CDB_LI = 64,

	// Union/Intersection Evaluation action: delete target entirely [super/subclass status]
	Subclass_LI = 128,
	Superclass_LI = 256,

	// Intersection evaluation action: replace [A,B],[C,D] with [C,B], domain intersection of original domains
	Intersect_CB_LI = 512,
	Intersect_AD_LI = 1024,

	// Masks of interest for certain uses
	Envelop_LI = Superclass_LI,
	Enveloped_LI = Subclass_LI,
	Equal_LI = Superclass_LI | Subclass_LI,
	Overlap_LI = Superclass_LI | Subclass_LI | Intersect_CB_LI | Intersect_AD_LI,
	Mergeable_LI = Superclass_LI | Subclass_LI | Union_BC_LI | Union_AD_LI | Union_ACD_LI | Union_ABD_LI | Union_CAB_LI | Union_CDB_LI,
	// destructive, here, means lose an interval entirely
	DestructiveMergeable_LI = Superclass_LI | Subclass_LI | Union_BC_LI | Union_AD_LI,
	NonDestructiveMergeable_LI =  Union_ACD_LI | Union_ABD_LI | Union_CAB_LI | Union_CDB_LI,
	DestructiveIntersectable_LI = Intersect_CB_LI | Intersect_AD_LI
};

class LinearInterval final : public MetaConceptWith2Args
{
private:
	autoval_ptr<AbstractClass> IntervalDomain;
	bool LeftPointOpen;
	bool RightPointOpen;	

	LinearInterval() = delete;
public:
	typedef bool (LinearInterval::*DestructiveLinearIntervalProcess)(LinearInterval& RHS);
	typedef bool (LinearInterval::*DestructiveLinearIntervalProcessV2)(MetaConcept*& RHS);

	static bool KarlStrombergIntervals;

	LinearInterval(MetaConcept*& lhs, MetaConcept*& rhs, AbstractClass*& UltimateDomain, bool OpenOnLeft, bool OpenOnRight);
	LinearInterval(const LinearInterval& src);
	LinearInterval(LinearInterval&& src) = default;
	LinearInterval& operator=(const LinearInterval& src);
	LinearInterval& operator=(LinearInterval&& src) = default;
	virtual ~LinearInterval() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(LinearInterval*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this),dest); }
	void MoveInto(LinearInterval*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	const AbstractClass* UltimateType() const override { return IntervalDomain; }
	virtual bool ForceUltimateType(const AbstractClass* const src);
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
	bool IsAbstractClassDomain() const override;

	virtual bool StdAddCanDestructiveInteract() const;
	virtual bool StdAddCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const;

// Formal manipulation functions
	bool IsExpandable(unsigned long& ExpandedArity) const;
	bool ExpandArgsHere(MetaConcept** VR_ArgArray) const;

	bool HasAsElement(const MetaConcept& rhs) const;
	TVal _hasAsElement(const MetaConcept& rhs) const;	// AbstractClass needs access to this one
	bool DoesNotHaveAsElement(const MetaConcept& rhs) const;
	bool Subclass(const LinearInterval& rhs) const;
	TVal _subclass(const LinearInterval& rhs) const;
	bool NotSubclass(const LinearInterval& rhs) const;
	bool ClearlyOverlapping(const LinearInterval& rhs) const;
	bool ClearlyNotOverlapping(const LinearInterval& rhs) const;
	bool ClearlyMergeable(const LinearInterval& rhs) const;
	bool ClearlyNotMergeable(const LinearInterval& rhs) const;
	bool DestructiveMergeWith(LinearInterval& rhs);	// true: success, should discard RHS
	bool NonDestructiveMergeWith(LinearInterval& rhs);
	bool DestructiveIntersectWith(LinearInterval& rhs);	// true: success, should discard RHS
	bool ClearlyExtendedBy(const MetaConcept& rhs) const { return _clearlyExtendedBy(rhs).is(true); };
	bool ClearlyNotExtendedBy(const MetaConcept& rhs) const { return _clearlyExtendedBy(rhs).is(false); };
	bool DestructiveExtendBy(MetaConcept*& rhs);	// true: success; RHS is null
	// next one implemented in AddInter.cxx
	bool TranslateInterval(MetaConcept*& rhs);

	unsigned long IntersectionUnionStatus(const LinearInterval& rhs) const;

	static bool MetaInterpretLinearInterval(MetaConcept*& Arg1, const LinearInterval& LHSArg, const LinearInterval& RHSArg, DestructiveLinearIntervalProcess TargetOperation);
	static bool MetaInterpretLinearInterval(MetaConcept*& Arg1, const LinearInterval& LHSArg, const MetaConcept& RHSArg, DestructiveLinearIntervalProcessV2 TargetOperation);
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	std::string to_s_aux() const override;
	void _forceStdForm() override;
	virtual bool _IsExplicitConstant() const;

	virtual void DiagnoseInferenceRules();	// This is *not* the Interface!
	virtual bool InvokeEqualArgRule();
private:
	TVal _clearlyOverlapping(const LinearInterval& rhs) const;
	TVal _clearlyMergeable(const LinearInterval& rhs) const;
	TVal _clearlyExtendedBy(const MetaConcept& rhs) const;
};

#endif
