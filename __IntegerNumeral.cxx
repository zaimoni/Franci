// __IntegerNumeral.cpp
// standalone version of IntegerNumeral (not glued to MetaConcept)
// note: direct multiplicative inverse of integers are also subsumed by this type

// Integer1.cxx
// Implementation of IntegerNumeral
// This is an abusive name; direct multiplicative inverses of integers are also subsumed by this type.

#include "__IntegerNumeral.hxx"
#include "__GCF.hxx"
#include "Zaimoni.STL/lite_alg.hpp"
#include "Zaimoni.STL/MetaRAM2.hpp"

using namespace std;
using namespace zaimoni;

// multiplicative inverse
#define MULT_INV_TEXT "<sup>-1</sup>"

// \todo consider upgrading on 64-bit ptr systems
// This type works in base 10^9
enum PowersOfTen	{
					TEN_TO_0	= 1,
					TEN_TO_1	= 10,
					TEN_TO_2	= 100,
					TEN_TO_3	= 1000,
					TEN_TO_4	= 10000,
					TEN_TO_5	= 100000,
					TEN_TO_6	= 1000000,
					TEN_TO_7	= 10000000,
					TEN_TO_8	= 100000000,
					TEN_TO_9	= 1000000000,
					WRAPLB		= 1000000000
					};

// VFT2Idx constants
enum VFT2IdxConstants	{
						ZeroIdx_VFT2 = 0,
						LargeIntInvIdx_VFT2 = 1,
						SmallIntInvIdx_VFT2 = 2,
						SmallIntIdx_VFT2 = 3,
						LargeIntIdx_VFT2 = 4,
						ZeroOffset_VFT2 = LargeIntIdx_VFT2	// NOTE: SYNC with Integer.hxx, LenName.cxx, Destruct.cxx
						};

// Memory representation types
enum RepresentationTypes_INT	{
								Large_INT = 0,
								Small_INT = 1,
								HardCoded_INT = 2,
								MaxRepresentation_INT = HardCoded_INT+1
								};

// design decision to shove 1/int into mode tables was to facilitate total ordering
static const unsigned short IntegerRAMForm[2*ZeroOffset_VFT2+1]
 =	{
	Large_INT,
	Small_INT,
	Small_INT,
	Large_INT,
	HardCoded_INT,
	Large_INT,
	Small_INT,
	Small_INT,
	Large_INT
	};

// FORMALLY CORRECT: 4/16/98, Kenneth Boyd
_IntegerNumeral::SyntaxOKAuxFunc _IntegerNumeral::SyntaxOKAux[MaxRepresentation_INT]
 =	{
	&_IntegerNumeral::SyntaxOKAuxLargeInt,
	&_IntegerNumeral::SyntaxOKAuxSmallInt,
	&_IntegerNumeral::SyntaxOKAuxHardCodedInt
	};

_IntegerNumeral::FinishAssignmentAuxFunc _IntegerNumeral::FinishAssignmentAuxV2[MaxRepresentation_INT]
 =	{	&_IntegerNumeral::FinishAssignmentAuxLargeInt,
		&_IntegerNumeral::FinishAssignmentAuxSmallInt,
		&_IntegerNumeral::FinishAssignmentAuxHardCodedInt
	};

_IntegerNumeral::ConstructSelfNameAuxFunc _IntegerNumeral::ConstructSelfNameAux2[2*ZeroOffset_VFT2+1]
 =	{
	&_IntegerNumeral::ConstructSelfNameNegativeInt,
	&_IntegerNumeral::ConstructSelfNameSmallNegativeInt,
	&_IntegerNumeral::ConstructSelfNameSmallNegativeIntInv,
	&_IntegerNumeral::ConstructSelfNameNegativeIntInv,
	&_IntegerNumeral::ConstructSelfNameZeroInt,
	&_IntegerNumeral::ConstructSelfNamePositiveIntInv,
	&_IntegerNumeral::ConstructSelfNameSmallPositiveIntInv,
	&_IntegerNumeral::ConstructSelfNameSmallPositiveInt,
	&_IntegerNumeral::ConstructSelfNamePositiveInt
	};

_IntegerNumeral::LengthOfSelfNameAuxFunc _IntegerNumeral::LengthOfSelfNameAux2[2*ZeroOffset_VFT2+1]
 =	{
	&_IntegerNumeral::LengthOfSelfNameNegativeInt,
	&_IntegerNumeral::LengthOfSelfNameSmallNegativeInt,
	&_IntegerNumeral::LengthOfSelfNameSmallNegativeIntInv,
	&_IntegerNumeral::LengthOfSelfNameNegativeIntInv,
	&_IntegerNumeral::LengthOfSelfNameZeroInt,
	&_IntegerNumeral::LengthOfSelfNamePositiveIntInv,
	&_IntegerNumeral::LengthOfSelfNameSmallPositiveIntInv,
	&_IntegerNumeral::LengthOfSelfNameSmallPositiveInt,
	&_IntegerNumeral::LengthOfSelfNamePositiveInt
	};

_IntegerNumeral::OpLT_AuxFunc _IntegerNumeral::OpLT_SameTypeClassify[2*ZeroOffset_VFT2+1]
 =	{	&_IntegerNumeral::OpLT_1st2ndNegativeInt,
		&_IntegerNumeral::OpLT_1st2ndSmallNegativeInt,
		&_IntegerNumeral::OpLT_1st2ndSmallPositiveInt,		// OpLT_1st2ndSmallNegativeIntInv
		&_IntegerNumeral::OpLT_1st2ndPositiveInt,			// OpLT_1st2ndNegativeIntInv
		&_IntegerNumeral::OpLT_TrivialFalse,
		&_IntegerNumeral::OpLT_1st2ndNegativeInt,			// OpLT_1st2ndPositiveIntInv
		&_IntegerNumeral::OpLT_1st2ndSmallNegativeInt,		// OpLT_1st2ndSmallPositiveIntInv
		&_IntegerNumeral::OpLT_1st2ndSmallPositiveInt,
		&_IntegerNumeral::OpLT_1st2ndPositiveInt
	};

// NOTE: 1/0 is garbage [NOT SyntaxOK]
// NOTE: 1/1=1, 1/-1=-1; catch these immediately

///////////
// Divisibility tests in Base 10^9
//	*	2, 5: direct modulo on lowest significant digit.  IF the numeral ends is 0, we have
//		2^9 and 5^9 as factors, and may continue the exercise.
//	*	10^9-1: take sum of digits, reducing by 10^9-1 as necessary to prevent overflows.
//		This will diagnose:
//			3^4
//			37
//		333667	[this is a prime]
//		[Erasthones sieve: check up to 577, starting at 7]
//	*	10^9+1: take sum of alternating digits, reducing by 10^9+1 as necessary; subtract sums
//		This will diagnose:
//			7
//			11
//			13
//			19
//			52579 [this is a prime]
//		[Erasthones sieve: check up to 999, starting at 17]

//	NOTE: RemainderOfLargeDivideByN is a RAM-safe operation.  This allows
//	testing for divisibility by any prime < 10^9 in a RAM-safe fashion.
//	This would be an exhaustive primality test for any number < 10^18.
//	There exists (if difficult-to-find) a minimal spanning set of products of 
//	primes to first powers allowing a linear-time test for all of these primes.  
//	More useful might be a condensed series in order (this would permit bailing 
//	out at a sufficiently early point)

// Constructors and destructors
// KB: cannot use standard declaration, since the = operator reacts to the memory management
// state of the LHS.
_IntegerNumeral::_IntegerNumeral(const _IntegerNumeral& src)
:	VFT2Idx(ZeroIdx_VFT2)
{	// FORMALLY CORRECT: 1/17/1999, Kenneth Boyd
	LongInteger = NULL;
	operator=(src);
}

_IntegerNumeral::_IntegerNumeral(unsigned short src)
{	// FORMALLY CORRECT: 5/18/2006, Kenneth Boyd
	if (0==src)
		VFT2Idx = ZeroIdx_VFT2;
	else{
		VFT2Idx = SmallIntIdx_VFT2;
		ShortInteger = src;
		};
}

_IntegerNumeral::_IntegerNumeral(signed short src)
{	// FORMALLY CORRECT: 5/18/2006, Kenneth Boyd
	if (0==src)
		VFT2Idx = ZeroIdx_VFT2;
	else if (0<src)
		{
		VFT2Idx = SmallIntIdx_VFT2;
		ShortInteger = src;
		}
	else{
		VFT2Idx = -SmallIntIdx_VFT2;
		ShortInteger = -src;
		};
}

_IntegerNumeral::_IntegerNumeral(unsigned long src)
{	// FORMALLY CORRECT: 5/18/2006, Kenneth Boyd
	if (WRAPLB<=src)
		{
		VFT2Idx = LargeIntIdx_VFT2;
		LongInteger = _new_buffer_uninitialized_nonNULL_throws<unsigned long>(2);
		LongInteger[0] = src/WRAPLB;
		LongInteger[1] = src%WRAPLB;
		}
	else if (0<src)
		{
		ShortInteger = src;
		VFT2Idx = SmallIntIdx_VFT2;
		}
	else
		VFT2Idx = ZeroIdx_VFT2;
}

_IntegerNumeral::_IntegerNumeral(signed long src)
{	// FORMALLY CORRECT: 5/18/2006, Kenneth Boyd
	if (0==src)
		VFT2Idx = ZeroIdx_VFT2;
	else if (0<src)
		{
		if (WRAPLB<=src)
			{
			VFT2Idx = LargeIntIdx_VFT2;
			LongInteger = _new_buffer_uninitialized_nonNULL_throws<unsigned long>(2);
			LongInteger[0] = src%WRAPLB;
			LongInteger[1] = src/WRAPLB;
			}
		else{
			VFT2Idx = SmallIntIdx_VFT2;
			ShortInteger = src;
			}
		}
	else{
		if (-WRAPLB>=src)
			{
			VFT2Idx = -LargeIntIdx_VFT2;
			LongInteger = _new_buffer_uninitialized_nonNULL_throws<unsigned long>(2);
			src = -src;
			LongInteger[0] = src%WRAPLB;
			LongInteger[1] = src/WRAPLB;
			}
		else{
			VFT2Idx = -SmallIntIdx_VFT2;
			ShortInteger = -src;
			}
		}
}

_IntegerNumeral::_IntegerNumeral(const char* src)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/18/2006
	assert(IsLegalIntegerString(src));
	src += strspn(src,"0");
	if ('\0'==src[0])
		VFT2Idx = ZeroIdx_VFT2;
	else{
		size_t DigitCount1 = strlen(src);
		if (9>=DigitCount1)
			{	// short _IntegerNumeral
			VFT2Idx = SmallIntIdx_VFT2;
			// stuff code digits in
			ShortInteger = (src[0]-'0');
			++src;
			while('\0'!=src[0])
				{
				ShortInteger*=10;
				ShortInteger+=(src[0]-'0');
				++src;
				}
			}
		else{	// long _IntegerNumeral
			const size_t strict_ub = (DigitCount1-1)/9+1;
			VFT2Idx = LargeIntIdx_VFT2;
			LongInteger = _new_buffer_nonNULL_throws<unsigned long>(strict_ub);

			size_t i = strict_ub;
			do	{
				LongInteger[ --i] = (src[0]-'0');
				++src;
				--DigitCount1;
				while(DigitCount1>9*i)
					{
					LongInteger[i]*=10;
					LongInteger[i]+=(src[0]-'0');
					++src;
					--DigitCount1;
					};
				}
			while(0<i);
			}
		}		
}

_IntegerNumeral::~_IntegerNumeral()
{	// FORMALLY CORRECT: 4/16/98, Kenneth Boyd
	// META: LHS constant *must* be the representation index for large integers
	if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		free(LongInteger);
}

_IntegerNumeral& _IntegerNumeral::operator=(const _IntegerNumeral& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/18/2006
	// NOTE: mixed-mode interpretation problems.
	if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		{
		if (Large_INT!=IntegerRAMForm[src.VFT2Idx+ZeroOffset_VFT2])
			// Target Long, source isn't.
			FREE_AND_NULL(LongInteger);			
		};
	// not-large handled in fake VFT call (not ACID to handle here)

	(this->*FinishAssignmentAuxV2[IntegerRAMForm[src.VFT2Idx+ZeroOffset_VFT2]])(src);	// large can throw
	VFT2Idx = src.VFT2Idx;
	return *this;
}

_IntegerNumeral::_IntegerNumeral(_IntegerNumeral&& src) : ShortInteger(src.ShortInteger), VFT2Idx(src.VFT2Idx)
{
	src.ShortInteger = 0;
	src.VFT2Idx = ZeroIdx_VFT2;
}

_IntegerNumeral& _IntegerNumeral::operator=(_IntegerNumeral&& src)
{
	if (Large_INT == IntegerRAMForm[VFT2Idx + ZeroOffset_VFT2]) free(LongInteger);
	ShortInteger = src.ShortInteger;
	VFT2Idx = src.VFT2Idx;
	src.ShortInteger = 0;
	src.VFT2Idx = ZeroIdx_VFT2;
	return *this;
}

_IntegerNumeral& _IntegerNumeral::operator=(signed short src)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2003
	if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		free(LongInteger);
	if (0==src)
		VFT2Idx = ZeroIdx_VFT2;
	else if (0<src)
		{
		VFT2Idx = SmallIntIdx_VFT2;
		ShortInteger = src;
		}
	else{
		VFT2Idx = -SmallIntIdx_VFT2;
		ShortInteger = -src;
		};
	return *this;
}

_IntegerNumeral& _IntegerNumeral::operator=(unsigned short src)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2003
	if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		free(LongInteger);
	if (0==src)
		VFT2Idx = ZeroIdx_VFT2;
	else{
		VFT2Idx = SmallIntIdx_VFT2;
		ShortInteger = src;
		};
	return *this;
}

_IntegerNumeral& _IntegerNumeral::operator=(signed long src)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2003
	if (WRAPLB<=src || -WRAPLB>=src)
		{
		if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
			{
			if (2*sizeof(signed long)!=_msize(LongInteger))
				LongInteger = REALLOC(LongInteger,2*sizeof(signed long));	// min size two signed longs, so this is safe
			}
		else{
			LongInteger = _new_buffer_uninitialized_nonNULL_throws<unsigned long>(2);
			}
		}
	else if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		free(LongInteger);

	if (0==src)
		VFT2Idx = ZeroIdx_VFT2;
	else if (0<src)
		{
		if (WRAPLB<=src)
			{
			VFT2Idx = LargeIntIdx_VFT2;
			LongInteger[0] = src%WRAPLB;
			LongInteger[1] = src/WRAPLB;
			}
		else{
			VFT2Idx = SmallIntIdx_VFT2;
			ShortInteger = src;
			}
		}
	else{
		if (-WRAPLB>=src)
			{
			VFT2Idx = -LargeIntIdx_VFT2;
			src = -src;
			LongInteger[0] = src%WRAPLB;
			LongInteger[1] = src/WRAPLB;
			}
		else{
			VFT2Idx = -SmallIntIdx_VFT2;
			ShortInteger = -src;
			}
		}
	return *this;
}

