// AddMult.cxx
// implementation for inference rules that use both StdAddition and StdMultiplication

#include "Class.hxx"
#include "Integer1.hxx"
#include "StdAdd.hxx"
#include "StdMult.hxx"
#include "LowRel.hxx"

void StdAddition::AddTwoOneOverIntegers()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// only called from SelfEval...so throwing std::bad_alloc is fine
	//! \todo OPTIMIZE (dynamic RAM loading)
	// InferenceParameter1: the real integer
	// InferenceParameter2: StrictUB for IntegerNumeral type (the others are 1/integers)
	// want: 1/x+1/y |-> (x+y)/(xy)
	size_t i = InferenceParameter2;
	while(!ArgArray[--i]->IsUltimateType(&Rational));
	size_t j = i;
	while(!ArgArray[--j]->IsUltimateType(&Rational));
	// prepare:
	//	addition object with two arguments x,y			  
	//  multiplication object with three arguments (x+y),1/x,1/y
	//  then swap multiplication object, dispose of empty slot (make sure true integer is still bounded)
	autovalarray_ptr_throws<MetaConcept*> NewAddArgArray(2); 
	autovalarray_ptr_throws<MetaConcept*> NewMultArgArray(3); 
	ArgArray[j]->CopyInto(NewAddArgArray[1]);
	ArgArray[i]->CopyInto(NewAddArgArray[0]);
	NewMultArgArray[0] = new StdAddition();
	StdMultiplication* OverallTargetArg = new StdMultiplication();

	// No chance of RAM failure now
	NewAddArgArray[0]->SelfInverse(StdMultiplication_MC);
	NewAddArgArray[1]->SelfInverse(StdMultiplication_MC);
	static_cast<StdAddition*>(NewMultArgArray[0])->ReplaceArgArray(NewAddArgArray);
	static_cast<StdAddition*>(NewMultArgArray[0])->ForceCheckForEvaluation();
	TransferOutAndNULL(j,NewMultArgArray[1]);
	TransferOutAndNULL(i,NewMultArgArray[2]);
	OverallTargetArg->ReplaceArgArray(NewMultArgArray);
	OverallTargetArg->ForceCheckForEvaluation();
	assert(OverallTargetArg->SyntaxOK());
	// Now: place OverallTargetArg correctly
	if      (InferenceParameter1==InferenceParameter2)
		{
		ArgArray[j]=OverallTargetArg;
		InferenceParameter1-=2;
		}
	else if (j<InferenceParameter1)
		{
		ArgArray[j]=ArgArray[InferenceParameter1];
		ArgArray[InferenceParameter1]=OverallTargetArg;
		InferenceParameter1 = j;
		}
	else{
		ArgArray[j]=OverallTargetArg;
		}
	DeleteIdx(i);
	InferenceParameter2-=2;
	assert(SyntaxOK());
	ForceCheckForEvaluation();
}

