// Unparsed.cxx
// implementation for UnparsedText

#include "Class.hxx"
#include "LangCon_.hxx"
#include "Unparsed.hxx"
#include "Integer1.hxx"
#include "TruthVal.hxx"
#include "Keyword1.hxx"

#include "Zaimoni.STL/lite_alg.hpp"
#include "Zaimoni.STL/string.h"
#include "Zaimoni.STL/LexParse/LangConf.hpp"

//!\ todo WordProcessing elementary operation support
//! <br>void ZLSTarget(char const* Target, size_t length)	// have this
//! <br>void ZLSTarget(char const* Target)					// have this
//! <br>void ZLSTarget(char Target)							// have this
//! <br>void DeleteNCharsAtIdx(size_t N, size_t Idx)			// have this: frontend for TextSnip; maybe should eliminate TextSnip?
//! <br>void InsertSourceAtIdx(const char const* Source, size_t Idx, size_t length)	// have this
//! <br>void InsertSourceAtIdx(const char const* Source, size_t Idx)					// have this
//! <br>void InsertSourceAtIdx(char Source, size_t Idx)								// have this
//! <br>void OverwriteNthPlaceWith(size_t N, const char const* Source, size_t length)	// have this
//! <br>void OverwriteNthPlaceWith(size_t N, const char const* Source)					// have this
//! <br>void OverwriteNthPlaceWith(size_t N, char Source)								// have this
//! <br>void OverwriteReverseNthPlaceWith(size_t N, const char const* Source, size_t length);
//! <br>void OverwriteReverseNthPlaceWith(size_t N, const char const* Source);
//! <br>void OverwriteReverseNthPlaceWith(size_t N, char Source);	// have this
//! <br>void ReplaceTargetWithSource(const char const* Source, size_t SourceLength, const char const* Target, size_t TargetLength)
//! <br>void ReplaceTargetWithSource(const char const* Source, size_t SourceLength, const char const* Target)
//! <br>void ReplaceTargetWithSource(const char const* Source, const char const* Target, size_t TargetLength)
//! <br>void ReplaceTargetWithSource(const char const* Source, const char const* Target)
//! <br>void ReplaceTargetWithSource(const char const* Source, size_t SourceLength, char Target)
//! <br>void ReplaceTargetWithSource(const char const* Source, char Target)
//! <br>void ReplaceTargetWithSource(char Source, const char const* Target)	// have this
//! <br>void ReplaceTargetWithSource(char Source, char Target)		// have this

// OPTIMIZATION NOTES:
// This type is important at the lexing stage.  Time penalties here are one-shot.
// Anything called from UnparsedText::SplitIntoTwoTexts is important
// UnparsedText::CanEvaluateToSameType and UnparsedText::DestructiveEvaluateToSameType are important

// design:
// want to foolproof updating the system to parse more elemental types
// so...
// function pointers:
//* bool __(const char*) for can-evaluate
//* bool __(MetaConcept*&, const char) for evaluation
//* then just loop through arrays

zaimoni::weakautoarray_ptr<UnparsedText::Recognize*> UnparsedText::EvalRecognizers;
zaimoni::weakautoarray_ptr<UnparsedText::Parse*> UnparsedText::EvalParsers;

inline bool IsNumericChar(unsigned char Test) {return 10U>((unsigned int)Test-(unsigned int)'0');}

bool
IsUnaccentedAlphabeticChar(unsigned char Test)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	if (   in_range<'A','Z'>(Test)
		|| in_range<'a','z'>(Test))
		return true;
	return false;
}

bool
IsAlphabeticChar(unsigned char Test)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/27/2001
	// META: uses ASCII/default ISO web encoding implicitly
	// NOTE: lower-case eth (240) will pass as partial differential operator!
	if (   IsUnaccentedAlphabeticChar(Test)
//		|| (unsigned char)('\x8c')==Test				// OE ligature
//		|| (unsigned char)('\x9c')==Test				// oe ligature
//		|| (unsigned char)('\x9f')==Test				// Y umlaut
		|| ((unsigned char)('\xc0')<=Test && (unsigned char)('\xd6')>=Test)	// various accented characters
		|| ((unsigned char)('\xd8')<=Test && (unsigned char)('\xf6')>=Test)	// various accented characters
		|| ((unsigned char)('\xf8')<=Test /* && (unsigned char)('\xff')>=Test */))	// various accented characters
		return true;
	return false;
}

bool
IsQuasiNumericIDChar(char Test)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	if (   IsNumericChar(Test)
		|| IsAlphabeticChar(Test)
		||  '_'==Test)
		return true;
	return false;
}

bool
IsQuasiSymbolIDChar(char Test)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/14/2004
	if (   IsQuasiNumericIDChar(Test)
		|| FranciLexer.IsAtomicSymbol(Test))
		return false;
	return true;
}

size_t ScanHTMLStartTag(const char* const Text)
{
	const size_t TextLength = strlen(Text);
	if (   3<=TextLength
		&& '<'==Text[0]
		&& IsUnaccentedAlphabeticChar(Text[1]))
		{
		size_t i=2;
		while(IsUnaccentedAlphabeticChar(Text[i]) && ++i<TextLength-1);
		if 		('>'==Text[i])
			return i+1;
		else if (' '!=Text[i] || i==TextLength-1)
			return 0;
		do	{
			while(' '==Text[i] && ++i<TextLength-1);
			if ('>'==Text[i]) return i+1;
			if (!IsUnaccentedAlphabeticChar(Text[i])) return 0;
			while(IsUnaccentedAlphabeticChar(Text[i]) && ++i<TextLength-1);
			if 		('>'==Text[i])
				return i+1;
			else if ('='==Text[i])
				{
				if 		('"'==Text[i+1] && i<TextLength-4)
					{	// double-quote delimited string
					i +=2;
					while('"'!=Text[i] && ++i<TextLength-2);
					if ('"'!=Text[i] || i==TextLength-1)
						return 0;
					if ('>'==Text[i+1]) return i+2;
					}
				else if ('\''==Text[i+1] && i<TextLength-4)
					{	// single-quote delimited string
					i +=2;
					while('\''!=Text[i] && ++i<TextLength-2);
					if ('\''!=Text[i] || i==TextLength-1)
						return 0;
					if ('>'==Text[i+1]) return i+2;
					}
				else if (i<TextLength-3)
					{	// non-quoted constant (die at ', "; drop out at space)
					while(++i<TextLength-1 && ' '!=Text[i])
						if ('"'==Text[i] || '\''==Text[i])
							return 0;
					if ('>'==Text[i]) return i+1;
					}
				}
			}
		while(' '!=Text[i] || i==TextLength-1);
		}
	return 0;
}

