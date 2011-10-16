// QState.cxx
// implementation of QuantifiedStatement

#include "Class.hxx"
#include "TruthVal.hxx"
#include "MetaCon3.hxx"
#include "Equal.hxx"
#include "Variable.hxx"
#include "QState.hxx"
#include "LowRel.hxx"

#include "Zaimoni.STL/Pure.C/logging.h"

// VConsole needs these strings also
extern const char* const StartExplore = "Starting to explore situation.";
extern const char* const ExperimentWithSituation = "Experimenting with conditional situation:";

const char* const ProvenByContradiction = "Using proof by contradiction, we have this true in the primary situation:";

QuantifiedStatement::SelfEvaluateRule QuantifiedStatement::SelfEvaluateRuleLookup[MaxSelfEvalRuleIdx_SER]
  =	{
	&QuantifiedStatement::SortQuantifiers
	};


//! \todo NEW command
//! <br>symmetry: (condition) IMPLIES AND()
//! <br>This is stored by QuantifiedStatement(!), and does *not* enter the clause list.
//! Franci does *not* evaluate this straight.  Rather, she uses this when faced with 
//! a what-if, and then enters Explore.
//! <p>If the what-if doesn't break the condition, and the Explore fails on an OR-arg,
//! Franci asserts the logical negation of the OR
//! <p>For instance, a useful condition 
//! <br>AND(OR(C12_EQ_0,AND(NOT C12_EQ_0,IFF(UNKNOWN,C12_LT_0,C12_GT_0)),IFF(UNKNOWN,C12_EQ_0,C12_LT_0,C12_GT_0)), ...
//! <br>basically, evaluate AND(what-if,condition).  If it reduces to an AND of IFF clauses, all of 
//! which start with UNKNOWN, the condition is satisfied (symmetry active).  TRUE also works.
//! the AND contains a list of IFF clauses.  An Explore that contradicts, whose derived 
//! hypothesis NonstrictlyImplies an IFF clause, causes the IFF clause to be removed from
//! the symmetry.  The IFF clause then is converted to the corresponding AND/NOR, evaluated,
//! and then AND'ed with the derived hypothesis and evaluated.  This is repeated until 
//! no active symmetries are affected.
//! <p>An Explore that does *NOT* contradict, that NonStrictly implies an IFF clause, causes
//! all further tests NonStrictlyImplied by the corresponding IFF clauses to be pruned as 
//! plausible.  ANDs cascade as before.

// NOTE: Standard form of QuantifiedStatement
// ArgArray[0]: statement
// ArgArray[1]..ArgArray[n]: variable quantifications
// Free variables sort to the end.
// Within blocks of ForAll, ThereIs, ThereIsNot, And ForAllNot, sort by domain and varname.

// default crashes the logic engine
QuantifiedStatement::QuantifiedStatement(const QuantifiedStatement& src)
:	MetaConceptWithArgArray(src),
	QuantifiersExplicitlySorted(src.QuantifiersExplicitlySorted)
{
	if (1<fast_size())
		{	// fix up variables to point to current quantifications
		size_t i = fast_size();
		do	ArgArray[0]->ConvertVariableToCurrentQuantification(*static_cast<MetaQuantifier*>(ArgArray[--i]));
		while(1<i);
		};
}

const QuantifiedStatement& QuantifiedStatement::operator=(const QuantifiedStatement& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/1999
	// Note that the quantified variables must be properly substituted after the construction.
	MetaConceptWithArgArray::operator=(src);
	QuantifiersExplicitlySorted = src.QuantifiersExplicitlySorted;
	if (1<fast_size())
		{	// fix up variables to point to current quantifications
		size_t i = fast_size();
		do	ArgArray[0]->ConvertVariableToCurrentQuantification(*static_cast<MetaQuantifier*>(ArgArray[--i]));
		while(1<i);
		};
	return *this;
}

void QuantifiedStatement::MoveInto(QuantifiedStatement*& dest)		// can throw memory failure.  If it succeeds, it destroys the source.
{	// FORMALLY CORRECT: Kenneth Boyd, 3/17/2000
	if (!dest) dest = new QuantifiedStatement();
	MoveIntoAux(*dest);
}

//  Type ID functions
const AbstractClass* QuantifiedStatement::UltimateType() const
{return &TruthValues;}

// Syntactical equality and inequality
bool QuantifiedStatement::EqualAux2(const MetaConcept& rhs) const
{	//! \todo IMPLEMENT
	return false;
}

// FORMALLY CORRECT: Kenneth Boyd, 4/23/2000
void QuantifiedStatement::_forceStdForm() {ForceStdFormAux();}

//  Evaluation functions
bool QuantifiedStatement::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2000
	if (!SyntaxOKAux() || 2>size()) return false;
	{
	size_t i = fast_size();
	do	if (typeid(MetaQuantifier)!=typeid(*ArgArray[--i]))
			return false;
	while(1<i);
	}
	if (    MinPhrase1Idx_MC<=ArgArray[0]->ExactType()
		|| (ForAll_MC<=ArgArray[0]->ExactType() && UnparsedText_MC>=ArgArray[0]->ExactType()))
		return false;
	return true;
}

bool QuantifiedStatement::DelegateSelfEvaluate()
{
	assert(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER<IdxSelfCurrentEvalRule);
	assert(MaxEvalRuleIdx_ER+MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER>=IdxCurrentSelfEvalRule);
	return (this->*SelfEvaluateRuleLookup[IdxCurrentSelfEvalRule-(MetaConceptWithArgArray::MaxSelfEvalRuleIdx_SER+1)])();
}

