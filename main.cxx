// main.cxx

#include "OS.hxx"
#include "VConsole.hxx"

#ifdef ZAIMONI_HAVE_ACCURATE_MSIZE
#include "Zaimoni.STL/z_memory.h"
#endif

#include <stdlib.h>
#include <fstream>
#include <time.h>

using namespace std;

void InitializeDefaultSetsAndClasses(void); // defined in Class.cxx
void DiagnoseMetaConceptVFT(void);			// defined in MetaCon1.cxx
void InitializeFranciInterpreter(void);		// defined in LexParse.cxx
void InitializeLexerDefs(void);			// defined in LangConf.cxx
void Init_Unparsed_Eval(void);			// defined in Unparsed.cxx

#ifndef NDEBUG
// does a low-level system test
void SystemTest_MetaQuantifier(void);	// low-level test of MetaQuantifier

void
SystemTest(void)
{	// MUTABLE CODE
	SystemTest_MetaQuantifier();
}
#endif

void
OSIndependentInitialize(void)
{
	ExtractSystemInfo();	// this extracts hardware information
	InitializeDefaultSetsAndClasses();
	DiagnoseMetaConceptVFT();
	InitializeFranciInterpreter();
	InitializeLexerDefs();
	Init_Unparsed_Eval();
	srand(clock());	// sets RNG seed to # of clock ticks
}

#if defined(FRANCI_CLI) || !defined(_WIN32)
#pragma message("NOTE: Compiling main")
int main(int argc, char* argv[], char* envp[])
// the mainline of ProtoAI
{
	VConsole Tmp;
	_console = &Tmp;
	OSIndependentInitialize();

	//! \todo extract OS info
#ifndef NDEBUG
	SystemTest();
#endif

	do	{	//! \todo main window message handler iteration also goes here
		_console->LookAtConsoleInput();
		}
	while(1);
	//! \todo install the message handler loop, for OS ___
	return 0;	// success
};

#else

#define WIN32_LEAN_AND_MEAN 1
#include <WINDOWS.H>
// #include <MAPIDEFS.H>
// #include <MAPINLS.H>

#pragma message("NOTE: Compiling WinMain")

int WINAPI
WinMain(HINSTANCE  hInstance,	/* handle to current instance */
        HINSTANCE  hPrevInstance,	/* handle to previous instance */
        LPSTR  lpCmdLine,	/* pointer to command line */
        int  nShowCmd 	/* show state of window */)
{
	VConsole Tmp;
	_console = &Tmp;
	OSIndependentInitialize();
	//! \todo we (eventually) need a class to deal with the main window for Franci.
	//! this can wait a while (we need it when the math notation becomes more interesting).
	// Low-level data structure testing
#ifndef NDEBUG
	SystemTest();
#endif

	try	{
		//! \todo console/message handler loop; body should be a separate function [there are other
		//! places where the message-handler loop makes sense
		do	{
			//! \todo main window message handler iteration also goes here
			while(_console->LookAtConsoleInput());
			Sleep(100);
			}
		while(1);
		return EXIT_SUCCESS;	// success
		}
	catch(...)
		{
		return EXIT_FAILURE;
		}
}

#endif
