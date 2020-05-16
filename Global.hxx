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

constexpr const char* const AlphaCallAssumption = "ALPHA ERROR: function call assumptions violated.  I QUIT!";
constexpr const char* const AlphaDataIntegrity = "ALPHA ERROR: data structure integrity violated.  I QUIT!";
constexpr const char* const AlphaMustDefineVFunction = "ALPHA ERROR: virtual function needs to be defined for C++ class.  I QUIT!";
constexpr const char* const AlphaMiscallVFunction = "ALPHA ERROR: virtual function called for C++ class that must not implement it.  I QUIT!";
constexpr const char* const AlphaMiscallFunction = "ALPHA ERROR: function called in way that damages data integrity.  I QUIT!";
constexpr const char* const AlphaNoEffectFunctionCall = "ALPHA ERROR: function call has no effect.  I QUIT!";
constexpr const char* const AlphaRetValAssumption = "ALPHA ERROR: function return value assumptions violated.  I QUIT!";
constexpr const char* const RAMFailure = "FATAL ERROR: irrecoverable RAM failure in computation.  I QUIT!";

void UnconditionalDataIntegrityFailure(void) NO_RETURN;
void UnconditionalRAMFailure(void) NO_RETURN;
void UnconditionalCallAssumptionFailure(void) NO_RETURN;

void ReportTime(clock_t Clock0, clock_t Clock1);

#endif
