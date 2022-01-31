#ifndef LOGIC_HPP
#define LOGIC_HPP

// (C)2022, license: LICENSE.md

// We take deductive truth values to be implemented by the C++ langage.
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <utility>
#include <optional>
#include <ranges>
#include <stddef.h>

namespace logic {

template<class T>
std::vector<std::string> to_string_vector(const std::vector<T>& src) requires requires(std::vector<std::string> test) { test.push_back(to_string(*src.begin())); }
{
	std::vector<std::string> ret;
	ret.reserve(src.size());
	for (decltype(auto) x : src) ret.push_back(to_string(x));
	return ret;
}

inline std::string display_as_enumerated_set(const std::vector<std::string>& src)
{
	if (src.empty()) return "{}";
	bool want_comma = false;
	std::string ret("{ ");
	for (decltype(auto) x : src) {
		if (want_comma) ret += ", ";
		ret += x;
		want_comma = true;
	}

	return ret += " }";

}

// Our native logic, for convenience
enum class TruthValue : unsigned char {
	Contradiction = 0, // Kripke strong 3-valued logic; "super-false"
	False,
	True,
	Unknown // logic of belief-specific
};

static_assert(1 == sizeof(TruthValue));

template<bool val>
constexpr bool could_be(TruthValue src) {
	return ((int)src) & (1U << val);
}

static_assert(could_be<false>(TruthValue::False));
static_assert(could_be<false>(TruthValue::Unknown));
static_assert(could_be<true>(TruthValue::True));
static_assert(could_be<true>(TruthValue::Unknown));

static_assert(!could_be<true>(TruthValue::False));
static_assert(!could_be<true>(TruthValue::Contradiction));
static_assert(!could_be<false>(TruthValue::True));
static_assert(!could_be<false>(TruthValue::Contradiction));

constexpr TruthValue operator!(TruthValue src) {
	constexpr const TruthValue invert[4] = { TruthValue::Contradiction, TruthValue::True, TruthValue::False, TruthValue::Unknown };
	return invert[(int)src];
}

static_assert(TruthValue::Contradiction == !TruthValue::Contradiction);
static_assert(TruthValue::True == !TruthValue::False);
static_assert(TruthValue::False == !TruthValue::True);
static_assert(TruthValue::Unknown == !TruthValue::Unknown);

constexpr const char* to_string(TruthValue l)
{
	switch (l)
	{
	case TruthValue::Contradiction: return "CONTRADICTION";
	case TruthValue::True: return "TRUE";
	case TruthValue::False: return "FALSE";
	case TruthValue::Unknown: return "UNKNOWN";
	default: return nullptr;
	}
}

enum class logics {
	classical = 0,
	kleene_strong,
	kleene_weak,
	lisp_prolog,
	belnap,
	franci
};

constexpr bool is_sublogic(logics sub, logics sup) {
	if (sub == sup) return true;
	if (sub == logics::classical) return true;
	if (sup == logics::classical) return false;
	if (sub == logics::kleene_weak || sub == logics::lisp_prolog) return false;
	if (sup == logics::kleene_weak || sub == logics::lisp_prolog) return false;
	if (sub == logics::kleene_strong) return true;
	return false;
}

// API
struct logic_API {
	constexpr virtual bool is_commutative() const { return true; }
	constexpr virtual bool is_out_of_of_range(TruthValue x) const { return false;  }
	constexpr virtual const char* name() const = 0;

	static constexpr const TruthValue And_identity = TruthValue::True;
	constexpr virtual TruthValue And_annihilator() const { return TruthValue::False; }

	constexpr virtual TruthValue And(TruthValue lhs, TruthValue rhs) const = 0;

protected:
	logic_API() = default;
	virtual ~logic_API() = default;

	constexpr std::optional<TruthValue> And_core(TruthValue lhs, TruthValue rhs) const {
		if (is_out_of_of_range(lhs) || is_out_of_of_range(rhs)) throw std::logic_error(std::string(name())+" logic out of range");

		if (And_identity == lhs) return rhs;
		if (And_identity == rhs) return lhs;
		const auto a = And_annihilator();
		if (a == lhs) return a;
		if (a == rhs && is_commutative()) return a;
		if (lhs == rhs) return lhs;	// always idempotent
		return std::nullopt;
	}
};

struct Classical final : public logic_API {
	constexpr bool is_out_of_of_range(TruthValue x) const override { return TruthValue::Unknown == x || TruthValue::Contradiction == x; }
	constexpr const char* name() const override { return "Classical"; }

