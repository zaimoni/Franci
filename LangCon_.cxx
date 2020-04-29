// LangCon_.cxx
// specific language configuration

#include "LangCon_.hxx"
#include "Zaimoni.STL/LexParse/LangConf.hpp"

using namespace zaimoni;

#define QuasiSymbol_UT Flag2_LC
#define LogicKeyword_UT Flag5_LC
#define PredCalcKeyword_UT Flag6_LC
#define PrefixKeyword_UT Flag7_LC
#define InfixSymbol_UT Flag8_LC
#define HTMLTerminalTag_UT Flag9_LC
#define JSEntity_UT Flag10_LC
#define JSCharEntity_UT Flag11_LC
#define HTMLStartTag_UT Flag12_LC
#define XMLSelfEndTag_UT Flag13_LC

LangConf FranciLexer(	"//",
						NULL,
						NULL,
						NULL,
						&IsQuasiNumericIDChar,
						&QuasiEnglishFlags,
						&FranciScript_CleanToken,
						NULL,
						NULL,
						NULL,
						NULL,
						"'\"",
						" \t\r\n\v",
						"()[]{}, ",
						NULL,0,
						NULL,
						(HTMLStartTag_UT | HTMLTerminalTag_UT | JSEntity_UT | JSCharEntity_UT | XMLSelfEndTag_UT),0,
						'\\','\\',true,false);

void InitializeLexerDefs(void)
{
	FranciLexer.InstallTokenizingFilter(&ApplyHardFranciMacrosToImmediateText);
	FranciLexer.InstallTokenizer(&ScanHTMLTerminalTag,HTMLTerminalTag_UT);
	FranciLexer.InstallTokenizer(&ScanJSEntityTag,JSEntity_UT);
	FranciLexer.InstallTokenizer(&ScanJSCharEntityTag,JSCharEntity_UT);
	FranciLexer.InstallTokenizer(&ScanXMLSelfEndingTag,XMLSelfEndTag_UT);
	FranciLexer.InstallTokenizer(&ScanHTMLStartTag,HTMLStartTag_UT);
	FranciLexer.InstallTokenizer(&Detect_InfixSymbol,InfixSymbol_UT);
	FranciLexer.InstallTokenizer(&ScanQuasiSymbolTag,QuasiSymbol_UT);	// has to be last symbol tokenizer, it partial-matches other tags
	FranciLexer.InstallTokenizer(&Detect_LogicKeyword,LogicKeyword_UT);
	FranciLexer.InstallTokenizer(&Detect_PredCalcKeyword,PredCalcKeyword_UT);
	FranciLexer.InstallTokenizer(&Detect_PrefixKeyword,PrefixKeyword_UT);
}

#undef XMLSelfEndTag_UT
#undef HTMLStartTag_UT
#undef JSCharEntity_UT
#undef JSEntity_UT
#undef HTMLTerminalTag_UT
#undef InfixSymbol_UT
#undef PrefixKeyword_UT
#undef PredCalcKeyword_UT
#undef LogicKeyword_UT
#undef QuasiSymbol_UT