_IntegerNumeral& _IntegerNumeral::operator=(unsigned long src)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/13/2003
	if (WRAPLB<=src)
		{
		if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
			{
			if (2*sizeof(signed long)!=_msize(LongInteger))
				LongInteger = REALLOC(LongInteger,2*sizeof(signed long));	// min size two signed longs, so this is safe
			}
		else{
			LongInteger = _new_buffer_uninitialized_nonNULL_throws<unsigned long>(2);
			}
		VFT2Idx = LargeIntIdx_VFT2;
		LongInteger[0] = src/WRAPLB;
		LongInteger[1] = src%WRAPLB;
		}
	else{
		if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
			free(LongInteger);
		if (0<src)
			{
			ShortInteger = src;
			VFT2Idx = SmallIntIdx_VFT2;
			}
		else
			VFT2Idx = ZeroIdx_VFT2;
		}
	return *this;
}

void _IntegerNumeral::FinishAssignmentAuxSmallInt(const _IntegerNumeral& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/6/1999
	ShortInteger = src.ShortInteger;
}

void _IntegerNumeral::FinishAssignmentAuxLargeInt(const _IntegerNumeral& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/18/2006
	const unsigned long Tmp = ShortInteger;
	const bool TargetNotLarge = (Large_INT!=IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2]);
	if (TargetNotLarge) LongInteger=NULL;
	CopyDataFromPtrToPtr(LongInteger,src.LongInteger,ArraySize(src.LongInteger));
	if (NULL==LongInteger)
		{
		if (TargetNotLarge) ShortInteger = Tmp;
		throw bad_alloc();
		}
}

void _IntegerNumeral::FinishAssignmentAuxHardCodedInt(const _IntegerNumeral& src)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/6/1999
}

bool _IntegerNumeral::operator==(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2000
	if (VFT2Idx == rhs.VFT2Idx)
		{
		if      (HardCoded_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
			return true;
		else if (Small_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
			return ShortInteger == rhs.ShortInteger;
		// else if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		else if (ArraySize(LongInteger)==ArraySize(rhs.LongInteger))
			{
			size_t i = ArraySize(LongInteger);
			do	{
				--i;
				if (LongInteger[i] != rhs.LongInteger[i])
					return false;
				}
			while(0<i);
			return true;
			}
		};
	return false;
}

bool _IntegerNumeral::operator==(signed short rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/5/2001
	if (0==rhs)
		return ZeroIdx_VFT2==VFT2Idx;
	if (0>rhs)
		return -SmallIntIdx_VFT2==VFT2Idx && ShortInteger==(unsigned long)(-rhs);
	return SmallIntIdx_VFT2==VFT2Idx && ShortInteger==(unsigned long)(-rhs);
}

bool _IntegerNumeral::operator==(unsigned short rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/5/2001
	if (0==rhs)
		return ZeroIdx_VFT2==VFT2Idx;
	return SmallIntIdx_VFT2==VFT2Idx && ShortInteger==(unsigned long)(rhs);
}

bool _IntegerNumeral::AbsValIsOneOverN(unsigned short N) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/7/2001
	if (	(    SmallIntInvIdx_VFT2==VFT2Idx
		 	 || -SmallIntInvIdx_VFT2==VFT2Idx)
		&& ShortInteger==(unsigned long)(N))
		return true;
	return false;
}

//  Evaluation functions
bool _IntegerNumeral::SyntaxOK() const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/6/1999
	return (this->*SyntaxOKAux[IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2]])();
}

bool _IntegerNumeral::SyntaxOKAuxSmallInt() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2000
	return WRAPLB>ShortInteger;
}

bool _IntegerNumeral::SyntaxOKAuxLargeInt() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2000
	return NULL!=LongInteger && 2<=ArraySize(LongInteger);
}

bool _IntegerNumeral::SyntaxOKAuxHardCodedInt() const {return true;}

bool _IntegerNumeral::IsOne() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	return ZeroIdx_VFT2+SmallIntIdx_VFT2==VFT2Idx && 1==ShortInteger;
}

bool _IntegerNumeral::in_Z() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/11/2007
	if (-LargeIntInvIdx_VFT2==VFT2Idx) return false;
	if (-SmallIntInvIdx_VFT2==VFT2Idx) return false;
	if (LargeIntInvIdx_VFT2==VFT2Idx) return false;
	if (SmallIntInvIdx_VFT2==VFT2Idx) return false;
	return true;
}

bool _IntegerNumeral::SelfInverse(const operation_index Operation)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/15/2000
	if (Addition==Operation)
		{	// type swap
		VFT2Idx = -VFT2Idx;
		return true;
		};
	assert(Multiplication==Operation);
//	if (Multiplication==Operation)
//		{	// type swap
			// trap +-1 as a special case
		if      (0<VFT2Idx)
			{
			if (SmallIntIdx_VFT2!=VFT2Idx || 1!=ShortInteger)
				VFT2Idx = LargeIntIdx_VFT2+1-VFT2Idx;
			return true;
			}
		else if (0>VFT2Idx)
			{
			if (-SmallIntIdx_VFT2!=VFT2Idx || 1!=ShortInteger)
				VFT2Idx = -LargeIntIdx_VFT2-1-VFT2Idx;
			return true;
			}
//		};	
	return false;
}

bool
_IntegerNumeral::SelfInverseTo(const _IntegerNumeral& rhs, const operation_index Operation) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/30/2001
	if (   Addition==Operation
		&& VFT2Idx== -rhs.VFT2Idx)
		{
		if 		(HardCoded_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
			return true;
		else if (Small_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
			return ShortInteger==rhs.ShortInteger;
		// else if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2] && ...)
		if (   ArraySize(LongInteger)==ArraySize(rhs.LongInteger)
			&& LongInteger[0]==rhs.LongInteger[0]
			&& LongInteger[1]==rhs.LongInteger[1])
			{
			size_t i = ArraySize(LongInteger);
			while(1< --i)
				if (LongInteger[i]!=rhs.LongInteger[i])
					return false;
			return true;
			};
		};
	if (   Multiplication==Operation
		&& 0!=VFT2Idx
		&& 0!=rhs.VFT2Idx)
		{
		if      (    LargeIntIdx_VFT2+1==VFT2Idx+rhs.VFT2Idx
				 || -LargeIntIdx_VFT2-1==VFT2Idx+rhs.VFT2Idx)
			{
			if      (Small_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
				return ShortInteger==rhs.ShortInteger;
			else if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
				{
				size_t i = ArraySize(LongInteger);
				do	{
					i--;
					if (LongInteger[i]!=rhs.LongInteger[i])
						return false;
					}
				while(0<i);
				return true;
				}
			}
		// Trap +-1 as a special case
		else if (   (    SmallIntIdx_VFT2==VFT2Idx
				 	 &&  SmallIntIdx_VFT2==rhs.VFT2Idx)
				 || (   -SmallIntIdx_VFT2==VFT2Idx
				 	 && -SmallIntIdx_VFT2==rhs.VFT2Idx))
			return 1==ShortInteger && 1==rhs.ShortInteger;
		};
	return false;
}

bool _IntegerNumeral::NonTrivialGCF(const _IntegerNumeral& rhs) const
{	//! \todo finish implementing
	// Zeros are definition anomalies
	if (ZeroIdx_VFT2==VFT2Idx || ZeroIdx_VFT2==VFT2Idx) return false;
	if 		(Small_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		{
		if 		(Small_INT==IntegerRAMForm[rhs.VFT2Idx+ZeroOffset_VFT2])
			return 1<GCF_machine(ShortInteger,rhs.ShortInteger);
		assert(Large_INT==IntegerRAMForm[rhs.VFT2Idx+ZeroOffset_VFT2]);		
//		else if (Large_INT==IntegerRAMForm[rhs.VFT2Idx+ZeroOffset_VFT2])
			return 1<GCF_machine(ShortInteger,rhs.RemainderOfLargeDivideByN(ShortInteger));
		}
	assert(Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2]);
//	else if (Large_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		{
		if (Small_INT==IntegerRAMForm[rhs.VFT2Idx+ZeroOffset_VFT2])
			return 1<GCF_machine(rhs.ShortInteger,RemainderOfLargeDivideByN(rhs.ShortInteger));
		assert(Large_INT==IntegerRAMForm[rhs.VFT2Idx+ZeroOffset_VFT2]);
//		else if (Large_INT==IntegerRAMForm[rhs.VFT2Idx+ZeroOffset_VFT2])
			{
			unsigned long LMod10 = LongInteger[0]%10;
			unsigned long RMod10 = rhs.LongInteger[0]%10;
			if (   (0==LMod10%2 && 0==RMod10%2)
				|| (0==LMod10%5 && 0==RMod10%5))
				return true;
//!			Conservative option: scan for all primes < 10^9.  Any prime that detects in both at 
//!			all is nontrivial.
//!			FALLBACK ALGORITHM: uses RAM
//!			Option #1: crunch it
//!			\todo: reimplement this
			_IntegerNumeral** GCFArgArray = _new_buffer<_IntegerNumeral*>(2);
			if (!GCFArgArray) return true;	// false positive
			try	{
				GCFArgArray[0] = new _IntegerNumeral(*this);
				if (!GCFArgArray[0]->in_Z()) GCFArgArray[0]->SelfInverse(Multiplication);
				GCFArgArray[1] = new _IntegerNumeral(rhs);
				if (!GCFArgArray[1]->in_Z()) GCFArgArray[1]->SelfInverse(Multiplication);
				}
			catch(const bad_alloc&)
				{
				BLOCKDELETEARRAY_AND_NULL(GCFArgArray);
				return true;	// false positive
				}
			return !_GCF(GCFArgArray).IsOne();
//!			Option #2: This requires the ability to manipulate sets; small integers could be hard-coded
//!			Possible prime common factors = [2...FLOOR(SQRT(min(LHS,RHS)))] in _Z_
//!			Since we're here, we can immediately ignore multiples of 2,3,5,7,11,13,19,37,52579,333667 (apply this as a prefilter when expanding)
//!			After that...do an Erasthones sieve
#if 0
#endif			
			}
		}
	return false;
}

bool _IntegerNumeral::ResetLHSRHSToGCF(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2001
	// NOTE: This is not intended to use dynamic allocation of RAM.
	// NOTE: this is called from the _GCF class, when evaluating.  We only
	// need to handle non-negative integral arguments.  Zeros and ones should already have been cleared.
	assert(in_Z());
	assert(rhs.in_Z());
	assert(0<VFT2Idx);
	assert(0<rhs.VFT2Idx);
	if (LargeIntIdx_VFT2==VFT2Idx && LargeIntIdx_VFT2==rhs.VFT2Idx)
		{
		LHSRHSLargeRemoveVisibleNonCommonFactors(rhs);
		if (*this==rhs) return true;
		while(LargeIntIdx_VFT2==VFT2Idx && LargeIntIdx_VFT2==rhs.VFT2Idx)
			{
			if (*this<rhs)
				rhs.LargeAdjustAbsValByLarge(*this);
			else	// if (*this>rhs)
				LargeAdjustAbsValByLarge(rhs);
			LHSRHSLargeRemoveVisibleNonCommonFactors(rhs);
			if (*this==rhs) return true;
			}
		}
	if 		(SmallIntIdx_VFT2==VFT2Idx)
		{
		if 		(SmallIntIdx_VFT2==rhs.VFT2Idx)
			{
			ShortInteger = GCF_machine(ShortInteger,rhs.ShortInteger);
			rhs.ShortInteger = ShortInteger;
			return true;
			}
		assert(LargeIntIdx_VFT2==rhs.VFT2Idx);
//		else if (LargeIntIdx_VFT2==rhs.VFT2Idx)
			{
			ShortInteger = GCF_machine(ShortInteger,rhs.RemainderOfLargeDivideByN(ShortInteger));
			free(rhs.LongInteger);
			rhs.ShortInteger = ShortInteger;
			rhs.VFT2Idx = SmallIntIdx_VFT2;
			return true;
			}
		}
	assert(LargeIntIdx_VFT2==VFT2Idx);
//	else if (LargeIntIdx_VFT2==VFT2Idx)
		{
		if 		(SmallIntIdx_VFT2==rhs.VFT2Idx)
			{
			rhs.ShortInteger = GCF_machine(rhs.ShortInteger,RemainderOfLargeDivideByN(rhs.ShortInteger));
			free(LongInteger);
			ShortInteger = rhs.ShortInteger;
			VFT2Idx = SmallIntIdx_VFT2;
			return true;
			}
		}
	return false;
}

void
_IntegerNumeral::LHSRHSLargeRemoveVisibleNonCommonFactors(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// cf. divisibility tests, above
	//! \todo may need to completely rewrite this (*any* prime < 10^9 is RAM-conservatively visible!)
	unsigned long LHSNonCommonFactor = 1;
	unsigned long RHSNonCommonFactor = 1;
	do	{
		size_t High_2_5_Test = (ArraySize(LongInteger)<=ArraySize(rhs.LongInteger)) ? ArraySize(LongInteger)-1 : ArraySize(rhs.LongInteger)-1;
		size_t SweepIdx = 0;
		while(High_2_5_Test>SweepIdx && 0==LongInteger[SweepIdx] && 0==rhs.LongInteger[SweepIdx]) SweepIdx++;
		LHSNonCommonFactor = 1;
		RHSNonCommonFactor = 1;
		// powers of 2
		unsigned long PowerSweep = 512;
		unsigned long LHSTwoImage = LongInteger[SweepIdx];
		unsigned long RHSTwoImage = LongInteger[SweepIdx];
		do	{
			LHSTwoImage %= PowerSweep;
			RHSTwoImage %= PowerSweep;
			if (0==LHSTwoImage)
				{
				while(0!=RHSTwoImage)
					{
					LHSNonCommonFactor *= 2;
					PowerSweep /=2;
					RHSTwoImage %= PowerSweep;
					}
				break;
				}
			if (0==RHSTwoImage)
				{
				while(0!=LHSTwoImage)
					{
					RHSNonCommonFactor *= 2;
					PowerSweep /=2;
					LHSTwoImage %= PowerSweep;
					}
				break;
				}
			PowerSweep /=2;
			}
		while(1<PowerSweep);
		// powers of 5
		PowerSweep = 1953125;
		LHSTwoImage = LongInteger[SweepIdx];
		RHSTwoImage = LongInteger[SweepIdx];
		do	{
			LHSTwoImage %= PowerSweep;
			RHSTwoImage %= PowerSweep;
			if (0==LHSTwoImage)
				{
				while(0!=RHSTwoImage)
					{
					LHSNonCommonFactor *= 5;
					PowerSweep /=5;
					RHSTwoImage %= PowerSweep;
					}
				break;
				}
			if (0==RHSTwoImage)
				{
				while(0!=LHSTwoImage)
					{
					RHSNonCommonFactor *= 5;
					PowerSweep /=5;
					LHSTwoImage %= PowerSweep;
					}
				break;
				}
			PowerSweep /=5;
			}
		while(1<PowerSweep);

		if (1<LHSNonCommonFactor)
			LargeDivideByN((WRAPLB>LHSNonCommonFactor) ? LHSNonCommonFactor : LHSNonCommonFactor/2);
		if (1<RHSNonCommonFactor)
			rhs.LargeDivideByN((WRAPLB>RHSNonCommonFactor) ? RHSNonCommonFactor : RHSNonCommonFactor/2);
		}
	while(1<LHSNonCommonFactor || 1<RHSNonCommonFactor);
}

// static functions
bool _IntegerNumeral::IsLegalIntegerString(const char* x)
{	// FORMALLY CORRECT: 2/1/2000, Kenneth Boyd
	assert(x);
	size_t i = strlen(x);
	if (0<i)
		{
		while(0<--i)
			if (!zaimoni::in_range<'0','9'>((unsigned char)x[i]))
				return false;
		return zaimoni::in_range<'0','9'>((unsigned char)x[0]);
		};
	return false;
}

size_t _IntegerNumeral::LengthOfLegalIntegerSubstring(const char* x)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/1/2000
	assert(x);
	const size_t x_strlen = strlen(x);
	size_t i = 0;
	while(i<x_strlen && zaimoni::in_range<'0','9'>((unsigned char)x[i])) ++i;
	if (i<x_strlen) return i;
	return 0;
}

