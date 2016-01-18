// Console.cxx
// implementation of VConsole
// NOTE: this is OS-sensitive

// Logical windows:
//	top half: Franci
//  bottom half: user
//  1st line of bottom half: user name
//  1st line of top half, or title bar: Franci

// headers
#include "console.hpp"

// to be initialized by the application
Console* _console = NULL;

#ifdef _WIN32
#include <WINDOWS.H>

// prevent collision with later template function definition in MetaRAM.hpp
#undef DELETE
#else
#error("Fatal Error: VConsole not implemented.")
#endif

#include "../MetaRAM2.hpp"
#include "../fstream"
#include "../pure.C/format_util.h"
#include "../pure.C/logging.h"
#include <memory.h>
#include <time.h>

using namespace std;
using namespace zaimoni;

// private static data initialization
static bool LastMessage = false;

// public static data initialization
char* InputBuffer = NULL;		// Franci's input buffer.
size_t InputBufferLength = 0;	// Length of Franci's input buffer
// NOTE: Scriptfile will mess with input buffer
// NOTE: logfile requires tapping RET_handler, MetaSay
// defined in LexParse.cxx; used by RET_handler, VConsole::UseScript

// KB: global variables here 'really should be' static variables, but we want to protect
// the class header VConsole.hxx from the OS
// Keyboard/mouse model
KeyMouseResponse* LastHandlerUsed = NULL;
int CTRL_on = 0;
int SHIFT_on = 0;
int ALT_on = 0;
int NUMLOCK_on = 1;	// KB: the keyboard automatically compensates for most of this.
int CAPSLOCK_on = 0;

#ifdef _WIN32
// OS Interface
static HANDLE StdInputHandle = NULL;
static HANDLE StdOutputHandle = NULL;
static HANDLE StdErrorHandle = NULL;
static CONSOLE_SCREEN_BUFFER_INFO ScreenBufferState;
static CONSOLE_CURSOR_INFO CursorState;
// static CPINFO StandardANSICodePage;
static COORD LowerRightCornerConsole;

static COORD LogicalOrigin;
static COORD LogicalEnd;

// this does weird things in Win2000.  Really need *2* text consoles (one in, one out)
#define UserBarY() 13
#if 0
inline size_t UserBarY()
{return (LowerRightCornerConsole.Y>>1)+1;}	// >>1 is /2
#endif

void ExtendInputBuffer(size_t ExtraLength,COORD Origin)
{
	unsigned long CharCount;	
	if (!_resize(InputBuffer,InputBufferLength += ExtraLength))
		{
		FREE_AND_NULL(InputBuffer);
		InputBufferLength = 0;
		exit(EXIT_FAILURE);
		}
#ifdef _WIN32
	ReadConsoleOutputCharacter(StdOutputHandle,InputBuffer+InputBufferLength-ExtraLength,ExtraLength,Origin,&CharCount);
#endif
}

void
FinalExtendInputBuffer(size_t ExtraLength,COORD Origin,char*& InputBuffer2)
{
	unsigned long CharCount;	
	if (!_resize(InputBuffer2,InputBufferLength += ExtraLength))
		{
		FREE_AND_NULL(InputBuffer);
		InputBufferLength = 0;
		exit(EXIT_FAILURE);
		}
#ifdef _WIN32
	ReadConsoleOutputCharacter(StdOutputHandle,InputBuffer2+InputBufferLength-ExtraLength,ExtraLength,Origin,&CharCount);
#endif
	InputBuffer = NULL;
	InputBufferLength = 0;
}

// We have a reasonable definition for <, and == on COORD.  Use them.
int operator<(const COORD& LHS, const COORD& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/1999
	return (   LHS.Y<RHS.Y
			|| (LHS.Y==RHS.Y && LHS.X<RHS.X));
}

int operator==(const COORD& LHS, const COORD& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/1999
	return (LHS.X==RHS.X && LHS.Y==RHS.Y);
}

int operator-(const COORD& LHS, const COORD& RHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/1999
	if (LHS.Y==RHS.Y)
		return LHS.X-RHS.X;
	if (LHS.Y>RHS.Y)
		return -(RHS-LHS);
	return (RHS.Y-LHS.Y)*(LowerRightCornerConsole.X+1)-RHS.X+LHS.X;
}

const COORD& operator++(COORD& LHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/1999
	LHS.X++;
	if (LHS.X>LowerRightCornerConsole.X)
		{
		LHS.X = 0;
		LHS.Y++;
		}
	return LHS;
}

