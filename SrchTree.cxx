// SrchTree.cxx
// Implementation of SearchTree, which implements breadth-first searches of an n-ary tree

#include "Class.hxx"
#include "SrchTree.hxx"

SearchTree::SearchTree(MetaConcept**& NewArgArray,							// specs new search tree
			LowLevelAction* NewBranchingOperation,
			LowLevelBinaryRelation* NewCanUseBranchingOperation,
			LowLevelIntValueBinaryFunction* NewApprovalFunction,
			MetaConcept** NewBranchingOperationSources,
			MetaConcept** NewApprovalTargets)
:	MetaConceptWithArgArray(Unknown_MC,NewArgArray),
	Parent(0),
	BranchingOperation(NewBranchingOperation),
	CanUseBranchingOperation(NewCanUseBranchingOperation),
	ApprovalFunction2Ary(NewApprovalFunction),
	BranchingOperationSources(NewBranchingOperationSources),
	ApprovalTargets(NewApprovalTargets),
	__UniqueLeaves(true)
{
}

SearchTree::SearchTree(SearchTree* NewParent, MetaConcept**& NewArgArray)	// Leaf for search tree
:	MetaConceptWithArgArray(Unknown_MC,NewArgArray),
	Parent(NewParent),
	BranchingOperation(0),
	CanUseBranchingOperation(0),
	ApprovalFunction2Ary(0),
	__UniqueLeaves(false)
{
}

//  Type ID functions
const AbstractClass* SearchTree::UltimateType() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2003
	if (ArgArray.empty()) return 0;
	return ArgArray[2==fast_size()]->UltimateType();
}

//  Evaluation functions
bool SearchTree::SyntaxOK() const
{
	if (!SyntaxOKAux()) return false;
	if (!Parent)	// top-level
		{
		if (   !BranchingOperation
			|| !CanUseBranchingOperation
			|| !ApprovalFunction2Ary
			|| !ValidateArgArray(BranchingOperationSources)
			|| !ValidateArgArray(ApprovalTargets))
			return false;
		}
	else{
		if (   BranchingOperation
			|| CanUseBranchingOperation
			|| ApprovalFunction2Ary
			|| !BranchingOperationSources
			|| !ApprovalTargets)
			return false;
		};
	return true;
}

void SearchTree::_ForceArgSameImplementation(size_t n) { NARY_FORCEARGSAMEIMPLEMENTATION_BODY; }

//! \todo IMPLEMENT correctly (placeholder now)
std::string SearchTree::to_s_aux() const { return "SEARCHTREE(,)"; }

const MetaConcept*
SearchTree::FindLeafLikeThis(const MetaConcept& Target) const
{
	if (1==fast_size())
		return (*ArgArray[0]==Target) ? ArgArray[0] : NULL;

	size_t i = fast_size();
	do	{
		const MetaConcept* tmp = static_cast<SearchTree*>(ArgArray[--i])->FindLeafLikeThis(Target);
		if (tmp) return tmp;
		}
	while(1<i);
	return NULL;
}