#if 0
// At one point, a *very* bad conceptualization of the combinatorial permutation-count function
// thought we wanted this to do an inplace-string subtraction when rendering a series product
// as a PERM(n,k) instance.  Unfortunately, it's too expensive to compute the correct lower bound
// on-the-fly for textual rendering.

// If it should be useful to actually use this function: it's a static member function.
void
_IntegerNumeral::StringReduceAbsValByDigit(char* Name, signed long SafeLength, signed int Digit)
{	
	SUCCEED_OR_DIE(NULL!=Name);
	SUCCEED_OR_DIE(0<SafeLength);
	SUCCEED_OR_DIE(zaimoni::in_range<'0','9'>(Digit));
	if (0==Digit)
		return;
	signed int LocalDigit = (signed int)(Name[0])-(signed int)('0');
	if (1==SafeLength)
		{
		SUCCEED_OR_DIE(LocalDigit<Digit);
		Name[0] = char(Digit-LocalDigit+(signed int)('0'));
		return;
		};
	do	{
		if (LocalDigit>=Digit)
			{
			Name[0] = (char)(Digit-LocalDigit+(signed int)('0'));
			return;
			};
		Name[0] = (char)(10+Digit-LocalDigit+(signed int)('0'));
		Name--;
		LocalDigit = 1;
		Digit = (signed int)(Name[0])-(signed int)('0');
		}
	while(1<--SafeLength);
	SUCCEED_OR_DIE(LocalDigit>Digit);
	if (LocalDigit<Digit)
		Name[0] = (char)(Digit-LocalDigit+(signed int)('0'));
	else
		Name[0] = ' ';
}
#endif

bool _IntegerNumeral::operator<(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2000
	if (VFT2Idx==rhs.VFT2Idx)
		return (this->*OpLT_SameTypeClassify[VFT2Idx+ZeroOffset_VFT2])(rhs);
	return VFT2Idx<rhs.VFT2Idx;
}

bool _IntegerNumeral::OpLT_TrivialFalse(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/6/1999
	return false;
}

bool _IntegerNumeral::OpLT_1st2ndPositiveInt(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2000
	if 		(ArraySize(LongInteger)<ArraySize(rhs.LongInteger)) return true;
	else if (ArraySize(LongInteger)>ArraySize(rhs.LongInteger)) return false;
	{
	size_t i = ArraySize(LongInteger);
	do	{
		--i;
		if (LongInteger[i]<rhs.LongInteger[i]) return true;
		}
	while(0<i);
	return false;
	}
}

bool _IntegerNumeral::OpLT_1st2ndSmallPositiveInt(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/6/1999
	return ShortInteger<rhs.ShortInteger;
}

bool _IntegerNumeral::OpLT_1st2ndSmallNegativeInt(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/6/1999
	return ShortInteger>rhs.ShortInteger;
}

bool _IntegerNumeral::OpLT_1st2ndNegativeInt(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2000
	if 		(ArraySize(LongInteger)>ArraySize(rhs.LongInteger)) return true;
	else if (ArraySize(LongInteger)<ArraySize(rhs.LongInteger)) return false;

	size_t i = ArraySize(LongInteger);
	do	{
		--i;
		if (LongInteger[i]>rhs.LongInteger[i]) return true;
		}
	while(0<i);
	return false;
}

bool _IntegerNumeral::AsSignedLong(signed long& Result) const
{	//! \todo OPTIMIZE
	if (-SmallIntIdx_VFT2==VFT2Idx)
		{
		Result = -ShortInteger;
		return true;
		}
	if ( SmallIntIdx_VFT2==VFT2Idx)
		{
		Result = ShortInteger;
		return true;
		}
	if (-LargeIntIdx_VFT2==VFT2Idx)
		{
		if (   2==ArraySize(LongInteger)
			&& 2>=LongInteger[1]
			&& LONG_MAX-LongInteger[1]*WRAPLB>=LongInteger[0])
			{
			Result = -(LongInteger[1]*WRAPLB+LongInteger[0]);
			return true;
			}
		return false;
		}
	if ( LargeIntIdx_VFT2==VFT2Idx)
		{
		if (   2==ArraySize(LongInteger)
			&& 2>=LongInteger[1]
			&& LONG_MAX-LongInteger[1]*WRAPLB>=LongInteger[0])
			{
			Result = LongInteger[1]*WRAPLB+LongInteger[0];
			return true;
			}
		return false;
		}				
	if (ZeroIdx_VFT2==VFT2Idx)
		{
		Result = 0;
		return true;
		}
	return false;
}

bool
_IntegerNumeral::SmallDifference(const _IntegerNumeral& rhs, signed long& Result) const
{	//! \todo OPTIMIZE
	if (ZeroIdx_VFT2==VFT2Idx)
		{
		if (rhs.AsSignedLong(Result))
			{
			Result = -Result;
			return true;
			}
		return false;
		}
	if (ZeroIdx_VFT2==rhs.VFT2Idx) return AsSignedLong(Result);
	if (*this==rhs)
		{
		Result = 0;
		return true;
		};
	if (    LargeIntInvIdx_VFT2!=VFT2Idx
		&& -LargeIntInvIdx_VFT2!=VFT2Idx
		&&  LargeIntInvIdx_VFT2!=rhs.VFT2Idx
		&& -LargeIntInvIdx_VFT2!=rhs.VFT2Idx)
		{
		if (    SmallIntInvIdx_VFT2==VFT2Idx
		    || -SmallIntInvIdx_VFT2==VFT2Idx)
			{
			if (   2==ShortInteger
				&& VFT2Idx==-rhs.VFT2Idx
				&& 2==rhs.ShortInteger)
				{
				Result = (SmallIntInvIdx_VFT2==VFT2Idx) ? 1 : -1;
				return true;
				}
			return false;
			};
		if (    SmallIntInvIdx_VFT2!=rhs.VFT2Idx
		    && -SmallIntInvIdx_VFT2!=rhs.VFT2Idx)
			{
			if ( SmallIntIdx_VFT2==VFT2Idx)
				{
				if ( SmallIntIdx_VFT2==rhs.VFT2Idx)
					{
					Result = (signed long)ShortInteger-(signed long)rhs.ShortInteger;
					return true;
					};
				if (-SmallIntIdx_VFT2==rhs.VFT2Idx)
					{
					Result = ShortInteger+rhs.ShortInteger;
					return true;
					};				
				// at this point, it's large
				if ( LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					if (   2==ArraySize(rhs.LongInteger)
						&& 3>=rhs.LongInteger[1]
						&& LONG_MAX-(rhs.LongInteger[1]-1)*WRAPLB>=rhs.LongInteger[0]+(WRAPLB-ShortInteger));
						{
						Result = rhs.LongInteger[1]*WRAPLB+rhs.LongInteger[0]-ShortInteger;
						return true;
						};
					return false;
					};
				if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					if (   2==ArraySize(rhs.LongInteger)
						&& 2>=rhs.LongInteger[1]
						&& LONG_MAX-rhs.LongInteger[1]*WRAPLB>ShortInteger+rhs.LongInteger[0])
						{
						Result = rhs.LongInteger[1]*WRAPLB+ShortInteger+rhs.LongInteger[0];
						return true;
						}
					return false;
					};
				}
			if (-SmallIntIdx_VFT2==VFT2Idx)
				{
				if (-SmallIntIdx_VFT2==rhs.VFT2Idx)
					{
					Result = (signed long)rhs.ShortInteger-(signed long)ShortInteger;
					return true;
					};
				if ( SmallIntIdx_VFT2==rhs.VFT2Idx)
					{
					Result = -(signed long)(ShortInteger+rhs.ShortInteger);
					return true;
					};				
				// at this point, it's large
				if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					if (   2==ArraySize(rhs.LongInteger)
						&& 3>=rhs.LongInteger[1]
						&& LONG_MAX-(rhs.LongInteger[1]-1)*WRAPLB>=rhs.LongInteger[0]+(WRAPLB-ShortInteger));
						{
						Result = -(signed long)(rhs.LongInteger[1]*WRAPLB+rhs.LongInteger[0]-ShortInteger);
						return true;
						};
					return false;
					};
				if ( LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					if (   2==ArraySize(rhs.LongInteger)
						&& 2>=rhs.LongInteger[1]
						&& LONG_MAX-rhs.LongInteger[1]*WRAPLB>ShortInteger+rhs.LongInteger[0])
						{
						Result = -(signed long)(rhs.LongInteger[1]*WRAPLB+ShortInteger+rhs.LongInteger[0]);
						return true;
						}
					return false;
					};
				}
			if ( SmallIntIdx_VFT2==rhs.VFT2Idx)
				{
				// at this point, it's large
				if ( LargeIntIdx_VFT2==VFT2Idx)
					{
					if (   2==ArraySize(LongInteger)
						&& 3>=LongInteger[1]
						&& LONG_MAX-(LongInteger[1]-1)*WRAPLB>=LongInteger[0]+(WRAPLB-ShortInteger));
						{
						Result = -(signed long)(LongInteger[1]*WRAPLB+LongInteger[0]-ShortInteger);
						return true;
						};
					return false;
					};
				if (-LargeIntIdx_VFT2==VFT2Idx)
					{
					if (   2==ArraySize(LongInteger)
						&& 2>=LongInteger[1]
						&& LONG_MAX-LongInteger[1]*WRAPLB>ShortInteger+LongInteger[0])
						{
						Result = -(signed long)(LongInteger[1]*WRAPLB+ShortInteger+LongInteger[0]);
						return true;
						}
					return false;
					};
				}
			if (-SmallIntIdx_VFT2==rhs.VFT2Idx)
				{
				// at this point, it's large
				if (-LargeIntIdx_VFT2==VFT2Idx)
					{
					if (   2==ArraySize(LongInteger)
						&& 3>=LongInteger[1]
						&& LONG_MAX-(LongInteger[1]-1)*WRAPLB>=LongInteger[0]+(WRAPLB-ShortInteger));
						{
						Result = LongInteger[1]*WRAPLB+LongInteger[0]-ShortInteger;
						return true;
						};
					return false;
					};
				if ( LargeIntIdx_VFT2==VFT2Idx)
					{
					if (   2==ArraySize(LongInteger)
						&& 2>=LongInteger[1]
						&& LONG_MAX-LongInteger[1]*WRAPLB>ShortInteger+LongInteger[0])
						{
						Result = LongInteger[1]*WRAPLB+ShortInteger+LongInteger[0];
						return true;
						}
					return false;
					};
				}
			if ( LargeIntIdx_VFT2==VFT2Idx)
				{
				if ( LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					_IntegerNumeral LHSMirror = *this;
					_IntegerNumeral RHSMirror = rhs;
					RHSMirror.SelfInverse(Addition);
					LHSMirror.RearrangeSum(RHSMirror);
					if (RHSMirror.IsZero())
						return LHSMirror.AsSignedLong(Result);
					else{	// if LHSMirror.IsZero()
						RHSMirror.SelfInverse(Addition);
						return RHSMirror.AsSignedLong(Result);
						};
					}
				if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					if (   2==ArraySize(LongInteger)
						&& 2==ArraySize(rhs.LongInteger)
						&& 2>=LongInteger[1]+rhs.LongInteger[1]
						&& LONG_MAX-(LongInteger[1]+rhs.LongInteger[1])*WRAPLB>=LongInteger[0]+rhs.LongInteger[0])
						{
						Result = (LongInteger[1]+rhs.LongInteger[1])*WRAPLB+LongInteger[0]+rhs.LongInteger[0];
						return true;
						}
					return false;
					}
				}
			if (-LargeIntIdx_VFT2==VFT2Idx)
				{
				if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					_IntegerNumeral LHSMirror = *this;
					_IntegerNumeral RHSMirror = rhs;
					RHSMirror.SelfInverse(Addition);
					LHSMirror.RearrangeSum(RHSMirror);
					if (RHSMirror.IsZero())
						return LHSMirror.AsSignedLong(Result);
					assert(LHSMirror.IsZero());
					RHSMirror.SelfInverse(Addition);
					return RHSMirror.AsSignedLong(Result);
					}
				if ( LargeIntIdx_VFT2==rhs.VFT2Idx)
					{
					if (   2==ArraySize(LongInteger)
						&& 2==ArraySize(rhs.LongInteger)
						&& 2>=LongInteger[1]+rhs.LongInteger[1]
						&& LONG_MAX-(LongInteger[1]+rhs.LongInteger[1])*WRAPLB>=LongInteger[0]+rhs.LongInteger[0])
						{
						Result = -(signed long)((LongInteger[1]+rhs.LongInteger[1])*WRAPLB+LongInteger[0]+rhs.LongInteger[0]);
						return true;
						}
					return false;
					}
				}
			}
		}
	return false;
}

bool _IntegerNumeral::ReduceAbsValByN(unsigned long N)
{
	if (SmallIntInvIdx_VFT2>=VFT2Idx && -SmallIntInvIdx_VFT2<=VFT2Idx)
		return false;
	const size_t RAMMode = IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2];
	if (Large_INT==RAMMode)
		{
		ReduceLargeLHSAbsValByN(N);
		return true;
		}
	else if (Small_INT==RAMMode && N<=ShortInteger)
		{
		if (N==ShortInteger)
			{
			VFT2Idx = ZeroIdx_VFT2;
			return true;
			}
		else{
			ShortInteger -= N;
			return true;
			}
		}
	else{	// hardcoded
			// fix this when numerals other than 0 are hard-coded
		return false;
		}

}