bool QuantifiedStatement::SortQuantifiers()
{	// FORMALLY CORRECT: 9/29/1999
	// This algorithm has to be in-place [RAM conservation]
	// Until I have better ideas, this is going to be a bubble-sort
	LOG("(by sorting quantifiers)");
	size_t i = fast_size()-1;
	while(0< --i)
		if (static_cast<MetaQuantifier*>(ArgArray[i+1])->LexicalGT(*static_cast<MetaQuantifier*>(ArgArray[i])))
			{
			size_t j = i;
			MetaConcept* Tmp = ArgArray[j];
			do	ArgArray[j]=ArgArray[j+1];
			while(   ++j<fast_size()-1
				  && static_cast<MetaQuantifier*>(ArgArray[j+1])->LexicalGT(*static_cast<MetaQuantifier*>(Tmp)));
			ArgArray[j]=Tmp;
			};
	SetExplicitSort();
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

void QuantifiedStatement::DiagnoseInferenceRules() const
{	// UNSTABLE
	// NOTE; unused variable deletion [1 var] is the only evaluation to a different type
	// NOTE: unused variable deletion [2+ vars], evaluation of ArgArray[0], unrolling generalized
	// associativity, and sorting the quantification are the only evaluations that evaluate
	// to the same type.
	// The first three are standard.
	if ('\x00'==QuantifiersExplicitlySorted)
		{	// ArgArray[0] to same or different type
		// NOTE: the EqualRelation delegate functions probably want to be able to take
		// unions, intersections at least
		{
		bool Target = false;
		if (ArgArray[0]->IsExactType(LogicalAND_MC))
			static_cast<MetaConnective*>(ArgArray[0])->LogicalANDImproviseDomains();
		else if (ArgArray[0]->IsExactType(ALLEQUAL_MC))
			static_cast<EqualRelation*>(ArgArray[0])->ImproviseDomainsALLEQUAL(Target);
		else if (ArgArray[0]->IsExactType(EQUALTOONEOF_MC))
			{
			if (static_cast<EqualRelation*>(ArgArray[0])->ImproviseDomainsEQUALTOONEOF(Target))
				{	// force this arg true!
				InferenceParameter1 = 0;
				IdxCurrentEvalRule = EvalForceTrue_ER;
				return;
				};
			}
		};

		if (ArgArray[0]->CanEvaluate())
			{	// FORMALLY CORRECT: Kenneth Boyd, 10/22/1999
			InferenceParameter1 = 0;
			IdxCurrentSelfEvalRule=SelfEvalRuleEvaluateArg_SER;
			return;
			};
		// stacked quantifications
		if (ArgArray[0]->IsExactType(QuantifiedStatement_MC))
			{	// FORMALLY CORRECT: Kenneth Boyd, 2/20/1999
			InferenceParameter1 = 0;
			IdxCurrentSelfEvalRule=SelfEvalRuleUnrollGeneralizedAssociativity_SER;
			return;
			}

		if (1==fast_size())
			{	// no variables
			InvokeEvalForceArg(0);
			return;
			}

		// Unused variable deletion
		{	// FORMALLY CORRECT: Kenneth Boyd, 9/13/1999
		// NOTE: ArgArray[Idx] is a MetaQuantifier for Idx>=1, and ArgArray[0] is not
		// a MetaQuantifier.  We use UsesQuantifierAux.
		size_t i = 1;
		do	if (!ArgArray[0]->UsesQuantifierAux(*static_cast<MetaQuantifier*>(ArgArray[i])))
				{	// if all n quantifiers are to be deleted, use EvalForceArg instead.
				// Initialize the vector
				// [QuantifiedStatementWantsAllVarsNotInStatement]
				if (fast_size()==i+1)
					{
SingleDeleteExit:
					if (2<fast_size())
						{
						InferenceParameter1 = i;
						IdxCurrentSelfEvalRule=SelfEvalRuleCleanArg_SER;
						return;
						}
					else{
						InvokeEvalForceArg(0);
						return;
						};
					};
				unsigned long* VectorBuffer = _new_buffer_uninitialized<unsigned long>(fast_size()-i);
				if (!VectorBuffer) goto SingleDeleteExit;
				{
				size_t j = i;
				do	VectorBuffer[j-i]=j;
				while(fast_size()> ++j);
				};
				// VectorBuffer[0] is already tested.
				AllVarsNotInThisStatement(VectorBuffer);
				if (1==ArraySize(VectorBuffer))
					goto SingleDeleteExit;
				if (fast_size()==ArraySize(VectorBuffer)+1)
					{
					InvokeEvalForceArg(0);
					return;
					}
				CompactLowLevelArrayVectorToDeleteTrailingArgs(VectorBuffer);
				return;				
				}
		while(fast_size()> ++i);
		}

		// sorting
		{	// FORMALLY CORRECT: Kenneth Boyd, 9/29/1999
		size_t i = 1;
		while(++i<fast_size())
			if (static_cast<MetaQuantifier*>(ArgArray[i])->LexicalGT(*static_cast<MetaQuantifier*>(ArgArray[i-1])))
				{
				IdxCurrentSelfEvalRule=SortQuantifiers_SER;
				return;
				}
		};
		}
}

bool QuantifiedStatement::InvokeEqualArgRule(void) const {return false;}

void
MakeVarListPlausible(MetaConcept**& VarList, MetaConcept**& PlausibleVarList)
{	// FORMALLY CORRECT: 3/29/2000
	if (VarList)
		{
		if (!PlausibleVarList)
			PlausibleVarList=VarList;
		else{
			const size_t OldPlausibleSize = ArraySize(PlausibleVarList);
			if (_resize(PlausibleVarList,OldPlausibleSize+ArraySize(VarList)))
				{
				memmove(PlausibleVarList+OldPlausibleSize,VarList,_msize(VarList));
				free(VarList);
				}
			else
				_flush(VarList);
			};
		VarList=NULL;
		};
}

bool QuantifiedStatement::Explore(const clock_t EvalTime0, bool DoNotExplain, MetaConcept**& PlausibleVarList)
{	// MUTABLE
	// This routine handles Franci's nondeterministic reasoning.
	if (ArgArray[0]->IsExactType(LogicalAND_MC))
		{
		if (static_cast<MetaConnective*>(ArgArray[0])->LogicalANDDoNotExplore())
			return false;		// all args are TruthValue variables: no exploration

		MetaConnective* ExperimentalArg0 = NULL;
		INFORM(StartExplore);
		try	{
			static_cast<MetaConnective*>(ArgArray[0])->CopyInto(ExperimentalArg0);
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			};
		// #1) extract determined variables from working problem statement.
		// #2) make an 'interest rating' of possible nondeterministic expansions
		// #3) choose an 'interesting' expansion.

		// #1) extract determined variables from working problem statement.
		ExperimentalArg0->LogicalANDCleanOrthogonalClauses(EvalTime0);
		ExperimentalArg0->LogicalANDCleanRenamedVariables(EvalTime0);
		ExperimentalArg0->ForceStdForm();
		if (2==ExperimentalArg0->size() && ExperimentalArg0->CanEvaluate())
			{
			MetaConcept* tmp = ExperimentalArg0;
			if (!DestructiveSyntacticallyEvaluateOnce(tmp))
				UnconditionalRAMFailure();
			ExperimentalArg0 = static_cast<MetaConnective*>(tmp);
			ReportTime(EvalTime0,clock());
			};

		// #2) make an 'interest rating' of possible expansions
		// make list of THEREIS TruthValue variables in use
		{	// start VarList block
		MetaConcept** VarList = NULL;	// VarList will be a list, sorted by decreasing "interest",
										// of variables/clauses to try.
		{	// start TQuantList block
		MetaQuantifier** TQuantList = NULL;	// TVarList does *not* own the quantifiers!
		if (2<=fast_size())		// ASSUMPTION: sizeof(MetaQuantifier*)==4
			{
			TQuantList = _new_buffer_nonNULL<MetaQuantifier*>(fast_size()-1);

			memmove(TQuantList,&ArgArray[1],_msize(TQuantList));

			// actually, we could use any basis-enumerable set
			size_t i = 0;
			const size_t TQuantListSize = ArraySize(TQuantList);	// keep this variable, it prevents a crash bug
			while(0<TQuantList[i]->BasisClauseCount() && TQuantListSize> ++i);
			if (TQuantListSize>i)
				{
				size_t j = i;	// Idx2 is the strict upper bound on qualified variables
				while(TQuantListSize> ++i)
					if (0<TQuantList[i]->BasisClauseCount())
						TQuantList[j++]=TQuantList[i];
				// realloc is free-and-null for size 0
				TQuantList = REALLOC(TQuantList,j*sizeof(MetaQuantifier*));
				};
			// TQuantList is now constructed.  Franci screens it further as follows:
			ExperimentalArg0->BuildInfluenceCounts(TQuantList);
			};
		//! \todo this does basis clauses, so should be modified to handle any AND-like clause
		ExperimentalArg0->LogicalANDCreateVarListV2(TQuantList,VarList,PlausibleVarList);
		};	// end TQuantList block

		// #3) choose an 'interesting' expansion.  Do it.  Repeat until none are left.
		if (   VarList
			&& ScreenVarList(EvalTime0,DoNotExplain,VarList,"All plausible-to-verify single-variable contradictions have been checked.",ExperimentalArg0))
			{
			MakeVarListPlausible(VarList,PlausibleVarList);				
			return true;
			}

		// Check plausible variables just in case
		if (   PlausibleVarList
			&& ScreenVarList(EvalTime0,DoNotExplain,PlausibleVarList,"All necessary-to-verify single-variable contradictions have been checked.",ExperimentalArg0))
			{
			MakeVarListPlausible(VarList,PlausibleVarList);
			return true;
			};

		//! \todo IMPLEMEHT
		//! Franci could have a CriticalVarList that has little to no self-implying relations.
		//! [No would be better].  That is, it is an exhaustive consistency-testing basis.
		//! We can screen 2-ary...n-ary ands of entries in this VarList.  [Of course, it might
		//! be a good idea to know whether the contradicted form (an OR) actually *does* anything.
		//! before testing it.]

		// #4) Done:
		if (VarList) BLOCKDELETEARRAY_AND_NULL(VarList);
		};	// end VarList block

		//! \todo IMPLEMENT: Use Digraph as a last-ditch measure to deal with ALLDISTINCT, 
		//! DISTINCTFROMALLOF, EQUALTOONEOF, NOTALLDISTINCT, NOTALLEQUAL
		//! \todo IMPLEMENT: Use Digraph to Explore for circuits, etc. in => caused by
		//! the statements.  We are interested in:
		//! Circuits [generate IFF]
		//! Meshed stars [generate XOR]
#if 0
		Digraph* ImpliesTracer = new Digraph(NULL,true,NULL);
		ExperimentalArg0->InitializeImpliesTracerDigraph(ImpliesTracer);
		if (ImpliesTracer)
			{
			MetaConcept** ClauseList = NULL;
			ImpliesTracer->GenerateIFFClauseList(ClauseList,ArgArray);	// generates IFF clauses not implied by prior information
			if (NULL!=ClauseList)
				{
				DELETE(ImpliesTracer);
				delete ExperimentalArg0;
				//! \todo update ArgArray[0] (inhale ClauseList)
				return true;
				}
			ImpliesTracer->GenerateXORClauseList(ClauseList,ArgArray);	// generates XOR clauses not implied by prior information
			if (NULL!=ClauseList)
				{
				DELETE(ImpliesTracer);
				delete ExperimentalArg0;
				//! \todo update ArgArray[0] (inhale ClauseList)
				return true;
				}
			DELETE(ImpliesTracer);
			}
#endif

		delete ExperimentalArg0;
		INFORM("Finished exploring situation.  The final form is:");
		INFORM(*this);
		};
#if 0
	if (   ArgArray[0]->IsExactType(LogicalOR_MC)
		&& 2==ArgArray[0]->size()
		&& (   ArgArray[0]->ArgN(0)->StrictlyImpliesLogicalNOTOf(*ArgArray[0]->ArgN(1))
			|| ArgArray[0]->ArgN(1)->StrictlyImpliesLogicalNOTOf(*ArgArray[0]->ArgN(0)))
		{	// 2-ary OR with args that directly contradict...try to decide which one is useful viewpoint
		if (ArgArray[0]->ArgN(0)->IsExactType(LogicalAND_MC))
			{	// Arg0 is AND
			}
		if (ArgArray[0]->ArgN(1)->IsExactType(LogicalAND_MC))
			{	// Arg1 is AND
			}
		}
#endif
	return false;
}

#if 0
void
MetaConnnective::InitializeImpliesTracerDigraph(Digraph*& ImpliesTracer) const
{	//! \todo VERIFY
	MetaConcept** PhantomArgArray = NULL;
	size_t Idx = fast_size();
	do	if (ArgArray[--Idx]->CanMakeLHSImplyRHS())
			{	// tell Digraph to initialize itself!
			MetaConcept** MinimalExhaustiveValidLHS = NULL;
			MetaConcept** MinimalExhaustiveValidRHS = NULL;
			size_t Idx2;
			size_t Idx3;
			if (!ArgArray[Idx]->SetMinimalExhaustiveValidLHS(MinimalExhaustiveValidLHS))	//! \todo IMPLEMENT SetMinimalExhaustiveValidLHS member function
				{	// clean failure
				continue;
				}
			if (!ArgArray[Idx]->SetMinimalExhaustiveValidRHS(MinimalExhaustiveValidRHS))	//! \todo IMPLEMENT SetMinimalExhaustiveValidRHS member function
				{	// clean failure
				BLOCKDELETEARRAY(MinimalExhaustiveValidLHS);
				continue;
				}
			Idx2 = ArraySize(MinimalExhaustiveValidLHS);
			do	if (!ImpliesTracer->AddVertex(*MinimalExhaustiveValidLHS[--Idx2]))
					{	// clean failure [remove failed insertion from candidates]
					_delete_idx(MinimalExhaustiveValidLHS,Idx2);
					}
			while(0<Idx2);
			if (NULL==MinimalExhaustiveValidLHS)
				{
				BLOCKDELETEARRAY(MinimalExhaustiveValidRHS);
				continue;
				}
			Idx2 = ArraySize(MinimalExhaustiveValidRHS);
			do	if (!ImpliesTracer->AddVertex(*MinimalExhaustiveValidRHS[--Idx2]))
					{	// clean failure [remove failed insertion from candidates]
					_delete_idx(MinimalExhaustiveValidRHS,Idx2);
					}
			while(0<Idx2);
			if (NULL==MinimalExhaustiveValidRHS)
				{
				BLOCKDELETEARRAY(MinimalExhaustiveValidLHS);
				continue;
				}
			// initialize Digraph ImpliesTracer from MakesLHSImplyRHS invocations
			Idx2 = ArraySize(MinimalExhaustiveValidLHS);
			do	{
				Idx2--;
				Idx3 = ArraySize(MinimalExhaustiveValidRHS);
				do	if (ArgArray[Idx]->MakesLHSImplyRHS(MinimalExhaustiveValidLHS[Idx2],MinimalExhaustiveValidRHS[--Idx3])
						ImpliesTracer->SetEdge(MinimalExhaustiveValidLHS[Idx2],MinimalExhaustiveValidRHS[Idx3])
				while(0<Idx3)
				}
			while(0<Idx2);
			BLOCKDELETEARRAY(MinimalExhaustiveValidRHS);
			BLOCKDELETEARRAY(MinimalExhaustiveValidLHS);
			}
	while(0<Idx);
	if (ImpliesTracer && 0==ImpliesTracer->size())
		DELETE_AND_NULL(ImpliesTracer);
}
#endif

//! \todo MOVE TO LowRel.?xx
bool CouldAugmentHypothesis(const MetaConcept& lhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/2/2001
	return lhs.CouldAugmentHypothesis();
}

void MetaConnective::ScreenVarList_IFFClean(void)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2002
	if (fast_size()>++InferenceParameter1)
		{
		if (1==InferenceParameter1)
			{
			LOG("====");
			LOG(*this);
			FATAL("Something is trying to promote an argument to itself.");
			}
		SelfEvalRuleCleanTrailingArg();
		}
}

// #define FREE_WILL 1

bool QuantifiedStatement::ScreenVarList(const clock_t EvalTime0, bool DoNotExplain, MetaConcept**& VarList, const char* const FailureMessage, MetaConnective*& ExperimentalArg0)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/25/2002
	// ASSUMPTION: VarList!=NULL
	//! \todo use symmetries when applicable
	MetaConcept* TestVarStatement = NULL;
	MetaConcept** CouldAugmentHypothesisArgs = NULL;
	if (!ExperimentalArg0->GrepArgList(CouldAugmentHypothesisArgs,::CouldAugmentHypothesis))
		//! \todo resort to alternate algorithm
		return false;
		
	size_t NonRedundantStrictUB = 0;
	size_t i = 0;
	do	{	// Systematically create versions of ExperimentalArg0 with test variables.
		// NOTE: Promotion is done to a working copy of VarList[Idx]
		// CanAugmentToHypothesis should handle
		// * VarList[Idx] not a var or MetaConnective?  Try to find its basis clause.
		// * Intercept OR, AND-with-OR arg here: make it spawn its variants.
		INFORM(ExperimentWithSituation);
		while(ExperimentalArg0->FindArgRelatedToRHS(*VarList[i],NonStrictlyImpliesLogicalNOTOf))
			{
			_delete_idx(VarList,i);
			if (ArraySize(VarList)>i) goto ExitFailure;
			}

		MetaConcept* TmpVarList = NULL;
		MetaConcept* PromotedHypothesis = NULL;	// initialized within try block before used
		try	{
			ExperimentalArg0->CopyInto(TestVarStatement);
			MetaConcept* TempIFF = new MetaConnective(IFF_MCM);
			VarList[i]->CopyInto(PromotedHypothesis);
			// Augment VarList[i] right before using it
			if (NULL!=CouldAugmentHypothesisArgs)
				{
RetryAugmentation:
				signed long Idx4 = ArraySize(CouldAugmentHypothesisArgs);
				do	if (CouldAugmentHypothesisArgs[--Idx4]->CanAugmentHypothesis(*PromotedHypothesis))
						{
						signed long Idx5 = -1;
						while(!CouldAugmentHypothesisArgs[++Idx5]->CanAugmentHypothesis(*PromotedHypothesis));
						// base version
						MetaConcept* NewIFFArg = NULL;
						PromotedHypothesis->CopyInto(NewIFFArg);
						if (!static_cast<MetaConnective*>(TempIFF)->FastInsertSlotAt(0,NewIFFArg))
							UnconditionalRAMFailure();
							
						//! \todo IMPLEMENT: want a pseudorandom algorithm here; log the fact a pseudorandom algorithm was invoked
						//! Franci has a bias towards 'nice' promotions.
						//! The test span is Idx5 to Idx4, inclusive.
						MetaConcept* PromoteCandidate1 = NULL;
						size_t PromoteCandidate1Idx = fast_size();
						while(Idx5<=Idx4)
							{
							MetaConcept* PromoteCandidate0 = NULL;
							PromotedHypothesis->CopyInto(PromoteCandidate0);
							if (CouldAugmentHypothesisArgs[Idx5]->AugmentHypothesis(PromoteCandidate0))
								{
								if (StrictlyImpliesLogicalNOTOf(*ExperimentalArg0,*PromoteCandidate0))
									// overaugmented!  recover from IFF Arg0 and retry
									DELETE_AND_NULL(PromoteCandidate0);
								else if (2<=TempIFF->size() && static_cast<MetaConnective*>(TempIFF)->FindArgRelatedToLHS(*PromoteCandidate0,AreSyntacticallyEqual))
									{	// Augmentation process has constructed an IFF
									delete PromoteCandidate0;
									delete PromoteCandidate1;
									delete PromotedHypothesis;
									free(CouldAugmentHypothesisArgs);
									delete TestVarStatement;
									delete ExperimentalArg0;

									_shrink(VarList,i);

									// If matched arg is not at fast_size()-1, truncate args above there to match
									// NOTE: InfererenceParameter1 is one less than necessary to correctly invoke SelfEvalRuleCleanTrailingArg
									// use ScreenVarList_IFFClean()
									static_cast<MetaConnective*>(TempIFF)->ScreenVarList_IFFClean();
									INFORM("IFF statement constructed while attempting to boost hypotheses:");
									INFORM(*TempIFF);
									static_cast<MetaConnective*>(TempIFF)->ForceCheckForEvaluation();
									if (!static_cast<MetaConnective*>(ArgArray[0])->AddArgAtEndAndForceCorrectForm(TempIFF))
										UnconditionalRAMFailure();
									IdxCurrentEvalRule = None_ER;
									IdxCurrentSelfEvalRule = None_SER;
									return true;
									}
								else if (PromoteCandidate0->IsExactType(Variable_MC) || PromoteCandidate0->IsExactType(LogicalIFF_MC))
									{
									DELETE_AND_NULL(PromoteCandidate1);
									LOG("Using the following:");
									LOG(*CouldAugmentHypothesisArgs[Idx5]);
									LOG("To change this hypothesis");
									LOG(*PromotedHypothesis);
									delete PromotedHypothesis;
									PromotedHypothesis = PromoteCandidate0;
									LOG("to");
									LOG(*PromotedHypothesis);
									goto RetryAugmentation;	// successful augment: immediate retry
															// failure is a RAM failure
									}
								else{
									if (NULL!=PromoteCandidate1)
										{
										// check whether PromotedHypothesis dismembers 
										// (this is *not* the same as basis, which is what Franci uses
										// to heuristically guess advanced clauses for late testing)
										if (   PromoteCandidate1->IsExactType(LogicalAND_MC)
											&& PromoteCandidate0->IsExactType(LogicalAND_MC))
											{
											if      (PromoteCandidate1->size()>PromoteCandidate0->size())
												DELETE_AND_NULL(PromoteCandidate1);
											else if (PromoteCandidate1->size()<PromoteCandidate0->size())
												DELETE_AND_NULL(PromoteCandidate0);
											else{
												PromoteCandidate0->SelfLogicalNOT();	// need to test OR version for usability
												if (ExperimentalArg0->ImprovisedUsesSpeculativeOR(*static_cast<MetaConnective*>(PromoteCandidate0)))
													{
													PromoteCandidate0->SelfLogicalNOT();
													DELETE_AND_NULL(PromoteCandidate1);
													}
												else
													DELETE_AND_NULL(PromoteCandidate0);
												};
											}
										}
									if (NULL!=PromoteCandidate0)
										{
										PromoteCandidate1Idx = Idx5;
										delete PromoteCandidate1;
										PromoteCandidate1 = PromoteCandidate0;
										PromoteCandidate0 = NULL;
										}
									}
								while(++Idx5<=Idx4 && !CouldAugmentHypothesisArgs[Idx5]->CanAugmentHypothesis(*PromotedHypothesis));
								}
							}
						if (NULL!=PromoteCandidate1)
							{
							LOG("Using the following:");
							LOG(*CouldAugmentHypothesisArgs[PromoteCandidate1Idx]);
							LOG("To change this hypothesis");
							LOG(*PromotedHypothesis);
							delete PromotedHypothesis;
							PromotedHypothesis = PromoteCandidate1;
							LOG("to");
							LOG(*PromotedHypothesis);
							goto RetryAugmentation;	// successful augment: immediate retry
													// failure is a RAM failure
							};
						if (PromoteCandidate1Idx<fast_size())
							{
							LOG("Target:");
							LOG(*PromotedHypothesis);
							LOG("and augmenter:");
							LOG(*CouldAugmentHypothesisArgs[PromoteCandidate1Idx]);
							LOG("failed to augment.");
							break;
							}
						}
				while(0<Idx4);
				};
			// remove VarList args beyond location Idx 
			// if they are nonstrictly implied by the LogicalAND of the TempIFF args.
			delete TempIFF;
			{	// Take a look ahead and see if there's something that
				// StrictlyImplies what we're about to use.
			size_t SweepIdx = i;
			while(++SweepIdx<ArraySize(VarList))
				if (VarList[SweepIdx]->StrictlyImplies(*PromotedHypothesis))
					{	// looks good for promotion
					LOG("Using the following:");
					LOG(*VarList[SweepIdx]);
					LOG("rather than:");
					LOG(*PromotedHypothesis);

					MetaConcept* Tmp = VarList[SweepIdx];
					VarList[SweepIdx] = PromotedHypothesis;
					PromotedHypothesis = Tmp;
					}
			}
			if (*PromotedHypothesis!=*VarList[i])
				{
				TmpVarList = VarList[i];
				VarList[i] = NULL;
				PromotedHypothesis->CopyInto(VarList[i]);
				};
			{	// flush everything that's after this in the testing order,
				// and implied (QualInv2Form5Alt3.txt wants this)
			size_t SweepIdx = i;
			while(++SweepIdx<ArraySize(VarList))
				if (NonStrictlyImplies(*PromotedHypothesis,*VarList[SweepIdx]))
					{
					delete VarList[SweepIdx];
					size_t Offset = 1;
					while(++SweepIdx<ArraySize(VarList))
						if (NonStrictlyImplies(*PromotedHypothesis,*VarList[SweepIdx]))
							{
							delete VarList[SweepIdx];
							Offset++;
							}
						else
							VarList[SweepIdx-Offset] = VarList[SweepIdx];
					VarList = REALLOC(VarList,_msize(VarList)-Offset*sizeof(MetaConcept*));
					};
			}
			if (!static_cast<MetaConceptWithArgArray*>(TestVarStatement)->InsertSlotAt(TestVarStatement->size(),NULL))
				UnconditionalRAMFailure();			
			}
		catch(const bad_alloc&)
			{
			UnconditionalRAMFailure();
			};
		static_cast<MetaConceptWithArgArray*>(TestVarStatement)->TransferInAndOverwriteRaw(TestVarStatement->size()-1,PromotedHypothesis);
		static_cast<MetaConceptWithArgArray*>(TestVarStatement)->ForceCheckForEvaluation();

		// evaluate
		INFORM(*TestVarStatement);
		INFORM("");
		if (DoNotExplain) suspend_logging();
		while(OneStageAnalyzeSituation(TestVarStatement,EvalTime0));
						
		// simple analysis section
		if (TestVarStatement->IsExactType(TruthValue_MC))
			{
			if (!static_cast<TruthValue*>(TestVarStatement)->CouldBeTrue())
				{	// stalled at False or Contradiction.  Proof-by-contradiction
					// asserts ~A.
				free(CouldAugmentHypothesisArgs);
				delete TestVarStatement;
				delete ExperimentalArg0;
				delete TmpVarList;

				MetaConcept* Tmp = VarList[i];
				VarList[i] = NULL;
				_shrink(VarList,i);

				INFORM(ProvenByContradiction);
				Tmp->SelfLogicalNOT();
				INFORM(*Tmp);
				// assumes ArgArray[0] is a LogicalAND..should be "AND-invoker"
				if (!static_cast<MetaConnective*>(ArgArray[0])->AddArgAtEndAndForceCorrectForm(Tmp))
					UnconditionalRAMFailure();

#if 0
				// other cases
				// OR(A,AND(NOT A,B)) AND(NOT A,B) is AND-invoker: rewrite B as AND(NOT Tmp,B))
				// XOR(A1...An,B), AND(NOT A1...NOT An,B) as AND-invoker: rewrite B as AND(NOT Tmp,B)
#endif

				ResetExplicitSort();
				return true;
				};
			//! \todo if Franci ever stops at TRUE, she should know to extract the variable
			//! as an OR-atomic clause.  However, she should first know how to explore an OR-statement.
#if 0
			if (static_cast<TruthValue*>(TestVarStatement)->IsTrue())
				{
				free(CouldAugmentHypothesisArgs);
				delete TestVarStatement;
				delete ExperimentalArg0;
				delete TmpVarList;

				MetaConcept* Tmp = VarList[Idx];
				VarList[Idx] = NULL;
				_shrink(VarList,Idx);
				INFORM("Basis clause for XOR found")
				INFORM(*Tmp);
				// if ArgArray[0] is AND,
				// * convert to OR(Tmp,AND(NOT Tmp,ArgArray[0])).
				// * Force evaluation.  Similar handling for AND-likes.
				// if ArgArray[0] is OR(A,AND(NOT A,B)) with AND(NOT A,B) as AND-invoker
				// * convert to XOR(A,Tmp,B)
				// * evaluate; have to remember that A, Tmp are known-consistent; AND-invoker will be AND(NOT A,NOT Tmp,B)
				// if ArgArray[0] is XOR(A1..An,B) with AND-invoker AND(NOT A1,...,NOT An,B)
				//
				// * convert to XOR(A1..An,Tmp,B); force evaluation with Tmp as viewpoint on B
				};
#endif
			};

		if (NULL!=TmpVarList)
			{
			delete VarList[i];
			VarList[i] = TmpVarList;
			}
		// NOTE: don't use TmpVarList again!

		// if Franci stops on an AND-clause with more than 1 variable argument,
		// she should check to see if any derived variable arguments are yet to be tested (they
		// *don't* need to be tested, since they are already consistent).  This is harder 
		// than it looks; it improves total-failure performance, but can interact badly.
		if (TestVarStatement->IsExactType(LogicalAND_MC))
			static_cast<MetaConnective*>(TestVarStatement)->LogicalANDCleanConsistentVars(i,NonRedundantStrictUB,VarList);


		//! \todo If the experimental clause has a basis, it's implicitly an OR.  Dismember it, 
		//! and check for whether the basis clauses should have already been screened.  Those 
		//! that aren't,  should be next.
		//! \todo result-type AND should screen later indexes in VarList (already-implied=>don't test)
		//! \todo FIX: else clause should screen for symmetrized basis clauses and vars, and
		//! remove them from consideration.
		//! \todo store TestVarStatement in finite map for later coincidental processing
		//! \todo IMPLEMENT: do coincidental cross-checks for IFF, OR-elimination, XOR
		//! This requires storing the unresolved results in a finite map, and checking
		//! for when processing should occur
		//! VarList[_] antiidempotent to VarList[*]: check for IFF, OR-elimination
		//! all-done: check for emergent XOR
		}
	while(ArraySize(VarList)>++i);
ExitFailure:
	delete TestVarStatement;
	free(CouldAugmentHypothesisArgs);
	INFORM(FailureMessage);
	return false;
}

#undef FREE_WILL

void
MetaConnective::LogicalANDCleanConsistentVars(size_t& Idx, size_t& NonRedundantStrictUB, MetaConcept**& VarList)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This should be called only from QuantifiedStatement::ScreenVarList, for 
	// an AND clause.  In this case, the AND clause is about to be trashed; its data is
	// irrelevant.
	size_t OldRootIdx = Idx;
	size_t RootIdx = Idx;
	while(++Idx<ArraySize(VarList))
		if (NonStrictlyImplies(*this,*VarList[Idx]))
			{
			MetaConcept* Tmp = VarList[Idx];
			memmove(&VarList[RootIdx+1],&VarList[RootIdx],sizeof(MetaConcept*)*(Idx-RootIdx));
			VarList[RootIdx]=Tmp;
			RootIdx++;
			};
	if (OldRootIdx<NonRedundantStrictUB)
		{
		MetaConcept* Tmp = VarList[NonRedundantStrictUB];
		VarList[NonRedundantStrictUB] = VarList[OldRootIdx];
		VarList[OldRootIdx] = Tmp;
		};
	Idx = NonRedundantStrictUB;
	while(0<Idx)
		if (NonStrictlyImplies(*this,*VarList[--Idx]))
			{
			MetaConcept* Tmp = VarList[Idx];
			memmove(&VarList[Idx],&VarList[Idx+1],sizeof(MetaConcept*)*(NonRedundantStrictUB-Idx));
			VarList[NonRedundantStrictUB]=Tmp;
			NonRedundantStrictUB--;
			};
	NonRedundantStrictUB++;
	Idx = RootIdx;
}

