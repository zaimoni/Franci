// LenName.cxx
// implementation for LengthOfSelfName virtual functions

#include "MetaCon3.hxx"
#include "Equal.hxx"
#include "StdAdd.hxx"
#include "StdMult.hxx"
#include "GCF.hxx"
#include "ClauseN.hxx"
#include "PhraseN.hxx"
#include "QState.hxx"

#include "Clause2.hxx"
#include "Phrase2.hxx"
#include "Interval.hxx"

#include "Phrase1.hxx"
#include "Class.hxx"
#include "Quantify.hxx"
#include "Variable.hxx"

#include "TruthVal.hxx"
#include "Unparsed.hxx"

#include "Keyword1.hxx"

#include "Zaimoni.STL/string.h"

// AbstractClass
#define UNNAMED_CLASS "(Unnamed class)"
#define UNNAMED_SET "(Unnamed set)"
#define UNNAMED_PROPER_CLASS "(Unnamed proper class)"
#define EMPTY_STATEMENT "(Empty Statement)"
#define BAD_VARIABLE_SYNTAX "(Variable with syntax error)"

// multiplicative inverse
#define MULT_INV_TEXT "<sup>-1</sup>"

// MetaConnective
const char* const MetaConnectiveNames[8] =	{
											LogicKeyword_AND,	// AND
											LogicKeyword_OR,	// OR
											LogicKeyword_IFF,	// IFF
											LogicKeyword_XOR,	// XOR
											LogicKeyword_NXOR,	// NXOR
											LogicKeyword_NIFF,	// NIFF
											LogicKeyword_NOR,	// NOR
											LogicKeyword_NAND	// NAND
											};

static_assert(sizeof(const char*)*(LogicalNAND_MC-LogicalAND_MC+1)==sizeof(MetaConnectiveNames));

// MetaQuantifier
const char* const QuantificationNames[5] =	{
											PredCalcKeyword_FORALL,
											PredCalcKeyword_THEREIS,
											PredCalcKeyword_FREE,
											PredCalcKeyword_NOTFORALL,
											PredCalcKeyword_THEREISNO
											};

static_assert(sizeof(const char*)*(ThereIsNot_MC-ForAll_MC+1)==sizeof(QuantificationNames));

// Actual implementations of ConstructSelfLengthNameAux()
// NOTE: Name, in the next two routines, relies on implicit null termination.
void SnipText2(char*& Name, size_t nonstrict_lb, size_t strict_ub)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/2000
	assert(Name);
	assert(nonstrict_lb<strict_ub);
	size_t NameLength = ArraySize(Name);
	assert(NameLength>=strict_ub);
	assert(NameLength>strict_ub-nonstrict_lb);
	if (strict_ub<NameLength)
		memmove(Name+nonstrict_lb,Name+strict_ub,NameLength-strict_ub);
	NameLength-=(strict_ub-nonstrict_lb);
	Name = REALLOC(Name,NameLength);
}

// This has to tolerate NULL pointers: it may be called from a SyntaxOKAux failure
std::string ConstructPrefixArgList(const MetaConcept* const* const ArgArray)
{
	std::string ret("(");
	if (ArgArray) {
		const size_t ub = ArraySize(ArgArray);
		size_t i = 0;
		do {
			if (ArgArray[i]) ret += ArgArray[i]->to_s();
			else ret += "\"\"";
			if (++i >= ub) break;
			ret += ",";
		} while (true);
	};
	ret += ")";
	return ret;
}

#ifndef USE_TO_S
void
ConstructPrefixArgList(char* const PrefixArgListStart,const MetaConcept* const * const ArgArray)
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	char* ArgListPointer = PrefixArgListStart;
	*ArgListPointer++ = '(';

	if (ArgArray)
		{
		size_t i = 0;
		do	{
			if (NULL!=ArgArray[i])
				{
				ArgArray[i]->ConstructSelfName(ArgListPointer);
				ArgListPointer += ArgArray[i]->LengthOfSelfName();
				}
			else
				ArgListPointer = postpend_string<2>(ArgListPointer,"\"\"");
			if (++i>=ArraySize(ArgArray)) break;
			*ArgListPointer++ =',';
			}	
		while(true);
		};

	ArgListPointer[0] = ')';
}

size_t LengthOfPrefixArgList(const MetaConcept * const * const ArgArray)
{
	if (ArgArray)
		{
		size_t i = ArraySize(ArgArray);
		size_t Length = i+1;
		do	Length += ArgArray[--i] ? ArgArray[i]->LengthOfSelfName() : 2;
		while(0<i);
		return Length;
		};
	return 2;
}
#endif

std::string MetaConcept::to_s() const {
	std::string ret(to_s_aux());

	if (MultiPurposeBitmap & LogicalNegated_VF) ret = "NOT " + ret;
	else if (MultiPurposeBitmap & StdAdditionInv_VF) ret = "-" + ret;
	if (MultiPurposeBitmap & StdMultInv_VF) ret += MULT_INV_TEXT;

	return ret;
}