// RearrangeSum specification:
// 1+ args 0: return immediately
// 1 pos, 1 neg: decide which has higher absolute value, do absval subtraction on 
// larger, set smaller to zero
// 2 pos, or 2 neg: decide which has higher absolute value;
void _IntegerNumeral::RearrangeSum(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/7/2001
	// try to slide smaller into larger; do *not* try to allocate memory if it doesn't
	// completely slide.

	// 1+ args 0: return immediately
	if (ZeroIdx_VFT2==VFT2Idx || ZeroIdx_VFT2==rhs.VFT2Idx) return;

	// AddInv trap
	//! \todo condense relatively invariant code here from relevant cases
	if (VFT2Idx==-rhs.VFT2Idx)
		{
		const size_t RAMMode = IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2];
		if		(Large_INT==RAMMode)
			{
			if (LHSRHSLargeEqualAbsVal(rhs))
				{
				free(LongInteger);
				VFT2Idx = ZeroIdx_VFT2;
				free(rhs.LongInteger);
				rhs.VFT2Idx = ZeroIdx_VFT2;
				return;
				}
			if (LargeIntIdx_VFT2==VFT2Idx || -LargeIntIdx_VFT2==VFT2Idx)
				{
				LHSRHSLargeOppSignRearrange(rhs);
				return;
				}
			}
		else if (Small_INT==RAMMode)
			{
			if (ShortInteger==rhs.ShortInteger)
				{
				VFT2Idx = ZeroIdx_VFT2;
				rhs.VFT2Idx = ZeroIdx_VFT2;
				return;
				}
			if (SmallIntIdx_VFT2==VFT2Idx || -SmallIntIdx_VFT2==VFT2Idx)
				{
				LHSRHSSmallOppSignRearrange(rhs);
				return;
				}
			assert(SmallIntInvIdx_VFT2==VFT2Idx || -SmallIntInvIdx_VFT2==VFT2Idx);
			size_t LocalGCF = GCF_machine(ShortInteger,rhs.ShortInteger);
			size_t LHSMirror = ShortInteger/LocalGCF;
			size_t RHSMirror = rhs.ShortInteger/LocalGCF;
			if (LHSMirror>RHSMirror)
				{	// LHS +/- larger denominator than RHS -/+
					// LHS - larger denominator than RHS +
				if (   1==LHSMirror-RHSMirror
					&& (WRAPLB-1)/LHSMirror>=rhs.ShortInteger)
					{
					rhs.ShortInteger *= LHSMirror;
					VFT2Idx = ZeroIdx_VFT2;
					return;
					}
				if (RHSMirror==1 && LHSMirror-1==rhs.ShortInteger)
					{
					rhs.ShortInteger++;
					VFT2Idx = ZeroIdx_VFT2;
					return;
					}
				}
			else{	// LHS +/- smaller denominator than RHS -/+
				if (   1==RHSMirror-LHSMirror
					&& (WRAPLB-1)/RHSMirror>=ShortInteger)
					{
					ShortInteger *= RHSMirror;
					rhs.VFT2Idx = ZeroIdx_VFT2;
					return;
					}
				if (LHSMirror==1 && RHSMirror-1==ShortInteger)
					{
					ShortInteger++;
					rhs.VFT2Idx = ZeroIdx_VFT2;
					return;
					}
				}			
			return;
			}
		assert(HardCoded_INT==RAMMode);
		VFT2Idx = ZeroIdx_VFT2;
		rhs.VFT2Idx = ZeroIdx_VFT2;
		return;
		}

	//	LHSRHSSmallOppSignRearrange
	// 1 pos, 1 neg: decide which has higher absolute value, do absval subtraction on 
	// larger, set smaller to zero
	// 2 pos, or 2 neg: decide which has higher absolute value;
	if 		( LargeIntIdx_VFT2==VFT2Idx)
		{
		if      ( LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS large +, RHS large +
			LHSRHSLargeSameSignCondense(rhs);
			return;
			}
		else if ( SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS large +, RHS small +
			LHSLargeRHSSmallSameSignCondense(rhs);
			return;
			}
		else if (-SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS large +, RHS small -
			LHSLargeRHSSmallOppSignRearrange(rhs);
			return;
			}
		// NOTE: LHS large +, RHS large - caught in AddInv code
		// use  -1/2 to ratchet a positive integer down by 1 (resulting in +1/2)
		else if (-SmallIntInvIdx_VFT2==rhs.VFT2Idx && 2==rhs.ShortInteger)
			{
			rhs.VFT2Idx = SmallIntInvIdx_VFT2;
			ReduceLargeLHSAbsValByN(1);
			return;
			}
		return;
		}
	else if ( SmallIntIdx_VFT2==VFT2Idx)
		{
		if      ( LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS small +, RHS large +
			rhs.LHSLargeRHSSmallSameSignCondense(*this);
			return;
			}
		else if ( SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS small +, RHS small +
			LHSRHSSmallSameSignCondense(rhs);
			return;
			}
		// NOTE: LHS small +, RHS small - caught in AddInv code
		else if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS small +, RHS large -
			rhs.LHSLargeRHSSmallOppSignRearrange(*this);
			return;
			}
		// use  -1/2 to ratchet a positive integer down by 1 (resulting in +1/2)
		else if (-SmallIntInvIdx_VFT2==rhs.VFT2Idx && 2==rhs.ShortInteger)
			{
			rhs.VFT2Idx = SmallIntInvIdx_VFT2;
			if (0== --ShortInteger) VFT2Idx = ZeroIdx_VFT2;
			return;
			}
		return;
		}
	// GENERAL PROBLEM WITH 1/Integer: need to do only those that "close" (general form is a StdMultiplication object!)
	// So: 1/a-1/b, a<b Integer (positive)
	// need  b-a to divide a*b
	// b-a=1: OK, do a*b to complete
	// b-a=0: cancels out!
	// b-a>1 and GCF(a,b)=1: ??
	// for 1/a+1/b: need a+b to divide a*b
	// * a==b: natural result is 2/a, check for a even
	// * a<b, GCF(a,b)=1: check for b=a(a-1); then result is 1/(a-1)
	// ** otherwise: ???
	// dualize this for negative version
	// NOTE: for conservative rearrange, we never change mode to LargeIntInv
	else if ( SmallIntInvIdx_VFT2==VFT2Idx)
		{
		// use 1/2 to ratchet down the absolute value of a negative integer by 1 (turns to -1/2)
		// WANT: decrement absval for Small, Large _IntegerNumerals
		if (2==ShortInteger)
			{
			if      (-SmallIntIdx_VFT2==rhs.VFT2Idx)
				{
				VFT2Idx = -SmallIntInvIdx_VFT2;
				if (0== --rhs.ShortInteger)
					rhs.VFT2Idx = ZeroIdx_VFT2;
				return;
				}
			else if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
				{
				VFT2Idx = -SmallIntInvIdx_VFT2;
				rhs.ReduceLargeLHSAbsValByN(1);
				return;
				}
			};
		
		// others will be same-mode for result
		// need GCF to make this work
		if 		( SmallIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// same-sign
			if (ShortInteger==rhs.ShortInteger)
				{
				if (0==ShortInteger%2)
					{
					VFT2Idx = ZeroIdx_VFT2;
					rhs.ShortInteger >>= 1;	// divide by 2
					if (1==rhs.ShortInteger)
						rhs.VFT2Idx = SmallIntIdx_VFT2;
					return;
					}
				return;
				}
			size_t LocalGCF = GCF_machine(ShortInteger,rhs.ShortInteger);
			size_t LHSMirror = ShortInteger/LocalGCF;
			size_t RHSMirror = rhs.ShortInteger/LocalGCF;
			if (LHSMirror>RHSMirror)
				{
				if (LHSMirror==RHSMirror*(RHSMirror-1))
					{
					VFT2Idx = ZeroIdx_VFT2;
					rhs.ShortInteger -= LocalGCF;
					if (1==rhs.ShortInteger)
						rhs.VFT2Idx = SmallIntIdx_VFT2;
					return;
					}
				if (1==RHSMirror && (WRAPLB-1)>LHSMirror && 0==LocalGCF%(LHSMirror+1))
					{
					VFT2Idx = ZeroIdx_VFT2;
					rhs.ShortInteger = LHSMirror*(LocalGCF/(LHSMirror+1));
					return;
					}
				}
			else{	// LHSMirror<RHSMirror
				if (RHSMirror==LHSMirror*(LHSMirror-1))
					{
					rhs.VFT2Idx = ZeroIdx_VFT2;
					ShortInteger -= LocalGCF;
					if (1==ShortInteger)
						VFT2Idx = SmallIntIdx_VFT2;
					return;
					}
				if (1==LHSMirror && (WRAPLB-1)>RHSMirror && 0==LocalGCF%(RHSMirror+1))
					{
					rhs.VFT2Idx = ZeroIdx_VFT2;
					ShortInteger = RHSMirror*(LocalGCF/(RHSMirror+1));
					return;
					}
				}
			return;
			}
		// NOTE: LHS SmallIntInvIdx_VFT, RHS -SmallIntInvIdx_VFT2 cleared by AddInv code
		return;
		}
	else if ( LargeIntInvIdx_VFT2==VFT2Idx)
		{	// others will be same-mode for result
		// Additive inverse has already been cleared
		if 		( LargeIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// same-sign, both large: check for == case (cannot test anything else conservatively)
			if (   LHSRHSLargeEqualAbsVal(rhs)
				&& 0==LongInteger[0]%2)
				{
				free(rhs.LongInteger);
				rhs.VFT2Idx = ZeroIdx_VFT2;
				LargeSelfDivBy2();
				if (1==ArraySize(LongInteger))
					{
					unsigned long Tmp = LongInteger[0];
					free(LongInteger);
					VFT2Idx = SmallIntInvIdx_VFT2;
					ShortInteger = Tmp;
					}
				}
			return;
			}
#if 0
		else if (2==ArraySize(LongInteger))
			{
			if (SmallIntInvIdx==rhs.VFT2Idx)
				{	// same-sign
				// ...
				}
			}
#endif
		return;
		}
	else if (-LargeIntInvIdx_VFT2==VFT2Idx)
		{	// others will be same-mode for result
		// Additive inverse has already been cleared
		if (-LargeIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// same-sign, both large: check for == case (cannot test anything else conservatively)
			if (   LHSRHSLargeEqualAbsVal(rhs)
				&& 0==LongInteger[0]%2)
				{
				free(rhs.LongInteger);
				rhs.VFT2Idx = ZeroIdx_VFT2;
				LargeSelfDivBy2();
				if (1==ArraySize(LongInteger))
					{
					unsigned long Tmp = LongInteger[0];
					free(LongInteger);
					VFT2Idx = -SmallIntInvIdx_VFT2;
					ShortInteger = Tmp;
					}
				}
			return;
			}
#if 0
		else if (2==ArraySize(LongInteger))
			{
			if (-SmallIntInvIdx==rhs.VFT2Idx)
				{	// same-sign
				// ...
				}
			}
#endif
		return;
		}
	else if (-SmallIntInvIdx_VFT2==VFT2Idx)
		{	// use  -1/2 to ratchet a positive integer down by 1 (resulting in +1/2)
		if (2==ShortInteger)
			{
			if      (SmallIntIdx_VFT2==rhs.VFT2Idx)
				{
				VFT2Idx = SmallIntInvIdx_VFT2;
				if (0== --rhs.ShortInteger)
					rhs.VFT2Idx = ZeroIdx_VFT2;
				return;
				}
			else if (LargeIntIdx_VFT2==rhs.VFT2Idx)
				{
				VFT2Idx = SmallIntInvIdx_VFT2;
				rhs.ReduceLargeLHSAbsValByN(1);
				return;
				}
			};
		
		// others will be same-mode for result
		// need GCF to make this work
		// NOTE: LHS -SmallIntInvIdx_VFT, RHS SmallIntInvIdx_VFT2 cleared by AddInv code
		if (-SmallIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// same-sign
			if (ShortInteger==rhs.ShortInteger)
				{
				if (0==ShortInteger%2)
					{
					VFT2Idx = ZeroIdx_VFT2;
					rhs.ShortInteger >>= 1;	// divide by 2
					if (1==rhs.ShortInteger)
						rhs.VFT2Idx = -SmallIntIdx_VFT2;
					return;
					}
				return;
				}
			size_t LocalGCF = GCF_machine(ShortInteger,rhs.ShortInteger);
			size_t LHSMirror = ShortInteger/LocalGCF;
			size_t RHSMirror = rhs.ShortInteger/LocalGCF;
			if (LHSMirror>RHSMirror)
				{
				if (LHSMirror==RHSMirror*(RHSMirror-1))
					{
					VFT2Idx = ZeroIdx_VFT2;
					rhs.ShortInteger -= LocalGCF;
					if (1==rhs.ShortInteger)
						rhs.VFT2Idx = -SmallIntIdx_VFT2;
					return;
					}
				if (1==RHSMirror && (WRAPLB-1)>LHSMirror && 0==LocalGCF%(LHSMirror+1))
					{
					VFT2Idx = ZeroIdx_VFT2;
					rhs.ShortInteger = LHSMirror*(LocalGCF/(LHSMirror+1));
					return;
					}
				}
			else{	// LHSMirror<RHSMirror
				if (RHSMirror==LHSMirror*(LHSMirror-1))
					{
					rhs.VFT2Idx = ZeroIdx_VFT2;
					ShortInteger -= LocalGCF;
					if (1==ShortInteger)
						VFT2Idx = -SmallIntIdx_VFT2;
					return;
					}
				if (1==LHSMirror && (WRAPLB-1)>RHSMirror && 0==LocalGCF%(RHSMirror+1))
					{
					rhs.VFT2Idx = ZeroIdx_VFT2;
					ShortInteger = RHSMirror*(LocalGCF/(RHSMirror+1));
					return;
					}
				}
			return;
			}
		return;
		}
	else if (-SmallIntIdx_VFT2==VFT2Idx)
		{
		if      ( LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS small -, RHS large +
			rhs.LHSLargeRHSSmallOppSignRearrange(*this);
			return;
			}
		// NOTE: LHS small -, RHS small + caught in AddInv code
		else if (-SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS small -, RHS small -
			LHSRHSSmallSameSignCondense(rhs);
			return;
			}
		else if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS small -, RHS large -
			rhs.LHSLargeRHSSmallSameSignCondense(*this);
			return;
			}
		// use 1/2 to ratchet down the absolute value of a negative integer by 1 (turns to -1/2)
		else if (   SmallIntInvIdx_VFT2==rhs.VFT2Idx
				 && 2==rhs.ShortInteger)
			{
			rhs.VFT2Idx = -SmallIntInvIdx_VFT2;
			if (0==--ShortInteger) VFT2Idx = ZeroIdx_VFT2;
			return;
			}
		return;
		}
	else if (-LargeIntIdx_VFT2==VFT2Idx)
		{
		// NOTE: LHS large -, RHS large + caught in AddInv code
		if		( SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS large -, RHS small +
			LHSLargeRHSSmallOppSignRearrange(rhs);
			return;
			}
		else if (-SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS large -, RHS small -
			LHSLargeRHSSmallSameSignCondense(rhs);
			return;
			}
		else if (-LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS large -, RHS large -
			LHSRHSLargeSameSignCondense(rhs);
			return;
			}
		// use 1/2 to ratchet down the absolute value of a negative integer by 1 (turns to -1/2)
		else if (   SmallIntInvIdx_VFT2==rhs.VFT2Idx
				 && 2==rhs.ShortInteger)
			{
			rhs.VFT2Idx = -SmallIntInvIdx_VFT2;
			ReduceLargeLHSAbsValByN(1);
			return;
			}
		return;
		}
}

unsigned long _IntegerNumeral::LargeSelfDivBy2()
{	// FORMALLY CORRECT: Kenneth Boyd, 8/6/2001
	// NOTE: memory-model correction is *not* handled here.
	size_t i = ArraySize(LongInteger);
	size_t Remainder = 0;
	while(0<i)
		{
		--i;
		if (Remainder) LongInteger[i] += WRAPLB;
		Remainder = LongInteger[i]%2;
		LongInteger[i] >>= 1;
		};
	if (0==LongInteger[ArraySize(LongInteger)-1])
		LongInteger = REALLOC(LongInteger,_msize(LongInteger)-sizeof(unsigned long));
	return Remainder;
}

bool _IntegerNumeral::LHSRHSLargeEqualAbsVal(const _IntegerNumeral& rhs) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/6/2001
	size_t i = ArraySize(LongInteger);
	if (i==ArraySize(rhs.LongInteger))
		{
		do	{
			--i;
			if (LongInteger[i]!=rhs.LongInteger[i]) return false;
			}
		while(0<i);
		return true;
		}
	return false;
}

void _IntegerNumeral::ReduceLargeLHSAbsValByN(unsigned long N)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/6/2001
	if (LongInteger[0]>=N)
		{
		LongInteger[0]-=N;
		return;
		};
	N-=LongInteger[0];
	LongInteger[0] = WRAPLB;
	LongInteger[0]-=N;

	size_t i = 0;
	while(0==LongInteger[ ++i]);
	if (0== --LongInteger[i])
		{
		if (1==i)
			{	// change representation: now SMALL
			N = LongInteger[0];
			free(LongInteger);
			ShortInteger = N;
			if (0<VFT2Idx)
				{	// large int|LHS +
				VFT2Idx = SmallIntIdx_VFT2;
				return;
				}
			else{	// large int|LHS -
				VFT2Idx = -SmallIntIdx_VFT2;
				return;
				};
			};
		// shrink down by 1
		LongInteger = REALLOC(LongInteger,_msize(LongInteger)-sizeof(unsigned long));
		};
	while(0< --i) LongInteger[i]=WRAPLB-1;
}

