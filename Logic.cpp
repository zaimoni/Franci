// (C)2022, license: LICENSE.md

#include "Logic.hpp"

std::vector< std::weak_ptr<logic::TruthTable> > logic::TruthTable::_cache;
std::vector<logic::TruthTable::inverse_infer_spec> logic::TruthTable::_inferred_reevaluations;


#ifdef LOGIC_DRIVER
#include "test_driver.h"
#include "Zaimoni.STL/Pure.C/comptest.h"

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
#include <io.h>
#else
#include <unistd.h>
#endif

static_assert(logic::Classical().is_commutative());
static_assert(logic::KleeneStrong().is_commutative());
static_assert(logic::KleeneWeak().is_commutative());
static_assert(!logic::LispProlog().is_commutative());
static_assert(logic::Belnap().is_commutative());
static_assert(logic::Franci().is_commutative());

static void survey(const logic::TruthTable& src) {
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
#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
	const bool to_console = _isatty(_fileno(stdout));
#else
	const bool to_console = isatty(fileno(stdout));
#endif

	std::shared_ptr<logic::TruthTable> test_var[6];
	std::shared_ptr<logic::TruthTable> test_var_Q[6];
	std::shared_ptr<logic::TruthTable> test_Not[6];

	if (!to_console) STRING_LITERAL_TO_STDOUT("<pre>\n");
	int ub = (int)logic::logics::franci+1;
	while (0 <= --ub) {
		INFORM(toAPI((logic::logics)ub).name());
		survey(*(test_var[ub] = logic::TruthTable::variable(std::string("P<sub>")+std::to_string(ub) + "</sub>", (logic::logics)ub)));
		test_var_Q[ub] = logic::TruthTable::variable(std::string("Q<sub>") + std::to_string(ub)+"</sub>", (logic::logics)ub);
		survey(*(test_Not[ub] = logic::TruthTable::Not(test_var[ub])));
	}
	INC_INFORM("tracking: ");
	INFORM(logic::TruthTable::count_expressions());
	INC_INFORM("queued substitutions: ");
	INFORM(logic::TruthTable::count_inferred_reevaluations());

	STRING_LITERAL_TO_STDOUT("End testing\n");
	if (!to_console) STRING_LITERAL_TO_STDOUT("</pre>\n");
	return 0;	// success
};
#endif

