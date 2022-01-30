// (C)2022, license: LICENSE.md

#include "Logic.hpp"

#ifdef LOGIC_DRIVER
#include "test_driver.h"

static_assert(logic::Classical().is_commutative());
static_assert(logic::KleeneStrong().is_commutative());
static_assert(logic::KleeneWeak().is_commutative());
static_assert(!logic::LispProlog().is_commutative());
static_assert(logic::Belnap().is_commutative());
static_assert(logic::Franci().is_commutative());

int main(int argc, char* argv[], char* envp[])
{
	C_STRING_TO_STDOUT(logic::Classical::get().name());
	C_STRING_TO_STDOUT("\n");
	C_STRING_TO_STDOUT(logic::KleeneStrong::get().name());
	C_STRING_TO_STDOUT("\n");
	C_STRING_TO_STDOUT(logic::KleeneWeak::get().name());
	C_STRING_TO_STDOUT("\n");
	C_STRING_TO_STDOUT(logic::LispProlog::get().name());
	C_STRING_TO_STDOUT("\n");
	C_STRING_TO_STDOUT(logic::Belnap::get().name());
	C_STRING_TO_STDOUT("\n");
	C_STRING_TO_STDOUT(logic::Franci::get().name());
	C_STRING_TO_STDOUT("\n");
	STRING_LITERAL_TO_STDOUT("End testing\n");
	return 0;	// success
};
#endif