const COORD& operator--(COORD& LHS)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/1999
	if (0==LHS.X)
		{
		LHS.X = LowerRightCornerConsole.X;
		LHS.Y--; 
		}
	else
		LHS.X--;
	return LHS;
}
#endif

// KB: color codes
#ifdef _WIN32
enum TextColors
	{
	Text_White = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN,
	Text_Violet	= FOREGROUND_RED | FOREGROUND_BLUE,
	Text_Yellow = FOREGROUND_RED | FOREGROUND_GREEN,
	Text_Cyan = FOREGROUND_BLUE | FOREGROUND_GREEN,
	Text_Red = FOREGROUND_RED | FOREGROUND_INTENSITY,	// unreadable otherwise
	Text_Blue = FOREGROUND_BLUE,
	Text_Green = FOREGROUND_GREEN
	};
#endif

// messages; not owned by Console
// initialized by subclasses, this just NULLs them
const char* Console::LogClosed = NULL;
const char* Console::LogAlreadyClosed = NULL;
const char* Console::SelfLogSign = NULL;
const char* Console::UserLogSign = NULL;

#ifdef _WIN32
BOOL WINAPI KillConsole(DWORD dwCtrlType)
{
	FreeConsole();
	return FALSE;
}
#endif

//! \todo augmented family of new messaging functions based on MetaSay
//! These would do intra-sentence concatenation of ' ' using Perl-like join/PHP-like implode string processing
//! to create a temporary string for the normal messaging functions

static void ResetInput(size_t StartLine)
{
#ifdef _WIN32
	LogicalOrigin.X = 0;
	LogicalOrigin.Y = StartLine;
	LogicalEnd = LogicalOrigin;
#endif
}

static void PutCursorAtUserHome()
{	// FORMALLY CORRECT: 4/22/1999
#ifdef _WIN32
	LogicalOrigin.X = 0;
	LogicalOrigin.Y = UserBarY()+1;
	LogicalEnd = LogicalOrigin;
	SetConsoleCursorPosition(StdOutputHandle,LogicalOrigin);
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	GetConsoleCursorInfo(StdOutputHandle,&CursorState);
#endif
}

static void DrawUserBar()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/1999
#ifdef _WIN32
	COORD StartLine = {0, 0};
	unsigned long ActualCharCount;
	// User bar
	do	{
		FillConsoleOutputAttribute(	StdOutputHandle,
									((size_t)(StartLine.Y)==UserBarY()) ? BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN :  Text_White,
									LowerRightCornerConsole.X+1,
									StartLine,
									&ActualCharCount);
		}
	while(++StartLine.Y<=LowerRightCornerConsole.Y);
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	GetConsoleCursorInfo(StdOutputHandle,&CursorState);
#endif
}

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
	if (NULL!=StdInputHandle)
		{	// Franci: oops, this is a second text console
		MessageBox(NULL,"Pre-Alpha Error: only one text console allowed.","Franci: I QUIT!",
					MB_SETFOREGROUND | MB_OK | MB_TASKMODAL | MB_ICONSTOP);
		FreeConsole();
		exit(EXIT_FAILURE);
		}
	AllocConsole();
	StdInputHandle = GetStdHandle(STD_INPUT_HANDLE);
	StdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	StdErrorHandle = GetStdHandle(STD_ERROR_HANDLE);
	// Sets: MS-DOS United States
	SetConsoleCP(437);
	SetConsoleOutputCP(437);
	SetConsoleTextAttribute(StdOutputHandle,Text_White);
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	GetConsoleCursorInfo(StdOutputHandle,&CursorState);
	SetConsoleTitle(AppName);
	ResetInput(UserBarY()+1);
	LowerRightCornerConsole.X = ScreenBufferState.dwSize.X-1;
	LowerRightCornerConsole.Y = ScreenBufferState.dwSize.Y-1;
    SetConsoleCtrlHandler(KillConsole,TRUE);
#endif

	// enforce boundary between Franci, user halves
	DrawUserBar();
	// put cursor at start of user input
	PutCursorAtUserHome();
}

Console::~Console()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/1999
#ifdef _WIN32
	FreeConsole();
#endif
}