void _IntegerNumeral::LHSLargeRHSSmallOppSignRearrange(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/6/2001
	ReduceLargeLHSAbsValByN(rhs.ShortInteger);
	rhs.VFT2Idx = ZeroIdx_VFT2;
}

void _IntegerNumeral::LHSRHSLargeOppSignRearrange(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: 8/18/2001
	if 		(_msize(LongInteger)>_msize(rhs.LongInteger))
		{	// LHS has larger || than RHS
		LargeAdjustAbsValByLargeTargetHighestDifferent(rhs,ArraySize(rhs.LongInteger)-1);
		free(rhs.LongInteger);
		rhs.VFT2Idx = ZeroIdx_VFT2;
		return;
		}
	else if (_msize(LongInteger)<_msize(rhs.LongInteger))
		{	// RHS has larger || than LHS
		rhs.LargeAdjustAbsValByLargeTargetHighestDifferent(*this,ArraySize(LongInteger)-1);
		free(LongInteger);
		VFT2Idx = ZeroIdx_VFT2;
		return;
		}
	else{	// NOTE: AddInv case has already been handled.
		size_t i = ArraySize(LongInteger);
		while(--i,LongInteger[i]==rhs.LongInteger[i]) assert(0<i);
		if (LongInteger[i]>rhs.LongInteger[i])
			{	// LHS has larger || than RHS
			LargeAdjustAbsValByLargeTargetHighestDifferent(rhs,i);
			free(rhs.LongInteger);
			rhs.VFT2Idx = ZeroIdx_VFT2;
			return;
			}
		else{	// RHS has larger || than LHS
			rhs.LargeAdjustAbsValByLargeTargetHighestDifferent(*this,i);
			free(LongInteger);
			VFT2Idx = ZeroIdx_VFT2;
			return;
			};
		}
}

void _IntegerNumeral::LargeAdjustAbsValByLarge(const _IntegerNumeral& rhs)
{	// FORMALLY CORRECT: 8/19/2001
	// ASSUMPTION: LHS>RHS in absolute value
	// NOTE: proper implementation strategy may be to retain these routines as legacy,
	// factor out RHS-zero code into other routine
	if 		(_msize(LongInteger)>_msize(rhs.LongInteger))
		{	// LHS has larger || than RHS
		LargeAdjustAbsValByLargeTargetHighestDifferent(rhs,ArraySize(rhs.LongInteger)-1);
		return;
		}
	else{	// NOTE: AddInv case has already been handled.
		size_t i = ArraySize(LongInteger);
		while(--i,LongInteger[i]==rhs.LongInteger[i]) assert(0<i);
		LargeAdjustAbsValByLargeTargetHighestDifferent(rhs,i);
		return;
		}
}

static void
DigitalSubtract(unsigned long* const lhs,const unsigned long* const rhs, const size_t ub)
{
	size_t i = 0;
	do	{
		if (lhs[i]<rhs[i])
			{
			lhs[i] += WRAPLB;
			size_t j = i;
			while(0==rhs[ ++j]);
			lhs[j]--;
			while(i< --j) lhs[j]=WRAPLB-1;
			};
		lhs[i] -=rhs[i];
		}
	while(ub>= ++i);
}

void
_IntegerNumeral::LargeAdjustAbsValByLargeTargetHighestDifferent(const _IntegerNumeral& rhs, unsigned long TargetIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/22/2001
	DigitalSubtract(LongInteger,rhs.LongInteger,TargetIdx);
	size_t i = TargetIdx+1;
	while(0==LongInteger[ --i]);
	if (ArraySize(LongInteger)-1>i)
		{
		if (0<i)
			LongInteger = REALLOC(LongInteger,(i+1)*sizeof(unsigned long));
		else{	// LHS now small
			unsigned long Tmp = LongInteger[0];
			free(LongInteger);
			ShortInteger = Tmp;
			VFT2Idx = (0<VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
			}
		}
}

void _IntegerNumeral::LHSRHSSmallOppSignRearrange(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/8/2001
	if 		(ShortInteger>rhs.ShortInteger)
		{
		ShortInteger-=rhs.ShortInteger;
		rhs.VFT2Idx = ZeroIdx_VFT2;
		return;
		};
	assert(ShortInteger<rhs.ShortInteger);
	rhs.ShortInteger-=ShortInteger;
	VFT2Idx = ZeroIdx_VFT2;
}

void _IntegerNumeral::LHSRHSLargeSameSignCondense(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2000
	if 		(_msize(LongInteger)>_msize(rhs.LongInteger))
		{	// LHS clearly larger than RHS
		LHSRHSLargeSameSignCondenseLHSLonger(rhs);
		return;
		}
	else if (_msize(LongInteger)<_msize(rhs.LongInteger))
		{	// RHS clearly larger than LHS
		rhs.LHSRHSLargeSameSignCondenseLHSLonger(*this);
		return;
		}
	else{
		const size_t tmp = ArraySize(LongInteger)-1;
		if 		(WRAPLB-1>LongInteger[tmp]+rhs.LongInteger[tmp])
			{	// LHS subtly larger than RHS
			LHSRHSLargeSameSignCondenseSameLength(rhs);
			return;
			}
		else if (WRAPLB-1<LongInteger[tmp]+rhs.LongInteger[tmp])
			{	// RHS subtly larger than LHS
			rhs.LHSRHSLargeSameSignCondenseSameLength(*this);
			return;
			}
		else{	// NOTE: both leading digits are 1..999,999,998
			if (LongInteger[tmp]>=rhs.LongInteger[tmp])
				{
				LongInteger[tmp]++;
				rhs.LongInteger[tmp]--;
				LHSRHSLargeSameSignCondenseSameLength(rhs);
				return;
				}
			LongInteger[tmp]--;
			rhs.LongInteger[tmp]++;
			rhs.LHSRHSLargeSameSignCondenseSameLength(*this);
			return;
			}
		};
}

void
_IntegerNumeral::LHSRHSLargeSameSignCondenseLHSLonger(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	size_t i = ArraySize(rhs.LongInteger)-1;
WarySum:
	LongInteger[i]+=rhs.LongInteger[i];
	switch(cmp((unsigned long)(WRAPLB-1),LongInteger[i]))
	{
	case 1:
FinalSumIsSafe:
		do	{
			--i;
			LongInteger[i]+=rhs.LongInteger[i];
			if (WRAPLB<=LongInteger[i])
				{
				LongInteger[i]-=WRAPLB;
				LongInteger[i+1]++;
				};
			}
		while(0<i);
		free(rhs.LongInteger);
		rhs.VFT2Idx = ZeroIdx_VFT2;
		return;
	case 0:	// wary, until in the clear
		rhs.LongInteger[i]=0;
		if (0< --i) goto WarySum;
		LongInteger[0]+=rhs.LongInteger[0];
		if (WRAPLB>LongInteger[0])
			{
			free(rhs.LongInteger);
			rhs.VFT2Idx = ZeroIdx_VFT2;
			return;
			};
		rhs.LongInteger[0] = LongInteger[0]-(WRAPLB-1);
		LongInteger[0] = WRAPLB-1;
		rhs.LargeShrinkRepresentation(ArraySize(rhs.LongInteger)-1);
		return;
	case -1:
		if (WRAPLB-1>LongInteger[i+1])
			{
			LongInteger[i]-=WRAPLB;
			LongInteger[i+1]++;
			goto FinalSumIsSafe;
			};
		{
		size_t j = i+1;
		while(ArraySize(LongInteger)> ++j)
			if (WRAPLB-1>LongInteger[j])
				{
				LongInteger[i]-=WRAPLB;
				LongInteger[j-- ]++;
				do	LongInteger[j]=0;
				while(i< --j);
				goto FinalSumIsSafe;
				};
		}
		// everything above RHSIdx is base 10^9 999,999,999 here.
		rhs.LongInteger[i] = LongInteger[i]-(WRAPLB-1);
		LongInteger[i] = WRAPLB-1;
		do	{
			--i;
			LongInteger[i]+=rhs.LongInteger[i];
			if 		(WRAPLB-1<LongInteger[i])
				{
				rhs.LongInteger[i] = LongInteger[i]-(WRAPLB-1);
				LongInteger[i]=(WRAPLB-1);
				}
			else if (WRAPLB-1>LongInteger[i])
				{
				rhs.LongInteger[i] = 1+LongInteger[i];
				LongInteger[i] = WRAPLB-1;
				if (0<rhs.LongInteger[i+1])
					// immediate carry
					rhs.LongInteger[i+1]--;
				else{	// extended carry
					size_t j = i+1;
					while(0==rhs.LongInteger[ ++j]);
					rhs.LongInteger[j]--;
					while(i< --j)
						rhs.LongInteger[j] = WRAPLB-1;
					}
				};
			}
		while(0<i);
		rhs.LargeShrinkRepresentation(ArraySize(rhs.LongInteger)-1);
		return;
	}
}

void
_IntegerNumeral::LHSRHSLargeSameSignCondenseSameLength(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// LHS has larger absolute value than RHS
	size_t i = ArraySize(rhs.LongInteger)-1;
WarySum:
	LongInteger[i]+=rhs.LongInteger[i];
	switch(cmp((unsigned long)(WRAPLB-1),LongInteger[i]))
	{
	case 1:
		do	{
			--i;
			LongInteger[i]+=rhs.LongInteger[i];
			if (WRAPLB<=LongInteger[i])
				{
				LongInteger[i]-=WRAPLB;
				LongInteger[i+1]++;
				};
			}
		while(0<i);
		free(rhs.LongInteger);
		rhs.VFT2Idx = ZeroIdx_VFT2;
		return;
	case 0:	// wary, until in the clear
		rhs.LongInteger[i]=0;
		if (0< --i) goto WarySum;
		LongInteger[0]+=rhs.LongInteger[0];
		if (WRAPLB>LongInteger[0])
			{
			free(rhs.LongInteger);
			rhs.VFT2Idx = ZeroIdx_VFT2;
			return;
			};
		rhs.LongInteger[0] = LongInteger[0]-(WRAPLB-1);
		LongInteger[0] = WRAPLB-1;
		rhs.LargeShrinkRepresentation(ArraySize(rhs.LongInteger)-1);
		return;
	case -1:
		// everything above RHSIdx is base 10^9 999,999,999 here.
		rhs.LongInteger[i] = LongInteger[i]-(WRAPLB-1);
		LongInteger[i] = WRAPLB-1;
		do	{
			--i;
			LongInteger[i]+=rhs.LongInteger[i];
			if 		(WRAPLB-1<LongInteger[i])
				{
				rhs.LongInteger[i] = LongInteger[i]-(WRAPLB-1);
				LongInteger[i]=(WRAPLB-1);
				}
			else if (WRAPLB-1>LongInteger[i])
				{
				rhs.LongInteger[i] = 1+LongInteger[i];
				LongInteger[i] = WRAPLB-1;
				if (0<rhs.LongInteger[i+1])
					// immediate carry
					rhs.LongInteger[i+1]--;
				else{	// extended carry
					size_t j = i+1;
					while(0==rhs.LongInteger[ ++j]);
					rhs.LongInteger[j]--;
					while(i< --j)
						rhs.LongInteger[j] = WRAPLB-1;
					}
				};
			}
		while(0<i);
		rhs.LargeShrinkRepresentation(ArraySize(rhs.LongInteger)-1);
		return;
	}
}

void _IntegerNumeral::LargeShrinkRepresentation(size_t StartIdx)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2000
	// Determine actual length of RHS.LongArray
	// if 1, change representation, else resize to shorter length
	while(0==LongInteger[StartIdx])
		if (0==--StartIdx)
			{
			unsigned long Tmp = LongInteger[0];
			free(LongInteger);
			ShortInteger = Tmp;
			VFT2Idx = (0<VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
			return;
			};
	LongInteger = REALLOC(LongInteger,(StartIdx+1)*sizeof(unsigned long));
}

void _IntegerNumeral::LHSLargeRHSSmallSameSignCondense(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	LongInteger[0]+=rhs.ShortInteger;
	if 		(WRAPLB-1>=LongInteger[0])
		{	// final sum: cannot overflow into next higher up
		rhs.VFT2Idx = ZeroIdx_VFT2;
		return;
		}
	if (WRAPLB-1>LongInteger[1])
		{
		LongInteger[0]-=WRAPLB;
		LongInteger[1]++;
		rhs.VFT2Idx = ZeroIdx_VFT2;
		return;
		};
	{
	size_t i = 1;
	while(ArraySize(LongInteger)> ++i)
		if (WRAPLB-1>LongInteger[i])
			{
			LongInteger[0]-=WRAPLB;
			LongInteger[i-- ] ++;
			do	LongInteger[i]=0;
			while(0< --i);
			rhs.VFT2Idx = ZeroIdx_VFT2;
			return;
			};
	}
	// everything above RHSIdx is base 10^9 999,999,999 here.
	rhs.ShortInteger = LongInteger[0]-(WRAPLB-1);
	LongInteger[0] = WRAPLB-1;
	return;
}

bool
ZeroRHSInSmallSameSignCondense(unsigned long lhs,unsigned long rhs,signed long VFT2Idx)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/19/2000
	if (lhs==rhs) return 0>VFT2Idx;
	return lhs>rhs;
}

void _IntegerNumeral::LHSRHSSmallSameSignCondense(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/20/2000
	if (ZeroRHSInSmallSameSignCondense(ShortInteger,rhs.ShortInteger,VFT2Idx))
		{
		ShortInteger+=rhs.ShortInteger;
		if 		(WRAPLB-1>=ShortInteger)
			{	// final sum: cannot overflow into next higher up
			rhs.VFT2Idx = ZeroIdx_VFT2;
			return;
			}
		// everything above RHSIdx is base 10^9 999,999,999 here.
		rhs.ShortInteger = ShortInteger-(WRAPLB-1);
		ShortInteger = WRAPLB-1;
		return;
		}
	rhs.ShortInteger+=ShortInteger;
	if 		(WRAPLB-1>=rhs.ShortInteger)
		{	// final sum: cannot overflow into next higher up
		VFT2Idx = ZeroIdx_VFT2;
		return;
		}
	// everything above RHSIdx is base 10^9 999,999,999 here.
	ShortInteger = rhs.ShortInteger-(WRAPLB-1);
	rhs.ShortInteger = WRAPLB-1;
	return;
}

