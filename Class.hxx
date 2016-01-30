// Class.hxx
// header for AbstractClass

#ifndef ABSTRACTCLASS_DEF
#define ABSTRACTCLASS_DEF

#include "MetaCon5.hxx"

class AbstractClass;
namespace zaimoni {

template<>
struct is_polymorphic_final<AbstractClass> : public std::true_type {};

}

class TruthValue;

class AbstractClass : public MetaConceptWith1Arg
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
	autovalarray_ptr_throws<char> ClassName;
	unsigned long Attributes1Bitmap;
public:
	AbstractClass();	// as below, but implied NULL pointer
	AbstractClass(const char* const NewName);	// object now has copy of NewName
	AbstractClass(MetaConcept*& src);	// AbstractClass owns Target afterwards.
//	AbstractClass(const AbstractClass& src);	// default OK
	virtual ~AbstractClass();
	
	const AbstractClass& operator=(const AbstractClass& src);
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(AbstractClass*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(AbstractClass*& dest);	// destroys the source.
	void MoveInto(AbstractClass& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//	Arity functions
	virtual size_t size() const;
	virtual const MetaConcept* ArgN(size_t n) const;
	virtual MetaConcept* ArgN(size_t n);
//  Evaluation functions
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
// First four functions only return true if they are *certain*.
// Second four use TruthValue to handle UNKNOWN (which means "needs detailed analysis")
	bool ProperSubclass(const AbstractClass& rhs) const;
	bool Subclass(const AbstractClass& rhs) const;
	inline bool ProperSuperclass(const AbstractClass& rhs) const {return rhs.ProperSubclass(*this);};
	inline bool Superclass(const AbstractClass& rhs) const {return rhs.Subclass(*this);};
	void ProperSubclass(const AbstractClass& rhs, TruthValue& RetVal) const;
	void Subclass(const AbstractClass& rhs, TruthValue& RetVal) const;
	inline void ProperSuperclass(const AbstractClass& rhs, TruthValue& RetVal) const {rhs.ProperSubclass(*this,RetVal);};
	inline void Superclass(const AbstractClass& rhs, TruthValue& RetVal) const {rhs.Subclass(*this,RetVal);};
// The HasAsElement relation (easier to program than IsElementOf, which would be an Interface function)
// First returns true if it is *certain*
// Second uses TruthValue to handle UNKNOWN (which means "needs detailed analysis")
	bool HasAsElement(const MetaConcept& rhs) const;
	bool DoesNotHaveAsElement(const MetaConcept& rhs) const;
	void HasAsElement(const MetaConcept& rhs, TruthValue& RetVal) const;
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
	bool SupportsThisOperation(ExactType_MC Operation) const;
	bool CompletelyFailsToSupportThisOperation(ExactType_MC Operation) const;
	void SupportsThisOperation(ExactType_MC Operation, TruthValue& RetVal) const;
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
	void Subclass_core(const AbstractClass& rhs, TruthValue& RetVal) const;
};

bool ForceVarUltimateType(MetaConcept*& Target,const AbstractClass* TargetType);

// defined in MetaCon1.cxx
void CopyOrThrow(AbstractClass*& dest, AbstractClass*& src);	// throws bad_alloc on failure

// helpers defined in LexParse.cxx
const AbstractClass* NonnegativeInteger();

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
