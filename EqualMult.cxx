// EqualMult.cxx
// implementation of functions involving both EqualRelation and StdMultiplication

#include "Equal.hxx"
#include "StdMult.hxx"

bool EqualRelation::MultInvOutStdMultArg()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/26/2005
	//! \todo change name: this is called only as 2-ary!
	// InferenceParameter1 points to an argument of type StdMultiplication
	// The InferenceParameter1 of *that* points to the argument to be cancelled
	// we must distinguish between left-multiplication and right-multiplication
	// A VR arglist is required, which acquires the new args (as vector)
	// Then swap the arglists, and clean up the original args
	MetaConcept** VRArgList = _new_buffer<MetaConcept*>(2);
	if (VRArgList)
		{
		try	{
			ArgArray[1]->CopyInto(VRArgList[1]);
			ArgArray[0]->CopyInto(VRArgList[0]);
			}
		catch(const bad_alloc&)
			{	// TODO: improve error handling
			BLOCKDELETEARRAY(VRArgList);
			return false;
			};		
		MetaConcept* CancelThis = NULL;
		static_cast<MetaConceptWithArgArray*>(VRArgList[1])->TransferOutAndNULL(static_cast<MetaConceptWithArgArray*>(VRArgList[1])->ImageInferenceParameter1(),CancelThis);
		bool RightInverse = (static_cast<MetaConceptWithArgArray*>(VRArgList[1])->fast_size()-1==static_cast<MetaConceptWithArgArray*>(VRArgList[1])->ImageInferenceParameter1()) ? true : false;

		if (2!=static_cast<MetaConceptWithArgArray*>(VRArgList[1])->fast_size())
			static_cast<MetaConceptWithArgArray*>(VRArgList[1])->DeleteIdx(static_cast<MetaConceptWithArgArray*>(VRArgList[1])->ImageInferenceParameter1());
		else{
			MetaConcept* Tmp = NULL;
			static_cast<MetaConceptWithArgArray*>(VRArgList[1])->TransferOutAndNULL(1-static_cast<MetaConceptWithArgArray*>(VRArgList[1])->ImageInferenceParameter1(),Tmp);
			delete VRArgList[1];
			VRArgList[1]=Tmp;
			};

		if (CancelThis->SelfInverse(StdMultiplication_MC))
			{
			// error condtion should goto to faulty termination code
			if (VRArgList[1-InferenceParameter1]->IsExactType(StdMultiplication_MC))
				{	// StdMultiplication: add new arg to end (right-inverse) or
					// beginning (left-inverse)
				if (RightInverse)
					{
					if (!static_cast<MetaConceptWithArgArray*>(VRArgList[0])->AddArgAtEndAndForceCorrectForm(CancelThis))
						goto InternalFailure;
					}
				else{
					if (!static_cast<MetaConceptWithArgArray*>(VRArgList[0])->AddArgAtStartAndForceCorrectForm(CancelThis))
						goto InternalFailure;
					}
				}
			else{	// otherwise: create 2-ary StdMultiplication arg
				weakautoarray_ptr<MetaConcept*> TmpArgArray(2);
				if (TmpArgArray.empty()) goto InternalFailure;

				if (RightInverse)
					{
					TmpArgArray[1] = CancelThis;
					TmpArgArray[0] = VRArgList[0];
					}
				else{
					TmpArgArray[0] = CancelThis;
					TmpArgArray[1] = VRArgList[0];
					};

				VRArgList[0] = new(nothrow) StdMultiplication(TmpArgArray);
				if (!VRArgList[0]) goto InternalFailure;
				static_cast<StdMultiplication*>(VRArgList[0])->ForceCheckForEvaluation();
				}
			ArgArray = VRArgList;
			assert(SyntaxOK());
			return SelfEvalCleanEnd();
			}
InternalFailure:
		BLOCKDELETEARRAY(VRArgList);
		delete CancelThis;
		}
	return false;
}

