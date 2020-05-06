// TVal.cxx

#include "TVal.hxx"
#include "Keyword1.hxx"

#include <string.h>

// from Zaimoni.STL/Compiler.h
/* size of a static array */
#define STATIC_SIZE(A) (sizeof(A)/sizeof(*A))

static constexpr const char NegatedTVal[] = {(char)(TVal::Contradiction),
									 (char)(TVal::False),
									 (char)(TVal::True),
									 (char)(TVal::Unknown)};
static_assert(STATIC_SIZE(NegatedTVal) == (TVal::Unknown + 1));

bool TVal::read(const char* src)
{
	if (!src) return false;
	size_t i = 0;
	do	if (0 == strcmp(src, Names[i]))
		{
		_x = (char)i;
		return true;
		}
	while(STATIC_SIZE(Names)> ++i);
	return false;
}

void TVal::ConstructSelfNameAux(char* Name) const { strcpy(Name, Names[_x]); }
size_t TVal::LengthOfSelfName() const { return strlen(Names[_x]); }
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

