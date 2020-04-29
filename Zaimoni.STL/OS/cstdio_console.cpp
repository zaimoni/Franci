// Console.cxx
// implementation of VConsole
// NOTE: this is OS-sensitive
// NOTE: we handle C stdio separately, as we have no reliable macros for this

// Logical windows:
//	top half: Franci
//  bottom half: user
//  1st line of bottom half: user name
//  1st line of top half, or title bar: Franci

// headers
#include "console.hpp"

// to be initialized by the application
Console* _console = NULL;

#include "../MetaRAM2.hpp"
#include "../fstream"
#include "../Pure.C/format_util.h"
#include "../Pure.C/logging.h"
#include <memory.h>
#include <time.h>

using namespace std;
using namespace zaimoni;

// private static data initialization
static bool LastMessage = false;

// public static data initialization
// NOTE: Scriptfile will mess with input buffer
// NOTE: logfile requires tapping RET_handler, MetaSay
// defined in LexParse.cxx; used by RET_handler, VConsole::UseScript

// KB: global variables here 'really should be' static variables, but we want to protect
// the class header VConsole.hxx from the OS
// Keyboard/mouse model
KeyMouseResponse* LastHandlerUsed = NULL;

#define UserBarY() 13

// messages; not owned by Console
// initialized by subclasses, this just NULLs them
const char* Console::LogClosed = NULL;
const char* Console::LogAlreadyClosed = NULL;
const char* Console::SelfLogSign = NULL;
const char* Console::UserLogSign = NULL;

//! \todo augmented family of new messaging functions based on MetaSay
//! These would do intra-sentence concatenation of ' ' using Perl-like join/PHP-like implode string processing
//! to create a temporary string for the normal messaging functions

Console::Console()
:	CleanLog(CloseAndNULL<std::ifstream>),
	ImageCleanLog(CloseAndNULL<std::ofstream>),
	LogfileInName(NULL),
	LogfileOutName(NULL),
	AppName(NULL),
	LogfileAppname(NULL),
	ScriptUnopened(NULL),
	LogUnopened(NULL),
	LogAlreadyOpened(NULL),
	LogOpened(NULL),
	OS_ID(NULL)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/1999
#ifdef _WIN32
//	SetConsoleTitle(AppName);	// fix this later
#endif
}

Console::~Console()
{
}

void Console::set_AppName(const char* src)
{
	AppName = src;
}

//! This is the default getline handler for the CmdShell object in LexParse's InitializeFranciInterpreter
void GetLineFromKeyboardHook(char*& InputBuffer)
{
	if (feof(stdin)) exit(EXIT_SUCCESS);
	if (ferror(stdin)) exit(EXIT_FAILURE);
	size_t inchar = 0;
	int tmp = fgetc(stdin);
	while(EOF!=tmp && '\n'!=tmp)
		{
		if (char* const tmp2 = REALLOC(InputBuffer,++inchar)) InputBuffer = tmp2;
		else FATAL("RAM failure when getting line for parsing");
		InputBuffer[inchar-1] = (char)tmp;
		tmp = fgetc(stdin);
		}
}

// no-op, needed to inter-operate
void RET_handler_core()
{
}

int Console::LookAtConsoleInput()
{
	return ReturnHandler();
}

static custom_scoped_ptr<ifstream> ScriptFile(CloseAndNULL<ifstream>);
static size_t ScriptLength = 0;

//! this is the script override for FranciScript's GetLineHook in LexParse
bool GetLineForScriptHook(char*& InputBuffer)
{	// XXX code fragment needs global variable ScriptFile XXX
	if (ScriptFile->eof()) return false;

	const streamoff StartPosition = ScriptFile->tellg();
	ScriptFile->ignore(ScriptLength,'\n');
	const size_t Offset = ScriptFile->tellg()-StartPosition;
	if (0==Offset) return false;

	// #2: allocate RAM for readin
	char* Tmp = REALLOC(InputBuffer,Offset);
	if (!Tmp)
		{
		ScriptFile.clear();
		FREE_AND_NULL(InputBuffer);
		FATAL("RAM failure when getting line for parsing");
		};
	InputBuffer = Tmp;

	// #3: read it in
	ScriptFile->seekg(StartPosition,ios::beg);
	ScriptFile->getline(InputBuffer,Offset,'\n');

	if (InputBuffer && !InputBuffer[ArraySize(InputBuffer)-1])
#if ZAIMONI_REALLOC_TO_ZERO_IS_NULL
		InputBuffer = REALLOC(InputBuffer,_msize(InputBuffer)-sizeof(char));
#else
#error need to handle non-NULL realloc(x,0);
#endif
	return true;
}

void Console::UseScript(const char* ScriptName)
{	// FORMALLY CORRECT: 12/5/2004
	// Prefer that ScriptFile and ScriptLength be scoped to this function,
	// but that doesn't work with the CmdShell class
	ScriptFile = ReadOnlyInfileBinary(ScriptName,ScriptUnopened);
	if (!ScriptFile) return;
	ScriptFile->seekg(0,ios::end);
	ScriptLength = ScriptFile->tellg();
	ScriptFile->seekg(0,ios::beg);
	LinkInScripting();
	ScriptFile.clear();
}

