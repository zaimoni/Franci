// CmdShell.cxx
// Implementation of CmdShell

#include "CmdShell.hxx"
#include "Zaimoni.STL/LexParse/CSVTable.hpp"
#include "Zaimoni.STL/string.h"

using namespace std;
using namespace zaimoni;

CmdShell::CmdShell()
{
	command_names = NULL;
	do_commands = NULL;
	complete_commands = NULL;
	lastcmd_index = -1;
	prompt = NULL;
	identchars = NULL;
	intro = NULL;
	doc_header = NULL;
	misc_header = NULL;
	undoc_header = NULL;
	ruler = '=';
	own_prompt = false;
	own_identchars = false;
	own_intro = false;
	own_doc_header = false;
	own_misc_header = false;
	own_undoc_header = false;
	command_count = 0;
	StdOutHook = NULL;
	GetLineHook = NULL;
	EmptyLineHook = NULL;
	DefaultHook = NULL;
	PrecmdHook = NULL;
	PostcmdHook = NULL;
	PreloopHook = NULL;
	PostloopHook = NULL;

	std::fill_n(command_index,255,-1);
	std::fill_n(command_index_end,255,-1);
}

CmdShell::~CmdShell()
{
	lastcmd_index = -1;
	//! \todo: this really should be more reasonable (get rid of the const-cast safely)
	// The naive approach loses bytes to alignment goo
	if (own_prompt) free(const_cast<char*>(prompt));
	if (own_identchars) free(const_cast<char*>(identchars));
	if (own_intro) free(const_cast<char*>(intro));
	if (own_doc_header) free(const_cast<char*>(doc_header));
	if (own_misc_header) free(const_cast<char*>(misc_header));
	if (own_undoc_header) free(const_cast<char*>(undoc_header));

	if (0<command_count)
		{
		free(do_commands);

		autotransform(command_names,command_names+command_count,free);
		free(command_names);
		}
}

bool CmdShell::SyntaxOK()
{	//! \todo IMPLEMENT
	return true;
}

// NOTE: must have valid name
// must have at least one of do or help
// command_name will be owned by CmdShell on success, not on failure
// the handlers won't be
// You probably want to insert the handlers in alphabetical order if you have a choice.
// The commands 'help' and '?' are reserved: special handling
bool
CmdShell::AddCommandHandler(const char* new_command_name, CmdShellHandler2* new_do_command, CmdShellHandler2* new_complete_command)
{
	char* new_command_name2 = _new_buffer_uninitialized<char>(strlen(new_command_name)+CMD_SHELL_STRING_NULL_TERMINATION_LENGTH);
	if (NULL==new_command_name2) return false;
	strcpy(new_command_name2,new_command_name);
	bool Result = AddCommandHandler(new_command_name2,new_do_command,new_complete_command);
	free(new_command_name2);
	return Result;
}

