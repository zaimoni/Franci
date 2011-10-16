// global.cxx
// global overrides, etc.

#define CXX_GLOBAL

#include "global.hxx"
#include "Zaimoni.STL/MetaRAM2.hpp"

const char* const AlphaCallAssumption = "ALPHA ERROR: function call assumptions violated.  I QUIT!";
const char* const AlphaDataIntegrity = "ALPHA ERROR: data structure integrity violated.  I QUIT!";
const char* const AlphaMustDefineVFunction = "ALPHA ERROR: virtual function needs to be defined for C++ class.  I QUIT!";
const char* const AlphaMiscallVFunction = "ALPHA ERROR: virtual function called for C++ class that must not implement it.  I QUIT!";
const char* const AlphaMiscallFunction = "ALPHA ERROR: function called in way that damages data integrity.  I QUIT!";
const char* const AlphaNoEffectFunctionCall = "ALPHA ERROR: function call has no effect.  I QUIT!";
const char* const AlphaRetValAssumption = "ALPHA ERROR: function return value assumptions violated.  I QUIT!";
const char* const AlphaBadSyntaxGenerated = "ALPHA ERROR: Syntax error generated.  I QUIT!";
const char* const RAMFailure = "FATAL ERROR: irrecoverable RAM failure in computation.  I QUIT!";

//! \todo OPTIMIZE: STATIC MULTITHREADED: ReportTime is a target for a statically allocated buffer,
//! with AIMutex to block it....
void
ReportTime(clock_t Clock0, clock_t Clock1)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/18/1999
	char Buffer[] = "# 0.01 sec.:            ";
	_ltoa(((Clock1-Clock0)*100)/CLOCKS_PER_SEC,Buffer+13,10);
	INFORM(Buffer);
}

// this doesn't need to be moved to LangConf: it's generic
void
FlushLeadingBlankLinesFromTextBuffer(char*& Buffer, size_t& StartingLogicalLineNumber)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/27/2007
	size_t SweepIdx = strspn(Buffer,"\n");
	if (ArraySize(Buffer)==SweepIdx)
		{
		DELETE_AND_NULL(Buffer);
		return;
		}
	if (0<SweepIdx)
		{
		StartingLogicalLineNumber += SweepIdx;
		memmove(Buffer,&Buffer[SweepIdx],_msize(Buffer)-SweepIdx);
		Buffer = REALLOC(Buffer,_msize(Buffer)-SweepIdx);
		}
}

