// MetaCon1.hxx
// header for MetaConcept

#ifndef METACONCEPT_DEF
#define METACONCEPT_DEF

#include "Global.hxx"
#include <functional>
#include <utility>

enum LinguisticType_LT	{
						CommonNoun_LT	=	0x00000001,
						ProperNoun_LT	=	0x00000002,
						Noun			=	0x00000003,
						Verb_LT			=	0x00000004,
						Adjective_LT	=	0x00000008,
						Adverb_LT		=	0x00000010,
						Preposition_LT	=	0x00000020,
						Connective_LT	=	0x00000040,
						Interjection_LT	=	0x00000080
						};
enum ExactType_MC	{
					Unknown_MC			= 0,
					TruthValue_MC,			// 1
					IntegerNumeral_MC,		// 2
					LinearInterval_MC,	// 3
					LinearInfinity_MC,	// 4
					AbstractClass_MC,	// 5
					Variable_MC,	// 6
					// LOW-LEVEL DEPENDENCY: 0-ary types are a block, starting at 1
					// start MetaConnective block
					LogicalAND_MC,	// 7
					LogicalOR_MC,
					LogicalIFF_MC,
					LogicalXOR_MC,
					LogicalNXOR_MC,
					LogicalNIFF_MC,
					LogicalNOR_MC,
					LogicalNAND_MC,		// 14
					// end MetaConnective block
					// start EqualRelation block
					ALLEQUAL_MC,		// 15
					ALLDISTINCT_MC,
					EQUALTOONEOF_MC,
					DISTINCTFROMALLOF_MC,
					NOTALLDISTINCT_MC,
					NOTALLEQUAL_MC,	// 20
					// end EqualRelation block
					// LOW-LEVEL DEPENDENCIES:
					// * MetaQuantifier IDs are a block
					// * Lowest MetaQuantifier ID is ForAll_MC
					// * Paired s.t. logical-negations are on opposite ends, similar offset
					// * There are 5 identifiers in the block
					// * 6*[highest ID in block] does not cause an arithmetic overflow
					// start MetaQuantifier bloc
					ForAll_MC,	// 21
					ThereIs_MC,
					Free_MC,
					ForAllNot_MC,
					ThereIsNot_MC,	// 25
					// end MetaQuantifier block
					UnparsedText_MC,	// 26
					QuantifiedStatement_MC,
					Inparse_MC,
					StdAddition_MC,
					StdMultiplication_MC,
					SeriesAddition_MC,
					SeriesMultiplication_MC,
					GCF_MC,	// 33
					// Factorial block
					Factorial_MC,	// 34
					PermutationCount_MC,
					CombinationCount_MC,	// 36
					// LOW-LEVEL DEPENDENCIES:
					// * Phrase1Ary IDs are a block
					// * Lowest Phrase1Ary ID is IN_Phrase_MC
					// * all phrases, clauses are a block
					// start 1-ary phrase block
					IN_Phrase1_MC,	// 37
					// end 1-ary phrase block
					// LOW-LEVEL DEPENDENCIES:
					// * PhraseNAry IDs are a block
					// * 1st 5 have linear map to Metaquantifier block
					// start n-ary phrase block
					FORALL_PhraseN_MC, // 38
					THEREIS_PhraseN_MC,
					FREE_PhraseN_MC,
					NOTFORALL_PhraseN_MC,
					THEREISNO_PhraseN_MC, // 42
					// end n-ary phrase block
					ParseNode_MC, // 43
					UB_MC,
					// end n-ary clause block
					// semantic indexes
					MinSemanticIdx_MC	= 1,					// minimum index with meaning
					MaxSemanticIdx_MC	= UB_MC-1,	// maximum index with meaning
					MinPhrase1Idx_MC	= IN_Phrase1_MC,	// minimum index for Phrase1Ary
					MaxPhrase1Idx_MC	= IN_Phrase1_MC,  // maximum index for Phrase1Ary
					MinPhraseNIdx_MC	= FORALL_PhraseN_MC,	// minimum index for PhraseNAry
					MaxPhraseNIdx_MC	= THEREISNO_PhraseN_MC,	// maximum index for PhraseNAry
					MinZeroAryIdx_MC	= TruthValue_MC,		// minimum index for 0-ary type
					MaxZeroAryIdx_MC	= IntegerNumeral_MC,	// maximum index for 0-ary type
					MinClausePhraseIdx_MC = MinPhrase1Idx_MC,	// minimum index for internal phrase/clause
					MaxClausePhraseIdx_MC = MaxPhraseNIdx_MC,	// maximum index for internal phrase/clause
					MinSymbolicConstant_MC = LinearInfinity_MC,	// minimum index for symbolic constants
					MaxSymbolicConstant_MC = LinearInfinity_MC,	// maximum index for symbolic constants
					MinNonTrivialLogicalANDDetailedRule_MC = LogicalOR_MC,	// minimum index with nontrivial LogicalANDDetailedRule implementation
					MinNonTrivialNonTrivialModifies_MC = LogicalOR_MC,	// minimum index with nontrivial LogicalANDDetailedRule implementation
					MinTransSymm_MC		= LogicalIFF_MC			// minimum index for trans-symm == arg rule-finder in LogicalAND
					};