#ifndef USE_TO_S
void MetaConcept::ConstructSelfName(char*& Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 3/22/2000
	bool TopLevel = true;
	const size_t Correction = ((MultiPurposeBitmap & LogicalNegated_VF) ? 4 : 0)
							 +((MultiPurposeBitmap & StdAdditionInv_VF) ? 1 : 0)
							 +((MultiPurposeBitmap & StdMultInv_VF) ? (sizeof(MULT_INV_TEXT)-1) : 0);
//	DEBUG_LOG(name());
	const size_t BaseLength = LengthOfSelfName();
	if (!Name)
		{
		Name = _new_buffer<char>(BaseLength+Correction);
		if (!Name)
			{
			WARNING("Warning: RAM failure while constructing name");
			return;
			}
		}
	else
		TopLevel = false;

	if      (MultiPurposeBitmap & LogicalNegated_VF)
		{
		strcpy(Name,"NOT ");
		ConstructSelfNameAux(Name+4);
		}
	else if (MultiPurposeBitmap & StdAdditionInv_VF)
		{
		Name[0] = '-';
		ConstructSelfNameAux(Name+1);
		}
	else
		ConstructSelfNameAux(Name);

	if (MultiPurposeBitmap & StdMultInv_VF)
		strcpy(Name+BaseLength+Correction-(sizeof(MULT_INV_TEXT)-1),MULT_INV_TEXT);

	if (!TopLevel) return;

	// cleaning of arglists
	// (FORALL __ IN __)(FORALL __ IN __): merge
	// (THEREIS __ IN __)(THEREIS __ IN __): merge
	// (THEREISNO __ IN __)(THEREISNO __ IN __): merge
	// (NOTFORALL __ IN __)(NOTFORALL __ IN __): merge
	// (__ FREE)(__ FREE): merge
	//! \todo MOVE THIS [cleaning of arglists] TO SAFER LOCATION
FullRestart:
	{	// min. size pattern we can reduce is 16 chars long; min size half-pattern is 8 chars long
	size_t RParens1 = 7;
	size_t NameLen = ArraySize(Name);
	if (16>NameLen) return;
Restart1:
	if (RParens1<NameLen-8)
		{
		while(')'!=Name[RParens1] || '('!=Name[RParens1+1])
			if (++RParens1>=NameLen-8)
				return;
		size_t LParens1 = RParens1-1;
		// if this crashes, there's a bug in a ConstructSelfNameAux version
		while('('!=Name[LParens1])
			{
			assert(0<LParens1);
			--LParens1;
			};
		if (7>RParens1-LParens1)
			{	// not enough space
			RParens1+=2;
			goto Restart1;
			};
		size_t RParens2 = RParens1+2;
		// if this crashes, there's a bug in a ConstructSelfNameAux version
		while(')'!=Name[RParens2]) RParens2++;	
		if (8>RParens2-RParens1)
			{	// not enough space
			RParens1+=2;
			goto Restart1;
			};
		// We now have (_)(_)
		{
		size_t i = LParens1+1;
		while(')'!=Name[i]) ++i;
		if (i!=RParens1)
			{	// FAIL #1: (_)_) on left
			RParens1+=2;
			goto Restart1;
			};
		}
		{
		size_t i = RParens2-1;
		while('('!=Name[i]) --i;
		if (i!=RParens1+1)
			{	// FAIL #2: (_(_) on right
			RParens1+=2;
			goto Restart1;
			};
		}
		// think about actual cases:	(FORALL __ IN __)(FORALL __ IN __): merge
		//								(THEREIS __ IN __)(THEREIS __ IN __): merge
		//								(THEREISNO __ IN __)(THEREISNO __ IN __): merge
		//								(NOTFORALL __ IN __)(NOTFORALL __ IN __): merge
		//								(__ FREE)(__ FREE): merge
		if (   'E'==Name[RParens1-1]
			&& 'E'==Name[RParens2-1])
			{	// check for (__ FREE)(__ FREE): merge
			if (   0==strncmp(Name+(RParens1-5)," FRE",4)
				&& 0==strncmp(Name+(RParens2-5)," FRE",4))
				{	// (__ IS FREE)(__ IS FREE): merge with SnipText operation
				Name[RParens1-8]=',';
				SnipText2(Name,RParens1-7,RParens1+2);
				goto FullRestart;
				};
			};
		// other 4 all require " IN ___": check for this first
		{
		char Last4Buffer[4] = {'\x00','\x00','\x00','\x00'};
		size_t BacktrackIdx = 1;
		while(Name[RParens1-BacktrackIdx]==Name[RParens2-BacktrackIdx] && '('!=Name[RParens1-BacktrackIdx])
			{
			memmove(Last4Buffer+1,Last4Buffer,3);
			Last4Buffer[0]=Name[RParens1-BacktrackIdx];
			if (   0==strncmp(Last4Buffer," IN ",4))
				{
				SUCCEED_OR_DIE(4<BacktrackIdx);
				if		('F'==Name[LParens1+1])
					{	// check for (FORALL __ IN __)(FORALL __ IN __): merge
					if (   LParens1+8<RParens1-BacktrackIdx
						&& RParens1+9<RParens2-BacktrackIdx
						&& 0==strncmp(Name+(LParens1+2),"ORALL ",6)
						&& 0==strncmp(Name+(RParens1+2),"FORALL ",7))
						{	// (FORALL __ IN __)(FORALL __ IN __): splice it down
						Name[RParens1-BacktrackIdx]=',';
						SnipText2(Name,RParens1-BacktrackIdx+1,RParens1+9);
						goto FullRestart;
						}
					}
				else if ('T'==Name[LParens1+1])
					{	// check for (THEREIS __ IN __)(THEREIS __ IN __): merge
						//		     (THEREISNO __ IN __)(THEREISNO __ IN __): merge	
					if (   LParens1+9<RParens1-BacktrackIdx
						&& RParens1+10<RParens2-BacktrackIdx
						&& 0==strncmp(Name+(LParens1+2),"HEREIS",6)
						&& 0==strncmp(Name+(RParens1+2),"THEREIS",7))
						{	// last diagnosis, then splice it down
						if   (' '==Name[LParens1+8] && ' '==Name[RParens1+9])
							{	// (THEREIS __ IN __)(THEREIS __ IN __): splice it
							Name[RParens1-BacktrackIdx]=',';
							SnipText2(Name,RParens1-BacktrackIdx+1,RParens1+10);
							goto FullRestart;
							}
						else if (   LParens1+9<RParens1-BacktrackIdx
								 && RParens1+10<RParens2-BacktrackIdx
								 && 0==strncmp(Name+(LParens1+8),"NO ",3)
								 && 0==strncmp(Name+(RParens1+9),"NO ",3))
							{	// (THEREISNO __ IN __)(THEREISNO __ IN __): splice down
							Name[RParens1-BacktrackIdx]=',';
							SnipText2(Name,RParens1-BacktrackIdx+1,RParens1+12);
							goto FullRestart;
							};
						}
					}
				else if ('N'==Name[LParens1+1])
					{	// check for (NOTFORALL __ IN __)(NOTFORALL __ IN __): merge
					if (   LParens1+11<RParens1-BacktrackIdx
						&& RParens1+12<RParens2-BacktrackIdx
						&& 0==strncmp(Name+(LParens1+2),"OTFORALL ",9)
						&& 0==strncmp(Name+(RParens1+2),"NOTFORALL ",10))
						{	// (NOTFORALL __ IN __)(NOTFORALL __ IN __) : splice it down
						Name[RParens1-BacktrackIdx]=',';
						SnipText2(Name,RParens1-BacktrackIdx+1,RParens1+12);
						goto FullRestart;
						}
					};
				break;
				};
			BacktrackIdx++;
			}
		}
		}
	}
}
#endif

