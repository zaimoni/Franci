// ConEqual.cxx
// implementation for inference rules that use both MetaConnective and EqualRelation

#include "Equal.hxx"
#include "MetaCon3.hxx"
#include "Class.hxx"
#include "Interval.hxx"

bool EqualRelation::NAryEQUALSpawn2AryEQUAL(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/04/2004
	assert(!dest);
	assert(IsExactType(ALLEQUAL_MC));
	// Target: InferenceParameter1
	// Sum with arg in InferenceParameter1 : InferenceParameter2
	// This is being called for an n-ary equivalence relation that needs to split into an 
	// AND of equivalence relations: n-1 ary (with Sum removed), and 2-ary (Target and Sum)
	// throwing std::bad_alloc ock, this is called from Eval....
	//! \todo: reimplement to do this en masse [maybe not!  generic code can be hooked from 
	//! more than one operation: this simply sets up the 2-ary version]
	autoval_ptr<EqualRelation> TmpEQUALTarget2Ary;
	TmpEQUALTarget2Ary = new EqualRelation(ALLEQUAL_EM);
	TmpEQUALTarget2Ary->insertNSlotsAt(2,0);
	autoval_ptr<MetaConcept> TmpEQUALTargetNMinus1Ary;
	TmpEQUALTargetNMinus1Ary = new EqualRelation(ALLEQUAL_EM);
	autoval_ptr<MetaConnective> TmpANDTarget;
	TmpANDTarget = new MetaConnective(AND_MCM);
	TmpANDTarget->insertNSlotsAt(2,0);
	ArgArray[InferenceParameter1]->CopyInto(TmpEQUALTarget2Ary->ArgArray[0]);

	// RAM-safe now
	TransferOutAndNULL(InferenceParameter2,TmpEQUALTarget2Ary->ArgArray[1]);
	DeleteIdx(InferenceParameter2);
	TmpEQUALTarget2Ary->ForceCheckForEvaluation();
	TmpANDTarget->TransferInAndOverwrite(0,TmpEQUALTarget2Ary);
	IdxCurrentEvalRule = None_ER;	
	MoveInto(TmpEQUALTargetNMinus1Ary);
	TmpANDTarget->TransferInAndOverwrite(1,TmpEQUALTargetNMinus1Ary);
	TmpANDTarget->ForceCheckForEvaluation();
	dest = TmpANDTarget.release();
	assert(dest->SyntaxOK());
	return true;
}

void MetaConceptWithArgArray::Init2AryEqualForZeroEq2ArySum(MetaConcept*& dest)	// can throw bad_alloc
{	// FORMALLY CORRECT: Kenneth Boyd, 11/4/2005
	assert(!dest);
	EqualRelation* TmpEqual2 = new EqualRelation(ALLEQUAL_EM);

	// RAM-safe now
	if (     ArgArray[0]->IsMetaAddInverted()
		|| (!ArgArray[1]->IsMetaAddInverted() && ArgArray[0]->IsExplicitConstant()))
		SUCCEED_OR_DIE(ArgArray[0]->SelfInverse(StdAddition_MC));
	else
		SUCCEED_OR_DIE(ArgArray[1]->SelfInverse(StdAddition_MC));

	ArgArray.MoveInto(TmpEqual2->ArgArray);
	TmpEqual2->ForceCheckForEvaluation();
	dest = TmpEqual2;
	assert(dest->SyntaxOK());
}

bool EqualRelation::NAryEqRewriteZeroEq2ArySum(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/04/2004
	// InferenceParameter1 index the arg that tripped this; it is the highest 2-ary StdAddition
	// that is viable.
	assert(!dest);
	assert(ArgArray[0]->IsZero());
	// For efficiency reasons, Franci tries to do a mass application.  However, Franci
	// considers the rule to succeed if at least one rewrite succeeds.
	autoval_ptr<MetaConnective> NewAND;
	NewAND = new MetaConnective(AND_MCM);
	NewAND->insertNSlotsAt(2,0);
	autoval_ptr<MetaConcept> NewEqual;
	NewEqual = new EqualRelation(ALLEQUAL_EM);
	MetaConcept* tmp;
	static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->Init2AryEqualForZeroEq2ArySum(tmp);
	NewAND->TransferInAndOverwriteRaw(0,tmp);
	DeleteIdx(InferenceParameter1);
	// end function target instance 1

	// RAM-safe now
	// check for more!
	while(1<InferenceParameter1)
		if (   ArgArray[--InferenceParameter1]->IsExactType(StdAddition_MC)
			&& 2==ArgArray[InferenceParameter1]->size()
			&& ArgArray[InferenceParameter1]->NoMetaModifications())
			{
			MetaConcept* tmp = NULL;
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->Init2AryEqualForZeroEq2ArySum(tmp);
			if (!NewAND->InsertSlotAt(0,tmp))
				{
				delete tmp;
				goto SuccessExit;
				}
			DeleteIdx(InferenceParameter1);
			}
SuccessExit:
	if (2<=fast_size())
		{
		IdxCurrentEvalRule = None_ER;
		MoveInto(NewEqual);
		NewAND->TransferInAndOverwrite(NewAND->fast_size()-1,NewEqual);
		}
	else
		NewAND->FastDeleteIdx(NewAND->fast_size()-1);
	NewAND->ForceCheckForEvaluation();
	dest = NewAND.release();
	assert(dest->SyntaxOK());
	return true;
}

bool EqualRelation::ReduceIntervalForNOTALLEQUAL(MetaConcept*& dest)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/04/2004
	assert(!dest);
	// rewrite of NOTALLEQUAL(A,B,C...D)
	// to OR(ALLDISTINCT(C,D),NOTALLEQUAL(A,B,C))
	
	// InferenceParameter1 has index to LinearInterval with ultimate domain _Z_
	autoval_ptr<MetaConnective> NewOR;
	NewOR = new MetaConnective(OR_MCM);
	NewOR->insertNSlotsAt(2,0);
	autoval_ptr<EqualRelation> NewALLDISTINCT;
	NewALLDISTINCT = new EqualRelation(ALLDISTINCT_EM);
	NewALLDISTINCT->insertNSlotsAt(2,0);
	autoval_ptr<EqualRelation> NewNOTALLEQUAL;
	NewNOTALLEQUAL = new EqualRelation(NOTALLEQUAL_EM);
	autoval_ptr<MetaConcept> TempC;
	ArgArray[InferenceParameter1]->ArgN(0)->CopyInto(TempC);

	// RAM-safe now
	static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->TransferOutLHSAndNULL(NewALLDISTINCT->ArgArray[0]);
	static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->TransferOutRHSAndNULL(NewALLDISTINCT->ArgArray[1]);
	NewALLDISTINCT->ForceCheckForEvaluation();

	NewOR->TransferInAndOverwrite(0,NewALLDISTINCT);
	delete ArgArray[InferenceParameter1];
	ArgArray[InferenceParameter1]=TempC;
	ArgArray.MoveInto(NewNOTALLEQUAL->ArgArray);
	NewNOTALLEQUAL->ForceCheckForEvaluation();

	NewOR->TransferInAndOverwrite(1,NewNOTALLEQUAL);
	NewOR->ForceCheckForEvaluation();

	dest = NewOR.release();
	assert(dest->SyntaxOK());
	return true;
}