enum LegalInternalTransformationBitmap1_MC	{
											Symmetric_LITBMP1MC = 0x0001,
											SelfAssociative_LITBMP1MC = 0x0002,
											CompositionWithSelfOK_LITBMP1MC = 0x0004,
											Transitive_LITBMP1MC = 0x0008,
											HasAnnihilatorKey_LITBMP1MC = 0x0010,
											Reflexive_LITBMP1MC = 0x0020,
											SimplePhraseClauseTransition_LITBMP1MC = 0x0040,
											ProcessesIntegerIntervals_LITBMP1MC = 0x0080,
											// FullPairwise_LITBMP1MC = ??
											AND_of_BasisClauses_LITBMP1MC = 0x8000
											};

// These properties would help with the MetaConnective operations
// Symmetric: it is effectively a const operation to commute any two args
//		Requires that implementation allow arity 2+
// AntiSymmetric: with respect to a "natural" involution on the result type, a transposition involutes the result
//		Requires that implementation allow arity 2+
// SelfAssociative:
//		Requires n-ary implementation
//		automatic definition of finite series operation
//      automatic definition of finite positive integral power of operation on single element
//		Implies CompositionWithSelfOK
//      if also commutative, rearrangement property is effectively const operation
// CompositionWithSelfOK
//		Requires that implementation not 0-ary
//		typechecking filter: all input UltimateTypes must be compatible with result UltimateType
// Transitive: chains indicate that all pairwise combinations work, and that two statements
//		can be merged by having one arg in common
//		Requires n-ary implementation
//		Requires(?) UltimateType TruthValues.
//		is legitimate to add to the chain after testing against one item.
// FullPairwise: chains indicate that all pairwise combinations work, and that a new arg
//		can be merged in if it is related the same way to *all* args already there
//		Requires n-ary implementation
//		Requires(?) UltimateType TruthValues.
//		item is legitimate to add to the chain after testing against all items.

// MetaConnective notes: 
// NAND, NOR are Commutative [but they are DeMorganed out at construction....]
// AND, OR are Commutative and SelfAssociative
// IFF is Commutative and Transitive
// XOR is Commutative
// NIFF, NXOR are Commutative

// NOTE: Franci calls the Evaluate member function from exactly one place: DestructiveSyntacticallyEvaluateOnce
// * no self-evaluate rules can go off
// * object will immediately be deleted
// * call is wrapped around a try...catch(const std::bad_alloc&) block, so this is another way to signal RAM failure
// * incoming pointer is NULL...could do ASSERT rather than DELETE_AND_NULL
// * also can do ASSERT on SyntaxOK of target as a post-condition

class AbstractClass;
class MetaQuantifier;
class UnparsedText;
class Variable;

struct MetaConceptVFT	{
						size_t MinArity;
						size_t MaxArity;
						ExactType_MC ExactType;
						unsigned long Bitmap1;
						const char* name;
						char GrammarInfo;
						};


extern const MetaConceptVFT MetaConceptLookUp[MaxSemanticIdx_MC+1];

class MetaConcept;