// XXX prevent globals from being used below here
#define ScriptFile
#define ScriptLength

void Console::StartLogFile()
{	// FORMALLY CORRECT: Kenneth Boyd, 11/10/2004
	if (is_logfile_at_all())
		// ERROR
		SaysWarning(LogAlreadyOpened);
	else{
		if (!start_logfile(LogfileInName)) SaysError(LogUnopened);

		LastMessage = true;
		Log(LogfileAppname);
		Log(AppName);
		Log(OS_ID);
//		const time_t LogStartTime = time(NULL);	// this plays havoc with automated testing
//		Log(asctime(localtime(&LogStartTime)));
		SaysNormal(LogOpened);
		LastMessage = false;
		}
}

void Console::EndLogFile()
{	// FORMALLY CORRECT: 12/30/1999
	if (is_logfile_at_all())
		{
		resume_logging();
		SaysNormal(LogClosed);
		Log("</body>\n");
		end_logfile();
		}
	else
		SaysNormal(LogAlreadyClosed);
}

void Console::ResumeLogFile()
{	// FORMALLY CORRECT: 12/31/1999
	if (resume_logging()) Log("...");
}

void 
Console::CopyBlock(unsigned long StartBlock, unsigned long EndBlock, unsigned long& ReviewedPoint)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/17/2000
	// if ReviewedPoint!=StartBlock, results could look weird
	if (CleanLog->eof() && !CleanLog->fail()) CleanLog->clear();
	CleanLog->seekg(StartBlock,ios::beg);
	char Buffer[512];
	while(512+StartBlock<=EndBlock)
		{
		CleanLog->read(Buffer,512);
		ImageCleanLog->write(Buffer,512);
		ImageCleanLog->flush();
		StartBlock+=512;
		};
	if (StartBlock<EndBlock)
		{
		CleanLog->read(Buffer,EndBlock-StartBlock);
		ImageCleanLog->write(Buffer,EndBlock-StartBlock);
		ImageCleanLog->flush();
		};
	ReviewedPoint = EndBlock;
}

void Console::CleanLogFile()
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2005
	// Franci has to clean one of her own logfiles, here.
	end_logfile();

	// #1) open logfile, image
	CleanLog = ReadOnlyInfileBinary(LogfileInName,"I could not open the files required to clean the logfile.");
	if (!CleanLog) return;

	ImageCleanLog = OutfileBinary(LogfileOutName,"I could not open the files required to clean the logfile.");
	if (!ImageCleanLog)
		{
		CleanLog.clear();
		return;
		}

	// #2) analyze logfile for irrelevant deadends
	// For a first approximation, Franci wants to track blocks defined by
	//	"what if __"/"What if __"
	//	If there is more than one "Starting to explore situation", we have a possible
	//	restart: prior to the second and following instances, check for the first
	//  instance of "Exploring conditional situation".  If this is *not* the first
	//  instance after "Starting to explore situation", trim out the intervening logfile
	//  and replace it with "...".
	//  we can using leading '\n' to help out.
	// #3) copy logfile to image, pruning out irrelevant deadends
	unsigned long ReviewedPoint = 0;
	CleanLog->seekg(0,ios::end);
	const size_t LogLength = CleanLog->tellg();
	CleanLog->seekg(0,ios::beg);
	size_t StartBlock = 0;
	while(CleanLog->good())
		{
		const size_t TestPoint = CleanLog->tellg();
		if (ScanForStartLogFileBlock())
			{
			SaysNormal("Found block start");
			// check to see if the current block has a result.  If so, prune it.
			if (0!=StartBlock)
				ShrinkBlock(StartBlock,TestPoint,ReviewedPoint,LogLength);
			StartBlock = TestPoint;
			};
		};
	// check to see if the current block has a result.  If so, prune it.
	if (0!=StartBlock)
		ShrinkBlock(StartBlock,LogLength,ReviewedPoint,LogLength);
	// final copy
	if (0<ReviewedPoint)
		{
		if (ReviewedPoint<LogLength)
			CopyBlock(ReviewedPoint,LogLength,ReviewedPoint);
		
		// #4) close logfile and image
		CleanLog.clear();
		ImageCleanLog.clear();
		}
	else{
		CleanLog.clear();
		ImageCleanLog.clear();
		remove(LogfileOutName);
		}
}

void Console::Log(const char* x,size_t x_len)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/2/1999
	if (!LastMessage)
		{
		LastMessage = true;
		if (SelfLogSign) inc_log_substring(SelfLogSign,strlen(SelfLogSign));
		}
	add_log_substring(x,x_len);
}

void Console::LogUserInput(const char* x,size_t x_len)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/2/1999
	if (LastMessage)
		{
		LastMessage = false;
		if (UserLogSign) inc_log_substring(UserLogSign,strlen(UserLogSign));
		}
	add_log_substring(x,x_len);
}

