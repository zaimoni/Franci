// TruthVal.hxx
// TruthValue header

#if !defined(TRUTHVALUE_DEF)
#define TRUTHVALUE_DEF

#include "MetaCon6.hxx"

class TruthValue;
namespace zaimoni {

template<>
struct is_polymorphic_final<TruthValue> : public boost::true_type {};

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

#endif