// assistant data types for pattern recognition
typedef bool LowLevelUnaryProperty(const MetaConcept& lhs);
typedef bool LowLevelBinaryRelation(const MetaConcept& lhs, const MetaConcept& rhs);
typedef bool LowLevelSideEffectBinaryRelation(MetaConcept& lhs, MetaConcept& rhs);
typedef signed int LowLevelIntValueBinaryFunction(const MetaConcept& lhs, const MetaConcept& rhs);
typedef void LowLevelAction(MetaConcept*& Target, const MetaConcept& Inducer);

// NOTE: the above two types need a partial ordering; this partial ordering would detect
// "strictly more specific rule" (useful when choosing which of two distinct possibilities
// to use); overload < by analogy with "subset"
// NOTE: a 'free' UltimateType is denoted by NULL.

class MetaConcept
{
protected:
	const MetaConceptVFT* VFTable1;					// must initialize this for all real instances
	unsigned char MultiPurposeBitmap;
	enum Variable_VF	{
						NoModifiers_VF = 0x00,
						LogicalNegated_VF = 0x01,	// needs UltimateType = TruthValues
						StdAdditionInv_VF = 0x02,	// needs UltimateType to support StdAddition
						StdMultInv_VF = 0x04	// needs UltimateType to support StdMultiplication
						};
	MetaConcept() noexcept : VFTable1(0), MultiPurposeBitmap(NoModifiers_VF) {}
	explicit MetaConcept(ExactType_MC NewType) noexcept : VFTable1(&MetaConceptLookUp[NewType]),MultiPurposeBitmap(NoModifiers_VF) {};
	explicit MetaConcept(ExactType_MC NewType,unsigned char NewBitmap) noexcept : VFTable1(&MetaConceptLookUp[NewType]),MultiPurposeBitmap(NewBitmap) {};
	MetaConcept(const MetaConcept& src) = default;
	MetaConcept(MetaConcept&& src) = default;
	MetaConcept& operator=(const MetaConcept & src) = default;
	MetaConcept& operator=(MetaConcept&& src) = default;
public:
	enum Precedence {
		None = 0,	// atomic term
		Comma,
		Ellipsis,
		Addition,
		Multiplication,
		UnaryAddition,	// start unary operators
		Power,
		LParenthesis	// unconditional left-expression stop
	};

	using evalspec = std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >;

	// detects whether lhs is hard-coded logical negation of rhs
	friend bool IsAntiIdempotentTo(const MetaConcept& lhs, const MetaConcept& rhs);

	virtual ~MetaConcept() = default;
	virtual void CopyInto(MetaConcept*& dest) const = 0;	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
	ExactType_MC ExactType() const { return VFTable1->ExactType; }
	const char* name() const { return VFTable1->name; }
	bool IsExactType(ExactType_MC rhs) const { return VFTable1->ExactType == rhs; }
	bool IsInfinite() const;
	bool IsFinite() const;
	virtual const AbstractClass* UltimateType() const = 0;
	bool IsUltimateType(const AbstractClass* rhs) const;
	bool IsTypeMatch(ExactType_MC rhs1,const AbstractClass* rhs2) const { return IsExactType(rhs1) && IsUltimateType(rhs2); }
	bool HasSameImplementationAs(const MetaConcept& rhs) const { return typeid(*this) == typeid(rhs); }
	virtual bool ForceUltimateType(const AbstractClass* const rhs);
	bool IsPotentialVarName() const; // Unparsed.cxx
	bool IsPotentialArg() const; // InParse.cxx; \todo convert to static free function
//	Arity functions
	size_t min_size() const {return VFTable1->MinArity;};
	size_t max_size() const {return VFTable1->MaxArity;};
	virtual size_t size() const = 0;	// abstract function
	virtual const MetaConcept* ArgN(size_t n) const = 0;
	virtual MetaConcept* ArgN(size_t n) = 0;
// Syntactical equality and inequality
	friend bool operator==(const MetaConcept& lhs, const MetaConcept& rhs);

// Machine-format strict ordering
	bool CanCommuteUnderStdMultiplicationWith(const MetaConcept& rhs) const;
	bool InternalDataLT(const MetaConcept& rhs) const;
	// NOTE: next function is in SymConst.cxx
	bool SyntacticalStandardLT(const MetaConcept& rhs) const;
	// resume implementation in MetaCon1.cxx
	void ForceStdForm() { _forceStdForm(); }
	bool IsExplicitConstant() const { return _IsExplicitConstant(); }
	virtual bool NeverNeedsParentheses() const { return _IsExplicitConstant(); }	// expression never needs protection with parentheses from higher-priority operators
	virtual unsigned int OpPrecedence() const { return 0; }	// operator precedence.
	virtual bool IsAbstractClassDomain() const = 0;
//  Evaluation functions
	virtual evalspec canEvaluate() const = 0;
	virtual bool CanEvaluate() const = 0;	// either same or different-type
	virtual bool CanEvaluateToSameType() const = 0;				
	virtual bool SyntaxOK() const = 0;							
	virtual bool Evaluate(MetaConcept*& dest) = 0;		// same, or different type; if it succeeds, may be destructive.  Sole call wrapped in try-catch block for std::bad_alloc
	virtual bool DestructiveEvaluateToSameType() = 0;	// overwrites itself iff returns true
// text I/O functions
	std::string to_s() const;
	virtual std::string to_s_aux() const = 0;
//  defaults are 0 and fatal/false
	virtual unsigned long FunctionArity() const { return 0; }
	virtual bool EvaluateFunction(MetaConcept** const& ArgValList, unsigned long*& ArgList, MetaConcept*& Result);
	virtual const char* ViewKeyword() const { return 0; }