bool
CmdShell::AddCommandHandler(char*& new_command_name, CmdShellHandler2* new_do_command, CmdShellHandler2* new_complete_command)
{
	if (   is_empty_string(new_command_name)
		|| NULL==new_do_command
		|| !strcmp(new_command_name,"help")
		|| !strcmp(new_command_name,"?"))
		return false;

	signed long i = command_index[(unsigned char)(new_command_name[0])];
	if (-1!=i)
		{
		const signed long search_bound = command_index_end[(unsigned char)(new_command_name[0])];
		do	if (!strcmp(command_names[i],new_command_name))
				return false;
		while(++i<=search_bound);
		}

	char** new_command_names = reinterpret_cast<char**>(realloc(command_names,sizeof(char*)*(command_count+1)));
	CmdShellHandler2** new_do_commands = reinterpret_cast<CmdShellHandler2**>(realloc(do_commands,sizeof(CmdShellHandler2*)*(command_count+1)));
	CmdShellHandler2** new_complete_commands = reinterpret_cast<CmdShellHandler2**>(realloc(complete_commands,sizeof(CmdShellHandler2*)*(command_count+1)));
	if (NULL==new_command_names || NULL==new_do_commands || NULL==new_complete_commands)
		{
		command_count--;
		if (NULL!=new_command_names) command_names = reinterpret_cast<char**>(realloc(new_command_names,sizeof(char*)*command_count));
		if (NULL!=new_do_commands) do_commands = reinterpret_cast<CmdShellHandler2**>(realloc(new_do_commands,sizeof(CmdShellHandler2*)*command_count));
		if (NULL!=new_complete_commands) complete_commands = reinterpret_cast<CmdShellHandler2**>(realloc(new_complete_commands,sizeof(CmdShellHandler2*)*command_count));
		return false;
		}
	command_names = new_command_names;
	do_commands = new_do_commands;
	complete_commands = new_complete_commands;

	if (0==command_count)
		{
		command_names[0] = new_command_name;
		do_commands[0] = new_do_command;
		complete_commands[0] = new_complete_command;
		command_index_end[(unsigned char)(new_command_name[0])] = 0;
		command_index[(unsigned char)(new_command_name[0])] = 0;
		new_command_name = NULL;
		command_count++;
		return true;
		}

	i = command_count;
	do	if (0>strcmp(command_names[--i],new_command_name) || 0==i)
			{
			if (0<i || 0>strcmp(command_names[i],new_command_name)) i++;
			if (i<command_count)
				{
				const signed long block_length = command_count-i;
				memmove(command_names+i+1,command_names+i,sizeof(char*)*block_length);
				memmove(do_commands+i+1,do_commands+i,sizeof(CmdShellHandler2*)*block_length);
				memmove(complete_commands+i+1,complete_commands+i,sizeof(CmdShellHandler2*)*block_length);
				}
			command_names[i] = new_command_name;
			do_commands[i] = new_do_command;
			complete_commands[i] = new_complete_command;
			signed long j = UCHAR_MAX;
			do	if (-1!=command_index_end[j])
					{
					if (i<=command_index_end[j])
						{
						command_index_end[j]++;
						if (i<command_index[j]) command_index[j]++;
						if (i==command_index[j] && j!=(unsigned char)(new_command_name[0])) command_index[j]++;
						}
					else if (j==(unsigned char)(new_command_name[0]) && i-1==command_index_end[j])
						{
						command_index_end[j]++;
						}
					}
				else if (j==(unsigned char)(new_command_name[0]))
					{
					command_index_end[j] = i;
					command_index[j] = i;
					}
			while(0<=--j);
			if (i<=lastcmd_index) lastcmd_index++;
			break;
			}
	while(0<i);

	new_command_name = NULL;
	command_count++;
	return true;
}

bool
CmdShell::RemoveCommandHandler(const char* kill_command_name)
{
	if (is_empty_string(kill_command_name))
		return quieter_errors;
	signed long i = command_index_end[(unsigned char)(kill_command_name[0])];
	if (-1==i) return quieter_errors;
	const signed long search_bound = command_index[(unsigned char)(kill_command_name[0])];
	do	if (!strcmp(command_names[i],kill_command_name))
			{
			// following code is definitely *NOT* multi-thread-safe....
			free(command_names[i]);
			command_count--;
			if (i<command_count)
				{
				const signed long block_length = command_count-i;
				memmove(command_names+i,command_names+i+1,sizeof(char*)*block_length);
				memmove(do_commands+i,do_commands+i+1,sizeof(CmdShellHandler2*)*block_length);
				memmove(complete_commands+i,complete_commands+i+1,sizeof(CmdShellHandler2*)*block_length);
				}
			if (0<command_count)
				{
				command_names = reinterpret_cast<char**>(realloc(command_names,sizeof(char*)*command_count));
				do_commands = reinterpret_cast<CmdShellHandler2**>(realloc(do_commands,sizeof(CmdShellHandler2*)*command_count));
				complete_commands = reinterpret_cast<CmdShellHandler2**>(realloc(complete_commands,sizeof(CmdShellHandler2*)*command_count));
				}
			else{
				FREE_AND_NULL(command_names);
				FREE_AND_NULL(do_commands);
				FREE_AND_NULL(complete_commands);
				}
			if (lastcmd_index==i) lastcmd_index = -1;
			if (lastcmd_index>i) lastcmd_index--;
			signed long j = UCHAR_MAX;
			do	if (   -1!=command_index_end[j]
					&&  i<=command_index_end[j])
					{
					if (command_index_end[j]==command_index[j])
						{
						command_index_end[j] = -1;
						command_index[j] = -1;						
						}
					else{
						command_index_end[j]--;
						if (i<command_index[j]) command_index[j]--;
						}
					}
			while(0<=--j);
			return true;
			}
	while(--i>=search_bound);
	return quieter_errors;
}

