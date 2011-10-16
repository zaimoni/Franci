// Class.cxx
// implementation for AbstractClass

#include "Class.hxx"
#include "TruthVal.hxx"
#include "Integer1.hxx"
#include "SymConst.hxx"
#include "Interval.hxx"

// defined in LexParse.cxx
bool ImproviseVar(MetaConcept*& Target, const AbstractClass* Domain);

// ASSUMPTION: new implemented with calloc

const char* const Reserved_TruthValues = "TruthValues";
const char* const Reserved_Integer = "_Z_";
const char* const Reserved_Rational = "_Q_";
const char* const Reserved_Real = "_R_";
const char* const Reserved_Complex = "_C_";
const char* const Reserved_NULLSet = "NULLSET";
const char* const Reserved_ClassAllSets = "_Set_";
const char* const Reserved_ClassAdditionDefined = "_ADD_DEF_";
const char* const Reserved_ClassMultiplicationDefined = "_MULT_DEF_";
const char* const Reserved_ClassAdditionMultiplicationDefined = "_ADD_MULT_DEF_";

//! \todo When the alternative description methods are used, they will use Arg1.

AbstractClass::AbstractClass(void)
:	MetaConceptWith1Arg(AbstractClass_MC),
	Attributes1Bitmap(0)
{
}

AbstractClass::AbstractClass(const char* const NewName)	// object now has copy of NewName
:	MetaConceptWith1Arg(AbstractClass_MC),
	ClassName((assert(NewName && *NewName),ZAIMONI_LEN_WITH_NULL(strlen(NewName)))),
	Attributes1Bitmap(0)
{
	strcpy(ClassName,NewName);
}

AbstractClass::AbstractClass(MetaConcept*& src)
:	MetaConceptWith1Arg(AbstractClass_MC,(assert(src),assert(src->IsAbstractClassDomain()),assert(!src->CanEvaluate()),src)),
	Attributes1Bitmap(0)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
	Set_IsProperSet();
}

const AbstractClass& AbstractClass::operator=(const AbstractClass& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/26/2006
	if (ClassName.size()>=src.ClassName.size())
		{
		MetaConceptWith1Arg::operator=(src);
		ClassName = src.ClassName;		
		}
	else{
		autovalarray_ptr_throws<char> tmp(src.ClassName);
		MetaConceptWith1Arg::operator=(src);
		tmp.MoveInto(ClassName);
		}
	Attributes1Bitmap = src.Attributes1Bitmap;
	return *this;
}

bool AbstractClass::SetToThis(const AbstractClass& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/25/2009
	try	{
		*this=src;
		return true;
		}
	catch(const std::bad_alloc&)
		{
		return false;
		};
}

void AbstractClass::MoveInto(AbstractClass*& dest)	// can throw memory failure.  If it succeeds, it destroys the source.
{	// FORMALLY CORRECT: Kenneth Boyd, 12/9/2004
	if (!dest) dest = new AbstractClass();
	MoveInto(*dest);
}

void AbstractClass::MoveInto(AbstractClass& dest)	// destroys the source.
{	// FORMALLY CORRECT: Kenneth Boyd, 11/26/2006
	ClassName.MoveInto(dest.ClassName);
	dest.Attributes1Bitmap=Attributes1Bitmap;
	MoveIntoAux(dest);
}

bool AbstractClass::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: 1/22/1999, Kenneth Boyd
	if (   MetaConceptWith1Arg::EqualAux2(rhs)
		&& Attributes1Bitmap == static_cast<const AbstractClass&>(rhs).Attributes1Bitmap
		&& 0 == strcmp(ClassName,static_cast<const AbstractClass&>(rhs).ClassName))
		return true;
	return false;
}

bool AbstractClass::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/27/2000
	if (!MetaConceptWith1Arg::EqualAux2(rhs))
		return MetaConceptWith1Arg::InternalDataLTAux(rhs);

	const AbstractClass& VR_rhs = static_cast<const AbstractClass&>(rhs);
	{
	int Tmp = strcmp(ClassName,VR_rhs.ClassName);
	if (Tmp) return 0>Tmp;
	}
	// type is known to be the same
	if (VR_rhs.Arg1.empty()) return false;
	if (Arg1.empty()) return true;
	return Arg1->InternalDataLT(*VR_rhs.Arg1);
}

bool AbstractClass::_IsExplicitConstant() const
{	//! \todo I'll have to change this later...
	return true;
}

//  Type ID functions
const AbstractClass* AbstractClass::UltimateType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/27/2000
	if (ProperSet & Attributes1Bitmap)
		return &ClassAllSets;
	else
		return NULL;
}

//	Arity functions
size_t AbstractClass::size() const {return 0;}
const MetaConcept* AbstractClass::ArgN(size_t n) const {return NULL;}
MetaConcept* AbstractClass::ArgN(size_t n) {return NULL;}

//  Evaluation functions
//! \todo change the evaluation functions to delegate to non-NULL Arg1
bool AbstractClass::CanEvaluate() const {return false;}
bool AbstractClass::CanEvaluateToSameType() const {return false;}

