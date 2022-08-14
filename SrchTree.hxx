// SrchTree.hxx
// Declaration of SearchTree, which implements breadth-first searches of an n-ary tree

// NOTES for the Approval Function:
// 0: terminate this leaf
// 1: still processing

#ifndef SEARCHTREE_DEF
#define SEARCHTREE_DEF

#include "MetaCon2.hxx"

enum SEARCHTREE_RESERVED	{
							SC_FATAL = 0,
							SC_ANALYZING = 1
							};

// NOTE: The true type of a SearchTree is that of its first-branching node.

class SearchTree final : public MetaConceptWithArgArray
{
private:
	SearchTree* Parent;
	LowLevelAction* BranchingOperation;
	LowLevelBinaryRelation* CanUseBranchingOperation;
	LowLevelIntValueBinaryFunction* ApprovalFunction2Ary;
	MetaConcept** BranchingOperationSources;	// not owned
	MetaConcept** ApprovalTargets;	// not owned
	bool __OwnApprovalTargets;
	bool __UniqueLeaves;

public:
	SearchTree(MetaConcept**& NewArgArray,							// specs new search tree
				LowLevelAction* NewBranchingOperation,
				LowLevelBinaryRelation* NewCanUseBranchingOperation,
				LowLevelIntValueBinaryFunction* NewApprovalFunction,
				MetaConcept** NewBranchingOperationSources,
				MetaConcept** NewApprovalTargets);
	SearchTree(SearchTree* NewParent, MetaConcept**& NewArgArray);	// Leaf for search tree
	SearchTree(const SearchTree& src) = default;
	SearchTree(SearchTree&& src) = default;
	SearchTree& operator=(const SearchTree & src) = default;
	SearchTree& operator=(SearchTree&& src) = default;
	virtual ~SearchTree() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(SearchTree*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(SearchTree*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override { return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >(); }
	virtual bool SyntaxOK() const;
// Internal functions
//	void HasUniqueLeaves() {__UniqueLeaves = true;};
//	void HasNonUniqueLeaves() {__UniqueLeaves = false;};
	int BreadthSearchOneStage(bool& RAMStalled);
	int ApprovalScore(const MetaConcept* const Target) const;
	bool DestructiveExtractUniqueResult(MetaConcept*& dest);

protected:
	void _forceStdForm() override {}
	std::string to_s_aux() const override;

private:
	void _ForceArgSameImplementation(size_t n) override;

	const MetaConcept* FindLeafLikeThis(const MetaConcept& Target) const;
	bool getBranchedArgArray(SearchTree* const Target, MetaConcept**& NewArray) const;
	bool FlushUnwantedChildren();
	void ScheduleChildDestruction(size_t i);

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
};

#endif	// SEARCHTREE_DEF
