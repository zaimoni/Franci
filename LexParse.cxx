// LexParse.cxx
// implementation of Franci's linguistic knowledge

#include "InParse.hxx"
#include "Class.hxx"
#include "TruthVal.hxx"
#include "MetaCon3.hxx"
#include "QState.hxx"
#include "Variable.hxx"
#include "Unparsed.hxx"
#include "FlatFile.hxx"
#include "CmdShell.hxx"
#include <ctype.h>

#include "Zaimoni.STL/except/syntax_error.hpp"
#include "Zaimoni.STL/Pure.C/logging.h"

// defined in VConsole.cxx
void GetLineFromKeyboardHook(char*& InputBuffer);

CmdShell FranciScript;

// reference strings for commands
#define CONSTRAINT "constraint:"
#define EVALUATE "evaluate"
#define EVALUATE_SITUATION "evaluate situation"
#define NEW_VARIABLE "new variable:"
#define WHAT_IF "what if"
#define WHAT_IS "what is"

typedef bool LexParseHandler(char*& InputBuffer);
typedef LexParseHandler* ActiveLexParseHandler(char*& InputBuffer);

//! \todo at some point, we're going to need a 'default knowledge' variable

// improvisation of variables is controlled from the syntax-checker routine
// [actually, the syntax-checkers at Phrase__ and Clause__]
// [wants global access to Situation]
// inline variables are controlled from the syntax-checker routine
// [wants global access to current parsing]
// new global function for above: ResolvePotentialVariables

//! \todo support for multiple problems
//! This only provides support for one problem at a time, which is implicit.
//! While building this, the problem is really a QuantifiedStatement
//! This is a target for a C++ class; must do this before multiple-situation support
MetaConcept* Situation = NULL;
bool NoMoreVarsForSituation = false;	// turns on a syntax checker when true
size_t SituationTimeLimit = 0;			// time limit in seconds; 0 is no time limit
clock_t EvalTime0;						// start evaluation
bool DoNotExplain = false;				// turns off full explanations
// 2-stage construction so improvisiation for evaluate-expression does not affect situation
autoarray_ptr<MetaQuantifier*> NewVarsOnThisPass;		// new vars on this pass ... oops, this is a proper class...

bool
InitMetaConceptParserArray(autoarray_ptr<MetaConcept*>& ArgArray,char*& InputBuffer)
{	//! \pre ArgArray is intended to have one slot (e.g., autoarray_ptr<MetaConcept*> ArgArray(1);
	DEBUG_FAIL_OR_LEAVE(1!=ArgArray.size(),return false);
	assert(NULL==ArgArray[0]);
	ArgArray[0] = new(nothrow) UnparsedText(InputBuffer);
	if (NULL==ArgArray[0]) return false;
	// below doesn't work for FORTRAN (comments are 5 spaces with & as sixth character)
	static_cast<UnparsedText*>(ArgArray[0])->WS_Strip();
	return true;
}


// AbstractClass helpers -- here to permit clearing on new situation
static AbstractClass* Z_0 = NULL;
const AbstractClass* NonnegativeInteger()
{	// FORMALLY CORRECT: Kenneth Boyd, 1/9/2004
	if (NULL==Z_0) Integer.ConstructUpwardTopologicalRay(0,false,Z_0);
	return Z_0;
}

bool DestructiveSyntacticallyEvaluateOnce(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/2/2006
	assert(dest);
	const long revert_to = get_checkpoint();
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	DEBUG_LOG(dest->name());
	DEBUG_LOG(*dest);
	dest->ForceStdForm();
	if (dest->CanEvaluate())
		{
		DEBUG_LOG("dest->CanEvaluate() true");
		if (dest->CanEvaluateToSameType())
			{
			DEBUG_LOG("Leaving; dest->CanEvaluateToSameType() true");
			bool Tmp = dest->DestructiveEvaluateToSameType();
			SUCCEED_OR_DIE(dest->SyntaxOK());
			DEBUG_LOG(dest->name());
			DEBUG_LOG(*dest);	
			return Tmp;
			};
		DEBUG_LOG("dest->CanEvaluateToSameType() false");
		try	{
			MetaConcept* Tmp = NULL;
			if (dest->Evaluate(Tmp))
				{
				DEBUG_LOG("Leaving; dest->Evaluate(Tmp) true");
				assert(Tmp);
				delete dest;
				dest = Tmp;
				SUCCEED_OR_DIE(dest->SyntaxOK());
				DEBUG_LOG(dest->name());
				DEBUG_LOG(*dest);
				return true;
				};
			DEBUG_LOG("Leaving/false fallthrough");
			}
		catch(const std::bad_alloc&)
			{
			DEBUG_LOG("Leaving/false bad_alloc");
			return false;
			}
		};
	// fall-through
	revert_checkpoint(revert_to);
	return false;
}

// #define FRANCI_WARY 1

bool
OneStageAnalyzeSituation(MetaConcept*& Situation, const clock_t EvalTime0)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/10/2003
	// returns true iff more processing to do
	// using goto to condense exit code; this routine is I/O heavy
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	DEBUG_LOG(Situation->name());
	Situation->ForceStdForm();
	if (Situation->CanEvaluate())
		{
		DEBUG_LOG("CanEvaluate OK, true");
		bool WantsStateDump = !Situation->IsExactType(QuantifiedStatement_MC) || static_cast<QuantifiedStatement*>(Situation)->WantStateDump();
		DEBUG_LOG("OneStageAnalyzeSituation/DestructiveSyntacticallyEvaluateOnce");
		if (!DestructiveSyntacticallyEvaluateOnce(Situation))
			{
			_console->ResumeLogFile();
			WARNING("Stopping evaluation due to RAM shortage.");
			goto StopEvalLeave;
			};
		DEBUG_LOG("OneStageAnalyzeSituation/DestructiveSyntacticallyEvaluateOnce OK/true");
		if (WantsStateDump)
			Situation->LogThis("evaluates to:");
		ReportTime(EvalTime0,clock());
		DEBUG_LOG("Leaving/true");
		DEBUG_LOG(ZAIMONI_FUNCNAME);
		return true;
		};
	DEBUG_LOG("CanEvaluate OK, false");
	_console->ResumeLogFile();
	Situation->LogThis("Final form is:");
	INFORM("Finished evaluating situation.");
