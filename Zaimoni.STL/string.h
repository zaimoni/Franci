/* string.h */
/* header to wrap C string operations */

#ifndef ZAIMONI_STRING_H
#define ZAIMONI_STRING_H 1

#include <ctype.h>

#include "Logging.h"
#include "string2.h"	/* for the typedefs */

/* wrappers (Perl mnemonics) */
inline bool streq(const char* LHS, const char* RHS) {return 0==strcmp(LHS,RHS);}	/* equal */
inline bool strne(const char* LHS, const char* RHS) {return 0!=strcmp(LHS,RHS);}	/* not equal */
inline bool strlt(const char* LHS, const char* RHS) {return 0 >strcmp(LHS,RHS);}	/* lexical less than */
inline bool strgt(const char* LHS, const char* RHS) {return 0 <strcmp(LHS,RHS);}	/* lexical greater than */
inline bool strle(const char* LHS, const char* RHS) {return 0>=strcmp(LHS,RHS);}	/* lexical less than or equal */
inline bool strge(const char* LHS, const char* RHS) {return 0<=strcmp(LHS,RHS);}	/* lexical greater than or equal */

/* case insensitive versions of above */
inline bool strieq(const char* LHS, const char* RHS) {return 0==stricmp(LHS,RHS);}
inline bool strine(const char* LHS, const char* RHS) {return 0!=stricmp(LHS,RHS);}
inline bool strilt(const char* LHS, const char* RHS) {return 0 >stricmp(LHS,RHS);}
inline bool strigt(const char* LHS, const char* RHS) {return 0 <stricmp(LHS,RHS);}
inline bool strile(const char* LHS, const char* RHS) {return 0>=stricmp(LHS,RHS);}
inline bool strige(const char* LHS, const char* RHS) {return 0<=stricmp(LHS,RHS);}

/* postpend string series */
inline char*
postpend_string(char* const dest, const char* src)
{
	assert(NULL!=dest);
	assert(NULL!=src);
	assert(0<strlen(src));
	strcpy(dest,src);
	return dest+strlen(src);
}

#ifdef __cplusplus
template<size_t i>
inline char*
postpend_string(char* const dest, const char* src)
{
	assert(NULL!=dest);
	assert(NULL!=src);
	assert(i==strlen(src));
	strcpy(dest,src);
	return dest+i;
}
#endif

/* extended testing */
#ifdef __cplusplus
template<typename boolfunc>
bool
string_nand(const char* target, boolfunc Test)
{
	while('\x00'!=*target)
		if (!Test(*target++)) return true;
	return false;
}

template<typename boolfunc>
bool
string_or(const char* target, boolfunc Test)
{
	while('\x00'!=*target)
		if (Test(*target++)) return true;
	return false;
}
#endif

#endif

