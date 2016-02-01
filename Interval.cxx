// Interval.cxx
// implementation for class LinearInterval, which represents linear intervals over various mathematical 
// domains.
// A linear interval is considered to be one of the following:
// * The range of a linear map f: I |-> Range, where I is one of [0,1], [0,1) i.e. [0,1[,
//   (0,1] i.e. ]0,1], and (0,1) i.e. ]0,1[
// * The points in a totally ordered topological space X that are between A,B in the topological 
//   space.
// In either case, we have open/closedness on each endpoint, depending on whether the endpoint 
// is actually included.
// We must specify the domain explicitly.

// LinearInterval is acceptable to AbstractClass.

//! \todo IMPLEMENT
//!   bool IsFinite()
//!   bool IsExplicitlyFinite(IntegerNumeral& Cardinality)
//!   bool IsExplicitlySmallFinite(unsigned long& Cardinality)
//!	above are member functions of both AbstractClass and LinearInterval.
//! \todo we now have linear infinity
//! UltimateType _Z_/_Q_/_R_ and open interval -&infin; to &infin; evaluates to _Z_/_Q_/_R_; 
//! however, we don't want to automate this much: X EQUALTOONEOF -&infin;,&infin;
//! is actually a domain spec

#include "MetaCon2.hxx"	// needed regardless, we're implementing something here
#include "Class.hxx"
#include "TruthVal.hxx"
#include "Integer1.hxx"
#include "Interval.hxx"

bool LinearInterval::KarlStrombergIntervals = true;

//! \todo modify following to use LinearInterval output when appropriate: 
//! ALLDISTINCT/NOTALLDISTINCT
//! EQUALTOONEOF/DISTINCTFROMALLOF

LinearInterval::LinearInterval(MetaConcept*& LHS, MetaConcept*& RHS, AbstractClass*& UltimateDomain, bool OpenOnLeft, bool OpenOnRight)
:	MetaConceptWith2Args(LinearInterval_MC,LHS,RHS),
	IntervalDomain(UltimateDomain),
	LeftPointOpen(OpenOnLeft),
	RightPointOpen(OpenOnRight)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/2005
	_forceStdForm();
}

LinearInterval::LinearInterval(const LinearInterval& src)
:	MetaConceptWith2Args(src),
	LeftPointOpen(src.LeftPointOpen),
	RightPointOpen(src.RightPointOpen)
{
	src.IntervalDomain->CopyInto(IntervalDomain);
}

const LinearInterval& LinearInterval::operator=(const LinearInterval& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/23/2002
	autodel_ptr<AbstractClass> Tmp;
	src.IntervalDomain->CopyInto(Tmp);
	MetaConceptWith2Args::operator=(src);
	IntervalDomain = Tmp;

	LeftPointOpen = src.LeftPointOpen;
	RightPointOpen = src.RightPointOpen;
	return *this;
}

void LinearInterval::MoveInto(LinearInterval*& dest)		// can throw memory failure.  If it succeeds, it destroys the source.
{	// FORMALLY CORRECT: Kenneth Boyd, 5/23/2002
	if (!dest) dest = new LinearInterval();
	MoveInto(*dest);
}

void LinearInterval::MoveInto(LinearInterval& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	dest.LeftPointOpen = LeftPointOpen;
	dest.RightPointOpen = RightPointOpen;
	dest.IntervalDomain = IntervalDomain;
	MoveIntoAux(dest);
}

const AbstractClass* LinearInterval::UltimateType() const
{
	return IntervalDomain;
}

bool
LinearInterval::ForceUltimateType(const AbstractClass* const rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/1/2003
	return !IntervalDomain.empty() && IntervalDomain->IntersectWith(*rhs);
}

bool LinearInterval::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	const LinearInterval& VR_rhs = static_cast<const LinearInterval&>(rhs);
	if (   LeftPointOpen==VR_rhs.LeftPointOpen
		&& RightPointOpen==VR_rhs.RightPointOpen
		&& MetaConceptWith2Args::EqualAux2(rhs))
		return true;
	return false;
}