bool
SearchTree::getBranchedArgArray(SearchTree* const Target, MetaConcept**& NewArray) const
{	// TODO: VERIFY
	if (Parent) return Parent->getBranchedArgArray(Target,NewArray);

	assert(!NewArray);
	MetaConcept* LocalRoot = Target->ArgArray[0];
	size_t i = ArraySize(BranchingOperationSources);
	do	if (CanUseBranchingOperation(*LocalRoot,*BranchingOperationSources[--i]))
			{
			size_t CountBranches = 1;
			size_t HighBound = i;
			size_t LowBound = i;
			while(0<i)
				if (CanUseBranchingOperation(*LocalRoot,*BranchingOperationSources[--i]))
					{
					CountBranches++;
					LowBound = i;
					};

			zaimoni::autovalarray_ptr<MetaConcept*> tmp(CountBranches+1);
			if (!tmp.empty())
				{
				size_t j = HighBound+1;
				do	if (CanUseBranchingOperation(*LocalRoot,*BranchingOperationSources[--j]))
						{
						zaimoni::autovalarray_ptr<MetaConcept*> tmp_unary(1);
						if (tmp_unary.empty()) return true;
						try	{
							LocalRoot->CopyInto(tmp_unary[0]);
							BranchingOperation(tmp_unary[0],*BranchingOperationSources[j]);
							SUCCEED_OR_DIE(*LocalRoot!=*tmp_unary[0]);
							if (__UniqueLeaves && FindLeafLikeThis(*tmp_unary[0]))
								{
								if (2==tmp.size()) return false;
								tmp.FastDeleteIdx(CountBranches--);
								}
							else
								tmp[CountBranches--] = new SearchTree(Target,tmp_unary);
							}
						catch(const bad_alloc&)
							{
							return true;
							}
						}
				while(LowBound<j);
				tmp[0] = LocalRoot;
				NewArray = tmp.release();
				}
			return true;
			}
	while(0<i);
	return false;
}

int SearchTree::ApprovalScore(const MetaConcept* const Target) const
{
	if (Parent) return Parent->ApprovalScore(Target);

	signed int CumulativeRating = 0;
	if (ApprovalFunction2Ary)
		{	//	2-ary approval function; highest rating passes through (may need other modes later)
		size_t i = ArraySize(ApprovalTargets);
		do	{
			signed int LocalRating = ApprovalFunction2Ary(*Target,*ApprovalTargets[--i]);
			if (LocalRating>CumulativeRating)
				CumulativeRating = LocalRating;
			}
		while(0<i);
		return CumulativeRating;
		}
	return SC_FATAL;
}

bool SearchTree::DestructiveExtractUniqueResult(MetaConcept*& dest)
{
	if (2<fast_size()) return false;
	if (2==fast_size())
		return static_cast<SearchTree*>(ArgArray[1])->DestructiveExtractUniqueResult(dest);

	FastCleanTransferOutAndNULL(0,dest);
	return true;
}

bool SearchTree::FlushUnwantedChildren()
{
	if (   1<InferenceParameter1
		&& SelfEvalRuleCleanTrailingArg_SER==IdxCurrentSelfEvalRule)
		{
		SelfEvalRuleCleanTrailingArg();
		return true;			
		}
	return false;
}