bool
CmdShell::get_line_to_interpret(char*& InputBuffer)
{
	// default: return false (to stop execution), set InputBuffer to NULL;
	// if the configuration hook is defined, strip leading/trailing whitespace
	// cannot do interior spaces because this doesn't handle string literals, etc.
	// Franci needs this
	if (NULL!=GetLineHook || NULL!=GetLineHookNoFail)
		{
		bool Result = false;
		if (NULL!=GetLineHookNoFail)
			{
			GetLineHookNoFail(InputBuffer);
			Result = true;
			}
		else{
			Result = GetLineHook(InputBuffer);
			}
		if (Result && NULL!=InputBuffer)
			{	// strip leading/trailing whitespace
			size_t InitialLength = strlen(InputBuffer);
			size_t Offset = 0;
			while(isspace(InputBuffer[Offset])) Offset++;
			if (Offset==InitialLength)
				{
				FREE_AND_NULL(InputBuffer);
				return true;
				}
			while(isspace(InputBuffer[InitialLength-1])) InitialLength--;
			InitialLength -= Offset;
			memmove(InputBuffer,InputBuffer+Offset,InitialLength);
			InputBuffer[InitialLength] = '\x00';
			InputBuffer = reinterpret_cast<char*>(realloc(InputBuffer,InitialLength+1));
			}
		return Result;
		}
	FREE_AND_NULL(InputBuffer);
	return false;
}

void
CmdShell::cmdstep(void)
{
	char* InputBuffer = NULL;
	preloop();
	if (get_line_to_interpret(InputBuffer))
		{
		precmd(InputBuffer);
		onecmd(InputBuffer);
		}
	postloop();
}

void
CmdShell::cmdloop(void)
{
	char* InputBuffer = NULL;
	preloop();
	do	{
		if (!get_line_to_interpret(InputBuffer)) break;
		precmd(InputBuffer);
		}
	while(onecmd(InputBuffer));
	postloop();
}

// This function *MUST* clean up InputBuffer.
// A command prefix can contain any content other than NUL, but it will not be recognized unless it is space-terminated
// We should have already displayed the original command line.
bool
CmdShell::onecmd(char*& InputBuffer)
{
	signed long match_index = -1;
	if (!is_empty_string(InputBuffer))
		{
		// do command-matching
		signed long InputBufferLength = strlen(InputBuffer);
		if (!strncmp(InputBuffer,"help",4))
			{
			if (4==strlen(InputBuffer))
				{
				FREE_AND_NULL(InputBuffer);
				help(InputBuffer);
				return true;
				}
			else if (' '==InputBuffer[4])
				{
				InputBufferLength -= 5;
				memmove(InputBuffer,InputBuffer+5,InputBufferLength+CMD_SHELL_STRING_NULL_TERMINATION_LENGTH);
				InputBuffer = reinterpret_cast<char*>(realloc(InputBuffer,InputBufferLength+CMD_SHELL_STRING_NULL_TERMINATION_LENGTH));
				help(InputBuffer);
				return true;
				}
			}
		if ('?'==InputBuffer[0])
			{
			if (1==strlen(InputBuffer))
				{
				FREE_AND_NULL(InputBuffer);
				help(InputBuffer);
				return true;
				}
			else if (' '==InputBuffer[1])
				{
				InputBufferLength -= 2;
				memmove(InputBuffer,InputBuffer+2,InputBufferLength+CMD_SHELL_STRING_NULL_TERMINATION_LENGTH);
				InputBuffer = reinterpret_cast<char*>(realloc(InputBuffer,InputBufferLength+CMD_SHELL_STRING_NULL_TERMINATION_LENGTH));
				help(InputBuffer);
				return true;
				}
			}

		match_index = exact_command_match(InputBuffer);
		// try completion
		if (-1==match_index)
			{
			signed long i = command_count-1;
			do	if (    NULL!=complete_commands[i]
					&&  complete_commands[i](InputBuffer))
					// input buffer has been prepared for direct match
					{
					match_index = exact_command_match(InputBuffer);
					break;
					}
			while(--i>=0);
			}
		};

	if (-1!=match_index && lastcmd_index<command_count && NULL!=do_commands[lastcmd_index])
		{
		lastcmd_index = match_index;
		bool TentativeContinue = (do_commands[lastcmd_index])(InputBuffer);
		return postcmd(TentativeContinue,InputBuffer);
		}
	else if (default_handler(InputBuffer))
		{
		FREE_AND_NULL(InputBuffer);
		return true;
		}
	else{	//! \todo error out fatally
		FREE_AND_NULL(InputBuffer);
		return false;
		}
}

void
CmdShell::emptyline(void)
{	// default: do nothing
	if (NULL!=EmptyLineHook) EmptyLineHook();
}

bool
CmdShell::default_handler(char*& InputBuffer)
{	// default: do nothing
	// Franci needs this to do an error message
	if (NULL!=DefaultHookNoFail)
		{
		DefaultHookNoFail(InputBuffer);
		return true;
		}
	if (NULL!=DefaultHook) return DefaultHook(InputBuffer);
	return true;
}

