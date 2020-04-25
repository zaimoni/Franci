// LangConf.cpp
// configuration class for lexing programming languages
// (C)2009,2010 Kenneth Boyd, license: MIT.txt

#include <memory.h>
#include "LangConf.hpp"
#include "../POD.hpp"
#include "../Pure.C/format_util.h"

using namespace std;
using namespace zaimoni;

//! <br>* global filters 					[FlatFile]
//! <br>* tokenizing filters				[UnparsedText]
//! <br>* tokenizers 						[UnparsedText]

//! <br>* single-line commenter [string]	[FlatFile, UnparsedText]
//! <br>* multi-line commenter start		[FlatFile]
//! <br>* multi-line commenter end			[FlatFile]
//! <br>* wordchar spec [function]			[UnparsedText]
//! <br>* quotes [string]					[FlatFile, UnparsedText]
//! <br>* whitespace [function or string]	[FlatFile, UnparsedText]
//! <br>* atomic symbols [string]			[UnparsedText]
//! <br>* string escape [char]				[FlatFile, UnparsedText]
//! <br>* escape the escape [char]			[FlatFile, UnparsedText]
//! <br>* break token on newline [boolean: controls whether FlatFile's per-line shatter is realistic]
//! <br>NOTE: we can pre-emptively break on newline outside of strings no matter what.
//! <br>NOTE: we can discard pure-whitespace lines no matter what

#ifndef ZAIMONI_FORCE_ISO
static bool _applyFilters(LangConf::Filter** FilterList, const size_t FilterSize, char*& x, const char* filename)
#else
static bool _applyFilters(LangConf::Filter** FilterList, const size_t FilterSize, char*& x, size_t& x_len, const char* filename)
#endif
{	//! \pre FilterList!=NULL
	//! \pre x!=NULL
	size_t i = 0;
#ifndef ZAIMONI_FORCE_ISO
	do	if (!FilterList[i](x,filename) || !x) return false;
#else
	do	if (!FilterList[i](x,x_len,filename) || !x) return false;
#endif
	while(FilterSize> ++i);
	return true;
}

static size_t
_applyTokenizers(LangConf::Tokenizer** Tokenizers,lex_flags* TokenizerFlags,const char* Target,lex_flags& Flags,const size_t strictUB)
{	//! \pre NULL!=Tokenizers
	size_t i = 0;
	do	{
		size_t TokenLength = Tokenizers[i](Target);
		if (0<TokenLength)
			{
			Flags = TokenizerFlags[i];
			return TokenLength;
			}
		}
	while(strictUB> ++i);
	return 0;
}

void LangConf::ExtractLineFromTextBuffer(char*& Buffer, char*& NewLine) const
{
#ifndef ZAIMONI_FORCE_ISO
	size_t BufferLength = _msize(Buffer);
#else
	size_t BufferLength = strlen(Buffer);
#endif
	size_t i = 0;
	if (BreakTokenOnNewline)
		{
		while('\n'!=Buffer[i])
			{
			if (i+2==BufferLength && '\n'==Buffer[i+1])
				{
				Buffer = REALLOC(Buffer,ZAIMONI_LEN_WITH_NULL(--BufferLength));
				ZAIMONI_NULL_TERMINATE(Buffer[BufferLength]);
				}
			if (++i==BufferLength)
				{
				NewLine = Buffer;
				Buffer = NULL;
				return;
				}
			}
		}
	else{
		bool InQuotes = false;
		while('\n'!=Buffer[i] || InQuotes)
			{
			if (i+2==BufferLength && '\n'==Buffer[i+1] && !InQuotes)
				{
				Buffer = REALLOC(Buffer,ZAIMONI_LEN_WITH_NULL(--BufferLength));
				ZAIMONI_NULL_TERMINATE(Buffer[BufferLength]);
				}
			if (strchr(Quotes,Buffer[i]))
				InQuotes = !InQuotes;
			else if ((InQuotes || !EscapeOnlyWithinQuotes) && i+1<BufferLength)
				{
				if (	(   EscapeEscape==Buffer[i]
						 && Escape==Buffer[i+1])
					||	(   Escape==Buffer[i]
						 && strchr(Quotes,Buffer[i+1])))
					i++;
				}
			if (++i==BufferLength)
				{
				NewLine = Buffer;
				Buffer = NULL;
				return;
				}
			}
		}

	size_t NewLineLength = (i<=(BufferLength>>1)) ? i : BufferLength-(i+1);
#if ZAIMONI_REALLOC_TO_ZERO_IS_NULL
	NewLine = REALLOC(NewLine,ZAIMONI_LEN_WITH_NULL(NewLineLength));
#else
	NewLine = (0<NewLineLength) ? REALLOC(NewLine,ZAIMONI_LEN_WITH_NULL(NewLineLength)) : NULL;
#endif
	if (!NewLine) return;
	ZAIMONI_NULL_TERMINATE(NewLine[NewLineLength]);

	Buffer[i] = '\0';
	if (i<=(BufferLength>>1))
		{
		strcpy(NewLine,Buffer);
		memmove(Buffer,&Buffer[i+1],BufferLength-(i+1));
		Buffer = REALLOC(Buffer,ZAIMONI_LEN_WITH_NULL(BufferLength-(i+1)));
		ZAIMONI_NULL_TERMINATE(Buffer[BufferLength-(i+1)]);
		}
	else{
		strcpy(NewLine,&Buffer[i+1]);
		Buffer = REALLOC(Buffer,ZAIMONI_LEN_WITH_NULL(i));
		ZAIMONI_NULL_TERMINATE(Buffer[i]);
		char* AltNewLine = Buffer;
		Buffer = NewLine;
		NewLine = AltNewLine;
		}	
}

