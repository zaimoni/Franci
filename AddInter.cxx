// AddInter.cxx
// implementation of functions involving both StdAddition and LinearInterval

#include "Class.hxx"
#include "StdAdd.hxx"
#include "Interval.hxx"

bool MetaConceptWithArgArray::TranslateInterval()
{
	assert(IsExactType(StdAddition_MC));
	assert(ArgArray.size()>InferenceParameter1);
	assert(ArgArray.size()>InferenceParameter2);
	assert(ArgArray[InferenceParameter1]);
	assert(ArgArray[InferenceParameter1]->IsExactType(LinearInterval_MC));
	// hosting type: StdAddition
	// InferenceParameter1: LinearInterval
	// InferenceParameter2: Translation constant
	if (static_cast<LinearInterval*>(ArgArray[InferenceParameter1])->TranslateInterval(ArgArray[InferenceParameter2]))
		{
		DeleteIdx(InferenceParameter2);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	return false;
}

bool LinearInterval::TranslateInterval(MetaConcept*& rhs)
{
	assert(rhs);
	MetaConcept** StdAdd1ArgArray = NULL;
	MetaConcept** StdAdd2ArgArray = NULL;
	MetaConcept* RHSClone = NULL;
	StdAddition* NewAdd1 = NULL;
	StdAddition* NewAdd2 = NULL;
	const bool LHSAlreadyStdAddition = LHS_Arg1->IsExactType(StdAddition_MC);
	const bool RHSAlreadyStdAddition = RHS_Arg2->IsExactType(StdAddition_MC);
	if (!LHSAlreadyStdAddition)
		{
		StdAdd1ArgArray = _new_buffer<MetaConcept*>(2);
		if (NULL==StdAdd1ArgArray) return false;
		}
	if (!RHSAlreadyStdAddition)
		{
		StdAdd2ArgArray = _new_buffer<MetaConcept*>(2);
		if (NULL==StdAdd2ArgArray)
			{
			free(StdAdd1ArgArray);
			return false;
			}			
		}
	try	{
		rhs->CopyInto(RHSClone);
		if (LHSAlreadyStdAddition)
			{
			if (!static_cast<StdAddition*>((MetaConcept*)LHS_Arg1)->FastInsertSlotAt(LHS_Arg1->size(),NULL))
				throw bad_alloc();
			}
		else
			NewAdd1 = new StdAddition(StdAdd1ArgArray);

		if (RHSAlreadyStdAddition)
			{
			if (!static_cast<StdAddition*>((MetaConcept*)RHS_Arg2)->FastInsertSlotAt(RHS_Arg2->size(),NULL))
				throw bad_alloc();
			}
		else
			NewAdd2 = new StdAddition(StdAdd2ArgArray);
		}
	catch(const bad_alloc&)
		{
		delete NewAdd2;
		delete NewAdd1;
		delete RHSClone;
		free(StdAdd2ArgArray);
		free(StdAdd1ArgArray);
		return false;
		}
	if (LHSAlreadyStdAddition)
		static_cast<MetaConceptWithArgArray*>((MetaConcept*)LHS_Arg1)->TransferInAndOverwriteRaw(LHS_Arg1->size()-1,RHSClone);
	else{
		NewAdd1->TransferInAndOverwrite(0,LHS_Arg1);
		NewAdd1->TransferInAndOverwriteRaw(1,RHSClone);
		LHS_Arg1 = NewAdd1;
		}
	if (RHSAlreadyStdAddition)
		static_cast<MetaConceptWithArgArray*>((MetaConcept*)RHS_Arg2)->TransferInAndOverwriteRaw(RHS_Arg2->size()-1,rhs);
	else{
		NewAdd2->TransferInAndOverwrite(0,RHS_Arg2);
		NewAdd2->TransferInAndOverwriteRaw(1,rhs);
		RHS_Arg2 = NewAdd1;
		}
	rhs = NULL;
	static_cast<MetaConceptWithArgArray*>((MetaConcept*)LHS_Arg1)->ForceCheckForEvaluation();
	static_cast<MetaConceptWithArgArray*>((MetaConcept*)RHS_Arg2)->ForceCheckForEvaluation();
	assert(SyntaxOK());
	IdxCurrentSelfEvalRule = None_SER;
	return true;
}