size_t ScanXMLSelfEndingTag(const char* const Text)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/14/2004
	const size_t TextLength = strlen(Text);
	if (   4<=TextLength
		&& '<'==Text[0]
		&& IsUnaccentedAlphabeticChar(Text[1]))
		{
		size_t i = 2;
		while(IsUnaccentedAlphabeticChar(Text[i]) && ++i<TextLength-2);
		if (0==strncmp(Text+i,"/>",2)) return i+2;
		return 0;
		}
	return 0;
}

size_t ScanHTMLTerminalTag(const char* const Text)
{
	const size_t TextLength = strlen(Text);
	if (   4<=TextLength
		&& 0==strncmp(Text,"</",2)
		&& IsUnaccentedAlphabeticChar(Text[2]))
		{
		size_t i = 3;
		while(IsUnaccentedAlphabeticChar(Text[i]) && ++i<TextLength-1);
		if ('>'==Text[i]) return i+1;
		return 0;
		}
	return 0;
}

size_t ScanJSEntityTag(const char* const Text)
{
	const size_t TextLength = strlen(Text);
	if (   3<=TextLength
		&& '&'==Text[0] 
		&& IsUnaccentedAlphabeticChar(Text[1]))
		{
		size_t i = 2;
		do	if (';'==Text[i])
				return i+1;
			else if (!IsUnaccentedAlphabeticChar(Text[i]))
				return 0;
		while(TextLength> ++i);
		}
	return 0;
}

size_t ScanJSCharEntityTag(const char* const Text)
{
	const size_t TextLength = strlen(Text);
	if (   4<=TextLength
		&& 0==strncmp(Text,"&#",2)
		&& IsNumericChar(Text[2]))
		{
		size_t i = 2;
		do	if (';'==Text[i])
				return i+1;
			else if (!IsNumericChar(Text[i]))
				return 0;
		while(TextLength> ++i);
		}
	return 0;
}

size_t ScanQuasiSymbolTag(const char* const Text)
{
	const size_t TextLength = strlen(Text);
	size_t i = 0;
	while(i<TextLength && IsQuasiSymbolIDChar(Text[i]))
		i++;
	if (2<=i)
		{
		if (   ScanJSEntityTag(&Text[i-1])
			|| ScanHTMLStartTag(&Text[i-1])
			|| ScanXMLSelfEndingTag(&Text[i-1]))
			i -= 1;
		else if (3<=i)
			{ 
			if (    ScanHTMLTerminalTag(&Text[i-2])
				 || ScanJSCharEntityTag(&Text[i-2]))
				i -= 2;
			}
		}
	return i;
}

size_t ScanQuasiEnglishNumericTag(const char* const Text)
{
	const size_t TextLength = strlen(Text);
	size_t i = 0;
	while(i<TextLength && IsQuasiNumericIDChar(Text[i])) ++i;
	return i;
}

static inline void
_TextSnip(char*& Text, const size_t Start, const size_t End)
{
	size_t TextLength = strlen(Text);
	TextLength -= End-Start;
	memmove(Text+Start,Text+End,TextLength-Start+1);
	_shrink(Text,TextLength);
}

inline void
UnparsedText::TextSnip(const size_t Start, const size_t End)
// Start: 1st character to delete
// End: 1st character after the last character to delete
{
	_TextSnip(Text,Start,End);
	SpecializeSemantics();
}

//! \todo ALTER MEMORY MANAGEMENT PARADIGM
//! NEW PARADIGM: keywords are *not* owned by UnparsedText, other text is.
//! NEW PARADIGM??: dynamic allocation uses memory-manager autonull character

UnparsedText::UnparsedText(char*& src)
:	MetaConceptZeroArgs(UnparsedText_MC),
	Text(src),
	LogicalLineNumber(0),
	LogicalLineOrigin(0),
	SourceFileName(NULL),
	SelfClassifyBitmap(None_UT)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
	if (!Text.empty()) SpecializeSemantics();
}

UnparsedText::UnparsedText(char*& src,unsigned short NewSelfClassify)
:	MetaConceptZeroArgs(UnparsedText_MC),
	Text(src),
	LogicalLineNumber(0),
	LogicalLineOrigin(0),
	SourceFileName(NULL),
	SelfClassifyBitmap(NewSelfClassify)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
	if (!SelfClassifyBitmap && !Text.empty()) SpecializeSemantics();
}


UnparsedText::UnparsedText(char*& src, bool Interpreted)
:	MetaConceptZeroArgs(UnparsedText_MC),
	Text(src),
	LogicalLineNumber(0),
	LogicalLineOrigin(0),
	SourceFileName(NULL),
	SelfClassifyBitmap((Interpreted) ? None_UT : Uninterpreted_UT)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/30/2005
	if (!SelfClassifyBitmap && !Text.empty()) SpecializeSemantics();
}

UnparsedText& UnparsedText::up_reference(MetaConcept* src)
{
	SUCCEED_OR_DIE(src);
	SUCCEED_OR_DIE(IsType(src->ExactType()));
	return *static_cast<UnparsedText*>(src);
}

const UnparsedText& UnparsedText::up_reference(const MetaConcept* src)
{
	SUCCEED_OR_DIE(src);
	SUCCEED_OR_DIE(IsType(src->ExactType()));
	return *static_cast<const UnparsedText*>(src);
}

UnparsedText& UnparsedText::up_reference(MetaConcept& src)
{
	SUCCEED_OR_DIE(IsType(src.ExactType()));
	return *static_cast<UnparsedText*>(&src);
}