std::string AbstractClass::to_s_aux() const
{
	if (!ClassName.empty()) return ClassName;
	if (Arg1.empty()) return UNNAMED_CLASS;

	const bool WantSetBraces = Arg1->IsExplicitConstant() || Arg1->IsTypeMatch(LinearInterval_MC, &Integer);
	std::string ret;

	if (WantSetBraces) ret += "{";
	ret += Arg1->to_s();
	if (WantSetBraces) ret += "}";
	return ret;
}

#ifndef USE_TO_S
void AbstractClass::ConstructSelfNameAux(char* Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: 12/29/2002
	if (ClassName.empty())
		{
		if (Arg1.empty())
			strcpy(Name,UNNAMED_CLASS);
		else{
			bool WantSetBraces = false;
			if (    Arg1->IsExplicitConstant()
				|| (Arg1->IsTypeMatch(LinearInterval_MC,&Integer)))
				WantSetBraces = true;

			if (WantSetBraces) *Name++ = '{';

			Arg1->ConstructSelfName(Name);
			Name+=Arg1->LengthOfSelfName();

			if (WantSetBraces)
				*Name++ = '}';
			}
		}
	else
		strcpy(Name,ClassName.c_str());
}
#endif

// technically, this is just for infix 2-ary clauses
void Clause2Arg::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	LHS_Arg1->ConstructSelfName(Name);
	Name += LHS_Arg1->LengthOfSelfName();

	*Name++ = ' ';
	Name = postpend_string(Name,((NULL!=ClauseKeyword) ? ClauseKeyword : "\"\""));			
	*Name++ = ' ';

	RHS_Arg2->ConstructSelfName(Name);
}

void ClauseNArg::ConstructSelfNameAux(char* Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	(this->*ConstructSelfNameAuxArray[ExactType()-MinClauseNIdx_MC])(Name);
}

void ClauseNArg::ConstructSelfNamePrefixArglist(char* Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	Name = postpend_string(Name,((NULL!=ClauseKeyword) ? ClauseKeyword : "\"\""));			
	ConstructPrefixArgList(Name);
}

#ifndef USE_TO_S
void GCF::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: 8/11/2001
	strcpy(Name,GCF_NAME);
	ConstructPrefixArgList(Name+(sizeof(GCF_NAME)-1));
}
#endif

void EqualRelation::ConstructSelfNameAux(char* Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	(this->*SelfNameAux[array_index()])(Name);
}

void EqualRelation::ConstructSelfNameAuxALLEQUAL(char* Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	size_t i = 0;
	do	{
		if (    ArgArray[i]->IsExactType(ALLEQUAL_MC)
		    || (   ArgArray[i]->IsExactType(ALLDISTINCT_MC)
				&& static_cast<EqualRelation*>(ArgArray[i])->fast_size()))
			*Name++ ='(';

		ArgArray[i]->ConstructSelfName(Name);
		Name+=ArgArray[i]->LengthOfSelfName();
		if (    ArgArray[i]->IsExactType(ALLEQUAL_MC)
		    || (    ArgArray[i]->IsExactType(ALLDISTINCT_MC)
				&& static_cast<EqualRelation*>(ArgArray[i])->fast_size()))
			*Name++ =')';

		if (fast_size()-1>i)
			Name = postpend_string<2>(Name,"==");
		}
	while(fast_size()> ++i);
}

