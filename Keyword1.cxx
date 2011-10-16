// Keyword1.cxx
// math and logic keywords

//! \todo the length of all of these keywords are constants.  Define an enumeration, and
//! replace all references to strlen(keyword) with this enumeration.
//! Do an initialization check to verify that the enumeration is correct.

#include "keyword1.hxx"

const char* const LogicKeyword_NXOR = "NXOR";
const char* const LogicKeyword_NIFF = "NIFF";
const char* const LogicKeyword_NOR = "NOR";
const char* const LogicKeyword_NAND = "NAND";
const char* const LogicKeyword_NIMPLIES = "NIMPLIES";
const char* const LogicKeyword_NOT = "NOT";

const char* const PredCalcKeyword_THEREIS = "THEREIS";
const char* const PredCalcKeyword_NOTFORALL = "NOTFORALL";
const char* const PredCalcKeyword_THEREISNO = "THEREISNO";
const char* const PredCalcKeyword_FREE = "FREE";
const char* const PredCalcKeyword_IN = "IN";

const char* const PrefixKeyword_FACTORIAL = "FACTORIAL";
const char* const PrefixKeyword_PERMUTATION = "PERM";
const char* const PrefixKeyword_PERMUTATION_ALT = "Perm";
const char* const PrefixKeyword_COMBINATION = "COMB";
const char* const PrefixKeyword_COMBINATION_ALT = "Comb";

const char* const EqualRelation_NOTALLDISTINCT = "NOTALLDISTINCT";
const char* const EqualRelation_NOTALLEQUAL = "NOTALLEQUAL";
const char* const EqualRelation_DISTINCTFROMALLOF = "DISTINCTFROMALLOF";
const char* const EqualRelation_EQUALTOONEOF = "EQUALTOONEOF";

const char* const SymbolEqual = "==";
const char* const SymbolNotEqual = "!=";
const char* const SymbolPlusSign = "+";
const char* const SymbolMultSign = "\xb7";
const char* const SymbolEllipsis = "...";
const char* const SymbolEllipsis2 = "\x75";
const char* const JSCharEntityPlusSign = "043";
const char* const JSCharEntityMultSign = "183";
const char* const JSCharEntityEllipsis = "133";