int PrintCharAt(char Target)
{	// enforce insert-and-displace mode
#ifdef _WIN32
	if (LogicalEnd == LowerRightCornerConsole)
		Console::ScrollUserScreenOneLine();
	// The actual print
	if (ScreenBufferState.dwCursorPosition<LogicalEnd)
		{	// #0: push all chars at or beyond insertion point 1 char forward
		COORD Idx = LogicalEnd;
		COORD IdxPlus1 = Idx;
		++IdxPlus1;
		do
			{
			unsigned long ReadWriteCount;
			char Buffer1;
			--Idx;
			--IdxPlus1;
			ReadConsoleOutputCharacter(StdOutputHandle,&Buffer1,1,Idx,&ReadWriteCount);
			WriteConsoleOutputCharacter(StdOutputHandle,&Buffer1,1,IdxPlus1,&ReadWriteCount);
			}
		while(ScreenBufferState.dwCursorPosition<Idx);
		};
	++LogicalEnd;
	// #1: print char
	{
	unsigned long CharsWritten;
	FillConsoleOutputCharacter(StdOutputHandle,Target,1,ScreenBufferState.dwCursorPosition,&CharsWritten);
	if (1!=CharsWritten)
		return 0;
	}
	// #2: adjust cursor by 1
	++ScreenBufferState.dwCursorPosition;
	SetConsoleCursorPosition(StdOutputHandle,ScreenBufferState.dwCursorPosition);	
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	return 1;
#endif
}

// Screen-maneuvering handlers
int HOME_handler()
{	// sends cursor to LogicalOrigin
#ifdef _WIN32
	SetConsoleCursorPosition(StdOutputHandle,LogicalOrigin);
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	GetConsoleCursorInfo(StdOutputHandle,&CursorState);
	return 1;
#endif
}

int END_handler()
{	// sends cursor to LogicalEnd
#ifdef _WIN32
	SetConsoleCursorPosition(StdOutputHandle,LogicalEnd);
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	GetConsoleCursorInfo(StdOutputHandle,&CursorState);
	return 1;
#endif
}

int LARROW_handler()
{	// sends cursor to LogicalOrigin
#ifdef _WIN32
	COORD TmpPosition = ScreenBufferState.dwCursorPosition;
	if (LogicalOrigin<TmpPosition)
		{
		--TmpPosition;
		SetConsoleCursorPosition(StdOutputHandle,TmpPosition);
		GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
		GetConsoleCursorInfo(StdOutputHandle,&CursorState);
		}
	return 1;
#endif
}

int RARROW_handler()
{	// sends cursor to LogicalOrigin
#ifdef _WIN32
	COORD TmpPosition = ScreenBufferState.dwCursorPosition;
	if (TmpPosition<LogicalEnd)
		{
		++TmpPosition;
		SetConsoleCursorPosition(StdOutputHandle,TmpPosition);
		GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
		GetConsoleCursorInfo(StdOutputHandle,&CursorState);
		}
	return 1;
#endif
}

int UARROW_handler()
{	// sends cursor to LogicalOrigin
#ifdef _WIN32
	COORD TmpPosition = ScreenBufferState.dwCursorPosition;
	if (TmpPosition.Y>LogicalOrigin.Y)
		{
		--TmpPosition.Y;
		SetConsoleCursorPosition(StdOutputHandle,TmpPosition);
		GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
		GetConsoleCursorInfo(StdOutputHandle,&CursorState);
		}
	return 1;
#endif
}

int DARROW_handler()
{	// sends cursor to LogicalOrigin
#ifdef _WIN32
	COORD TmpPosition = ScreenBufferState.dwCursorPosition;
	if (    TmpPosition.Y<LogicalEnd.Y-1
		|| (TmpPosition.X<=LogicalEnd.X && TmpPosition.Y<LogicalEnd.Y))
		{
		++TmpPosition.Y;
		SetConsoleCursorPosition(StdOutputHandle,TmpPosition);
		GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
		GetConsoleCursorInfo(StdOutputHandle,&CursorState);
		}
	return 1;
#endif
}

// This family of functions prints ascii characters in Console
// Identifier-competent characters

#define DECLARE_ASCII(X,SHIFT_KEY,NONSHIFT_KEY)	\
int	ASCII##X()	\
{	\
	return PrintCharAt(((SHIFT_on) ? !CAPSLOCK_on : CAPSLOCK_on) ? SHIFT_KEY : NONSHIFT_KEY);	\
}

