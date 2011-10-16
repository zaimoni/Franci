// global.hxx

#ifndef FRANCI_GLOBAL_HXX
#define FRANCI_GLOBAL_HXX

// rest of headers
#include "Zaimoni.STL/Compiler.h"
#include "Zaimoni.STL/polymorphic.hpp"
#include "Zaimoni.STL/z_memory.h"
#include "VConsole.hxx"
#include <time.h>

using namespace std;
using namespace zaimoni;

template<typename T,typename U>
void
CopyInto_ForceSyntaxOK(const T& src, U*& dest)
{
	zaimoni::CopyInto(src,dest);
	if (NULL!=dest && !dest->SyntaxOK())
		{
		delete dest;
		dest = NULL;
		throw bad_alloc();
		}
}

extern const char* const AlphaCallAssumption;
extern const char* const AlphaDataIntegrity;
extern const char* const AlphaMustDefineVFunction;
extern const char* const AlphaMiscallVFunction;
extern const char* const AlphaMiscallFunction;
extern const char* const AlphaNoEffectFunctionCall;
extern const char* const AlphaRetValAssumption;
extern const char* const AlphaBadSyntaxGenerated;
extern const char* const RAMFailure;

void UnconditionalDataIntegrityFailure(void) NO_RETURN;
void UnconditionalRAMFailure(void) NO_RETURN;
void UnconditionalCallAssumptionFailure(void) NO_RETURN;

void ReportTime(clock_t Clock0, clock_t Clock1);

void FlushLeadingBlankLinesFromTextBuffer(char*& Buffer, size_t& StartingLogicalLineNumber);

#endif
