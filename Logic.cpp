// (C)2022, license: LICENSE.md

#include "Logic.hpp"

std::vector< std::weak_ptr<logic::TruthTable> > logic::TruthTable::_cache;

#ifdef LOGIC_DRIVER
#include "test_driver.h"

static_assert(logic::Classical().is_commutative());
static_assert(logic::KleeneStrong().is_commutative());
static_assert(logic::KleeneWeak().is_commutative());
static_assert(!logic::LispProlog().is_commutative());
static_assert(logic::Belnap().is_commutative());
static_assert(logic::Franci().is_commutative());

static void survey(const logic::TruthTable& src) {
	STL_STRING_TO_STDOUT(src.name());
	C_STRING_TO_STDOUT("\n");
	STL_STRING_TO_STDOUT(src.desc());
	C_STRING_TO_STDOUT("\n");
	INFORM(src.is_propositional_variable() ? "is propositional variable" : "is not propositional variable");
	INFORM(src.is_primary_term() ? "is primary term" : "is not primary term");
	auto stage_values = src.possible_values().value();
	auto stage = logic::display_as_enumerated_set(logic::to_string_vector(stage_values));
	STL_STRING_TO_STDOUT(stage);
	C_STRING_TO_STDOUT("\n\n");
}

int main(int argc, char* argv[], char* envp[])
{
	std::shared_ptr<logic::TruthTable> test_var[6];
	std::shared_ptr<logic::TruthTable> test_Not[6];

	int ub = (int)logic::logics::franci+1;
	while (0 <= --ub) {
		INFORM(toAPI((logic::logics)ub).name());
		survey(*(test_var[ub] = logic::TruthTable::variable(std::string(1, 'A' + ub), (logic::logics)ub)));
		survey(*(test_Not[ub] = logic::TruthTable::Not(test_var[ub])));
	}

	STRING_LITERAL_TO_STDOUT("End testing\n");
	return 0;	// success
};
#endif