bool AbstractClass::SyntaxOK() const
{	// FORMALLY CORRECT: 12/30/2002, Kenneth Boyd
	if (   (ProperClass & Attributes1Bitmap)
        && (ProperSet & Attributes1Bitmap))		// no class is both a proper set, and a proper class
		return false;
	if (    ClassName.empty()						// non-NULL name, or non-NULL SyntaxOK Arg1
		&& (Arg1.empty() || !Arg1->SyntaxOK()))
		return false;
	return true;
}							

bool AbstractClass::Evaluate(MetaConcept*& dest) {return false;}
bool AbstractClass::DestructiveEvaluateToSameType() {return false;}

// Type-specific controls
void
AbstractClass::Set_IsProperSet(void)
{	// FORMALLY CORRECT: 4/16/98, Kenneth Boyd
	Attributes1Bitmap &= ~ProperClass;
	Attributes1Bitmap |= ProperSet;
}

void
AbstractClass::Set_IsProperClass(void)
{	// FORMALLY CORRECT: 4/16/98, Kenneth Boyd
	Attributes1Bitmap &= ~ProperSet;
	Attributes1Bitmap |= ProperClass;
}

//! \todo We'll have to force proper set if the definitions don't work with proper classes on the 
//! next three.  I don't have time to verify failure of (or show consistency for) this now.
void
AbstractClass::Set_IsDenseUnderStandardTopology(void)
{	// INDEFINITE VERIFY
	Attributes1Bitmap &= ~DiscreteUnderStandardTopology;
	Attributes1Bitmap |= DenseUnderStandardTopology;
}

void
AbstractClass::Set_IsDiscreteUnderStandardTopology(void)
{	// INDEFINITE VERIFY
	Attributes1Bitmap &= ~DenseUnderStandardTopology;
	Attributes1Bitmap |= DiscreteUnderStandardTopology;
}

void
AbstractClass::Set_TopologicalCompletesWithInfinity(void)
{
	Attributes1Bitmap &= ~ContainsInfinity;
	Attributes1Bitmap |= TopologicalCompletesWithInfinity;
}

void
AbstractClass::Set_ContainsInfinity(void)
{
	Attributes1Bitmap &= ~TopologicalCompletesWithInfinity;
	Attributes1Bitmap |= ContainsInfinity;
}


const char* AbstractClass::ViewKeyword() const {return ClassName;}

bool AbstractClass::IsReservedSetClassName(const char* Name)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/27/2000
	if (   0==strcmp(Name,Reserved_TruthValues)
		|| 0==strcmp(Name,Reserved_Integer)
		|| 0==strcmp(Name,Reserved_Rational)
		|| 0==strcmp(Name,Reserved_Real)
		|| 0==strcmp(Name,Reserved_Complex)
		|| 0==strcmp(Name,Reserved_NULLSet)
		|| 0==strcmp(Name,Reserved_ClassAllSets)
		|| 0==strcmp(Name,Reserved_ClassAdditionDefined)
		|| 0==strcmp(Name,Reserved_ClassMultiplicationDefined)
		|| 0==strcmp(Name,Reserved_ClassAdditionMultiplicationDefined))
		return true;
	return false;
}

// only called from UnparsedText::Evaluate(...); guard is IsReservedSetClassName
bool
AbstractClass::ConvertToReservedAbstractClass(MetaConcept*& Target, const char* Text)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/27/2000
	assert(!Target);
	try	{
		if		(0==strcmp(Text,Reserved_TruthValues))
			TruthValues.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_Integer))
			Integer.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_Rational))
			Rational.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_Real))
			Real.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_Complex))
			Complex.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_NULLSet))
			NULLSet.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_ClassAllSets))
			ClassAllSets.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_ClassAdditionDefined))
			ClassAdditionDefined.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_ClassMultiplicationDefined))
			ClassMultiplicationDefined.CopyInto(Target);
		else if (0==strcmp(Text,Reserved_ClassAdditionMultiplicationDefined))
			ClassAdditionMultiplicationDefined.CopyInto(Target);
		else
			FATAL(AlphaCallAssumption);
		return true;
		}
	catch(const bad_alloc&)
		{
		return false;
		};
}

// The subclass relation
// First two functions only return true if they are *certain*.
// Second two use TruthValue to handle UNKNOWN (which means "needs detailed analysis")
// These may want function pointers
bool AbstractClass::ProperSubclass(const AbstractClass& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	TruthValue RetVal;
	ProperSubclass(rhs,RetVal);
	return RetVal.IsTrue();
}

bool AbstractClass::Subclass(const AbstractClass& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	TruthValue RetVal;
	Subclass(rhs,RetVal);
	return RetVal.IsTrue();
}

void AbstractClass::ProperSubclass(const AbstractClass& rhs, TruthValue& RetVal) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	if (*this==rhs || NULLSet==rhs || TruthValues==rhs)
		{
		RetVal.SetFalse();
		return;
		};
	Subclass_core(rhs,RetVal);
}

// redundant exit code.
void
AbstractClass::Subclass(const AbstractClass& rhs, TruthValue& RetVal) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	if (*this==rhs)				// no further analysis
		{
		RetVal.SetTrue();
		return;
		}
	Subclass_core(rhs,RetVal);
}