// Internal functions
int SearchTree::BreadthSearchOneStage(bool& RAMStalled)
{	// TODO: VERIFY
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	// scan loop for evaluation
	if (SelfEvalRuleCleanTrailingArg_SER==IdxCurrentSelfEvalRule)
		AUDIT_STATEMENT(return SC_FATAL);

	signed int HighScore = SC_ANALYZING;

	// TODO: OPTIMIZE: RAM: memory management is useful here!
	// if we have a RAM failure:
	// * if SelfEvalRuleCleanTrailingArg_SER is already primed, we can flush those args 
	// and retry
	// * if no args are known-flushable yet, treat as in-progress (it is)
	// (after something has been deleted)
	if (1==fast_size())
		{	// spawn
		DEBUG_LOG("Spawning from");
		DEBUG_LOG(*ArgArray[0]);

		signed int Score = ApprovalScore(ArgArray[0]);
		if (SC_ANALYZING!=Score)
			{
			DEBUG_LOG("Score:");
			DEBUG_LOG(Score);
			RAMStalled = false;
			return Score;
			}

		MetaConcept** BranchedArgArray = NULL;
		if (getBranchedArgArray(this,BranchedArgArray))
			{
			DEBUG_LOG("have branches");
			if (NULL==BranchedArgArray)
				{	// don't reset RAMStalled: this allows detecting problems
				DEBUG_LOG("RAM failure");
				AUDIT_STATEMENT(return SC_ANALYZING);
				}
			}
		else{
			DEBUG_LOG("No branches");
			RAMStalled = false;
			AUDIT_STATEMENT(return SC_FATAL);
			}
		if (ArgArray!=BranchedArgArray)
			{
			FREE((MetaConcept**)ArgArray);
			ArgArray.NULLPtr();
			ArgArray = BranchedArgArray;
			}

		size_t i = fast_size();
		// NOTE: ApprovalScore also needs RAMStalled interface
		do	{
			DEBUG_LOG(i);
			signed int Score = ApprovalScore(static_cast<SearchTree*>(ArgArray[--i])->ArgArray[0]);
			
			DEBUG_LOG("Score:");
			DEBUG_LOG(Score);
			DEBUG_LOG(HighScore);
			if 		(Score<HighScore)
				{
				ScheduleChildDestruction(i);
				}
			else if (Score>HighScore)
				{
				HighScore = Score;
				++i;
				if (i<fast_size())
					{
					if (SelfEvalRuleCleanTrailingArg_SER!=IdxCurrentSelfEvalRule)
						ScheduleChildDestruction(i);
					while(i<InferenceParameter1)
						ScheduleChildDestruction(i);
					}
				--i;
				}
			}
		while(1<i);
		RAMStalled = false;
		}
	else{	// already spawned: recurse
		DEBUG_LOG("Testing breadth-first search alternatives");
		size_t i = fast_size();
		do	{
			signed int Score = SC_FATAL;
			bool LocalRAMStalled = true;
			--i;
			DEBUG_LOG(i);
			LOG(*ArgArray[i]->ArgN(0));
			do	Score = static_cast<SearchTree*>(ArgArray[i])->BreadthSearchOneStage(LocalRAMStalled);
			while(LocalRAMStalled && FlushUnwantedChildren());
			if (!LocalRAMStalled) RAMStalled = false;

			DEBUG_LOG("loop OK");
			DEBUG_LOG(Score);
			DEBUG_LOG(HighScore);
			if 		(Score<HighScore)
				{
				ScheduleChildDestruction(i);
				RAMStalled = false;
				}
			else if (Score>HighScore)
				{
				HighScore = Score;
				++i;
				if (i<fast_size())
					{
					if (SelfEvalRuleCleanTrailingArg_SER!=IdxCurrentSelfEvalRule)
						ScheduleChildDestruction(i);
					while(i<InferenceParameter1)
						ScheduleChildDestruction(i);
					}
				--i;
				RAMStalled = false;
				}
			}
		while(1<i);
		}

	DEBUG_LOG("Branching OK");

	if (SelfEvalRuleCleanTrailingArg_SER==IdxCurrentSelfEvalRule)
		{
		if (1==InferenceParameter1)
			{
			if (NULL==Parent && 3<=fast_size())
				{
				InferenceParameter1++;
				SelfEvalRuleCleanTrailingArg();
				InferenceParameter1--;
				IdxCurrentSelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
				AUDIT_STATEMENT(return SC_FATAL);
				}
			else{
				SelfEvalRuleCleanTrailingArg();
				AUDIT_STATEMENT(return SC_FATAL);
				}
			};
		SelfEvalRuleCleanTrailingArg();
		}
	DEBUG_LOG("Leaving SearchTree::BreadthSearchOneStage");
	if (SC_ANALYZING<HighScore && 2<fast_size())
		{
		InferenceParameter1 = 2;
		SelfEvalRuleCleanTrailingArg();		
		}
	return HighScore;
}

void SearchTree::ScheduleChildDestruction(size_t i)
{
	if (SelfEvalRuleCleanTrailingArg_SER!=IdxCurrentSelfEvalRule)
		{
		IdxCurrentSelfEvalRule = SelfEvalRuleCleanTrailingArg_SER;
		InferenceParameter1 = fast_size();
		}
	if (i<InferenceParameter1 && i< --InferenceParameter1)
		swap(ArgArray[i],ArgArray[InferenceParameter1]);
}

// FORMALLY CORRECT: Kenneth Boyd, 8/17/2003
void SearchTree::DiagnoseInferenceRules() const {}

// FORMALLY CORRECT: Kenneth Boyd, 8/17/2003
bool SearchTree::InvokeEqualArgRule() const {return false;}