bool _IntegerNumeral::ForceRearrangeSum(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/20/2000
	// try to slide smaller into larger; *do* try to allocate memory if it doesn't
	// completely slide.
	RearrangeSum(rhs);

	// 1+ args 0: return immediately
	if (ZeroIdx_VFT2==VFT2Idx || ZeroIdx_VFT2==rhs.VFT2Idx) return true;
	//! \todo introduce handling of 1/Integer
	//! 1 pos, 1 neg: decide which has higher absolute value, do absval subtraction on 
	//! larger, set smaller to zero
	//! 2 pos, or 2 neg: decide which has higher absolute value;
#if 0
	// GENERAL PROBLEM WITH 1/Integer: need to do only those that "close"
	// So: 1/a-1/b, a<b Integer (positive)
	// need  b-a to divide a*b
	// b-a=0: OK (add-inv): done already
	// b-a=1: OK, do a*b to complete
	// b-a>1 and GCF(a,b)=1: ??
	// for 1/a+1/b: need a+b to divide a*b
	// * a==b: natural result is 2/a, check for a even (did this already)
	// * a<b, GCF(a,b)=1: ??
	// dualize this for negative version
	// NOTE: for nonconservative rearrange, we may safely neglect any small special cases
#endif
	if (VFT2Idx==rhs.VFT2Idx)
		{
		if (LargeIntIdx_VFT2==VFT2Idx || -LargeIntIdx_VFT2==VFT2Idx)
			// LHS large +/-, RHS large +/- [aligned +/-]
			return LHSRHSLargeSameSignForceCondense(rhs);
		if (SmallIntIdx_VFT2==VFT2Idx || -SmallIntIdx_VFT2==VFT2Idx)			
			// LHS small +/-, RHS small +/-	[aligned +/-]
			return LHSRHSSmallSameSignForceCondense(rhs);
		}
	else if	(LargeIntIdx_VFT2==VFT2Idx)
		{	// LHS large +
		if (SmallIntIdx_VFT2==rhs.VFT2Idx)
			// LHS large +, RHS small +
			return LHSLargeRHSSmallSameSignForceCondense(rhs);
		}
	else if (SmallIntIdx_VFT2==VFT2Idx)
		{	// LHS small +
		if (LargeIntIdx_VFT2==rhs.VFT2Idx)
			// LHS small +, RHS large +
			return rhs.LHSLargeRHSSmallSameSignForceCondense(*this);
		}
	else if (-LargeIntIdx_VFT2==VFT2Idx)
		{	// LHS large -, RHS -
		if (-SmallIntIdx_VFT2==rhs.VFT2Idx)
			// LHS large -, RHS small -
			return LHSLargeRHSSmallSameSignForceCondense(rhs);
		}
	else if (-SmallIntIdx_VFT2==VFT2Idx)
		{	// LHS small -, RHS -
		if      (-LargeIntIdx_VFT2==rhs.VFT2Idx)
			// LHS small -, RHS large -
			return rhs.LHSLargeRHSSmallSameSignForceCondense(*this);
		}
	return false;
}

bool
LHSIntArrayIsLarger(unsigned long* lhs, unsigned long* rhs, signed long VFT2Idx)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/20/2000
	if (_msize(lhs)==_msize(rhs))
		{
		size_t i = ArraySize(rhs)-1;
		while(lhs[i]==rhs[i] && 0< --i);
		if (lhs[i]==rhs[i]) return 0>VFT2Idx;
		return lhs[i]>rhs[i];
		}
	return _msize(lhs)>_msize(rhs);
}

bool _IntegerNumeral::LHSRHSLargeSameSignForceCondense(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/20/2000
	if (LHSIntArrayIsLarger(LongInteger,rhs.LongInteger,VFT2Idx))
		{
		unsigned long* Tmp = REALLOC(LongInteger,_msize(LongInteger)+sizeof(unsigned long));
		if (!Tmp) return false;
		LongInteger = Tmp;
		LongInteger[ArraySize(LongInteger)-1]=0;
		}
	else{
		unsigned long* Tmp = REALLOC(rhs.LongInteger,_msize(rhs.LongInteger)+sizeof(unsigned long));
		if (!Tmp) return false;
		rhs.LongInteger = Tmp;
		rhs.LongInteger[ArraySize(rhs.LongInteger)-1]=0;
		};
	LHSRHSLargeSameSignCondense(rhs);
	return true;
}

bool
_IntegerNumeral::LHSLargeRHSSmallSameSignForceCondense(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/20/2000
	unsigned long* Tmp = REALLOC(LongInteger,_msize(LongInteger)+sizeof(unsigned long));
	if (!Tmp) return false;
	LongInteger = Tmp;
	LongInteger[ArraySize(LongInteger)-1]=0;
	LHSLargeRHSSmallSameSignCondense(rhs);
	return true;
}

bool _IntegerNumeral::LHSRHSSmallSameSignForceCondense(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/20/2000
	unsigned long* Tmp = _new_buffer_uninitialized<unsigned long>(2);
	if (!Tmp) return false;

	Tmp[0] = ShortInteger+rhs.ShortInteger-WRAPLB;
	Tmp[1] = 1;

	if (ZeroRHSInSmallSameSignCondense(ShortInteger,rhs.ShortInteger,VFT2Idx))
		{
		rhs.VFT2Idx = ZeroIdx_VFT2;
		LongInteger = Tmp;
		VFT2Idx = (VFT2Idx>0) ? LargeIntIdx_VFT2 : -LargeIntIdx_VFT2;
		return true;
		}
	else{
		VFT2Idx = ZeroIdx_VFT2;
		rhs.LongInteger = Tmp;
		rhs.VFT2Idx = (rhs.VFT2Idx>0) ? LargeIntIdx_VFT2 : -LargeIntIdx_VFT2;
		return true;
		};
}

void _IntegerNumeral::RearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 5/4/2006
	// This routine coordinates those parts of multiplying _IntegerNumerals that do not involve 
	// dynamic memory allocation.
	// NOTE: zeros have been cleared out already
	if (   ((SmallIntIdx_VFT2==VFT2Idx || -SmallIntIdx_VFT2==VFT2Idx) && 1==ShortInteger)
		|| ((SmallIntIdx_VFT2==rhs.VFT2Idx || -SmallIntIdx_VFT2==rhs.VFT2Idx) && 1==rhs.ShortInteger))
		return;
	if 		( LargeIntIdx_VFT2==VFT2Idx || -LargeIntIdx_VFT2==VFT2Idx)
		{	// LHS Large +/-
		if (SmallIntInvIdx_VFT2==rhs.VFT2Idx || -SmallIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Large +/-, RHS SmallInv +/-
				// NOTE: TOTAL CASE
			LHSLargeRHSSmallExactlyOneInvRearrangeProduct(rhs);
			return;
			}
		else if ( SmallIntIdx_VFT2==rhs.VFT2Idx || SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Large +/-, RHS Small +/-
			LHSLargeRHSSmallZeroOrTwoInvRearrangeProduct(rhs);
			return;
			}
#if 0
		else if ( LargeIntIdx_VFT2==rhs.VFT2Idx || -LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Large +/-, RHS Large +/-
			}
		else{	// if ( LargeIntInvIdx_VFT2==rhs.VFT2Idx || -LargeIntInvIdx_VFT2==rhs.VFT2Idx)
				// LHS Large +/-, RHS LargeInv +/-
			}
#endif
		}
	else if ( SmallIntIdx_VFT2==VFT2Idx || -SmallIntIdx_VFT2==VFT2Idx)
		{	// LHS Small +/-
		if (SmallIntInvIdx_VFT2==rhs.VFT2Idx || -SmallIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Small +/-, RHS SmallInv +/-
				// TOTAL CASE: ignore in Force version
			LHSRHSSmallExactlyOneInvRearrangeProduct(rhs);
			return;
			}
		else if (SmallIntIdx_VFT2==rhs.VFT2Idx || -SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Small +/-, RHS Small +/-
			LHSRHSSmallZeroOrTwoInvRearrangeProduct(rhs);
			return;
			}
		else if (LargeIntInvIdx_VFT2==rhs.VFT2Idx || -LargeIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Small +/-, RHS LargeInv +/-
				// NOTE: TOTAL CASE
			rhs.LHSLargeRHSSmallExactlyOneInvRearrangeProduct(*this);
			return;
			}
		assert(LargeIntIdx_VFT2==rhs.VFT2Idx || -LargeIntIdx_VFT2==rhs.VFT2Idx);
		// LHS Small +/-, RHS Large +/-
		rhs.LHSLargeRHSSmallZeroOrTwoInvRearrangeProduct(*this);
		return;
		}
	else if ( SmallIntInvIdx_VFT2==VFT2Idx || -SmallIntInvIdx_VFT2==VFT2Idx)
		{	// LHS SmallInv +/-
		if (SmallIntIdx_VFT2==rhs.VFT2Idx || -SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS SmallInv +, RHS Small +/-
				// TOTAL CASE: ignore in Force version
			LHSRHSSmallExactlyOneInvRearrangeProduct(rhs);
			return;
			}
		else if (SmallIntInvIdx_VFT2==rhs.VFT2Idx || -SmallIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Small +/-, RHS Small +/-
			LHSRHSSmallZeroOrTwoInvRearrangeProduct(rhs);
			return;
			}
		else if (LargeIntIdx_VFT2==rhs.VFT2Idx || -LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS SmallInv +/-, RHS Large +/-
				// TOTAL CASE: ignore in Force version
			rhs.LHSLargeRHSSmallExactlyOneInvRearrangeProduct(*this);
			return;
			}
		assert(LargeIntInvIdx_VFT2==rhs.VFT2Idx || -LargeIntInvIdx_VFT2==rhs.VFT2Idx);
		// LHS SmallInv +/-, RHS LargeInv +/-
		rhs.LHSLargeRHSSmallZeroOrTwoInvRearrangeProduct(*this);
		return;
		}
	else if ( LargeIntInvIdx_VFT2==VFT2Idx || -LargeIntInvIdx_VFT2==VFT2Idx)
		{	// LHS LargeInv +/-
		if		(SmallIntIdx_VFT2==rhs.VFT2Idx || -SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS LargeInv +/-, RHS Small +/-
				// NOTE: TOTAL CASE
			LHSLargeRHSSmallExactlyOneInvRearrangeProduct(rhs);
			return;
			}
		else if ( SmallIntInvIdx_VFT2==rhs.VFT2Idx || -SmallIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS LargeInv +/-, RHS SmallInv +/-
			LHSLargeRHSSmallZeroOrTwoInvRearrangeProduct(rhs);
			return;
			}
#if 0
		else if ( LargeIntIdx_VFT2==RHS.VFT2Idx || -LargeIntIdx_VFT2==RHS.VFT2Idx)
			{	// LHS LargeInv +, RHS Large +
			}
		else{ 	// if ( LargeIntInvIdx_VFT2==RHS.VFT2Idx || -LargeIntInvIdx_VFT2==RHS.VFT2Idx)
				// LHS LargeInv +, RHS LargeInv +
			}
#endif
		}
}

void
_IntegerNumeral::LHSRHSSmallExactlyOneInvRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/8/2001
	const unsigned long LocalGCF = GCF_machine(ShortInteger,rhs.ShortInteger);
	if (1<LocalGCF)
		{
		ShortInteger /= LocalGCF;
		if (1==ShortInteger)
			VFT2Idx = (ZeroIdx_VFT2<VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
		rhs.ShortInteger /= LocalGCF;
		if (1==rhs.ShortInteger)
			rhs.VFT2Idx = (ZeroIdx_VFT2<rhs.VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
		};
}

void
_IntegerNumeral::LHSRHSSmallZeroOrTwoInvRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	//! \todo META: two sides are completely symmetric, consider folding code into another routine
	if (ShortInteger<rhs.ShortInteger)
		{
		const unsigned long BoundQuotient = (WRAPLB-1)/rhs.ShortInteger;
		if (BoundQuotient>=ShortInteger)
			{
			rhs.ShortInteger *= ShortInteger;
			ShortInteger = 1;
			VFT2Idx = (ZeroIdx_VFT2<VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
			return;
			}
		if (2<=BoundQuotient)
			{
			unsigned long i = BoundQuotient;
			do	if (0==ShortInteger%i)
					{
					rhs.ShortInteger *= i;
					ShortInteger /= i;
					return;
					}		
			while(1< --i);
			}
		}
	else{
		const unsigned long BoundQuotient = (WRAPLB-1)/ShortInteger;
		if (BoundQuotient>=rhs.ShortInteger)
			{
			ShortInteger *= rhs.ShortInteger;
			rhs.ShortInteger = 1;
			rhs.VFT2Idx = (ZeroIdx_VFT2<rhs.VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
			return;
			}
		if (2<=BoundQuotient)
			{
			unsigned long i = BoundQuotient;
			do	if (0==rhs.ShortInteger%i)
					{
					ShortInteger *= i;
					rhs.ShortInteger /= i;
					return;
					}		
			while(1< --i);
			}
		}
}

void
_IntegerNumeral::LHSLargeRHSSmallExactlyOneInvRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// NOTE: the object is cancellation...which is problematic due to Franci's available
	// divisibility tests in base 10^9.	Cf. discussion at top of this source file.
	// RHS Small allows more flexibility...a total solution.
	const unsigned long Remainder = RemainderOfLargeDivideByN(rhs.ShortInteger);
	// NOTE: for this code to work, LargeDivideByN must be a digital operation
	if (0==Remainder)
		{
		LargeDivideByN(rhs.ShortInteger);
		rhs.ShortInteger = 1;
		rhs.VFT2Idx = (0<rhs.VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
		return;
		}
	const unsigned long LocalGCF = GCF_machine(rhs.ShortInteger,Remainder);
	if (1<LocalGCF)
		{
		rhs.ShortInteger /= LocalGCF;
		if (1==rhs.ShortInteger)
			rhs.VFT2Idx = (0<rhs.VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
		LargeDivideByN(LocalGCF);		
		}
}

#if 0
void
_IntegerNumeral::LHSRHSLargeExactlyOneInvRearrangeProduct(_IntegerNumeral& rhs)
{	//! \todo IMPLEMENT
	// NOTE: the object is cancellation.  More efficient methods are highly recommended.
	// This only does those parts that do not require nonconservative intermediate stages.
}

void
_IntegerNumeral::LHSRHSLargeZeroOrTwoInvRearrangeProduct(_IntegerNumeral& rhs)
{	//! \todo IMPLEMENT
	// NOTE: the object is rearrangement...reducing the representation size of either LHS
	// or RHS without risking a RAM allocation.  This routine is rather specialized 
	// (byte-nibbling)
}
#endif


unsigned long
_IntegerNumeral::RemainderOfLargeDivideByN(unsigned long N) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This is scaled for N>10^9
	unsigned long Buffer[18];	// Franci's scratchpad
	unsigned long VirtualQuotient[2];
	Buffer[0] = N;
	Buffer[1] = 0;
	size_t i = 2;
	do	{
		Buffer[i] = (Buffer[i-2]%TEN_TO_8)*TEN_TO_1;
		Buffer[i+1] = Buffer[i-1]*TEN_TO_1+Buffer[i-2]/TEN_TO_8;
		i+=2;
		}
	while(18>i);
	// table initialized: N, 10N, .... 10^9*N
	i = ArraySize(LongInteger);
	VirtualQuotient[0]=0;
	do	{
		VirtualQuotient[1] = VirtualQuotient[0];
		VirtualQuotient[0] = LongInteger[ --i];
		size_t j = 18;
		do	{
			j-=2;
			while(    VirtualQuotient[1]>Buffer[j+1]
				  || (VirtualQuotient[1]==Buffer[j+1] && VirtualQuotient[0]>=Buffer[j]))
				{
				VirtualQuotient[1]-=Buffer[j+1];
				if (VirtualQuotient[0]<Buffer[j])
					{
					VirtualQuotient[1]--;
					VirtualQuotient[0]+=WRAPLB;
					}
				VirtualQuotient[0]-=Buffer[j];
				};
			}
		while(0<j);
		}
	while(0<i);
	return VirtualQuotient[0];
}

unsigned long _IntegerNumeral::LargeDivideByN(unsigned long N)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This is scaled for N>10^9
	unsigned long Buffer[18];	// Franci's scratchpad
	unsigned long VirtualQuotient[2];
	Buffer[0] = N;
	Buffer[1] = 0;
	size_t i = 2;
	do	{
		Buffer[i] = (Buffer[i-2]%TEN_TO_8)*TEN_TO_1;
		Buffer[i+1] = Buffer[i-1]*TEN_TO_1+Buffer[i-2]/TEN_TO_8;
		i+=2;
		}
	while(18>i);
	// table initialized: N, 10N, .... 10^9*N
	i = ArraySize(LongInteger);
	VirtualQuotient[0]=0;
	do	{
		VirtualQuotient[1] = VirtualQuotient[0];
		VirtualQuotient[0] = LongInteger[ --i];
		LongInteger[i] = 0;
		size_t j = 18;
		size_t ScalingFactor = TEN_TO_9;
		do	{
			ScalingFactor/=10;
			j-=2;
			while(    VirtualQuotient[1]>Buffer[j+1]
				  || (VirtualQuotient[1]==Buffer[j+1] && VirtualQuotient[0]>=Buffer[j]))
				{
				VirtualQuotient[1]-=Buffer[j+1];
				if (VirtualQuotient[0]<Buffer[j])
					{
					VirtualQuotient[1]--;
					VirtualQuotient[0]+=WRAPLB;
					}
				VirtualQuotient[0]-=Buffer[j];
				LongInteger[i]+=ScalingFactor;
				}
			}
		while(0<j);
		}
	while(0<i);
	if (0==LongInteger[ArraySize(LongInteger)-1])
		{
		if (2<ArraySize(LongInteger))
			LongInteger = REALLOC(LongInteger,_msize(LongInteger)-sizeof(unsigned long));
		else{
			if (1==ShortInteger || in_Z())
				VFT2Idx = (0<VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
			else
				VFT2Idx = (0<VFT2Idx) ? SmallIntInvIdx_VFT2 : -SmallIntInvIdx_VFT2;
			unsigned long Tmp = LongInteger[0];
			free(LongInteger);
			ShortInteger = Tmp;
			}
		}
	return VirtualQuotient[0];
}

void
_IntegerNumeral::LHSLargeRHSSmallZeroOrTwoInvRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2001
	// NOTE: the object is rearrangement...making LHS larger and RHS smaller without
	// risking a RAM allocation.
	//! \todo byte-nibbling variant: shrink down LHS representation while keeping RHS small
	const unsigned long LHSMultiplierLimit = (WRAPLB-1)/(LongInteger[ArraySize(LongInteger)-1]+1);
	if (LHSMultiplierLimit>=rhs.ShortInteger)
		{
		SUCCEED_OR_DIE(0!=MultiplyLargeByN(rhs.ShortInteger));
		rhs.ShortInteger = 1;
		return;
		}
	else if (2<=LHSMultiplierLimit)
		{
		unsigned long i = LHSMultiplierLimit;
		do	if (0==rhs.ShortInteger%i)
				{
				SUCCEED_OR_DIE(0!=MultiplyLargeByN(i));
				rhs.ShortInteger /= i;
				return;
				}
		while(1< --i);
		}
}

// digital operation; ignores Inv considerations.
unsigned long _IntegerNumeral::MultiplyLargeByN(unsigned long N)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/14/2001
	// NOTE: ideally, range-limited to 10^9 digit
	// NOTE: This is an ASM Target!
	const unsigned long NHigh3 = N/TEN_TO_6; 
	const unsigned long NMid3 = N/TEN_TO_3-TEN_TO_3*NHigh3;
	const unsigned long NLow3 = N%TEN_TO_3;
	unsigned long Carry = 0;
	size_t i = 0;
	do	{
		const unsigned long DHigh3 = LongInteger[i]/TEN_TO_6; 
		const unsigned long DMid3 = LongInteger[i]/TEN_TO_3-TEN_TO_3*DHigh3;
		const unsigned long DLow3 = LongInteger[i]%TEN_TO_3;
		const unsigned long Bridge = NHigh3*DLow3+NMid3*DMid3+NLow3*DHigh3;
		unsigned long Accumulator = Carry;
		Accumulator += NLow3*DLow3+TEN_TO_3*(NLow3*DMid3+NMid3*DLow3);
		Carry = TEN_TO_3*NHigh3*DHigh3+NHigh3*DMid3+NMid3*DHigh3;
		Carry += Accumulator/WRAPLB;
		Accumulator %= WRAPLB;
		Accumulator += TEN_TO_6*(Bridge%TEN_TO_3);
		Carry += Bridge/TEN_TO_3;
		LongInteger[i] = Accumulator;
		}
	while(ArraySize(LongInteger)> ++i);
	return Carry;
}

