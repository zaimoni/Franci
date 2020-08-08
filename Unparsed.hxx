// Unparsed.hxx
// header for UnparsedText

#ifndef UNPARSED_TEXT_DEF
#define UNPARSED_TEXT_DEF

#include "MetaCon6.hxx"
#include "Zaimoni.STL/AutoPtr.hpp"

class UnparsedText;
namespace zaimoni {

template<>
struct is_polymorphic_final<UnparsedText> : public std::true_type {};

}

class UnparsedText final : public MetaConceptZeroArgs
{
private:
	autovalarray_ptr_throws<char> Text;

public:
	typedef bool Recognize(const char*);
	typedef bool Parse(MetaConcept*&, const char*);

	enum SelfClassify_UT {
		None_UT = 0x00,					// LangConf
		SemanticChar_UT = 0x01,			// LangConf
		LeadingIntegerNumeral_UT = 0x02,
		QuasiSymbol_UT = 0x04,
		QuasiEnglish_UT = 0x08,
		VarName_UT = 0x10,
		LogicKeyword_UT = 0x20,
		PredCalcKeyword_UT = 0x40,
		PrefixKeyword_UT = 0x80,
		InfixSymbol_UT = 0x100,
		HTMLTerminalTag_UT = 0x200,
		JSEntity_UT = 0x400,
		JSCharEntity_UT = 0x800,
		HTMLStartTag_UT = 0x1000,
		XMLSelfEndTag_UT = 0x2000,
		Uninterpreted_UT = 0x8000		// LangConf
	};

	size_t LogicalLineNumber;					// default 0, not manipulated by UnparsedText
	size_t LogicalLineOrigin;					// default 0, not manipulated by UnparsedText
	const char* SourceFileName;	// *NOT* owned; default NULL, not manipulated by UnparsedText

private:
	unsigned short SelfClassifyBitmap;

	static zaimoni::weakautoarray_ptr<Recognize*> EvalRecognizers;
	static zaimoni::weakautoarray_ptr<Parse*> EvalParsers;

public:
	explicit UnparsedText(char*& src);
	UnparsedText(char*& src, bool Interpreted);

protected:
	UnparsedText(char*& src, unsigned short NewSelfClassify);

public:
	UnparsedText(const UnparsedText& src) = default;
	UnparsedText(UnparsedText&& src) = default;
	UnparsedText& operator=(const UnparsedText& src) = default;
	UnparsedText& operator=(UnparsedText&& src) = default;
	~UnparsedText() = default;

