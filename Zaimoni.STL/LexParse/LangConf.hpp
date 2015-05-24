// LangConf.hpp
// configuration class for lexing programming languages
// (C)2009,2010 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_STL_LEXPARSE_LANGCONF_HPP
#define ZAIMONI_STL_LEXPARSE_LANGCONF_HPP 1

#include "../AutoPtr.hpp"
#include "../POD.hpp"
#include "std.h"

namespace zaimoni {

template<class T1, class T2, class T3> struct POD_triple;
struct PP_auxfunc;	// generic preprocessor support; actual preprocessor must provide definition

//! <p>NOTE: it is guaranteed that global and tokenizing filters will not be called to operate on NULL text.

// None_LC, AtomicChar_LC, and Uninterpreted_LC are reserved
// The other 14 bitflags are interpreted in a language-dependent fashion
enum SelfClassify_LC	{
						None_LC		= 0x00,
						AtomicChar_LC	= 0x01,
						Flag1_LC	= 0x02,
						Flag2_LC	= 0x04,
						Flag3_LC	= 0x08,
						Flag4_LC	= 0x10,
						Flag5_LC	= 0x20,
						Flag6_LC	= 0x40,
						Flag7_LC	= 0x80,
						Flag8_LC	= 0x100,
						Flag9_LC	= 0x200,
						Flag10_LC	= 0x400,
						Flag11_LC	= 0x800,
						Flag12_LC	= 0x1000,
						Flag13_LC	= 0x2000,
						Flag14_LC	= 0x4000,
						Uninterpreted_LC = 0x8000
						};

//! From the probably-failed ShellLex module:
//! <p>Token, Source stacks don't appear very relevant: InParse/FlatFile auto-handle this

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

struct _error_location
{
	size_t line;
	size_t position;
	const char* filename;	// not owned
	char* lex_me;			// not truly owned, but manipulated anyway

	void clear()
		{
		line = 1;
		position = 0;
		filename = NULL;
		FREE_AND_NULL(lex_me);
		};
	void new_frame(const char* _filename)
		{
		line = 1;
		position = 0;
		filename = _filename;
		lex_me = NULL;
		};
	void reset_counts()
		{
		line = 1;
		position = 0;
		};
};

class LangConf
{
public:
#ifndef ZAIMONI_FORCE_ISO
	typedef bool Filter(char*& x, const char* filename);
#else
	typedef bool Filter(char*& x, size_t& x_len, const char* filename);
#endif
	typedef size_t Tokenizer(const char* Target);
	typedef size_t PredictTransform(const char* Target, size_t src_len);
	typedef void Transform(char* dest, const char* src, size_t src_len);
	typedef bool Classify(char Target);
	typedef lex_flags ClassifyToken(const char* Target);
	typedef void Cleanup(char*& Target, unsigned long Flags);
private:
	weakautoarray_ptr<Filter*> GlobalFilters;
	weakautoarray_ptr<Filter*> TokenizingFilters;
	weakautoarray_ptr<Tokenizer*> Tokenizers;
	autovalarray_ptr<lex_flags> TokenizerFlags;
	autovalarray_ptr<_error_location> _loc_stack;
	mutable unsigned long error_count;
	mutable unsigned long warning_count;
public:
	_error_location _loc;
	const char* const SingleLineCommenter;
	const char* const MultiLineCommentStart;
	const char* const MultiLineCommentEnd;
	Classify* const IsLegalChar;
	Classify* const IsWordChar;
	ClassifyToken* const WordFlags;
	Cleanup* const RAMOptimize;
	PredictTransform* const EscapeStringLength;
	Transform* const EscapeString;
	PredictTransform* const UnescapeStringLength;
	Transform* const UnescapeString;
	const char* const Quotes;
	const char* const WhiteSpace;
	const char* const AtomicSymbols;
	const zaimoni::POD_pair<const char*,size_t>* const InvariantKeywords;	// C-string, length pairs
	PP_auxfunc* const pp_support;
	const lex_flags NoLexFlags;
	const size_t len_InvariantKeywords;
	const size_t len_SingleLineCommenter;
	const size_t len_MultiLineCommentStart;
	const size_t len_MultiLineCommentEnd;
	const size_t comment_hiding_tokenizers;
	const char Escape;
	const char EscapeEscape;
	const bool EscapeOnlyWithinQuotes;
	const bool BreakTokenOnNewline;