void StdAddition::AddThreeOneOverIntegers()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// only called from SelfEval...so throwing std::bad_alloc is fine
	//! \todo OPTIMIZE (dynamic RAM loading)
	// InferenceParameter1: the real integer
	// InferenceParameter2: StrictUB for IntegerNumeral type (the others are 1/integers)	
	// want: 1/x+1/y+1/z |-> (yz+xz+xy)/(xyz)
	size_t Idx3 = InferenceParameter2;
	while(!ArgArray[--Idx3]->IsUltimateType(&Rational));
	size_t Idx2 = Idx3;
	while(!ArgArray[--Idx2]->IsUltimateType(&Rational));
	size_t Idx1 = Idx2;
	while(!ArgArray[--Idx1]->IsUltimateType(&Rational));
	// prepare:
	//	multplication object with two arguments y,z
	//	multplication object with two arguments x,z			  
	//	multplication object with two arguments x,y			  
	//  addition object with three arguments (above)
	//  multiplication object with four arguments: addition object (above), 1/x, 1/y, 1/z
	//  then swap multiplication object, dispose of empty slot (make sure true integer is still bounded)
	autovalarray_ptr_throws<MetaConcept*> MultArgArray1(2);
	autovalarray_ptr_throws<MetaConcept*> MultArgArray2(2);
	autovalarray_ptr_throws<MetaConcept*> MultArgArray3(2);	
	autovalarray_ptr_throws<MetaConcept*> NewAddArgArray(3);
	autovalarray_ptr_throws<MetaConcept*> NewMultArgArray(4);
	ArgArray[Idx1]->CopyInto(MultArgArray2[0]);
	ArgArray[Idx1]->CopyInto(MultArgArray3[0]);
	ArgArray[Idx2]->CopyInto(MultArgArray1[0]);
	ArgArray[Idx2]->CopyInto(MultArgArray3[1]);
	ArgArray[Idx3]->CopyInto(MultArgArray1[1]);
	ArgArray[Idx3]->CopyInto(MultArgArray2[1]);
	NewAddArgArray[0] = new StdMultiplication();
	NewAddArgArray[1] = new StdMultiplication();
	NewAddArgArray[2] = new StdMultiplication();
	NewMultArgArray[0] = new StdAddition();
	StdMultiplication* OverallTargetArg = new StdMultiplication();

	// No chance of RAM failure now
	MultArgArray1[0]->SelfInverse(StdMultiplication_MC);
	MultArgArray1[1]->SelfInverse(StdMultiplication_MC);
	MultArgArray2[0]->SelfInverse(StdMultiplication_MC);
	MultArgArray2[1]->SelfInverse(StdMultiplication_MC);
	MultArgArray3[0]->SelfInverse(StdMultiplication_MC);
	MultArgArray3[1]->SelfInverse(StdMultiplication_MC);
	static_cast<StdMultiplication*>(NewAddArgArray[0])->ReplaceArgArray(MultArgArray1);
	static_cast<StdMultiplication*>(NewAddArgArray[1])->ReplaceArgArray(MultArgArray2);
	static_cast<StdMultiplication*>(NewAddArgArray[2])->ReplaceArgArray(MultArgArray3);
	static_cast<StdAddition*>(NewMultArgArray[0])->ReplaceArgArray(NewAddArgArray);
	static_cast<StdMultiplication*>(NewAddArgArray[0])->ForceCheckForEvaluation();
	static_cast<StdMultiplication*>(NewAddArgArray[1])->ForceCheckForEvaluation();
	static_cast<StdMultiplication*>(NewAddArgArray[2])->ForceCheckForEvaluation();
	static_cast<StdAddition*>(NewMultArgArray[0])->ForceCheckForEvaluation();
	TransferOutAndNULL(Idx1,NewMultArgArray[1]);
	TransferOutAndNULL(Idx2,NewMultArgArray[2]);
	TransferOutAndNULL(Idx3,NewMultArgArray[3]);
	OverallTargetArg->ReplaceArgArray(NewMultArgArray);
	OverallTargetArg->ForceCheckForEvaluation();
	assert(OverallTargetArg->SyntaxOK());
	// Now: place OverallTargetArg correctly
	if      (InferenceParameter1==InferenceParameter2)
		{
		ArgArray[Idx1]=OverallTargetArg;
		InferenceParameter1-=3;
		}
	else if (Idx1<InferenceParameter1)
		{
		ArgArray[Idx1]=ArgArray[InferenceParameter1];
		ArgArray[InferenceParameter1]=OverallTargetArg;
		InferenceParameter1 = Idx1;
		}
	else{
		ArgArray[Idx1]=OverallTargetArg;
		}
	FlushNULLFromArray((MetaConcept**&)ArgArray,Idx2);
	InferenceParameter2-=3;
	assert(SyntaxOK());
	ForceCheckForEvaluation();
}

