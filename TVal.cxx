// TVal.cxx

#include "TVal.hxx"
#include "Keyword1.hxx"

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

void TVal::SelfLogicalNOT() {_x = NegatedTVal[_x];}
bool TVal::isAntiIdempotentTo(const TVal& rhs) const { return _x==NegatedTVal[_x]; }