DECLARE_ASCII(A,'A','a')
DECLARE_ASCII(B,'B','b')
DECLARE_ASCII(C,'C','c')
DECLARE_ASCII(D,'D','d')
DECLARE_ASCII(E,'E','e')
DECLARE_ASCII(F,'F','f')
DECLARE_ASCII(G,'G','g')
DECLARE_ASCII(H,'H','h')
DECLARE_ASCII(I,'I','i')
DECLARE_ASCII(J,'J','j')
DECLARE_ASCII(K,'K','k')
DECLARE_ASCII(L,'L','l')
DECLARE_ASCII(M,'M','m')
DECLARE_ASCII(N,'N','n')
DECLARE_ASCII(O,'O','o')
DECLARE_ASCII(P,'P','p')
DECLARE_ASCII(Q,'Q','q')
DECLARE_ASCII(R,'R','r')
DECLARE_ASCII(S,'S','s')
DECLARE_ASCII(T,'T','t')
DECLARE_ASCII(U,'U','u')
DECLARE_ASCII(V,'V','v')
DECLARE_ASCII(W,'W','w')
DECLARE_ASCII(X,'X','x')
DECLARE_ASCII(Y,'Y','y')
DECLARE_ASCII(Z,'Z','z')

#undef DECLARE_ASCII

#define DECLARE_ASCII(X,SHIFT_KEY,NONSHIFT_KEY)	\
int	ASCII##X()	\
{	\
	return  PrintCharAt((SHIFT_on) ? SHIFT_KEY : NONSHIFT_KEY);	\
}

int ASCII0()
{
	if (SHIFT_on)
		return PrintCharAt('(') && PrintCharAt(')');
	else
		return PrintCharAt('0');
}

DECLARE_ASCII(1,'!','1')
DECLARE_ASCII(2,'@','2')
DECLARE_ASCII(3,'#','3')
DECLARE_ASCII(4,'$','4')
DECLARE_ASCII(5,'%','5')
DECLARE_ASCII(6,'^','6')
DECLARE_ASCII(7,'&','7')
DECLARE_ASCII(8,'*','8')

int ASCII9()
{
	if (SHIFT_on)
		return PrintCharAt('(') && PrintCharAt(')') && LARROW_handler();
	else
		return PrintCharAt('9');
}

DECLARE_ASCII(SemiColon,':',';')
DECLARE_ASCII(Equals,'+','=')
DECLARE_ASCII(Virgule,'|','\\')
DECLARE_ASCII(ReverseQuote,'~','`')

int ASCIILeftBracket()
{
	if (SHIFT_on)
		return PrintCharAt('{') && PrintCharAt('}') && LARROW_handler();
	else
		return PrintCharAt('[') && PrintCharAt(']') && LARROW_handler();
}

int ASCIIRightBracket()
{
	if (SHIFT_on)
		return PrintCharAt('{') && PrintCharAt('}');
	else
		return PrintCharAt('[') && PrintCharAt(']');
}

DECLARE_ASCII(Apostrophe,'"','\'')
DECLARE_ASCII(Comma,'<',',')
DECLARE_ASCII(Period,'>','.')
DECLARE_ASCII(Underscore,'_','-')

//! \todo this keycode [slash/questionmark] also comes from the keypad; keypad version must not react to SHIFT
DECLARE_ASCII(Slash,'?','/')

#undef DECLARE_ASCII

#define DECLARE_KEYPAD(X)	\
int	KEYPAD##X()	\
{	\
	return PrintCharAt(NUMLOCK_KEY);	\
}

//! \todo insert shifted keypad 0 handler
#define NUMLOCK_KEY '0'
DECLARE_KEYPAD(0)
#undef NUMLOCK_KEY

//! \todo End shifted keypad 1 handler
#define NUMLOCK_KEY '1'
DECLARE_KEYPAD(1)
#undef NUMLOCK_KEY

#define NUMLOCK_KEY '2'
DECLARE_KEYPAD(2)
#undef NUMLOCK_KEY

//! \todo Page Down shifted keypad 3 handler
#define NUMLOCK_KEY '3'
DECLARE_KEYPAD(3)
#undef NUMLOCK_KEY

#define NUMLOCK_KEY '4'
DECLARE_KEYPAD(4)
#undef NUMLOCK_KEY

// TODO: ??? handler
#define NUMLOCK_KEY '5'
DECLARE_KEYPAD(5)
#undef NUMLOCK_KEY

#define NUMLOCK_KEY '6'
DECLARE_KEYPAD(6)
#undef NUMLOCK_KEY