	LangConf(	const char* _singleLineCommenter,
				const char* _multiLineCommentStart,
				const char* _multiLineCommentEnd, Classify* _isLegalChar,
				Classify* _isWordChar, ClassifyToken* _wordFlags,
				Cleanup* _ramOptimize, PredictTransform* _escapeStringLength,
				Transform* _escapeString, PredictTransform* _unescapeStringLength,
				Transform* _unescapeString, const char* _quotes,
				const char* _whiteSpace, const char* _atomicSymbols,
				const zaimoni::POD_pair<const char*,size_t>* _invariantKeywords, size_t _len_InvariantKeywords,
				PP_auxfunc* _pp_support,
				lex_flags _noLexFlags, size_t _comment_hiding_tokenizers, 
				char _escape, char _escapeEscape, bool _escapeOnlyWithinQuotes,
				bool _breakTokenOnNewLine)
	:	error_count(0),
		warning_count(0),
		SingleLineCommenter(_singleLineCommenter),
		MultiLineCommentStart(_multiLineCommentStart),
		MultiLineCommentEnd(_multiLineCommentEnd),
		IsLegalChar(_isLegalChar),
		IsWordChar(_isWordChar),
		WordFlags(_wordFlags),
		RAMOptimize(_ramOptimize),
		EscapeStringLength(_escapeStringLength),
		EscapeString(_escapeString),
		UnescapeStringLength(_unescapeStringLength),
		UnescapeString(_unescapeString),
		Quotes(_quotes),
		WhiteSpace(_whiteSpace),
		AtomicSymbols(_atomicSymbols),
		InvariantKeywords(_invariantKeywords),
		pp_support(_pp_support),
		NoLexFlags(_noLexFlags),
		len_InvariantKeywords(_len_InvariantKeywords),
		len_SingleLineCommenter((NULL==_singleLineCommenter) ? 0 : strlen(_singleLineCommenter)),
		len_MultiLineCommentStart((NULL==_multiLineCommentStart) ? 0 : strlen(_multiLineCommentStart)),
		len_MultiLineCommentEnd((NULL==_multiLineCommentEnd) ? 0 : strlen(_multiLineCommentEnd)),
		comment_hiding_tokenizers(_comment_hiding_tokenizers),
		Escape(_escape),
		EscapeEscape(_escapeEscape),
		EscapeOnlyWithinQuotes(_escapeOnlyWithinQuotes),
		BreakTokenOnNewline(_breakTokenOnNewLine)
		{};

//	~LangConf();	// default OK

	bool IsAtomicSymbol(char Target) const {return NULL!=AtomicSymbols && strchr(AtomicSymbols,Target);};
	bool IsWS(char Target) const {return strchr(WhiteSpace,Target);};
	bool IsWS_NotFirst(char Target) const {return strchr(WhiteSpace+1,Target);};
	void CompactWSAtIdx(char*& Text,size_t Idx) const	// no client
		{
		assert(NULL!=Text);
		assert(Idx<strlen(Text) && IsWS(Text[Idx]));
		_compactWSAtIdx(Text,Idx);
		};
	void CompactToken(char*& Target, lex_flags Flags) const {if (NULL!=RAMOptimize) RAMOptimize(Target,Flags);};
	void ExtractLineFromTextBuffer(char*& Buffer, char*& NewLine) const;

	size_t len_SingleLineComment(const char* const Test) const	// internal only
		{
		assert(NULL!=SingleLineCommenter);
		assert(0!=len_SingleLineCommenter);	
		return _len_SingleLineComment(Test);
		};
	size_t len_MultiLineComment(const char* const Test) const	// internal only
		{
		assert(NULL!=MultiLineCommentStart);
		assert(0!=len_MultiLineCommentStart);	
		assert(NULL!=MultiLineCommentEnd);
		assert(0!=len_MultiLineCommentEnd);	
		return _len_MultiLineComment(Test);
		};
#ifndef ZAIMONI_FORCE_ISO
	void FlattenComments(char*& x)
#else
	void FlattenComments(char*& x, size_t& x_len)
#endif
		{
		assert(x && *x);
		// end-of-file check should have intercepted trailing \n
#ifndef ZAIMONI_FORCE_ISO
		assert('\n'!=x[ArraySize(x)-1]);	
		_flattenComments(x);
#else
		assert('\n'!=x[x_len-1]);
		_flattenComments(x,x_len);
#endif
		};

#ifndef ZAIMONI_FORCE_ISO
	bool ApplyGlobalFilters(char*& x, const char* filename) const;
	bool ApplyTokenizingFilters(char*& x) const;
#else
	bool ApplyGlobalFilters(char*& x, size_t& x_len, const char* filename) const;
	bool ApplyTokenizingFilters(char*& x, size_t& x_len) const;
#endif
private:
	size_t TokenizeCore(const char* Target, lex_flags& Flags) const;
	size_t UnfilteredCommentHidingNextToken(const char* Target, lex_flags& Flags) const;	// internal
public:
	size_t UnfilteredNextToken(const char* Target, lex_flags& Flags) const;
#ifndef ZAIMONI_FORCE_ISO
	size_t NextToken(char*& x, lex_flags& Flags) const;
#else
	size_t NextToken(char*& x, size_t& x_len, lex_flags& Flags) const;
#endif
	