void
QuantifiedStatement::AllVarsNotInThisStatement(unsigned long*& VectorBuffer) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/19/1999
	size_t Idx = 1;
	size_t Idx2 = 1;
	do	if (!ArgArray[0]->UsesQuantifierAux(*static_cast<MetaQuantifier*>(ArgArray[VectorBuffer[Idx]])))
			VectorBuffer[Idx2++]=VectorBuffer[Idx];
	while(ArraySize(VectorBuffer)>++Idx);
	if (Idx2<Idx)
		VectorBuffer = REALLOC(VectorBuffer,Idx2*sizeof(unsigned long));
}

bool MetaConnective::LogicalANDDoNotExplore() const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/9/2000
	// This routine implements the 'no information can be extracted' tests.
	// ASSUMPTION: the statement cannot be easily evaluated further.
	// If all args are variables, no information can be extracted.
	// NOTE: a TruthValue variable is implicitly == true.
	assert(IsExactType(LogicalAND_MC));
	size_t NonTrivialCount = 0;
	size_t i = fast_size();
	do	if (   !ArgArray[--i]->LogicalANDOrthogonalClause()
			&& 2==++NonTrivialCount)
			return false;
	while(0<i);
	return true;
}

void MetaConnective::LogicalANDCleanOrthogonalClauses(const clock_t EvalTime0)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/2/2000
	//! /pre this is LogicalAND_MC
	assert(IsExactType(LogicalAND_MC));
	size_t i = 0;
	do	if (ArgArray[i]->LogicalANDOrthogonalClause())
			{
			INFORM("Removing orthogonal clauses from AND:");
			INFORM(*ArgArray[i]);
			DELETE_AND_NULL(ArgArray[i]);
			while(fast_size()> ++i)
				if (ArgArray[i]->LogicalANDOrthogonalClause())
					{
					INFORM(*ArgArray[i]);
					DELETE_AND_NULL(ArgArray[i]);
					}
			FlushNULLFromArray((MetaConcept**&)ArgArray,0);
			return;			
			}
	while(fast_size()> ++i);
}

