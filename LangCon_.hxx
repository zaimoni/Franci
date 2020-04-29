// LangCon_.hxx
// auxilliary declarations to follow LangConf.hxx

#ifndef LANGCON__HXX
#define LANGCON__HXX 1

namespace zaimoni {

class LangConf;

}

extern zaimoni::LangConf FranciLexer;		// Franci Script language

// following defined in UnparsedText
bool IsQuasiNumericIDChar(char Test);
bool ApplyHardFranciMacrosToImmediateText(char*& Buffer, const char* filename);
size_t ScanHTMLTerminalTag(const char* Text);
size_t ScanJSEntityTag(const char* Text);
size_t ScanJSCharEntityTag(const char* Text);
size_t ScanXMLSelfEndingTag(const char* Text);
size_t ScanHTMLStartTag(const char* Text);
size_t ScanQuasiSymbolTag(const char* Text);
size_t Detect_LogicKeyword(const char* Target);
size_t Detect_PredCalcKeyword(const char* Target);
size_t Detect_PrefixKeyword(const char* Target);
size_t Detect_InfixSymbol(const char* Target);
unsigned long QuasiEnglishFlags(const char* Text);
void FranciScript_CleanToken(char*& Target, unsigned long Flags);

#endif
