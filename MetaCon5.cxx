// MetaCon5.cxx
// implementation of MetaConceptWith1Args

#include "MetaCon5.hxx"
#include "Quantify.hxx"
#include "LowRel.hxx"

void MetaConceptWith1Arg::MoveIntoAux(MetaConceptWith1Arg& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2002
	dest.MetaConcept::operator=(*this);
	Arg1.MoveInto(dest.Arg1);
}

// FORMALLY CORRECT: Kenneth Boyd, 12/25/1998
#define ARGN_BODY return (0 == n) ? (MetaConcept*)Arg1 : NULL;

STANDARD_DECLARE_ARGN(MetaConceptWith1Arg,ARGN_BODY)

#undef ARGN_BODY

bool MetaConceptWith1Arg::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: 2020-05-23
	if (Arg1.empty()) return true;
	const MetaConceptWith1Arg& VR_rhs = static_cast<const MetaConceptWith1Arg&>(rhs);
	if (VR_rhs.Arg1.empty()) return false;
	return Arg1->InternalDataLT(*VR_rhs.Arg1);
}

bool MetaConceptWith1Arg::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	const MetaConceptWith1Arg& VR_rhs = static_cast<const MetaConceptWith1Arg&>(rhs);
	if (Arg1.empty()) return VR_rhs.Arg1.empty();
	if (!VR_rhs.Arg1.empty()) return *Arg1==*VR_rhs.Arg1;
	return false;
}

bool MetaConceptWith1Arg::_IsExplicitConstant() const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/10/2000
	return Arg1.empty() || Arg1->IsExplicitConstant();
}

void MetaConceptWith1Arg::_forceStdForm()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/2000
	if (!Arg1.empty()) Arg1->ForceStdForm();
}

void MetaConceptWith1Arg::ConvertVariableToCurrentQuantification(MetaQuantifier& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/20/1999
	if (!Arg1.empty())
		Arg1->ConvertVariableToCurrentQuantification(src);
}

bool MetaConceptWith1Arg::UsesQuantifierAux(const MetaQuantifier& x) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/20/1999
	return x.MetaConceptPtrUsesThisQuantifier(Arg1);
}