StopEvalLeave:
	ReportTime(EvalTime0,clock());
	DEBUG_LOG("Leaving/false");
	DEBUG_LOG(ZAIMONI_FUNCNAME);
	return false;
}

#undef FRANCI_WARY

/////////
inline void
DestructiveSyntacticallyEvaluate(MetaConcept*& Target)
{while(DestructiveSyntacticallyEvaluateOnce(Target));}

static MetaQuantifier* PointToLookupQuantifier(const UnparsedText& Target, MetaConcept*& Situation, const AbstractClass* Domain)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/15/1999
	// Domain NULL: FREE, improvised
	// Domain nonNULL: THEREIS, improvised
	assert(Target.IsQuasiEnglishOrVarName());
	if (NULL!=Situation && Situation->IsExactType(QuantifiedStatement_MC))
		{	// the situation itself has quantifications.  Use these in preference to straight
			// improvisation.
		size_t i = Situation->size();
		while(1<i)
			{
			MetaConcept* Tmp = Situation->ArgN(--i);
			assert(NULL!=Tmp);
			assert(typeid(MetaQuantifier)==typeid(*Tmp));
			if (   NULL!=Target.ViewKeyword() && NULL!=Tmp->ViewKeyword()
				&& !strcmp(Target.ViewKeyword(),static_cast<MetaQuantifier*>(Tmp)->ViewKeyword()))
				{
				if (!Tmp->ForceUltimateType(Domain)) return NULL;
				return static_cast<MetaQuantifier*>(Tmp);
				};
			}
		};
	
	if (NULL!=NewVarsOnThisPass)
		{	// scan variables that already have been improvised
		size_t i = NewVarsOnThisPass.ArraySize();
		do	{
			MetaQuantifier* Tmp = NewVarsOnThisPass[--i];
			assert(NULL!=Tmp);
			if (   NULL!=Target.ViewKeyword() && NULL!=Tmp->ViewKeyword()
				&& !strcmp(Target.ViewKeyword(),Tmp->ViewKeyword()))
				{
				if (NULL==Domain || Tmp->IsUltimateType(Domain)) return Tmp;
				// NOTE: we may be able to put this off a while
				//! \todo IMPLEMENT: attempt to fix domain; if successful, return Tmp
				};
			}
		while(0<i);
		};
	return NULL;
}

MetaQuantifier*
PointToImprovisedQuantifier(const UnparsedText& Target, MetaConcept*& Situation, const AbstractClass* Domain)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/15/1999
	// Domain NULL: FREE, improvised
	// Domain nonNULL: THEREIS, improvised
	// NOTE: using goto to condense exit code
	assert(Target.IsQuasiEnglishOrVarName());
	{
	MetaQuantifier* Tmp = PointToLookupQuantifier(Target,Situation,Domain);
	if (NULL!=Tmp) return Tmp;
	}

	// No more variables: do not improvise
	if (NoMoreVarsForSituation) return NULL;
	if (!NewVarsOnThisPass.Resize(NewVarsOnThisPass.size()+1)) return NULL;

	// if that didn't work, improvise a new variable.
	try	{
		NewVarsOnThisPass.back() = new MetaQuantifier(Target.ViewKeyword(),Domain,(NULL==Domain) ? Free_MQM
																						: ThereIs_MQM,true);
#if 0
		// but SyntaxOK is a freebie (any failures would be fatal RAM failure)
		if (!NewVarsOnThisPass.back()->SyntaxOK())
			{
			NewVarsOnThisPass.Shrink(ArraySize(NewVarsOnThisPass)-1);
			return NULL;
			}
#endif
		return NewVarsOnThisPass.back();
		}
	catch(const bad_alloc&)
		{
		NewVarsOnThisPass.Shrink(NewVarsOnThisPass.ArraySize()-1);
		return NULL;
		}
}

// #define FRANCI_WARY 1

bool
ImproviseVar(MetaConcept*& Target, const AbstractClass* Domain)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/15/1999
	// 2 cases: variable, we want to change the domain
	// UnparsedText, we want a variable
	assert(NULL!=Target && Target->IsPotentialVarName());
	if		(Target->IsExactType(Variable_MC))
		{	// domain-change
//		if (NULL!=Domain)	// Franci: changing to FREE loses information -- fails
		// NOTE: we shouldn't need to do this for a while....
		//! \todo IMPLEMENT: improvised free variable specializes instantly to improvised THEREIS
		// identical domain is an alpha error [no-action call]
		// otherwise, must do domain-compatibility check: need non-empty intersection of
		// current, requested domains.  Hard-code this for now.  All of this can be delegated.
		// all of this only works on improvised quantifiers
//			return static_cast<Variable*>(Target)->ChangeDomain(Domain);
		return false;
		}
	else{	// request-var
		SUCCEED_OR_DIE(Target->IsExactType(UnparsedText_MC));
		// 1) splice in an quantifier at top-level with the current var-name
		// and domain, if necessary.  Return pointer to quantifier.
#ifdef FRANCI_WARY
		LOG("Attempting PointToImprovisedQuantifier");
		MetaQuantifier* Tmp2 = PointToImprovisedQuantifier(*static_cast<UnparsedText*>(Target),Situation,Domain);
		LOG("PointToImprovisedQuantifier OK");
#else
		MetaQuantifier* Tmp2 = PointToImprovisedQuantifier(*static_cast<UnparsedText*>(Target),Situation,Domain);
#endif
		if (NULL!=Tmp2)
			{	// 2) create variable pointing to quantifier
			Variable* Tmp = new(nothrow) Variable(Tmp2);
			if (NULL!=Tmp)
				{
				// 3) delete the raw name, and put in the variable.
				delete Target;
				Target = Tmp; 
				return true;
				}
			};
		};
	return false;
}

#undef FRANCI_WARY

bool
CoerceArgType(MetaConcept* const& Arg, const AbstractClass& ForceType)
{
	if (	!Arg->ForceUltimateType(&ForceType)
		&& (   !Arg->IsPotentialVarName()
			|| !ImproviseVar(*const_cast<MetaConcept**>(&Arg),&ForceType)))
		return false;
	return true;
}