//! \todo Home shifted keypad 7 handler
#define NUMLOCK_KEY '7'
DECLARE_KEYPAD(7)
#undef NUMLOCK_KEY

#define NUMLOCK_KEY '8'
DECLARE_KEYPAD(8)
#undef NUMLOCK_KEY

//! \todo Page Up shifted keypad 9 handler
#define NUMLOCK_KEY '9'
DECLARE_KEYPAD(9)
#undef NUMLOCK_KEY

#undef DECLARE_KEYPAD
// shifted numeric keys
int ASCIISpace()
{
	return PrintCharAt(' ');
}

int ASCIIAsterisk()
{
	return PrintCharAt('*');
}

int ASCIIPlus()
{
	return PrintCharAt('+');
}

int ASCIIMinus()
{
	return PrintCharAt('-');
}

// Interpreter functions

#ifdef _WIN32
// Strange keys
int CTRL_on_handler()
{
	CTRL_on = 1;
	return 1;
}

int CTRL_off_handler()
{
	CTRL_on = 0;
	return 1;
}

int SHIFT_on_handler()
{
	SHIFT_on = 1;
	return 1;
}

int SHIFT_off_handler()
{
	SHIFT_on = 0;
	return 1;
}

int ALT_toggle_handler()
{
	ALT_on = 1-ALT_on;
	return 1;
}

int META_DEL()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/1999
	COORD TmpPosition = ScreenBufferState.dwCursorPosition;
	if (TmpPosition<LogicalEnd)
		{
		{
		COORD IdxMinus1 = TmpPosition;
		COORD Idx = IdxMinus1;
		++Idx;
		while(IdxMinus1<LogicalEnd)
			{
			unsigned long ReadWriteCount;
			char Buffer1;
			ReadConsoleOutputCharacter(StdOutputHandle,&Buffer1,1,Idx,&ReadWriteCount);
			WriteConsoleOutputCharacter(StdOutputHandle,&Buffer1,1,IdxMinus1,&ReadWriteCount);
			++Idx;
			++IdxMinus1;
			};
		}
		--LogicalEnd;
		};
	return 1;
}

int BACKSPACE_handler()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/1999
	if (LogicalOrigin<ScreenBufferState.dwCursorPosition)
		{
		LARROW_handler();
		return META_DEL();
		};
	return 1;
}

//! \todo This keycode is also from the numeric keypad.  If from keypad, and NUMLOCK, then
//! should print a period.
int DEL_handler()
{	// MAIN CODE: FORMALLY CORRECT 4/22/1999
	return META_DEL();
}

//! This is the default getline handler for the CmdShell object in LexParse's InitializeFranciInterpreter
void GetLineFromKeyboardHook(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/5/2004
	FinalExtendInputBuffer(LogicalEnd-LogicalOrigin,LogicalOrigin,InputBuffer);
	if (NULL!=InputBuffer && '\x00'==InputBuffer[ArraySize(InputBuffer)-1])
		{
#if ZAIMONI_REALLOC_TO_ZERO_IS_NULL
		InputBuffer = REALLOC(InputBuffer,_msize(InputBuffer)-sizeof(char));
#else
#error need to handle non-NULL realloc(x,0);
#endif
		InputBufferLength--;
		}
}

void RET_handler_core()
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2005
	LogicalOrigin.X = 0;
	while(LowerRightCornerConsole.Y<=LogicalEnd.Y+1)
		Console::ScrollUserScreenOneLine();		
	LogicalOrigin.Y = LogicalEnd.Y+2;
	LogicalEnd = LogicalOrigin;
	SetConsoleCursorPosition(StdOutputHandle,LogicalOrigin);
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	GetConsoleCursorInfo(StdOutputHandle,&CursorState);
}

