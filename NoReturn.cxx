// NoReturn.cxx
// functions that don't return (and thus should be isolated from the rest of the code)

#include "Class.hxx"
#include "LowRel.hxx"

// MetaConcept
bool
MetaConcept::EvaluateFunction(MetaConcept** const& ArgValList, unsigned long*& ArgList, MetaConcept*& Result)
{	// FORMALLY CORRECT: 9/6/2001
	LOG(name());
	FATAL_CODE("EvaluateFunction must be defined",3);
	return false;
}

void MetaConcept::SelfLogicalNOT()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/31/2001
	LOG(name());
	FATAL_CODE((IsUltimateType(&TruthValues)) ? "SelfLogicalNOT must be defined" : "SelfLogicalNOT must not be defined",3);
}

bool MetaConcept::isAntiIdempotentTo(const MetaConcept& rhs) const
{
	LOG(name());
	FATAL_CODE((IsUltimateType(&TruthValues)) ? "isAntiIdempotentTo must be defined" : "isAntiIdempotentTo must not be defined",3);
	return false;
}

bool MetaConcept::CanAmplifyThisClause(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/22/2003
	LOG(name());
	FATAL_CODE("CanAmplifyThisClause must not be defined",3);
	return false;
}

bool MetaConcept::AmplifyThisClause(MetaConcept*& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/22/2003
	LOG(name());
	FATAL_CODE("AmplifyThisClause must not be defined",3);
	return false;
}

bool MetaConcept::DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/6/2000
	LOG(name());
	FATAL_CODE("DirectCreateBasisClauseIdx must not be defined",3);
	return false;
}

bool MetaConcept::MakesLHSImplyRHS(const MetaConcept& lhs, const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/3/2000
	LOG(name());
	FATAL_CODE("MakesLHSImplyRHS must not be defined",3);
	return false;
}

bool MetaConcept::ValidLHSForMakesLHSImplyRHS(const MetaConcept& lhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/3/2000
	LOG(name());
	FATAL_CODE("ValidLHSForMakesLHSImplyRHS must not be defined",3);
	return false;
}


bool MetaConcept::ValidRHSForMakesLHSImplyRHS(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/3/2000
	LOG(name());
	FATAL_CODE("ValidRHSForMakesLHSImplyRHS must not be defined",3);
	return false;
}

bool MetaConcept::DirectCreateANDFactorIdx(size_t Idx, MetaConcept*& dest) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2003
	LOG(name());
	FATAL_CODE("DirectCreateANDFactorIdx must not be defined",3);
	return false;
}

bool MetaConcept::AugmentHypothesis(MetaConcept*& Hypothesis) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/11/2001
	LOG(name());
	FATAL_CODE("AugmentHypothesis must not be defined",3);
	return false;
}

bool
MetaConcept::ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/13/2000
	LOG(name());
	FATAL_CODE(HasAnnihilatorKey() ? "ThisIsAnnihilatorKey must be defined" : "ThisIsAnnihilatorKey must not be defined",3);
	return false;
}

// Global.hxx
// these aren't strictly necessary, but they improve code size by hiding a parameter
void UnconditionalDataIntegrityFailure()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/18/1999
	FATAL_CODE(AlphaDataIntegrity,3);
}

void UnconditionalRAMFailure()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/18/1999
	FATAL_CODE(RAMFailure,3);
}

void UnconditionalCallAssumptionFailure()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/18/1999
	FATAL_CODE(AlphaCallAssumption,3);
}


