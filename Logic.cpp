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

int main(int argc, char* argv[], char* envp[])
{
	std::shared_ptr<logic::TruthTable> test_var[6];
	std::vector<logic::TruthValue> test_values[6];

	int ub = (int)logic::logics::franci+1;
	while (0 <= --ub) {
		C_STRING_TO_STDOUT(toAPI((logic::logics)ub).name());
		C_STRING_TO_STDOUT("\n");
		test_var[ub] = logic::TruthTable::variable(std::string(1, 'A'+ub), (logic::logics)ub);
		STL_STRING_TO_STDOUT(test_var[ub]->name());
		C_STRING_TO_STDOUT("\n");
		test_values[ub] = test_var[ub]->variable_values().value();
		auto stage = logic::display_as_enumerated_set(logic::to_string_vector(test_values[ub]));
		STL_STRING_TO_STDOUT(stage);
		C_STRING_TO_STDOUT("\n");
		test_values[ub] = test_var[ub]->possible_values().value();
		stage = logic::display_as_enumerated_set(logic::to_string_vector(test_values[ub]));
		STL_STRING_TO_STDOUT(stage);
		C_STRING_TO_STDOUT("\n");
	}


	STRING_LITERAL_TO_STDOUT("End testing\n");
	return 0;	// success
};
#endif