KeyMouseResponse* InterpretKeyDown(KEY_EVENT_RECORD& Target)
{	// METAPHOR: CTRL-ALT; SHIFT/CAPSLOCK
	// prefilter code: invariant keys
	switch(Target.wVirtualKeyCode)
	{
	case '\x10': return SHIFT_on_handler;	// shift keys
	case '\x11': return CTRL_on_handler;	// CTRL
	// HACK
	// for some reason, Win95 sends ALT-down when ALT-up is physically what happens
	// this code relies on an OS bug.  If no keys are pressed between ALT-down and ALT-up,
	// the meta-code calling this routine will not see ALT-up.
	case '\x12': return ALT_toggle_handler;	// alt keys
	case 0x14: return NULL;					//! \todo CAPSLOCK is changing.  Sequence is 14h 0h 0h 14h
	case 0x20: return ASCIISpace;
	case 0x90: return NULL;					//! \todo PAUSE key: Franci is about to go to
											//! sleep for a while.  She wakes up with CTRL-SysRq
											//! alternatively, NUMLOCK is changing [sequence is 90h 0h 0h 90h]
											//! system automatically updates NUMLOCK_on
	}

	if (CTRL_on || ALT_on) return NULL;		// general screen against CTRL-ALT characters

	// virtual scan code router
	switch(Target.wVirtualKeyCode)
	{
	case 'A': return ASCIIA;	// letters
	case 'B': return ASCIIB;
	case 'C': return ASCIIC;
	case 'D': return ASCIID;
	case 'E': return ASCIIE;
	case 'F': return ASCIIF;
	case 'G': return ASCIIG;
	case 'H': return ASCIIH;
	case 'I': return ASCIII;
	case 'J': return ASCIIJ;
	case 'K': return ASCIIK;
	case 'L': return ASCIIL;
	case 'M': return ASCIIM;
	case 'N': return ASCIIN;
	case 'O': return ASCIIO;
	case 'P': return ASCIIP;
	case 'Q': return ASCIIQ;
	case 'R': return ASCIIR;
	case 'S': return ASCIIS;
	case 'T': return ASCIIT;
	case 'U': return ASCIIU;
	case 'V': return ASCIIV;
	case 'W': return ASCIIW;
	case 'X': return ASCIIX;
	case 'Y': return ASCIIY;
	case 'Z': return ASCIIZ;
	// digits
	case '0': return ASCII0;	// 0, )
	case '1': return ASCII1;	// 1, !
	case '2': return ASCII2;	// 2, @
	case '3': return ASCII3;	// 3, #
	case '4': return ASCII4;	// 4, $
	case '5': return ASCII5;	// 5, %
	case '6': return ASCII6;	// 6, ^
	case '7': return ASCII7;	// 7, &
	case '8': return ASCII8;	// 8, *
	case '9': return ASCII9;	// 9, (
	// strangely encoded keys -- main keyboard
	case 0x08: return BACKSPACE_handler;// backspace handler
	case 0x09: return NULL;	//! \todo tab handler
	case 0x0d: return ReturnHandler;	// RET handler (parses entire entry and reformats)
	case 0x1b: return NULL;	//! \todo ESC handler
	case 0xba: return ASCIISemiColon;	// ;, :
	case 0xbb: return ASCIIEquals;		// =, +
	case 0xbc: return ASCIIComma;		// ,, <
	case 0xbd: return ASCIIUnderscore;	// -, _
	case 0xbe: return ASCIIPeriod;		// ., >
	case 0xbf: return ASCIISlash;		// /, ?
	case 0xc0: return ASCIIReverseQuote;	// `, ~
	case 0xdb: return ASCIILeftBracket;		// [, {
	case 0xdc: return ASCIIVirgule;			// \, |
	case 0xdd: return ASCIIRightBracket;	// ], }
	case 0xde: return ASCIIApostrophe;		// ', "
	// strangely encoded keys: standard keypad
	case 0x60: return KEYPAD0;
	case 0x61: return KEYPAD1;
	case 0x62: return KEYPAD2;
	case 0x63: return KEYPAD3;
	case 0x64: return KEYPAD4;
	case 0x65: return KEYPAD5;
	case 0x66: return KEYPAD6;
	case 0x67: return KEYPAD7;
	case 0x68: return KEYPAD8;
	case 0x69: return KEYPAD9;
	case 0x6a: return ASCIIAsterisk;
	case 0x6b: return ASCIIPlus;
	case 0x6d: return ASCIIMinus;
	// strangely encoded keys: word-processor block
	case 0x21: return NULL;	//! \todo page up handler
	case 0x22: return NULL;	//! \todo page down handler
	case 0x23: return END_handler;	// end handler handler
	case 0x24: return HOME_handler;	// home handler
	case 0x25: return LARROW_handler;	// left arrow handler
	case 0x26: return UARROW_handler;	// up arrow handler
	case 0x27: return RARROW_handler;	// right arrow handler
	case 0x28: return DARROW_handler;	// down arrow handler
	case 0x2d: return NULL;	//! \todo insert handler
	case 0x2e: return DEL_handler;	// del handler.  This keycode is also from the numeric keypad
	// strangely encoded keys: function keys, F1-F12 in order
	case 0x70: return NULL;
	case 0x71: return NULL;
	case 0x72: return NULL;
	case 0x73: return NULL;
	case 0x74: return NULL;
	case 0x75: return NULL;
	case 0x76: return NULL;
	case 0x77: return NULL;
	case 0x78: return NULL;
	case 0x79: return NULL;
	case 0x7a: return NULL;
	case 0x7b: return NULL;
	default:   return NULL;	
	}
}			

