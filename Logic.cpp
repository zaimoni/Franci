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
	auto stage_values = src.possible_values().value();
	auto stage = logic::display_as_enumerated_set(logic::to_string_vector(stage_values));
	STL_STRING_TO_STDOUT(stage);
	if (auto is_var = src.variable_values()) {
		C_STRING_TO_STDOUT("\n");
		stage = logic::display_as_enumerated_set(logic::to_string_vector(*is_var));
		STL_STRING_TO_STDOUT(stage);
	}
	C_STRING_TO_STDOUT("\n\n");
}

int main(int argc, char* argv[], char* envp[])
{
	std::shared_ptr<logic::TruthTable> test_var[6];
	std::shared_ptr<logic::TruthTable> test_Not[6];

	int ub = (int)logic::logics::franci+1;
	while (0 <= --ub) {
		C_STRING_TO_STDOUT(toAPI((logic::logics)ub).name());
		C_STRING_TO_STDOUT("\n");
		survey(*(test_var[ub] = logic::TruthTable::variable(std::string(1, 'A' + ub), (logic::logics)ub)));
		survey(*(test_Not[ub] = logic::TruthTable::Not(test_var[ub])));
	}


	STRING_LITERAL_TO_STDOUT("End testing\n");
	return 0;	// success
};
#endif