void EqualRelation::ConstructSelfNameAuxALLDISTINCT(char* Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	if (2==fast_size())
		{	// infix != case
		size_t i = 0;
		do	{
			if (    ArgArray[i]->IsExactType(ALLEQUAL_MC)
				|| (    ArgArray[i]->IsExactType(ALLDISTINCT_MC)
					&& static_cast<EqualRelation*>(ArgArray[i])->fast_size()))
				*Name++ ='(';

			ArgArray[i]->ConstructSelfName(Name);
			Name+=ArgArray[i]->LengthOfSelfName();
			if (    ArgArray[i]->IsExactType(ALLEQUAL_MC)
				|| (    ArgArray[i]->IsExactType(ALLDISTINCT_MC)
					&& static_cast<EqualRelation*>(ArgArray[i])->fast_size()))
				*Name++ =')';

			if (1>i)
				Name = postpend_string<2>(Name,"!=");
			}
		while(2> ++i);
		}
	else{	// prefix ALLDISTINCT case
		strcpy(Name,EqualRelation_ALLDISTINCT);
		ConstructPrefixArgList(Name+strlen(EqualRelation_ALLDISTINCT));
		}
}

void EqualRelation::ConstructSelfNameAuxDISTINCTFROMALLOF(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2000
	if (3==fast_size())
		{
		ArgArray[1]->ConstructSelfName(Name);
		Name += ArgArray[1]->LengthOfSelfName();
		Name = postpend_string<2>(Name,"!=");
		ArgArray[0]->ConstructSelfName(Name);
		Name += ArgArray[0]->LengthOfSelfName();
		Name = postpend_string<2>(Name,"!=");
		ArgArray[2]->ConstructSelfName(Name);
		}
	else{
		*Name++ ='(';
		ArgArray[0]->ConstructSelfName(Name);
		Name += ArgArray[0]->LengthOfSelfName();
		*Name++ =' ';
		Name = postpend_string<17>(Name,EqualRelation_DISTINCTFROMALLOF);
		*Name++ =' ';
		ConstructCommaListVarNames(Name,1,fast_size());
		Name[0]=')';
		};
}

void EqualRelation::ConstructSelfNameAuxEQUALTOONEOF(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2000
	*Name++ ='(';
	ArgArray[0]->ConstructSelfName(Name);
	Name += ArgArray[0]->LengthOfSelfName();
	*Name++ =' ';
	Name = postpend_string<12>(Name,EqualRelation_EQUALTOONEOF);
	*Name++ =' ';
	ConstructCommaListVarNames(Name,1,fast_size());
	Name[0]=')';
}

void EqualRelation::ConstructSelfNameAuxNOTALLDISTINCT(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	strcpy(Name,EqualRelation_NOTALLDISTINCT);
	ConstructPrefixArgList(Name+strlen(EqualRelation_NOTALLDISTINCT));
}

void EqualRelation::ConstructSelfNameAuxNOTALLEQUAL(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	strcpy(Name,EqualRelation_NOTALLEQUAL);
	ConstructPrefixArgList(Name+strlen(EqualRelation_NOTALLEQUAL));
}

void LinearInterval::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/19/2002
	//! \todo FIX this when non-default domains possible
	if (Integer==*IntervalDomain)
		{
		LHS_Arg1->ConstructSelfNameAux(Name);
		Name += LHS_Arg1->LengthOfSelfName();
		if (LeftPointOpen && !LHS_Arg1->IsExactType(LinearInfinity_MC))
			Name = postpend_string<2>(Name,"+1");
		Name = postpend_string<3>(Name,"...");
		RHS_Arg2->ConstructSelfNameAux(Name);
		Name += RHS_Arg2->LengthOfSelfName();
		if (RightPointOpen && !RHS_Arg2->IsExactType(LinearInfinity_MC))
			Name = postpend_string<2>(Name,"-1");
		}
	else{	// normal-looking interval
		if (LeftPointOpen)
			*Name++ = (KarlStrombergIntervals) ? ']' : '(';
		else
			*Name++ = '[';
		LHS_Arg1->ConstructSelfNameAux(Name);
		Name += LHS_Arg1->LengthOfSelfName();

		*Name++ = ',';
		
		RHS_Arg2->ConstructSelfNameAux(Name);
		Name += RHS_Arg2->LengthOfSelfName();
		if (RightPointOpen)
			*Name++ = (KarlStrombergIntervals) ? '[' : ')';
		else
			*Name++ = ']';
		}
}

void MetaConceptWithArgArray::ConstructPrefixArgList(char* const PrefixArgListStart) const
{	// FORMALLY CORRECT: Kenneth Boyd, 5/19/2006
	::ConstructPrefixArgList(PrefixArgListStart,ArgArray);
}

std::string MetaConceptWithArgArray::ConstructPrefixArgList() const
{
}

//! \todo FIX: AESTHETIC: MetaConnective::ConstructSelfName
//!		** needs to be aware of variable-negation issues at arity 2 [do this as a retrofit]
//!		** should print IFF(A,NOT B) as XOR(A,B)
//!		** should print IFF(NOT A,B) as XOR(A,B)
void MetaConnective::ConstructSelfNameAux(char* Name) const		// overwrites what is already there
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	const size_t i = array_index();
	strcpy(Name,MetaConnectiveNames[i]);
	ConstructPrefixArgList(Name+strlen(MetaConnectiveNames[i]));
}

void
MetaConceptWithArgArray::ConstructCommaListVarNames(char*& CommaListStart, size_t MinIdx, size_t StrictMaxIdx) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2000
	if (size()>MinIdx)
		{
		if (fast_size()<StrictMaxIdx) StrictMaxIdx=fast_size();
		size_t i = MinIdx;
		do	{
			if (ArgArray[i])
				{
				ArgArray[i]->ConstructSelfName(CommaListStart);
				CommaListStart += ArgArray[i]->LengthOfSelfName();
				}
			else{
				CommaListStart = postpend_string<2>(CommaListStart,"\"\"");
				};
			if (StrictMaxIdx<= ++i)
				break;
			*CommaListStart++ =',';
			}
		while(true);
		}
	else
		CommaListStart = postpend_string<2>(CommaListStart,"\"\"");
}

