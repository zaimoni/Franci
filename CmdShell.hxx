// CmdShell.hxx
// Declaration for CmdShell
// NOTE: AddCommandHandler, RemoveCommandHandler, and help should not be used when multi-threaded.
// NOTE: it is safe for the complete handlers to assume NULL!=InputBuffer and '\x00'!=InputBuffer[0]

#if !defined(CMDSHELL_HXX)
#define CMDSHELL_HXX 1

// how many bytes are needed for null-termination of dynamically allocated strings
// Use 1 if you have a generic C memory manager
// Use 0 if your C memory manager does null-initialization of at least one byte as a terminating guard block
#ifndef CMD_SHELL_STRING_NULL_TERMINATION_LENGTH
#define CMD_SHELL_STRING_NULL_TERMINATION_LENGTH 0
#endif

typedef void CmdShellHandler(char*& InputBuffer);
typedef void CmdShellIOHook(const char* Text);
typedef void CmdShellHelp(void);
typedef bool CmdShellHandler2(char*& InputBuffer);
typedef bool CmdShellHandler3(bool defaultreturn, char*& InputBuffer);

class CmdShell
{
protected:
	signed long command_index[(unsigned char)(-1)+1];
	signed long command_index_end[(unsigned char)(-1) +1];
	char** command_names;					// owned
	CmdShellHandler2** do_commands;			// not owned
	CmdShellHandler2** complete_commands;	// not owned

	signed long command_count;
	signed long lastcmd_index;	// last non-empty prefix seen
public:
	const char* prompt;
	const char* identchars;
	const char* intro;
	const char* doc_header;
	const char* misc_header;
	const char* undoc_header;
	char ruler;				// default: =
	bool own_prompt;		// tell destructor whether it is safe to deallocate corresponding pointers.
	bool own_identchars;
	bool own_intro;
	bool own_doc_header;
	bool own_misc_header;
	bool own_undoc_header;
	bool quieter_errors;

	CmdShellIOHook* StdOutHook;			// for the help/? pseudocommand
	CmdShellHandler* GetLineHookNoFail;	// get_line_to_interpret
	CmdShellHandler2* GetLineHook;		// get_line_to_interpret
	CmdShellHelp* EmptyLineHook;		// emptyline
	CmdShellHandler* DefaultHookNoFail;	// default
	CmdShellHandler2* DefaultHook;		// default
	CmdShellHandler* PrecmdHook;			// precmd
	CmdShellHandler3* PostcmdHook;		// postcmd
	CmdShellHelp* PreloopHook;			// preloop
	CmdShellHelp* PostloopHook;			// postloop

private:
	// not copyable
	CmdShell(const CmdShell& Source);
	void operator=(const CmdShell& Source);
public:
	CmdShell();
	virtual ~CmdShell();

// specific
	bool SyntaxOK();
	bool AddCommandHandler(const char* new_command_name, CmdShellHandler2* new_do_command, CmdShellHandler2* new_complete_command);
	bool AddCommandHandler(char*& new_command_name, CmdShellHandler2* new_do_command, CmdShellHandler2* new_complete_command);
// TODO: AdjustCommandHandler_do(char*& new_command_name, CmdShellHandler2* new_do_command)
	bool RemoveCommandHandler(const char* kill_command_name);
	bool get_line_to_interpret(char*& InputBuffer);
	void cmdstep(void);	// extension beyond Python cmd: lets us iterate command once
	void cmdloop(void);
	bool onecmd(char*& InputBuffer);
	void emptyline(void);
	bool default_handler(char*& InputBuffer);
	void precmd(char*& InputBuffer);
	bool postcmd(bool logical_continue, char*& InputBuffer);
	void preloop(void);
	void postloop(void);

	const char* lastcmd(void);					// last command
private:
	signed long exact_command_match(char*& InputBuffer) const;
	void help(char*& InputBuffer) const;
};

#endif
