// console.hpp
// header for Console
// this is OS-sensitive

#ifndef ZAIMONI_STL_CONSOLE_HPP
#define ZAIMONI_STL_CONSOLE_HPP 1

#include <iosfwd>
#include <string.h>
#include "../custom_scoped_ptr.hpp"

// returns C-true iff successful
typedef int KeyMouseResponse(void);
typedef KeyMouseResponse* LowLevelInterpretKey(int X);

// must be configurable because it routes commands
// define in module implementing Console subclass
extern KeyMouseResponse* ReturnHandler;

// use following in custom ReturnHandler; handles screen IO issues
void RET_handler_core(void);

// use following in overriding getline hook for CmdShell
bool GetLineForScriptHook(char*& InputBuffer);

// use following as standard getline hook for CmdShell
void GetLineFromKeyboardHook(char*& InputBuffer);

// this is an intentional singleton class
// also, linking in Console provides the Zaimoni.STL core logging functions
// this delegates the logfile manipulations to Pure.C/logging.h|c
class Console
{
public:
	Console();
	virtual ~Console();	// use this as out-of-line anchor method for vtable

	void UseScript(const char* ScriptName);
	void CleanLogFile();
	void StartLogFile();

	static void EndLogFile();
	static int LookAtConsoleInput();

	// this does not log the text for automated testing
	static void Whisper(const char* x);

	// these four log the text
	static void SaysNormal(const char* x);	// white text
	static void SaysWarning(const char* x);	// yellow text; consider sound effects
	static void SaysError(const char* x);	// red text; consider sound effects

	static void ResumeLogFile();

	static void Log(const char* x,size_t x_len);
	static void LogUserInput(const char* x,size_t x_len);
	static void Log(const char* x) {Log(x,strlen(x));};
	static void LogUserInput(const char* x) {LogUserInput(x,strlen(x));};
protected:
	// next three must be implemented by derived classes
	virtual void LinkInScripting() {};
	virtual bool ScanForStartLogFileBlock() {return false;};
	virtual void ShrinkBlock(unsigned long StartBlock, unsigned long EndBlock, unsigned long& ReviewedPoint,size_t LogLength) {};

	void CopyBlock(unsigned long StartBlock, unsigned long EndBlock, unsigned long& ReviewedPoint);

	// data members
	zaimoni::custom_scoped_ptr<std::ifstream> CleanLog;			// infile for log cleaning
	zaimoni::custom_scoped_ptr<std::ofstream> ImageCleanLog;	// outfile for log cleaning

	const char* LogfileInName;	// name of infile for log cleaning; not owned
	const char* LogfileOutName;	// name of outfile for log cleaning; not owned

	// messages; not owned by Console
	// initialized by subclasses, this just NULLs them
	const char* AppName;
	const char* LogfileAppname;
	const char* ScriptUnopened;
	const char* LogUnopened;
	const char* LogAlreadyOpened;
	const char* LogOpened;
	static const char* LogClosed;
	static const char* LogAlreadyClosed;
	const char* OS_ID;
	static const char* SelfLogSign;
	static const char* UserLogSign;
};

// support enhanced warning display
/* overloadable adapters for C++ and debug-mode code */
/* all-uppercased because we may use macro wrappers on these */
void SEVERE_WARNING(const char* const B);	// useful for Console class
void WARNING(const char* const B);			// useful for Console class

inline bool WARN(bool A, const char* const B)
{
	if (A)
		{
		WARNING(B);
		return true;
		}
	return false;
}

// might as well provide a pointer to an actual instance to be initialized by the application
extern Console* _console;

#endif