bool
LookUpVar(MetaConcept*& Target, const AbstractClass* Domain)
{	// FORMALLY CORRECT: Kenneth Boyd, 12/15/1999
	// 2 cases: variable, we want to change the domain
	// UnparsedText, we want a variable
	assert(NULL!=Target && Target->IsExactType(UnparsedText_MC));
	// request-var
	// 1) splice in an quantifier at top-level with the current var-name
	// and domain, if necessary.  Return pointer to quantifier.
	MetaQuantifier* Tmp2 = PointToLookupQuantifier(*static_cast<UnparsedText*>(Target),Situation,Domain);
	if (NULL!=Tmp2)
		{	// 2) create variable pointing to quantifier
		Variable* Tmp = new(nothrow) Variable(Tmp2);
		if (NULL!=Tmp)
			{	// 3) delete the raw name, and put in the variable.
			delete Target;
			Target = Tmp; 
			return true;
			}
		};
	return false;
}

// normal implementation
inline bool
NoCurrentSituation(void)
{return NULL==Situation && NoMoreVarsForSituation;}

// handlers
bool
StartLogfile_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/1999
	_console->StartLogFile();
	return true;
}

bool
EndLogfile_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/1999
	_console->EndLogFile();
	return true;
}

bool
CleanLogfile_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/16/2000
	_console->CleanLogFile();
	return true;
}

bool
UseScript_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/1999
	_console->UseScript(InputBuffer);
	return true;
}

bool
NewSituation_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 1/9/2004
	NoMoreVarsForSituation = false;
	SituationTimeLimit = 0;
	if (NULL!=Situation)
		{
		INFORM("Forgetting old situation.");
		DELETE_AND_NULL(Situation);
		}

	// AbstractClass helper cleanup
	DELETE_AND_NULL(Z_0);

	INFORM("New situation created.");
	return true;
}

void
ConstraintMerge(MetaConnective* Situation,MetaConcept* NewConstraint,MetaConcept*& Result)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/2005
	// Add the constraint to the AND-object
	if (!Situation->AddArgAtEndAndForceCorrectForm(NewConstraint))
		UnconditionalRAMFailure();

	Result = Situation;
}

void
ConstraintMerge(MetaConcept* Situation,MetaConcept* NewConstraint,MetaConcept*& Result)
{	// FORMALLY CORRECT, 11/15/1999
	// create AND object holding both the Situation and the constraint
	assert(Situation);
	assert(NewConstraint);
	MetaConcept** NewArgList = _new_buffer_nonNULL<MetaConcept*>(2);

	NewArgList[0]=Situation;
	NewArgList[1]=NewConstraint;
	Result = new(nothrow) MetaConnective(NewArgList,AND_MCM);
	if (!Result)
		{
		delete NewArgList[0];
		delete NewArgList[1];
		FREE(NewArgList);
		UnconditionalRAMFailure();
		};
}

void
ConstraintMerge(QuantifiedStatement* Situation,MetaConcept* NewConstraint,MetaConcept*& Result)
{	// FORMALLY CORRECT: 9/28/1999
	// add the constraint to the Arg(0) object
	MetaConcept* Result2 = NULL;
	Situation->TransferOutAndNULL(0,Result2);
	if (Result2->IsExactType(LogicalAND_MC))
		ConstraintMerge(static_cast<MetaConnective*>(Result2),NewConstraint,Result2);
	else
		ConstraintMerge(Result2,NewConstraint,Result2);
	Situation->TransferInAndOverwriteRaw(0,Result2);
	Result = Situation;
}

static void ConstraintMerge(QuantifiedStatement* Situation,QuantifiedStatement* NewConstraint,MetaConcept*& Result)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/29/1999
	// add the constraint to the Arg(0) object; make sure improvised variables are
	// transferred.  Note that NewConstraint may simply be a quantified-statement without
	// improvised variables.
	assert(2<=NewConstraint->size());
	if (static_cast<MetaQuantifier*>(NewConstraint->ArgN(1))->IsImprovisedVar())
		{	// we need to shift improvised variables
		size_t VarCount = NewConstraint->size()-1;
		if (!Situation->InsertNSlotsAtV2(VarCount,1))
			UnconditionalRAMFailure();
		do	{
			MetaConcept* Tmp = NULL;
			NewConstraint->TransferOutAndNULL(VarCount,Tmp);
			Situation->TransferInAndOverwriteRaw(VarCount,Tmp);
			}
		while(0<--VarCount);
		MetaConcept* OldArgZero = NULL;
		NewConstraint->TransferOutAndNULL(0,OldArgZero);
		delete NewConstraint;
		ConstraintMerge(Situation,OldArgZero,Result);
		}
	else	// we don't need to shift anything, RHS is an atomic statement
		ConstraintMerge(Situation,static_cast<MetaConcept*>(NewConstraint),Result);
}

void
ConstraintForSituationAux1(MetaConcept* InitialResult)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/16/2000
	if		(NULL==Situation)
		{
		Situation = InitialResult;
		return;
		}
	else if (Situation->IsExactType(QuantifiedStatement_MC))
		{
		if		(InitialResult->IsExactType(QuantifiedStatement_MC))
			{
			ConstraintMerge(static_cast<QuantifiedStatement*>(Situation),static_cast<QuantifiedStatement*>(InitialResult),Situation);
			return;
			}
		else{
			ConstraintMerge(static_cast<QuantifiedStatement*>(Situation),InitialResult,Situation);
			return;
			};
		}
	else if (Situation->IsExactType(LogicalAND_MC))
		{
		if		(InitialResult->IsExactType(QuantifiedStatement_MC))
			{
			ConstraintMerge(static_cast<QuantifiedStatement*>(InitialResult),static_cast<MetaConnective*>(Situation),Situation);
			return;
			}
		else{
			ConstraintMerge(static_cast<MetaConnective*>(Situation),InitialResult,Situation);
			return;
			};
		}
	else{
		assert(Situation->IsUltimateType(&TruthValues));
		if		(InitialResult->IsExactType(QuantifiedStatement_MC))
			{
			ConstraintMerge(static_cast<QuantifiedStatement*>(InitialResult),Situation,Situation);
			return;
			}
		else if (InitialResult->IsExactType(LogicalAND_MC))
			{
			ConstraintMerge(static_cast<MetaConnective*>(InitialResult),Situation,Situation);
			return;
			}
		else{
			ConstraintMerge(Situation,InitialResult,Situation);
			return;
			};
		};
}