// #1: parse how many lines are required
static int ParseMessageIntoLines(const char* x, size_t x_len, size_t* const LineBreakTable, const size_t width)
{	// Message is assumed not to contain formatting characters.
	memset(LineBreakTable,0,13*sizeof(size_t));
	size_t LineCount = 0;		
	// #1: prune off trailing whitespace
	while(0<x_len && (' '==x[x_len-1] || '\n'==x[x_len-1])) --x_len;
	// #2: if <= Domain.X, it's over
Restart:
	while(0<x_len)
		{
		memmove(LineBreakTable,LineBreakTable+1,sizeof(size_t)*12);
		++LineCount;
		const size_t ub = (x_len<=width) ? x_len : width;
		const char* newline = strchr(x,'\n');
		if (NULL==newline)
			{
			LineBreakTable[12] += ub;
			x += ub;
			x_len -= ub;
			goto Restart;
			};
		const size_t i = newline-x;
		if (ub<i)
			{
			LineBreakTable[12] += ub;
			x += ub;
			x_len -= ub;
			goto Restart;
			}
		// #3: look for \n [we have done pre-formatting with this]
		LineBreakTable[12] += i;
		x += i+1;
		x_len -= i+1;
		};
	return LineCount;
}

static void line_out(const char* const x, size_t span)
{
	if (0<span && span!=fwrite(x,sizeof(*x),span,stdout)) exit(EXIT_FAILURE);
}

static void line_out(const char* const x, size_t i, size_t* const LineBreakTable)
{
	if (0<LineBreakTable[i])
		{
		if ('\n'==x[LineBreakTable[i-1]]) ++(LineBreakTable[i-1]);
		if (LineBreakTable[i]>LineBreakTable[i-1])
			{
			const size_t span = LineBreakTable[i]-LineBreakTable[i-1];
			if (span!=fwrite(x+LineBreakTable[i-1],sizeof(*x),span,stdout)) exit(EXIT_FAILURE);
			if (EOF==fputc('\r',stdout)) exit(EXIT_FAILURE);
			if (EOF==fputc('\n',stdout)) exit(EXIT_FAILURE);
			}
		};
}

static void _whisper(const char* x,size_t x_len)
{
	size_t LineBreakTable[UserBarY()];
	// #1: parse how many lines are required
	const size_t LineCount = ParseMessageIntoLines(x,x_len,LineBreakTable,80);
	if (0>=LineCount) return;

	// #2: scroll to make space; colorize new space now
	if (UserBarY()<LineCount)
		// LONG MESSAGE
		line_out("...",3);
	else	// SHORT MESSAGE
		line_out(x,LineBreakTable[0]);

	size_t i = 1;
	do	line_out(x,i,LineBreakTable);
	while(UserBarY()> ++i);
}

// we are used for automated testing
void Console::Whisper(const char* x)
{
	if (!x || !*x) return;
	_whisper(x,strlen(x));
}

static void MetaSay(const char* x,size_t x_len)
{
	Console::Log(x,x_len);	// log the message
	size_t LineBreakTable[UserBarY()];
	// #1: parse how many lines are required
	const size_t LineCount = ParseMessageIntoLines(x,x_len,LineBreakTable,80);
	if (0>=LineCount) return;

	// #2: scroll to make space; colorize new space now
	if (UserBarY()<LineCount)
		// LONG MESSAGE
		line_out("...",3);
	else	// SHORT MESSAGE
		line_out(x,LineBreakTable[0]);

	size_t i = 1;
	do	line_out(x,i,LineBreakTable);
	while(UserBarY()> ++i);
}

void Console::SaysNormal(const char* x)		// white text
{
	if (!x || !*x) return;
	MetaSay(x,strlen(x));
}

void Console::SaysWarning(const char* x)	// yellow text; consider sound effects
{
	if (!x || !*x) return;
	MetaSay(x,strlen(x));
}

void Console::SaysError(const char* x)		// red text; consider sound effects
{
	if (!x || !*x) return;
	MetaSay(x,strlen(x));
}

// Logging.h hooks
EXTERN_C void _fatal(const char* const B)
{
	Console::SaysError(B);
	Console::EndLogFile();
//	MessageBox(NULL,B,"Console",MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);
//	while(!Console::LookAtConsoleInput());
	exit(EXIT_FAILURE);
}

EXTERN_C void _fatal_code(const char* const B,int exit_code)
{
	Console::SaysError(B);
	Console::EndLogFile();
//	MessageBox(NULL,B,"Console",MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);
//	while(!Console::LookAtConsoleInput());
	exit(exit_code);
}

EXTERN_C void _inform(const char* const B, size_t len)
{
	if (!B || !*B || 0>=len) return;
	MetaSay(B,len);
}

EXTERN_C void _log(const char* const B,size_t len)
{
	Console::Log(B,len);
}

void SEVERE_WARNING(const char* const B)
{
	Console::SaysError(B);
}

void WARNING(const char* const B)
{
	Console::SaysWarning(B);
}