void LinearInterval::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/14/2006
	if (   RHS_Arg2->SyntacticalStandardLT(*LHS_Arg1)		// use standard ordering first
		|| (   !LHS_Arg1->SyntacticalStandardLT(*RHS_Arg2)
			&&  RHS_Arg2->InternalDataLT(*LHS_Arg1)))		// fallback is lexical
		{
		swap(LHS_Arg1,RHS_Arg2);
		swap(LeftPointOpen,RightPointOpen);
		}

	if (   !IsUltimateType(NULL)
		&& !UltimateType()->Subclass(Real)
		&&  LHS_Arg1->UltimateType()->Subclass(Real)
		&&  RHS_Arg2->UltimateType()->Subclass(Real))
		{	// TODO: force UltimateType to be intersection of Reals and current domain
		IntervalDomain->IntersectWith(Real);
		}

	//! \todo need some way to force _Z_ intervals to close integer endpoints; not urgent
	if (!IsUltimateType(NULL) && UltimateType()->Subclass(Real))
		{
		if (LHS_Arg1->IsExactType(LinearInfinity_MC))
			LeftPointOpen = true;
		if (RHS_Arg2->IsExactType(LinearInfinity_MC))
			RightPointOpen = true;
		}
}

bool LinearInterval::SyntaxOK() const
{	// MUTABLE
	if (   !LHS_Arg1.empty() && LHS_Arg1->SyntaxOK()
		&& !RHS_Arg2.empty() && RHS_Arg2->SyntaxOK()
		&& !IntervalDomain.empty() && IntervalDomain->SyntaxOK())
		{
		// Interval with integer endpoints over subclass of complex numbers.
		// E.g.: Interval over integers: A...B
		if (   IntervalDomain->Subclass(Complex)
			&& Integer.Subclass(*IntervalDomain)
			&& LHS_Arg1->IsUltimateType(&Integer)
			&& RHS_Arg2->IsUltimateType(&Integer))
			return true;
		// reality check: real endpoints must tolerate intersection with reals for domain
		if (    LHS_Arg1->UltimateType()->Subclass(Real)
			&&  RHS_Arg2->UltimateType()->Subclass(Real))
			return !IntervalDomain->IntersectionWithIsNULLSet(Real);

		//! \todo other interval types
		}
	IdxCurrentSelfEvalRule = SelfEvalSyntaxBadNoRules_SER;
	return false;
}

bool LinearInterval::_IsExplicitConstant() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/9/2002
	if (IsUltimateType(&Integer)) return false;
	return MetaConceptWith2Args::_IsExplicitConstant();
}

bool LinearInterval::IsAbstractClassDomain() const
{
	if (	LHS_Arg1->IsExplicitConstant()
		&&	RHS_Arg2->IsExplicitConstant())
		return true;
	return false;
}

bool LinearInterval::StdAddCanDestructiveInteract() const
{	
	if (   !IntervalDomain.empty()
		&& !IsMetaAddInverted()		// remove this when AddInv properly implemented for LinearInterval
		&& !IsMetaMultInverted())
		return IntervalDomain->SupportsThisOperation(StdAddition_MC);
	return false;
}

bool
LinearInterval::StdAddCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const
{
	if (   Target.IsExplicitConstant()
		&& Target.IsFinite()
		&& NULL!=Target.UltimateType()
		&& Target.UltimateType()->Subclass(*IntervalDomain)
		&& (LHS_Arg1->IsExplicitConstant() || LHS_Arg1->IsExactType(StdAddition_MC))
		&& (RHS_Arg2->IsExplicitConstant() || RHS_Arg2->IsExactType(StdAddition_MC)))
		{	// default...should have alternate targets
		ActOnThisRule = MetaConceptWithArgArray::TranslateInterval_SER;
		return true;
		}
	return false;
}