void MetaConnective::LogicalANDCleanRenamedVariables(const clock_t EvalTime0)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/9/2000
	// A renamed variable is an IFF-arg that is a variable, and is not the FIRST
	// variable.  If Franci finds a renamed variable, she removes it and makes a comment.
	// Anything that doesn't have significant internal structure counts.
	//! \todo FIX: to make this fully correct, IFF must attempt to do variable rewrites within
	//! nonvariable args of itself.  This should be in the n-ary auxilliary function.
	//! \todo FIX: need parallel for ALLEQUAL
	assert(IsExactType(LogicalAND_MC));
	bool HaveResponded = false;
	bool LastRemoveRewriteVar = false;
	size_t i = 0;
	while(LogicalIFF_MC>ArgArray[i]->ExactType() && fast_size()-1>i) i++;
	if (ArgArray[i]->IsExactType(LogicalIFF_MC))
		do	static_cast<MetaConnective*>(ArgArray[i])->LogicalIFFRemoveRedundantVariables(HaveResponded,LastRemoveRewriteVar);
		while(fast_size()> ++i && ArgArray[i]->IsExactType(LogicalIFF_MC));
	if (HaveResponded) ReportTime(EvalTime0,clock());
}

void
MetaConnective::LogicalIFFRemoveRedundantVariables(bool& HaveResponded, bool& LastRemoveRewriteVar)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/27/2000
	// NOTE: IFF-clauses that fail to have at least one arg at least as high (numerically type)
	// as Variable_MC are *all* truthvalue args, which *will* reduce away.
	// Also, these are not AND-orthogonal, so they will have at least one non-var arg.
	if (ArgArray[1]->IsExactType(Variable_MC))
		{
		if 		(ArgArray[0]->IsExactType(Variable_MC))
			{
			if (!LastRemoveRewriteVar)
				{
				INFORM("Removing rewritten variable:");
				LastRemoveRewriteVar = true;
				};
			// not AND-orthogonal, so at least one non-var arg after vars
			size_t i = 1;
			do	INFORM(*ArgArray[i]);
			while(ArgArray[++i]->IsExactType(Variable_MC));
			DeleteNSlotsAt(i-1,1);
			HaveResponded = true;
			}
		else if (Variable_MC>ArgArray[0]->ExactType())
			// IFF arg in 0th place must be TruthValue.
			// this is being called from Explore to set up...so
			// the TruthValue is UNKNOWN, and there's only one.
			{
			if (   2<fast_size()
				&& ArgArray[2]->IsExactType(Variable_MC))
				{
				if (!LastRemoveRewriteVar)
					{
					INFORM("Removing rewritten variable:");
					LastRemoveRewriteVar = true;
					};
				size_t i = 2;
				do	INFORM(*ArgArray[i]);
				while(ArgArray[++i]->IsExactType(Variable_MC));
				DeleteNSlotsAt(i-2,2);
				HaveResponded = true;
				}
			}
		}
}

