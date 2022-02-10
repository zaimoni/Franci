// (C)2022, license: LICENSE.md

#include "Logic.hpp"

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
	auto stage = src.possible_values_text();
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
	std::shared_ptr<logic::TruthTable> test_var_R[6];
	std::shared_ptr<logic::TruthTable> test_Not[6];
	std::shared_ptr<logic::TruthTable> test_Not_Q[6];
	std::shared_ptr<logic::TruthTable> test_Not_R[6];

	if (!to_console) STRING_LITERAL_TO_STDOUT("<pre>\n");
	{	// audit P and ~P; assume others work if those two do
	const std::string P("P");
	const std::string Q("Q");
	const std::string R("R");

	int ub = (int)logic::logics::franci+1;
	while (0 <= --ub) {
		INFORM(toAPI((logic::logics)ub).name());
		const auto suffix = std::string("<sub>") + std::to_string(ub) + "</sub>";
		survey(*(test_var[ub] = logic::TruthTable::variable(P+suffix, (logic::logics)ub)));
		test_var_Q[ub] = logic::TruthTable::variable(Q + suffix, (logic::logics)ub);
		test_var_R[ub] = logic::TruthTable::variable(R + suffix, (logic::logics)ub);
		survey(*(test_Not[ub] = logic::TruthTable::Not(test_var[ub])));
		test_Not_Q[ub] = logic::TruthTable::Not(test_var_Q[ub]);
		test_Not_R[ub] = logic::TruthTable::Not(test_var_R[ub]);
	}
	INC_INFORM("tracking: ");
	INFORM(logic::TruthTable::count_expressions());
	}

	// coverage test for logical negation, infer truth value
#if 0
	logic::TruthTable::infer(test_var[0], logic::TruthValue::True);
	survey(*test_var[0]);
	survey(*test_Not[0]);

	logic::TruthTable::infer(test_Not_Q[0], logic::TruthValue::True);
	survey(*test_var_Q[0]);
	survey(*test_Not_Q[0]);
#endif

	// coverage test for logical negation, exclude truth value
#if 0
	logic::TruthTable::exclude(test_var[0], logic::TruthValue::True);
	survey(*test_var[0]);
	survey(*test_Not[0]);

	logic::TruthTable::exclude(test_Not_Q[0], logic::TruthValue::True);
	survey(*test_var_Q[0]);
	survey(*test_Not_Q[0]);

	logic::TruthTable::exclude(test_var[1], logic::TruthValue::True);
	survey(*test_var[1]);
	survey(*test_Not[1]);

	logic::TruthTable::exclude(test_Not_Q[1], logic::TruthValue::True);
	survey(*test_var_Q[1]);
	survey(*test_Not_Q[1]);
#endif

	// coverage test for non-strict implication
#if 0
	{
	std::vector<std::shared_ptr<logic::TruthTable> > overview;

	ub = (int)logic::logics::franci + 1;
	while (0 <= --ub) {
		int ub2 = (int)logic::logics::franci + 1;
		while (0 <= --ub2) {
			try {
				auto stage = logic::TruthTable::NonStrictlyImplies(test_var[ub], test_var_Q[ub2]);
				overview.push_back(std::move(stage));
			} catch (const std::logic_error& e) {
				INC_INFORM(e.what());
				INC_INFORM(": ");
				INC_INFORM(toAPI((logic::logics)ub).name());
				INC_INFORM(", ");
				INFORM(toAPI((logic::logics)ub2).name());
			}
		}
	}
	}
#endif

	auto P_implies_Q = logic::TruthTable::NonStrictlyImplies(test_var[0], test_var_Q[0]);
	auto Q_implies_R = logic::TruthTable::NonStrictlyImplies(test_var_Q[0], test_var_R[0]);
	survey(*P_implies_Q);
	INFORM(P_implies_Q->table_as_text().c_str());
	logic::TruthTable::infer(P_implies_Q, logic::TruthValue::True);
	INFORM(P_implies_Q->possible_values_text().c_str());
	INFORM(P_implies_Q->table_as_text().c_str());
	survey(*Q_implies_R);

	auto P_implies_Q_vars = P_implies_Q->catalog_vars();
	auto Q_implies_R_vars = Q_implies_R->catalog_vars();

	for (decltype(auto) x : P_implies_Q_vars) INFORM(x->desc().c_str());
	for (decltype(auto) x : Q_implies_R_vars) INFORM(x->desc().c_str());

	STRING_LITERAL_TO_STDOUT("End testing\n");
	if (!to_console) STRING_LITERAL_TO_STDOUT("</pre>\n");
	return 0;	// success
};
#endif