bool StdAddition::CleanIntegerNumeralBlock()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// only called from SelfEval...so throwing std::bad_alloc is fine
	// #1: Find upper bound of IntegerNumeral block
	size_t StrictUBIntBlock = 1;
	while(fast_size()> ++StrictUBIntBlock && ArgArray[StrictUBIntBlock]->IsExactType(IntegerNumeral_MC));
	while(   ArgArray[0]->IsNegative()
		  && ArgArray[StrictUBIntBlock-1]->IsPositive()
		  && ArgArray[0]->IsUltimateType(&Integer)
		  && ArgArray[StrictUBIntBlock-1]->IsUltimateType(&Integer))
		{	// Sum of positive and negative integer is RAM-safe
		static_cast<IntegerNumeral*>(ArgArray[0])->RearrangeSum(*static_cast<IntegerNumeral*>(ArgArray[StrictUBIntBlock-1]));
		if (ArgArray[StrictUBIntBlock-1]->IsZero())
			{
			FastDeleteIdx(--StrictUBIntBlock);
			if (1==StrictUBIntBlock)
				{
				assert(SyntaxOK());
				return SelfEvalCleanEnd();
				}
			};
		if (ArgArray[0]->IsZero())
			{
			FastDeleteIdx(0);
			if (1==--StrictUBIntBlock)
				{
				assert(SyntaxOK());
				return SelfEvalCleanEnd();
				}
			};
		};
	// Not finished yet.  Two stages here:
	// #1: do a sweep to reduce to a single integer plus a collection of 1/integers.
	// #2: take the 1/integers into a single product
	// apparently, we must *not* go here for a single integer plus a single 1/integer
	size_t i = StrictUBIntBlock;
	do	{
		size_t j = --i;
		do	if (static_cast<IntegerNumeral*>(ArgArray[--j])->ForceRearrangeSum(*static_cast<IntegerNumeral*>(ArgArray[i])))
				{
				if (ArgArray[j]->IsZero())
					{
					FastDeleteIdx(j);
					if (1==--StrictUBIntBlock)
						{
						assert(SyntaxOK());
						return SelfEvalCleanEnd();
						}
					if (1>= --i) break;
					};
				if (ArgArray[i]->IsZero())
					{
					FastDeleteIdx(i);
					if (1== --StrictUBIntBlock)
						{
						assert(SyntaxOK());
						return SelfEvalCleanEnd();
						}
					if (1>= --i) break;
					j = i;
					};
				}
		while(0<j);
		}
	while(1<i);
	if (2==StrictUBIntBlock && !ArgArray[0]->IsUltimateType(ArgArray[1]->UltimateType()))
		{	// Integer+1/Integer is intrinsically stable
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	// Not finished yet.
	InferenceParameter1=StrictUBIntBlock;
	i = StrictUBIntBlock;
	do	if (ArgArray[--i]->IsUltimateType(&Integer))
			{
			InferenceParameter1 = i;
			while(0<i)
				if (ArgArray[--i]->IsUltimateType(&Integer))
					return false;	// RAM shortage, or algorithm hole
			}
	while(0<i);
	InferenceParameter2 = StrictUBIntBlock;
	// InferenceParameter1 holds the sole integer in the list; others are 1/Integer
	// InferenceParameter2 holds StrictUBIntBlock
	// Object is to dispose of the 1/Integer listings using
	// 1/x+1/y = (x+y)/xy; 1/x+1/y+1/z=(yz+xz+xy)/xyz
	// Franci can go further...but the immediate object is simply to break an infinite recursion 
	// here.
	if (InferenceParameter1==StrictUBIntBlock)
		{	// NO integers to watch out for.
		if (1<InferenceParameter2 && 0!=InferenceParameter2%2)
			AddThreeOneOverIntegers();
		while(1<InferenceParameter2) AddTwoOneOverIntegers();
		}
	else{	// integer
		if (2<InferenceParameter2 && 0==InferenceParameter2%2)
			AddThreeOneOverIntegers();
		while(2<InferenceParameter2) AddTwoOneOverIntegers();
		}
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool StdAddition::EqualArgsToIntegerProduct()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// InferenceParameter1: lowest Idx of == args
	// InferenceParameter2: how many == args
	// only called from SelfEval...so throwing std::bad_alloc is fine
	autovalarray_ptr_throws<MetaConcept*> MultArgArray(2);
	MultArgArray[0] = new IntegerNumeral((unsigned long)(InferenceParameter2));
	StdMultiplication* TargetMult = new StdMultiplication();

	// RAM-safe now
	size_t i = fast_size();
	size_t LastHit = 0;
	while(InferenceParameter1< --i)
		if (*ArgArray[InferenceParameter1]==*ArgArray[i])
			{
			DELETE_AND_NULL(ArgArray[i]);
			LastHit = i;
			}
	FlushNULLFromArray((MetaConcept**&)ArgArray,LastHit);

	MultArgArray[1] = ArgArray[InferenceParameter1];
	ArgArray[InferenceParameter1] = TargetMult;
	TargetMult->ReplaceArgArray(MultArgArray);
	TargetMult->ForceCheckForEvaluation();
	assert(TargetMult->SyntaxOK());
	assert(SyntaxOK());
	return SelfEvalCleanEnd();
}

