#ifndef PARSE_NODE_DEF
#define PARSE_NODE_DEF 1

#include "MetaCon1.hxx"
#include "Zaimoni.STL/LexParse/Kuroda.hpp"

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
	ParseNode(kuroda::parser<MetaConcept>::sequence& dest, size_t lb, size_t ub, slice mode);	// slicing constructor
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

	const MetaConcept* c_infix_N(size_t n) const { return _infix.size() > n ? _infix[n] : 0; }
	MetaConcept* infix_N(size_t n) { return _infix.size() > n ? _infix[n] : 0; }
	void infix_reset(size_t n) { if (_infix.size() > n) _infix[n] = 0; }

	const MetaConcept* c_anchor() const { return _anchor; }
	const MetaConcept* c_post_anchor() const { return _post_anchor; }

	size_t apply_all_infix(std::function<bool(MetaConcept*&)> xform);
	bool syntax_check_infix(std::function<bool(kuroda::parser<MetaConcept>::sequence&)> xform) { return xform(_infix); }
	bool is_parentheses_wrapped() const;

	bool IsAbstractClassDomain() const override { return false; }
	unsigned int OpPrecedence() const override;
	//  Evaluation functions
	evalspec canEvaluate() const override { return evalspec(); }
	bool CanEvaluate() const override { return false; }
	bool CanEvaluateToSameType() const override { return false; }
	bool SyntaxOK() const override { return true; }	// \todo build out
	bool Evaluate(MetaConcept*& dest) override { return false; }
	bool DestructiveEvaluateToSameType() override { return false; }

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
protected:
	std::string to_s_aux() const override;
};

inline const ParseNode* IsParenthesesWrapped(const MetaConcept* arg)
{
	if (decltype(auto) node = up_cast<ParseNode>(arg)) {
		if (node->is_parentheses_wrapped()) return node;
	}
	return 0;
}

inline ParseNode* IsParenthesesWrapped(MetaConcept* arg)
{
	if (decltype(auto) node = up_cast<ParseNode>(arg)) {
		if (node->is_parentheses_wrapped()) return node;
	}
	return 0;
}

inline std::pair<MetaConcept*, ParseNode*> UnwrapParentheses(MetaConcept*& arg)
{
	while (decltype(auto) node = IsParenthesesWrapped(arg)) {
		if (decltype(auto) inner_node = IsParenthesesWrapped(node->infix_N(0))) {
			node->infix_reset(0);
			delete arg;
			arg = inner_node;
			continue;
		}
		return std::pair(node->infix_N(0), node);
	}
	return std::pair(nullptr, nullptr);
}

bool TestThroughParenthesesWrapping(const MetaConcept* arg, std::function<bool(const MetaConcept&)> test)
{
	while(decltype(auto) node = IsParenthesesWrapped(arg)) arg = node->c_infix_N(0);
	return test(*arg);
}

bool ApplyThroughParenthesesWrapping(MetaConcept* arg, std::function<bool(MetaConcept&)> xform)
{
	while (decltype(auto) node = IsParenthesesWrapped(arg)) arg = node->infix_N(0);
	return xform(*arg);
}

bool ApplyUnwrapAllParentheses(MetaConcept*& arg, std::function<bool(MetaConcept&)> xform)
{
	auto test = UnwrapParentheses(arg);
	if (test.first) {
		test.second->infix_reset(0);
		delete arg;
		arg = test.first;
	}
	return xform(*arg);
}

bool ApplyUnwrapAllParentheses(MetaConcept*& arg, std::function<bool(MetaConcept*&)> xform)
{
	auto test = UnwrapParentheses(arg);
	if (test.first) {
		test.second->infix_reset(0);
		delete arg;
		arg = test.first;
	}
	return xform(arg);
}

// #define ALLOW_POWER_PRECEDENCE 1

#endif