//! \todo FIX: partially reprogram the following these as abstract binary relations....or redefine them to use handlers
void AbstractClass::Subclass_core(const AbstractClass& rhs, TruthValue& RetVal) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	if (ClassAllSets==rhs)	// no further analysis
		{
		RetVal.SetTrue();
		return;
		}
	else if (ClassAdditionDefined==rhs)
		{
		RetVal = SupportsThisOperation(StdAddition_MC);
		return;
		}
	else if (ClassMultiplicationDefined==rhs)
		{
		RetVal = SupportsThisOperation(StdMultiplication_MC);
		return;
		}
	else if (ClassAdditionMultiplicationDefined==rhs)
		{
		RetVal = SupportsThisOperation(StdMultiplication_MC) && SupportsThisOperation(StdAddition_MC);
		return;
		}
	else if (!Arg1.empty())
		{
		if (Arg1->IsExactType(LinearInterval_MC))
			{	// LinearInterval
			if (rhs.Arg1.empty())
				{	// RHS is symbolic domain
				if (IsUltimateType(NULL))
					RetVal.SetFalse();
				else
					UltimateType()->Subclass(rhs,RetVal);
				return;
				}
			else if (rhs.Arg1->IsExplicitConstant())
				{	// RHS is singleton: fail
				RetVal.SetFalse();
				return;
				}
			else if (rhs.Arg1->IsExactType(LinearInterval_MC))
				{	// RHS is LinearInterval: delegate to LinearInterval code
				static_cast<LinearInterval*>((MetaConcept*)Arg1)->Subclass(*static_cast<LinearInterval*>((MetaConcept*)rhs.Arg1));
				}
			}
		else if (Arg1->IsExplicitConstant())
			{	// singleton
			rhs.HasAsElement(*Arg1,RetVal);
			return;
			}
		}
	else{	// Symbolic LHS here
		// META: This is a hardcoded binary relation.  We should be able to improve this later.
		if (!rhs.Arg1.empty())
			{	// detailed RHS
			if (   rhs.Arg1->IsExactType(LinearInterval_MC)
				|| rhs.Arg1->IsExplicitConstant())
				{
				RetVal.SetFalse();
				return;
				}
			}
		else{	// symbolic RHS here
			if (Integer==*this)
				{
				if (Rational==rhs || Real==rhs || Complex==rhs)
					RetVal.SetTrue();
				else
					RetVal.SetUnknown();
				return;
				}
			else if (Rational==*this)
				{
				if		(Real==rhs || Complex==rhs)
					RetVal.SetTrue();
				else if (Integer==rhs)
					RetVal.SetFalse();
				else
					RetVal.SetUnknown();
				return;
				}
			else if (Real==*this)
				{
				if		(Complex==rhs)
					RetVal.SetTrue();
				else if (Integer==rhs || Rational==rhs)
					RetVal.SetFalse();
				else
					RetVal.SetUnknown();
				return;
				}
			else if (Complex==*this)
				{
				if (Integer==rhs || Rational==rhs || Real==rhs)
					RetVal.SetFalse();
				else
					RetVal.SetUnknown();
				return;
				}
			}
		}
	RetVal.SetUnknown();
}

// The HasAsElement relation (easier to program than IsElementOf, which would be an Interface function)
// First returns true if it is *certain*
// Second uses TruthValue to handle UNKNOWN (which means "needs detailed analysis"); it should
// be needed only for proper classes, or partial domain intersections.
bool AbstractClass::HasAsElement(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/12/2002
	TruthValue RetVal;
	HasAsElement(rhs,RetVal);
	return RetVal.IsTrue();
}

bool AbstractClass::DoesNotHaveAsElement(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/12/2002
	TruthValue RetVal;
	HasAsElement(rhs,RetVal);
	return RetVal.IsFalse();
}

// #define FRANCI_WARY 1

void
AbstractClass::HasAsElement(const MetaConcept& rhs, TruthValue& RetVal) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	// actually, this can get quite interesting, since Bernays mentions that it is an axiom
	// that a set cannot have itself as an element.
	// Also, an abstractclass returns itself as its UltimateType.
	// However...Franci isn't quite that knowledgeable--yet.
	// Consider making this susceptible to array lookup.
#ifdef FRANCI_WARY
	LOG("Entering HasAsElement");