void
MetaConceptWithArgArray::BuildInfluenceCounts(MetaQuantifier**& TVarList) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/2/2000
	if (!TVarList) return;
	// #1) Build influence-counts
	unsigned long* TVarListInfluenceCount = _new_buffer<unsigned long>(ArraySize(TVarList));
	if (!TVarListInfluenceCount) UnconditionalRAMFailure();

	{
	size_t i = ArraySize(TVarList);
	do	{
		i--;
		size_t j = fast_size();
		do	if (ArgArray[--j]->UsesQuantifierAux(*TVarList[i]))
				TVarListInfluenceCount[i]++;
		while(0<j);
		}
	while(0<i);
	}
	
	// #2) if a quantifier's influence-count == 0, kill it.
	{
	size_t i = 0;
	do	if (0==TVarListInfluenceCount[i])
			{
			size_t j = i;
			while(ArraySize(TVarList)> ++i)
				if (0<TVarListInfluenceCount[i])
					{
					TVarList[j]=TVarList[i];
					TVarListInfluenceCount[j++]=TVarListInfluenceCount[i];
					};
			if (0<j)
				{
				TVarListInfluenceCount = REALLOC(TVarListInfluenceCount,j*sizeof(unsigned long));
				TVarList = REALLOC(TVarList,j*sizeof(MetaQuantifier*));
				break;
				}
			else{
				DELETEARRAY(TVarListInfluenceCount);
				DELETEARRAY_AND_NULL(TVarList);
				return;
				}
			}
	while(ArraySize(TVarList)> ++i);
	}
	// #3) sort by decreasing influence-count
	// Insertsort
	// replace by more efficient in-place sort
	if (1<ArraySize(TVarList))
		{
		size_t i = 1;
		do	if (TVarListInfluenceCount[i-1]<TVarListInfluenceCount[i])
				{
				size_t j = i;
				do	{
					swap(TVarList[j-1],TVarList[j]);
					swap(TVarListInfluenceCount[j-1],TVarListInfluenceCount[j]);
					}
				while(0< --j && TVarListInfluenceCount[j-1]<TVarListInfluenceCount[j]);
				}
		while(ArraySize(TVarList)> ++i);
		}
	DELETEARRAY(TVarListInfluenceCount);
}