	void LogThis(const char* const Header) const;
// Formal manipulation functions
	bool SelfLogicalNOTWorks(void) const;	// defined inline in Class.hxx
	virtual void SelfLogicalNOT();	// instantiate when UltimateType is TruthValues
	virtual int _strictlyImplies(const MetaConcept& rhs) const { return 0; }
	virtual bool StrictlyImplies(const MetaConcept& rhs) const { return false; }
	virtual void StrictlyModifies(MetaConcept*& rhs) const {} // only RHS affected
	virtual bool CanStrictlyModify(const MetaConcept& rhs) const { return false; } // could affect only RHS
	virtual bool SelfInverse(const ExactType_MC Operation);
	virtual bool SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const;
	virtual bool LogicalANDFindDetailedRule(MetaConcept& rhs, size_t LHSIdx, size_t RHSIdx, size_t& Param1, size_t& Param2, signed short& SelfEvalIdx, unsigned short& EvalIdx) {return false;};
	virtual bool LogicalANDNonTrivialFindDetailedRule() const {return false;};
	// an "orthogonal clause" is one that has the potential to be incapable of affecting
	// Franci's reasoning about an AND clause.
	virtual bool LogicalANDOrthogonalClause() const {return false;};
	// Logical Amplification support
	virtual bool WantToBeAmplified() const {return false;};
	virtual ExactType_MC CanAmplifyClause() const { return Unknown_MC; }
	virtual bool CanAmplifyThisClause(const MetaConcept& rhs) const;
	virtual bool AmplifyThisClause(MetaConcept*& rhs) const;
	// Basis clause support
	virtual size_t BasisClauseCount() const { return 0; }
	virtual bool DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const;
	// "AND-factor" support
	virtual size_t ANDFactorCount() const {return 0;};
	virtual bool DirectCreateANDFactorIdx(size_t Idx, MetaConcept*& dest) const;
	// Interaction clause support
	virtual ExactType_MC CanMakeLHSImplyRHS() const { return Unknown_MC; }
	virtual bool MakesLHSImplyRHS(const MetaConcept& lhs, const MetaConcept& rhs) const;
	virtual bool ValidLHSForMakesLHSImplyRHS(const MetaConcept& lhs) const;
	virtual bool ValidRHSForMakesLHSImplyRHS(const MetaConcept& rhs) const;
	bool MakesLHSImplyLogicalNOTOfRHS(const MetaConcept& lhs, const MetaConcept& rhs) const;
	bool ValidRHSForMakesLHSImplyLogicalNOTOfRHS(const MetaConcept& rhs) const;
	size_t RadixSortIdxSourceListByRHSCompatible(MetaConcept** const IdxSourceList) const;
	// QState.cxx support: Hypothesis augmentation
	virtual ExactType_MC CouldAugmentHypothesis() const { return Unknown_MC; }
	virtual std::function<bool(MetaConcept*&)> _CanAugmentHypothesis(const MetaConcept& Hypothesis) const { return nullptr; }
	virtual bool CanAugmentHypothesis(const MetaConcept& Hypothesis) const {return false;};
	bool IsOne() const {return _IsOne();};				// meaningful with *
	bool IsZero() const {return _IsZero();};			// meaningful with +, *
	virtual bool IsPositive() const {return false;};	// needs total order *and* IsZero
	virtual bool IsNegative() const {return false;};	// needs total order *and* IsZero
	virtual bool IsNotZero() const {return IsPositive() || IsNegative();};
	virtual bool IsNotPositive() const {return IsZero() || IsNegative();};
	virtual bool IsNotNegative() const {return IsPositive() || IsZero();};