#endif
	RetVal.SetFalse();
	if (*this==NULLSet) return;
	if		(rhs.IsExactType(TruthValue_MC))
		{
		if (*this==TruthValues) RetVal.SetTrue();
		return;
		}
	else if (rhs.IsExactType(AbstractClass_MC))
		{
		if (static_cast<const AbstractClass&>(rhs).IsProperClass())
			return;
		if (*this==ClassAllSets && !rhs.IsUltimateType(NULL) && Superclass(*rhs.UltimateType()))
			RetVal.SetTrue();
		return;
		}
	else if (rhs.IsExactType(Variable_MC))
		{	// META: this assumes all top-level 1-ary constraints on the variable are shifted into the domain spec
		if (NULL!=rhs.UltimateType())
			Superclass(*rhs.UltimateType(),RetVal);
		return;
		}
	else if (rhs.IsExactType(IntegerNumeral_MC))
		{
		Superclass(Integer,RetVal);
		return;
		}
	else if (rhs.IsExactType(LinearInterval_MC))
		{
		//! \todo smarter test.  This won't work once sets have internal structure.
		Superclass(*rhs.UltimateType(),RetVal);
		return;
		}
	else if (rhs.IsExactType(LinearInfinity_MC))
		{	//! \todo: smarter test...really should be relying on natural total ordering, etc.
			//! <br> current implementation breaks on C<sup>#</sup>
		if (HasInfinity()) RetVal.SetTrue();
		return;
		}

	if (!Arg1.empty())
		{
		if 		(Arg1->IsExactType(LinearInterval_MC))
			{
#ifdef FRANCI_WARY
			LOG("Attempting LinearInterval handling of HasAsElement");
#endif
			static_cast<LinearInterval*>((MetaConcept*)Arg1)->HasAsElement(rhs,RetVal);
#ifdef FRANCI_WARY
			LOG("LinearInterval handling of HasAsElement OK");
#endif
			return;
			}
		else if (Arg1->IsExplicitConstant())
			{
			RetVal = (*Arg1==rhs);
			return;
			}
		}

	if (!rhs.IsUltimateType(NULL))
		{
		if (*this==ClassAdditionDefined)
			{
			rhs.UltimateType()->SupportsThisOperation(StdAddition_MC,RetVal);
			return;
			}
		else if (*this==ClassMultiplicationDefined)
			{
			rhs.UltimateType()->SupportsThisOperation(StdMultiplication_MC,RetVal);
			return;
			}
		else if (*this==ClassAdditionMultiplicationDefined)
			{
			rhs.UltimateType()->SupportsThisOperation(StdAddition_MC,RetVal);
			if (!RetVal.IsUnknown()) return;
			rhs.UltimateType()->SupportsThisOperation(StdMultiplication_MC,RetVal);
			return;
			}
		else if (rhs.UltimateType()->IntersectionWithIsNULLSet(*UltimateType()))
			return;
		}

	RetVal.SetUnknown();
}

#undef FRANCI_WARY

bool AbstractClass::IntersectWith(const AbstractClass& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2002
	if (Subclass(rhs)) return true;
	return SetToIntersection(*this,rhs);
}

// NOTE: only needs adaptation for two linear intervals
// Two non-collinear linear intervals will intersect in a singleton, if at all
bool
AbstractClass::SetToIntersection(const AbstractClass& lhs, const AbstractClass& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2005 
	if (NULLSet==lhs || NULLSet==rhs) return SetToThis(NULLSet);

	TruthValue RetVal;
	lhs.Subclass(rhs,RetVal);
	if 		(RetVal.IsTrue()) return SetToThis(lhs);
	else if (RetVal.IsFalse() && !lhs.Arg1.empty() && lhs.Arg1->IsExplicitConstant())
		return SetToThis(NULLSet);

	rhs.Subclass(lhs,RetVal);
	if 		(RetVal.IsTrue()) return SetToThis(rhs);
	else if (RetVal.IsFalse() && !rhs.Arg1.empty() && rhs.Arg1->IsExplicitConstant())
		return SetToThis(NULLSet);

	// TruthValues, and Subclass tests didn't go off: NULLSet
	if (lhs==TruthValues || rhs==TruthValues) return SetToThis(NULLSet);

	// Specials:
	if (   (lhs==ClassMultiplicationDefined && rhs==ClassAdditionDefined)
		|| (lhs==ClassAdditionDefined && rhs==ClassMultiplicationDefined))
		return SetToThis(ClassAdditionMultiplicationDefined);

	if (lhs.Arg1->IsExactType(LinearInterval_MC) && rhs.Arg1->IsExactType(LinearInterval_MC))
		{
		if 		(DestructiveIntersectable_LI & static_cast<LinearInterval*>((MetaConcept*)lhs.Arg1)->IntersectionUnionStatus(*static_cast<LinearInterval*>((MetaConcept*)rhs.Arg1)))
			return LinearInterval::MetaInterpretLinearInterval(Arg1,*static_cast<LinearInterval*>((MetaConcept*)lhs.Arg1),*static_cast<LinearInterval*>((MetaConcept*)rhs.Arg1),&LinearInterval::DestructiveIntersectWith);
		}

	//! \todo construct Intersection object and assign to Arg1, deleting proper name info if required
	return false;
}

bool AbstractClass::UnionWith(const AbstractClass& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2002
	if (rhs.Subclass(*this)) return true;
	return SetToUnion(*this,rhs);
}

bool AbstractClass::SetToUnion(const AbstractClass& lhs, const AbstractClass& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2005 
	if (lhs.Subclass(rhs)) return SetToThis(rhs);
	if (rhs.Subclass(lhs)) return SetToThis(lhs);

	if (!lhs.Arg1.empty() && !rhs.Arg1.empty())
		{
		if 		(lhs.Arg1->IsExactType(LinearInterval_MC))
			{	// LHS LinearInterval
			if 		(rhs.Arg1->IsExactType(LinearInterval_MC))
				{	// RHS LinearInterval
					// NOTE: substantial RAM efficiency advantage if Arg1==LHS.Arg1
				if (DestructiveMergeable_LI & static_cast<LinearInterval*>((MetaConcept*)lhs.Arg1)->IntersectionUnionStatus(*static_cast<LinearInterval*>((MetaConcept*)rhs.Arg1)))
					return LinearInterval::MetaInterpretLinearInterval(Arg1,*static_cast<LinearInterval*>((MetaConcept*)lhs.Arg1),*static_cast<LinearInterval*>((MetaConcept*)rhs.Arg1),&LinearInterval::DestructiveMergeWith);
				}
			else if (rhs.Arg1->IsExplicitConstant())
				// RHS ExplicitConstant
				return LinearInterval::MetaInterpretLinearInterval(Arg1,*static_cast<LinearInterval*>((MetaConcept*)lhs.Arg1),*rhs.Arg1,&LinearInterval::DestructiveExtendBy);
			}
		else if (lhs.Arg1->IsExplicitConstant())
			{	// LHS ExplicitConstant
			if 		(rhs.Arg1->IsExactType(LinearInterval_MC))
				// RHS LinearInterval
				return LinearInterval::MetaInterpretLinearInterval(Arg1,*static_cast<LinearInterval*>((MetaConcept*)rhs.Arg1),*lhs.Arg1,&LinearInterval::DestructiveExtendBy);
#if 0
			// don't need this now: this should be an Enumerated set
			else if (rhs.Arg1->IsExplicitConstant())
				{	// RHS ExplicitConstant
				};
#endif
			};
		}

	// TODO: construct Union object and assign to Arg1, deleting proper name info
	// if required
	return false;
}

