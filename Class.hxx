// Class.hxx
// header for AbstractClass

#ifndef ABSTRACTCLASS_DEF
#define ABSTRACTCLASS_DEF

#include "MetaCon5.hxx"
#include "TVal.hxx"
#include <string>

class AbstractClass;
namespace zaimoni {

template<>
struct is_polymorphic_final<AbstractClass> : public std::true_type {};

}

class AbstractClass final : public MetaConceptWith1Arg
{
private:
	enum AbstractClassAttributes1	{
									ProperSet							= 0x00000001,
									ProperClass							= 0x00000002,
									DenseUnderStandardTopology			= 0x00000004,
									DiscreteUnderStandardTopology		= 0x00000008,
									TopologicalCompletesWithInfinity	= 0x00000010,
									ContainsInfinity					= 0x00000020
									};
	std::string ClassName;
	unsigned long Attributes1Bitmap;
public:
	AbstractClass() : MetaConceptWith1Arg(AbstractClass_MC),Attributes1Bitmap(0) {}	// as below, but implied NULL pointer
	AbstractClass(std::string&& src) : MetaConceptWith1Arg(AbstractClass_MC), ClassName((assert(!src.empty()),std::move(src))), Attributes1Bitmap(0) {}
	AbstractClass(MetaConcept*& src);	// AbstractClass owns Target afterwards.
	AbstractClass(const AbstractClass& src) = default;
	AbstractClass(AbstractClass&& src) = default;
	AbstractClass& operator=(const AbstractClass& src);	// ACID
	AbstractClass& operator=(AbstractClass&& src) = default;
	virtual ~AbstractClass() = default;
	
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(AbstractClass*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(AbstractClass*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//	Arity functions
	size_t size() const override { return 0; }
	const MetaConcept* ArgN(size_t n) const override { return 0; }
	MetaConcept* ArgN(size_t n) override { return 0; }
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const;
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual const char* ViewKeyword() const;
// Type-specific controls
	bool SetToThis(const AbstractClass& src);	// should this be Interface?

	void SetName(const char* NewName);
	void Set_IsProperSet();
	void Set_IsProperClass();
	void Set_IsDenseUnderStandardTopology();
	void Set_IsDiscreteUnderStandardTopology();
	void Set_TopologicalCompletesWithInfinity();
	void Set_ContainsInfinity();
	inline bool IsProperSet() const {return (ProperSet & Attributes1Bitmap);};
	inline bool IsProperClass() const {return (ProperClass & Attributes1Bitmap);};
	inline bool IsDenseUnderStandardTopology() const {return (DenseUnderStandardTopology & Attributes1Bitmap);};
	inline bool IsDiscreteUnderStandardTopology() const {return (DiscreteUnderStandardTopology & Attributes1Bitmap);};
	inline bool ToleratesInfinity() const {return (TopologicalCompletesWithInfinity & Attributes1Bitmap);};
	inline bool HasInfinity() const {return (ContainsInfinity & Attributes1Bitmap);};
	static bool IsReservedSetClassName(const char* Name);
	static bool ConvertToReservedAbstractClass(MetaConcept*& Target, const char* Text);
// The subclass relation
	bool ProperSubclass(const AbstractClass& rhs) const { return _properSubclass(rhs).is(true); };
	bool Subclass(const AbstractClass& rhs) const { return _subclass(rhs).is(true); };
	inline bool ProperSuperclass(const AbstractClass& rhs) const {return rhs.ProperSubclass(*this);};
	inline bool Superclass(const AbstractClass& rhs) const {return rhs.Subclass(*this);};
// The HasAsElement relation (easier to program than IsElementOf, which would be an Interface function)
// First returns true if it is *certain*
	bool HasAsElement(const MetaConcept& rhs) const { return _hasAsElement(rhs).is(true); };
	bool DoesNotHaveAsElement(const MetaConcept& rhs) const { return _hasAsElement(rhs).is(false); };
// Set union, intersection
	bool IntersectWith(const AbstractClass& lhs);
	bool SetToIntersection(const AbstractClass& lhs, const AbstractClass& rhs);
	bool UnionWith(const AbstractClass& rhs);
	bool SetToUnion(const AbstractClass& lhs, const AbstractClass& rhs);
	bool IntersectionWithIsNULLSet(const AbstractClass& rhs) const;

//  basis clause support
	virtual size_t BasisClauseCount() const;
	virtual bool DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const;

//	Operation support routines
	bool SupportsThisOperation(ExactType_MC Operation) const { return _supportsThisOperation(Operation).is(true); };
	bool CompletelyFailsToSupportThisOperation(ExactType_MC Operation) const { return _supportsThisOperation(Operation).is(false); }
;
	// NOTE: returns true iff identity is created properly.
	// StdAddition: omnizero sets Tmp to NULL [correct default for within the StdAdd. Inverse clean rule]
	bool CanCreateIdentityForOperation(ExactType_MC Operation) const;
	bool CreateIdentityForOperation(MetaConcept*& dest, ExactType_MC Operation) const;
	void ConstructUpwardTopologicalRay(signed long EndPoint,bool Open,AbstractClass*& Domain) const;
	void ConstructUpwardTopologicalRay(MetaConcept*& EndPoint,bool Open,AbstractClass*& Domain) const;
	void ConstructUpwardTopologicalRay(const MetaConcept& EndPoint,bool Open,AbstractClass*& Domain) const;
	void ConstructDownwardTopologicalRay(signed long EndPoint,bool Open,AbstractClass*& Domain) const;
	void ConstructDownwardTopologicalRay(MetaConcept*& EndPoint,bool Open,AbstractClass*& Domain) const;
	void ConstructDownwardTopologicalRay(const MetaConcept& EndPoint,bool Open,AbstractClass*& Domain) const;
	bool StdMultiplicationCommutativeWithDomain(const AbstractClass& RHS) const;
	bool CanBeRingAddition(ExactType_MC Operation) const;
	// Note: use N==0 to check for infinite-period elements
	bool MayHaveElementsOfPeriodNUnderOp(size_t N, ExactType_MC Operation) const;
	bool IsRingUnderOperationPair(ExactType_MC CandidateAdd, ExactType_MC CandidateMult) const;
	bool HasStandardPartialOrdering() const;
	bool HasStandardTotalOrdering() const;
	bool Arg1IsAfterEndpointAlongVectorAB(const MetaConcept& Arg1, const MetaConcept& A, const MetaConcept& B) const;
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	virtual bool _IsExplicitConstant() const;
private:
//	Operation support routines
	TVal _supportsThisOperation(ExactType_MC Operation) const;
// The subclass relation
	TVal _properSubclass(const AbstractClass& rhs) const;
	TVal _subclass(const AbstractClass& rhs) const;
	TVal _properSuperclass(const AbstractClass& rhs) const {return rhs._properSubclass(*this);};
	TVal _superclass(const AbstractClass& rhs) const {return rhs._subclass(*this);};
	TVal _subclass_core(const AbstractClass& rhs) const;
// The HasAsElement relation (easier to program than IsElementOf, which would be an Interface function)
	TVal _hasAsElement(const MetaConcept& rhs) const;
};

bool ForceVarUltimateType(MetaConcept*& Target,const AbstractClass* TargetType);

// defined in MetaCon1.cxx
void CopyOrThrow(AbstractClass*& dest, AbstractClass*& src);	// throws bad_alloc on failure

// helpers defined in LexParse.cxx
const AbstractClass& NonnegativeInteger();

extern const AbstractClass TruthValues;
extern const AbstractClass Integer;
extern const AbstractClass Rational;
extern const AbstractClass Real;
extern const AbstractClass Complex;
extern const AbstractClass NULLSet;
extern const AbstractClass ClassAllSets;
extern const AbstractClass ClassAdditionDefined;
extern const AbstractClass ClassMultiplicationDefined;
extern const AbstractClass ClassAdditionMultiplicationDefined;

inline bool MetaConcept::SelfLogicalNOTWorks() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/14/1999
	return IsUltimateType(&TruthValues);
}
#endif
