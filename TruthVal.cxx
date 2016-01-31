// TruthVal.cxx
// implementation of TruthValue

#include "TruthVal.hxx"

#ifdef ALPHA_TRUTHVAL
#else // ALPHA_TRUTHVAL
#include "Class.hxx"
#include "Keyword1.hxx"

#include <boost/functional.hpp>

static const char NegatedTVal[4] =	{(char)(TruthValue::Contradiction),
									 (char)(TruthValue::False),
									 (char)(TruthValue::True),
									 (char)(TruthValue::Unknown)};

// OPTIMIZATION NOTES
// This is an end-use type.  It is VERY STABLE: except for the functions that should use TruthValueTable,
// routines have been unaltered since creation.  ASM with backup sourcecode is reasonable.

static size_t TValFromString(const char* Text)
{	// FORMALLY CORRECT: 6/16/1999
	if (NULL!=Text)
		{
		size_t i = 0;
		do	if (0==strcmp(Text,TruthValueNames[i])) return i;
		while(STATIC_SIZE(TruthValueNames)> ++i);
		}
	return STATIC_SIZE(TruthValueNames);
}

bool TruthValue::IsLegalTValString(const char* Text)
{	// FORMALLY CORRECT: 9/4/2006
	return STATIC_SIZE(TruthValueNames)>TValFromString(Text);
}

// only called from UnparsedText::Evaluate(...); guard clause is IsLegalTValString
bool TruthValue::ConvertToTruthValue(MetaConcept*& dest,const char* Text)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/4/2006
	assert(NULL==dest);
	assert(IsLegalTValString(Text));
	unsigned char TValIdx = TValFromString(Text);
	assert(STATIC_SIZE(TruthValueNames)>TValIdx);
	dest = new(nothrow) TruthValue((Flags)TValIdx);
	return NULL!=dest;
}

bool TruthValue::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	return TVal==static_cast<const TruthValue&>(rhs).TVal;
}

bool TruthValue::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	return TVal<static_cast<const TruthValue&>(rhs).TVal;
}

const AbstractClass* TruthValue::UltimateType() const {return &TruthValues;}
bool TruthValue::CanEvaluate() const {return false;}
bool TruthValue::CanEvaluateToSameType() const {return false;}
bool TruthValue::SyntaxOK() const {return true;}
bool TruthValue::Evaluate(MetaConcept*& dest) {return false;}
bool TruthValue::DestructiveEvaluateToSameType() {return false;}

// FORMALLY CORRECT: Kenneth Boyd, 6/17/1999
void TruthValue::SelfLogicalNOT() {TVal = NegatedTVal[TVal];}

bool TruthValue::isAntiIdempotentTo(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/11/1999
	return static_cast<const TruthValue&>(rhs).TVal==NegatedTVal[TVal];
}

// does a low-level system test
#ifndef NDEBUG
void
SystemTest_TruthValue(void)	// low-level test of TruthValue
{	//! \todo IMPLEMENT
// TruthValue block
	{
	TruthValue Tmp;

	// 0-ary constructor test
	REPORT(!Tmp.IsUnknown(),"ERROR: Truthvalue() or IsUnknown()");
	// Set__ tests
	Tmp.SetContradiction();
	REPORT(!Tmp.IsContradiction(),"ERROR: SetContradiction() or IsContradiction()");
	REPORT(Tmp.CouldBeTrue(),"ERROR: SetContradiction() or CouldBeTrue()");
	REPORT(Tmp.CouldBeFalse(),"ERROR: SetContradiction() or CouldBeFalse()");
	Tmp.SetTrue();
	REPORT(!Tmp.IsTrue(),"ERROR: SetTrue() or IsTrue()");
	REPORT(!Tmp.CouldBeTrue(),"ERROR: SetTrue() or CouldBeTrue()");
	REPORT(Tmp.CouldBeFalse(),"ERROR: SetTrue() or CouldBeFalse()");
	Tmp.SetFalse();
	REPORT(!Tmp.IsFalse(),"ERROR: SetFalse() or IsFalse()");
	REPORT(Tmp.CouldBeTrue(),"ERROR: SetFalse() or CouldBeTrue()");
	REPORT(!Tmp.CouldBeFalse(),"ERROR: SetFalse() or CouldBeFalse()");
	Tmp.SetUnknown();
	REPORT(!Tmp.IsUnknown(),"ERROR: SetUnknown() or IsUnknown()");
	REPORT(!Tmp.CouldBeTrue(),"ERROR: SetUnknown() or CouldBeTrue()");
	REPORT(!Tmp.CouldBeFalse(),"ERROR: SetUnknown() or CouldBeFalse()");

	// Force__ tests
	Tmp.SetContradiction();
	Tmp.ForceTrue();
	REPORT(!Tmp.IsContradiction(),"ERROR: ForceTrue()");
	Tmp.SetContradiction();
	Tmp.ForceFalse();
	REPORT(!Tmp.IsContradiction(),"ERROR: ForceFalse()");
	Tmp.SetTrue();
	Tmp.ForceTrue();
	REPORT(!Tmp.IsTrue(),"ERROR: ForceTrue()");
	Tmp.SetTrue();
	Tmp.ForceFalse();
	REPORT(!Tmp.IsContradiction(),"ERROR: ForceFalse()");
	Tmp.SetFalse();
	Tmp.ForceTrue();
	REPORT(!Tmp.IsContradiction(),"ERROR: ForceTrue()");
	Tmp.SetFalse();
	Tmp.ForceFalse();
	REPORT(!Tmp.IsFalse(),"ERROR: ForceFalse()");
	Tmp.SetUnknown();
	Tmp.ForceTrue();
	REPORT(!Tmp.IsTrue(),"ERROR: ForceTrue()");
	Tmp.SetUnknown();
	Tmp.ForceFalse();
	REPORT(!Tmp.IsFalse(),"ERROR: ForceFalse()");

	// SelfLogicalNOT
	Tmp.SetContradiction();
	Tmp.SelfLogicalNOT();
	REPORT(!Tmp.IsContradiction(),"ERROR: SelfLogicalNOT()");
	Tmp.SetTrue();
	Tmp.SelfLogicalNOT();
	REPORT(!Tmp.IsFalse(),"ERROR: SelfLogicalNOT()");
	Tmp.SetFalse();
	Tmp.SelfLogicalNOT();
	REPORT(!Tmp.IsTrue(),"ERROR: SelfLogicalNOT()");
	Tmp.SetUnknown();
	Tmp.SelfLogicalNOT();
	REPORT(!Tmp.IsUnknown(),"ERROR: SelfLogicalNOT()");
	}
}
#endif

#endif // ALPHA_TRUTHVAL