// NOTE: only needs adaptation for two linear intervals
bool AbstractClass::IntersectionWithIsNULLSet(const AbstractClass& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/8/2005 
	// generic code
	if (NULLSet==*this || NULLSet==rhs) return true;

	TruthValue RetVal;
	Subclass(rhs,RetVal);
	if 		(RetVal.IsTrue()) return false;
	else if (RetVal.IsFalse() && !Arg1.empty() && Arg1->IsExplicitConstant())
		return true;

	rhs.Subclass(*this,RetVal);
	if 		(RetVal.IsTrue()) return false;
	else if (RetVal.IsFalse() && !rhs.Arg1.empty() && rhs.Arg1->IsExplicitConstant())
		return true;

	if (TruthValues==*this || TruthValues==rhs) return true;
	if (   (ClassAllSets==*this && rhs.Attributes1Bitmap & ProperSet)
		|| (ClassAllSets==rhs && this->Attributes1Bitmap & ProperSet))
		return true;

	if (Arg1->IsExactType(LinearInterval_MC) && rhs.Arg1->IsExactType(LinearInterval_MC))
		{
		if (Intersect_NULLset_LI & static_cast<LinearInterval*>((MetaConcept*)Arg1)->IntersectionUnionStatus(*static_cast<LinearInterval*>((MetaConcept*)rhs.Arg1)))
			return true;
		return false;
		}
	return false;
}

size_t AbstractClass::BasisClauseCount() const
{
	if (*this==NULLSet) return 0;		// NULLSet has no elements
	if (IsProperClass()) return 0;		// Proper classes are infinite
	if (*this==TruthValues) return 2;	// true or false (but 
	return 0;							// don't know how to do it yet
}

bool AbstractClass::DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const
{
	assert(BasisClauseCount()>Idx);
	assert(!dest);
	assert(*this!=TruthValues);	// handle this elsewhere
	FATAL(AlphaMiscallVFunction);
	return false;	
}

bool AbstractClass::SupportsThisOperation(ExactType_MC Operation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	TruthValue RetVal;
	SupportsThisOperation(Operation,RetVal);
	return RetVal.IsTrue();
}

bool AbstractClass::CompletelyFailsToSupportThisOperation(ExactType_MC Operation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/26/2002
	TruthValue RetVal;
	SupportsThisOperation(Operation,RetVal);
	return RetVal.IsFalse();
}

void AbstractClass::SupportsThisOperation(ExactType_MC Operation, TruthValue& RetVal) const
{	// MUTABLE
	// IMPLEMENTATION PARADIGM UNSTABLE
	RetVal.SetUnknown();
	if (*this!=TruthValues && *this!=NULLSet)
		{
		if (!Arg1.empty() && !Arg1->IsUltimateType(NULL))
			{
			if (   Arg1->IsExplicitConstant()
				|| Arg1->IsExactType(LinearInterval_MC))
				return Arg1->UltimateType()->SupportsThisOperation(Operation,RetVal);
			};

		if (*this!=ClassAllSets)
			{
			// Subclass(ClassAdditionDefined) recurses here, so cannot use that
			if (StdAddition_MC==Operation && *this!=ClassMultiplicationDefined)
				{
				if (   *this==ClassAdditionDefined					// algebraic: omnizero
					|| *this==ClassAdditionMultiplicationDefined	// algebraic: omnizero
					|| Superclass(Integer))		// integer 0 ok
					{
					RetVal.SetTrue();
					return;
					}
				};

			// Subclass(ClassMultiplicationDefined) recurses here, so cannot use that
			if (StdMultiplication_MC==Operation && *this!=ClassAdditionDefined)
				{
				if (   *this==ClassMultiplicationDefined			// algebraic: omnione
					|| *this==ClassAdditionMultiplicationDefined	// algebraic: omnione
					|| Superclass(Integer))		// integer 1 ok
					{
					RetVal.SetTrue();
					return;
					}
				}
			}
		}
	else{
		RetVal.SetFalse();
		return;
		}
}

