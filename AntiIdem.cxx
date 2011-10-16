// AntiIdem.cxx
// core implementation of antiidempotency
// this has to be memory-safe

#include "MetaCon1.hxx"
#include "Class.hxx"
#include "MetaCon3.hxx"
#include "Equal.hxx"
#include "Variable.hxx"
#include "LowRel.hxx"

bool IsAntiIdempotentTo(const MetaConcept& lhs, const MetaConcept& rhs)
{
	assert(lhs.IsUltimateType(&TruthValues));
	assert(rhs.IsUltimateType(&TruthValues));
	if (typeid(lhs)!=typeid(rhs)) return false;
	return lhs.isAntiIdempotentTo(rhs);
}

static bool detectAntiIdempotentAuxANDOR(const MetaConnective& lhs, const MetaConnective& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/5/1999
	if (LogicalAND_MC+LogicalOR_MC==lhs.ExactType()+rhs.ExactType())
		return lhs.OrderIndependentPairwiseRelation(rhs,IsAntiIdempotentTo);
	return false;
}

static bool detectAntiIdempotentAuxNANDNOR(const MetaConnective& lhs, const MetaConnective& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/9/1999
	UnconditionalDataIntegrityFailure();
	return false;
}

static bool detectAntiIdempotentAuxIFFXOR(const MetaConnective& lhs, const MetaConnective& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/27/1999
	if (LogicalAND_MC+LogicalNAND_MC==lhs.ExactType()+rhs.ExactType())
		return lhs.OrderIndependentPairwiseRelation(rhs,AreSyntacticallyEqual);
	if (   2==lhs.fast_size() && rhs.IsExactType(lhs.ExactType())
		&& 2==rhs.fast_size())
		return lhs.OneEqualOneIdempotentFirstTwoArgs(rhs);
	return false;
}

static bool detectAntiIdempotentAuxNIFFNXOR(const MetaConnective& lhs, const MetaConnective& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/5/1999
	if (LogicalAND_MC+LogicalNAND_MC==lhs.ExactType()+rhs.ExactType())
		return lhs.OrderIndependentPairwiseRelation(rhs,AreSyntacticallyEqual);
	return false;
}

typedef bool MetaConnectiveBinaryRelation(const MetaConnective& lhs, const MetaConnective& rhs);

MetaConnectiveBinaryRelation* const detectAntiIdempotentAux[]
  =	{
	detectAntiIdempotentAuxANDOR,
	detectAntiIdempotentAuxANDOR,
	detectAntiIdempotentAuxIFFXOR,
	detectAntiIdempotentAuxIFFXOR,
	detectAntiIdempotentAuxNIFFNXOR,
	detectAntiIdempotentAuxNIFFNXOR,
	detectAntiIdempotentAuxNANDNOR,
	detectAntiIdempotentAuxNANDNOR
	};
BOOST_STATIC_ASSERT(StrictBound_MCM==STATIC_SIZE(detectAntiIdempotentAux));

bool MetaConnective::isAntiIdempotentTo(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	return (detectAntiIdempotentAux[array_index()])(*this,static_cast<const MetaConnective&>(rhs));
}

bool Variable::isAntiIdempotentTo(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/1/2001
	const Variable& VR_rhs = static_cast<const Variable&>(rhs);
	if (   NULL!=Arg1
		&& NULL!=VR_rhs.Arg1
		&& *Arg1==*VR_rhs.Arg1
		&& (LogicalNegated_VF & MultiPurposeBitmap)^(LogicalNegated_VF & VR_rhs.MultiPurposeBitmap))
		return true;
	return false;
}

bool EqualRelation::isAntiIdempotentTo(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/14/2000
	assert(!ArgArray.empty());
	const EqualRelation& VR_rhs = static_cast<const EqualRelation&>(rhs);
	assert(!VR_rhs.ArgArray.empty());
	if (2==fast_size() && !ArgArray[0]->IsExactType(LinearInterval_MC) && !ArgArray[1]->IsExactType(LinearInterval_MC))
		return rhs.ExactType()==(ALLDISTINCT_MC+ALLEQUAL_MC)-ExactType()
				&& OrderIndependentPairwiseRelation(VR_rhs,AreSyntacticallyEqual);
	else if (rhs.ExactType()==(NOTALLEQUAL_MC+ALLEQUAL_MC)-ExactType())
		{
		if (IsExactType(DISTINCTFROMALLOF_MC) || IsExactType(EQUALTOONEOF_MC))
			{
			if (*ArgArray[0]!=*VR_rhs.ArgArray[0]) return false;
			};
		return OrderIndependentPairwiseRelation(VR_rhs,AreSyntacticallyEqual);
		};
	return false;
}