void
MetaConceptWithArgArray::ConstructCommaListVarNames(char*& CommaListStart) const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	if (!ArgArray.empty())
		{
		size_t i = 0;
		do	{
			if (ArgArray[i])
				{
				ArgArray[i]->ConstructSelfName(CommaListStart);
				CommaListStart += ArgArray[i]->LengthOfSelfName();
				}
			else{
				CommaListStart = postpend_string<2>(CommaListStart,"\"\"");
				};
			if (fast_size()<= ++i) break;
			*CommaListStart++ =',';
			}
		while(true);
		}
	else
		CommaListStart = postpend_string<2>(CommaListStart,"\"\"");
}

void Phrase1Arg::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	(this->*VFTable2Lookup[ExactType()-MinPhrase1Idx_MC].ConstructSelfName)(Name);
}

void Phrase1Arg::ConstructSelfName_Prefix(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	Name = postpend_string(Name,((NULL!=PhraseKeyword) ? PhraseKeyword : "\"\""));

	*Name++ =' ';
	if (!Arg1.empty())
		{
		Arg1->ConstructSelfName(Name);
		return;
		};
	strcpy(Name,"\"\"");
}

void Phrase1Arg::ConstructSelfName_FunctionLike(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	Name = postpend_string(Name,((NULL!=PhraseKeyword) ? PhraseKeyword : "\"\""));
	*Name++ ='(';
	if (!Arg1.empty())
		{
		Arg1->ConstructSelfName(Name);
		Name += Arg1->LengthOfSelfName();
		}
	else
		Name = postpend_string<2>(Name,"\"\"");
	*Name =')';
}

// technically, this is just for infix 2-ary phrases
void Phrase2Arg::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/13/2000
	LHS_Arg1->ConstructSelfName(Name);
	Name += LHS_Arg1->LengthOfSelfName();

	*Name++ =' ';
	Name = postpend_string(Name,((NULL!=PhraseKeyword) ? PhraseKeyword : "\"\""));
	*Name++ =' ';

	RHS_Arg2->ConstructSelfName(Name);
}

void PhraseNArg::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	(this->*ConstructSelfNameAuxArray[ExactType()-MinPhraseNIdx_MC])(Name);
}

void
PhraseNArg::ConstructSelfNamePrefixCommaListVarNames(char* const Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	char* NamePtr = Name;
	NamePtr = postpend_string(NamePtr,((NULL!=PhraseKeyword) ? PhraseKeyword : "\"\""));
	*NamePtr++ =' ';
	ConstructCommaListVarNames(NamePtr);
}

void
PhraseNArg::ConstructSelfNamePostfixCommaListVarNames(char* const Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/21/1999
	char* NamePtr = Name;
	ConstructCommaListVarNames(NamePtr);
	*NamePtr++ = ' ';
	strcpy(NamePtr,((NULL!=PhraseKeyword) ? PhraseKeyword : "\"\""));
}

// The self-name is of the form <reverse-order quantification list><statement>
// the estimator is somewhat unreliable: variables with the same quantification and domain
// should be compacted into a list of var-names later
void
QuantifiedStatement::ConstructSelfNameAux(char* Name) const	// overwrites what is already there
{	// FORMALLY CORRECT: 2/12/2000
	if (ArgArray.empty())
		strcpy(Name,EMPTY_STATEMENT);
	else{
		size_t i = fast_size();
		do	if (ArgArray[--i])
				{
				ArgArray[i]->ConstructSelfName(Name);
				Name += ArgArray[i]->LengthOfSelfName();
				}
			else{
				Name = postpend_string<2>(Name,"()");
				}
		while(1<i);

		*Name++ ='(';

		if (ArgArray[0])
			{
			ArgArray[0]->ConstructSelfName(Name);
			Name += ArgArray[0]->LengthOfSelfName();
			}
		else{
			Name = postpend_string<2>(Name,"\"\"");
			};
		Name[0]=')';
		};
}

// The self-name is of the form ([Quantification-type] Var-name [IN <class-name>]|[IS FREE])
void MetaQuantifier::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/29/1999
	*Name++ ='(';

	if (IsExactType(Free_MC))
		{
		memmove(Name,VariableName,_msize(VariableName));
		Name+=_msize(VariableName);
		Name = postpend_string<4>(Name," IS ");
		memmove(Name,QuantificationNames[ExactType()-ForAll_MC],
				strlen(QuantificationNames[ExactType()-ForAll_MC]));
		Name+=strlen(QuantificationNames[ExactType()-ForAll_MC]);
		}
	else{
		memmove(Name,QuantificationNames[ExactType()-ForAll_MC],
				strlen(QuantificationNames[ExactType()-ForAll_MC]));
		Name+=strlen(QuantificationNames[ExactType()-ForAll_MC]);
		*Name++ =' ';
		memmove(Name,VariableName,_msize(VariableName));
		Name+=_msize(VariableName);
		Name = postpend_string<4>(Name," IN ");
		Arg1->ConstructSelfName(Name);
		Name += Arg1->LengthOfSelfName();
		}

	Name[0]=')';
}