bool
AbstractClass::CanCreateIdentityForOperation(ExactType_MC Operation) const
{	// MUTABLE
	if (!Arg1.empty() && !Arg1->IsUltimateType(NULL))
		{
		if (   Arg1->IsExplicitConstant()
			|| Arg1->IsExactType(LinearInterval_MC))
			return Arg1->UltimateType()->CanCreateIdentityForOperation(Operation);
		};

	if (SupportsThisOperation(Operation))
		{
		if (StdAddition_MC==Operation)
			{
			if (   *this==ClassAdditionDefined
				|| *this==ClassAdditionMultiplicationDefined)
				return true;	// algebraic: omnizero
			if (Superclass(Integer)) return true;	// integer 0 is appropriate
			};
		if (StdMultiplication_MC==Operation)
			{
			if (   *this==ClassMultiplicationDefined
				|| *this==ClassAdditionMultiplicationDefined)
				return true;	// algebraic: omnione
			if (Superclass(Integer)) return true;	// integer 1 is appropriate
			}
		}
	return false;
}

bool
AbstractClass::CreateIdentityForOperation(MetaConcept*& dest, ExactType_MC Operation) const
{	// MUTABLE
	assert(!dest);
	if (!Arg1.empty() && !Arg1->IsUltimateType(NULL))
		{
		if (   Arg1->IsExplicitConstant()
			|| Arg1->IsExactType(LinearInterval_MC))
			return Arg1->UltimateType()->CreateIdentityForOperation(dest,Operation);
		};

	if (SupportsThisOperation(Operation))
		{
		if (StdAddition_MC==Operation)
			{
			if (   *this==ClassAdditionDefined
				|| *this==ClassAdditionMultiplicationDefined)
				return true;						// algebraic: omnizero; caller must do it

			if (Superclass(Integer))	// create integer 0
				{
				dest = new(nothrow) IntegerNumeral();
				return NULL!=dest;
				}
			};
		if (StdMultiplication_MC==Operation)
			{
			if (   *this==ClassMultiplicationDefined
				|| *this==ClassAdditionMultiplicationDefined)
				return true;						// algebraic: omnione; caller must do it

			if (Superclass(Integer))	// create integer 1
				{
				dest = new(nothrow) IntegerNumeral((unsigned short)(1));
				return NULL!=dest;
				}
			}
		}
	return false;
}

void
AbstractClass::ConstructUpwardTopologicalRay(signed long EndPoint,bool Open,AbstractClass*& Domain) const
{
	if (Subclass(Real))
		{
		AbstractClass* IntervalDomain = NULL;
		MetaConcept* rhs = NULL;
		MetaConcept* lhs = NULL;
		MetaConcept* Target = NULL;
		try	{
			CopyInto(IntervalDomain);
			rhs = new SymbolicConstant(LinearInfinity_SC);
			lhs = new IntegerNumeral(EndPoint);
			Target = new LinearInterval(lhs, rhs, IntervalDomain, Open, true);
			Domain = new AbstractClass(Target);
			return;
			}
		catch(const bad_alloc&)
			{
			delete Target;
			delete lhs;
			delete rhs;
			delete IntervalDomain;
			UnconditionalRAMFailure();
			}
		};
	UnconditionalCallAssumptionFailure();
}

void
AbstractClass::ConstructUpwardTopologicalRay(MetaConcept*& EndPoint,bool Open,AbstractClass*& Domain) const
{
	if (Subclass(Real))
		{
		assert(!Domain);
		AbstractClass* IntervalDomain = NULL;
		MetaConcept* rhs = NULL;
		MetaConcept* Target = NULL;
		try	{
			CopyInto(IntervalDomain);
			rhs = new SymbolicConstant(LinearInfinity_SC);
			Target = new LinearInterval(EndPoint, rhs, IntervalDomain, Open, true);
			Domain = new AbstractClass(Target);
			return;
			}
		catch(const bad_alloc&)
			{
			delete Target;
			delete rhs;
			delete IntervalDomain;
			UnconditionalRAMFailure();
			}
		};
	UnconditionalCallAssumptionFailure();
}

void
AbstractClass::ConstructUpwardTopologicalRay(const MetaConcept& EndPoint,bool Open,AbstractClass*& Domain) const
{
	if (Subclass(Real))
		{
		assert(!Domain);
		AbstractClass* IntervalDomain = NULL;
		MetaConcept* rhs = NULL;
		MetaConcept* lhs = NULL;
		MetaConcept* Target = NULL;
		try	{
			CopyInto(IntervalDomain);
			rhs = new SymbolicConstant(LinearInfinity_SC);
			EndPoint.CopyInto(lhs);
			Target = new LinearInterval(lhs, rhs, IntervalDomain, Open, true);
			Domain = new AbstractClass(Target);
			return;
			}
		catch(const bad_alloc&)
			{
			delete Target;
			delete lhs;
			delete rhs;
			delete IntervalDomain;
			UnconditionalRAMFailure();
			}
		};
	UnconditionalCallAssumptionFailure();
}

