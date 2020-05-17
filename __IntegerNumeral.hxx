// __IntegerNumeral.hpp
// standalone version of IntegerNumeral (not glued to MetaConcept)

#ifndef __INTEGERNUMERAL_HPP
#define __INTEGERNUMERAL_HPP 1

#include <stddef.h>
#include <algorithm>
#include "Zaimoni.STL/Compiler.h"
#include "Zaimoni.STL/polymorphic.hpp"

class _IntegerNumeral {
private:
	static_assert(sizeof(unsigned long) <= sizeof(unsigned long*));
	static_assert(sizeof(uintptr_t) == sizeof(unsigned long*));
	union {
		uintptr_t ShortInteger;
		unsigned long* LongInteger;
	};
	signed short VFT2Idx;

public:
	enum operation_index 	{
							Addition = 0,
							Multiplication = 1
							};

	_IntegerNumeral() :	VFT2Idx(0) {};	// defaults to zero
	_IntegerNumeral(const _IntegerNumeral& src);
	_IntegerNumeral(_IntegerNumeral&& src);
	explicit _IntegerNumeral(unsigned short src);
	explicit _IntegerNumeral(signed short src);
	explicit _IntegerNumeral(unsigned long src);
	explicit _IntegerNumeral(signed long src);
	explicit _IntegerNumeral(const char* src);
	~_IntegerNumeral();

	_IntegerNumeral& operator=(const _IntegerNumeral& src);
	_IntegerNumeral& operator=(_IntegerNumeral&& src);
	_IntegerNumeral& operator=(signed short src);
	_IntegerNumeral& operator=(unsigned short src);
	_IntegerNumeral& operator=(signed long src);
	_IntegerNumeral& operator=(unsigned long src);