KeyMouseResponse* InterpretKeyUp(KEY_EVENT_RECORD& Target)
{
	switch(Target.wVirtualKeyCode)
	{
	case '\x11': return CTRL_off_handler;
	case '\x10': return SHIFT_off_handler;
	default: return NULL;
	};
}

KeyMouseResponse* InterpretMouse(MOUSE_EVENT_RECORD& Target)
{
	return NULL;
}
#endif

int Console::LookAtConsoleInput()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004 [Windows]
#ifdef _WIN32
	unsigned long EventsReadIn;
	GetNumberOfConsoleInputEvents(StdInputHandle,&EventsReadIn);
	if (0<EventsReadIn)
		{
		INPUT_RECORD CurrentConsoleIN;
		ReadConsoleInput(StdInputHandle,&CurrentConsoleIN,1,&EventsReadIn);
		switch(CurrentConsoleIN.EventType)
		{
		case KEY_EVENT:;
			if (CurrentConsoleIN.Event.KeyEvent.bKeyDown)		// Key down: take key input
				{	// Key down: take key input
				// Tester code: need to find method of making displayed char agree with key
				// pressed....
				KeyMouseResponse* KeyDownHandler = InterpretKeyDown(CurrentConsoleIN.Event.KeyEvent);
				// SHIFT_off, CTRL_off are transparent
				if (   SHIFT_on_handler==KeyDownHandler
					|| CTRL_on_handler==KeyDownHandler)
					return KeyDownHandler();
				if (LastHandlerUsed!=KeyDownHandler)
					{
					LastHandlerUsed = KeyDownHandler;
					if (NULL!=KeyDownHandler)
						return KeyDownHandler();
					}
				}
			else{	// Key up
				// '\n' is important
				KeyMouseResponse* KeyUpHandler = InterpretKeyUp(CurrentConsoleIN.Event.KeyEvent);
				// SHIFT_off, CTRL_off are transparent
				if (   SHIFT_off_handler==KeyUpHandler
					|| CTRL_off_handler==KeyUpHandler)
					return KeyUpHandler();
				if (LastHandlerUsed!=KeyUpHandler)
					{
					LastHandlerUsed = KeyUpHandler;
					if (NULL!=KeyUpHandler)
						return KeyUpHandler();
					}
				}
			return 1;
		case MOUSE_EVENT:;
			// TODO: mouse click in user area means 'relocate the cursor'
			// TODO: click&drag selects text for overwrite by next key
			{
			KeyMouseResponse* MouseHandler = InterpretMouse(CurrentConsoleIN.Event.MouseEvent);
			if (LastHandlerUsed!=MouseHandler)
				{
				LastHandlerUsed = MouseHandler;
				if (NULL!=MouseHandler)
					return MouseHandler();
				}
			}
			return 1;
		default:;
			return 1;
		}
		};
	return 0;	// false
#endif
}

void Console::ScrollUserScreenOneLine()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/16/1999
#ifdef _WIN32
	SMALL_RECT OriginalRectangle;
	const COORD NewOrigin = {0, UserBarY()+1};
	CHAR_INFO FillCharColor;
	OriginalRectangle.Left = 0;
	OriginalRectangle.Right = ScreenBufferState.dwSize.X;
	OriginalRectangle.Top = NewOrigin.Y+1;
	OriginalRectangle.Bottom = ScreenBufferState.dwSize.Y;
	FillCharColor.Char.AsciiChar = ' ';
	FillCharColor.Attributes = Text_White;

	if (LogicalOrigin.Y>NewOrigin.Y)
		LogicalOrigin.Y--;
	else	// we need to buffer the line about to be scrolled away.
			// Presumably, the user wants it as-is [he has had 11 lines of opportunity to edit....
		ExtendInputBuffer(ScreenBufferState.dwSize.X,NewOrigin);

	LogicalEnd.Y--;
	ScrollConsoleScreenBuffer(StdOutputHandle,&OriginalRectangle,NULL,NewOrigin,&FillCharColor);
	ScreenBufferState.dwCursorPosition.Y--;
	SetConsoleCursorPosition(StdOutputHandle,ScreenBufferState.dwCursorPosition);
	GetConsoleScreenBufferInfo(StdOutputHandle,&ScreenBufferState);
	GetConsoleCursorInfo(StdOutputHandle,&CursorState);	
