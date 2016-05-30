// TVal.cxx

#include "TVal.hxx"
#include "Keyword1.hxx"

#include <string.h>

// from Zaimoni.STL/Compiler.h
/* size of a static array */
#define STATIC_SIZE(A) (sizeof(A)/sizeof(*A))

static const char* const TruthValueNames[4] =	{	TruthValue_Contradiction,
													TruthValue_True,
													TruthValue_False,
													TruthValue_Unknown	};

static const char NegatedTVal[4] =	{(char)(TVal::Contradiction),
									 (char)(TVal::False),
									 (char)(TVal::True),
									 (char)(TVal::Unknown)};

bool TVal::read(const char* src)
{
	if (!src) return false;
	size_t i = 0;
	do	if (0==strcmp(src,TruthValueNames[i]))
		{
		_x = (char)i;
		return true;
		}
	while(STATIC_SIZE(TruthValueNames)> ++i);
	return false;
}

void TVal::ConstructSelfNameAux(char* Name) const { strcpy(Name,TruthValueNames[_x]); }
size_t TVal::LengthOfSelfName() const { return strlen(TruthValueNames[_x]); }
void TVal::SelfLogicalNOT() {_x = NegatedTVal[_x];}
bool TVal::isAntiIdempotentTo(const TVal& rhs) const { return _x==NegatedTVal[rhs._x]; }

bool TVal::is_legal(const char* Text)
{	// FORMALLY CORRECT: 9/4/2006
	TVal tmp;
	return tmp.read(Text);
}

TVal operator&&(const TVal& lhs, const TVal& rhs)
{
	return 	 (TVal::Contradiction==lhs._x || TVal::Contradiction==rhs._x) ? TVal::Contradiction : 
			((TVal::False==lhs._x || TVal::False==rhs._x) ? TVal::False : 
			 ((TVal::Unknown==lhs._x || TVal::Unknown==rhs._x) ? TVal::Unknown : TVal::True));
}