#ifndef ZAIMONI_FORCE_ISO
bool LangConf::ApplyGlobalFilters(char*& x, const char* filename) const
#else
bool LangConf::ApplyGlobalFilters(char*& x, size_t& x_len, const char* filename) const
#endif
{
	if (x)
		{	// legal char set is a global filter, but should be fairly late (after comment-stripping)
#ifndef ZAIMONI_FORCE_ISO
		if (!GlobalFilters.empty() && !_applyFilters(GlobalFilters,GlobalFilters.size(),x,filename))
#else
		if (!GlobalFilters.empty() && !_applyFilters(GlobalFilters,GlobalFilters.size(),x,x_len,filename))
#endif
			return !x;
		}
	return true;
}

#ifndef ZAIMONI_FORCE_ISO
bool LangConf::ApplyTokenizingFilters(char*& x) const
#else
bool LangConf::ApplyTokenizingFilters(char*& x, size_t& x_len) const
#endif
{
	if (TokenizingFilters.empty() || !x) return true;
#ifndef ZAIMONI_FORCE_ISO
	return _applyFilters(TokenizingFilters,TokenizingFilters.size(),x,NULL);
#else
	return _applyFilters(TokenizingFilters,TokenizingFilters.size(),x,x_len,NULL);
#endif
}

size_t LangConf::TokenizeCore(const char* Target, lex_flags& Flags) const
{
	if (IsAtomicSymbol(Target[0]))
		{
		Flags = AtomicChar_LC;
		return 1;
		}

	if (NULL!=Tokenizers)
		{
		const size_t TokenLength = _applyTokenizers(Tokenizers,TokenizerFlags,Target,Flags,Tokenizers.size());
		if (0<TokenLength) return TokenLength;
		}

	Flags = None_LC;
	size_t i = 0;
	const size_t TextLength = strlen(Target);
	while(i<TextLength && IsWordChar(Target[i])) ++i;
	if (0<i)
		{
		if (NULL!=WordFlags) Flags = WordFlags(Target);
		return i;
		}
	return 1;
}

size_t
LangConf::UnfilteredCommentHidingNextToken(const char* Target, lex_flags& Flags) const
{
	if (NULL==Target || 0==comment_hiding_tokenizers || NULL==Tokenizers)
		return 0;

	return _applyTokenizers(Tokenizers,TokenizerFlags,Target,Flags,comment_hiding_tokenizers);
}

size_t
LangConf::UnfilteredNextToken(const char* Target, lex_flags& Flags) const
{
	if (NULL==Target) return 0;
	return TokenizeCore(Target,Flags);
}

#ifndef ZAIMONI_FORCE_ISO
size_t LangConf::NextToken(char*& x, lex_flags& Flags) const
#else
size_t LangConf::NextToken(char*& x, size_t& x_len, lex_flags& Flags) const
#endif
{
	if (!x) return 0;
#ifndef ZAIMONI_FORCE_ISO
	if (!TokenizingFilters.empty() && !_applyFilters(TokenizingFilters,TokenizingFilters.size(),x,NULL))
#else
	if (!TokenizingFilters.empty() && !_applyFilters(TokenizingFilters,TokenizingFilters.size(),x,x_len,NULL))
#endif
		return 0;

	return TokenizeCore(x,Flags);
}

bool LangConf::InstallTokenizer(Tokenizer* src,lex_flags src_flags)
{
	const size_t StackSize = Tokenizers.size();
	if (!Tokenizers.InsertSlotAt(StackSize,src)) return false;
	if (!TokenizerFlags.InsertSlotAt(StackSize,src_flags))
		{
		Tokenizers.DeleteIdx(StackSize);
		return false;
		}
	return true;
}