	virtual bool SmallDifference(const MetaConcept& rhs, signed long& Result) const;

	// StdAddition support
	virtual bool StdAddCanRAMConserveDestructiveInteract() const {return false;};
	virtual bool StdAddCanRAMConserveDestructiveInteract(const MetaConcept& Target,signed short& ActOnThisRule) const {return false;};
	virtual bool StdAddCanDestructiveInteract() const {return false;};
	virtual bool StdAddCanDestructiveInteract(const MetaConcept& Target,signed short& ActOnThisRule) const {return false;};

	// StdMultiplication support
	virtual bool StdMultCanRAMConserveDestructiveInteract() const {return false;};
	virtual bool StdMultCanRAMConserveDestructiveInteract(const MetaConcept& Target,signed short& ActOnThisRule) const {return false;};
	virtual bool StdMultCanDestructiveInteract() const {return false;};
	virtual bool StdMultCanDestructiveInteract(const MetaConcept& Target,signed short& ActOnThisRule) const {return false;};

	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src) = 0;
	// Proper uses; == doesn't count.
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const = 0;
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const = 0;
	bool MetaConceptPtrRelatedToThisConceptBy(const MetaConcept* LHS, LowLevelBinaryRelation* TargetRelation) const;
	// substitution implementation
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& LHS, const MetaConcept& RHS, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) = 0;
	virtual evalspec _CanUseThisAsMakeImply(const MetaConcept& Target) { return evalspec(); }
	bool CanUseThisAsMakeImply(const MetaConcept& Target) const;
// Grammar support
// Formal properties
	inline bool IsSymmetric() const {return (VFTable1->Bitmap1 & Symmetric_LITBMP1MC);};
	inline bool IsTransitive() const {return (VFTable1->Bitmap1 & Transitive_LITBMP1MC);};
	inline bool IsSymmetricTransitive() const {return (Symmetric_LITBMP1MC | Transitive_LITBMP1MC)==(VFTable1->Bitmap1 & (Symmetric_LITBMP1MC | Transitive_LITBMP1MC));};
	inline bool HasSimpleTransition() const {return (VFTable1->Bitmap1 & SimplePhraseClauseTransition_LITBMP1MC);};
	inline bool HasAnnihilatorKey() const {return (VFTable1->Bitmap1 & HasAnnihilatorKey_LITBMP1MC);};
	inline bool IsANDOfBasisClauses() const {return (VFTable1->Bitmap1 & AND_of_BasisClauses_LITBMP1MC);};
	inline bool NoMetaModifications() const {return !MultiPurposeBitmap;};
	inline bool IsMetaAddInverted() const {return StdAdditionInv_VF & MultiPurposeBitmap;};
	inline bool IsMetaMultInverted() const {return StdMultInv_VF & MultiPurposeBitmap;};
	// NOTE: this must be true only for n-ary types that need this kind of protection!
	inline bool UsesIntegerIntervalsAsArgs() const {return ProcessesIntegerIntervals_LITBMP1MC & MultiPurposeBitmap;};
	virtual bool ThisIsAnnihilatorKey(size_t& ArgIdx, signed short& SelfEvalRule, unsigned short& EvalRule) const;
// Atomic versions
	inline bool CouldBeAtomicGrammatical(LinguisticType_LT Target) const {return (VFTable1->GrammarInfo) & Target;};
// Functional versions
	virtual bool CouldBeGrammatical(LinguisticType_LT Target) const {return CouldBeAtomicGrammatical(Target);};

	void _syntax_ok() const;