void
ConstraintForSituationAux2(autoarray_ptr<MetaConcept*>& ArgArray, MetaConcept*& InitialResult)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/19/2006
	try	{
		autodel_ptr<QuantifiedStatement> Tmp;
		Tmp = new QuantifiedStatement;
		Tmp->insertNSlotsAt(NewVarsOnThisPass.ArraySize()+1,0);
		InitialResult = ArgArray[0];
		ArgArray[0] = NULL;
		ArgArray.clear();
		Tmp->TransferInAndOverwriteRaw(0,InitialResult);
		{
		size_t Idx = NewVarsOnThisPass.ArraySize();
		do	{
			Tmp->TransferInAndOverwriteRaw(Idx,NewVarsOnThisPass[Idx-1]);
			NewVarsOnThisPass[--Idx] = NULL;
			}
		while(0<Idx);
		}
		InitialResult = Tmp;
		delete [] (MetaQuantifier**)(NewVarsOnThisPass);
		NewVarsOnThisPass.NULLPtr();
		}
	catch(const bad_alloc&)
		{	// clean out new variables
		NewVarsOnThisPass.clear();
		ArgArray.clear();
		UnconditionalRAMFailure();
		};
}

bool
ConstraintForSituation_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/19/2006
	autoarray_ptr<MetaConcept*> ArgArray(1);
	if (!InitMetaConceptParserArray(ArgArray,InputBuffer)) return false;

	NewVarsOnThisPass.clear();
	try	{
		while(InterpretOneStage(ArgArray));
		}
	catch(const syntax_error& e)
		{
		_console->SaysError(e.what());
		LogThis(ArgArray);
		return true;
		};
	if (1==ArgArray.size())
		{
		if (!ArgArray[0]->SyntaxOK())
			{
			Franci_SyntaxError();
			LogThis(ArgArray);
			return true;
			};
		// success.  Extract the lone argument and process it.
		if (!ArgArray[0]->IsUltimateType(&TruthValues))
			{
			WARNING("Constraints must be interpretable as true or false.");
			WARNING("Ignoring attempted constraint that is not interpretable as true or false.");
			INFORM(*ArgArray[0]);
			return true;
			}

		MetaConcept* InitialResult = NULL;
        if (NULL==NewVarsOnThisPass)
			{
			InitialResult = ArgArray[0];
			ArgArray[0] = NULL;
			ArgArray.clear();
			}
		else if (NoMoreVarsForSituation)
			{
			WARNING("But you said no more variables could be improvised for this situation.");
			WARNING("Ignoring statement that improvises new variables.");
			NewVarsOnThisPass.clear();
			return true;
			}
		else
			ConstraintForSituationAux2(ArgArray,InitialResult);
		if (!DoNotExplain)
			{
			INFORM("Accepting constraint:");
			INFORM(*InitialResult);
			}
		ConstraintForSituationAux1(InitialResult);
		return true;
		};
	Franci_IncompleteUnderstanding();
	LogThis(ArgArray);
	return true;
}

static bool NewVarsForSituation_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/19/2006
	if (NoMoreVarsForSituation)
		INFORM("But we already agreed that there would be no more variables.\n");
	else{
		autoarray_ptr<MetaConcept*> ArgArray(1);
		if (!InitMetaConceptParserArray(ArgArray,InputBuffer)) return false;

		NewVarsOnThisPass.clear();
		try	{
			while(InterpretOneStage(ArgArray));
			}
		catch(const syntax_error& e)
			{
			_console->SaysError(e.what());
			LogThis(ArgArray);
			return true;
			};
		if (!DoNotExplain) LogThis(ArgArray);
		{
		size_t i = ArgArray.ArraySize();
		do	{
			--i;
			if (typeid(MetaQuantifier)!=typeid(*ArgArray[i]))	// typeid macro-like in GCC 4.4+
				{
				Franci_SyntaxError();
				return true;
				}
			}
		while(0<i);
		// This is a list of globally quantified variables.  Move it in.
		i = ArgArray.ArraySize();
		const bool BootstrapQuantifiedSituation = NULL==Situation || !Situation->IsExactType(QuantifiedStatement_MC);
		const size_t OriginalArity = (BootstrapQuantifiedSituation) ? 1 : Situation->size();
		QuantifiedStatement* Tmp2 = static_cast<QuantifiedStatement*>(Situation);
		try	{
			if		(BootstrapQuantifiedSituation)
				{
				Tmp2 = new QuantifiedStatement();
				Tmp2->insertNSlotsAt(i+1,0);
				Tmp2->TransferInAndOverwriteRaw(0,(NULL==Situation) ? new TruthValue(true) : Situation);
				}
			else	// need to add vars to QuantifiedStatement.
				Tmp2->insertNSlotsAt(i,OriginalArity);
			}
		catch(const bad_alloc&)
			{
			ArgArray.clear();
			UnconditionalRAMFailure();
			}
		do	{
			--i;
			Tmp2->TransferInAndOverwriteRaw(OriginalArity+i,ArgArray[i]);
			ArgArray[i] = NULL;
			}
		while(0<i);
		if (BootstrapQuantifiedSituation) Situation = Tmp2;
		delete [] (MetaQuantifier**)NewVarsOnThisPass;
		NewVarsOnThisPass.NULLPtr();
		}
		if (!DoNotExplain) LOG("New variables recorded.");
		}
	return true;
}

bool
NoMoreVarsForSituation_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/4/1999
	NoMoreVarsForSituation = true;
	INFORM((NoCurrentSituation()) ? "OK; there is no current situation now." : "OK.");
	return true;
}

bool
DefineVarOrRelation_handler(char*& InputBuffer)
{
	autoarray_ptr<MetaConcept*> ArgArray(1);
	if (!InitMetaConceptParserArray(ArgArray,InputBuffer)) return false;

	NewVarsOnThisPass.clear();
	try	{
		while(InterpretOneStage(ArgArray));
		}
	catch(const syntax_error& e)
		{
		_console->SaysError(e.what());
		LogThis(ArgArray);
		return true;
		};
	LogThis(ArgArray);
	if (1!=ArgArray.size())
		{
		Franci_IncompleteUnderstanding();
		return true;
		};
	if (!ArgArray[0]->SyntaxOK())
		{
		Franci_SyntaxError();
		return true;
		};
	//! \todo IMPLEMENT: Extract the lone argument and process it.
	INFORM("I'm sorry, but I haven't been taught that yet.");
	return true;
}