void
LangConf::Error(const char* msg, const char* filename, size_t line, size_t position) const
{
	char Buffer[10];
	++error_count;
	if (NULL==msg) return;
	const size_t msg_length = strlen(msg);
	if (0==msg_length) return;
	size_t leading_space = sizeof(": error: ")-1;
	size_t filename_length = 0;
	if (NULL!=filename)
		{
		filename_length = strlen(filename);
		if (0<filename_length)
			leading_space += filename_length;
		};
	if (0<line)
		{
		z_umaxtoa(line,Buffer,10);
		leading_space += strlen(Buffer)+sizeof(", line ")-1;
		};
	if (0==leading_space)
		{
		INFORM(msg);
		return;
		};
	char* target = reinterpret_cast<char*>(calloc(strlen(msg)+leading_space,1));
	if (NULL==target)
		{
		INFORM(msg);
		return;
		};
	char* tracking_target = target;
	if (NULL!=filename)
		{
		strcpy(target,filename);
		tracking_target += filename_length;
		};
	if (0<line)
		{
		strcpy(tracking_target,", line ");
		tracking_target += sizeof(", line ")-1;
		strcpy(tracking_target,Buffer);
		tracking_target += strlen(Buffer);
		}
	strcpy(tracking_target,": error: ");
	tracking_target += sizeof(": error: ")-1;
	strcpy(tracking_target,msg);
	INFORM(target);
	free(target);
}

size_t LangConf::_len_SingleLineComment(const char* const Test) const
{
	if (0!=strncmp(Test,SingleLineCommenter,len_SingleLineCommenter)) return 0;
	return strcspn(Test,"\n");
}

size_t LangConf::_len_MultiLineComment(const char* const Test) const
{
	if (0!=strncmp(Test,MultiLineCommentStart,len_MultiLineCommentStart)) return 0;
	const char* const end_comment = strstr(Test+len_MultiLineCommentStart,MultiLineCommentEnd);
	if (NULL==end_comment)
		{
		Error("ends in unterminated multi-line comment");
		return strlen(Test);
		}
	return (end_comment-Test)+len_MultiLineCommentEnd;
}

size_t
LangConf::_lex_find(const char* const x, size_t x_len, const char* const target, size_t target_len) const
{
	if (0==x_len) return 0;
	if (0==target_len) return 0;

	lex_flags scratch_flags;
	size_t offset = 0;
	while(offset<x_len)
		{
		const size_t token_len = UnfilteredNextToken(x+offset,scratch_flags);
		if (0==token_len) return 0;
		if (token_len==target_len && !strncmp(x+offset,target,target_len)) return offset;
		offset += token_len;
		}
	return SIZE_MAX;
}

void LangConf::_compactWSAtIdx(char*& Text,size_t Idx) const
{
	size_t TextLength = strlen(Text);
	Text[++Idx]=' ';
	if (Idx<=TextLength && IsWS(Text[Idx+1]))
		{
		const size_t offset = strspn(Text+Idx,WhiteSpace);
		if (0<offset)
			{
			TextLength -= offset;
			if (Idx<TextLength)
				memmove(Text+Idx+1,Text+Idx+1+offset,TextLength-Idx-1);
			Text = REALLOC(Text,TextLength);
			}
		};
}

static bool check_newline(char Test, _error_location& _loc)
{
	if ('\n'==Test)
		{
		++_loc.line;
		_loc.position=0;
		return true;
		}
	return false;
}

#ifndef ZAIMONI_FORCE_ISO
void LangConf::_flattenComments(char*& Text)
#else
void LangConf::_flattenComments(char*& Text, size_t& TextLength)
#endif
{	// note: have to be able to lex
#ifndef ZAIMONI_FORCE_ISO
	size_t TextLength = ArraySize(Text);
#endif
	if (2>=TextLength) return;

	// forward pass
	_loc.reset_counts();
	size_t deduct = 0;
	size_t Idx = 0;
	do	{
		if (check_newline(Text[Idx],_loc)) continue;
		size_t entity_len = len_SingleLineComment(Text+Idx);
		if (0<entity_len)
			{
			if (entity_len+Idx+deduct>TextLength) entity_len = TextLength-(deduct+Idx);
			memmove(Text+Idx,Text+Idx+entity_len,TextLength-(Idx+entity_len+deduct));
			deduct += entity_len;
			}
		else{
			entity_len = len_MultiLineComment(Text+Idx);
			if (0<entity_len)
				{
				if (entity_len+Idx+deduct>TextLength) entity_len = TextLength-(deduct+Idx);
				// ok...must:
				// * count lines
				// * replace with single space
				// * put newlines after next newline
				const size_t lines_spanned = std::count(Text+Idx,Text+Idx+entity_len,'\n');
				if (lines_spanned)
					{	// have to account for lines
					const char* backline = std::find(std::reverse_iterator<char*>(Text+Idx+entity_len),std::reverse_iterator<char*>(Text+Idx),'\n').base();
					const size_t backspan = backline-(Text+Idx);

					memset(Text+Idx,'\n',lines_spanned);
					memset(Text+Idx+lines_spanned,' ',(entity_len-backspan));
					memmove(Text+Idx+lines_spanned+(entity_len-backspan),Text+Idx+entity_len,TextLength-(Idx+entity_len));
					deduct += (backspan-lines_spanned);
					}
				else{	// white-out and let later filters clean up
					memset(Text+Idx,' ',entity_len);
					Idx += entity_len-1;
					};
				}
			else{
				lex_flags discard;
				size_t c_string_char_literal_len = UnfilteredCommentHidingNextToken(Text+Idx,discard);
				if (0<c_string_char_literal_len) Idx += c_string_char_literal_len-1;
				}
			}
		}
	while(TextLength-deduct>++Idx);
	if (0<deduct)
		{
		Text = REALLOC(Text,ZAIMONI_LEN_WITH_NULL(TextLength-=deduct));
		ZAIMONI_NULL_TERMINATE(Text[TextLength]);
		}
	return;
}

