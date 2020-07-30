// InParse.hxx
// formerly header for InParse, the type for an incompletely parsed object
// this coordinates the parsing for FranciScript

#ifndef INPARSE_HXX
#define INPARSE_HXX

#include <stddef.h>

class MetaConcept;

// primary pattern matching
void Franci_SyntaxError(void);
void Franci_IncompleteUnderstanding(void);
// bool Franci_IsSyntacticalSymmetry(const MetaConcept* const *const ArgArray);
void LogThis(const MetaConcept * const * const ArgArray);	// may not be best location for these two
void SayThisNormally(const MetaConcept * const * const ArgArray);

// aux pattern matching
bool ArglistAry2PlusRecognize(const MetaConcept* const * ArgArray,size_t i);
bool CommalistAry2PlusRecognize(const MetaConcept* const * ArgArray,size_t i);
bool ArgThatCannotExtendLeftRecognize(const MetaConcept* const * ArgArray,size_t i);
size_t PrefixCommaListVarNamesRecognize(const MetaConcept* const * ArgArray,size_t KeywordIdx);
size_t PostfixCommaListVarNamesRecognize(const MetaConcept* const * ArgArray,size_t KeywordIdx);

// next is implemented in ClauseN.cxx
void LinearIntervalInit(MetaConcept**& ArgArray, size_t& i,MetaConcept*& dest);

#endif