void
AbstractClass::ConstructDownwardTopologicalRay(signed long EndPoint,bool Open,AbstractClass*& Domain) const
{
	if (Subclass(Real))
		{
		assert(!Domain);
		AbstractClass* IntervalDomain = NULL;
		MetaConcept* rhs = NULL;
		MetaConcept* lhs = NULL;
		MetaConcept* Target = NULL;
		try	{
			CopyInto(IntervalDomain);
			rhs = new SymbolicConstant(LinearInfinity_SC);
			rhs->SelfInverse(StdAddition_MC);
			lhs = new IntegerNumeral(EndPoint);
			Target = new LinearInterval(rhs, lhs, IntervalDomain, true, Open);
			Domain = new AbstractClass(Target);
			return;
			}
		catch(const bad_alloc&)
			{
			delete Target;
			delete lhs;
			delete rhs;
			delete IntervalDomain;
			UnconditionalRAMFailure();
			}
		};
	UnconditionalCallAssumptionFailure();
}

void
AbstractClass::ConstructDownwardTopologicalRay(MetaConcept*& EndPoint,bool Open,AbstractClass*& Domain) const
{
	if (Subclass(Real))
		{
		assert(!Domain);
		AbstractClass* IntervalDomain = NULL;
		MetaConcept* rhs = NULL;
		MetaConcept* Target = NULL;
		try	{
			CopyInto(IntervalDomain);
			rhs = new SymbolicConstant(LinearInfinity_SC);
			rhs->SelfInverse(StdAddition_MC);
			Target = new LinearInterval(rhs, EndPoint, IntervalDomain, true, Open);
			Domain = new AbstractClass(Target);
			return;
			}
		catch(const bad_alloc&)
			{
			delete Target;
			delete rhs;
			delete IntervalDomain;
			UnconditionalRAMFailure();
			}
		};
	UnconditionalCallAssumptionFailure();
}

void
AbstractClass::ConstructDownwardTopologicalRay(const MetaConcept& EndPoint,bool Open,AbstractClass*& Domain) const
{
	if (Subclass(Real))
		{
		assert(!Domain);
		AbstractClass* IntervalDomain = NULL;
		MetaConcept* rhs = NULL;
		MetaConcept* lhs = NULL;
		MetaConcept* Target = NULL;
		try	{
			CopyInto(IntervalDomain);
			rhs = new SymbolicConstant(LinearInfinity_SC);
			rhs->SelfInverse(StdAddition_MC);
			EndPoint.CopyInto(lhs);
			Target = new LinearInterval(rhs, lhs, IntervalDomain, true, Open);
			Domain = new AbstractClass(Target);
			return;
			}
		catch(const bad_alloc&)
			{
			delete Target;
			delete lhs;
			delete rhs;
			delete IntervalDomain;
			UnconditionalRAMFailure();
			}
		};
	UnconditionalCallAssumptionFailure();
}


bool
AbstractClass::StdMultiplicationCommutativeWithDomain(const AbstractClass& rhs) const
{	// MUTABLE
	if (Subclass(Complex) && rhs.Subclass(Complex))
		// _C_ is a field
		return true;
	return false;
}

bool
AbstractClass::CanBeRingAddition(ExactType_MC Operation) const
{	// MUTABLE
	// IMPLEMENTATION PARADIGM UNSTABLE
	if (   *this!=TruthValues
		&& *this!=NULLSet)
		{
		if (StdAddition_MC==Operation)
			{
			if (   *this==Integer
				|| *this==Rational
				|| *this==Real
				|| *this==Complex)
				return true;
			};
		};
	return false;
}

bool
AbstractClass::MayHaveElementsOfPeriodNUnderOp(size_t N, ExactType_MC Operation) const
{	// MUTABLE
	if (SupportsThisOperation(Operation))
		{
		if (StdAddition_MC==Operation)
			{
			if (*this==Integer || *this==Rational || *this==Real || *this==Complex)
				return (1>=N) ? true : false;
			// classes always return true
			};
		if (StdMultiplication_MC==Operation)
			{
			if (*this==Integer || *this==Rational || *this==Real)
				return (2>=N) ? true : false;				
			// classes, Complex numbers always return true
			};
		}
	return true;
}

bool
AbstractClass::IsRingUnderOperationPair(ExactType_MC CandidateAdd, ExactType_MC CandidateMult) const
{
	if (StdAddition_MC==CandidateAdd && StdMultiplication_MC==CandidateMult)
		{
		if (*this==Integer || *this==Rational || *this==Real || *this==Complex)
			return true;
		return false;
		};
	return false;
}

bool AbstractClass::HasStandardPartialOrdering() const
{	// MUTABLE
	if (HasStandardTotalOrdering()) return true;
	return false;
}

bool AbstractClass::HasStandardTotalOrdering() const
{	// MUTABLE
	if (Subclass(Real)) return true;
	return false;
}