void
LangConf::_line_lex(const char* const x, const size_t x_len, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const
{
	if (NULL==x) return;
	if (0==x_len) return;

	// doing two passes to guarantee correct memory allocation
	size_t token_count = 0;
	size_t offset = 0;
	lex_flags scratch_flags = 0;
	while(offset<x_len)
		{
		const size_t skip_ws = strspn(x+offset,WhiteSpace);
		if (x_len-offset<=skip_ws) break;
		offset += skip_ws;

		++token_count;

		const size_t token_len = UnfilteredNextToken(x+offset,scratch_flags);
		if (x_len-offset<=token_len) break;
		offset += token_len;
		};
	pretokenized.resize(token_count);

	offset = 0;
	token_count = 0;
	while(offset<x_len)
		{
		const size_t skip_ws = strspn(x+offset,WhiteSpace);
		if (x_len-offset<=skip_ws) break;
		offset += skip_ws;

		pretokenized[token_count].first = offset;
		pretokenized[token_count].second = UnfilteredNextToken(x+offset,pretokenized[token_count].third);
		if (x_len-offset<=pretokenized[token_count].second) break;
		offset += pretokenized[token_count++].second;
		};
}

bool
LangConf::_line_lex_find(const char* const x, const size_t x_len, const char* const target, size_t target_len, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const
{
	if (NULL==x) return false;
	if (0==x_len) return false;
	if (NULL==target) return false;
	if (0==target_len) return false;

	// doing two passes to guarantee correct memory allocation
	size_t token_count = 0;
	size_t offset = 0;
	lex_flags scratch_flags = 0;
	bool found_target = false;
	while(offset<x_len)
		{
		const size_t skip_ws = strspn(x+offset,WhiteSpace);
		if (x_len-offset<=skip_ws) break;
		offset += skip_ws;

		const size_t token_len = UnfilteredNextToken(x+offset,scratch_flags);
		++token_count;
		if (token_len==target_len && !strncmp(target,x+offset,target_len)) found_target = true;
		if (x_len-offset<=token_len) break;
		offset += token_len;
		};
	if (!found_target) return false;
	pretokenized.resize(token_count);

	offset = 0;
	token_count = 0;
	while(offset<x_len)
		{
		const size_t skip_ws = strspn(x+offset,WhiteSpace);
		if (x_len-offset<=skip_ws) break;
		offset += skip_ws;

		pretokenized[token_count].first = offset;
		pretokenized[token_count].second = UnfilteredNextToken(x+offset,pretokenized[token_count].third);
		offset += pretokenized[token_count++].second;
		};
	return true;
}

/*! 
 * test whether two characters will glue two non-whitespace preprocessing tokens into one.
 * This can tolerate false positives, but not false negatives.
 * 
 * \param lhs: left-hand character
 * \param rhs: right-hand character
 * 
 * \return bool true iff they will glue tokens
 */
bool LangConf::_require_padding(char lhs, char rhs) const
{
	if (strchr(WhiteSpace+1,lhs)) return false;	// whitespace is fine
	if (strchr(WhiteSpace+1,rhs)) return false;
	if (strchr(AtomicSymbols,lhs)) return false;	// atomic characters are fine
	if (strchr(AtomicSymbols,rhs)) return false;
	if ('\''==lhs || '"'==lhs) return false;	// string/character literals are fine
	if ('\''==rhs || '"'==rhs) return false;
	// word-chars glue to word-chars
	// symbol-chars glue to symbol-chars
	// universal-char-names will glue as well as normal word-chars
	const bool rhs_word_char = IsWordChar(rhs);
	if (IsWordChar(lhs)) return '\\'==rhs || rhs_word_char;
	return !rhs_word_char;
}