bool _IntegerNumeral::ForceRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// This routine coordinates those parts of multiplying _IntegerNumerals that involve
	// dynamic memory allocation.
	// NOTE: zeros have been cleared out already
	RearrangeProduct(rhs);
	if (   ((SmallIntIdx_VFT2==VFT2Idx || -SmallIntIdx_VFT2==VFT2Idx) && 1==ShortInteger)
		|| ((SmallIntIdx_VFT2==rhs.VFT2Idx || -SmallIntIdx_VFT2==rhs.VFT2Idx) && 1==rhs.ShortInteger))
		return true;
	if 		( LargeIntIdx_VFT2==VFT2Idx || -LargeIntIdx_VFT2==VFT2Idx)
		{	// LHS Large +/-
		// NOTE: LHS Large +, RHS SmallInv +/- perfectly handled conservatively
		if 		( SmallIntIdx_VFT2==rhs.VFT2Idx || -SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Large +/-, RHS Small +/-
			return LHSLargeRHSSmallZeroOrTwoInvForceRearrangeProduct(rhs);
			}
		else if ( LargeIntIdx_VFT2==rhs.VFT2Idx || -LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Large +/-, RHS Large +/-
			return LHSRHSLargeZeroOrTwoInvForceRearrangeProduct(rhs);
			}
		else if ( LargeIntInvIdx_VFT2==rhs.VFT2Idx || -LargeIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Large +/-, RHS LargeInv +/-
			return LHSRHSLargeExactlyOneInvForceRearrangeProduct(rhs);
			}
		}
	else if ( SmallIntIdx_VFT2==VFT2Idx || -SmallIntIdx_VFT2==VFT2Idx)
		{	// LHS Small +/-
		// NOTE: LHS Small +/-, RHS SmallInv +/- perfectly handled conservatively
		// NOTE: LHS Small +/-, RHS LargeInv +/- perfectly handled conservatively
		if 		( LargeIntIdx_VFT2==rhs.VFT2Idx || -LargeIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Small +/-, RHS Large +/-
			return rhs.LHSLargeRHSSmallZeroOrTwoInvForceRearrangeProduct(*this);
			}
		else if ( SmallIntIdx_VFT2==rhs.VFT2Idx || -SmallIntIdx_VFT2==rhs.VFT2Idx)
			{	// LHS Small +/-, RHS Small +/-
			return LHSRHSSmallZeroOrTwoInvForceRearrangeProduct(rhs);
			}
		}
	else if ( SmallIntInvIdx_VFT2==VFT2Idx || -SmallIntInvIdx_VFT2==VFT2Idx)
		{	// LHS SmallInv +/-
		// NOTE: LHS SmallInv +/-, RHS Small +/- perfectly handled conservatively
		// NOTE: LHS SmallInv +/-, RHS Large +/- perfectly handled conservatively		
		if		( LargeIntInvIdx_VFT2==rhs.VFT2Idx || -LargeIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS SmallInv +/-, RHS LargeInv +/-
			return rhs.LHSLargeRHSSmallZeroOrTwoInvForceRearrangeProduct(*this);
			}
		if 		( SmallIntInvIdx_VFT2==rhs.VFT2Idx || -SmallIntInvIdx_VFT2==rhs.VFT2Idx)
			{	// LHS SmallInv +/-, RHS SmallInv +/-
			return LHSRHSSmallZeroOrTwoInvForceRearrangeProduct(rhs);
			}
		}
	assert(LargeIntInvIdx_VFT2==VFT2Idx || -LargeIntInvIdx_VFT2==VFT2Idx);
	// LHS LargeInv +/-
	// NOTE: LHS LargeInv +/-, RHS Small +/- perfectly handled conservatively
	if 		( SmallIntInvIdx_VFT2==rhs.VFT2Idx || -SmallIntInvIdx_VFT2==rhs.VFT2Idx)
		{	// LHS LargeInv +/-, RHS SmallInv +/-
		return LHSLargeRHSSmallZeroOrTwoInvForceRearrangeProduct(rhs);
		}
	else if ( LargeIntIdx_VFT2==rhs.VFT2Idx || -LargeIntIdx_VFT2==rhs.VFT2Idx)
		{	// LHS LargeInv +/-, RHS Large +/-
		return LHSRHSLargeExactlyOneInvForceRearrangeProduct(rhs);
		}
	else if ( LargeIntInvIdx_VFT2==rhs.VFT2Idx || -LargeIntInvIdx_VFT2==rhs.VFT2Idx)
		{	// LHS LargeInv +/-, RHS LargeInv +/-
		return LHSRHSLargeZeroOrTwoInvForceRearrangeProduct(rhs);
		}
	return false;
}

bool
_IntegerNumeral::LHSLargeRHSSmallZeroOrTwoInvForceRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/15/2001
	// NOTE: the object is rearrangement.  RAM reallocation OK.
	const unsigned long Overflow = MultiplyLargeByN(rhs.ShortInteger);
	if (0!=Overflow)
		{
		unsigned long* Tmp = REALLOC(LongInteger,_msize(LongInteger)+sizeof(unsigned long));
		SUCCEED_OR_DIE(NULL!=Tmp);	// possibly should throw std::bad_alloc instead
		Tmp[ArraySize(Tmp)-1] = Overflow;
		LongInteger = Tmp;
		}
	rhs.ShortInteger = 1;
	rhs.VFT2Idx = (0<rhs.VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
	return true;
}

bool
_IntegerNumeral::LHSRHSSmallZeroOrTwoInvForceRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/15/2001
	// NOTE: the object is rearrangement.  RAM reallocation OK.
	const unsigned long LHigh3 = ShortInteger/TEN_TO_6; 
	const unsigned long LMid3 = ShortInteger/TEN_TO_3-TEN_TO_3*LHigh3;
	const unsigned long LLow3 = ShortInteger%TEN_TO_3;
	const unsigned long RHigh3 = rhs.ShortInteger/TEN_TO_6; 
	const unsigned long RMid3 = rhs.ShortInteger/TEN_TO_3-TEN_TO_3*RHigh3;
	const unsigned long RLow3 = rhs.ShortInteger%TEN_TO_3;
	unsigned long Low9Accumulator = LLow3*RLow3+TEN_TO_3*(LMid3*RLow3+LLow3*RMid3);
	unsigned long High9Accumulator = TEN_TO_3*LHigh3*RHigh3+LHigh3*RMid3+LMid3*RHigh3+Low9Accumulator/WRAPLB;
	unsigned long Bridge = LHigh3*RLow3+LMid3*RMid3+LLow3*RHigh3;
	Low9Accumulator %= WRAPLB;
	Low9Accumulator += TEN_TO_6*(Bridge%TEN_TO_3);
	High9Accumulator += Bridge/TEN_TO_3+Low9Accumulator/WRAPLB;
	Low9Accumulator %= WRAPLB;
	if (0==High9Accumulator)
		ShortInteger = Low9Accumulator;
	else{
		unsigned long* Tmp = _new_buffer_uninitialized<unsigned long>(2);
		if (!Tmp) return false;
		Tmp[1] = High9Accumulator;
		Tmp[0] = Low9Accumulator;
		if (in_Z())
			VFT2Idx = (0<VFT2Idx) ? LargeIntIdx_VFT2 : -LargeIntIdx_VFT2;
		else
			VFT2Idx = (0<VFT2Idx) ? LargeIntInvIdx_VFT2 : -LargeIntInvIdx_VFT2;
		LongInteger = Tmp;
		}
	rhs.ShortInteger = 1;
	rhs.VFT2Idx = (0<rhs.VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;	
	return true;		
}

bool
_IntegerNumeral::LHSRHSLargeExactlyOneInvForceRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// NOTE: the object is rearrangement.  RAM reallocation OK.
//!	\todo: reimplement this
	_IntegerNumeral** GCFArgArray = _new_buffer<_IntegerNumeral*>(2);
	if (!GCFArgArray) return false;
	try	{
		GCFArgArray[0] = new _IntegerNumeral(*this);
		if (!GCFArgArray[0]->in_Z()) GCFArgArray[0]->SelfInverse(Multiplication);
		GCFArgArray[1] = new _IntegerNumeral(rhs);
		if (!GCFArgArray[1]->in_Z()) GCFArgArray[1]->SelfInverse(Multiplication);
		}
	catch(const bad_alloc&)
		{
		BLOCKDELETEARRAY_AND_NULL(GCFArgArray);
		return false;
		}
	_GCF TestGCF(GCFArgArray);
	if (TestGCF().IsOne()) return true;
	if (Small_INT==IntegerRAMForm[VFT2Idx+ZeroOffset_VFT2])
		{
		LargeDivideByN(TestGCF().ShortInteger);
		rhs.LargeDivideByN(TestGCF().ShortInteger);
		return true;
		}
	unsigned long* ScratchMultiples = NULL;
	unsigned long* LHSQuotientWorkspace = NULL;
	unsigned long* RHSQuotientWorkspace = NULL;
	if (!TestGCF().ConstructScratchMultiples(ScratchMultiples))
		return false;
	if (!ConstructQuotientWorkspace(TestGCF(),LHSQuotientWorkspace))
		{
		free(ScratchMultiples);
		return false;
		}
	if (!rhs.ConstructQuotientWorkspace(TestGCF(),RHSQuotientWorkspace))
		{
		free(LHSQuotientWorkspace);
		free(ScratchMultiples);
		return false;
		}
	LargeDivideByLarge(TestGCF(),LHSQuotientWorkspace,ScratchMultiples);		// consumes LHSQuotientWorkspace
	rhs.LargeDivideByLarge(TestGCF(),RHSQuotientWorkspace,ScratchMultiples);	// consumes RHSQuotientWorkspace
	free(ScratchMultiples);
	return false;
}

void
_IntegerNumeral::MultiplyByNIntoBuffer(unsigned long N,unsigned long* const ResultBuffer) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	const unsigned long NHigh3 = N/TEN_TO_6; 
	const unsigned long NMid3 = N/TEN_TO_3-TEN_TO_3*NHigh3;
	const unsigned long NLow3 = N%TEN_TO_3;
	size_t i = 0;
	do	{
		const unsigned long LHigh3 = LongInteger[i]/TEN_TO_6; 
		const unsigned long LMid3 = LongInteger[i]/TEN_TO_3-TEN_TO_3*LHigh3;
		const unsigned long LLow3 = LongInteger[i]%TEN_TO_3;
		unsigned long Low9Accumulator = LLow3*NLow3+TEN_TO_3*(LMid3*NLow3+LLow3*NMid3);
		unsigned long High9Accumulator = TEN_TO_3*LHigh3*NHigh3+LHigh3*NMid3+LMid3*NHigh3+Low9Accumulator/WRAPLB;
		unsigned long Bridge = LHigh3*NLow3+LMid3*NMid3+LLow3*NHigh3;
		Low9Accumulator %= WRAPLB;
		Low9Accumulator += TEN_TO_6*(Bridge%TEN_TO_3);
		High9Accumulator += Bridge/TEN_TO_3+Low9Accumulator/WRAPLB;
		Low9Accumulator %= WRAPLB;
		ResultBuffer[i] += Low9Accumulator;
		ResultBuffer[i+1] += High9Accumulator;
		if (WRAPLB<=ResultBuffer[i])
			{
			ResultBuffer[i] -= WRAPLB;
			ResultBuffer[i+1]+=1;
			}
		if (WRAPLB<=ResultBuffer[i+1])
			{
			ResultBuffer[i+1] -= WRAPLB;
			ResultBuffer[i+2]+=1;
			}			
		}
	while(ArraySize(LongInteger) > ++i);
}