const UnparsedText& UnparsedText::up_reference(const MetaConcept& src)
{
	SUCCEED_OR_DIE(IsType(src.ExactType()));
	return *static_cast<const UnparsedText*>(&src);
}

UnparsedText& UnparsedText::fast_up_reference(MetaConcept* src)
{
	SUCCEED_OR_DIE(IsType(src->ExactType()));
	return *static_cast<UnparsedText*>(src);
}

const UnparsedText& UnparsedText::fast_up_reference(const MetaConcept* src)
{
	SUCCEED_OR_DIE(IsType(src->ExactType()));
	return *static_cast<const UnparsedText*>(src);
}

unsigned int UnparsedText::OpPrecedence() const
{
//	if (IsUnclassified()) return Precedence::None; // parse first
	if (IsLogicalMultiplicationSign()) return Precedence::Multiplication;
	if (IsLogicalPlusSign()) return Precedence::Addition;
	if (IsLogicalEllipsis()) return Precedence::Ellipsis;
	if (IsHTMLStartTag("sup") || IsHTMLStartTag("sub")) return Precedence::LParenthesis;
	if (IsSemanticChar()) {
		if (strchr("([{", Text[0])) return Precedence::LParenthesis;
		if (',' == Text[0]) return Precedence::Comma;
	}
	// backstop: arithmetic + and -
	if (EndsWith('+') || EndsWith('-')) {
		return 1 == Text.size() ? Precedence::Addition : Precedence::UnaryAddition;
	}
	return Precedence::None;
}

// Syntactical equality and inequality
bool UnparsedText::EqualAux2(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	const UnparsedText& VR_rhs = static_cast<const UnparsedText&>(rhs);
	if (NULL==Text) return VR_rhs.Text.empty();
	if (VR_rhs.Text.empty()) return false;
	return 0==strcmp(Text,VR_rhs.Text);
}

bool UnparsedText::InternalDataLTAux(const MetaConcept& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2000
	return 0>strcmp(Text,static_cast<const UnparsedText&>(rhs).Text);
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > UnparsedText::canEvaluate() const // \todo obviate CanEvaluate
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

//  Evaluation functions
bool UnparsedText::CanEvaluate() const
{	// FORMALLY CORRECT: 9/4/2006
	if (!MustBeParsedInContext() && !Text.empty())
		{
		size_t i = EvalRecognizers.size();
		while(0<i) if ((EvalRecognizers[--i])(Text)) return true;
		};
	return CanEvaluateToSameType();
}

//! \todo move common-core to LangConf.hxx
struct HardMacro
	{
	const char* const Trigger;
	const char* const OverwriteTarget;
	size_t IndexForNonIdentifierChar;
	size_t TextSnipArg1;
	size_t TextSnipArg2;
	size_t ShouldOverwrite;
	};

#define FranciScriptHardMacroCount 15;
static const HardMacro FranciScriptHardMacro[]
	=	{
		{"ARE FREE","FREE",8,0,4,0},
		{"IF AND ONLY IF","IFF",14,2,13,0},
		{"IS FREE","FREE",7,0,3,0},
		{"NOT AND","NAND",7,1,4,0},
		{"NOT OR","NOR",6,1,4,0},
		{"NOT IFF","NIFF",7,1,4,0},
		{"NOT XOR","NXOR",7,1,4,0},
		{"NOT NXOR","XOR",8,0,5,0},
		{"NOT NIFF","IFF",8,0,5,0},
		{"NOT NOR","OR",7,0,5,0},
		{"NOT NAND","AND",8,0,5,0},
		{"NOT UNKNOWN","UNKNOWN",11,0,4,0},
		{"NOT TRUE","FALSE",8,0,3,1},
		{"NOT FALSE","TRUE",9,0,5,1},
		{"NOT CONTRADICTION","CONTRADICTION",17,0,4,0}
		};

static bool CanApplyHardFranciMacrosToImmediateText(const char* Buffer)
{
	const size_t TextLength = strlen(Buffer);
	size_t MacroIdx = FranciScriptHardMacroCount;
	do	{
		const HardMacro& Tmp = FranciScriptHardMacro[--MacroIdx];
		if (   0==strncmp(Buffer,Tmp.Trigger,Tmp.IndexForNonIdentifierChar)
			&& (	 TextLength==Tmp.IndexForNonIdentifierChar
				|| 	(TextLength>Tmp.IndexForNonIdentifierChar && !IsQuasiNumericIDChar(Buffer[Tmp.IndexForNonIdentifierChar]))))
			return true;
		}
	while(0<MacroIdx);
	return false;
}

// tokenizing filter for FranciLexer
bool ApplyHardFranciMacrosToImmediateText(char*& Buffer, const char* filename)
{
	const size_t TextLength = strlen(Buffer);
	size_t MacroIdx = FranciScriptHardMacroCount;
	do	{
		const HardMacro& Tmp = FranciScriptHardMacro[--MacroIdx];
		if (   0==strncmp(Buffer,Tmp.Trigger,Tmp.IndexForNonIdentifierChar)
			&& (	 TextLength==Tmp.IndexForNonIdentifierChar
				|| 	!IsQuasiNumericIDChar(Buffer[Tmp.IndexForNonIdentifierChar])))
			{
			_TextSnip(Buffer,Tmp.TextSnipArg1,Tmp.TextSnipArg2);
			if (Tmp.ShouldOverwrite)
				strncpy(Buffer,Tmp.OverwriteTarget,Tmp.IndexForNonIdentifierChar);
			MacroIdx = FranciScriptHardMacroCount;
			}
		}
	while(0<MacroIdx);
	return true;
}

#undef FranciScriptHardMacroCount

bool UnparsedText::CanEvaluateToSameType() const
{	// FORMALLY CORRECT: 12/14/2004
	if (!MustBeParsedInContext() && !Text.empty())
		return CanApplyHardFranciMacrosToImmediateText(Text);
	return false;
}

bool UnparsedText::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/10/1999
	if (!Text.empty()) return 0<strlen(Text);
	return true;
}