// basic version: n-1 + signs, n args
// each arg that isn't higher precedence gets a pair of parentheses
// NOTE: a 0-ary sum returns the universal result 0
// NOTE: a 1-ary sum returns the single arg
void StdAddition::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/12/2000
	if (_IsZero())
		{
		Name[0] = '0';
		return;
		};
	char* VRName = Name;
	if (   !ArgArray[0]->NeverNeedsParentheses()
		&& !ArgArray[0]->IsExactType(StdMultiplication_MC))
		{
		*VRName++ ='(';
		ArgArray[0]->ConstructSelfName(VRName);
		VRName += ArgArray[0]->LengthOfSelfName(); 
		*VRName++ =')';
		}
	else{
		ArgArray[0]->ConstructSelfName(VRName);
		VRName += ArgArray[0]->LengthOfSelfName(); 
		};

	if (1<fast_size())
		{
		size_t i = 1;
		do	{
			*VRName++ = '+';
			if (   !ArgArray[i]->NeverNeedsParentheses()
				&& !ArgArray[i]->IsExactType(StdMultiplication_MC))
				{
				*VRName++ ='(';
				ArgArray[i]->ConstructSelfName(VRName);
				VRName += ArgArray[i]->LengthOfSelfName(); 
				*VRName++ =')';
				}
			else{
				ArgArray[i]->ConstructSelfName(VRName);
				VRName += ArgArray[i]->LengthOfSelfName();
				};
			}
		while(fast_size()> ++i);
		};
}

void StdMultiplication::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	if (_IsOne())
		{
		Name[0] = '1';
		return;
		};
	char* VRName = Name;
	if (!ArgArray[0]->NeverNeedsParentheses())
		{
		*VRName++ ='(';
		ArgArray[0]->ConstructSelfName(VRName);
		VRName += ArgArray[0]->LengthOfSelfName(); 
		*VRName++ =')';
		}
	else{
		ArgArray[0]->ConstructSelfName(VRName);
		VRName += ArgArray[0]->LengthOfSelfName(); 
		};

	if (1<fast_size())
		{
		size_t i = 1;
		do	{
			*VRName++ = '\xb7';
			if (!ArgArray[i]->NeverNeedsParentheses())
				{
				*VRName++ ='(';
				ArgArray[i]->ConstructSelfName(VRName);
				VRName += ArgArray[i]->LengthOfSelfName(); 
				*VRName++ =')';
				}
			else{
				ArgArray[i]->ConstructSelfName(VRName);
				VRName += ArgArray[i]->LengthOfSelfName();
				};
			}
		while(fast_size()> ++i);
		};
}

void UnparsedText::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	size_t TextLength = Text.empty() ? 0 : strlen(Text);
	if (IsUninterpreted())
		{
		if (0<TextLength) memmove(Name,Text,TextLength);
		return;
		}
	if (IsHTMLStartTag())
		{
		Name[0]='<';
		if (0<TextLength) memmove(Name+1,Text,TextLength);
		Name[TextLength+1]='>';
		return;
		};
	if (IsHTMLTerminalTag())
		{
		strcpy(Name,"</");
		if (0<TextLength) memmove(Name+2,Text,TextLength);
		Name[TextLength+2]='>';
		return;
		};
	if (IsJSEntity())
		{
		Name[0]='&';
		if (0<TextLength) memmove(Name+1,Text,TextLength);
		Name[TextLength+1]=';';
		return;
		}
	if (0<TextLength) memmove(Name+1,Text,TextLength);
	if		(IsLogicKeyword())
		{
		Name[0]='\'';
		Name[TextLength+1]='\'';
		}
	else if (IsPredCalcKeyword())
		{
		Name[0]='!';
		Name[TextLength+1]='!';
		}
	else{
		Name[0]='"';
		Name[TextLength+1]='"';
		}
}

std::string Variable::to_s_aux() const { return SyntaxOK() ? ViewKeyword() : BAD_VARIABLE_SYNTAX; }

#ifndef USE_TO_S
void Variable::ConstructSelfNameAux(char* Name) const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/31/2001
	strcpy(Name,(SyntaxOK() ? ViewKeyword() : BAD_VARIABLE_SYNTAX));
}
#endif

// Actual implementations of LengthOfSelfName()
#ifndef USE_TO_S
size_t AbstractClass::LengthOfSelfName() const
{	// FORMALLY CORRECT: 12/29/2002
	if (!ClassName.empty()) return strlen(ClassName.c_str());
	if (Arg1.empty()) return sizeof(UNNAMED_CLASS)-1;

	size_t CorrectLength = Arg1->LengthOfSelfName();
	if (Arg1->IsExplicitConstant() || Arg1->IsTypeMatch(LinearInterval_MC,&Integer))
		CorrectLength += 2;
	return CorrectLength;
}
#endif

//! \todo FIX: These hard-code infix 2-ary clauses without parentheses blocking.
//! other paradigms will require moving the hard-coding to virtual functions.
size_t Clause2Arg::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	size_t Length = ClauseKeyword ? 2+strlen(ClauseKeyword) : 4;
	Length += LHS_Arg1->LengthOfSelfName();
	Length += RHS_Arg2->LengthOfSelfName();
	return Length;
}

// text I/O functions
size_t ClauseNArg::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	return (this->*LengthOfSelfNameAuxArray[ExactType()-MinClauseNIdx_MC])();
}

size_t ClauseNArg::LengthOfSelfNamePrefixArglist() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/27/1999
	return LengthOfPrefixArgList()+((NULL!=ClauseKeyword) ? strlen(ClauseKeyword) : 2);
}

#ifndef USE_TO_S
size_t GCF::LengthOfSelfName() const
{	// FORMALLY CORRECT: 8/11/2001
	return (sizeof(GCF_NAME)-1)+LengthOfPrefixArgList();
}
#endif