bool
SituationHasTimeLimit_handler(char*& InputBuffer)
{	// Franci must diagnose seconds/minutes/hours, and handle negative or zero correctly.
	// Franci must also impose plausibility checks.
	autoarray_ptr<MetaConcept*> ArgArray(1);
	if (!InitMetaConceptParserArray(ArgArray,InputBuffer)) return false;

	NewVarsOnThisPass.clear();
	try	{
		while(InterpretOneStage(ArgArray));
		}
	catch(const syntax_error& e)
		{
		_console->SaysError(e.what());
		LogThis(ArgArray);
		return true;
		};
	if (1!=ArgArray.size())
		{
		Franci_IncompleteUnderstanding();
		return true;
		};
	if (!ArgArray[0]->SyntaxOK())
		{
		Franci_SyntaxError();
		return true;
		};
	//! \todo IMPLEMENT: Extract the lone argument and process it.
	INFORM("I'm sorry, but I haven't been taught that yet.");
	return true;
}

bool
EvaluateSituation_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/22/1999
	if (NULL!=Situation)
		{
		INFORM("The current situation is:");
		INFORM(*Situation);
		INFORM("");
		const clock_t TimeNULL = clock();
		while(TimeNULL==(EvalTime0=clock()));
		if (DoNotExplain) suspend_logging();
		while(OneStageAnalyzeSituation(Situation,EvalTime0));
		}
	else
		INFORM("But there is no information to analyze.");
	return true;
}

bool
EvaluateExpression_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/19/2006
	autoarray_ptr<MetaConcept*> ArgArray(1);
	if (!InitMetaConceptParserArray(ArgArray,InputBuffer)) return false;

	NewVarsOnThisPass.clear();
	try	{
		while(InterpretOneStage(ArgArray));
		}
	catch(const syntax_error& e)
		{
		_console->SaysError(e.what());
		LogThis(ArgArray);
		return true;
		};
	if (1!=ArgArray.size())
		{
		Franci_IncompleteUnderstanding();
		LogThis(ArgArray);
		// clean out new variables
		NewVarsOnThisPass.clear();
		return true;
		};
	// NOTE: variable resolution is handled in ...SyntaxOK()
	if (!ArgArray[0]->SyntaxOK())
		{	//! \todo extract to syntax-error reporting routine.  Needs work to avoid
			//! monotony.  Also needs to document errors
		Franci_SyntaxError();
		LogThis(ArgArray);
		// clean out new variables
		NewVarsOnThisPass.clear();
		return true;
		};
	// success.  Extract the lone argument and process it.
	MetaConcept* InitialResult = NULL;
			
	if (NULL==NewVarsOnThisPass)
		{
		InitialResult = ArgArray[0];
		ArgArray[0] = NULL;
		ArgArray.clear();
		}
	else{
		try	{
			autodel_ptr<QuantifiedStatement> Tmp;
			Tmp = new QuantifiedStatement;
			Tmp->insertNSlotsAt(NewVarsOnThisPass.ArraySize()+1,0);
			InitialResult = ArgArray[0];
			ArgArray[0] = NULL;
			ArgArray.clear();
			Tmp->TransferInAndOverwriteRaw(0,InitialResult);
			{
			size_t Idx = NewVarsOnThisPass.ArraySize();
			do	{
				Tmp->TransferInAndOverwriteRaw(Idx,NewVarsOnThisPass[Idx-1]);
				NewVarsOnThisPass[--Idx] = NULL;
				}
			while(0<Idx);
			}
			InitialResult = Tmp.release();
			}
		catch(const bad_alloc&)
			{	// clean out new variables
			NewVarsOnThisPass.clear();
			ArgArray.clear();
			UnconditionalRAMFailure();
			};
		};

	// clean out new variables
	NewVarsOnThisPass.clear();

	INFORM(*InitialResult);
	INFORM("evaluates to:");
	{
	const clock_t ClockNULL = clock();
	clock_t Clock0;
	clock_t Clock1;
		
	do	Clock0 = clock();
	while(ClockNULL==Clock0);

	if (DoNotExplain) suspend_logging();
	DestructiveSyntacticallyEvaluate(InitialResult);
	_console->ResumeLogFile();

	Clock1 = clock();
	INFORM(*InitialResult);
	ReportTime(Clock0,Clock1);
	}
	return true;
}

bool
WhatIf_handler(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	//! \todo this handler should be blocked by a currently evaluating situation
	autoarray_ptr<MetaConcept*> ArgArray(1);
	if (!InitMetaConceptParserArray(ArgArray,InputBuffer)) return false;

	NewVarsOnThisPass.clear();
	try	{
		while(InterpretOneStage(ArgArray));
		}
	catch(const syntax_error& e)
		{
		_console->SaysError(e.what());
		LogThis(ArgArray);
		return true;
		};
	if (1!=ArgArray.size())
		{
		Franci_IncompleteUnderstanding();
		LogThis(ArgArray);
		// clean out new variables
		NewVarsOnThisPass.clear();
		return true;
		};
	// NOTE: variable resolution is handled in ...SyntaxOK()
	if (!ArgArray[0]->SyntaxOK())
		{	//! \todo extract to syntax-error reporting routine.  Needs work to avoid
			//! monotony.  Also needs to document errors
		Franci_SyntaxError();
		LogThis(ArgArray);
		// clean out new variables
		NewVarsOnThisPass.clear();
		return true;
		};

	// success.  Extract the lone argument and process it.
	if (NULL!=NewVarsOnThisPass)
		{
		WARNING("I cannot improvise variables for a 'what if' situation.");
		WARNING("Ignoring flawed 'what if' situation.");
		NewVarsOnThisPass.clear();
		return true;
		};

	MetaConcept* InitialResult = ArgArray[0];
	ArgArray[0] = NULL;
	ArgArray.clear();
	if (!InitialResult->IsUltimateType(&TruthValues))
		{
		WARNING("The 'what if' constraint must be interpretable as true or false.");
		WARNING("Ignoring flawed 'what if' situation.");
		delete InitialResult;
		return true;
		};
	INFORM("Accepting 'what if'");
	INFORM(*InitialResult);
	INFORM("");

	// We need to create a 'virtual situation that is the AND of the new constraint
	// and the original.  We then evaluate this instead of the current situation.
	{
	MetaConcept* TmpSituation = Situation;
	Situation = NULL;
	try	{
		TmpSituation->CopyInto(Situation);
		if (Situation->IsExactType(QuantifiedStatement_MC))
			{
			ConstraintMerge(static_cast<QuantifiedStatement*>(Situation),InitialResult,Situation);
			static_cast<QuantifiedStatement*>(Situation)->ResetExplicitSort();
			}
		else if (Situation->IsExactType(LogicalAND_MC))
			ConstraintMerge(static_cast<MetaConnective*>(Situation),InitialResult,Situation);
		else{
			assert(Situation->IsUltimateType(&TruthValues));
			if (InitialResult->IsExactType(LogicalAND_MC))
				ConstraintMerge(static_cast<MetaConnective*>(InitialResult),Situation,Situation);
			else
				ConstraintMerge(Situation,InitialResult,Situation);
			};
		}
	catch(const bad_alloc&)
		{
		DELETE_AND_NULL(Situation);
		DELETE(InitialResult);
		Situation = TmpSituation;
		SEVERE_WARNING("It appears there isn't any scratch paper for that request at this time.");
		return true;
		}

	INFORM("The current conditional situation is:");
	INFORM(*Situation);
	INFORM("");
	{
	const clock_t TimeNULL = clock();
	while(TimeNULL==(EvalTime0=clock()));
	}
	MetaConcept** PlausibleVarList = NULL;	// Variables known to work already
Restart:
	if (DoNotExplain) suspend_logging();
	while(OneStageAnalyzeSituation(Situation,EvalTime0));
	if (   Situation->IsExactType(QuantifiedStatement_MC)
		&& static_cast<QuantifiedStatement*>(Situation)->Explore(EvalTime0,DoNotExplain,PlausibleVarList))
		{
		INFORM("The improved conditional situation is:");
		INFORM(*Situation);
		INFORM("");
		goto Restart;
		};
	if (NULL!=PlausibleVarList)
		BLOCKDELETEARRAY(PlausibleVarList);
	delete Situation;
	Situation = TmpSituation;
	}
	return true;
}

