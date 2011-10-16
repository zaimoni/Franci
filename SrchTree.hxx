// SrchTree.hxx
// Declaration of SearchTree, which implements breadth-first searches of an n-ary tree

// NOTES for the Approval Function:
// 0: terminate this leaf
// 1: still processing

#ifndef SEARCHTREE_DEF
#define SEARCHTREE_DEF

#include "MetaCon2.hxx"

class SearchTree;
namespace zaimoni {

template<>
struct is_polymorphic_final<SearchTree> : public boost::true_type {};

}

enum SEARCHTREE_RESERVED	{
							SC_FATAL = 0,
							SC_ANALYZING = 1
							};

// NOTE: The true type of a SearchTree is that of its first-branching node.

class SearchTree : public MetaConceptWithArgArray
{
private:
	SearchTree* Parent;
	LowLevelAction* BranchingOperation;
	LowLevelBinaryRelation* CanUseBranchingOperation;
	LowLevelIntValueBinaryFunction* ApprovalFunction2Ary;
	autoarray_ptr<MetaConcept*> BranchingOperationSources;
	autoarray_ptr<MetaConcept*> ApprovalTargets;
	bool __OwnBranchingOperationSources;
	bool __OwnApprovalTargets;
	bool __UniqueLeaves;
public:
	SearchTree(MetaConcept**& NewArgArray,							// specs new search tree
				LowLevelAction* NewBranchingOperation,
				LowLevelBinaryRelation* NewCanUseBranchingOperation,
				LowLevelIntValueBinaryFunction* NewApprovalFunction,
				MetaConcept**& NewBranchingOperationSources,
				MetaConcept**& NewApprovalTargets,
				bool OwnBranchingOperationSources,
				bool OwnApprovalTargets);
	SearchTree(SearchTree* NewParent, MetaConcept**& NewArgArray);	// Leaf for search tree
	SearchTree(const SearchTree& src);
	virtual ~SearchTree();

	const SearchTree& operator=(const SearchTree& src);
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(SearchTree*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(SearchTree*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual void ConstructSelfNameAux(char* Name) const;	// overwrites what is already there
// Internal functions
	void HasUniqueLeaves() {__UniqueLeaves = true;};
	void HasNonUniqueLeaves() {__UniqueLeaves = false;};
	signed int BreadthSearchOneStage(bool& RAMStalled);
	signed int ApprovalScore(const MetaConcept* const Target) const;
	bool DestructiveExtractUniqueResult(MetaConcept*& dest);
protected:
	virtual void _forceStdForm();
private:
	const MetaConcept* FindLeafLikeThis(const MetaConcept& Target) const;
	bool getBranchedArgArray(SearchTree* const Target, MetaConcept**& NewArray) const;
	bool FlushUnwantedChildren();
	void ScheduleChildDestruction(size_t i);

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
};

#endif	// SEARCHTREE_DEF
