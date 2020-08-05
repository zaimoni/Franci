// Quantify.cxx
// implementation of MetaQuantifier

#include "Class.hxx"
#include "Quantify.hxx"
#include "Keyword1.hxx"

// NOTE: we exploit the fact that Franci's memory blocks are null-terminated.
unsigned long MetaQuantifier::NextID = 0;

MetaQuantifier::MetaQuantifier(const char* Name, const AbstractClass* Domain, MetaQuantifierMode CreationMode)
:	MetaConceptWith1Arg((ExactType_MC)(CreationMode+ForAll_MC)),
	ID(NextID++),		// should not reach ULONG_MAX, and will roll over if it does
	VariableName((!Name || !*Name) ? 0 : ZAIMONI_LEN_WITH_NULL(strlen(Name))),
	Bitmap1(None_MQ)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/18/2006
	if (!VariableName.empty()) strcpy(VariableName,Name);
	if (Domain) Domain->CopyInto(Arg1);
}

MetaQuantifier::MetaQuantifier(const char* Name, const AbstractClass* Domain, MetaQuantifierMode CreationMode, bool Improvised)
:	MetaConceptWith1Arg((ExactType_MC)(CreationMode+ForAll_MC)),
	ID(NextID++),		// should not reach ULONG_MAX, and will roll over if it does
	VariableName((!Name || !*Name) ? 0 : ZAIMONI_LEN_WITH_NULL(strlen(Name))),
	Bitmap1((Improvised)	? Improvised_MQ
							: None_MQ)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/18/2006
	if (!VariableName.empty()) strcpy(VariableName,Name);
	if (Domain) Domain->CopyInto(Arg1);
}

MetaQuantifier& MetaQuantifier::operator=(const MetaQuantifier& src)
{	// FORMALLY CORRECT: 11/26/2006
	if (VariableName.size()>=src.VariableName.size())
		{
		this->MetaConceptWith1Arg::operator=(src);
		VariableName = src.VariableName;
		}
	else{
		autovalarray_ptr_throws<char> Tmp(src.VariableName);
		this->MetaConceptWith1Arg::operator=(src);
		Tmp.MoveInto(VariableName);
		}
	Bitmap1 = src.Bitmap1;
	ID = src.ID;
	return *this;
}

//  Type ID functions
const AbstractClass* MetaQuantifier::UltimateType() const
{return static_cast<AbstractClass* const>((MetaConcept*)Arg1);}

// #define FRANCI_WARY 1

bool MetaQuantifier::ForceUltimateType(const AbstractClass* const rhs)
{	// FORMALLY CORRECT: 2020-08-04
	if (NULL==rhs) return true;
	if (NULLSet==*rhs) return false;
	if (NULL==Arg1) {	// original is Free_MC
		try	{ // change to ThereIs_MC with given domain
			Arg1 = new AbstractClass(*rhs);
			SetExactType((ExactType_MC)(ThereIs_MC));
			return true;
		} catch(const bad_alloc&) {
			return false;
		};
	};
#ifdef FRANCI_WARY
	LOG("Attempting MetaQuantifier::ForceUltimateType Subclass");
#endif
	if (UltimateType()->Subclass(*rhs)) return true;
#ifdef FRANCI_WARY
	LOG("Subclass OK");
#endif
	// Take intersection: if non-null, use *that* domain.
	AbstractClass Tmp;
	if (Tmp.SetToIntersection(*UltimateType(),*rhs) && NULLSet!=Tmp) {
#ifdef FRANCI_WARY
		LOG("SetToIntersection OK, succeeded");
#endif
		try	{
			Tmp.CopyInto(Arg1);
			SetExactType((ExactType_MC)(ThereIs_MC));
			return true;
		} catch(const bad_alloc&) {
			return false;
		};
	};
#ifdef FRANCI_WARY
	LOG("SetToIntersection OK, failed");
#endif
	return false;
}

#undef FRANCI_WARY

// Syntactical equality and inequality
bool MetaQuantifier::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: 6/19/2000
	if (   EqualAux(static_cast<const MetaQuantifier&>(rhs))
		&& ID==static_cast<const MetaQuantifier&>(rhs).ID
		&& !strcmp(VariableName,static_cast<const MetaQuantifier&>(rhs).VariableName))
		return true;
	return false;
}

bool MetaQuantifier::InternalDataLTAux(const MetaConcept& rhs) const
{
	if (!MetaConceptWith1Arg::EqualAux2(rhs)) return MetaConceptWith1Arg::InternalDataLTAux(rhs);
	return static_cast<const MetaQuantifier&>(rhs).LexicalGT(*this);
}

bool MetaQuantifier::_IsExplicitConstant() const
{return false;}

bool MetaQuantifier::EqualAux(const MetaQuantifier& rhs) const
{	// FORMALLY CORRECT: 6/19/2000
	if (Arg1.empty())
		{
		if (rhs.Arg1.empty()) return true;
		}
	else{
		if (!rhs.Arg1.empty() && *Arg1==*rhs.Arg1) return true;
		};
	return false;
}

//  Evaluation functions
bool MetaQuantifier::CanEvaluate() const {return false;}
bool MetaQuantifier::CanEvaluateToSameType() const {return false;}