	void CopyInto(MetaConcept*& dest) const override { CopyInto_ForceSyntaxOK(*this, dest); };	// can throw memory failure
	void CopyInto(UnparsedText*& dest) const { CopyInto_ForceSyntaxOK(*this, dest); };
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(UnparsedText*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

	static UnparsedText& up_reference(MetaConcept* src);
	static const UnparsedText& up_reference(const MetaConcept* src);
	static UnparsedText& up_reference(MetaConcept& src);
	static const UnparsedText& up_reference(const MetaConcept& src);
	static UnparsedText& fast_up_reference(MetaConcept* src);
	static const UnparsedText& fast_up_reference(const MetaConcept* src);

	//  Type ID functions
	const AbstractClass* UltimateType() const override { return 0; }	// untyped i.e. free
	constexpr static bool IsType(ExactType_MC x) { return UnparsedText_MC == x; }
	unsigned int OpPrecedence() const override;
	//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const;
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true
// text I/O functions
	const char* ViewKeyword() const override { return Text; }
	size_t text_size() const { return Text.size(); }
	void SwapWith(UnparsedText& Target);
// Pattern analysis: returns true if pattern found, and then fills the bounding coordinates
//  Quotation marks do not include leading/trailing spaces, if any
//  A QuasiEnglishNumeric ID is something that could eventually parse into an English word or
//  number.  Alphabetic characters and numeric characters are allowed.  Interpreting decimal
//  points comes later.  An all-capital QuasiEnglishNumeric ID is biased towards being a
//  math keyword; normal capitalization is biased towards English.  Mixed letters and numbers
//  is biased towards a math keyword.
//	A QuasiSymbolID is something that could eventually parse into atomic punctuation, or a 
//  reasonable mathematical symbol.  It cannot contain spaces, or alphabetic/numeric characters.
//  While balanced syntactical items do not count, the QuasiSymbolID length-finder does not
//  check for them (much).
//	Franci interprets ...s' and ...z' as more likely to be english possessives than quotation
//  endings.
//	Franci interprets ' surrounded by letters as more likely to be English than quotation start/end
//  Franci interprets '70 as a century-suppressed year
//	Franci is aware of ' and " as single-variable derivative markers.
	inline bool IsUnclassified(void) const {return None_UT==SelfClassifyBitmap;};
	inline bool IsQuasiEnglishNumeric(void) const {return (LeadingIntegerNumeral_UT | QuasiEnglish_UT | VarName_UT) & SelfClassifyBitmap;};
	inline bool IsQuasiSymbol(void) const {return QuasiSymbol_UT==SelfClassifyBitmap;};
	bool IsQuasiSymbolEndingInMinusSign(void) const;
	bool IsQuasiSymbolEndingInPlusSign(void) const;
	inline bool IsSemanticChar(void) const {return SemanticChar_UT==SelfClassifyBitmap;};
	bool IsSemanticChar(char Target) const;
	inline bool IsLeadingIntegerNumeral(void) const {return LeadingIntegerNumeral_UT==SelfClassifyBitmap;};
	inline bool IsQuasiEnglish(void) const {return QuasiEnglish_UT==SelfClassifyBitmap;};
	inline bool IsVarName(void) const {return VarName_UT==SelfClassifyBitmap;};
	bool IsQuasiEnglishOrVarName(void) const {return (QuasiEnglish_UT | VarName_UT) & SelfClassifyBitmap;};
	inline bool IsLogicKeyword(void) const {return LogicKeyword_UT==SelfClassifyBitmap;};
	bool IsLogicKeyword(const char* Target) const;
	inline bool IsPredCalcKeyword(void) const {return PredCalcKeyword_UT==SelfClassifyBitmap;};
	bool IsPredCalcKeyword(const char* Target) const;
	inline bool IsPrefixKeyword(void) const {return PrefixKeyword_UT==SelfClassifyBitmap;};
	bool IsPrefixKeyword(const char* Target) const;
	inline bool IsInfixSymbol(void) const {return InfixSymbol_UT==SelfClassifyBitmap;};
	bool IsInfixSymbol(const char* Target) const;
	inline bool IsHTMLStartTag(void) const {return HTMLStartTag_UT==SelfClassifyBitmap;};
	bool IsHTMLStartTag(const char* Target) const;
	bool IsHTMLTerminalTag() const { return HTMLTerminalTag_UT == SelfClassifyBitmap; }
	bool IsHTMLTerminalTag(const char* Target) const;
	bool IsJSEntity() const { return JSEntity_UT == SelfClassifyBitmap; }
	bool IsJSEntity(const char* Target) const;
	bool IsJSCharEntity() const { return JSCharEntity_UT == SelfClassifyBitmap; }
	bool IsJSCharEntity(const char* Target) const;
	inline bool MustBeParsedInContext(void) const {return (HTMLTerminalTag_UT | JSEntity_UT | JSCharEntity_UT) & SelfClassifyBitmap;}
	inline bool IsUninterpreted(void) const {return (Uninterpreted_UT & SelfClassifyBitmap) ? true : false;};
	bool IsMultiplicationSymbol() const; // 2020-08-03: dead function?  Cf. IsLogicalMultiplicationSign
	inline bool IsLogicOrPredCalcKeyword(void) const {return (LogicKeyword_UT | PredCalcKeyword_UT) & SelfClassifyBitmap;};
	inline bool IsLogic_Prefix_OrPredCalcKeyword(void) const {return (LogicKeyword_UT | PrefixKeyword_UT | PredCalcKeyword_UT) & SelfClassifyBitmap;};
	inline bool IsSymbol(void) const {return (QuasiSymbol_UT | InfixSymbol_UT) & SelfClassifyBitmap;};
	inline bool IsCharacter(char Target) const {return IsSemanticChar() && Text[0]==Target;};
	// following implemented in Clause2.cxx
	ExactType_MC TypeFor2AryClauseInfixKeyword(void) const;
	// following implemented in Phrase2.cxx
	ExactType_MC TypeFor2AryPhraseKeyword(void) const;
	// following implemented in ClauseN.cxx
	ExactType_MC TypeForNAryClauseKeyword(void) const;
	// resume implementing in Unparsed.cxx
	bool ArgCannotExtendLeftThroughThis(void) const;
	bool ArgCannotExtendRightThroughThis(void) const;
	bool IsLogicalPlusSign() const;
	bool IsLogicalMultiplicationSign() const;
	bool IsLogicalEllipsis() const;
	bool IsLogicalInfinity() const { return IsJSEntity("infin"); }

	size_t LengthOfNumericIntegerToSplitOff(void) const;	// 2020-08-06 dead function?
	bool AnyNonAlphabeticChars() const;	// 2020-08-06 dead function?

	size_t LengthOfQuasiEnglishNumericID() const;	// 2020-08-06 dead function?
	size_t LengthOfQuasiSymbolID() const;
	size_t LengthOfHTMLStartTag() const;	// 2020-08-06 dead function?
	size_t LengthOfHTMLTerminalTag() const;	// 2020-08-06 dead function?

	// pattern processing
	size_t CanSplitIntoTwoTexts() const;
	bool SplitIntoTwoTexts(MetaConcept*& Text2);
	void ZLSTarget(char const* Target, size_t length);
	void ZLSTarget(char const* Target);
	void ZLSTarget(char Target);
	void DeleteNCharsAtIdx(size_t N, size_t i);
	void InsertSourceAtIdx(const char* src, size_t i, size_t length);
	void InsertSourceAtIdx(const char* src, size_t i);
	void InsertSourceAtIdx(char src, size_t i);
	void OverwriteNthPlaceWith(size_t N, const char* src, size_t length);
	void OverwriteNthPlaceWith(size_t N, const char* src);
	void OverwriteNthPlaceWith(size_t N, char src);
	void OverwriteReverseNthPlaceWith(size_t N, char src);
	void ReplaceTargetWithSource(char src, const char* Target);
	void ReplaceTargetWithSource(char src, const char Target);
	void TruncateByN(size_t N);
	bool ConcatenateLHSTruncatedByN(UnparsedText& RHS,unsigned long N);	// returns true iff success; RHS invalid then
	bool IsString(const char* const Target) const {return 0==strcmp(Text,Target);};
	bool IsSubstring(char Target,size_t Offset) const;
	bool IsSubstring(const char* const Target,size_t Offset) const;
	bool StartsWith(char Target) const;
	bool StartsWith(const char* const Target) const;
	bool EndsWith(char Target) const;
	bool EndsAtOffsetWith(size_t Offset,char Target) const;
	bool WS_Strip(void);	// returns true if there is content left over
	void WipeText(void);
	void ExtractText(char*& dest) {Text.TransferOutAndNULL(dest);};	// destroys syntactical validity by NULLing Text; use this right before destruction.
	void ImportText(char*& src);
	bool PreChop(char*& Target, size_t strict_ub);	// excises first n characters to string Target
	void ScanThroughQuote(size_t& i, bool break_token_on_newline, char escaped_quotes, char escape);
	void ScanThroughNewline(size_t& Offset);
	bool ScanToChar(size_t& i,char Target) const;

	static void InstallEvalRule(Recognize* new_recognizer,Parse* new_parser);

protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	std::string to_s_aux() const override;

private:
	void TextSnip(const size_t Start, const size_t End);
	void SpecializeSemantics(void);
};

template<char c> bool IsSemanticChar(const MetaConcept* x) {
	if (const auto src = up_cast<UnparsedText>(x)) return src->IsSemanticChar(c);
	return false;
}

inline bool IsHTMLStartTag(const MetaConcept* x, const char* Target)
{
	if (const auto src = up_cast<UnparsedText>(x)) return src->IsHTMLStartTag(Target);
	return false;
}

inline const char* IsHTMLTerminalTag(const MetaConcept* x)
{
	if (const auto src = up_cast<UnparsedText>(x)) {
		if (src->IsHTMLTerminalTag()) return src->ViewKeyword();
	}
	return 0;
}

inline bool RejectTextToVar(const MetaConcept* x)
{
	if (const auto src = up_cast<UnparsedText>(x)) return !src->IsQuasiEnglishOrVarName();
	return false;
}

#endif