bool
AbstractClass::Arg1IsAfterEndpointAlongVectorAB(const MetaConcept& Arg1, const MetaConcept& A, const MetaConcept& B) const
{	// TODO: VERIFY
	if (IsDenseUnderStandardTopology()) return false;
	//! \todo more sophisticated tests for local denseness
	if (*this==Integer)
		{
#if 0
		if (   Arg1.IsUltimateType(&Integer)
			&& A.IsUltimateType(&Integer)
			&& B.IsUltimateType(&Integer)
			&& A!=B)
			{	//! \todo implementation of standard partial ordering
			signed long Result;
			if (Arg1.SmallDifference(B,Result))
				{
				if (A<B)
					{
					if (1==Result)
						return true;
					}
				else{
					if (-1==Result)
						return true;
					}
				}
			}
#else
		if (   Arg1.IsExactType(IntegerNumeral_MC)
			&& A.IsExactType(IntegerNumeral_MC)
			&& B.IsExactType(IntegerNumeral_MC)
			&& Arg1.IsUltimateType(&Integer)
			&& A.IsUltimateType(&Integer)
			&& B.IsUltimateType(&Integer)
			&& A!=B)
			{
			signed long Result;
			if 		(*static_cast<const IntegerNumeral*>(&A)<*static_cast<const IntegerNumeral*>(&B))
				{	// A<B
				if (   static_cast<const IntegerNumeral*>(&Arg1)->SmallDifference(*static_cast<const IntegerNumeral*>(&B),Result)
					&& 1==Result)
					return true;
				}
			else{	// we have trichotomy: A<B, A==B, or A>B, so A>B
				if (   static_cast<const IntegerNumeral*>(&B)->SmallDifference(*static_cast<const IntegerNumeral*>(&Arg1),Result)
					&& 1==Result)
					return true;
				}
			}
#endif
		}
	return false;
}

bool
ForceVarUltimateType(MetaConcept*& Target,const AbstractClass* TargetType)
{
	assert(Target);
	if (   !Target->IsUltimateType(TargetType)			// TargetType non-NULL after this
		&& !Target->ForceUltimateType(TargetType)
		&& (   !Target->IsPotentialVarName()
			|| !ImproviseVar(Target,TargetType)))
		return false;
	return true;
}

// misnamed specialization of above: force auto-fails
bool ForceUltimateTypeTruthValues(MetaConcept*& Target)
{
	assert(Target);
	if (   !Target->IsUltimateType(&TruthValues)
		&& (   !Target->IsPotentialVarName()
			|| !ImproviseVar(Target,&TruthValues)))
		return false;
	return true;
}

// default initialization -- HIGHLY MUTABLE
const AbstractClass TruthValues(Reserved_TruthValues);
const AbstractClass Integer(Reserved_Integer);
const AbstractClass Rational(Reserved_Rational);
const AbstractClass Real(Reserved_Real);
const AbstractClass Complex(Reserved_Complex);
const AbstractClass NULLSet(Reserved_NULLSet);
const AbstractClass ClassAllSets(Reserved_ClassAllSets);
const AbstractClass ClassAdditionDefined(Reserved_ClassAdditionDefined);
const AbstractClass ClassMultiplicationDefined(Reserved_ClassMultiplicationDefined);
const AbstractClass ClassAdditionMultiplicationDefined(Reserved_ClassAdditionMultiplicationDefined);

#if 0
static void
FlushDynamicSets(void)
{
}
#endif

void
InitializeDefaultSetsAndClasses(void)
// HIGHLY MUTABLE
{	// using const_casts because of initialization
	const_cast<AbstractClass&>(TruthValues).Set_IsProperSet();
	const_cast<AbstractClass&>(Integer).Set_IsProperSet();
	const_cast<AbstractClass&>(Integer).Set_IsDiscreteUnderStandardTopology();
	const_cast<AbstractClass&>(Integer).Set_TopologicalCompletesWithInfinity();
	const_cast<AbstractClass&>(Rational).Set_IsProperSet();
	const_cast<AbstractClass&>(Rational).Set_IsDenseUnderStandardTopology();
	const_cast<AbstractClass&>(Rational).Set_TopologicalCompletesWithInfinity();
	const_cast<AbstractClass&>(Real).Set_IsProperSet();
	const_cast<AbstractClass&>(Real).Set_IsDenseUnderStandardTopology();
	const_cast<AbstractClass&>(Real).Set_TopologicalCompletesWithInfinity();
	const_cast<AbstractClass&>(Complex).Set_IsProperSet();
	const_cast<AbstractClass&>(Complex).Set_IsDenseUnderStandardTopology();
	const_cast<AbstractClass&>(Complex).Set_TopologicalCompletesWithInfinity();
	const_cast<AbstractClass&>(NULLSet).Set_IsProperSet();
	const_cast<AbstractClass&>(ClassAllSets).Set_IsProperClass();
	const_cast<AbstractClass&>(ClassAllSets).Set_ContainsInfinity();
	const_cast<AbstractClass&>(ClassAdditionDefined).Set_IsProperClass();
	const_cast<AbstractClass&>(ClassAdditionDefined).Set_ContainsInfinity();
	const_cast<AbstractClass&>(ClassMultiplicationDefined).Set_IsProperClass();
	const_cast<AbstractClass&>(ClassMultiplicationDefined).Set_ContainsInfinity();
	const_cast<AbstractClass&>(ClassAdditionMultiplicationDefined).Set_IsProperClass();
	const_cast<AbstractClass&>(ClassAdditionMultiplicationDefined).Set_ContainsInfinity();

	assert(TruthValues.SyntaxOK());
	assert(Integer.SyntaxOK());
	assert(Rational.SyntaxOK());
	assert(Real.SyntaxOK());
	assert(Complex.SyntaxOK());
	assert(NULLSet.SyntaxOK());
	assert(ClassAllSets.SyntaxOK());
	assert(ClassAdditionDefined.SyntaxOK());
	assert(ClassMultiplicationDefined.SyntaxOK());
	assert(ClassAdditionMultiplicationDefined.SyntaxOK());

	// register cleanup function
// 	atexit(FlushDynamicSets);
}