void
MetaConnective::LogicalANDCreateVarListV2(MetaQuantifier**& TVarList, MetaConcept**& VarList, MetaConcept**& PlausibleVarList)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2006
	//! \todo convert this to be only MetaConceptWithArgArray::
	assert(!VarList);
	if (TVarList)
		{
		size_t NeedListEntries = 0;
		size_t i = ArraySize(TVarList);
		do	NeedListEntries += TVarList[--i]->BasisClauseCount();
		while(0<i);
		i = ArraySize(TVarList);
		VarList = _new_buffer<MetaConcept*>(NeedListEntries);
		if (!VarList) UnconditionalRAMFailure();

		do	{
			size_t j = TVarList[--i]->BasisClauseCount();
			do	if (!TVarList[i]->DirectCreateBasisClauseIdx(--j,VarList[--NeedListEntries]))
					UnconditionalRAMFailure();
			while(0<j);
			}
		while(0<i);
		DELETEARRAY_AND_NULL(TVarList);
		// remove PlausibleVars not in VarList [do not imply anything in VarList]
		if (PlausibleVarList)
			{
			i = 0;
			size_t Offset = 0;
			do	{
				size_t j = ArraySize(VarList);
				do	if (NonStrictlyImplies(*PlausibleVarList[i],*VarList[--j]))
						{
						PlausibleVarList[i-Offset]=PlausibleVarList[i];
						goto SpareThisVar;
						}
				while(0<j);
				DELETE(PlausibleVarList[i]);
				Offset++;
SpareThisVar:;
				}
			while(ArraySize(PlausibleVarList)> ++i);
			if (Offset==i)
				DELETEARRAY_AND_NULL(PlausibleVarList);
			else if (0<Offset)
				PlausibleVarList = REALLOC(PlausibleVarList,_msize(PlausibleVarList)-sizeof(Variable*)*Offset);
			}
		}

	AddBasisClausesToVarList(VarList);
	//! \todo (?): IMPLEMENT: auxilliary logical-implies effects
	//! the real-time clean code's fairly efficient, so just swap A,B when A=>B and is array-later
	//! we'll want the augmented version

	// "put plausible vars at end of list"
	if (VarList && PlausibleVarList)
		{
		size_t ImplausibleIdx = 0;
		size_t PlausibleIdx = fast_size()-1;
RetryTarget:
		size_t i = ArraySize(PlausibleVarList);
		do	if (NonStrictlyImplies(*PlausibleVarList[--i],*VarList[PlausibleIdx]))
				{
				PlausibleIdx--;
				i = (ImplausibleIdx<PlausibleIdx) ? ArraySize(PlausibleVarList) : 0;
				}
		while(0<i);
		while(ImplausibleIdx<PlausibleIdx)
			{
			i = ArraySize(PlausibleVarList);
			do	if (NonStrictlyImplies(*PlausibleVarList[--i],*VarList[ImplausibleIdx]))
					{
					MetaConcept* Tmp = VarList[ImplausibleIdx];
					memmove(&VarList[ImplausibleIdx],&VarList[ImplausibleIdx+1],sizeof(MetaConcept*)*(PlausibleIdx-ImplausibleIdx));
					VarList[PlausibleIdx]=Tmp;
					if (ImplausibleIdx<--PlausibleIdx)
						goto RetryTarget;
					i = 0;
					}
			while(0<i);
			ImplausibleIdx++;
			};
		BLOCKDELETEARRAY_AND_NULL(PlausibleVarList);
		}
}

