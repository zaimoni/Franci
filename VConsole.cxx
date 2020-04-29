// VConsole.cxx
// implementation of VConsole
// NOTE: this is OS-sensitive

// Logical windows:
//	top half: Franci
//  bottom half: user
//  1st line of bottom half: user name
//  1st line of top half, or title bar: Franci

// headers
#include <memory.h>
#include <fstream>
#include "keyword1.hxx"
#include "OS.hxx"
#include "CmdShell.hxx"
#include "VConsole.hxx"

using namespace std;

// NOTE: Scriptfile will mess with input buffer
// NOTE: logfile requires tapping RET_handler, MetaSay
// defined in LexParse.cxx; used by RET_handler, VConsole::UseScript
extern CmdShell FranciScript;

// Strange keys
static int RET_handler(void)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/4/2005
	FranciScript.cmdstep();
	RET_handler_core();
	return 1;
}

// return handler
KeyMouseResponse* ReturnHandler = RET_handler;

VConsole::VConsole()
{	// other initialization	
	LogfileInName = "Franci.htm";	// name of infile for log cleaning
	LogfileOutName = "Franci2.htm";	// name of outfile for log cleaning

	// app-level strings
	set_AppName("Franci, Math Consultant\x99 V0.4.0.0");
	LogfileAppname = "<head><title>Franci, Math Consultant\x99 V0.4.0.0 logfile</title></head><body>";
	ScriptUnopened = "I regret that I could not open the script.";
	LogUnopened = "I wasn't able to open the log file Franci.htm.";
	LogAlreadyOpened = "But the log file Franci.htm is already open.";
	LogOpened = "<br>Log file Franci.htm now open.";
	LogClosed = "Closing log file Franci.htm.";
	LogAlreadyClosed = "Easier done than requested: the log file is already closed.";
	OS_ID = ::OS_ID;
	SelfLogSign = "</pre></b><p>Franci:<br><pre>";
	UserLogSign = "</pre>User:<b><pre>\n";
}

// Interpreter functions

void
VConsole::LinkInScripting(void)
{
	CmdShellHandler* OldGetLineHookNoFail = FranciScript.GetLineHookNoFail;
	CmdShellHandler2* OldGetLineHook = FranciScript.GetLineHook;
	FranciScript.GetLineHookNoFail = NULL; 
	FranciScript.GetLineHook = GetLineForScriptHook;
	FranciScript.cmdloop();
	FranciScript.GetLineHookNoFail = OldGetLineHookNoFail;
	FranciScript.GetLineHook = OldGetLineHook;
}

bool
VConsole::ScanForStartLogFileBlock(void)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/16/2000
	char Buffer[10];
	CleanLog->getline(Buffer,10);
	if (    ('W'==Buffer[0] || 'w'==Buffer[0])
		&&  0==strcmp(Buffer+1,"hat if "))
		return true;
	return false;
}


// defined in QState.cxx
extern const char* const StartExplore;
extern const char* const ExperimentWithSituation;

//! := strlen(StartExplore)
#define STRLEN_StartExplore 31
//! := strlen(ExperimentWithSituation)
#define STRLEN_ExperimentWithSituation 42

// probably should be virtual
void
VConsole::ShrinkBlock(unsigned long StartBlock, unsigned long EndBlock, unsigned long& ReviewedPoint,
			size_t LogLength)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2005
	// The eval-type command is at the front; there are no others.
	// #1: find two instances of "Starting to explore situation."
	// #2: if second found, find immediately prior instance of
	//	"Experimenting with conditional situation:": this is the new stable point
	// #3: also find first instance of "Experimenting with conditional situation:" after
	//	first instance of "Starting to explore situation."
	// #4: replace gap between two with "..."
	// these two strings are reference strings declared in QState.cxx
	// NOTE: strlen(StartExplore)==30
	// NOTE: strlen(ExperimentWithSituation)==41
	char Buffer[42];
	if (CleanLog->eof() && !CleanLog->fail())
		CleanLog->clear();
	CleanLog->seekg(StartBlock,ios::beg);
	size_t StartExplore2=0;
	unsigned long TestPoint = CleanLog->tellg();
	do	{
		CleanLog->getline(Buffer,STRLEN_StartExplore);
		if 		(!strcmp(Buffer,StartExplore))
			{
			StartExplore2 = TestPoint;
			TestPoint = CleanLog->tellg();
			}
		else if (    0!=StartExplore2
				 && (   !strcmp(Buffer,TruthValue_False)
				     || !strcmp(Buffer,TruthValue_Unknown)))
			{
			size_t FirstExperiment = 0;
			size_t LastExperiment = 0;
			size_t TestPoint2 = StartExplore2;
			TestPoint = CleanLog->tellg();
			CleanLog->seekg(StartExplore2,ios::beg);
			do	{
				CleanLog->getline(Buffer,STRLEN_ExperimentWithSituation);
				if (!strcmp(Buffer,ExperimentWithSituation))
					{
					FirstExperiment=TestPoint2;
					LastExperiment=TestPoint2;
					};
				TestPoint2=CleanLog->tellg();
				}
			while(0==FirstExperiment && TestPoint2<TestPoint);
			while(TestPoint2<TestPoint)
				{
				CleanLog->getline(Buffer,STRLEN_ExperimentWithSituation);
				if (!strcmp(Buffer,ExperimentWithSituation))
					LastExperiment=TestPoint2;
				TestPoint2=CleanLog->tellg();
				};
			if (FirstExperiment<LastExperiment)
				{
				SaysNormal("Cleaning block");
				if (ReviewedPoint<FirstExperiment)
					CopyBlock(ReviewedPoint,FirstExperiment,ReviewedPoint);
				*ImageCleanLog<<'.'<<'.'<<'.'<<'\n';
				ReviewedPoint=LastExperiment;
				TestPoint=TestPoint2;
				};
			// reset state
			CleanLog->seekg(TestPoint,ios::beg);
			StartExplore2 = 0;
			}
		else
			TestPoint = CleanLog->tellg();
		}
	while(TestPoint<EndBlock && !CleanLog->bad());
}

#undef STRLEN_ExperimentWithSituation
#undef STRLEN_StartExplore