void
CmdShell::precmd(char*& InputBuffer)
{	// default: do nothing
	if (NULL!=PrecmdHook) PrecmdHook(InputBuffer);
}

// NOTE: this *MUST* clean up InputBuffer
bool
CmdShell::postcmd(bool logical_continue, char*& InputBuffer)
{	// default: pass through logical_continue.  Must blank InputBuffer regardless.
	if (NULL!=PostcmdHook) logical_continue = PostcmdHook(logical_continue,InputBuffer);
	FREE_AND_NULL(InputBuffer);
	return logical_continue;
}

void
CmdShell::preloop(void)
{	// default: do nothing
	if (NULL!=PreloopHook) PreloopHook();
}

void
CmdShell::postloop(void)
{	// default: do nothing
	if (NULL!=PostloopHook) PostloopHook();
}

const char*
CmdShell::lastcmd(void)
{
	if (-1==lastcmd_index) return NULL;
	return command_names[lastcmd_index];
}

signed long
CmdShell::exact_command_match(char*& InputBuffer) const
{
	signed long i = command_index_end[(unsigned char)(InputBuffer[0])];
	if (0<=i)
		{
		const signed long search_bound = command_index[(unsigned char)(InputBuffer[0])];
		do	{
			const unsigned long command_length = strlen(command_names[i]);
			if (   NULL!=do_commands[i]
				&& !strncmp(command_names[i],InputBuffer,command_length))
				{
				if (command_length==strlen(InputBuffer))
					{
					FREE_AND_NULL(InputBuffer);
					return i;
					}
				else if (' '==InputBuffer[command_length])
					{
					signed long InputBufferLength = strlen(InputBuffer);
					InputBufferLength -= command_length;
					memmove(InputBuffer,InputBuffer+command_length+1,InputBufferLength+CMD_SHELL_STRING_NULL_TERMINATION_LENGTH);
					InputBuffer = reinterpret_cast<char*>(realloc(InputBuffer,InputBufferLength+CMD_SHELL_STRING_NULL_TERMINATION_LENGTH));
					return i;
					}
				}
			}
		while(--i>=search_bound);
		};
	return -1;
}

// #define FRANCI_WARY 1