void
MetaConnective::SuppressNonstrictlyImpliedArgsAtLargeArity(MetaConcept**& VarList, size_t& Idx3)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/27/2003
	while(0<Idx3 && 2<fast_size())
		{
		Idx3--;
		while(FindArgRelatedToLHS(*VarList[--Idx3],NonStrictlyImplies))
			{
			DeleteIdx(InferenceParameter1);
			if (2==fast_size())
				{
				Idx3++;
				return;
				}
			}
		}
}

static void
DirectCreateBasisClausesToVarlist(const MetaConcept& src,MetaConcept**& VarList)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2006
	const size_t BasisCount = src.BasisClauseCount();
	if (0<BasisCount)
		{
		const size_t TargetIdx = SafeArraySize(VarList);
		if (!_resize(VarList,TargetIdx+BasisCount))
			UnconditionalRAMFailure();

		size_t i = TargetIdx;
		do      src.DirectCreateBasisClauseIdx(i-TargetIdx,VarList[i]);
		while(++i<ArraySize(VarList));
		}
}

void MetaConnective::AddBasisClausesToVarList(MetaConcept**& VarList) const
{	// FORMALLY CORRECT: Kenneth Boyd, 4/22/2006
	//! \todo convert this to be only MetaConceptWithArgArray::
	const size_t OriginalSize = SafeArraySize(VarList);

#if 0
	// if the arg breaks down into its own basis clauses, do that and exit (but probably will be inert)
#endif

#if 0
	// if the arg has AND-factors with basis clauses, iterate through those (LogicalAND handled below)
#endif

	// LogicalAND handling...this is actually iterating through the AND-factors and taking basis clauses of each AND-factor
	size_t i = fast_size();
	// Create BasisClauseList
	do	DirectCreateBasisClausesToVarlist(*ArgArray[--i],VarList);
	while(0<i);

	const size_t NewSize = SafeArraySize(VarList);
	if (NewSize>OriginalSize)
		{	// purge non-unique entries via NonStrictlyImplies.  Vars won't count.
		size_t BlotIdx = NewSize;
		size_t ViewPointIdx = OriginalSize;
		do	{
			size_t OldBlotIdx = BlotIdx;
			size_t Idx2 = BlotIdx;
			while(   ViewPointIdx<--Idx2
				  && NonStrictlyImplies(*VarList[ViewPointIdx],*VarList[Idx2]))
				{
				if (Idx2<--BlotIdx)
					{
					MetaConcept* Tmp = VarList[Idx2];
					memmove(&VarList[Idx2],&VarList[Idx2+1],sizeof(MetaConcept*)*(BlotIdx-Idx2));
					VarList[--BlotIdx] = Tmp;
					}
				};
			while(OldBlotIdx>BlotIdx)
				{
				OldBlotIdx--;
				Idx2 = BlotIdx;
				while(   ViewPointIdx<--Idx2
					  && NonStrictlyImplies(*VarList[OldBlotIdx],*VarList[Idx2]))
					{
					if (Idx2<--BlotIdx)
						{
						MetaConcept* Tmp = VarList[Idx2];
						memmove(&VarList[Idx2],&VarList[Idx2+1],sizeof(MetaConcept*)*(BlotIdx-Idx2));
						VarList[--BlotIdx] = Tmp;
						}
					};
				};
			}
		while(++ViewPointIdx<BlotIdx);
		if (BlotIdx<ArraySize(VarList))
			_delete_n_slots_at(VarList,BlotIdx,ArraySize(VarList)-BlotIdx);
		}
}

