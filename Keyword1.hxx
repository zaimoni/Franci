// Keyword1.hxx
// math and logic keywords

// following defined in LenName.cxx
extern const char* const QuantificationNames[5];

// following defined in Keyword1.cxx
constexpr const char* const LogicKeyword_NXOR = "NXOR";
constexpr const char* const LogicKeyword_NIFF = "NIFF";
constexpr const char* const LogicKeyword_NOR = "NOR";
constexpr const char* const LogicKeyword_NAND = "NAND";
constexpr const char* const LogicKeyword_IMPLIES = "IMPLIES";
constexpr const char* const LogicKeyword_NOT = "NOT";

constexpr const char* const PredCalcKeyword_THEREIS = "THEREIS";
constexpr const char* const PredCalcKeyword_NOTFORALL = "NOTFORALL";
constexpr const char* const PredCalcKeyword_THEREISNO = "THEREISNO";
constexpr const char* const PredCalcKeyword_FREE = "FREE";
constexpr const char* const PredCalcKeyword_IN = "IN";

constexpr const char* const PrefixKeyword_FACTORIAL = "FACTORIAL";
constexpr const char* const PrefixKeyword_PERMUTATION = "PERM";
constexpr const char* const PrefixKeyword_PERMUTATION_ALT = "Perm";
constexpr const char* const PrefixKeyword_COMBINATION = "COMB";
constexpr const char* const PrefixKeyword_COMBINATION_ALT = "Comb";

constexpr const char* const EqualRelation_NOTALLDISTINCT = "NOTALLDISTINCT";
constexpr const char* const EqualRelation_NOTALLEQUAL = "NOTALLEQUAL";
constexpr const char* const EqualRelation_DISTINCTFROMALLOF = "DISTINCTFROMALLOF";
constexpr const char* const EqualRelation_EQUALTOONEOF = "EQUALTOONEOF";

constexpr const char* const SymbolEqual = "==";
constexpr const char* const SymbolNotEqual = "!=";
constexpr const char* const SymbolPlusSign = "+";
constexpr const char* const SymbolMultSign = "\xb7";
constexpr const char* const SymbolEllipsis = "...";
constexpr const char* const SymbolEllipsis2 = "\x75";
constexpr const char* const JSCharEntityPlusSign = "043";
constexpr const char* const JSCharEntityMultSign = "183";
constexpr const char* const JSCharEntityEllipsis = "133";

#define LogicKeyword_XOR LogicKeyword_NXOR+1
#define LogicKeyword_IFF LogicKeyword_NIFF+1
#define LogicKeyword_OR LogicKeyword_NOR+1
#define LogicKeyword_AND LogicKeyword_NAND+1

#define PredCalcKeyword_FORALL PredCalcKeyword_NOTFORALL+3

#define EqualRelation_ALLEQUAL EqualRelation_NOTALLEQUAL+3
#define EqualRelation_ALLDISTINCT EqualRelation_NOTALLDISTINCT+3

#define JSCharEntityPlusSignV2 JSCharEntityPlusSign+1