	bool InstallGlobalFilter(Filter* src) { return GlobalFilters.InsertSlotAt(GlobalFilters.size(),src);	};
	bool InstallTokenizingFilter(Filter* src)	{	return TokenizingFilters.InsertSlotAt(TokenizingFilters.size(),src);	};
	bool InstallTokenizer(Tokenizer* src,lex_flags src_flags);

	size_t lex_find(const char* const x, const char* const target) const
		{
		assert(x && *x);
		assert(target && *target);
		return _lex_find(x,strlen(x),target,strlen(target));
		};
	size_t lex_find(const char* const x, const char* const target, size_t target_len) const
		{
		assert(x && *x);
		assert(target && *target);
		assert(target_len<=strlen(target));
		return _lex_find(x,strlen(x),target,target_len);
		};
	size_t lex_find(const char* const x, size_t x_len, const char* const target) const
		{
		assert(x && *x);
		assert(target && *target);
		assert(x_len==strlen(x));
		return _lex_find(x,x_len,target,strlen(target));
		};
	size_t lex_find(const char* const x, size_t x_len, const char* const target, size_t target_len) const
		{
		assert(x && *x);
		assert(target && *target);
		assert(x_len==strlen(x));
		assert(target_len<=strlen(target));
		return _lex_find(x,x_len,target,target_len);
		};
	// usage of POD_triple: offset, token_len, flags
	void line_lex(const char* const x, const size_t x_len, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const
		{
		assert(x && *x);
		assert(x_len==strlen(x));
		_line_lex(x,x_len,pretokenized);
		};

	bool line_lex_find(const char* const x, const char* const target, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const
		{
		assert(x && *x);
		assert(target && *target);
		return _line_lex_find(x,strlen(x),target,strlen(target),pretokenized);
		};
	bool line_lex_find(const char* const x, const char* const target, size_t target_len, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const
		{
		assert(x && *x);
		assert(target && *target);
		assert(target_len<=strlen(target));
		return _line_lex_find(x,strlen(x),target,target_len,pretokenized);
		};
	bool line_lex_find(const char* const x, const size_t x_len, const char* const target, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const
		{
		assert(x && *x);
		assert(target && *target);
		assert(x_len==strlen(x));
		return _line_lex_find(x,x_len,target,strlen(target),pretokenized);
		};
	bool line_lex_find(const char* const x, const size_t x_len, const char* const target, size_t target_len, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const
		{
		assert(x && *x);
		assert(target && *target);
		assert(x_len==strlen(x));
		assert(target_len<=strlen(target));
		return _line_lex_find(x,x_len,target,target_len,pretokenized);
		};
	bool require_padding(char lhs, char rhs) const
		{
		assert(WhiteSpace);
		assert(AtomicSymbols);
		assert(IsWordChar);
		return _require_padding(lhs,rhs);
		};
		
	void NewContext(const char* _filename)	// no clients
		{
		error_count = 0;
		warning_count = 0;
		_loc.clear();
		_loc.filename = _filename;
		};
	void PushFrame(const char* _filename)	// no clients
		{
		_loc_stack.Resize(_loc_stack.size()+1);
		_loc_stack.back() = _loc;
		_loc.new_frame(_filename);
		};
	void PopFrame()	// no clients
		{
		_loc.clear();
		if (!_loc_stack.empty())
			{
			_loc = _loc_stack.back();
			_loc_stack.Shrink(_loc_stack.size()-1);
			}
		};

private:
	void Error(const char* msg, const char* filename, size_t line = 0, size_t position = 0) const;	// internal
	void Error(const char* msg) const {Error(msg,_loc.filename,_loc.line,_loc.position);};			// internal
	size_t _len_SingleLineComment(const char* const Test) const;
	size_t _len_MultiLineComment(const char* const Test) const;
	// returns SIZE_MAX on failure, as 0 is a valid offset
	size_t _lex_find(const char* const x, size_t x_len, const char* const target, size_t target_len) const;
	void _compactWSAtIdx(char*& Text,size_t Idx) const;
#ifndef ZAIMONI_FORCE_ISO
	void _flattenComments(char*& Text);
#else
	void _flattenComments(char*& Text, size_t& TextLength);
#endif
	void _line_lex(const char* const x, const size_t x_len, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const;
	bool _line_lex_find(const char* const x, const size_t x_len, const char* const target, size_t target_len, autovalarray_ptr<POD_triple<size_t,size_t,lex_flags> >& pretokenized) const;
	bool _require_padding(char lhs, char rhs) const;
};

}	// namespace zaimoni

#endif