bool
UnparsedText::Evaluate(MetaConcept*& dest)		// same, or different type
{	// FORMALLY CORRECT: 9/4/2006
	// Conversion targets: integer numeral, truth value keywords, abstract class names
	if (!MustBeParsedInContext() && !Text.empty())
		{
		size_t i = EvalRecognizers.size();
		while(0<i)
			if ((EvalRecognizers[--i])(Text) && (EvalParsers[i])(dest,Text)) return true;
		};
	return false;
}

bool
UnparsedText::DestructiveEvaluateToSameType()	// overwrites itself iff returns true
{	// FORMALLY CORRECT: 12/14/2004
	if (CanApplyHardFranciMacrosToImmediateText(Text))
		return FranciLexer.ApplyTokenizingFilters(Text);
	return false;
}

void
UnparsedText::SwapWith(UnparsedText& Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/2/2005
	swap(Text,Target.Text);
	swap(SelfClassifyBitmap,Target.SelfClassifyBitmap);
}

bool
UnparsedText::IsQuasiSymbolEndingInMinusSign(void) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2000
	if (IsQuasiSymbol() && '-'==Text[strlen(Text)-1])
		return true;
	return false;
}

bool
UnparsedText::IsQuasiSymbolEndingInPlusSign(void) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2000
	if (IsQuasiSymbol() && '+'==Text[strlen(Text)-1])
		return true;
	return false;
}


bool
UnparsedText::IsSemanticChar(char Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	if (IsSemanticChar() && Text[0]==Target)
		return true;
	return false;
}

bool
UnparsedText::IsLogicKeyword(const char* Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	if (IsLogicKeyword() && !strcmp(Text,Target))
		return true;
	return false;
}

bool 
UnparsedText::IsPredCalcKeyword(const char* Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	if (IsPredCalcKeyword() && !strcmp(Text,Target))
		return true;
	return false;
}

bool
UnparsedText::IsPrefixKeyword(const char* Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	if (IsPrefixKeyword() && !strcmp(Text,Target))
		return true;
	return false;
}

bool
UnparsedText::IsInfixSymbol(const char* Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/7/2000
	if (IsInfixSymbol() && !strcmp(Text,Target))
		return true;
	return false;
}

bool UnparsedText::IsHTMLStartTag(const char* Target) const
{
	if (IsHTMLStartTag() && !Text.empty()) {
		const size_t targetLength = strlen(Target);
		if (   !strnicmp(Text, Target, targetLength)
			&& (strlen(Text) == targetLength || ' ' == Text[targetLength]))
			return true;
	}
	return false;
}

bool UnparsedText::IsHTMLTerminalTag(const char* Target) const
{
	return IsHTMLTerminalTag() && !stricmp(Text,Target);
}

bool UnparsedText::IsJSEntity(const char* Target) const
{
	return IsJSEntity() && !strcmp(Text,Target);
}

bool UnparsedText::IsJSCharEntity(const char* Target) const
{
	return IsJSCharEntity() && !strcmp(Text,Target);
}

bool UnparsedText::IsMultiplicationSymbol() const
{
	return IsJSEntity("middot") || IsInfixSymbol("\xb7");
}

bool
UnparsedText::ArgCannotExtendLeftThroughThis(void) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/23/1999
	// This rule is an auxilliary for Inparse, in determining whether nesting is redundant.
	// __ (|[ "mathobject" ]|)
	// potential function names do *not* have this effect
	if		(IsSemanticChar())
		{	// OK: ( [ , {
		if (NULL!=strchr("([,{",Text[0])) return true;
		}
	else if (IsLogic_Prefix_OrPredCalcKeyword())
		return true;
	else if (IsLogicalEllipsis())
		return true;
	else if (   IsHTMLStartTag("sup")
			 || IsHTMLStartTag("sub"))
		return true;
	return false;
}

bool
UnparsedText::ArgCannotExtendRightThroughThis(void) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/23/1999
	// This rule is an auxilliary for Inparse, in determining whether nesting is redundant.
	// (|[ "mathobject" ]|) __
	// potential function names do *not* have this effect
	// Symbols do *not* have this effect
	if		(IsSemanticChar())
		{	// OK: ) ] , }
		if (NULL!=strchr(")],}",Text[0])) return true;
		}
	else if (IsLogic_Prefix_OrPredCalcKeyword())
		return true;
	else if (IsLogicalEllipsis())
		return true;
	return false;
}

bool UnparsedText::IsLogicalPlusSign() const
{
	return IsInfixSymbol(SymbolPlusSign) || IsJSCharEntity(JSCharEntityPlusSign) || IsJSCharEntity(JSCharEntityPlusSignV2);
}

bool UnparsedText::IsLogicalMultiplicationSign() const
{
	return IsInfixSymbol(SymbolMultSign) || IsJSEntity("middot") || IsJSCharEntity(JSCharEntityMultSign);
}

bool UnparsedText::IsLogicalEllipsis() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/16/2006
	if (   IsInfixSymbol(SymbolEllipsis)	// "..."
		|| IsInfixSymbol(SymbolEllipsis2)	// '\x75'
		|| IsJSEntity("hellip")
		|| IsJSCharEntity(JSCharEntityEllipsis))	// &#133;
		return true;
	return false;
}

size_t
UnparsedText::LengthOfNumericIntegerToSplitOff(void) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/8/1999
	return IntegerNumeral::LengthOfLegalIntegerSubstring(Text);
}

bool UnparsedText::AnyNonAlphabeticChars() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/17/2004
	return string_nand(Text,IsAlphabeticChar);
}

size_t UnparsedText::LengthOfQuasiEnglishNumericID() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/23/1999
	if (IsUnclassified()) {
		const size_t TextLength = strlen(Text);
		size_t i = 0;
		while(i<TextLength && IsQuasiNumericIDChar(Text[i])) ++i;
		return i;
	} else if (IsQuasiEnglishNumeric()) return strlen(Text);
	return 0;
}

size_t UnparsedText::LengthOfQuasiSymbolID() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/7/2000
	if (IsUnclassified()) {
		const size_t TextLength = strlen(Text);
		size_t i = 0;
		while(i<TextLength && IsQuasiSymbolIDChar(Text[i])) ++i;
		return i;
	} else if (IsSymbol()) return strlen(Text);
	return 0;
}

size_t UnparsedText::LengthOfHTMLStartTag() const
{
	return IsHTMLStartTag() ? strlen(Text) + 2 : 0;
}

