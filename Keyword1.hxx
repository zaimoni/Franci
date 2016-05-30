// Keyword1.hxx
// math and logic keywords

// following defined in LenName.cxx
extern const char* const MetaConnectiveNames[8];
extern const char* const QuantificationNames[5];

// following defined in Keyword1.cxx
extern const char* const LogicKeyword_NXOR;
extern const char* const LogicKeyword_NIFF;
extern const char* const LogicKeyword_NOR;
extern const char* const LogicKeyword_NAND;
extern const char* const LogicKeyword_NIMPLIES;
extern const char* const LogicKeyword_NOT;

extern const char* const PredCalcKeyword_THEREIS;
extern const char* const PredCalcKeyword_NOTFORALL;
extern const char* const PredCalcKeyword_THEREISNO;
extern const char* const PredCalcKeyword_FREE;
extern const char* const PredCalcKeyword_IN;

#define GCF_NAME "GCF"

extern const char* const PrefixKeyword_FACTORIAL;
extern const char* const PrefixKeyword_PERMUTATION;
extern const char* const PrefixKeyword_PERMUTATION_ALT;
extern const char* const PrefixKeyword_COMBINATION;
extern const char* const PrefixKeyword_COMBINATION_ALT;

#define TruthValue_True "TRUE"
#define TruthValue_False "FALSE"
#define TruthValue_Unknown "UNKNOWN"
#define TruthValue_Contradiction "CONTRADICTION"

extern const char* const EqualRelation_NOTALLDISTINCT;
extern const char* const EqualRelation_NOTALLEQUAL;
extern const char* const EqualRelation_DISTINCTFROMALLOF;
extern const char* const EqualRelation_EQUALTOONEOF;

extern const char* const SymbolEqual;
extern const char* const SymbolNotEqual;
extern const char* const SymbolPlusSign;
extern const char* const SymbolMultSign;
extern const char* const SymbolEllipsis;
extern const char* const SymbolEllipsis2;
extern const char* const JSCharEntityPlusSign;
extern const char* const JSCharEntityMultSign;
extern const char* const JSCharEntityEllipsis;

#define LogicKeyword_XOR LogicKeyword_NXOR+1
#define LogicKeyword_IFF LogicKeyword_NIFF+1
#define LogicKeyword_OR LogicKeyword_NOR+1
#define LogicKeyword_AND LogicKeyword_NAND+1
#define LogicKeyword_IMPLIES LogicKeyword_NIMPLIES+1

#define PredCalcKeyword_FORALL PredCalcKeyword_NOTFORALL+3

#define EqualRelation_ALLEQUAL EqualRelation_NOTALLEQUAL+3
#define EqualRelation_ALLDISTINCT EqualRelation_NOTALLDISTINCT+3

#define JSCharEntityPlusSignV2 JSCharEntityPlusSign+1