#if 0
bool LinearInterval::StdMultCanDestructiveInteract() const
{	
	if (   NULL!=IntervalDomain
		&& IntervalDomain->IsDenseUnderStandardTopology()
		&& !IsMetaAddInverted()		// remove this when AddInv properly implemented for LinearInterval
		&& !IsMetaMultInverted())
		return IntervalDomain->SupportsThisOperation(StdMultiplication_MC);
	return false;
}

bool
LinearInterval::StdMultCanDestructiveInteract(const MetaConcept& Target,size_t& ActOnThisRule) const
{
	if (   Target.IsExplicitConstant()
		&& Target.IsNotZero()
		&& Target.IsFinite()
		&& NULL!=Target.UltimateType()
		&& Target.UltimateType()->Subclass(*IntervalDomain)
		&& (LHS_Arg1->IsExplicitConstant() || LHS_Arg1->IsExactType(StdMultiplication_MC))
		&& (RHS_Arg2->IsExplicitConstant() || RHS_Arg2->IsExactType(StdMultiplication_MC)))
		{
		ActOnThisRule = MetaConceptWithArgArray::ScaleInterval_SER;	//! \todo IMPLEMENT, file for StdMultiplication/LinearInterval functions
		return true;
		}
	return false;
}
#endif

bool LinearInterval::IsExpandable(unsigned long& ExpandedArity) const
{	//! \todo IMPLEMENT
#if 0
	if (   LHS_Arg1->IsUltimateType(&Integer)
		&& RHS_Arg2->IsUltimateType(&Integer))
		{	// want absolute difference of LHS_Arg1 and RHS_Arg2 to fit in unsigned long
		signed long Tmp;
		RHS_Arg2->SmallDifference(*LHS_Arg1,Tmp);
		if (0<Tmp)
			{
			ExpandedArity = Tmp;
			return true;
			}
		ExpandedArity = 0;
		return false;
		}
#endif
	ExpandedArity = 0;
	return false;
}

bool LinearInterval::ExpandArgsHere(MetaConcept** VR_ArgArray) const
{	//! \todo IMPLEMENT
#if 0
	if (   LHS_Arg1->IsExactType(IntegerNumeral_MC)
		&& LHS_Arg1->IsUltimateType(&Integer)
		&& RHS_Arg2->IsExactType(IntegerNumeral_MC)
		&& RHS_Arg2->IsUltimateType(&Integer))
		{
		unsigned long Offset = 0;
		try	{
			LHS_Arg1->CopyInto(VR_ArgArray[0]);
			while(VR_ArgArray[Offset]->InternalDataLT(RHS_Arg2))
				{
				Offset++;
				VR_ArgArray[Offset-1]->CopyInto(VR_ArgArray[Offset]);
				// static_cast<IntegerNumeral*>(VR_ArgArray[Offset])->...(1);
				//! \todo need to add constant 1 to new IntegerNumeral; this is not a safe manuever
				}
			}
		catch(const bad_alloc&)
			{
			DELETE_AND_NULL(VR_ArgArray[Offset]);
			while(Offset>0)
				DELETE_AND_NULL(VR_ArgArray[--Offset]);
			return false;
			}
		return true;
		}
#endif
	return false;
}

bool LinearInterval::HasAsElement(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/10/2002
	return _hasAsElement(rhs).is(true);
}

bool LinearInterval::DoesNotHaveAsElement(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/10/2002
	return _hasAsElement(rhs).is(false);
}

TVal LinearInterval::_hasAsElement(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/2/2005
	if (*LHS_Arg1==rhs) return !LeftPointOpen;
	if (*RHS_Arg2==rhs) return !RightPointOpen;
	if (UltimateType()->DoesNotHaveAsElement(rhs)) return false;

	if (LHS_Arg1->SyntacticalStandardLT(*RHS_Arg2))
		{
		if 		(   rhs.SyntacticalStandardLT(*LHS_Arg1)
				 || RHS_Arg2->SyntacticalStandardLT(rhs))
			return false;
		else if (   LHS_Arg1->SyntacticalStandardLT(rhs)
				 && rhs.SyntacticalStandardLT(*RHS_Arg2))
			return true;
		}
	return TVal();
}

