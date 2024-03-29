// Integer1.hxx
// definition of IntegerNumeral

#ifndef INTEGERNUMERAL_DEF
#define INTEGERNUMERAL_DEF

#if !defined(ABSTRACTCLASS_DEF)
#error Include Class.hxx *before* Integer1.hxx
#endif

#include "MetaCon6.hxx"
#include "__IntegerNumeral.hxx"

// This type works in base 10^9
enum PowersOfTen	{
					TEN_TO_0	= 1,
					TEN_TO_1	= 10,
					TEN_TO_2	= 100,
					TEN_TO_3	= 1000,
					TEN_TO_4	= 10000,
					TEN_TO_5	= 100000,
					TEN_TO_6	= 1000000,
					TEN_TO_7	= 10000000,
					TEN_TO_8	= 100000000,
					TEN_TO_9	= 1000000000,
					WRAPLB		= 1000000000
					};

class IntegerNumeral final : public MetaConceptZeroArgs,public _IntegerNumeral
{
public:
	IntegerNumeral() : MetaConceptZeroArgs(IntegerNumeral_MC) {};
	IntegerNumeral(const IntegerNumeral& src) = default;
	IntegerNumeral(IntegerNumeral&& src) = default;
	IntegerNumeral& operator=(const IntegerNumeral& src) = default;
	IntegerNumeral& operator=(IntegerNumeral&& src) = default;
	explicit IntegerNumeral(unsigned short src) : MetaConceptZeroArgs(IntegerNumeral_MC),_IntegerNumeral(src) {};
	explicit IntegerNumeral(signed short src) : MetaConceptZeroArgs(IntegerNumeral_MC),_IntegerNumeral(src) {};
	explicit IntegerNumeral(unsigned long src) : MetaConceptZeroArgs(IntegerNumeral_MC),_IntegerNumeral(src) {};
	explicit IntegerNumeral(signed long src) : MetaConceptZeroArgs(IntegerNumeral_MC),_IntegerNumeral(src) {};
	explicit IntegerNumeral(const char* src) : MetaConceptZeroArgs(IntegerNumeral_MC),_IntegerNumeral(src) {};
	virtual ~IntegerNumeral() = default;

// inherited from MetaConcept
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(IntegerNumeral*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(IntegerNumeral*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
	bool operator==(signed short rhs) const {return _IntegerNumeral::operator==(rhs);};
	bool operator==(unsigned short rhs) const {return _IntegerNumeral::operator==(rhs);};

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
	constexpr static bool IsType(ExactType_MC x) { return IntegerNumeral_MC == x; }
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override { return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >(); }
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const {return _IntegerNumeral::SyntaxOK();};
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true
// text I/O functions
	bool IsOne() const {return _IntegerNumeral::IsOne();};
	bool IsZero() const {return _IntegerNumeral::IsZero();};
	virtual bool IsPositive() const {return _IntegerNumeral::IsPositive();};
	virtual bool IsNegative() const {return _IntegerNumeral::IsNegative();};
	virtual bool SmallDifference(const MetaConcept& rhs, signed long& Result) const;

// Formal manipulation functions
	virtual bool SelfInverse(const ExactType_MC Operation);
	virtual bool SelfInverseTo(const MetaConcept& rhs, const ExactType_MC Operation) const;

// IntegerNumeral specific functions
	const IntegerNumeral& operator=(signed short src);
	const IntegerNumeral& operator=(unsigned short src);
	const IntegerNumeral& operator=(signed long src);
	const IntegerNumeral& operator=(unsigned long src);

	static bool ConvertToIntegerNumeral(MetaConcept*& dest,const char* Text);

	// this goes here because of the automatic memory management
	bool DestructiveAddABBothZ(IntegerNumeral*& A, IntegerNumeral*& B);

protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	virtual bool SyntacticalStandardLTAux(const MetaConcept& rhs) const;
	std::string to_s_aux() const override { return _IntegerNumeral::to_s(); }
	virtual bool _IsOne() const {return _IntegerNumeral::IsOne();};
	virtual bool _IsZero() const {return _IntegerNumeral::IsZero();};
};
#endif