bool
_IntegerNumeral::LHSRHSLargeZeroOrTwoInvForceRearrangeProduct(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// NOTE: the object is rearrangement.  RAM reallocation OK.
	unsigned long* ResultBuffer = _new_buffer<unsigned long>(ArraySize(LongInteger)+ArraySize(rhs.LongInteger));
	if (!ResultBuffer) return false;
	size_t i = 0;
	do	MultiplyByNIntoBuffer(rhs.LongInteger[i],ResultBuffer+i);
	while(ArraySize(rhs.LongInteger) > ++i);
	// set RHS to 1
	rhs.VFT2Idx = (0<VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
	free(rhs.LongInteger);
	rhs.ShortInteger = 1;
	free(LongInteger);
	LongInteger = ResultBuffer;
	if (0==LongInteger[ArraySize(LongInteger)-1])
		LongInteger = REALLOC(LongInteger,_msize(LongInteger)-sizeof(unsigned long));
	return true;
}

bool
_IntegerNumeral::ConstructScratchMultiples(unsigned long*& ScratchMultiples) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/24/2001
	// This constructs the ScratchMultiples table for a Large _IntegerNumeral division by 
	// a Large _IntegerNumeral.  Called on the Large _IntegerNumeral divisor.
	// 1x, 10x, ... 10^8x
	assert(!ScratchMultiples);
	ScratchMultiples = _new_buffer<unsigned long>(9*(ArraySize(LongInteger)+1));
	if (!ScratchMultiples) return false;
	memmove(ScratchMultiples,LongInteger,_msize(LongInteger));
	size_t i = ArraySize(LongInteger)+1;
	do	{
		size_t j = ArraySize(LongInteger);
		ScratchMultiples[i+j] += ScratchMultiples[i+j-ArraySize(LongInteger)-1]*TEN_TO_1;
		do	{
			--j;
			ScratchMultiples[i+j] += (ScratchMultiples[i+j-ArraySize(LongInteger)-1]%TEN_TO_8)*TEN_TO_1;
			ScratchMultiples[i+j+1] += ScratchMultiples[i+j-ArraySize(LongInteger)-1]/TEN_TO_8;
			}
		while(0<j);
		i += ArraySize(LongInteger)+1;
		}
	while(9*(ArraySize(LongInteger)+1)>i);
	return true;
}

bool
_IntegerNumeral::ConstructQuotientWorkspace(const _IntegerNumeral& Divisor,unsigned long*& QuotientWorkspace) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/20/2001
	assert(!QuotientWorkspace);
	QuotientWorkspace = _new_buffer<unsigned long>(ArraySize(LongInteger)-ArraySize(Divisor.LongInteger)+1);
	return NULL!=QuotientWorkspace;
}

static bool
LargeDivideByLargeAux(unsigned long* const LongInteger, const unsigned long* const ScratchMultiples, size_t i, size_t ScanIdx, size_t ScanIdx2)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if 		(ArraySize(LongInteger)<ScanIdx) return false;
	else if (ArraySize(LongInteger)==ScanIdx)
		{
		if (0<ScratchMultiples[ScanIdx2]) return false;
		do	{
			if 		(LongInteger[--ScanIdx]<ScratchMultiples[--ScanIdx2])
				return false;
			else if (LongInteger[ScanIdx]>ScratchMultiples[ScanIdx2])
				return true;
			}
		while(i<ScanIdx);
		return true;
		}
	else if (LongInteger[ScanIdx]<ScratchMultiples[ScanIdx2])
		return false;
	else if (LongInteger[ScanIdx]==ScratchMultiples[ScanIdx2])
		{
		do	{
			if 		(LongInteger[--ScanIdx]<ScratchMultiples[--ScanIdx2])
				return false;
			else if (LongInteger[ScanIdx]>ScratchMultiples[ScanIdx2])
				return true;
			}
		while(i<ScanIdx);				
		}
	return true;
}

void
_IntegerNumeral::LargeDivideByLarge(const _IntegerNumeral& Divisor,unsigned long*& QuotientWorkspace, const unsigned long* const ScratchMultiples)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	const size_t ScratchRecordLength = ArraySize(Divisor.LongInteger)+1;
	size_t i = ArraySize(QuotientWorkspace);
	do	{
		--i;
		size_t j = 9*ScratchRecordLength;
		unsigned long ScaleFactor = TEN_TO_9;
		do	{
			j -= ScratchRecordLength;
			ScaleFactor /= TEN_TO_1;
			size_t ScanIdx = i+ScratchRecordLength-1;
			size_t ScanIdx2 = j+ScratchRecordLength-1;
			while(LargeDivideByLargeAux(LongInteger,ScratchMultiples,i,ScanIdx,ScanIdx2))
				{
				DigitalSubtract(&LongInteger[i],&ScratchMultiples[j],(0==ScratchMultiples[j+ScratchRecordLength-1]) ? ScratchRecordLength-2 : ScratchRecordLength-1);
				QuotientWorkspace[i] += ScaleFactor;
				}
			}
		while(0<j);
		}
	while(0<i);
	free(LongInteger);
	LongInteger = QuotientWorkspace;
	QuotientWorkspace = NULL;
	if (    1==ArraySize(LongInteger)
		|| (2==ArraySize(LongInteger) && 0==LongInteger[ArraySize(LongInteger)-1]))
		{
		unsigned long Tmp = LongInteger[0];
		if (in_Z() || 1==Tmp)
			VFT2Idx = (0<VFT2Idx) ? SmallIntIdx_VFT2 : -SmallIntIdx_VFT2;
		else
			VFT2Idx = (0<VFT2Idx) ? SmallIntInvIdx_VFT2 : -SmallIntInvIdx_VFT2;			
		free(LongInteger);
		ShortInteger = Tmp;
		if (0==Tmp) VFT2Idx = ZeroIdx_VFT2;
		}
	else if (0==LongInteger[ArraySize(LongInteger)-1])
		LongInteger = REALLOC(LongInteger,_msize(LongInteger)-sizeof(unsigned long));
}

void
_IntegerNumeral::DestructiveNormalProductFormWithRHS(_IntegerNumeral& rhs)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if (IsZero())
		{
		rhs=(unsigned short)(0);
		return;
		}
	if (rhs.IsZero())
		{
		*this=(unsigned short)(0);
		return;
		}
	if (IsOne() || rhs.IsOne())	return;
	if (*this==(signed short)(-1) || rhs==(signed short)(-1) || (IsNegative() && rhs.IsNegative()))
		{
		SelfInverse(Addition);
		rhs.SelfInverse(Addition);
		}
}

// text string I/O
void _IntegerNumeral::ConstructSelfNameAux(char* Name) const	// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2000
	(this->*ConstructSelfNameAux2[VFT2Idx+ZeroOffset_VFT2])(Name);
}

static bool TextDigitZeroInvisible(char*& Name, unsigned short Digit)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	if (0==Digit) return false;
	assert(10>Digit);
	*Name++ = char('0'+Digit);
	return true;
}

static void TextDigitZeroShows(char*& Name, unsigned short Digit)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	assert(10>Digit);
	*Name++ = char('0'+Digit);
}

static void Leading10_9Place(char*& Name, unsigned long Place)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/9/1999
	// NOTE: this has some problems with similar termination code, which justifies the use
	// of goto.
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_9-Place%TEN_TO_8)/TEN_TO_8))
		goto Place10_7Visible;
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_8-Place%TEN_TO_7)/TEN_TO_7))
		goto Place10_6Visible;
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_7-Place%TEN_TO_6)/TEN_TO_6))
		goto Place10_5Visible;
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_6-Place%TEN_TO_5)/TEN_TO_5))
		goto Place10_4Visible;
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_5-Place%TEN_TO_4)/TEN_TO_4))
		goto Place10_3Visible;
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_4-Place%TEN_TO_3)/TEN_TO_3))
		goto Place10_2Visible;
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_3-Place%TEN_TO_2)/TEN_TO_2))
		goto Place10_1Visible;
	if (TextDigitZeroInvisible(Name,(Place%TEN_TO_2-Place%TEN_TO_1)/TEN_TO_1))
		goto Place10_0Visible;
	SUCCEED_OR_DIE(TextDigitZeroInvisible(Name,Place%TEN_TO_1));
	return;
//	Final code
Place10_7Visible:
	TextDigitZeroShows(Name,(Place%TEN_TO_8-Place%TEN_TO_7)/TEN_TO_7);
Place10_6Visible:
	TextDigitZeroShows(Name,(Place%TEN_TO_7-Place%TEN_TO_6)/TEN_TO_6);
Place10_5Visible:
	TextDigitZeroShows(Name,(Place%TEN_TO_6-Place%TEN_TO_5)/TEN_TO_5);
Place10_4Visible:
	TextDigitZeroShows(Name,(Place%TEN_TO_5-Place%TEN_TO_4)/TEN_TO_4);
Place10_3Visible:
	TextDigitZeroShows(Name,(Place%TEN_TO_4-Place%TEN_TO_3)/TEN_TO_3);
Place10_2Visible:
	TextDigitZeroShows(Name,(Place%TEN_TO_3-Place%TEN_TO_2)/TEN_TO_2);
Place10_1Visible:
	TextDigitZeroShows(Name,(Place%TEN_TO_2-Place%TEN_TO_1)/TEN_TO_1);
Place10_0Visible:
	TextDigitZeroShows(Name,Place%TEN_TO_1);
}

static void NonLeading10_9Place(char*& Name, unsigned long Place)
{	// FORMALLY CORRECT: Kenneth Boyd, 6/9/1999
	TextDigitZeroShows(Name,(Place%TEN_TO_9-Place%TEN_TO_8)/TEN_TO_8);
	TextDigitZeroShows(Name,(Place%TEN_TO_8-Place%TEN_TO_7)/TEN_TO_7);
	TextDigitZeroShows(Name,(Place%TEN_TO_7-Place%TEN_TO_6)/TEN_TO_6);
	TextDigitZeroShows(Name,(Place%TEN_TO_6-Place%TEN_TO_5)/TEN_TO_5);
	TextDigitZeroShows(Name,(Place%TEN_TO_5-Place%TEN_TO_4)/TEN_TO_4);
	TextDigitZeroShows(Name,(Place%TEN_TO_4-Place%TEN_TO_3)/TEN_TO_3);
	TextDigitZeroShows(Name,(Place%TEN_TO_3-Place%TEN_TO_2)/TEN_TO_2);
	TextDigitZeroShows(Name,(Place%TEN_TO_2-Place%TEN_TO_1)/TEN_TO_1);
	TextDigitZeroShows(Name,Place%TEN_TO_1);
}

void _IntegerNumeral::ConstructSelfNamePositiveInt(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2000
	Leading10_9Place(Name,LongInteger[ArraySize(LongInteger)-1]);
	if (2<ArraySize(LongInteger))
		{
		size_t i = ArraySize(LongInteger)-2;
		do	NonLeading10_9Place(Name,LongInteger[i]);
		while(0< --i);
		}
	NonLeading10_9Place(Name,LongInteger[0]);	
}

void _IntegerNumeral::ConstructSelfNameSmallPositiveInt(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/7/1999
	Leading10_9Place(Name,ShortInteger);
}

void _IntegerNumeral::ConstructSelfNamePositiveIntInv(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/30/2001
	Leading10_9Place(Name,LongInteger[ArraySize(LongInteger)-1]);
	if (2<ArraySize(LongInteger))
		{
		size_t i = ArraySize(LongInteger)-2;
		do	NonLeading10_9Place(Name,LongInteger[i]);
		while(0< --i);
		}
	NonLeading10_9Place(Name,LongInteger[0]);	
	strcpy(Name,MULT_INV_TEXT);
}

void _IntegerNumeral::ConstructSelfNameSmallPositiveIntInv(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/30/2001
	Leading10_9Place(Name,ShortInteger);
	strcpy(Name,MULT_INV_TEXT);
}

void _IntegerNumeral::ConstructSelfNameZeroInt(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/7/1999
	Name[0]='0';
}

void _IntegerNumeral::ConstructSelfNameSmallNegativeInt(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/7/1999
	Name[0]='-';
	ConstructSelfNameSmallPositiveInt(Name+1);
}

void _IntegerNumeral::ConstructSelfNameNegativeInt(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 6/7/1999
	Name[0]='-';
	ConstructSelfNamePositiveInt(Name+1);
}

void _IntegerNumeral::ConstructSelfNameSmallNegativeIntInv(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/30/2001
	Name[0]='-';
	ConstructSelfNameSmallPositiveIntInv(Name+1);
}

void _IntegerNumeral::ConstructSelfNameNegativeIntInv(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/30/2001
	Name[0]='-';
	ConstructSelfNamePositiveIntInv(Name+1);
}

// start at 0 to get length
size_t _IntegerNumeral::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/17/2000
	return (this->*LengthOfSelfNameAux2[VFT2Idx+ZeroOffset_VFT2])();
}

size_t Leading10_9DigitLength(unsigned long Digit)
{
	return 	  (TEN_TO_8<=Digit) ? 9
			: (TEN_TO_7<=Digit) ? 8
			: (TEN_TO_6<=Digit) ? 7
			: (TEN_TO_5<=Digit) ? 6
			: (TEN_TO_4<=Digit) ? 5
			: (TEN_TO_3<=Digit) ? 4
			: (TEN_TO_2<=Digit) ? 3
			: (TEN_TO_1<=Digit) ? 2
			: 1;
}

size_t _IntegerNumeral::LengthOfSelfNamePositiveIntInv() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	size_t Length = (ArraySize(LongInteger)-1)*9;
	Length += Leading10_9DigitLength(LongInteger[ArraySize(LongInteger)-1])+13;
	return Length;
}

size_t _IntegerNumeral::LengthOfSelfNameSmallPositiveIntInv() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	return Leading10_9DigitLength(ShortInteger)+13;
}

size_t _IntegerNumeral::LengthOfSelfNamePositiveInt() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	size_t Length = (ArraySize(LongInteger)-1)*9;
	Length += Leading10_9DigitLength(LongInteger[ArraySize(LongInteger)-1]);
	return Length;
}

size_t _IntegerNumeral::LengthOfSelfNameSmallPositiveInt() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	return Leading10_9DigitLength(ShortInteger);
}

size_t _IntegerNumeral::LengthOfSelfNameZeroInt() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	return 1;
}

size_t _IntegerNumeral::LengthOfSelfNameSmallNegativeInt() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	return Leading10_9DigitLength(ShortInteger)+1;
}

size_t _IntegerNumeral::LengthOfSelfNameNegativeInt() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	size_t Length = (ArraySize(LongInteger)-1)*9;
	Length += Leading10_9DigitLength(LongInteger[ArraySize(LongInteger)-1])+1;
	return Length;
}

size_t _IntegerNumeral::LengthOfSelfNameSmallNegativeIntInv() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	return Leading10_9DigitLength(ShortInteger)+14;
}

size_t _IntegerNumeral::LengthOfSelfNameNegativeIntInv() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	size_t Length = (ArraySize(LongInteger)-1)*9;
	Length += Leading10_9DigitLength(LongInteger[ArraySize(LongInteger)-1])+14;
	return Length;
}