bool LinearInterval::Subclass(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	return Subclass_LI & IntersectionUnionStatus(rhs);
}

bool LinearInterval::NotSubclass(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	unsigned long Result = IntersectionUnionStatus(rhs);
	return Result && !(Subclass_LI & Result);
}

TVal LinearInterval::_subclass(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	unsigned long Result = IntersectionUnionStatus(rhs);
	if (Result) return (Subclass_LI & Result);
	return TVal();	// unknown
}

bool
LinearInterval::ClearlyOverlapping(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	return Overlap_LI & IntersectionUnionStatus(rhs);
}

bool
LinearInterval::ClearlyNotOverlapping(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	unsigned long Result = IntersectionUnionStatus(rhs);
	return Result && !(Overlap_LI & Result);
}

TVal LinearInterval::_clearlyOverlapping(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	unsigned long Result = IntersectionUnionStatus(rhs);
	if (Result) return (Overlap_LI & Result);
	return TVal();
}

bool
LinearInterval::ClearlyMergeable(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	return Mergeable_LI & IntersectionUnionStatus(rhs);
}

bool
LinearInterval::ClearlyNotMergeable(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	unsigned long Result = IntersectionUnionStatus(rhs);
	return Result && !(Mergeable_LI & Result);
}

TVal LinearInterval::_clearlyMergeable(const LinearInterval& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/2002
	unsigned long Result = IntersectionUnionStatus(rhs);
	if (Result) return(Mergeable_LI & Result);
	return TVal();	// unknown
}

TVal LinearInterval::_clearlyExtendedBy(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/6/2002
	if (   (LeftPointOpen  && rhs==*LHS_Arg1)
		|| (RightPointOpen && rhs==*RHS_Arg2))
		return true;	// the missing endpoint of an open interval closes it
	
	if (   (LeftPointOpen && RightPointOpen)				// other tests require a closed endpoint
		||  UltimateType()->IsDenseUnderStandardTopology())	// stereotypical test for dense topological spaces
		return false;

	// let the UltimateType worry about discreteness (or lack thereof), etc.
	// NOTE: will need a 'obviously correct form check' on this
	if (   (!RightPointOpen && UltimateType()->Arg1IsAfterEndpointAlongVectorAB(rhs,*LHS_Arg1,*RHS_Arg2))
		|| (!LeftPointOpen  && UltimateType()->Arg1IsAfterEndpointAlongVectorAB(rhs,*RHS_Arg2,*LHS_Arg1)))
		return true;
	// if the interval is in a well-defined standard form, this is definitely false
	// ultimately depends on a decent partial-ordering type, and a knowledge of how powerful 
	// the logic engine is
	if (   IsUltimateType(&Integer)
		&& LHS_Arg1->IsExactType(IntegerNumeral_MC)
		&& RHS_Arg2->IsExactType(IntegerNumeral_MC))
		return false;
	return TVal();	// unknown
}

bool LinearInterval::DestructiveExtendBy(MetaConcept*& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/2/2005
	// the missing endpoint of an open interval closes it
	if (LeftPointOpen && *rhs==*LHS_Arg1)
		{
		LeftPointOpen = false;
		DELETE_AND_NULL(rhs);
		return true;
		};
	if (RightPointOpen && *rhs==*RHS_Arg2)
		{	// the missing endpoint of an open interval closes it
		RightPointOpen = false;
		DELETE_AND_NULL(rhs);
		return true;
		};

	// let the UltimateType worry about discreteness (or lack thereof), etc.
	if (!RightPointOpen && UltimateType()->Arg1IsAfterEndpointAlongVectorAB(*rhs,*LHS_Arg1,*RHS_Arg2))
		{
		RHS_Arg2 = rhs;
		rhs = NULL;
		return true;
		}
	if (!LeftPointOpen  && UltimateType()->Arg1IsAfterEndpointAlongVectorAB(*rhs,*RHS_Arg2,*LHS_Arg1))
		{
		LHS_Arg1 = rhs;
		rhs = NULL;
		return true;
		}
	return false;
}