bool MetaQuantifier::SyntaxOK() const
{	// FORMALLY CORRECT: 12/31/1998
	if (!VariableName.empty())
		{
		if (IsExactType(Free_MC))
			{
			if (Arg1.empty()) return true;
			}
		else{
			if (!Arg1.empty() && Arg1->IsExactType(AbstractClass_MC))
				return true;
			}
		}
	return false;
}

bool MetaQuantifier::Evaluate(MetaConcept*& dest) {return false;}
bool MetaQuantifier::DestructiveEvaluateToSameType() {return false;}

bool
MetaQuantifier::MetaConceptPtrUsesThisQuantifier(const MetaConcept* lhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
	if (NULL!=lhs)
		{
		if (*this==*lhs || lhs->UsesQuantifierAux(*this))
			return true;
		}
	return false;
}

size_t MetaQuantifier::BasisClauseCount() const
{
	if (NULL==Arg1) return 0;
	if (!IsExactType(ThereIs_MC)) return 0;
	return Arg1->BasisClauseCount();
}

//  Free vars are > all nonfree vars, and variable name order tie-breaks.
//  Nonfree vars: forall-types never compare with thereis-types; within, use name to tie-break
bool MetaQuantifier::LexicalGT(const MetaQuantifier& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/8/1999
	ExactType_MC LHSType = ExactType();
	ExactType_MC RHSType = rhs.ExactType();
	switch(5*LHSType+RHSType-6*ForAll_MC)
	{
	case 5*Free_MC+ForAll_MC-6*ForAll_MC:
	case 5*Free_MC+ThereIs_MC-6*ForAll_MC:
	case 5*Free_MC+ForAllNot_MC-6*ForAll_MC:
	case 5*Free_MC+ThereIsNot_MC-6*ForAll_MC:
		return true;
	case 5*ForAll_MC+ForAll_MC-6*ForAll_MC:
	case 5*ForAll_MC+ThereIsNot_MC-6*ForAll_MC:
	case 5*ThereIsNot_MC+ForAll_MC-6*ForAll_MC:
	case 5*ThereIsNot_MC+ThereIsNot_MC-6*ForAll_MC:
	case 5*ThereIs_MC+ThereIs_MC-6*ForAll_MC:
	case 5*ThereIs_MC+ForAllNot_MC-6*ForAll_MC:
	case 5*ForAllNot_MC+ThereIs_MC-6*ForAll_MC:
	case 5*ForAllNot_MC+ForAllNot_MC-6*ForAll_MC:
		// the following compares are domain lexical, then var-name lexical
		// forall-forall, thereis-thereis
		{
		if (Arg1->InternalDataLT(*rhs.Arg1)) return false;
		if (rhs.Arg1->InternalDataLT(*Arg1)) return true;
		}
	case 5*Free_MC+Free_MC-6*ForAll_MC:
		// the following compares are var-name lexical:
		// free-free
		return 0<strcmp(VariableName,rhs.VariableName);
	default:
		return false;
	}
}

bool MetaQuantifier::ChangeVarNameTo(const char* NewVarName)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
	if (NULL==NewVarName || '\0'== *NewVarName) return false;
	const size_t name_len = strlen(NewVarName);
	if (!VariableName.Resize(ZAIMONI_LEN_WITH_NULL(name_len)))
		return false;
	strcpy(VariableName,NewVarName);
	return true;
}

#ifndef NDEBUG
void
SystemTest_MetaQuantifier(void)	// low-level test of MetaQuantifier
{
#if 0
	REPORT(true,"Testing MetaQuantifier");
	{
	MetaQuantifier Tmp1("A1",&TruthValues,ForAll_MQM);
	REPORT(!Tmp1.IsUltimateType(&TruthValues),"ERROR: MetaQuantifier(,,)");
	MetaQuantifier Tmp2("A1",&TruthValues,ThereIs_MQM);
	MetaQuantifier Tmp4("A1",&TruthValues,ForAllNot_MQM);
	MetaQuantifier Tmp5("A1",&TruthValues,ThereIsNot_MQM);
	MetaQuantifier Tmp8("A1",NULL,Free_MQM);
	{
	MetaQuantifier Tmp3("A1",&TruthValues,Free_MQM);
	MetaQuantifier Tmp6("A1",NULL,ForAll_MQM);
	REPORT(!Tmp6.IsUltimateType(NULL),"ERROR: MetaQuantifier(,,)");
	MetaQuantifier Tmp7("A1",NULL,ThereIs_MQM);
	MetaQuantifier Tmp9("A1",NULL,ForAllNot_MQM);
	MetaQuantifier Tmp0("A1",NULL,ThereIsNot_MQM);

	// SyntaxOK check
	REPORT(!Tmp1.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(!Tmp2.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(!Tmp4.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(!Tmp5.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(!Tmp8.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(Tmp3.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(Tmp6.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(Tmp7.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(Tmp9.SyntaxOK(),"ERROR: SyntaxOK");
	REPORT(Tmp0.SyntaxOK(),"ERROR: SyntaxOK");
	}	// undeclare syntactically incorrect versions
	//! \todo test ==
	}
	REPORT(true,"I'm finished testing MetaQuantifier.");
#endif
}
#endif