// C stdio gives: remove, rename, tmpfile, tmpnam [but tmpfile and tmpnam are broken on Vista]
// C stdlib gives: system, getenv
// io.h gives: ... (look at POSIX spec, may want more wrappers)
bool
System_handler(char*& InputBuffer)
{
	if (NULL!=InputBuffer)
		{
		int Code = system(InputBuffer);
		}
	else
		INFORM("There is no command to hand off to the operating system.");
	return true;
}

#if 0
bool
LoadFunctionFile_handler(char*& InputBuffer)
{
	return ExternalFunction::LoadFunctionsFromFile(InputBuffer);
}

bool
RemoveFunction_handler(char*& InputBuffer)
{
	return ExternalFunction::UnloadFunction(InputBuffer);
}

bool
RemoveAllFunctions_handler(char*& InputBuffer)
{
	return ExternalFunction::UnloadAllFunctions();
}
#endif

bool
FranciDoesNotUnderstand(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/26/2005
	if (InputBuffer!=NULL)
		WARNING("I don't recognize that command at all.");
	return true;
}

// Array support for 'quick' parsing
// only need printable characters, and not space at that: downshift by ' '+1
// need go no higher that '~'

	// FRANCI SCRIPTING LANGUAGE
	// Stage I implementation: 
	//		*	start logfile		// DONE
	//		*   end logfile			// DONE
	//		*	use script _____	// DONE
	//		======
	// Stage II implementation: 
	//		*	new situation		// DONE
	//		*	no more variables	// DONE
	//		*	constraint: ____	// DONE
	//		*	evaluate situation	// DONE
	//		*	evaluate _____		// DONE
	//		======
	//		*	what if ____		// DONE
	//		*	new variable: ____ [IN [domain]] // DONE
	//			Franci can do automatic type inferences	// DONE
	//		======
	//		*	_____ [ELIZA MODE]
	//		======
	//		*	0-ary Keywords: TRUE, FALSE, UNKNOWN, CONTRADICTION [TruthValue]
	//		*	n-ary prefix Keywords: AND(), OR(), NAND(), NOR() IFF(), XOR(), NIFF(), NXOR() [MetaConnective]
	//		*	Infix Keywords: ___ AND ___, ___ OR ___, ___ IFF ___ [MetaConnective]
	//		*   Integers
	//		*	Keywords: ___ IMPLIES ___, NOT ___, ____ IF AND ONLY IF ____
	//			[define in terms of software operations]
	// Stage III implementation
	//		*	n-ary prefix or quasi-prefix: (NOT)ALLEQUAL, (NOT)ALLDISTINCT, DISTINCTFROMALLOF, EQUALTOONEOF 
	//! \todo	*	time limit: ___ [seconds|minutes|hours]
	//! \todo	*	___:=_____  [defines both variables and relations]
	//! \todo	*	We must allow general ID (0-ary or n-ary) followed by :=
	//! \todo: system ____, possibly other OS-ish commands [note: have System_handler if-0'd out]
	//! \todo: function handling: load functionfile ___, remove function __, remove all functions, (have handlers for these in ExternalFunction)

void
RemoveLeadingTrailingWhiteSpace(char*& InputBuffer)
{
	size_t Idx = ArraySize(InputBuffer);
	size_t Idx2 = 0;
	while(isspace(InputBuffer[--Idx]))
		if (0==Idx)
			{
			DELETEARRAY_AND_NULL(InputBuffer);
			return;	// Franci: I don't have to react to pure whitespace
			};
	while(isspace(InputBuffer[Idx2])) Idx2++;
	memmove(InputBuffer,InputBuffer+Idx2,Idx-Idx2+1);
	InputBuffer = REALLOC(InputBuffer,ZAIMONI_LEN_WITH_NULL(Idx-Idx2+1));
	ZAIMONI_NULL_TERMINATE(InputBuffer[Idx-Idx2+1]);
}

bool
NotInQuotation_DetectSpuriousWhiteSpace(const size_t Idx, const size_t InputBufferLength, const char* InputBuffer)
{
	if (' '==InputBuffer[Idx])
		{
		if (strchr("([{",InputBuffer[Idx-1])) return true;
		// ), ], } end pairs
		// ?, ! end sentence without altering semantics
		if (strchr(")]}?!",InputBuffer[Idx+1])) return true;
		switch(InputBuffer[Idx+1])
		{
		case '.':	// ends sentence; no problem unless immediately-prior is a digit
			if ('0'<=InputBuffer[Idx-1] && '9'>=InputBuffer[Idx-1])
				return false;
			return true;
		case ' ':	// return true iff won't end sentence
			return strchr(".?!\"'",InputBuffer[Idx-1]);
		}
		}
	return false;
}