void LinearInterval::DiagnoseInferenceRules()
{	//! \todo IMPLEMENT
	// NOTE: empty intervals are handled by the master type, not this!
	// an interval with cardinality 1 evaluates to its sole member
	// special case: equal endpoints, closed interval
	//! \todo An ALLEQUAL clause should be able to trigger singleton reduction as a StrictlyModify move
	if (*LHS_Arg1==*RHS_Arg2)
		{
		if (!LeftPointOpen && !RightPointOpen)
			{
			IdxCurrentEvalRule = ForceLHSArg1_ER;
			return;
			}
		//! \bug NULLSet from singleton with open endpoint(s)...needs special detection and handling
		IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
		return;
		}
	if 		(LHS_Arg1->CanEvaluate())
		{
		IdxCurrentSelfEvalRule = (RHS_Arg2->CanEvaluate()) ? EvaluateBothArgs_SER : EvaluateLHSArg1_SER;
		return;
		}
	else if (RHS_Arg2->CanEvaluate())
		{
		IdxCurrentSelfEvalRule = EvaluateRHSArg2_SER;
		return;
		}

	//! \todo Need destructive evaluation rules to guarantee normal-form
	//! <br>Since constants will translate/scale intervals if they have appropriate internal structure,
	//! <br>force paired sums and products (one of which is 2-ary with 1 constant) to "unfactor"
	//! <br>This also requires testing for arg evaluation

	// next four assume it is known that LHS<RHS
	//! \todo an interval with domain integer and closed RHS of type StdAddition with constant 
	//! part -1 should convert to open RHS without -1
	//! \todo an interval with domain integer and closed LHS of type StdAddition with constant 
	//! part +1 should convert to open LHS without +1

	// next two should not happen from direct data entry:
	//! \todo an interval with domain integer and open RHS of type StdAddition with constant 
	//! part +1 should convert to closed RHS without +1
	//! \todo an interval with domain integer and open LHS of type StdAddition with constant 
	//! part -1 should convert to closed LHS without -1
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

bool LinearInterval::InvokeEqualArgRule()
{	//! \todo IMPLEMENT
	//! <br>a closed interval with equal endpoints evaluates to its endpoint
	//! <br>a half-open or open interval with equal endpoints evalutes to NULLSet
	return false;
}

// Mergeable_LI = Superclass_LI | Subclass_LI | Union_BC_LI | Union_AD_LI | Union_ACD_LI | Union_ABD_LI | Union_CAB_LI | Union_CDB_LI
// Superclass, Subclass, Union_BC, Union_AD are destructive
// Union_ACD_LI, Union_ABD_LI, Union_CAB_LI, Union_CDB_LI are nondestructive
// META: While it is *structured* to recalculate, maybe it isn't *efficient*
// true: success, RHS is discarded
bool LinearInterval::DestructiveMergeWith(LinearInterval& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/2/2005
	unsigned long Result = IntersectionUnionStatus(rhs);
	if (DestructiveMergeable_LI & Result)
		{
		if 		(Superclass_LI & Result)
			return true;
		else if (Subclass_LI & Result)
			{	// i.e, RHS is superclass
			rhs.MoveInto(*this);
			return true;
			}
		else if (Union_BC_LI & Result)
			{	// domains equal: need AB |-> CB
			LHS_Arg1 = rhs.LHS_Arg1;
			LeftPointOpen = rhs.LeftPointOpen;
			return true;
			}
		else if (Union_AD_LI & Result)
			{	// domains equal: need AB |-> AD
			RHS_Arg2 = rhs.RHS_Arg2;
			RightPointOpen = rhs.RightPointOpen;
			return true;
			}
		else
			FATAL(AlphaRetValAssumption);
		}
	return false;
}

// true: success, should keep RHS
bool LinearInterval::NonDestructiveMergeWith(LinearInterval& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/16/2002
	unsigned long Result = IntersectionUnionStatus(rhs);
	if (NonDestructiveMergeable_LI & Result)
		{
		if 		(Union_ACD_LI & Result)
			{	// AB, CD |-> AC,CD
			rhs.LHS_Arg1->CopyInto(RHS_Arg2);
			RightPointOpen = !rhs.LeftPointOpen;
			return true;
			}
		else if (Union_ABD_LI & Result)
			{	// AB, CD |-> AB,BD
			RHS_Arg2->CopyInto(rhs.LHS_Arg1);
			rhs.LeftPointOpen = !RightPointOpen;
			return true;
			}
		else if (Union_CAB_LI & Result)
			{	// AB, CD |-> AB,CA
			LHS_Arg1->CopyInto(rhs.RHS_Arg2);
			rhs.RightPointOpen = !LeftPointOpen;
			return true;
			}
		else if (Union_CDB_LI & Result)
			{	// AB, CD |-> DB,CD
			rhs.RHS_Arg2->CopyInto(LHS_Arg1);
			LeftPointOpen = !rhs.RightPointOpen;
			return true;
			}
		else
			FATAL(AlphaRetValAssumption);
		}
	return false;
}

bool LinearInterval::DestructiveIntersectWith(LinearInterval& rhs)
{	//! \todo IMPLEMENT
	unsigned long Result = IntersectionUnionStatus(rhs);
	if (DestructiveIntersectable_LI & Result)
		{	//! \todo intersect domains
		if 		(Intersect_CB_LI & Result)
			{	// overwrite LHS_Arg1 with RHS.LHS_Arg1
			if (ForceUltimateType(rhs.UltimateType()))
				{
				LHS_Arg1 = rhs.LHS_Arg1;
				return true;
				}
			}
		else if (Intersect_AD_LI & Result)
			{	// overwrite RHS_Arg1 with RHS.RHS_Arg1
			if (ForceUltimateType(rhs.UltimateType()))
				{
				RHS_Arg2 = rhs.RHS_Arg2;
				return true;
				}
			}
		else
			FATAL(AlphaRetValAssumption);
		}
	return false;
}


unsigned long
LinearInterval::IntersectionUnionStatus(const LinearInterval& rhs) const
{	//! \todo IMPLEMENT
	//! \todo universal check when domains are equal, and subclass of Reals
	if (	rhs.IsUltimateType(UltimateType())
		&&	UltimateType()->Subclass(Real)
		&&	LHS_Arg1->SyntacticalStandardLT(*RHS_Arg2)
		&&  rhs.LHS_Arg1->SyntacticalStandardLT(*rhs.RHS_Arg2))
		{	// Integer domain has weirdness: discrete topology sometimes allows concise union 
			// even when intersection is NULLSet
		if (RHS_Arg2->SyntacticalStandardLT(*rhs.LHS_Arg1))
			{
			signed long Result;
			if (   IsUltimateType(&Integer)
				&& rhs.LHS_Arg1->SmallDifference(*RHS_Arg2,Result) && 1==Result)
				return Intersect_NULLset_LI | Union_AD_LI;
			else
				return Intersect_NULLset_LI;
			};
		if (rhs.RHS_Arg2->SyntacticalStandardLT(*LHS_Arg1))
			{
			signed long Result;
			if (   IsUltimateType(&Integer)
				&& LHS_Arg1->SmallDifference(*rhs.RHS_Arg2,Result) && 1==Result)
				return Intersect_NULLset_LI | Union_BC_LI;
			else
				return Intersect_NULLset_LI;
			};

		// disjoint now handled: B<C, A<D not true
		// ABCD	*
		// ACBD overlap
		// ACDB envelop
		// CABD envelop
		// CADB overlap
		// CDAB	*
		// NOTE: being able to snap-evaluate "decidable total-order" of SyntacticalStandardLT
		// would allow some conditional accelerations here.
		if 		(*LHS_Arg1==*rhs.LHS_Arg1)
			{	// C==A
			if 		(*RHS_Arg2==*rhs.RHS_Arg2)
				return Subclass_LI | Superclass_LI;
			else if (RHS_Arg2->SyntacticalStandardLT(*rhs.RHS_Arg2))
				return Superclass_LI;
			else if (rhs.RHS_Arg2->SyntacticalStandardLT(*RHS_Arg2))
				return Subclass_LI;
			}
		else if (LHS_Arg1->SyntacticalStandardLT(*rhs.LHS_Arg1))
			{	// AC
			if 		(RHS_Arg2->SyntacticalStandardLT(*rhs.RHS_Arg2))
				return Union_AD_LI | Intersect_CB_LI;
			else if (rhs.RHS_Arg2->SyntacticalStandardLT(*RHS_Arg2))
				return Superclass_LI;
			}
		else if (rhs.LHS_Arg1->SyntacticalStandardLT(*LHS_Arg1))
			{	// CA
			if 		(rhs.RHS_Arg2->SyntacticalStandardLT(*RHS_Arg2))
				return Union_BC_LI | Intersect_AD_LI;
			else if (RHS_Arg2->SyntacticalStandardLT(*rhs.RHS_Arg2))
				return Subclass_LI;
			}
		}

	// universal test: check for subclassing/equality when both endpoints equal
	if (   *LHS_Arg1==*rhs.LHS_Arg1
		&& *RHS_Arg2==*rhs.RHS_Arg2)
		{
		if    (LeftPointOpen==rhs.LeftPointOpen)
			{
			if    (RightPointOpen==rhs.RightPointOpen)
				{
				if (*UltimateType()==*rhs.UltimateType())
					return Subclass_LI | Superclass_LI;
				else if (UltimateType()->Subclass(*rhs.UltimateType()))
					return Subclass_LI;
				else if (rhs.UltimateType()->Subclass(*UltimateType()))
					return Superclass_LI;
				else
					return Intersect_AD_LI;
				}
			else if (RightPointOpen && !rhs.RightPointOpen)
				{
				if (UltimateType()->Subclass(*rhs.UltimateType()))
					return Subclass_LI;
				else
					return Intersect_AD_LI;
				}
			else{	// (!RightPointOpen && rhs.RightPointOpen)
				if (rhs.UltimateType()->Subclass(*UltimateType()))
					return Superclass_LI;
				else
					return Intersect_AD_LI;
				}
			}
		else if (LeftPointOpen && !rhs.LeftPointOpen)
			{
			if (   (RightPointOpen || !rhs.RightPointOpen)
				&& UltimateType()->Subclass(*rhs.UltimateType()))
				return Subclass_LI;				
			else	// (!RightPointOpen && rhs.RightPointOpen)
				return Intersect_AD_LI;
			}
		else{	// (!LeftPointOpen && rhs.LeftPointOpen)
			if (   (!RightPointOpen || rhs.RightPointOpen)
				&& rhs.UltimateType()->Subclass(*UltimateType()))
				return Superclass_LI;				
			else // (RightPointOpen && !rhs.RightPointOpen)
				return Intersect_AD_LI;				
			}
		};

	//! \todo intervals in vector spaces should check for collinearity and singleton intersection
	if 		(IsUltimateType(rhs.UltimateType()))
		{
		return 0;
		}
	else if (UltimateType()->Subclass(*rhs.UltimateType()))
		{
		return 0;
		}
	else if (rhs.UltimateType()->Subclass(*UltimateType()))
		{
		return 0;
		}
	else{
		return Intersect_NULLset_LI;
		}
}

bool
LinearInterval::MetaInterpretLinearInterval(MetaConcept*& Arg1, const LinearInterval& LHSArg, const LinearInterval& RHSArg, DestructiveLinearIntervalProcess TargetOperation)
{	// FORMALLY CORRECT: 12/31/2002
	// NOTE: substantial RAM efficiency advantage if Arg1==LHS.Arg1
	LinearInterval* RHSMirror = NULL;
	if (*Arg1==LHSArg)
		{
		try	{
			RHSArg.CopyInto(RHSMirror);
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		if ((static_cast<LinearInterval*>(Arg1)->*TargetOperation)(*RHSMirror))
			{	// deal with singleton results
			delete RHSMirror;
			static_cast<LinearInterval*>(Arg1)->DiagnoseInferenceRules();
			if (ForceLHSArg1_ER==static_cast<LinearInterval*>(Arg1)->IdxCurrentEvalRule)
				{
				MetaConcept* Tmp = NULL;
				static_cast<LinearInterval*>(Arg1)->TransferOutLHSAndNULL(Tmp);
				delete Arg1;
				Arg1 = Tmp;
				}
			return true;
			}
		}
	else{
		LinearInterval* LHSMirror = NULL;
		try	{
			LHSArg.CopyInto(LHSMirror);
			RHSArg.CopyInto(RHSMirror);
			}
		catch(const bad_alloc&)
			{
			delete LHSMirror;
			return false;
			}
		if ((LHSMirror->*TargetOperation)(*RHSMirror))
			{	// deal with singleton results
			delete RHSMirror;
			LHSMirror->DiagnoseInferenceRules();
			if (ForceLHSArg1_ER==LHSMirror->IdxCurrentEvalRule)
				{
				LHSMirror->TransferOutLHSAndNULL(Arg1);
				delete LHSMirror;
				}
			else{
				delete Arg1;
				Arg1 = LHSMirror;
				}
			return true;
			}
		delete LHSMirror;
		}
	delete RHSMirror;
	return false;
}

bool
LinearInterval::MetaInterpretLinearInterval(MetaConcept*& Arg1, const LinearInterval& LHSArg, const MetaConcept& RHSArg, DestructiveLinearIntervalProcessV2 TargetOperation)
{	// FORMALLY CORRECT: 12/31/2002
	// NOTE: substantial RAM efficiency advantage if Arg1==LHS.Arg1
	MetaConcept* RHSMirror = NULL;
	if (*Arg1==LHSArg)
		{
		try	{
			RHSArg.CopyInto(RHSMirror);
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		if ((static_cast<LinearInterval*>(Arg1)->*TargetOperation)(RHSMirror))
			{	// deal with singleton results
			static_cast<LinearInterval*>(Arg1)->DiagnoseInferenceRules();
			if (ForceLHSArg1_ER==static_cast<LinearInterval*>(Arg1)->IdxCurrentEvalRule)
				{
				MetaConcept* Tmp = NULL;
				static_cast<LinearInterval*>(Arg1)->TransferOutLHSAndNULL(Tmp);
				delete Arg1;
				Arg1 = Tmp;
				}
			return true;
			}
		}
	else{
		LinearInterval* LHSMirror = NULL;
		try	{
			LHSArg.CopyInto(LHSMirror);
			RHSArg.CopyInto(RHSMirror);
			}
		catch(const bad_alloc&)
			{
			delete LHSMirror;
			return false;
			}
		if ((LHSMirror->*TargetOperation)(RHSMirror))
			{	// deal with singleton results
			LHSMirror->DiagnoseInferenceRules();
			if (ForceLHSArg1_ER==LHSMirror->IdxCurrentEvalRule)
				{
				LHSMirror->TransferOutLHSAndNULL(Arg1);
				delete LHSMirror;
				}
			else{
				delete Arg1;
				Arg1 = LHSMirror;
				}
			return true;
			}
		delete LHSMirror;
		}
	delete RHSMirror;
	return false;
}