	void MoveInto(_IntegerNumeral*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

	bool operator==(const _IntegerNumeral& rhs) const;	
	bool operator==(signed short rhs) const;
	bool operator==(unsigned short rhs) const;

	bool operator<(const _IntegerNumeral& RHS) const;

	bool AbsValIsOneOverN(unsigned short N) const;
//  Evaluation functions
	bool SyntaxOK() const;
// text I/O functions
	size_t LengthOfSelfName() const;
protected:
	void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
public:
	bool IsOne() const;
	bool IsZero() const {return 0==VFT2Idx;};
	bool IsPositive() const {return 0<VFT2Idx;};
	bool IsNegative() const {return 0>VFT2Idx;};
	bool in_Z() const;	// multiplicative inverses are merely rationals, not integers

// Formal manipulation functions
	bool SelfInverse(const operation_index Operation);
	bool SelfInverseTo(const _IntegerNumeral& rhs, const operation_index Operation) const;
	bool NonTrivialGCF(const _IntegerNumeral& rhs) const;	// digital operation
	bool ResetLHSRHSToGCF(_IntegerNumeral& rhs);
	void LHSRHSLargeRemoveVisibleNonCommonFactors(_IntegerNumeral& rhs);

// _IntegerNumeral specific functions
	static bool IsLegalIntegerString(const char* x) ALL_PTR_NONNULL;
	static size_t LengthOfLegalIntegerSubstring(const char* x) ALL_PTR_NONNULL;

	bool AsSignedLong(signed long& Result) const;
	bool SmallDifference(const _IntegerNumeral& rhs, signed long& Result) const;
	bool ReduceAbsValByN(unsigned long N);
	void RearrangeSum(_IntegerNumeral& rhs);
	bool ForceRearrangeSum(_IntegerNumeral& rhs);
	void RearrangeProduct(_IntegerNumeral& rhs);
	bool ForceRearrangeProduct(_IntegerNumeral& rhs);
	void DestructiveNormalProductFormWithRHS(_IntegerNumeral& rhs);

	friend void swap(_IntegerNumeral& lhs, _IntegerNumeral& rhs)
		{	std::swap(lhs.ShortInteger,rhs.ShortInteger);
			std::swap(lhs.VFT2Idx,rhs.VFT2Idx);};

private:
	typedef void (_IntegerNumeral::*FinishAssignmentAuxFunc)(const _IntegerNumeral& src);
	typedef bool (_IntegerNumeral::*OpLT_AuxFunc)(const _IntegerNumeral& rhs) const;
	typedef size_t (_IntegerNumeral::*LengthOfSelfNameAuxFunc)() const;
	typedef void (_IntegerNumeral::*ConstructSelfNameAuxFunc)(char* Name) const;
	typedef bool (_IntegerNumeral::*SyntaxOKAuxFunc)() const;

#define ZeroOffset_VFT2 4
#define MaxRepresentation_INT 3
	static SyntaxOKAuxFunc SyntaxOKAux[MaxRepresentation_INT];
	static FinishAssignmentAuxFunc FinishAssignmentAuxV2[MaxRepresentation_INT];
	static ConstructSelfNameAuxFunc ConstructSelfNameAux2[2*ZeroOffset_VFT2+1];
	static LengthOfSelfNameAuxFunc LengthOfSelfNameAux2[2*ZeroOffset_VFT2+1];
	static OpLT_AuxFunc OpLT_SameTypeClassify[2*ZeroOffset_VFT2+1];
#undef MaxRepresentation_INT
#undef ZeroOffset_VFT2

	// static array functions
	bool SyntaxOKAuxSmallInt() const;
	bool SyntaxOKAuxLargeInt() const;
	bool SyntaxOKAuxHardCodedInt() const;

	void FinishAssignmentAuxSmallInt(const _IntegerNumeral& src);
	void FinishAssignmentAuxLargeInt(const _IntegerNumeral& src);
	void FinishAssignmentAuxHardCodedInt(const _IntegerNumeral& src);

	void ConstructSelfNamePositiveInt(char* Name) const;
	void ConstructSelfNameSmallPositiveInt(char* Name) const;
	void ConstructSelfNameSmallPositiveIntInv(char* Name) const;
	void ConstructSelfNamePositiveIntInv(char* Name) const;
	void ConstructSelfNameZeroInt(char* Name) const;
	void ConstructSelfNameSmallNegativeIntInv(char* Name) const;
	void ConstructSelfNameNegativeIntInv(char* Name) const;
	void ConstructSelfNameSmallNegativeInt(char* Name) const;
	void ConstructSelfNameNegativeInt(char* Name) const;

	size_t LengthOfSelfNamePositiveInt() const;
	size_t LengthOfSelfNameSmallPositiveInt() const;
	size_t LengthOfSelfNameSmallPositiveIntInv() const;
	size_t LengthOfSelfNamePositiveIntInv() const;
	size_t LengthOfSelfNameZeroInt() const;
	size_t LengthOfSelfNameNegativeIntInv() const;
	size_t LengthOfSelfNameSmallNegativeIntInv() const;
	size_t LengthOfSelfNameSmallNegativeInt() const;
	size_t LengthOfSelfNameNegativeInt() const;

	bool OpLT_TrivialFalse(const _IntegerNumeral& rhs) const;
	bool OpLT_1st2ndPositiveInt(const _IntegerNumeral& rhs) const;
	bool OpLT_1st2ndSmallPositiveInt(const _IntegerNumeral& rhs) const;
	bool OpLT_1st2ndSmallNegativeInt(const _IntegerNumeral& rhs) const;
	bool OpLT_1st2ndNegativeInt(const _IntegerNumeral& rhs) const;

	// for RearrangeSum
	unsigned long LargeSelfDivBy2();	// return value is remainder
	bool LHSRHSLargeEqualAbsVal(const _IntegerNumeral& rhs) const;
	void ReduceLargeLHSAbsValByN(unsigned long N);
	void LHSLargeRHSSmallOppSignRearrange(_IntegerNumeral& rhs);
	void LHSRHSLargeOppSignRearrange(_IntegerNumeral& rhs);
	void LargeAdjustAbsValByLarge(const _IntegerNumeral& rhs);
	void LargeAdjustAbsValByLargeTargetHighestDifferent(const _IntegerNumeral& rhs, unsigned long TargetIdx);
	void LHSRHSSmallOppSignRearrange(_IntegerNumeral& rhs);
	void LHSRHSLargeSameSignCondense(_IntegerNumeral& rhs);
	void LHSRHSLargeSameSignCondenseLHSLonger(_IntegerNumeral& rhs);
	void LHSRHSLargeSameSignCondenseSameLength(_IntegerNumeral& rhs);
	void LargeShrinkRepresentation(size_t StartIdx);
	void LHSLargeRHSSmallSameSignCondense(_IntegerNumeral& rhs);
	void LHSRHSSmallSameSignCondense(_IntegerNumeral& rhs);

	// for ForceRearrangeSum
	bool LHSRHSLargeSameSignForceCondense(_IntegerNumeral& rhs);
	bool LHSLargeRHSSmallSameSignForceCondense(_IntegerNumeral& rhs);
	bool LHSRHSSmallSameSignForceCondense(_IntegerNumeral& rhs);

	// for RearrangeProduct
	void LHSRHSSmallExactlyOneInvRearrangeProduct(_IntegerNumeral& rhs);
	void LHSRHSSmallZeroOrTwoInvRearrangeProduct(_IntegerNumeral& rhs);
	void LHSLargeRHSSmallZeroOrTwoInvRearrangeProduct(_IntegerNumeral& rhs);
	unsigned long MultiplyLargeByN(unsigned long N);	// NOTE: digital operation.  Returns Carry; does *not* do RAM manipulation.
	unsigned long RemainderOfLargeDivideByN(unsigned long N) const;	// NOTE: digital operation.  Returns Remainder.
	unsigned long LargeDivideByN(unsigned long N);	// NOTE: digital operation.  Returns remainder.

	// for ForceRearrangeProduct
	bool LHSLargeRHSSmallZeroOrTwoInvForceRearrangeProduct(_IntegerNumeral& rhs);
	void LHSLargeRHSSmallExactlyOneInvRearrangeProduct(_IntegerNumeral& rhs);
	bool LHSRHSSmallZeroOrTwoInvForceRearrangeProduct(_IntegerNumeral& rhs);
	bool LHSRHSLargeExactlyOneInvForceRearrangeProduct(_IntegerNumeral& rhs);
	void MultiplyByNIntoBuffer(unsigned long N,unsigned long* const ResultBuffer) const;
	bool LHSRHSLargeZeroOrTwoInvForceRearrangeProduct(_IntegerNumeral& rhs);
	bool ConstructScratchMultiples(unsigned long*& ScratchMultiples) const;
	bool ConstructQuotientWorkspace(const _IntegerNumeral& Divisor,unsigned long*& QuotientWorkspace) const;
	void LargeDivideByLarge(const _IntegerNumeral& Divisor,unsigned long*& QuotientWorkspace, const unsigned long* const ScratchMultiples);
};

inline bool operator==(signed short lhs, const _IntegerNumeral& rhs) {return rhs==lhs;}
inline bool operator==(unsigned short lhs, const _IntegerNumeral& rhs) {return rhs==lhs;}

inline bool operator>(const _IntegerNumeral& lhs, const _IntegerNumeral& rhs) {return rhs<lhs;}
inline bool operator<=(const _IntegerNumeral& lhs, const _IntegerNumeral& rhs) {return !(rhs<lhs);}
inline bool operator>=(const _IntegerNumeral& lhs, const _IntegerNumeral& rhs) {return !(lhs<rhs);}

#endif