void
RemoveSpuriousWhiteSpace(char*& InputBuffer)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2005
	// Whitespace is meaningful when:
	// #1) it is inside of a string/char literal [but not a quoted sentence!]
	// #2) its removal alters the preprocessing literal/numeral lists
	// #3) its removal could alter english syntax
	// Magic constant 3 is the minimum length for a string to contain spurious whitespace
#ifdef FORCE_ISO
	const size_t buf_len = strlen(InputBuffer);
#else
	const size_t buf_len = _msize(InputBuffer);
#endif
	if (3<=buf_len)
		{
		size_t VirtualBufferSize = buf_len;
		size_t HighBoundQuotes = VirtualBufferSize;
		size_t Idx = VirtualBufferSize;
		size_t LowBoundQuotes = VirtualBufferSize+1;

		// establish true high bound
		do	if (strchr("'\"",InputBuffer[--Idx]))
				{
				HighBoundQuotes = Idx;
				break;
				}
		while(0<Idx);
		if (HighBoundQuotes<VirtualBufferSize)
			{	// establish true low bound
			LowBoundQuotes = HighBoundQuotes;
			if (0<LowBoundQuotes)
				{
				Idx = 0;
				do	if (strchr("'\"",InputBuffer[Idx]))
						{
						LowBoundQuotes = Idx;
						break;
						}
				while(LowBoundQuotes>++Idx);
				}
			}

		Idx = buf_len-2;	// do screening above high-bound
		if (LowBoundQuotes<=HighBoundQuotes)
			{	// Quotation marks: exclude these, there may be literals
			while(HighBoundQuotes<Idx && 3<=VirtualBufferSize)
				if (NotInQuotation_DetectSpuriousWhiteSpace(Idx,VirtualBufferSize,InputBuffer))
					{
					memmove(InputBuffer+Idx,InputBuffer+Idx+1,VirtualBufferSize-Idx);
					VirtualBufferSize--;
					if (1>=VirtualBufferSize-Idx)
						Idx = VirtualBufferSize-2;
					}
				else
					Idx--;

			if (1>=LowBoundQuotes || 3>VirtualBufferSize)
				goto RAMCleanExit;

			// do screening below low-bound
			Idx = LowBoundQuotes-1;
			}

		// do screening below low-bound, if any
		while(0<Idx && 3<=VirtualBufferSize)
			if (NotInQuotation_DetectSpuriousWhiteSpace(Idx,VirtualBufferSize,InputBuffer))
				{
				memmove(InputBuffer+Idx,InputBuffer+Idx+1,VirtualBufferSize-Idx);
				VirtualBufferSize--;
				if (1>=VirtualBufferSize-Idx)
					Idx = VirtualBufferSize-2;
				}
			else
				Idx--;

RAMCleanExit:
		if (VirtualBufferSize<buf_len)
			{
			InputBuffer = REALLOC(InputBuffer,ZAIMONI_LEN_WITH_NULL(VirtualBufferSize));
			ZAIMONI_NULL_TERMINATE(InputBuffer[VirtualBufferSize]);
			}
		};
}

void
ConvertHTMLEntityCharsToASCIIChars(char*& InputBuffer)
{	//! \todo IMPLEMENT(?)
	// Franci parses HTML entity-like things (&(quasisymbol);, &#(quasisymbol);) in UnparsedText.
	// This routine covers "reflex" parses that should not be caught then, for various reasons.

	// Franci converts most recognizable HTML 4.0 entity-chars to ASCII-chars at this time.
	// EXCEPTIONS: &lt; &gt; &amp; (these may be needed later to prevent ambiguity concerns)
	// COVERED: quot, nbsp, iexcl, cent, pound, curren, yen, brvbar, sect, uml, copy,
	// ordf, laquo, not, shy, reg, macr, deg, plusmn, sup2, sup3, acute, micro, para, middot,
	// cedil, sup1, ordm, raquo, frac14, frac12, frac34, iquest, Agrave, Aacute, Acirc,
	// Atilde, Auml, Aring, AElig, Ccedil, Egrave, Eacute, Ecirc, Euml, Igrave, Iacute,
	// Icirc, Iuml, ETH, Ntilde, Ograve, Oacute, Ocirc, Otilde, Ouml, times, Oslash,
	// Ugrave, Uacute, Ucirc, Uuml, Yacute, THORN, szlig, agrave. aacute, acirc, atilde,
	// auml, aring, aelig, ccedil, egrave, eacute, ecirc, euml, igrave, iacute,
	// icirc, iuml, eth, ntilde, ograve, oacute, ocirc, otilde, ouml, divide, oslash,
	// ugrave, ucirc, uuml, yacute, thorn, yuml
	// this could be viewed as quot, then an array indexed from 160 to 255 (decimal) ASCII
	// [but there may be a more clever methodology]
	//! \todo CRITICAL IMPLEMENTATION: &middot; etc. (this is the multiplication symbol!)
	//! must support these variants: &middot; &#183; <sup>.</sup>
	//! map all of these to '\xb7'
	//! \todo escaped HTML chars:
}

void
CmdShell_StdOut(const char* Text)
{
	INFORM((NULL!=Text) ? Text : "\n");
}

void
Franci_precmd(char*& InputBuffer)
{
	if (NULL!=InputBuffer)
		{
		// This is responsible for null-terminating InputBuffer
		// #1: remove leading/trailing ' '
		RemoveLeadingTrailingWhiteSpace(InputBuffer);
		if (NULL==InputBuffer) return;

		// kill C++ comments at start of line
		if (   '/'==InputBuffer[0]
			&& '/'==InputBuffer[1])
			{
			DELETEARRAY_AND_NULL(InputBuffer);
			return;
			}
		_console->LogUserInput(InputBuffer);

		// #3: remove spurious whitespace
		RemoveSpuriousWhiteSpace(InputBuffer);
		if (NULL==InputBuffer) return;
		// #4: HTML entity crunch
		ConvertHTMLEntityCharsToASCIIChars(InputBuffer);
//		default: verbose
		DoNotExplain = false;
//		should be case_insensitive on commands...get some help from CmdShell to know when to selectively lowercase
//		if (NULL==InputBuffer)
//			return;
		}
}

