// TVal.cxx

#include "TVal.hxx"
#include "Keyword1.hxx"

#include <string.h>

// from Zaimoni.STL/Compiler.h
/* size of a static array */
#define STATIC_SIZE(A) (sizeof(A)/sizeof(*A))

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

#if 0
// only called from UnparsedText::Evaluate(...); guard clause is IsLegalTValString
bool TruthValue::ConvertToTruthValue(MetaConcept*& dest,const char* Text)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/4/2006
	assert(!dest);
	TVal tmp;
	SUCCEED_OR_DIE(tmp.read(Text));
	dest = new(nothrow) TruthValue(tmp);
	return dest;
}
#endif
