// Parser.hpp
// header for Parser

#ifndef ZAIMONI_STL_LEXPARSE_PARSER_HPP
#define ZAIMONI_STL_LEXPARSE_PARSER_HPP 1

namespace zaimoni {

//! use volatile if the parsing ruleset is meant to alter itself
//! only for deterministic parsing
template<typename T>
class Parser
{
public:
	typedef bool ParseFunc(T**& ArgArray,size_t Idx);
	typedef bool RecognizeFunc(T*& ArgArray);

	// following are not really owned
	RecognizeFunc* UnaryEscape;
	ParseFunc** O_n_rules;		// dominantly postfix
	size_t O_n_rulecount;		// need this because O_n_rules may be static array
	ParseFunc** O_n_2_rules;	// dominantly prefix
	size_t O_n_2_rulecount;		// need this because O_n_2_rules may be static array

private:
	// disallow copy
	Parser(const Parser& Source);
	const Parser& operator=(const Parser& Source);
public:
	Parser(RecognizeFunc* _UnaryEscape, ParseFunc** _O_n_rules, size_t _O_n_rulecount, ParseFunc** _O_n_2_rules, size_t _O_n_2_rulecount)
	:	UnaryEscape(_UnaryEscape),
		O_n_rules(_O_n_rules),
		O_n_rulecount(_O_n_rulecount),
		O_n_2_rules(_O_n_2_rules),
		O_n_2_rulecount(_O_n_2_rulecount)
		{};
	Parser() : UnaryEscape(NULL),O_n_rules(NULL),O_n_2_rules(NULL) {};
//	~Parser();	// default OK

	bool ParseOneStep(T**& ArgArray);
	void Parse(T**& ArgArray);

private:
	bool _MetaParse(T**& ArgArray);
};

template<typename T>
bool
Parser<T>::ParseOneStep(T**& ArgArray)
{
	if (NULL==ArgArray) return false;
	return _MetaParse(ArgArray);
}

template<typename T>
void
Parser<T>::Parse(T**& ArgArray)
{	
	if (NULL==ArgArray) return;
	while(_MetaParse(ArgArray));
	// when nothing triggers, fall through and out
}

template<typename T>
bool
Parser<T>::_MetaParse(T**& ArgArray)
{	//! \pre NULL!=ArgArray
	assert(NULL!=ArgArray);
	size_t ArgLength = ArraySize(ArgArray);
	if (NULL!=UnaryEscape)
		{
		if (1==ArgLength && UnaryEscape(ArgArray[0])) return false;
		size_t Idx = 0;
		do	UnaryEscape(ArgArray[Idx]);
		while(++Idx<ArgLength);
		}
	if (NULL!=O_n_rules && 0<O_n_rulecount)
		{
		size_t Idx = 0;
		do	{
			size_t Idx2 = 0;
			do	if ((O_n_rules[Idx2])(ArgArray,Idx))
					return true;
			while(++Idx2<O_n_rulecount);
			}
		while(++Idx<ArgLength);
		}
	if (NULL!=O_n_2_rules && 0<O_n_2_rulecount)
		{
		size_t Idx = 0;
		do	{
			size_t Idx2 = 0;
			do	if ((O_n_2_rules[Idx2])(ArgArray,Idx))
					return true;
			while(++Idx2<O_n_2_rulecount);
			}
		while(++Idx<ArgLength);
		}
	return false;
}

}	// namespace zaimoni

#endif
