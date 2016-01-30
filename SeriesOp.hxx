// SeriesOp.hxx
// declaration of SeriesOperation, which represents (will represent) finite and infinite 
// summation/product/etc. series notation.

// SeriesOperation will initially support + and *.
// The argument order is important:
// Arg 0: the index variable.  We may have re-quantification issues from 
// this.
// Arg 1: the range.  We currently support single integers as a degenerate case, and 
// integer-domain intervals.  We choose this formalism to simplify the introduction of 
// generic indexing sets.
// Arg 2: the expression to be series-operated over.
// The arity must be exactly 3, or else the constructor will refuse to 
// consume the presented ArgList

#ifndef SERIES_OPERATION_DEF
#define STD_SERIES_OPERATION_DEF

#include "MetaCon2.hxx"

class SeriesOperation;
namespace zaimoni {

template<>
struct is_polymorphic_final<SeriesOperation> : public std::true_type {};

}

class SeriesOperation : public MetaConceptWithArgArray
{
	enum EvalRuleIdx_ER	{
		ExpandZeroAry_ER = MetaConceptWithArgArray::MaxEvalRuleIdx_ER+1,
		ExpandUnary_ER,
		ExpandIntegerNumeralRange_ER,
		MaxEvalRuleIdx_ER = ExpandIntegerNumeralRange_ER-MetaConceptWithArgArray::MaxEvalRuleIdx_ER
		};

	typedef bool (SeriesOperation::*EvaluateToOtherRule)(MetaConcept*& dest);
	static EvaluateToOtherRule EvaluateRuleLookup[MaxEvalRuleIdx_ER];

	mutable autoval_ptr<AbstractClass> DesiredType;
public:
	enum IndexCodes	{
					INDEXVAR_IDX = 0,
					DOMAIN_IDX =  1,
					EXPRESSION_IDX = 2,
					LEGAL_ARITY = 3,
					OperationCount = SeriesMultiplication_MC-SeriesAddition_MC+1
					};

	static ExactType_MC CoreOperation[OperationCount];	

	SeriesOperation(ExactType_MC Operation);
	SeriesOperation(MetaConcept**& NewArgList,ExactType_MC Operation);
//	SeriesOperation(const SeriesOperation& src);	// default OK
	virtual ~SeriesOperation();

	const SeriesOperation& operator=(const SeriesOperation& src);	// provided to be ACID
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(SeriesOperation*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(SeriesOperation*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
	virtual bool ForceUltimateType(const AbstractClass* const rhs);

//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
// Type-specific functions
	bool IsFiniteSeries() const;
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	virtual void _forceStdForm();
	virtual bool _IsExplicitConstant() const;
	virtual bool _IsOne() const;
	virtual bool _IsZero() const;

//  Helper functions for CanEvaluate... routines
	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
private:
	size_t FactorialIsAppropriateRepresentation() const;

	virtual bool DelegateEvaluate(MetaConcept*& dest);		// different type

	bool ExpandZeroAry(MetaConcept*& dest);
	bool ExpandUnary(MetaConcept*& dest);
	bool ExpandIntegerNumeralRange(MetaConcept*& dest);	
};

#endif
