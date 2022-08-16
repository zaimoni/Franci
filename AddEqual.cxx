// AddEqual.cxx
// implementation of functions involving both EqualRelation and StdMultiplication

#include "Equal.hxx"
#include "StdAdd.hxx"

bool EqualRelation::AddInvOutStdAddArg()
{	// FORMALLY CORRECT: 10/26/2005
	// InferenceParameter1 points to an argument of type StdAddition
	// The InferenceParameter1 of *that* points to the argument to be cancelled
	// A VR arglist is required, which acquires the new args (as vector)
	// Then swap the arglists, and clean up the original args
	// NOTE: contrary to naming, this is only called for 2-ary ALLEQUAL
	// only called from SelfEval... so throwing std::bad_alloc is fine
	autovalarray_ptr_throws<MetaConcept*> VRArgList(2);
	ArgArray[0]->CopyInto(VRArgList[0]);
	ArgArray[1]->CopyInto(VRArgList[1]);
	if (!VRArgList[InferenceParameter1]->ArgN(static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->ImageInferenceParameter1())->SelfInverse(StdAddition_MC)) return false;
	
	autoval_ptr<MetaConcept> CancelThis;
	static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->TransferOutAndNULL(static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->ImageInferenceParameter1(),(MetaConcept*&)CancelThis);
	if (2!=static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->fast_size())
		static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->DeleteIdx(static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->ImageInferenceParameter1());
	else{
		MetaConcept* Tmp = NULL;
		static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->TransferOutAndNULL(1-static_cast<MetaConceptWithArgArray*>(VRArgList[InferenceParameter1])->ImageInferenceParameter1(),Tmp);
		delete VRArgList[InferenceParameter1];
		VRArgList[InferenceParameter1]=Tmp;
		};

	// error condtion should goto to faulty termination code
	if (VRArgList[1-InferenceParameter1]->IsExactType(StdAddition_MC))
		{	// StdAddition: add new arg to end
		if (!static_cast<MetaConceptWithArgArray*>(VRArgList[1-InferenceParameter1])->AddArgAtEndAndForceCorrectForm(CancelThis))
			return false;
		}
	else{	// otherwise: create 2-ary StdAddition arg
		autovalarray_ptr_throws<MetaConcept*> TmpArgArray(2);
		TmpArgArray[1] = CancelThis.release();
		TmpArgArray[0]=VRArgList[1-InferenceParameter1];
		VRArgList[1-InferenceParameter1] = nullptr;	// avoid double-delete
		VRArgList[1-InferenceParameter1] = new StdAddition(TmpArgArray);
		static_cast<StdAddition*>(VRArgList[1-InferenceParameter1])->ForceCheckForEvaluation();
		}
	VRArgList.MoveInto(ArgArray);
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}