// text I/O functions
// ALLEQUAL: use n-1 infix == operators
// ALLDISTINCT: n-ary prefix keyword, paren arglist; 2-ary uses != operator
// DISTINCTFROMALLOF: n-ary prefix keyword; 3-ary uses != operator, others use EQUALTOONEOF pattern
// EQUALTOONEOF: n-ary prefix keyword: 1st arg before, n-1 in commalist after
// NOTALLEQUAL: n-ary prefix keyword, paren arglist
// NOTALLDISTINCT: n-ary prefix keyword, paren arglist
//! \todo autoformat code should respond to infix operators ==, !=
size_t EqualRelation::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	return (this->*LengthOfSelfNameAux[array_index()])();
}

size_t EqualRelation::LengthOfSelfNameALLEQUAL() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	// We use n-1 infix operators ==.  Each argument that invokes != or == requires parentheses.
	size_t Length = strlen(SymbolEqual)*(fast_size()-1);
	size_t i = fast_size();
	do	{
		Length+=ArgArray[--i]->LengthOfSelfName();
		if (    ArgArray[i]->IsExactType(ALLEQUAL_MC)
		    || (    ArgArray[i]->IsExactType(ALLDISTINCT_MC)
				&& static_cast<EqualRelation*>(ArgArray[i])->fast_size()))
			Length += 2;
		}
	while(0<i);
	return Length;
}

size_t EqualRelation::LengthOfSelfNameALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	if (2==fast_size())
		{	// infix operator !=
		size_t Length =  strlen(SymbolNotEqual)
						+ArgArray[0]->LengthOfSelfName()
						+ArgArray[1]->LengthOfSelfName();		
		if (    ArgArray[0]->IsExactType(ALLEQUAL_MC)
		    || (    ArgArray[0]->IsExactType(ALLDISTINCT_MC)
				&& static_cast<EqualRelation*>(ArgArray[0])->fast_size()))
			Length += 2;
		if (    ArgArray[1]->IsExactType(ALLEQUAL_MC)
		    || (    ArgArray[1]->IsExactType(ALLDISTINCT_MC)
				&& static_cast<EqualRelation*>(ArgArray[1])->fast_size()))
			Length += 2;
		return Length;
		}
	else	// prefix keyword ALLDISTINCT
		return strlen(EqualRelation_ALLDISTINCT)+LengthOfPrefixArgList();
}

size_t EqualRelation::LengthOfSelfNameDISTINCTFROMALLOF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2000
	if (3==fast_size())
		return 2*strlen(SymbolNotEqual)
		      +ArgArray[0]->LengthOfSelfName()
			  +ArgArray[1]->LengthOfSelfName()
			  +ArgArray[2]->LengthOfSelfName();
	return 4+strlen(EqualRelation_DISTINCTFROMALLOF)
	        +ArgArray[0]->LengthOfSelfName()
		    +LengthOfCommaListVarNames(1,fast_size());
}

size_t EqualRelation::LengthOfSelfNameEQUALTOONEOF() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/10/2000
	return 4+strlen(EqualRelation_EQUALTOONEOF)
	        +ArgArray[0]->LengthOfSelfName()
		    +LengthOfCommaListVarNames(1,fast_size());
}

size_t EqualRelation::LengthOfSelfNameNOTALLDISTINCT() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	return strlen(EqualRelation_NOTALLDISTINCT)+LengthOfPrefixArgList();
}

size_t EqualRelation::LengthOfSelfNameNOTALLEQUAL() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/3/2000
	return strlen(EqualRelation_NOTALLEQUAL)+LengthOfPrefixArgList();
}

size_t LinearInterval::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/19/2002
	//! \todo FIX this when non-default domains possible [we'll subscript the non-default domain]
	if (Integer==*IntervalDomain)
		// Interval over integers: A...B
		return LHS_Arg1->LengthOfSelfName()+RHS_Arg2->LengthOfSelfName()+((LeftPointOpen && !LHS_Arg1->IsExactType(LinearInfinity_MC)) ? 2 : 0)+((RightPointOpen && !RHS_Arg2->IsExactType(LinearInfinity_MC)) ? 2 : 0)+3;
	// normal-looking interval
	return LHS_Arg1->LengthOfSelfName()+RHS_Arg2->LengthOfSelfName()+3;
}

size_t MetaConceptWithArgArray::LengthOfCommaListVarNames() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/15/1999
	if (ArgArray.empty()) return 2;

	size_t Length = fast_size()-1;
	size_t i = Length+1;
	do	{
		--i;
		Length += (NULL==ArgArray[i])	? 2
										: ArgArray[i]->LengthOfSelfName();
		}
	while(0<i);
	return Length;
}

size_t
MetaConceptWithArgArray::LengthOfCommaListVarNames(size_t MinIdx, size_t StrictMaxIdx) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/11/2000
	assert(MinIdx<StrictMaxIdx);
	if (ArgArray.empty() || fast_size()<=MinIdx) return 2;

	if (fast_size()<StrictMaxIdx)
		StrictMaxIdx=fast_size();
	size_t Length = StrictMaxIdx-MinIdx-1;
	size_t i = MinIdx;
	do	Length += (ArgArray[i]) ? ArgArray[i]->LengthOfSelfName()
								: 2;
	while(StrictMaxIdx> ++i);
	return Length;
}

// NOTE: displaying a prefix arglist
// 2 chars: begin, end parentheses [may need to generalize to other bounding characters]
// 1 char: each separating comma [n-1]
// length of each entry; NULL is 2 [""]
//! \todo FIX: PrefixArglist functions must react to phrase forms of EQUALTOONEOF,
//! DISTINCTFROMALLOF by parenthesizing them.
#ifndef USE_TO_S
size_t MetaConceptWithArgArray::LengthOfPrefixArgList() const
{	// FORMALLY CORRECT: Kenneth Boyd, 11/17/1999
	return ::LengthOfPrefixArgList(ArgArray);
}
#endif