//! \todo determine whether condensing the following routines' common code saves executable size.  If so, do it.
static bool complete_constraint(char*& InputBuffer)
{
	if ('C'==InputBuffer[0])
		{
		InputBuffer[0] = 'c';
		if (!strncmp(CONSTRAINT,InputBuffer,sizeof(CONSTRAINT)-1))
			{
			if ('\0'==InputBuffer[sizeof(CONSTRAINT)-1] || ' '==InputBuffer[sizeof(CONSTRAINT)-1])
				{
				DoNotExplain = true;
				return true;
				}
			}
		InputBuffer[0] = 'C';
		}
	return false;
}

static bool complete_evaluate(char*& InputBuffer)
{
	if ('E'==InputBuffer[0])
		{
		InputBuffer[0] = 'e';
		if (!strncmp(EVALUATE,InputBuffer,sizeof(EVALUATE)-1))
			{
			if ('\0'==InputBuffer[sizeof(EVALUATE)-1] || ' '==InputBuffer[sizeof(EVALUATE)-1])
				{
				DoNotExplain = true;
				return true;
				}
			}
		InputBuffer[0] = 'E';
		}
	return false;
}

static bool complete_evaluate_situation(char*& InputBuffer)
{
	if ('E'==InputBuffer[0])
		{
		InputBuffer[0] = 'e';
		if (!strncmp(EVALUATE_SITUATION,InputBuffer,sizeof(EVALUATE_SITUATION)-1))
			{
			if ('\0'==InputBuffer[sizeof(EVALUATE_SITUATION)-1] || ' '==InputBuffer[sizeof(EVALUATE_SITUATION)-1])
				{
				DoNotExplain = true;
				return true;
				}
			}
		InputBuffer[0] = 'E';
		}
	return false;
}

static bool complete_new_variable(char*& InputBuffer)
{
	if ('N'==InputBuffer[0])
		{
		InputBuffer[0] = 'n';
		if (!strncmp(NEW_VARIABLE,InputBuffer,sizeof(NEW_VARIABLE)-1))
			{
			if ('\0'==InputBuffer[sizeof(NEW_VARIABLE)-1] || ' '==InputBuffer[sizeof(NEW_VARIABLE)-1])
				{
				DoNotExplain = true;
				return true;
				}
			}
		InputBuffer[0] = 'N';
		}
	return false;
}

static bool complete_what_if(char*& InputBuffer)
{
	if ('W'==InputBuffer[0])
		{
		InputBuffer[0] = 'w';
		if (!strncmp(WHAT_IF,InputBuffer,sizeof(WHAT_IF)-1))
			{
			if ('\0'==InputBuffer[sizeof(WHAT_IS)-1] || ' '==InputBuffer[sizeof(WHAT_IS)-1])
				{
				DoNotExplain = true;
				return true;
				}
			}
		InputBuffer[0] = 'W';
		}
	return false;
}

static bool complete_what_is(char*& InputBuffer)
{
	if ('W'==InputBuffer[0])
		{
		InputBuffer[0] = 'w';
		if (!strncmp(WHAT_IS,InputBuffer,sizeof(WHAT_IS)-1))
			{
			if ('\0'==InputBuffer[sizeof(WHAT_IF)-1] || ' '==InputBuffer[sizeof(WHAT_IS)-1])
				{
				DoNotExplain = true;
				return true;
				}
			}
		InputBuffer[0] = 'W';
		}
	return false;
}

// this goes in main.cxx's OSIndependentInitialize
void
InitializeFranciInterpreter(void)
{
	FranciScript.doc_header = "Commands";
	FranciScript.own_doc_header = false;
	FranciScript.misc_header = "Language constructs, etc.";
	FranciScript.own_misc_header = false;
	FranciScript.undoc_header = "Undocumented commands";
	FranciScript.own_undoc_header = false;

//LexParse.cxx: In function 'void InitializeFranciInterpreter()':
//LexParse.cxx:1474: warning: deprecated conversion from string constant to 'char*'
//LexParse.cxx:1476: warning: deprecated conversion from string constant to 'char*'
//LexParse.cxx:1478: warning: deprecated conversion from string constant to 'char*'

// typedef void CmdShellIOHook(const char* Text);
// need wrapper for StdOutHook
	FranciScript.StdOutHook = &CmdShell_StdOut;
	FranciScript.DefaultHook = &FranciDoesNotUnderstand;	// really should be no-fail; change it back once CmdShell implementation is working
	FranciScript.PrecmdHook = &Franci_precmd;
	FranciScript.GetLineHookNoFail = &GetLineFromKeyboardHook;

	FranciScript.AddCommandHandler("clean logfile",&CleanLogfile_handler,NULL);
	FranciScript.AddCommandHandler(CONSTRAINT,&ConstraintForSituation_handler,&complete_constraint);	// command_complete handles: Constraint: for terse
	FranciScript.AddCommandHandler("end logfile",&EndLogfile_handler,NULL);
	FranciScript.AddCommandHandler(EVALUATE,&EvaluateExpression_handler,&complete_evaluate);	// command_complete handles: capitalize for terse
	FranciScript.AddCommandHandler(EVALUATE_SITUATION,&EvaluateSituation_handler,&complete_evaluate_situation);	// command_complete handles: capitalize for terse
	FranciScript.AddCommandHandler("new situation",&NewSituation_handler,NULL);
	FranciScript.AddCommandHandler(NEW_VARIABLE,&NewVarsForSituation_handler,&complete_new_variable);	// command_complete handles: capitalize for terse
	FranciScript.AddCommandHandler("no more variables",&NoMoreVarsForSituation_handler,NULL);
	FranciScript.AddCommandHandler("start logfile",&StartLogfile_handler,NULL);
//	FranciScript.AddCommandHandler("rm",&StartLogfile_handler,NULL);
//	FranciScript.AddCommandHandler("del",&StartLogfile_handler,NULL);
	if (system(NULL)) FranciScript.AddCommandHandler("system",&System_handler,NULL);			// only allow the system hook if it can be found
	FranciScript.AddCommandHandler("time limit",&SituationHasTimeLimit_handler,NULL);
	FranciScript.AddCommandHandler("use script",&UseScript_handler,NULL);
	FranciScript.AddCommandHandler(WHAT_IF,&WhatIf_handler,&complete_what_if);				// command_complete handles: capitalize for terse
	FranciScript.AddCommandHandler(WHAT_IS,&EvaluateExpression_handler,&complete_what_is);	// command_complete handles: capitalize for terse
}