	constexpr TruthValue And(TruthValue lhs, TruthValue rhs) const override { return And_core(lhs, rhs).value(); };

	static auto& get() {
		static Classical ooao;
		return ooao;
	}
};

struct KleeneStrong final : public logic_API {
	constexpr bool is_out_of_of_range(TruthValue x) const override { return TruthValue::Contradiction == x; }
	constexpr const char* name() const override { return "Kleene's strong"; }

	constexpr TruthValue And(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = And_core(lhs, rhs)) return *x;
		return TruthValue::Unknown;
	};

	static auto& get() {
		static KleeneStrong ooao;
		return ooao;
	}
};

struct KleeneWeak final : public logic_API {
	constexpr bool is_out_of_of_range(TruthValue x) const override { return TruthValue::Contradiction == x; }
	constexpr const char* name() const override { return "Kleene's weak"; }
	constexpr TruthValue And_annihilator() const override { return TruthValue::Unknown; }

	constexpr TruthValue And(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = And_core(lhs, rhs)) return *x;
		return TruthValue::False;
	};

	static auto& get() {
		static KleeneWeak ooao;
		return ooao;
	}
};

struct LispProlog final : public logic_API {
	constexpr bool is_commutative() const override { return false; }
	constexpr bool is_out_of_of_range(TruthValue x) const override { return TruthValue::Contradiction == x; }
	constexpr const char* name() const override { return "Lisp/Prolog"; }

	constexpr TruthValue And(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = And_core(lhs, rhs)) return *x;
		return TruthValue::Unknown; // other left annhiliator
	};

	static auto& get() {
		static LispProlog ooao;
		return ooao;
	}
};

struct Belnap final : public logic_API {
	constexpr const char* name() const override { return "Belnap's"; }

	constexpr TruthValue And(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = And_core(lhs, rhs)) return *x;
		return TruthValue::False; // Unknown & Contradiction, or vice versa
	};

	static auto& get() {
		static Belnap ooao;
		return ooao;
	}
};

struct Franci final : public logic_API {
	constexpr const char* name() const override { return "Franci's"; }
	constexpr TruthValue And_annihilator() const override { return TruthValue::Contradiction; }

	constexpr TruthValue And(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = And_core(lhs, rhs)) return *x;
		return TruthValue::False; // False & Unknown, or vice versa
	};

	static auto& get() {
		static Franci ooao;
		return ooao;
	}
};

logic_API& toAPI(logics src)
{
	switch(src) {
	case logics::classical: return Classical::get();
	case logics::kleene_strong: return KleeneStrong::get();
	case logics::kleene_weak: return KleeneWeak::get();
	case logics::lisp_prolog: return LispProlog::get();
	case logics::belnap: return Belnap::get();
	default: return Franci::get();
	}
}

template<class T> requires(std::ranges::range<T>)
auto decode_bitmap(unsigned long long src, const T& key)
{
	std::vector<std::remove_cv_t<std::remove_reference_t<decltype(*key.begin())> > > ret;
	// unpack bitmap into std::vector (function target?)
	unsigned int i = 0;
	for (decltype(auto) x : key) {
		if (src & (1ULL << i)) ret.push_back(x);
		++i;
	}
	return ret;
}

class TruthTable final
{
private:
	static constexpr const bool integrity_check = false; // control expensive checks centrally

	static std::vector< std::weak_ptr<TruthTable> > _cache;
	static constexpr const TruthValue ref_classical[] = { TruthValue::False, TruthValue::True };
	static constexpr const TruthValue ref_threeval[] = { TruthValue::False, TruthValue::True, TruthValue::Unknown };
	static constexpr const TruthValue ref_fourval[] = { TruthValue::False, TruthValue::True, TruthValue::Unknown, TruthValue::Contradiction };
	static constexpr const std::pair<const char*, int> predefined[] = {{"~", 1}, {"&", 2}};