#endif
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
		const time_t LogStartTime = time(NULL);
		Log(asctime(localtime(&LogStartTime)));
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

// #2: scroll to make space; colorize new space now
static void ScrollToMakeSpace(size_t LineCount, int ColorCode)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/7/2003
#ifdef _WIN32
	SMALL_RECT OriginalRectangle;
	COORD NewOrigin = {0, 0};
	CHAR_INFO FillCharColor;
	OriginalRectangle.Left = 0;
	OriginalRectangle.Right = LowerRightCornerConsole.X;
	OriginalRectangle.Top = (13<LineCount) ? 13 : LineCount;
	OriginalRectangle.Bottom = UserBarY()-1;
	FillCharColor.Char.AsciiChar = ' ';
	FillCharColor.Attributes = ColorCode;
	ScrollConsoleScreenBuffer(StdOutputHandle,&OriginalRectangle,NULL,NewOrigin,&FillCharColor);
#endif
}	

static void line_out(const char* const x, size_t span)
{
	if (0<span)
		{
#ifdef _WIN32
		COORD TargetLocation = {0, 0};
		unsigned long CharsWritten;
		WriteConsoleOutputCharacter(StdOutputHandle,x,
								span,TargetLocation,&CharsWritten);
#endif
		};
}

static void line_out(const char* const x, size_t i, size_t* const LineBreakTable)
{
	if (0<LineBreakTable[i])
		{
#ifdef _WIN32
		COORD TargetLocation = {0, i};
		unsigned long CharsWritten;
		if ('\n'==x[LineBreakTable[i-1]]) ++(LineBreakTable[i-1]);
		if (LineBreakTable[i]>LineBreakTable[i-1])
			{
			const size_t span = LineBreakTable[i]-LineBreakTable[i-1];
			WriteConsoleOutputCharacter(StdOutputHandle,x+LineBreakTable[i-1],
										span,
										TargetLocation,&CharsWritten);
			}
#endif
		};
}

static void MetaSay(const char* x,size_t x_len, int ColorCode)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/21/1999
	if (!x || '\0'== *x || 0==x_len) return;
	Console::Log(x,x_len);	// log the message
	size_t LineBreakTable[UserBarY()];
	// #1: parse how many lines are required
	const size_t LineCount = ParseMessageIntoLines(x,x_len,LineBreakTable,ScreenBufferState.dwSize.X);
	if (0>=LineCount) return;

	// #2: scroll to make space; colorize new space now
	ScrollToMakeSpace(LineCount,ColorCode);
	if (UserBarY()<LineCount)
		// LONG MESSAGE
		line_out("...",3);
	else	// SHORT MESSAGE
		line_out(x,LineBreakTable[0]);

	size_t i = 1;
	do	line_out(x,i,LineBreakTable);
	while(UserBarY()> ++i);
}


void Console::SaysNormal(const char* x,size_t x_len)		// white text
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/1999
	MetaSay(x,x_len,Text_White);
}

void Console::SaysWarning(const char* Message)	// yellow text; consider sound effects
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/1999
	MetaSay(Message,strlen(Message),Text_Yellow);
}

void Console::SaysError(const char* Message)		// red text; consider sound effects
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/1999
	MetaSay(Message,strlen(Message),Text_Red);
} 

// Logging.h hooks
EXTERN_C void _fatal(const char* const B)
{
	Console::SaysError(B);
	Console::EndLogFile();
	MessageBox(NULL,B,"Console",MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);
	while(!Console::LookAtConsoleInput());
	exit(EXIT_FAILURE);
}

EXTERN_C void _fatal_code(const char* const B,int exit_code)
{
	Console::SaysError(B);
	Console::EndLogFile();
	MessageBox(NULL,B,"Console",MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);
	while(!Console::LookAtConsoleInput());
	exit(exit_code);
}

EXTERN_C void _inform(const char* const B, size_t len)
{
	Console::SaysNormal(B,len);
}

#if 0
EXTERN_C void
_inc_inform(const char* const B, size_t len)
{
	// ...
}
#endif

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