void MetaConnective::LogicalANDImproviseDomains()
{	// FORMALLY CORRECT: Kenneth Boyd, 6/29/2000
	// ALLEQUAL: take intersection of all domains.  If nonNULL, force this domain.
	// EQUALTOONEOF: take union of all RHS domains.  Intersect this with arg0 domain.  If nonNULL,
	// force this domain on arg0.
	if (   ALLEQUAL_MC<=ArgArray[fast_size()-1]->ExactType()
		&& EQUALTOONEOF_MC>=ArgArray[0]->ExactType())
		{
Restart:
		bool Target = false;
		size_t i = fast_size();
		while(0<i && ALLEQUAL_MC<=ArgArray[--i]->ExactType())
			if		(ArgArray[i]->IsExactType(ALLEQUAL_MC))
				static_cast<EqualRelation*>(ArgArray[i])->ImproviseDomainsALLEQUAL(Target);
			else if (ArgArray[i]->IsExactType(EQUALTOONEOF_MC))
				{
				if (static_cast<EqualRelation*>(ArgArray[i])->ImproviseDomainsEQUALTOONEOF(Target))
					{
					if (2>=fast_size())
						{
						InvokeEvalForceArg(1-i);
						return;
						};
					DeleteIdx(i);
					};
				}
		if (Target) goto Restart;
		};
}

bool QuantifiedStatement::WantStateDump() const
{
	assert(!ArgArray.empty());
	if (ArgArray[0]->IsExactType(LogicalAND_MC))
		return static_cast<MetaConnective*>(ArgArray[0])->WantStateDump();
	return false;
}

bool MetaConnective::WantStateDump() const
{
	assert(IsExactType(LogicalAND_MC));
	if (   LogicalANDSpliceNAryEqualArg_SER==IdxCurrentSelfEvalRule
		|| LogicalANDSpliceIFFAntiIdempotentArg_SER==IdxCurrentSelfEvalRule
		|| LogicalANDSpliceALLEQUALAddInvArg_SER==IdxCurrentSelfEvalRule
		|| Ary2IFFToOR_SER==IdxCurrentSelfEvalRule
		|| Ary2IFFToORV2_SER==IdxCurrentSelfEvalRule
		|| Ary2IFFToORV3_SER==IdxCurrentSelfEvalRule)
		return false;
	return true;
}

bool MetaConnective::ANDValidConclusionForSymmetry() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/30/2000
	size_t i = fast_size();
	do	if (   !ArgArray[--i]->IsExactType(ALLEQUAL_MC)
			&& !ArgArray[i]->IsExactType(LogicalIFF_MC))
			return false;
	while(0<i);
	return true;
}