size_t UnparsedText::LengthOfHTMLTerminalTag() const
{
	return IsHTMLTerminalTag() ? strlen(Text) + 3 : 0;
}

// pattern processing
size_t UnparsedText::CanSplitIntoTwoTexts() const
{
	if (Text.empty()) return false;
	unsigned long Flags = 0;
	const size_t LengthOfText1 = FranciLexer.UnfilteredNextToken(Text, Flags);
	return (0 < LengthOfText1 && strlen(Text) > LengthOfText1) ? LengthOfText1 : 0;
}

bool UnparsedText::SplitIntoTwoTexts(MetaConcept*& Text2)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/22/2002
	// this routine, if it returns false, leaves everything unchanged (we will lose 
	// prior contents of Text2, regardless).
	// if it returns true, the creation has been successful.
	DELETE_AND_NULL(Text2);
	if (NULL==Text) return false;

	// auto-initialize LengthOfText1
	unsigned long Flags = 0;
	size_t LengthOfText1 = FranciLexer.UnfilteredNextToken(Text,Flags);

	size_t TextLength = strlen(Text);
	if (0<LengthOfText1 && TextLength>LengthOfText1)
		{
	// #1: decide which target is smaller: Text1 or Text2
	// #2: make image of smaller one, and create object in Text2 for smaller one.
	// IF STILL HERE, will complete; two cases...
	// #3: Shrink down the larger one (it should be here)
	// #4: if Text2 object is really Text1, swap this with Text2
		size_t EndText1 = LengthOfText1-1;
		size_t StartText2 = LengthOfText1;
		// UnparsedText constructor guarantees non-space is first character
		while(/*0<EndText1 && */ ' '==Text[EndText1]) --EndText1;
		// UnparsedText constructor guarantees non-space is last character
		while(/*TextLength>StartText2 && */ ' '==Text[StartText2]) ++StartText2;

		char* Buffer = _new_buffer<char>((TextLength-StartText2<EndText1+1)	? TextLength-StartText2+1
																		: EndText1+1);
		if (NULL==Buffer) return false;

		try	{
			if (TextLength-StartText2<EndText1+1)
				{	// Text2 shorter than Text1
				// #2a: make image of smaller one, and create object in Text2 for smaller one.
				memmove(Buffer,Text+StartText2,TextLength-StartText2+1);
				UnparsedText* Tmp = new UnparsedText(Buffer);
				Tmp->WS_Strip();
				// IF STILL HERE, will complete; two cases...
				// #3a: Shrink down the larger one (it should be here)
				Text.Shrink(EndText1+1);
				WS_Strip();
				Text2 = Tmp;
				return true;
				}
			else{	// Text1 shorter than Text2
				// #2b: make image of smaller one, and create object in Text2 for smaller one.
				memmove(Buffer,Text,EndText1+1);
				FranciLexer.CompactToken(Buffer,Flags);
				UnparsedText* Tmp = new UnparsedText(Buffer,(unsigned short)Flags);
				Tmp->WS_Strip();
				// IF STILL HERE, will complete; two cases...
				// #3b: Shrink down the larger one (it should be here)
				TextLength -= StartText2;
				memmove(Text,Text+StartText2,TextLength);
				Text.Shrink(TextLength);
				WS_Strip();
				// #4b: Text2 object is really Text1, swap this with Text2
				SwapWith(*Tmp);
				Text2 = Tmp;
				return true;
				}
			}
		catch(const bad_alloc&)
			{
			return false;
			}
		};
	return false;
}

void
UnparsedText::ZLSTarget(char const* Target, size_t length)
{
	SUCCEED_OR_DIE(!Text.empty());
	SUCCEED_OR_DIE(NULL!=Target);
	size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(0<length);
	size_t i = 0;
	do	if (0==strncmp(Target,Text+i,length))
			{
			size_t Offset = length;
			i += length;
			if (TextLength>i)
				{
				do	if (0==strncmp(Target,Text+i,length))
						{
						Offset += length;
						i += length;
						}
					else{
						Text[i-Offset] = Text[i];
						}
				while(TextLength>++i);
				}
			Text.Shrink(TextLength-Offset);
			return;
			}
	while(TextLength>++i);
	SpecializeSemantics();
}

void
UnparsedText::ZLSTarget(char const* Target)
{
	SUCCEED_OR_DIE(NULL==Target);
	ZLSTarget(Target,strlen(Target));
}

void
UnparsedText::ZLSTarget(char Target)
{
	SUCCEED_OR_DIE(!Text.empty());
	size_t TextLength = strlen(Text);
	size_t i = 0;
	do	if (Target==Text[i])
			{
			size_t Offset = 1;
			i += 1;
			if (TextLength>i)
				{
				do	if (Target==Text[i])
						{
						Offset += 1;
						i += 1;
						}
					else{
						Text[i-Offset] = Text[i];
						}
				while(TextLength>++i);
				}
			Text.Shrink(TextLength-Offset);
			return;
			}
	while(TextLength>++i);
	SpecializeSemantics();
}

void
UnparsedText::DeleteNCharsAtIdx(size_t N, size_t i)
{
	SUCCEED_OR_DIE(!Text.empty());
	SUCCEED_OR_DIE(N+i<=strlen(Text));
	TextSnip(N,N+i);
}

void
UnparsedText::InsertSourceAtIdx(const char* Source, size_t i, size_t length)
{
	VERIFY(NULL==Text || NULL==Source || 0>=length,AlphaCallAssumption);
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(i<=TextLength);
	if (!Text.Resize(TextLength+length)) UnconditionalRAMFailure();
	if (i<TextLength) memmove(&Text[i+length],&Text[i],TextLength-i);
	memmove(&Text[i],Source,length);
	SpecializeSemantics();
}

void
UnparsedText::InsertSourceAtIdx(const char* src, size_t i)
{
	SUCCEED_OR_DIE(!Text.empty());
	SUCCEED_OR_DIE(NULL!=src);
	SUCCEED_OR_DIE(i<=strlen(Text));
	InsertSourceAtIdx(src,i,strlen(src));
}

