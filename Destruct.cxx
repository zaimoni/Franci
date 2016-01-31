// Destruct.cxx
// implementation for most MetaConcept-based destructors
// migrate them here as they mature (code locality vs. G++)

#include "MetaCon3.hxx"
#include "Equal.hxx"
#include "StdAdd.hxx"
#include "StdMult.hxx"
#include "GCF.hxx"
#include "ClauseN.hxx"
#include "PhraseN.hxx"
#include "QState.hxx"
#include "SeriesOp.hxx"
#include "Combin1.hxx"

#include "Clause2.hxx"
#include "Phrase2.hxx"
#include "Interval.hxx"

#include "Phrase1.hxx"
#include "Class.hxx"
#include "Quantify.hxx"
#include "Variable.hxx"

#include "TruthVal.hxx"
#include "Integer1.hxx"
#include "Unparsed.hxx"
#include "SymConst.hxx"

// 0-ary
UnparsedText::~UnparsedText()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
}

IntegerNumeral::~IntegerNumeral()
{	// FORMALLY CORRECT: 7/12/2007, Kenneth Boyd
}

#ifndef ALPHA_TRUTHVAL
TruthValue::~TruthValue()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/6/1998
}
#endif

Variable::~Variable()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2006
}

SymbolicConstant::~SymbolicConstant()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/8/2002
}

MetaConceptZeroArgs::~MetaConceptZeroArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/6/1998
}

// 1-ary
Phrase1Arg::~Phrase1Arg()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/12/1999
}

MetaQuantifier::~MetaQuantifier()
{	// FORMALLY CORRECT: 12/31/1998
}

AbstractClass::~AbstractClass()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
}

MetaConceptWith1Arg::~MetaConceptWith1Arg()
{	// FORMALLY CORRECT: Kenneth Boyd, 12/25/1998
}

// 2-ary
Clause2Arg::~Clause2Arg()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/21/1999
}

Phrase2Arg::~Phrase2Arg()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/13/2000
}

LinearInterval::~LinearInterval()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/31/2005
}

MetaConceptWith2Args::~MetaConceptWith2Args()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/2/2005
}

// n-ary
ClauseNArg::~ClauseNArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/8/1999
}

PhraseNArg::~PhraseNArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
}

QuantifiedStatement::~QuantifiedStatement()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/20/2000
	// We have to delete the statement [ArgArray[0]] BEFORE the quantification, in order to 
	// prevent dangling pointers.  This has to be done here [no guarantee that the immediate
	// ancestor does it correctly].
	DELETE_AND_NULL(ArgArray[0]);
}

SeriesOperation::~SeriesOperation()
{	// FORMALLY CORRECT: Kenneth Boyd, 03/02/2010
}

CombinatorialLike::~CombinatorialLike()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/12/2003
}

GCF::~GCF()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/8/2000
}

StdAddition::~StdAddition()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/28/2005
}

StdMultiplication::~StdMultiplication()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/28/2005
}

EqualRelation::~EqualRelation()
{	// FORMALLY CORRECT: Kenneth Boyd, 2/2/2000
}

MetaConnective::~MetaConnective()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/20/98
}

MetaConceptWithArgArray::~MetaConceptWithArgArray()
{	// FORMALLY CORRECT: Kenneth Boyd, 5/19/2006
}

// foundation
MetaConcept::~MetaConcept()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/6/1998
}