// this function *MUST* clean up InputBuffer
// StdOutHook must generate appropriate newlines when given NULL
// StdOutHook must translate newlines appropriately
void
CmdShell::help(char*& InputBuffer) const
{
	const bool RealText = !is_empty_string(InputBuffer);
	{
	CSVTable HelpTable('\t','\x00',true);
#ifdef FRANCI_WARY
	INFORM("HelpTable constructed");
#endif
	//! \todo: more sophisticated handling of where the help file is; need this before Franci knows about the system command.
	if (!HelpTable.RAMTable("Help.csv"))
		{
		StdOutHook("Sorry, Help.csv did not load.");
		return;
		}
	if (!HelpTable.FirstRowIsColumnHeaders())
		{
		StdOutHook("Sorry, couldn't set up column headers from help file.");
		return;
		}
#ifdef FRANCI_WARY
	INFORM("HelpTable loaded");
#endif
	// SQL-like search: SELECT Datum FROM ... WHERE HelpEntry='InputBuffer' AND DataID='AliasFor'
	// (rewrite InputBuffer to Datum.  subst./repeat until not found
	signed long HelpEntryCol = HelpTable.ColumnFromColumnHeader("HelpEntry");
	signed long DataIDCol = HelpTable.ColumnFromColumnHeader("DataID");
	signed long DatumCol = HelpTable.ColumnFromColumnHeader("Datum");
	size_t ResultRow = 0;
#ifdef FRANCI_WARY
	INFORM("Columns initialized");
#endif

	if (RealText)
		{
#ifdef FRANCI_WARY
		INFORM("Starting Alias check");
#endif
		while(HelpTable.SELECT_MinTerm(HelpEntryCol,InputBuffer,streq,DataIDCol,"AliasFor",streq,ResultRow))
			{
#ifdef FRANCI_WARY
			INFORM("Iterating Alias check");
#endif
			HelpTable.CellText(ResultRow,DatumCol,InputBuffer);	//! \todo IMPLEMENT DestructiveCellText
			HelpTable.DeleteRow(ResultRow);
			}
		}
#ifdef FRANCI_WARY
	INFORM("Past Alias check");
#endif
	// destructive-filter table entries that aren't relevant
	if (RealText && HelpTable.SELECT_Filter(HelpEntryCol,InputBuffer,streq))
		{
#ifdef FRANCI_WARY
		INFORM("Found entry");
#endif
		HelpTable.DiscardColumn(HelpEntryCol);
		signed long DataIDCol = HelpTable.ColumnFromColumnHeader("DataID");
		signed long DatumCol = HelpTable.ColumnFromColumnHeader("Datum");
		const size_t MaxRow = HelpTable.RowCount();
		size_t UsageRow = 0;
		size_t ArityRow = 0;
		size_t DescriptionRow = 0;

		// relay key information: Usage, Arity, Description
		if (HelpTable.SELECT_MinTerm(DataIDCol,"Usage",streq,UsageRow))
			{
#ifdef FRANCI_WARY
			INFORM("Found Usage");
#endif
			do	{
				const char* TextRef = HelpTable.CellText(UsageRow,DatumCol);
				if (NULL!=TextRef)
					{	// proper optimizing compiler would allow strlen("Usage: ") to evaluate at compile-time
					autoarray_ptr<char> OutText(ZAIMONI_LEN_WITH_NULL(7+strlen(TextRef)));
					strcpy(OutText,"Usage: ");
					strcpy(OutText+7,TextRef);
					StdOutHook(OutText);
					}
				}
			while(++UsageRow<MaxRow && streq("Usage",HelpTable.CellText(UsageRow,DataIDCol)));
			}
		if (HelpTable.SELECT_MinTerm(DataIDCol,"Arity",streq,ArityRow))
			{
#ifdef FRANCI_WARY
			INFORM("Found Arity");
#endif
			do	{
				const char* TextRef = HelpTable.CellText(ArityRow,DatumCol);
				if (NULL!=TextRef)
					{	// proper optimizing compiler would allow strlen("Arity: ") to evaluate at compile-time
					autoarray_ptr<char> OutText(ZAIMONI_LEN_WITH_NULL(7+strlen(TextRef)));
					strcpy(OutText,"Arity: ");
					strcpy(OutText+7,TextRef);
					StdOutHook(OutText);
					}
				}
			while(++ArityRow<MaxRow && streq("Arity",HelpTable.CellText(ArityRow,DataIDCol)));
			}
		if (HelpTable.SELECT_MinTerm(DataIDCol,"Description",streq,DescriptionRow))
			{
#ifdef FRANCI_WARY
			INFORM("Found Description");
#endif
			do	StdOutHook(HelpTable.CellText(DescriptionRow,DatumCol));
			while(++DescriptionRow<MaxRow && streq("Description",HelpTable.CellText(DescriptionRow,DataIDCol)));
			}
		}
	else{	// print summary 
			// commands with help
			// general help topics
			// undocumented commands
#ifdef FRANCI_WARY
		INFORM("Printing Summary");
#endif
		char RulerBar[10];
		memset(RulerBar,ruler,9);
		RulerBar[9] = '\x00';

		const char** SetImageArray = NULL;

		if (NULL!=doc_header) StdOutHook(doc_header);
		StdOutHook(RulerBar);
		if (HelpTable.set_grep(command_names,HelpEntryCol,streq,SetImageArray))
			{	// commands with help
#ifdef FRANCI_WARY
			INFORM("Past set_grep, OK");
#endif
			signed long i = ArraySize(SetImageArray);
#ifdef FRANCI_WARY
			LOG(i);
#endif
			do	StdOutHook(SetImageArray[--i]);
			while(0<i);
			}
		StdOutHook(NULL);
		StdOutHook(NULL);

		if (NULL!=misc_header) StdOutHook(misc_header);
		StdOutHook(RulerBar);
		if (HelpTable.set_invgrep_B_A(command_names,HelpEntryCol,streq,SetImageArray))
			{	// general help topics
#ifdef FRANCI_WARY
			INFORM("Past set_invgrep_B_A, OK");
#endif
			signed long i = ArraySize(SetImageArray);
#ifdef FRANCI_WARY
			LOG(i);
#endif
			do	StdOutHook(SetImageArray[--i]);
			while(0<i);
			}
		StdOutHook(NULL);
		StdOutHook(NULL);

		if (NULL!=undoc_header) StdOutHook(undoc_header);
		StdOutHook(RulerBar);
		if (HelpTable.set_invgrep_A_B(command_names,HelpEntryCol,streq,SetImageArray))
			{	// 
#ifdef FRANCI_WARY
			INFORM("Past set_invgrep_A_B, OK");
#endif
			signed long i = ArraySize(SetImageArray);
#ifdef FRANCI_WARY
			LOG(i);
#endif
			do	StdOutHook(SetImageArray[--i]);
			while(0<i);
			}
		StdOutHook(NULL);
		StdOutHook(NULL);
		free(SetImageArray);
		}
	}
	FREE_AND_NULL(InputBuffer);
}

#undef FRANCI_WARY