void
UnparsedText::InsertSourceAtIdx(char src, size_t i)
{
	SUCCEED_OR_DIE(!Text.empty());
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(i<=TextLength);
	if (!Text.Resize(TextLength+1)) UnconditionalRAMFailure();
	if (i<TextLength) memmove(&Text[i+1],&Text[i],TextLength-i);
	Text[i] = src;
	SpecializeSemantics();
}

void
UnparsedText::OverwriteNthPlaceWith(size_t N, const char* src, size_t length)
{
	SUCCEED_OR_DIE(!Text.empty());
	SUCCEED_OR_DIE(NULL!=src);
	SUCCEED_OR_DIE(0<length);
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(N<TextLength);
	memmove(&Text[N],src,(N+length>=TextLength) ? length : TextLength-N);
	SpecializeSemantics();
}

void
UnparsedText::OverwriteNthPlaceWith(size_t N, const char* src)
{
	SUCCEED_OR_DIE(!Text.empty());
	SUCCEED_OR_DIE(NULL!=src);
	SUCCEED_OR_DIE(N<=strlen(Text));
	OverwriteNthPlaceWith(N,src,strlen(src));
}

void
UnparsedText::OverwriteNthPlaceWith(size_t N, char src)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/1/2002
	SUCCEED_OR_DIE(!Text.empty());
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(N<TextLength);
	Text[N]=src;
	SpecializeSemantics();
}

void
UnparsedText::OverwriteReverseNthPlaceWith(size_t N, char src)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/1/2002
	SUCCEED_OR_DIE(!Text.empty());
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(N<TextLength);
	Text[TextLength-N-1]=src;
	SpecializeSemantics();
}

void
UnparsedText::ReplaceTargetWithSource(char src, const char* Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	SUCCEED_OR_DIE(!Text.empty());
	SUCCEED_OR_DIE(NULL!=Target);
	size_t TargetLength = strlen(Target);
	SUCCEED_OR_DIE(0<TargetLength);
	if (1==TargetLength)
		ReplaceTargetWithSource(src,Target[0]);
	else{
		size_t i = strlen(Text);
		do	if (0==strncmp(&Text[--i],Target,TargetLength))
				{
				Text[i]=src;
				DeleteNCharsAtIdx(i+1,TargetLength-1);
				}
		while(0<i);
		}
	SpecializeSemantics();
}

void
UnparsedText::ReplaceTargetWithSource(char src, const char Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	SUCCEED_OR_DIE(!Text.empty());
	size_t i = strlen(Text);
	do	if (Target==Text[--i])
			Text[i]=src;
	while(0<i);
	SpecializeSemantics();
}

void
UnparsedText::TruncateByN(size_t N)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/1/2002
	SUCCEED_OR_DIE(!Text.empty());
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(N<TextLength);
	Text.Shrink(TextLength-N);
	SpecializeSemantics();
}

bool
UnparsedText::ConcatenateLHSTruncatedByN(UnparsedText& RHS,unsigned long N)	// returns true iff success; RHS invalid then
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	size_t TextLength = (NULL==Text) ? 0 : strlen(Text);
	const size_t RHSTextLength = (NULL==RHS.Text) ? 0 : strlen(RHS.Text);
	SUCCEED_OR_DIE(N<TextLength);
	if (!Text.Resize(TextLength-N+RHSTextLength)) return false;
	memmove(&Text[TextLength-N],RHS.Text,RHSTextLength);
	TextLength -= N;
	TextLength += RHSTextLength;
	RHS.Text.reset();
	return true;
}

bool
UnparsedText::IsSubstring(char Target,size_t Offset) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/7/2002
	const size_t TextLength = (NULL==Text) ? 0 : strlen(Text);
	if (Offset<TextLength) return Target==Text[Offset];
	return false;
}

bool
UnparsedText::IsSubstring(const char* const Target,size_t Offset) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/7/2002
	const size_t TextLength = (NULL==Text) ? 0 : strlen(Text);
	if (Offset<TextLength)
		return 0==strncmp(Text+Offset,Target,strlen(Target));
	return false;
}

bool
UnparsedText::StartsWith(char Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/7/2002
	if (!Text.empty()) return Target==Text[0];
	return false;
}

bool
UnparsedText::StartsWith(const char* const Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/7/2002
	return 0==strncmp(Text,Target,strlen(Target));
}

bool UnparsedText::EndsWith(char Target) const
{
	const size_t TextLength = Text.empty() ? 0 : strlen(Text);
	return 0<TextLength && Target==Text[TextLength-1];
}

bool
UnparsedText::EndsAtOffsetWith(size_t Offset,char Target) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/7/2002
	const size_t TextLength = (NULL==Text) ? 0 : strlen(Text);
	if (0<TextLength && Offset+1==TextLength && Target==Text[TextLength-1])
		return true;
	return false;
}

bool
UnparsedText::WS_Strip(void)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/3/2002
	// (none of these should strip)
	// trim: leading, trailing spaces
	SUCCEED_OR_DIE(!Text.empty());
	size_t TextLength = strlen(Text);
	if (FranciLexer.IsWS(Text[0]) || FranciLexer.IsWS(Text[TextLength-1]))
		{
		{
		size_t i = strspn(Text,FranciLexer.WhiteSpace);
		if (i==TextLength)
			{
			Text.reset();
			return false;
			};
		if (0<i)
			{
			memmove(Text,Text+i,TextLength-i+1);
			TextLength -=i;
			};
		};

		while(FranciLexer.IsWS(Text[TextLength-1]))
			Text[--TextLength] = '\0';

		Text.Shrink(TextLength);
		}
	SpecializeSemantics();
	return true;
}

void UnparsedText::WipeText(void)
{
	Text.reset();
	SelfClassifyBitmap = None_UT;
}

void UnparsedText::ImportText(char*& src)
{
	Text = src;
	src = NULL;
	SelfClassifyBitmap = Uninterpreted_UT;
}