size_t MetaConnective::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	return strlen(MetaConnectiveNames[array_index()])+LengthOfPrefixArgList();
}

// Self-name: NULL: syntax-bad phrase
// Self-name: IN ___:
// NOTE: 1-ary phrases may be either prefix or postfix!  It depends on the keyword....
size_t Phrase1Arg::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	return (this->*VFTable2Lookup[ExactType()-MinPhrase1Idx_MC].LengthOfSelfName)();
}

size_t Phrase1Arg::LengthOfSelfName_Prefix() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	size_t Length = 1;
	Length += PhraseKeyword ? strlen(PhraseKeyword) : 2;	// ""
	Length += !Arg1.empty() ? Arg1->LengthOfSelfName() : 2;	// ""
	return Length;
}

size_t Phrase1Arg::LengthOfSelfName_FunctionLike() const
{	// FORMALLY CORRECT: Kenneth Boyd, 1/5/2003
	size_t Length = 2;
	Length += PhraseKeyword ? strlen(PhraseKeyword) : 2;	// ""
	Length += !Arg1.empty() ? Arg1->LengthOfSelfName() : 2;	// ""
	return Length;
}

//! \todo FIX: These hard-code infix 2-ary phrases without parentheses blocking.
//! other paradigms will require moving the hard-coding to virtual functions.
size_t Phrase2Arg::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/13/2000
	size_t Length = PhraseKeyword ? 2+strlen(PhraseKeyword) : 4;
	Length += LHS_Arg1->LengthOfSelfName();
	Length += RHS_Arg2->LengthOfSelfName();
	return Length;
}


size_t PhraseNArg::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	return (this->*LengthOfSelfNameAuxArray[ExactType()-MinPhraseNIdx_MC])();
}

size_t PhraseNArg::LengthOfSelfNamePrefixOrPostfixCommaListVarNames() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	return strlen(PhraseKeyword)+LengthOfCommaListVarNames()+1;
}

// The self-name is of the form <reverse-order quantification list><statement>
// the estimator is somewhat unreliable: variables with the same quantification and domain
// should be compacted into a list of var-names later
size_t QuantifiedStatement::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/12/2000
	if (NULL==ArgArray) return sizeof(EMPTY_STATEMENT)-1;

	size_t Length = 2;	// bounding parentheses on Arg 0
	size_t i = fast_size();
	do	Length += ArgArray[--i] ? ArgArray[i]->LengthOfSelfName() : 2;	// Arg 0: "" others: ()
	while(0<i);
	return Length;
}

// The self-name is of the form ([Quantification-type] Var-name [IN <class-name>]|[IS FREE])
size_t MetaQuantifier::LengthOfSelfName() const	// start at 0 to get length
{	// FORMALLY CORRECT: Kenneth Boyd, 8/28/1999
	size_t Length = _msize(VariableName) + 6;	// 2 for (), 4 for " IS " or " IN "
	Length += IsExactType(Free_MC)	? 4		// FREE
									: Arg1->LengthOfSelfName() + strlen(QuantificationNames[ExactType()-ForAll_MC])+1;
	return Length;
}

// basic version: n-1 + signs, n args
// each arg that isn't higher precedence gets a pair of parentheses
// NOTE: a 0-ary sum returns the universal result 0
// NOTE: a 1-ary sum returns the single arg
size_t StdAddition::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 8/12/2000
	if (_IsZero()) return 1;
	size_t i = fast_size();
	size_t Length = i-1;
	while(0<i)
		{
		Length += ArgArray[--i]->LengthOfSelfName();
		if (   !ArgArray[i]->IsExplicitConstant()
			&& !ArgArray[i]->IsExactType(Variable_MC)
			&& !ArgArray[i]->IsExactType(StdMultiplication_MC))
			Length += 2;
		};
	return Length;
}

size_t StdMultiplication::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	if (_IsOne()) return 1;
	size_t i = fast_size();
	size_t Length = i-1;
	while(0<i)
		{
		Length += ArgArray[--i]->LengthOfSelfName();
		if (   !ArgArray[i]->IsExplicitConstant()
			&& !ArgArray[i]->IsExactType(Variable_MC))
			Length += 2;
		};
	return Length;
}

size_t UnparsedText::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 12/29/2000
	size_t TextLength = Text.empty() ? 0 : strlen(Text);
	if (IsUninterpreted()) return TextLength;
	if (IsHTMLTerminalTag()) return 3+TextLength;
#if 0	// currently redundant
	if (IsHTMLStartTag()) return 2+TextLength;
	if (IsJSEntity()) return 2+TextLength;
#endif
	return 2+TextLength;
}

#ifndef USE_TO_S
// A variable's self name is merely the name of the variable as recorded in its quantification.
size_t Variable::LengthOfSelfName() const
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	if (!SyntaxOK()) return sizeof(BAD_VARIABLE_SYNTAX)-1;

	size_t Length = strlen(ViewKeyword());
	Length += 	 ((MultiPurposeBitmap & LogicalNegated_VF) ? 4 : 0)
				+((MultiPurposeBitmap & StdAdditionInv_VF) ? 1 : 0)
				+((MultiPurposeBitmap & StdMultInv_VF) ? (sizeof(MULT_INV_TEXT)-1) : 0);
	return Length;
}
#endif
