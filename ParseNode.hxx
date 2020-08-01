#ifndef PARSE_NODE_DEF
#define PARSE_NODE_DEF 1

#include "MetaCon1.hxx"
#include "Zaimoni.STL/AutoPtr.hpp"

class ParseNode;
namespace zaimoni {

	template<>
	struct is_polymorphic_final<ParseNode> : public std::true_type {};

}

class ParseNode final : public MetaConcept
{
private:
	zaimoni::autovalarray_ptr_throws<MetaConcept*> _prefix;
	zaimoni::autovalarray_ptr_throws<MetaConcept*> _infix;
	zaimoni::autovalarray_ptr_throws<MetaConcept*> _postfix;
	zaimoni::autoval_ptr<MetaConcept> _anchor;
	zaimoni::autoval_ptr<MetaConcept> _post_anchor;
public:
	enum slice {
		CLOSED = 0
	};

	ParseNode() noexcept : MetaConcept(ParseNode_MC) {}
	// \todo slicing constructor
	// \todo anchor constructor
	ParseNode(const ParseNode& src) = default;
	ParseNode(ParseNode&& src) = default;
	ParseNode& operator=(const ParseNode& src) = default;
	ParseNode& operator=(ParseNode&& src) = default;
	virtual ~ParseNode() = default;

	void CopyInto(MetaConcept*& dest) const override { CopyInto_ForceSyntaxOK(*this, dest); };	// can throw memory failure
	void CopyInto(ParseNode*& dest) const { CopyInto_ForceSyntaxOK(*this, dest); };	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(ParseNode*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

	const AbstractClass* UltimateType() const override { return 0; }
	constexpr static bool IsType(ExactType_MC x) { return ParseNode_MC == x; }

	size_t size() const override;
	const MetaConcept* ArgN(size_t n) const override;
	MetaConcept* ArgN(size_t n) override;

	size_t size_infix() const { return _infix.size(); }
	size_t size_prefix() const { return _prefix.size(); }
	size_t size_postfix() const { return _postfix.size(); }

	bool IsAbstractClassDomain() const override { return false; }
	//  Evaluation functions
	evalspec canEvaluate() const override { return evalspec(); }
	bool CanEvaluate() const override { return false; }
	bool CanEvaluateToSameType() const override { return false; }
	bool SyntaxOK() const override { return true; }	// \todo build out
	bool Evaluate(MetaConcept*& dest) override { return false; }
	bool DestructiveEvaluateToSameType() override { return false; }
// text I/O functions
	size_t LengthOfSelfName() const override { return 0; }	// \todo build out
	void ConstructSelfNameAux(char* Name) const {}	// \todo build out

	// these may need building out
	void ConvertVariableToCurrentQuantification(MetaQuantifier& src) override {}
	// Proper uses; == doesn't count.
	bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const override { return false; }
	bool UsesQuantifierAux(const MetaQuantifier& x) const override { return false; }
	bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& LHS, const MetaConcept& RHS, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) override { return false; }

	bool EqualAux2(const MetaConcept& rhs) const override { return false; }
	bool InternalDataLTAux(const MetaConcept& rhs) const override { return false; }
	void _forceStdForm() override {}
	bool _IsExplicitConstant() const override { return false; }
};

#endif