bool UnparsedText::PreChop(char*& Target, size_t strict_ub)
{	/*! \pre Offset is strictly less than strlen(Text), and positive */
	VERIFY(NULL==Text || 0>=strict_ub,AlphaCallAssumption);
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(strict_ub<TextLength);
	if (!_resize(Target,strict_ub)) return false;
	strncpy(Target,Text,strict_ub);
	memmove(Text,Text+strict_ub,TextLength-strict_ub);
	Text.Shrink(TextLength-strict_ub);
	return true;
}

void
UnparsedText::ScanThroughQuote(size_t& i, bool break_token_on_newline, char escaped_quotes, char escape)
{	/*! \pre Offset is strictly less than strlen(Text), and nonnegative */
	/*! \pre Offset points to a quote to be matched */
	/*! \pre escape is the escape character */
	/*! \pre escaped_quotes escapes the escape character */
	SUCCEED_OR_DIE(!Text.empty());
	const size_t TextLength = strlen(Text);
	SUCCEED_OR_DIE(i<TextLength);
	const char immediate_quote = Text[i];
	while(immediate_quote!=Text[++i])
		{
		if ('\0'==Text[i])
			break;
		else if ('\n'==Text[i])
			{
			if (break_token_on_newline) break;
			LogicalLineNumber++;
			}

		if (Text[i]==escaped_quotes && Text[i+1]==escape)
			i++;
		else if (Text[i]==escape)
			i++;
		}
}

void UnparsedText::ScanThroughNewline(size_t& Offset)
{
	while('\n'==Text[Offset])
		{
		Offset++;
		LogicalLineNumber++;
		}
}

bool UnparsedText::ScanToChar(size_t& i,char Target) const
{	//! \return returns true if stops on end-of-line
	//! \return returns false if stops at string termination
	while(Target!=Text[++i])
		if ('\0'==Text[i]) return false;
	return true;
}

void
UnparsedText::InstallEvalRule(Recognize* new_recognizer,Parse* new_parser)
{	// can tolerate FATAL behavior because this should be called during startup
	// LangConf needs to be able to recover
	size_t StackSize = EvalRecognizers.size();
	if (!EvalRecognizers.InsertSlotAt(StackSize,new_recognizer)) FATAL("Franci parser initialization failed");
	if (!EvalParsers.InsertSlotAt(StackSize,new_parser)) FATAL("Franci parser initialization failed");
}

// for main.cxx
void Init_Unparsed_Eval()
{
	UnparsedText::InstallEvalRule(&IntegerNumeral::IsLegalIntegerString,&IntegerNumeral::ConvertToIntegerNumeral);
	UnparsedText::InstallEvalRule(&TVal::is_legal,&TruthValue::read);
	UnparsedText::InstallEvalRule(&AbstractClass::IsReservedSetClassName,&AbstractClass::ConvertToReservedAbstractClass);
}

// integrated lexer flags: Uninterpreted_UT | HTMLStartTag_UT | HTMLTerminalTag_UT | XMLSelfTerminatingTag_UT
//	 | JSEntity_UT | JSCharEntity_UT | SemanticChar_UT;
// Others will be language-specific...really should recode this to facilitate C++ compiler mode

#define INFIX_SYMBOL_MAXSIZE 3
static const char* const InfixSymbolTable[5]
=	{
	SymbolPlusSign,		// length 1
	SymbolEllipsis2,
	SymbolNotEqual,		// length 2
	SymbolEqual,
	SymbolEllipsis		// length 3
	};

static const signed short InfixSymbolLowIndex[INFIX_SYMBOL_MAXSIZE]
=	{
	0,
	2,
	4
	};

static const signed short InfixSymbolHighIndex[INFIX_SYMBOL_MAXSIZE]
=	{
	1,
	3,
	4
	};

size_t Detect_InfixSymbol(const char* Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/16/2004
	// match to InfixSymbol_UT
	const size_t TextLength = strlen(Target);
	if (INFIX_SYMBOL_MAXSIZE>=TextLength)
		{
		signed short i = InfixSymbolLowIndex[TextLength-1];
		if (0<=i)
			{
			do	if (0==strcmp(Target,InfixSymbolTable[i]))
					return TextLength;
			while(++i<=InfixSymbolHighIndex[TextLength-1]);
			}
		}
	return 0;
}

#define LOGIC_KEYWORD_MAXSIZE 8
// Franci recognizes the following logic keywords:
// AND OR IFF XOR NAND NOR NIFF NXOR NOT IMPLIES
static const char* const LogicKeywordTable[11]
=	{
	LogicKeyword_OR,		// length 2
	LogicKeyword_AND,		// length 3
	LogicKeyword_IFF,
	LogicKeyword_XOR,
	LogicKeyword_NOT,
	LogicKeyword_NOR,
	LogicKeyword_NAND,		// length 4
	LogicKeyword_NIFF,
	LogicKeyword_NXOR,
	LogicKeyword_IMPLIES,	// length 7
	LogicKeyword_NIMPLIES	// length 8
	};

static const signed short LogicKeywordLowIndex[LOGIC_KEYWORD_MAXSIZE]
=	{
	-1,
	0,
	1,
	6,
	-1,
	-1,
	9,
	10
	};

static const signed short LogicKeywordHighIndex[LOGIC_KEYWORD_MAXSIZE]
=	{
	-1,
	0,
	5,
	8,
	-1,
	-1,
	9,
	10
	};

size_t
Detect_LogicKeyword(const char* Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/15/2004
	// match to LogicKeyword_UT
	const size_t TextLength = strlen(Target);
	if (LOGIC_KEYWORD_MAXSIZE>=TextLength)
		{
		signed short i = LogicKeywordLowIndex[TextLength-1];
		if (0<=i)
			{
			do	if (0==strcmp(Target,LogicKeywordTable[i]))
					return TextLength;
			while(++i<=LogicKeywordHighIndex[TextLength-1]);
			}
		}
	return 0;
}

#define PREDCALC_KEYWORD_MAXSIZE 9
// Franci also recognizes the following predicate calculus keywords
// IN FREE FORALL THEREIS NOTFORALL THEREISNO
static const char* const PredCalcKeywordTable[6]
=	{
	PredCalcKeyword_IN,			// length 2
	PredCalcKeyword_FREE,		// length 4
	PredCalcKeyword_FORALL,		// length 6
	PredCalcKeyword_THEREIS,	// length 7
	PredCalcKeyword_NOTFORALL,
	PredCalcKeyword_THEREISNO,	// length 9
	};

