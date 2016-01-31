// TruthVal.hxx
// TruthValue header

#ifndef TRUTHVAL_HXX
#define TRUTHVAL_HXX 1

//#define ALPHA_TRUTHVAL 1

#ifdef ALPHA_TRUTHVAL
#include "MetaCon7.hxx"
#include "TVal.hxx"

class AbstractClass;
extern const AbstractClass TruthValues;

template<>
struct MetaConcept_lookup<TVal>
{	
	static ExactType_MC exact_type() {return TruthValue_MC;}
	static const AbstractClass* ultimate_type() {return &TruthValues;};
	static bool syntax_ok(const TVal& x) {return true;};
	static size_t length_of_self_name(const TVal& x) {return x.LengthOfSelfName();};
	static void construct_self_name_aux(char* dest,const TVal& x) {x.ConstructSelfNameAux(dest);};
	static bool lt_aux(const TVal& lhs, const TVal& rhs) {return lhs<rhs;};
};

typedef MetaConceptExternal<TVal> TruthValue;

#else // ALPHA_TRUTHVAL
#include "MetaCon6.hxx"

class TruthValue;
namespace zaimoni {

template<>
struct is_polymorphic_final<TruthValue> : public std::true_type {};

}

class TruthValue : public MetaConceptZeroArgs
{
private:
	unsigned char TVal;
public:
	enum Flags	{
				Contradiction	= 0,
				True,
				False,
				Unknown
				};

	// note: values are declared in increasing lexical order
	TruthValue() THROW() : MetaConceptZeroArgs(TruthValue_MC),TVal(Unknown) {};
	TruthValue(Flags TestValue) THROW() : MetaConceptZeroArgs(TruthValue_MC),TVal(TestValue) {};
//	TruthValue(const TruthValue& src);	// default OK

	virtual ~TruthValue();

//	const TruthValue& operator=(const TruthValue& Source); 	// default OK
	const TruthValue& operator=(bool src)
		{
		TVal = (src) ? (unsigned char)(True) : (unsigned char)(False);
		return *this;
		};

// Specific functions: use typecasting to access from generic
	bool IsTrue() const {return True==TVal;};
	bool IsFalse() const {return False==TVal;};
	bool IsUnknown() const {return Unknown==TVal;};
	bool IsContradiction() const {return Contradiction==TVal;};

	bool CouldBeTrue() const {return True & TVal;};	// possibility checking
	bool CouldBeFalse() const {return False & TVal;};

	void SetTrue() {TVal = True;};			// explicit assignment of type
	void SetFalse() {TVal = False;};
	void SetUnknown() {TVal = Unknown;};
	void SetContradiction() {TVal = Contradiction;};

	void ForceTrue() {TVal &= True;};		// applying an inference
	void ForceFalse() {TVal &= False;};

	static bool IsLegalTValString(const char* Text);
	static bool ConvertToTruthValue(MetaConcept*& dest,const char* Text);

// These are required by MetaConcept
	virtual void CopyInto(MetaConcept*& dest) const {zaimoni::CopyInto(*this,dest);};	// can throw memory failure
	void CopyInto(TruthValue*& dest) const {zaimoni::CopyInto(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(TruthValue*& dest) {zaimoni::CopyInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const;
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true
// text I/O functions
	virtual size_t LengthOfSelfName() const;
// Formal manipulation functions
	virtual void SelfLogicalNOT();
//  type-specific auxilliaries
	size_t CreateLinearArrayIndex(const TruthValue& Arg2) const {return 4*TVal+Arg2.TVal;};
	size_t CreateLinearArrayIndex() const {return TVal;}
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
private:
	virtual bool isAntiIdempotentTo(const MetaConcept& rhs) const;
};
#endif // ALPHA TRUTHVAL

#endif