protected:
	void SetExactType(ExactType_MC NewType) {VFTable1=&MetaConceptLookUp[NewType];};
	virtual bool EqualAux2(const MetaConcept& rhs) const = 0;
	virtual bool CanCommuteUnderStdMultiplicationWithAux(const MetaConcept& rhs) const { return false; }
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const = 0;
	virtual bool SyntacticalStandardLTAux(const MetaConcept& rhs) const {return false;};	// override for all types that have a well-defined partial ordering
	virtual void _forceStdForm() = 0;
	virtual bool _IsExplicitConstant() const = 0;
	virtual bool _IsOne() const {return false;};
	virtual bool _IsZero() const {return false;};
	bool ValidateFunctionArgList(MetaConcept** const& ArgValList, unsigned long*& ArgList) const;
	// detects whether rhs is hard-coded logical negation of *this
	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;

	// Next three implemented in MetaCon3.cxx
	int _tValStrictlyImpliesDefault(const MetaConcept& rhs) const;
	bool TValStrictlyImpliesDefault(const MetaConcept& rhs) const;
	bool TValStrictlyImpliesLogicalNOTOfDefault(const MetaConcept& rhs) const;
	bool TValLogicalNOTOfStrictlyImpliesDefault(const MetaConcept& rhs) const;
};

inline bool operator!=(const MetaConcept& lhs, const MetaConcept& rhs) {return !(lhs==rhs);}

// use standard reporting per Logging.h
void INFORM(const MetaConcept& B);
void LOG(const MetaConcept& B);

template<class T>
std::enable_if_t<std::is_base_of_v<MetaConcept, T>, T*> up_cast(MetaConcept* src)
{
	if (src && T::IsType(src->ExactType())) return static_cast<T*>(src);
	return 0;
}

template<class T>
std::enable_if_t<std::is_base_of_v<MetaConcept, T>, const T*> up_cast(const MetaConcept* src)
{
	if (src && T::IsType(src->ExactType())) return static_cast<const T*>(src);
	return 0;
}

template<class T>
std::enable_if_t<std::is_base_of_v<MetaConcept, T>, T*> fast_up_cast(MetaConcept* src)
{
	if (T::IsType(src->ExactType())) return static_cast<T*>(src);
	return 0;
}

template<class T>
std::enable_if_t<std::is_base_of_v<MetaConcept, T>, const T*> fast_up_cast(const MetaConcept* src)
{
	if (T::IsType(src->ExactType())) return static_cast<const T*>(src);
	return 0;
}

// next is in Class.cxx
// misnamed: properly ForceVarUltimateTypeTruthValues.  Not important enough to update.
bool ForceUltimateTypeTruthValues(MetaConcept*& Target);

#if 0
// next three in Related.cxx
void ForceCheckForEvaluation(const MetaConcept* Target);
void ForceClauseCheckForEvaluation(const MetaConcept* Target);
void ForcePhraseCheckForEvaluation(const MetaConcept* Target);
#endif

// next is in MetaCon2.cxx
bool ValidateArgArray(const MetaConcept * const * const ArgArray);

// next three in LexParse
bool DestructiveSyntacticallyEvaluateOnce(MetaConcept*& dest);
bool OneStageAnalyzeSituation(MetaConcept*& Situation, const clock_t EvalTime0);
std::pair<Variable*, UnparsedText*> LooksLikeVarName(MetaConcept* x);

/* NOTE: RHS is NULL afterwards if DestructiveStrongVarSubstitute goes off.
   returns true on success, false on no-effect */
bool DestructiveStrongVarSubstitute(MetaConcept*& dest, const MetaConcept& lhs, MetaConcept*& rhs);

/* NOTE: StrongVarSubstitute returns true on success, false on RAM failure */
bool StrongVarSubstitute(MetaConcept*& dest, const MetaConcept& lhs, const MetaConcept& rhs);

void CopyOrThrow(MetaConcept*& dest, MetaConcept*& src);	// throws bad_alloc on failure

namespace zaimoni {

template<>
struct is_polymorphic_base<MetaConcept> : public std::true_type {};

}

// FORMALLY CORRECT: Kenneth Boyd 9/13/1999
// A is the actual type, B is the function body as a macro
#define STANDARD_DECLARE_ARGN(A,B)	\
const MetaConcept*	\
A::ArgN(size_t n) const	\
{	\
	B	\
}	\
	\
MetaConcept*	\
A::ArgN(size_t n)	\
{	\
	B	\
}

#endif