	std::string symbol;
	logics _logic;
	std::vector< std::shared_ptr<TruthTable> > _args;
	std::optional<unsigned char> _var_values;	// bitmap, expected range 0...15
	// \todo following needs 2 bits for each input, and 2 bits for self
	std::optional<std::vector<unsigned short> > _var_enumerated;
	unsigned char (*update_bitmap)(const std::shared_ptr<TruthTable>& src);	// for unary logical connectives/functions
	std::vector<unsigned short>(*update_enumeration)(const decltype(_args)& src);	// for n-ary logical connectives/functions

	// if we need these, undelete
	TruthTable() = delete;
	TruthTable(const TruthTable& src) = delete;
	TruthTable(TruthTable&& src) = delete;
	TruthTable& operator=(const TruthTable& src) = delete;
	TruthTable& operator=(TruthTable&& src) = delete;

	constexpr static auto _var_begin(logics l) {
		switch (l) {
		case logics::classical: return std::begin(ref_classical);
		case logics::kleene_strong:
		case logics::kleene_weak:
		case logics::lisp_prolog:	return std::begin(ref_threeval);
		case logics::belnap:
		case logics::franci:	return std::begin(ref_fourval);
		}
	}

	constexpr static auto _var_end(logics l) {
		switch (l) {
		case logics::classical: return std::end(ref_classical);
		case logics::kleene_strong:
		case logics::kleene_weak:
		case logics::lisp_prolog:	return std::end(ref_threeval);
		case logics::belnap:
		case logics::franci:	return std::end(ref_fourval);
		}
	}

	constexpr static decltype(_var_values) _var_agnostic(logics l) {
		switch (l) {
		case logics::classical: return (1ULL << std::end(ref_classical) - std::begin(ref_classical)) - 1ULL;
		case logics::kleene_strong:
		case logics::kleene_weak:
		case logics::lisp_prolog:	return (1ULL << std::end(ref_threeval) - std::begin(ref_threeval)) - 1ULL;
		case logics::belnap:
		case logics::franci:	return (1ULL << std::end(ref_fourval) - std::begin(ref_fourval)) - 1ULL;
		}
	}

	// Propositional variable.  No arguments
	TruthTable(const std::string& name, logics l) : symbol(name), _logic(l), _var_values(_var_agnostic(l)), update_bitmap(nullptr), update_enumeration(nullptr) {}

public:
	~TruthTable() = default;

	auto set_theoretic_range() const {
		switch(_logic) {
		case logics::classical: return std::ranges::subrange(ref_classical);
		case logics::kleene_strong:
		case logics::kleene_weak:
		case logics::lisp_prolog:	return std::ranges::subrange(ref_threeval);
		case logics::belnap:
		case logics::franci:	return std::ranges::subrange(ref_fourval);
		}
	}

	auto arity() const { return _args.size(); }
	auto& name() const { return symbol; } // at least for arity 0
	auto catalog_vars() {
		std::vector<TruthTable*> ret;
		_catalog_vars(this, ret);
		return ret;
	}
	std::optional<std::vector<TruthValue> > variable_values() const {
		ptrdiff_t ub = _args.size();
		if (ub) return std::nullopt;	// either error case, or a logical connective
		if (!_var_values) return std::nullopt; // error case
		return decode_bitmap(*_var_values, set_theoretic_range());
	}

	std::optional<std::vector<TruthValue> > possible_values() const {
		if (_var_values) {
exit_by_values:
			return decode_bitmap(*_var_values, set_theoretic_range());
		}
		if (_var_enumerated) {
			auto code = extract_truth_bitmap(*_var_enumerated, 0);	// *our* value, not our arguments' values
			if (code) {
				*const_cast<std::optional<unsigned char>*>(&_var_values) = code; // cache variable update
				return decode_bitmap(code, set_theoretic_range());
			}
		}
		if (update_bitmap) {
			if (1 == _args.size()) {
				if (auto code = update_bitmap(_args.front())) {
					if (code) {
						*const_cast<decltype(_var_values)*>(&_var_values) = code; // cache variable update
						goto exit_by_values;
					}
				}
			};	// otherwise, invariant violation
		}
		if (update_enumeration && !_var_enumerated) {
			if (2 <= _args.size()) {
				*const_cast<decltype(_var_enumerated)*>(&_var_enumerated) = update_enumeration(_args); // cache variable update
			};	// otherwise, invariant violation
		}
		// \todo populate from arguments
		return std::nullopt;
	}

