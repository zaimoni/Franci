// Quantify.hxx
// header for MetaQuantifier

#ifndef METAQUANTIFIER_DEF
#define METAQUANTIFIER_DEF

#include "MetaCon5.hxx"

// NOTE: since I may want to use UNICODE, consider wchar_t for variable names
// META: this is a multi-modal type.  Modes are:
//	Free: Arg1=NULL (undefined domain)
//	ForAll: Arg1=ptr to instance of AbstractClass.  Semantics of ForAll(x), Y(x) quantifier.
//  ThereIs: Arg1=ptr to instance of AbstractClass.  Semantics of ThereIs(x) s.t. Y(x) quantifier.
//  ThereIsNot: Arg1=ptr to instance of AbstractClass.  Semantics of ThereIs(x) s.t. ~Y(x) quantifier.
//	ForAllNot: Arg1=ptr to instance of AbstractClass.  Semantics of ForAll(x), ~Y(x) quantifier.
// META: logical-negation is not supported.  However, QuantifiedStatement will need to know
//  how to manipulate quantifiers to logical-negate itself.  Thus, ForAll is "dual" to
//	ThereIsNot, and ThereIs is "dual" to ForAllNot.  This constitutes a low-level dependency.
// META: the MetaQuantifier family is *illegal* as a naked fact.

enum MetaQuantifierMode		{
							ForAll_MQM = ForAll_MC-ForAll_MC,
							ThereIs_MQM = ThereIs_MC-ForAll_MC,
							Free_MQM = Free_MC-ForAll_MC,
							ForAllNot_MQM = ForAllNot_MC-ForAll_MC,
							ThereIsNot_MQM = ThereIsNot_MC-ForAll_MC
							};

// would like MetaConceptWith1Arg<AbstractClass> but this fails against ModifyArgWithRHSInducedActionWhenLHSRelatedToArg
// workaround needs virtual template function (syntax error, but could be simulated)
class MetaQuantifier final : public MetaConceptWith1Arg<>
{
private:
	static unsigned long NextID;
	unsigned long ID;
	std::string VariableName;
	unsigned char Bitmap1;
	enum Modifiers_MQ	{
						None_MQ = 0x00,
						Improvised_MQ = 0x01
						};

public:
	MetaQuantifier(const char* Name, const AbstractClass* Domain, MetaQuantifierMode CreationMode);
	MetaQuantifier(const char* Name, const AbstractClass* Domain, MetaQuantifierMode CreationMode, bool Improvised);
	MetaQuantifier(const MetaQuantifier& src) = default;
	MetaQuantifier(MetaQuantifier&& src) = default;
	MetaQuantifier& operator=(const MetaQuantifier & src);	// ACID
	MetaQuantifier& operator=(MetaQuantifier&& src) = default;
	virtual ~MetaQuantifier() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(MetaQuantifier*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this),dest); }
	void MoveInto(MetaQuantifier*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
	virtual bool ForceUltimateType(const AbstractClass* const rhs);
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override { return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >(); }
	bool CanEvaluate() const override { return false; }
	bool CanEvaluateToSameType() const override { return false; }
	virtual bool SyntaxOK() const;
	bool Evaluate(MetaConcept*& dest) override { return false; }
	bool DestructiveEvaluateToSameType() override { return false; }
// text I/O functions
	std::string to_s_start() const;
	std::string to_s_end() const;
	const char* ViewKeyword() const override { return VariableName.c_str(); }
	bool MetaConceptPtrUsesThisQuantifier(const MetaConcept* lhs) const;
	bool IsLayoutCompatible(const MetaQuantifier* rhs) const;

//  basis clause support
	size_t BasisClauseCount() const override;
// next implemented in Variable.cxx, may relocate further
	bool DirectCreateBasisClauseIdx(size_t Idx, MetaConcept*& dest) const override;

//	Lexical order for quantifiers: a partial order.
//  Free vars are > all nonfree vars, and variable name order tie-breaks.
//  Nonfree vars: forall-types never compare with thereis-types; within, use name to tie-break
	bool LexicalGT(const MetaQuantifier& rhs) const;
	bool ChangeVarNameTo(const char* NewVarName);
	bool IsImprovisedVar() const { return Improvised_MQ & Bitmap1; }

protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const;
	std::string to_s_aux() const override;
	bool _IsExplicitConstant() const override { return false; }

private:
	// This is a weak-equality test
	bool EqualAux(const MetaQuantifier& rhs) const;
};

template<class T>
bool MetaConceptWith1Arg<T>::UsesQuantifierAux(const MetaQuantifier& x) const { return x.MetaConceptPtrUsesThisQuantifier(Arg1); }

#endif