static const signed short PredCalcKeywordLowIndex[PREDCALC_KEYWORD_MAXSIZE]
=	{
	-1,
	0,
	-1,
	1,
	-1,
	2,
	3,
	-1,
	4
	};

static const signed short PredCalcKeywordHighIndex[PREDCALC_KEYWORD_MAXSIZE]
=	{
	-1,
	0,
	-1,
	1,
	-1,
	2,
	3,
	-1,
	5
	};

size_t
Detect_PredCalcKeyword(const char* Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/16/2004
	// match to PredCalcKeyword_UT
	const size_t TextLength = strlen(Target);
	if (PREDCALC_KEYWORD_MAXSIZE>=TextLength)
		{
		signed short i = PredCalcKeywordLowIndex[TextLength-1];
		if (0<=i)
			{
			do	if (0==strcmp(Target,PredCalcKeywordTable[i]))
					return TextLength;
			while(++i<=PredCalcKeywordHighIndex[TextLength-1]);
			}
		}
	return 0;
}

#define PREFIX_KEYWORD_MAXSIZE 17
// Franci recognizes the following prefix n-ary keywords (parentheses arglist)
// ALLEQUAL ALLDISTINCT NOTALLEQUAL NOTALLDISTINCT
// Franci recongizes the following prefix n-ary keywords (comma arglist,
//  1st arg before keyword)
// EQUALTOONEOF DISTINCTFROMALLOF
static const char* const PrefixKeywordTable[11]
=	{
	PrefixKeyword_PERMUTATION,		// length 4
	PrefixKeyword_PERMUTATION_ALT,
	PrefixKeyword_COMBINATION,
	PrefixKeyword_COMBINATION_ALT,
	EqualRelation_ALLEQUAL,			// length 8
	PrefixKeyword_FACTORIAL,		// length 9
	EqualRelation_ALLDISTINCT,		// length 11
	EqualRelation_NOTALLEQUAL,
	EqualRelation_EQUALTOONEOF,		// length 12
	EqualRelation_NOTALLDISTINCT,	// length 14
	EqualRelation_DISTINCTFROMALLOF	// length 17
	};

static const signed short PrefixKeywordLowIndex[PREFIX_KEYWORD_MAXSIZE]
=	{
	-1,
	-1,
	-1,
	0,
	-1,
	-1,
	-1,
	4,
	5,
	-1,
	6,
	8,
	-1,
	9,
	-1,
	-1,
	10
	};

static const signed short PrefixKeywordHighIndex[PREFIX_KEYWORD_MAXSIZE]
=	{
	-1,
	-1,
	-1,
	3,
	-1,
	-1,
	-1,
	4,
	5,
	-1,
	7,
	8,
	-1,
	9,
	-1,
	-1,
	10
	};

size_t
Detect_PrefixKeyword(const char* Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/16/2004
	// match to PrefixKeyword_UT
	const size_t TextLength = strlen(Target);
	if (PREFIX_KEYWORD_MAXSIZE>=TextLength)
		{
		signed short i = PrefixKeywordLowIndex[TextLength-1];
		if (0<=i)
			{
			do	if (0==strcmp(Target,PrefixKeywordTable[i]))
					return TextLength;
			while(++i<=PrefixKeywordHighIndex[TextLength-1]);
			}
		}
	return 0;
}

// For FranciLexer
unsigned long
QuasiEnglishFlags(const char* Text)
{
	if      (zaimoni::in_range<'0','9'>((unsigned char)Text[0]))
		return UnparsedText::LeadingIntegerNumeral_UT;
	else if (string_nand(Text,IsAlphabeticChar))
		return UnparsedText::VarName_UT;
	else
		return UnparsedText::QuasiEnglish_UT;
}

// For FranciLexer
void
FranciScript_CleanToken(char*& Target, unsigned long Flags)
{
	if 		(Flags & (UnparsedText::HTMLStartTag_UT | UnparsedText::JSEntity_UT))
		{
		const size_t TextLength = strlen(Target)-2;
		memmove(Target,&Target[1],TextLength);
		_shrink(Target,TextLength);
		}
	else if (Flags & (UnparsedText::XMLSelfEndTag_UT))
		{
		const size_t TextLength = strlen(Target)-3;
		memmove(Target,&Target[1],TextLength);
		_shrink(Target,TextLength);
		}
	else if (Flags & (UnparsedText::HTMLTerminalTag_UT | UnparsedText::JSCharEntity_UT))
		{
		const size_t TextLength = strlen(Target)-3;
		memmove(Target,&Target[2],TextLength);
		_shrink(Target,TextLength);
		}
}

void
UnparsedText::SpecializeSemantics(void)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/17/2004
	if (FranciLexer.NoLexFlags & SelfClassifyBitmap)
		return;
	SelfClassifyBitmap = None_UT;
	if (NULL==Text) return;
	size_t TextLength = strlen(Text);

	unsigned long Flags = 0;
	const size_t TokenLength = FranciLexer.UnfilteredNextToken(Text,Flags);
	if (TokenLength>=TextLength && Flags)
		{
		SelfClassifyBitmap = (unsigned short)Flags;
		FranciLexer.CompactToken(Text,SelfClassifyBitmap);
		}
}

// MetaConcept functions
bool
MetaConcept::IsPotentialVarName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/22/1999
	//! \todo augment this to handle super/subscripts
	if (   IsExactType(Variable_MC)
	    || (   IsExactType(UnparsedText_MC)
			&& static_cast<const UnparsedText*>(this)->IsQuasiEnglishOrVarName()))
		return true;
	return false;
}

bool
MetaConcept::IsPotentialArg() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/2001
	// Detection metaphor: we want to allow phrases that eventually evaluate to acceptable args.
	// this does *not* include IN __, or any of the quantifier phrases
	// right now, it suffices to detect 2-ary phrases
	if (   !IsUltimateType(NULL) || IsPotentialVarName()
		|| (MinPhrase2Idx_MC<=ExactType() && MaxPhrase2Idx_MC>=ExactType()))
		return true;
	return false;
}