	auto catalog_values(const std::vector<TruthTable*>& src) const {
		ptrdiff_t ub = src.size();
		std::vector<std::vector<TruthValue> > ret(src.size());
		while (0 < --ub) {
			if (auto x = src[ub]->variable_values()) ret[ub] = std::move(*x);
//			else ... // invariant violation
		}
		return ret;
	}

	// factories
	static std::shared_ptr<TruthTable> variable(const std::string& name, logics l) {
		auto ub = _cache.size();
		while (0 < ub) {
			--ub;
			if (auto x = _cache[ub].lock()) {
				if (name != x->symbol) continue;
				if (is_sublogic(x->_logic, l)) return x;
				return nullptr;
			} else {
				_cache[ub].swap(_cache.back());
				_cache.pop_back();
			}
		}
		std::shared_ptr<TruthTable> stage(new TruthTable(name, l));
		if (integrity_check && !stage->syntax_ok()) throw std::logic_error("invalid constructor");
		_cache.push_back(stage);
		return stage;
	}

private:
	bool syntax_ok() const {
		if (update_bitmap && update_enumeration) return false;
		if (update_bitmap && 1 != _args.size()) return false;
		if (update_enumeration && 2 > _args.size()) return false;
		return true;
	}

	static unsigned char extract_truth_bitmap(unsigned short src, int index) {
		return (src >> 4 * index) & 0x0FU;
	}

	static unsigned char extract_truth_bitmap(const std::vector<unsigned short>& src, int index) {
		unsigned char ret = 0;
		for (decltype(auto) x : src) {
			auto test = (x >> 2 * index) & 0x03U;
			ret |= (1ULL << test);
		}
		return ret;
	}

	// codes could either be bitmaps (range 0..15) or coded values (range 0..3)
	static void _update_truth_codes(std::vector<unsigned short>& dest, unsigned short already, const std::vector<unsigned char>& src) {
		for (decltype(auto) now : src) dest.push_back(already | now);
	}

	std::vector<unsigned short> cartesian_append_truth_codes(const std::vector<unsigned short>& prior, const std::vector<unsigned char>& src) {
		std::vector<unsigned short> ret;
		if (prior.empty()) {
			_update_truth_codes(ret, 0, src);
		} else {
			for (decltype(auto) already : prior) {
				_update_truth_codes(ret, already << 2, src);
			}
		}
		return ret;
	}

	std::vector<unsigned short> cartesian_append_truth_bitmap(const std::vector<unsigned short>& prior, unsigned char src) {
		auto to_append = decode_bitmap(src, set_theoretic_range());
		std::vector<unsigned short> ret;
		if (!to_append.empty()) {
			if (prior.empty()) {
				for (decltype(auto) t : to_append) ret.push_back((unsigned short)t);
			} else {
				for (decltype(auto) already : prior) {
					for (decltype(auto) t : to_append) ret.push_back(already << 2 | (unsigned short)t);
				}
			}
		}
		return ret;
	}

	static void _catalog_vars(TruthTable* origin, std::vector<TruthTable*>& ret) {
retry:
		// we almost certainly will be out of RAM long before this cast is undefined behavior
		ptrdiff_t ub = origin->_args.size();
		switch(ub) {
		case 0: // base case
			if (ptrdiff_t ub2 = ret.size()) {
				while (0 < --ub2) if (origin == ret[ub]) return;
				ret.push_back(origin);
			}
			return;
		case 1:	// tail-recurse
			origin = origin->_args.front().get();
			goto retry;
		}
		while (0 < --ub) _catalog_vars(origin->_args[ub].get(), ret);
	}
};

// Want to make a clear distinction between belief/contrafactual reasoning, and "absolute truth"
template<class T>
class Expression {
private:
protected:
	Expression() = default;
	Expression(const Expression& src) = default;
	Expression(Expression&& src) = default;
	Expression& operator=(const Expression& src) = default;
	Expression& operator=(Expression&& src) = default;
public:
	using evalspec = std::pair<std::function<bool()>, std::function<bool(Expression*&)> >;

	virtual ~Expression() = default;

	T operator()() const { return _eval(); }
	virtual evalspec canEvaluate() const = 0; // check for symbolic evaluation

	// I/O support
	virtual std::string to_s() const = 0;
private:
	virtual T _eval() const = 0;
};

}

#endif