bool StdMultiplication::AddIntegerToIntegerFraction()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// InferenceParameter2: IntegerNumeral (may be Z or Q...both have to be cleaned up)
	// InferenceParameter1: IntegerNumeral-based fraction (one Z, one Q)
	// InferenceParameter1 is irreducible by hypothesis
	// NOTE: this code uses goto to centralize RAM cleanup in failure mode.
	// NOTE: DestructiveAddABBothZ cleans up args if it succeeds..otherwise, it 
	// functions like a partial rearrange.
	// only called from SelfEval...so throwing std::bad_alloc is fine
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(InferenceParameter1!=InferenceParameter2);
	assert(ArgArray[InferenceParameter2]->IsExactType(IntegerNumeral_MC));
	const bool FirstArgZ = ArgArray[InferenceParameter1]->ArgN(0)->IsUltimateType(&Integer);
	const size_t BIdx = (FirstArgZ) ? 0 : 1;
	const size_t CIdx = (FirstArgZ) ? 1 : 0;
	//! \todo no-allocation version
	autoval_ptr<IntegerNumeral> A;
	A = new IntegerNumeral(*static_cast<IntegerNumeral*>(ArgArray[InferenceParameter2]));
	autoval_ptr<IntegerNumeral> B;
	B = new IntegerNumeral(*static_cast<IntegerNumeral*>(ArgArray[InferenceParameter1]->ArgN(BIdx)));
	autoval_ptr<IntegerNumeral> C;
	C = new IntegerNumeral(*static_cast<IntegerNumeral*>(ArgArray[InferenceParameter1]->ArgN(CIdx)));

	C->SelfInverse(StdMultiplication_MC);
	if (ArgArray[InferenceParameter2]->IsUltimateType(&Integer))
		{	// a+b/c
		// general answer: (ac+b)/c
		// a is non-zero by hypothesis (wouldn't get here)
		// *c<sup>-1</sup> is unavoidable; optimizations will be in a*c+b, which is 
		// best handled as an in-place replacement of b, zeroing a afterwards.
		// result will be irreducible when b/c is irreducible (condition).
		// NOTE: want function that processes ac+b "efficiently"
		if (!B->DestructiveAddABBothZ(A,C)) return false;
		static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->TransferInAndOverwrite(BIdx,B);
		static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->ForceCheckForEvaluation();
		assert(ArgArray[InferenceParameter1]->SyntaxOK());
		FastDeleteIdx(InferenceParameter2);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	else{	// 1/a+b/c
		// general answer: (c+ab)/ac
		// Result may not be irreducible (GCF(a,c) may happen).
		autoval_ptr<IntegerNumeral> AInv;
		autoval_ptr<IntegerNumeral> CInv;
		AInv = new IntegerNumeral(*A);
		CInv = new IntegerNumeral(*C);

		CInv->SelfInverse(StdMultiplication_MC);
		A->SelfInverse(StdMultiplication_MC);
		if (   !AInv->ForceRearrangeProduct(*CInv)
			|| !C->DestructiveAddABBothZ(A,B))
			return false;
		AInv->DestructiveNormalProductFormWithRHS(*CInv);
		// NOTE: maybe we want to be able to test absolute value?
		if      (AInv->IsOne())
			{
			static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->TransferInAndOverwrite(CIdx,CInv);
			AInv.reset();
			}
		else{
			SUCCEED_OR_DIE(CInv->IsOne());	// check that function return value is valid
			static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->TransferInAndOverwrite(CIdx,AInv);
			CInv.reset();
			}
		static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->TransferInAndOverwrite(BIdx,C);
		static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->ForceCheckForEvaluation();
		assert(ArgArray[InferenceParameter1]->SyntaxOK());
		FastDeleteIdx(InferenceParameter2);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	return false;
}

bool StdMultiplication::AddIntegerFractionToIntegerFraction()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// InferenceParameter1: IntegerNumeral-based fraction (one Z, one Q)
	// InferenceParameter2: IntegerNumeral-based fraction (one Z, one Q)
	// InferenceParameter1,InferenceParameter2 are irreducible by hypothesis
	// NOTE: this code uses goto to centralize RAM cleanup in failure mode.
	// base form: a/b+c/d = (ad+bc)/bd
	// only called from SelfEval...so throwing std::bad_alloc is fine
	assert(size()>InferenceParameter1);
	assert(size()>InferenceParameter2);
	assert(InferenceParameter1!=InferenceParameter2);
	const bool FirstProductFirstArgZ = ArgArray[InferenceParameter1]->ArgN(0)->IsUltimateType(&Integer);
	const size_t AIdx = (FirstProductFirstArgZ) ? 0 : 1;
	const size_t BIdx = (FirstProductFirstArgZ) ? 1 : 0;
	const bool SecondProductFirstArgZ = ArgArray[InferenceParameter2]->ArgN(0)->IsUltimateType(&Integer);
	const size_t CIdx = (SecondProductFirstArgZ) ? 0 : 1;
	const size_t DIdx = (SecondProductFirstArgZ) ? 1 : 0;
	//! \todo no-allocation version
	autoval_ptr<IntegerNumeral> A;
	A = new IntegerNumeral(*static_cast<IntegerNumeral*>(ArgArray[InferenceParameter1]->ArgN(AIdx)));
	autoval_ptr<IntegerNumeral> B;
	B = new IntegerNumeral(*static_cast<IntegerNumeral*>(ArgArray[InferenceParameter1]->ArgN(BIdx)));
	autoval_ptr<IntegerNumeral> C;
	C = new IntegerNumeral(*static_cast<IntegerNumeral*>(ArgArray[InferenceParameter2]->ArgN(CIdx)));
	autoval_ptr<IntegerNumeral> D;
	D = new IntegerNumeral(*static_cast<IntegerNumeral*>(ArgArray[InferenceParameter2]->ArgN(DIdx)));
	//! \todo time to use GCF short-circuits is here: extract GCF(B,D), then initialize DInv
	//! * Initialize smaller of BInv, DInv (as RAM)
	//! * remove GCF from B, D
	//! * Initialize other one
	autoval_ptr<IntegerNumeral> BInv;
	BInv = new IntegerNumeral(*B);
	autoval_ptr<IntegerNumeral> DInv;
	DInv = new IntegerNumeral(*D);

	B->SelfInverse(StdMultiplication_MC);
	D->SelfInverse(StdMultiplication_MC);
	if (A->ForceRearrangeProduct(*D))
		{
		A->DestructiveNormalProductFormWithRHS(*D);
		if (D->IsOne()) D.reset();
		else{
			SUCCEED_OR_DIE(A->IsOne());	// check that function return value is valid
			D.MoveInto(A);
			}
		};
	assert(D.empty());	// A*D either successful or hit SUCCEED_OR_DIE
	if (BInv->ForceRearrangeProduct(*DInv))
		{
		BInv->DestructiveNormalProductFormWithRHS(*DInv);
		if (DInv->IsOne()) DInv.reset();
		else{
			SUCCEED_OR_DIE(BInv->IsOne());	// check that function return value is valid
			DInv.MoveInto(BInv);
			}
		};
	assert(DInv.empty());	// BInv*DInv either successful, or hit SUCCEED_OR_DIE
	if (A->DestructiveAddABBothZ(B,C))
		{	// minimal to no RAM issues
		static_cast<StdMultiplication*>(ArgArray[InferenceParameter2])->TransferInAndOverwrite(CIdx,A);
		static_cast<StdMultiplication*>(ArgArray[InferenceParameter2])->TransferInAndOverwrite(DIdx,BInv);
		static_cast<StdMultiplication*>(ArgArray[InferenceParameter2])->ForceCheckForEvaluation();
		assert(ArgArray[InferenceParameter2]->SyntaxOK());
		FastDeleteIdx(InferenceParameter1);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	//! \todo FIX: recovery code
	return false;
}

// NOTE: due to issues with commutativity of multiplication, Franci relies on
// multiplicands commuting to a standard order.  The controlling function 
// AllNonConstArgsEqualUpToAddInv for the rule 
// MultDistributesOverAdd_CondenseProductsOnNonConstArgs must insure that 
// the constant args to be distributed against are all at one side or the other.
// For now, the algorithm is just interested in products over the complex 
// numbers.  This means we only need to worry about constants piling up on 
// the left. [This may mean rerigging the standard form code at some point.]
// The testing code should probably pass the useful parameters to this function:
// length of the block to be factored
// OppSignToggle
bool StdMultiplication::MultDistributesOverAdd_CondenseProductsOnNonConstArgs()
{	// FORMALLY CORRECT: Kenneth Boyd, 10/13/2004
	// Assumption: both args are of type StdMultiplication, and have been approved for factoring
	// only called from SelfEval...so throwing std::bad_alloc is fine
	assert(ArgArray.size()>InferenceParameter1);
	assert(ArgArray.size()>InferenceParameter2);
	assert(ArgArray[InferenceParameter1]);
	assert(ArgArray[InferenceParameter2]);
	assert(ArgArray[InferenceParameter1]->IsExactType(StdMultiplication_MC));
	assert(ArgArray[InferenceParameter2]->IsExactType(StdMultiplication_MC));
	const size_t ArityArg1 = static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->fast_size();
	const size_t ArityArg2 = static_cast<StdMultiplication*>(ArgArray[InferenceParameter2])->fast_size();

	const signed long CommonBlockArity = static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->InferenceParameter1;
	signed long OppSignToggle = static_cast<StdMultiplication*>(ArgArray[InferenceParameter1])->InferenceParameter2;
	const size_t BlockLength1 = ArityArg1-CommonBlockArity;
	const size_t BlockLength2 = ArityArg2-CommonBlockArity;

	autoval_ptr<StdAddition> NewSum;
	NewSum = new StdAddition();
	NewSum->insertNSlotsAt(2,0);
	autoval_ptr<StdMultiplication> SumProduct1;
	autoval_ptr<StdMultiplication> SumProduct2;
	autoval_ptr<IntegerNumeral> IntegerEscape;
	if      (0==BlockLength1)
		{
		SumProduct2 = new StdMultiplication();
		SumProduct2->insertNSlotsAt(BlockLength2,0);
		IntegerEscape = new IntegerNumeral(OppSignToggle);

		size_t i = BlockLength2;
		do	{
			MetaConcept* Tmp = NULL;
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->TransferOutAndNULL(--i,Tmp);
			SumProduct2->TransferInAndOverwriteRaw(i,Tmp);
			}
		while(0<i);
		SumProduct2->ForceCheckForEvaluation();

		NewSum->TransferInAndOverwrite(0,IntegerEscape);						
		NewSum->TransferInAndOverwrite(1,SumProduct2);						
		NewSum->ForceCheckForEvaluation();
		assert(NewSum->SyntaxOK());

		if (1<BlockLength2)
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->DeleteNSlotsAt(BlockLength2-1,0);
		static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->TransferInAndOverwrite(0,NewSum);
		static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->ForceCheckForEvaluation();
		assert(ArgArray[InferenceParameter2]->SyntaxOK());
		FastDeleteIdx(InferenceParameter1);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	else if (0==BlockLength2)
		{
		SumProduct1 = new StdMultiplication();
		SumProduct1->insertNSlotsAt(BlockLength1,0);
		IntegerEscape = new IntegerNumeral(OppSignToggle);

		size_t i = BlockLength1;
		do	{
			MetaConcept* Tmp = NULL;
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->TransferOutAndNULL(--i,Tmp);
			SumProduct1->TransferInAndOverwriteRaw(i,Tmp);
			}
		while(0<i);
		SumProduct1->ForceCheckForEvaluation();

		NewSum->TransferInAndOverwrite(0,SumProduct1);						
		NewSum->TransferInAndOverwrite(1,IntegerEscape);						
		NewSum->ForceCheckForEvaluation();
		assert(NewSum->SyntaxOK());

		if (1<BlockLength1)
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->DeleteNSlotsAt(BlockLength1-1,0);
		static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->TransferInAndOverwrite(0,NewSum);			
		static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->ForceCheckForEvaluation();
		assert(ArgArray[InferenceParameter1]->SyntaxOK());
		FastDeleteIdx(InferenceParameter2);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
	else{	// general case
		SumProduct1 = new StdMultiplication();
		SumProduct1->insertNSlotsAt(BlockLength1,0);
		SumProduct2 = new StdMultiplication();
		SumProduct2->insertNSlotsAt(BlockLength2,0);

		size_t i = BlockLength1;
		do	{
			MetaConcept* Tmp = NULL;
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->TransferOutAndNULL(--i,Tmp);
			SumProduct1->TransferInAndOverwriteRaw(i,Tmp);
			}
		while(0<i);
		i = BlockLength2;
		do	{
			MetaConcept* Tmp = NULL;
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter2])->TransferOutAndNULL(--i,Tmp);
			SumProduct2->TransferInAndOverwriteRaw(i,Tmp);
			}
		while(0<i);
		SumProduct1->ForceCheckForEvaluation();
		SumProduct2->ForceCheckForEvaluation();

		NewSum->TransferInAndOverwrite(0,SumProduct1);						
		NewSum->TransferInAndOverwrite(1,SumProduct2);						
		NewSum->ForceCheckForEvaluation();
		assert(NewSum->SyntaxOK());

		if (1<BlockLength1)
			static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->DeleteNSlotsAt(BlockLength1-1,0);
		static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->TransferInAndOverwrite(0,NewSum);			
		static_cast<MetaConceptWithArgArray*>(ArgArray[InferenceParameter1])->ForceCheckForEvaluation();
		assert(ArgArray[InferenceParameter1]->SyntaxOK());
		FastDeleteIdx(InferenceParameter2);
		assert(SyntaxOK());
		return SelfEvalCleanEnd();
		}
}

