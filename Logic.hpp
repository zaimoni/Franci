#ifndef LOGIC_HPP
#define LOGIC_HPP

// (C)2022, license: LICENSE.md

// We take deductive truth values to be implemented by the C++ langage.
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <optional>
#include <ranges>
#include <concepts>
#include <algorithm>
#include <stddef.h>

#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/observer.hpp"

namespace logic {
	using std::swap;

	struct proof_by_contradiction final : public std::runtime_error
	{
		proof_by_contradiction(const std::string& msg) : std::runtime_error(msg) {}
		proof_by_contradiction(const char* msg) : std::runtime_error(msg) {}

		proof_by_contradiction(const proof_by_contradiction& src) = default;
		proof_by_contradiction(proof_by_contradiction&& src) = default;
		proof_by_contradiction& operator=(const proof_by_contradiction& src) = default;
		proof_by_contradiction& operator=(proof_by_contradiction&& src) = default;
		~proof_by_contradiction() = default;
	};
}

// plausibly belongs in a more general library
template<class T> requires(std::ranges::range<T>)
std::vector<size_t> get_upper_bounds(const std::vector<T>& key) requires requires { key.front().size(); }
{
	std::vector<size_t> ret;
	for (decltype(auto) x : key) ret.push_back(x.size());
	return ret;
}

namespace Gray_code {

	// Naming attributed to Knuth.  Alternate name is chinese remainder theorem encoding
	template<std::unsigned_integral U>
	bool increment(std::vector<U>& code, const std::vector<size_t>& bounds) {
		size_t scan = 0;
		while (bounds.size() > scan) {
			if (++code[scan] < bounds[scan]) return true;
			code[scan++] = 0;
		}
		return false;
	}

	template<std::unsigned_integral U, class K>
	std::vector<K> cast_to(const std::vector<U>& code, const std::vector<std::vector<K> >& key)
		requires requires(K dest) { dest = key[0][code[0]]; }
	{
		std::vector<K> ret(code.size());
		if (int ub = code.size()) {
			while (0 <= --ub) ret[ub] = key[ub][code[ub]];
		}
		return ret;
	}

	template<std::unsigned_integral U, class K>
	void cast_to(const std::vector<U>& code, const std::vector<std::vector<K> >& key, std::vector<K>& dest)
		requires requires(K dest) { dest = key[0][code[0]]; }
	{
		if (int ub = code.size()) {
			while (0 <= --ub) dest[ub] = key[ub][code[ub]];
		}
	}
}

namespace enumerated {
	template<class V>
	class Set final {
		std::string _name;
		std::optional<std::vector<V> > _elements;
		std::vector<std::shared_ptr<zaimoni::observer<std::vector<V> > > > _watchers;

		Set() = default;	// unnamed set with no known elements

		// unnamed sets
		Set(const std::vector<V>& src) : _elements(src) {}
		Set(std::vector<V>&& src) : _elements(std::move(src)) {}

		// named sets
		Set(const std::string& name, const std::vector<V>& src) : _name(name), _elements(src) {}
		Set(const std::string& name, std::vector<V>&& src) : _name(name), _elements(std::move(src)) {}
		Set(std::string&& name, const std::vector<V>& src) : _name(std::move(name)), _elements(src) {}
		Set(std::string&& name, std::vector<V>&& src) : _name(std::move(name)), _elements(std::move(src)) {}

	public:
		Set(const Set& src) = delete;
		Set(Set&& src) = delete;
		Set& operator=(const Set& src) = delete;
		Set& operator=(Set&& src) = delete;
		~Set() = default;

		auto& name() const { return _name; }
		auto size() const { return _elements ? _elements->size() : 0; }
		bool empty() const { return _elements ? _elements->empty() : true; }
		bool is_defined() const{ return _elements; }  // constructive logic can have undefined variables
		const std::vector<V>* possible_values() const { return _elements ? &(*_elements) : nullptr; }

		void force_equal(const V& src) {
			if (!_elements) {
				// Prior declaration was constructive logic: this initializes
				_elements = std::vector<V>(1,src);
				notify();
				return;
			}
			ptrdiff_t ub = _elements->size();
			while (0 <= --ub) {
				if ((*_elements)[ub] == src) {
					if (1 < _elements->size()) {
						_elements->clear();
						_elements->push_back(src);
						notify();
					}
					return;
				}
			}
			throw logic::proof_by_contradiction("required equality failed");
		}

		void restrict_to(const std::vector<V>& src) {
			if (!_elements) {
				// Prior declaration was constructive logic: this initializes
				_elements = src;
				notify();
				return;
			}
			if (destructive_intersect(*_elements, src)) notify();
		}

		template<std::ranges::range SRC>
		void restrict_to(SRC&& src) requires(std::is_convertible_v<decltype(*src.begin()), V> && !std::is_same_v<SRC, std::vector<V> >) {
			restrict_to(std::vector<V>(src.begin(), src.end()));
		}

		void force_unequal(const V& src) {
			if (!_elements) {
				// Prior declaration was constructive logic: this initializes
				// \todo work out a way to record this
				return;
			}
			ptrdiff_t ub = _elements->size();
			while (0 <= --ub) {
				if (src == (*_elements)[ub]) {
					if (1 == _elements->size()) throw logic::proof_by_contradiction("required inequality failed");
					_elements->erase(_elements->begin() + ub);
					notify();
					return;
				}
			}
		}

		void exclude(const std::vector<V>& src) {
			if (!_elements) {
				// Prior declaration was constructive logic
				// \todo work out a way to record this
				return;
			}
			if (destructive_difference(*_elements, src)) notify();
		}

		void adjoin(const V& src) {
			if (!_elements) {
				// Prior declaration was constructive logic: this initializes
				_elements = std::vector<V>(1,src);
				notify();
				return;
			}
			if (!std::ranges:any_of(*_elements, [&](const V& x) {x == src})) {
				(*_elements).push_back(src);
				notify();
				return;
			}
		}

		template<std::invocable<V> FUNC>
		auto image_of(const FUNC& op) {
			std::vector<decltype(op(_elements->front()))> ret;
			if (!_elements) return std::optional<decltype(ret)>();	// constructive logic
			for (decltype(auto) x : *_elements) ret.push_back(op(x));
			return std::optional<decltype(ret)>(ret);
		}

		void watched_by(const std::shared_ptr<zaimoni::observer<std::vector<V> > >& src) {
			ptrdiff_t ub = _watchers.size();
			while (0 <= --ub) {
				if (_watchers[ub]) {
					if ((void*)(_watchers[ub].get()) == (void*)(src.get())) return;	// already installed
				} else if (1 == _watchers.size()) {
					decltype(_watchers)().swap(_watchers);
					return;
				} else {
					_watchers[ub].swap(_watchers.back());
					_watchers.pop_back();
				}
			}
			_watchers.push_back(src);
		}

		// factory functions
		static auto intuitionist_empty() { return std::shared_ptr<Set>(new Set()); }

		// unnamed set -- no caching
		template<std::ranges::range SRC>
		static auto declare(SRC&& src) requires(std::is_convertible_v<decltype(*src.begin()), V>)
		{
			return std::shared_ptr<Set>(new Set(std::vector<V>(src.begin(), src.end())));
		}

		// named sets -- these are cached
		template<std::ranges::range SRC>
		static auto declare(const std::string& name, SRC&& src) requires(std::is_convertible_v<decltype(*src.begin()), V>)
		{
			if (auto x = find(name)) {
				// already have this: treat as restriction
				x->restrict_to(src);
				return x;
			}

			std::shared_ptr<Set> stage(new Set(name, std::vector<V>(src.begin(), src.end())));
			cache().push_back(stage);
			return stage;
		}

		template<std::ranges::range SRC>
		static auto declare(std::string&& name, SRC&& src) requires(std::is_convertible_v<decltype(*src.begin()), V>)
		{
			if (auto x = find(name)) {
				// already have this: treat as restriction
				x->restrict_to(src);
				return x;
			}

			std::shared_ptr<Set> stage(new Set(std::move(name), std::vector<V>(src.begin(), src.end())));
			cache().push_back(stage);
			return stage;
		}

	private:
		static auto& cache() {
			static std::vector<std::weak_ptr<Set<V> > > ooao;
			return ooao;
		}

		static std::shared_ptr<Set<V> > find(const decltype(_name)& name) {
			auto& already = cache();
			ptrdiff_t ub = already.size();
			while (0 <= --ub) {
				if (const auto x = already[ub].lock()) {
					if (name == x->_name) return x;
				} else {
					already[ub].swap(already.back());
					already.pop_back();
				}
			}
			return nullptr;
		}

		void notify() {
			ptrdiff_t ub = _watchers.size();
			while (0 <= --ub) {
				if (_watchers[ub] && _watchers[ub]->onNext(*_elements)) continue;
				if (1 == _watchers.size()) {
					decltype(_watchers)().swap(_watchers);
					return;
				} else {
					_watchers[ub].swap(_watchers.back());
					_watchers.pop_back();
				}
			}
		}

		static bool destructive_intersect(std::vector<V>& host, const std::vector<V>& src) {
			bool now_empty = true;
			bool changed = false;
			ptrdiff_t ub = host.size();
			while (0 <= --ub) {
				if (std::ranges::any_of(src, [&](auto y) { return host[ub] == y; })) {
					now_empty = false;
					continue;
				}
				swap(host[ub], host.back());
				host.pop_back();
				changed = true;
			}
			if (now_empty) std::vector<V>().swap(host);
			return changed;
		}

		static void destructive_difference(std::vector<V>& host, const std::vector<V>& src) {
			bool now_empty = true;
			bool changed = false;
			ptrdiff_t ub = host.size();
			while (0 <= --ub) {
				if (!std::ranges::any_of(src, [&](auto y) { return host[ub] == y; })) {
					now_empty = false;
					continue;
				}
				swap(host[ub], host.back());
				host.pop_back();
				changed = true;
			}
			if (now_empty) std::vector<V>().swap(host);
			return changed;
		}

		static bool destructive_union(std::vector<V>& host, const std::vector<V>& src) {
			bool changed = false;
			for (decltype(auto) x : src) {
				if (std::ranges::any_of(host, [&x](auto y) { return x == y })) continue;
				host.push_back(x);
				changed = true;
			}
			return changed;
		}
	};

	template<class V>
	class UniformCartesianProductSubset final {
		std::vector<std::shared_ptr<Set<V> > > _args;
		std::shared_ptr<Set<std::vector<V> > > _elements;
		std::function<bool(const std::vector<V>&)> _axiom_predicate;

	public:
		UniformCartesianProductSubset() = default;
		UniformCartesianProductSubset(const decltype(_args)& args) : _args(args) {}
		UniformCartesianProductSubset(decltype(_args) && args) : _args(std::move(args)) {}
		UniformCartesianProductSubset(const decltype(_args)& args, decltype(_axiom_predicate) ok) : _args(args),_axiom_predicate(ok) {}
		UniformCartesianProductSubset(decltype(_args)&& args, decltype(_axiom_predicate) ok) : _args(std::move(args)), _axiom_predicate(ok) {}
		~UniformCartesianProductSubset() = default;

	private:
		void bootstrap() {
			if (_elements) return;
			// need to populate
			const auto strict_upper_bounds = get_upper_bounds(_args);
			if (std::ranges::any_of(strict_upper_bounds, [](auto x) { 0 >= x; })) throw std::logic_error("empty set in cartesian product");
			std::vector<size_t> key(_args.size(), 0);
			auto iter = Gray_code::cast_to(key, _args);
			bool ok = true;
			_elements = Set<V>::intuitionist_empty()
			do {
				if (!_axiom_predicate || _axiom_predicate(iter)) _elements->adjoin(std::shared_ptr<decltype(iter)>(new decltype(iter)(iter)));
				if (ok = Gray_code::increment(key)) Gray_code::cast_to(key, _args, iter);
			} while (ok);
		}
	};
}

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

template<class T>
static std::string format_as_primary_expression(std::shared_ptr<T> src) requires requires() {
	src->is_primary_term();
	src->desc();
}
{
	if (src->is_primary_term()) return src->desc();
	return std::string("(") + src->desc() + ")";
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
	if (sup == logics::kleene_weak || sup == logics::lisp_prolog) return false;
	if (sub == logics::kleene_strong) return true;
	return false;
}

constexpr std::optional<logics> common_logic(logics lhs, logics rhs)
{
	if (is_sublogic(lhs, rhs)) return rhs;
	if (is_sublogic(rhs, lhs)) return lhs;
	return std::nullopt;
}

// API
struct logic_API {
	constexpr virtual bool is_commutative() const { return true; }
	constexpr virtual bool is_out_of_of_range(TruthValue x) const { return false;  }
	constexpr virtual const char* name() const = 0;

	static constexpr const TruthValue And_identity = TruthValue::True;
	constexpr virtual TruthValue And_annihilator() const { return TruthValue::False; }
	constexpr virtual TruthValue Or_identity() const { return And_annihilator(); }
	constexpr virtual TruthValue Or_annihilator() const { return And_identity; }

	constexpr virtual TruthValue And(TruthValue lhs, TruthValue rhs) const = 0;
	constexpr virtual TruthValue Or(TruthValue lhs, TruthValue rhs) const = 0;

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

	constexpr std::optional<TruthValue> Or_core(TruthValue lhs, TruthValue rhs) const {
		if (is_out_of_of_range(lhs) || is_out_of_of_range(rhs)) throw std::logic_error(std::string(name()) + " logic out of range");

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
	constexpr TruthValue Or(TruthValue lhs, TruthValue rhs) const override { return Or_core(lhs, rhs).value(); };

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

	constexpr TruthValue Or(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = Or_core(lhs, rhs)) return *x;
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
	constexpr TruthValue Or_identity() const override { return TruthValue::False; }
	constexpr TruthValue Or_annihilator() const override { return TruthValue::Unknown; }

	constexpr TruthValue And(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = And_core(lhs, rhs)) return *x;
		return TruthValue::False;
	};

	constexpr TruthValue Or(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = Or_core(lhs, rhs)) return *x;
		return TruthValue::True;
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

	constexpr TruthValue Or(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = Or_core(lhs, rhs)) return *x;
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

	constexpr TruthValue Or(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = Or_core(lhs, rhs)) return *x;
		return TruthValue::True; // Unknown & Contradiction, or vice versa
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

	constexpr TruthValue Or(TruthValue lhs, TruthValue rhs) const override {
		if (auto x = Or_core(lhs, rhs)) return *x;
		return TruthValue::Unknown; // False & Unknown, or vice versa
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

// true n-ary connectives would warrant converting this to an abstract base class
class Connective final
{
private:
	static constexpr const bool integrity_check = true; // control expensive checks centrally

	std::string symbol;
	std::function<TruthValue(logics, TruthValue,TruthValue)> _op;	// typical core definition

public:
	Connective(const std::string& glyph, decltype(_op) op) : symbol(glyph), _op(op) {}
	~Connective() = default;

	auto& name() const { return symbol; }

	// diagonal.
	TruthValue eval(logics host, TruthValue src) { return _op(host, src, src); }
	// base case
	TruthValue eval(logics host, TruthValue lhs, TruthValue rhs) { return _op(host, lhs, rhs); }
	std::vector<unsigned short> enumerate(logics host, const std::vector<std::vector<TruthValue> >& src) {
		const int src_size = src.size();
		if (integrity_check && 0 >= src_size) throw std::logic_error("Connective::enumerate: nothing to enumerate");;
		if (integrity_check && 7 < src_size) throw std::logic_error("Connective::enumerate: implementation limit of 7 arguments");;
		std::vector<unsigned short> ret;
		std::vector<size_t> ubs = get_upper_bounds(src); // "upper bounds"
		std::vector<unsigned char> index(src.size(),0);
		std::vector<TruthValue> iter = Gray_code::cast_to(index, src);

		do {
			unsigned short stage = 0;
			TruthValue r_fold = (1 == src_size) ? eval(host, iter[0], iter[0]) : eval(host, iter[src_size-2], iter[src_size-1]);
			// load the bitmap with the argument tuple, compressed
			int ub = src_size;
			while (0 <= --ub) {
				stage <<= 2;
				stage |= (int)iter[ub];
			};
			if (2 < src_size) {	// assume truth-functional associative; otherwise a syntax error
				ub = src_size - 2;
				while (0 <= --ub) r_fold = eval(host, iter[ub], r_fold);
			};
			stage <<= 2;
			stage |= (int)r_fold;
			ret.push_back(stage);
			if (!Gray_code::increment(index, ubs)) break;
			Gray_code::cast_to(index, src, iter);
		} while(true);
		return ret;
	}
};

class TruthTable final
{
private:
	static constexpr const bool integrity_check = true; // control expensive checks centrally

	using logic_substitution_spec = std::pair<std::shared_ptr<TruthTable>, TruthValue>;
	using inverse_infer_spec = std::pair<std::shared_ptr<TruthTable>, logic_substitution_spec >;

	static std::vector<std::weak_ptr<TruthTable> > _cache;
	static std::vector<inverse_infer_spec> _inferred_reevaluations;

	static constexpr const TruthValue ref_classical[] = { TruthValue::False, TruthValue::True };
	static constexpr const TruthValue ref_threeval[] = { TruthValue::False, TruthValue::True, TruthValue::Unknown };
	static constexpr const TruthValue ref_fourval[] = { TruthValue::False, TruthValue::True, TruthValue::Unknown, TruthValue::Contradiction };
	static constexpr const TruthValue ref_display[] = { TruthValue::Contradiction, TruthValue::False, TruthValue::True, TruthValue::Unknown };

public:
	auto set_theoretic_range() const {
		switch (_logic) {
		case logics::classical: return std::ranges::subrange(ref_classical);
		case logics::kleene_strong:
		case logics::kleene_weak:
		case logics::lisp_prolog:	return std::ranges::subrange(ref_threeval);
		case logics::belnap:
		case logics::franci:	return std::ranges::subrange(ref_fourval);
		}
	}

private:
	std::string symbol;
	logics _logic;

	std::shared_ptr<enumerated::Set<TruthValue> > _prop_variable;

	// start legacy prototype
	std::vector<std::shared_ptr<TruthTable> > _args;
	std::shared_ptr<Connective> op; // for n-ary logical connectives/functions
	// end legacy prototype

	// for observer pattern: have to know who directly updates when we do
	mutable std::vector<std::weak_ptr<TruthTable> > _watching;

	// if we need these, undelete
	TruthTable() = delete;
	TruthTable(const TruthTable& src) = delete;
	TruthTable(TruthTable&& src) = delete;
	TruthTable& operator=(const TruthTable& src) = delete;
	TruthTable& operator=(TruthTable&& src) = delete;

	constexpr static unsigned char _var_agnostic(logics l) {
		switch (l) {
		case logics::classical: return 0x06;
		case logics::kleene_strong:
		case logics::kleene_weak:
		case logics::lisp_prolog:	return 0x0E;
		case logics::belnap:
		case logics::franci:	return 0x0F;
		}
	}

	// Propositional variable.  No arguments
	TruthTable(const std::string& name, logics l)
		: _logic(l),
		_prop_variable(enumerated::Set<TruthValue>::declare(name, set_theoretic_range()))
	{
	}

	// Unary connective
	TruthTable(const std::string& name, const std::shared_ptr<TruthTable>& src)
		: symbol(name),
		_logic(src->_logic),
		_prop_variable(enumerated::Set<TruthValue>::declare(set_theoretic_range())),
		_args(1,src)
	{
	}

	// binary connective
	// \todo: fixup logic field (ask the operation)
	TruthTable(const std::shared_ptr<TruthTable>& lhs, const std::shared_ptr<TruthTable>& rhs, decltype(op) update) : _logic(common_logic(lhs->_logic, rhs->_logic).value()), _args(2), op(update) {
		_args[0] = lhs;
		_args[1] = rhs;
	}

public:
	~TruthTable() = default;

	static auto display_range() { return std::ranges::subrange(ref_display); }
	static auto count_expressions() { return _cache.size(); }
	static auto count_inferred_reevaluations() { return _inferred_reevaluations.size(); }

	auto logic() const { return _logic; }
	auto arity() const { return _args.size(); }
	auto& name() const { 
		if (op) return op->name();
		if (!symbol.empty()) return symbol;
		if (_prop_variable) return _prop_variable->name();
		return symbol;
	}
	bool is_propositional_variable() const { return _args.empty(); }

	bool is_primary_term() const {
		if (_args.empty()) return true;	// propositional variable is ok
		if (symbol == "~") return true; // hard-coded logical not
		return false;	// haven't done these yet.
	}

	std::string desc() const {
		if (_args.empty()) {
			if (_prop_variable) return _prop_variable->name();
			return symbol;
		}

		const auto sym = name();
		if (sym == "~") {
			// hard-coded logical not; unary prefix operation.
			return sym + format_as_primary_expression(_args.front());
		}

		if (sym == "&rArr;") {
			// hard-coded binary infix operations
			return format_as_primary_expression(_args.front()) + sym + format_as_primary_expression(_args.back());
		}

		// general case: assume function-like
		bool have_seen_first = false;
		std::string ret(name());
		ret += "(";
		for (decltype(auto) x : _args) {
			if (have_seen_first) ret += ", ";
			ret += x->desc();
			have_seen_first = true;
		}
		ret += ")";
		return ret;
	}

	/// <summary>
	/// Ignores syntactical equivalences; this is about the parse tree being identical.
	/// We don't check the sublogic relation here, as we don't have context.
	/// </summary>
	static bool are_equivalent(const std::shared_ptr<TruthTable>& lhs, const std::shared_ptr<TruthTable>& rhs)
	{
		if (!lhs || !rhs) return false;	// pretend null is not equivalent to null, that's an error case anyway
		if (lhs.get() == rhs.get()) return true;
		if (lhs->name() != rhs->name()) return false;
		// \todo? screen on op as well?
		auto ub = lhs->arity();
		static_assert(std::is_unsigned_v<decltype(ub)>);
		if (ub != rhs->arity()) return false;
		while (0 < ub) {
			--ub;
			if (!are_equivalent(lhs->_args[ub], rhs->_args[ub])) return false;
		};
		return true;
	}

	auto catalog_vars() {
		std::vector<TruthTable*> ret;
		_catalog_vars(this, ret);
		return ret;
	}

	std::optional<std::vector<TruthValue> > possible_values() const {
		if (_prop_variable) {
			if (const auto x = _prop_variable->possible_values()) return *x;	// \todo eliminate full copy here
			return std::nullopt;
		}
		// \todo other implementations
		return std::nullopt;
	}

	// actively infer a truth value (triggers non-contradiction processing)
	static void infer(std::shared_ptr<TruthTable> target, TruthValue src, std::shared_ptr<TruthTable> origin=nullptr) {
		if (target->_prop_variable) {
			target->_prop_variable->force_equal(src);
			return;
		}
		// \todo other implementations
	}

	static void exclude(std::shared_ptr<TruthTable> target, TruthValue src, std::shared_ptr<TruthTable> origin = nullptr) {
		if (target->_prop_variable) {
			target->_prop_variable->force_unequal(src);
			return;
		}
		// \todo other implementations
	}

	// factories
	static std::shared_ptr<TruthTable> variable(const std::string& name, logics l) {
		if (auto x = is_in_cache(name, l)) return *x;

		std::shared_ptr<TruthTable> stage(new TruthTable(name, l));
		if constexpr (integrity_check) {
			if (!stage->syntax_ok()) throw std::logic_error("invalid constructor");
		}

		_cache.push_back(stage);
		return stage;
	}

	static std::shared_ptr<TruthTable> Not(const std::shared_ptr<TruthTable>& src) {
		if (!src) throw std::logic_error("empty proposition");
		if (auto x = is_in_cache("~", src)) return *x;

		std::shared_ptr<TruthTable> stage(new TruthTable("~", src));
		if constexpr (integrity_check) {
			if (!stage->syntax_ok()) throw std::logic_error("invalid constructor");
		}

		// wire in propositional variable updaters
		std::shared_ptr<enumerated::Set<TruthValue> > src_var = src->_prop_variable;
		std::shared_ptr<enumerated::Set<TruthValue> > new_var = stage->_prop_variable;
		if (src_var && new_var) {
			auto src_to_new = [weak_new_var = std::weak_ptr<enumerated::Set<TruthValue> >(new_var)](const std::vector<TruthValue>& src) {
				auto dest_var = weak_new_var.lock();
				if (!dest_var) return false;
				if (1 == src.size()) {
					dest_var->force_equal(!src.front());
					return true;
				}
				std::vector<TruthValue> stage(src);
				for (decltype(auto) x : stage) x = !x;
				dest_var->restrict_to(stage);
				if (dest_var->empty()) throw logic::proof_by_contradiction("required equality failed");
				return true;
			};
			auto new_to_src = [weak_src_var = std::weak_ptr<enumerated::Set<TruthValue> >(src_var)](const std::vector<TruthValue>& src) {
				auto dest_var = weak_src_var.lock();
				if (!dest_var) return false;
				if (1 == src.size()) {
					dest_var->force_equal(!src.front());
					return true;
				}
				std::vector<TruthValue> stage(src);
				for (decltype(auto) x : stage) x = !x;
				dest_var->restrict_to(stage);
				if (dest_var->empty()) throw logic::proof_by_contradiction("required equality failed");
				return true;
			};
			src_var->watched_by(std::shared_ptr<zaimoni::observer<std::vector<TruthValue> > >(new zaimoni::lambda_observer<std::vector<TruthValue> >(src_to_new)));
			new_var->watched_by(std::shared_ptr<zaimoni::observer<std::vector<TruthValue> > >(new zaimoni::lambda_observer<std::vector<TruthValue> >(new_to_src)));
		}

		_cache.push_back(stage);
		src->is_arg_for(stage);
		return stage;
	}

private:
#if 0
	static unsigned char update_Not(const std::shared_ptr<TruthTable>& src) {
		unsigned char ret = 0;
		if (const auto x = src->possible_values()) {
			for (decltype(auto) t : *x) update_truth_bitmap(ret, !t);
		}
		return ret;
	}

	static void infer_Not(TruthValue forced, const std::shared_ptr<TruthTable>& origin) {
		infer(origin->_args.front(), !forced, origin);
	}
#endif

	static TruthValue _nonstrict_implication(logics host, TruthValue lhs, TruthValue rhs) {
		return toAPI(host).Or(!lhs, rhs);
	}
	static auto nonstrict_implication() {
		static std::shared_ptr<Connective> ooao(new Connective("&rArr;", &_nonstrict_implication));
		return ooao;
	}

public:
	static std::shared_ptr<TruthTable> NonStrictlyImplies(const std::shared_ptr<TruthTable>& hypothesis, const std::shared_ptr<TruthTable>& consequence) {
		if (!hypothesis) throw std::logic_error("empty proposition");
		if (!consequence) throw std::logic_error("empty proposition");
		if (    hypothesis->logic() != consequence->logic()
			&& !is_sublogic(hypothesis->logic(), consequence->logic())
			&& !is_sublogic(consequence->logic(), hypothesis->logic()))
			throw std::logic_error("incompatible hypothesis and consequence");

		std::shared_ptr<TruthTable> stage(new TruthTable(hypothesis, consequence, nonstrict_implication()));
		if constexpr (integrity_check) {
			if (!stage->syntax_ok()) throw std::logic_error("invalid constructor");
		}

		if (auto x = is_in_cache(stage)) return *x;

		_cache.push_back(stage);
		hypothesis->is_arg_for(stage);
		consequence->is_arg_for(stage);
		return stage;
	}

// main private section
private:
	bool syntax_ok() const {
		if (op && 2 > _args.size()) return false;
		return true;
	}

	void is_arg_for(std::shared_ptr<TruthTable>& src) const {
		if (src) _watching.push_back(src);
	}

	static std::optional<std::shared_ptr<TruthTable> > is_in_cache(const std::shared_ptr<TruthTable>& src)
	{
		auto ub = _cache.size();
		while (0 < ub) {
			--ub;
			if (auto x = _cache[ub].lock()) {
				if (!are_equivalent(x, src)) continue;
				if (is_sublogic(x->_logic, src->_logic)) return x;
				return nullptr;
			} else {
				_cache[ub].swap(_cache.back());
				_cache.pop_back();
			}
		}
		return std::nullopt;
	}

	// checking for propositional variable without actually constructing it beforehand
	static std::optional<std::shared_ptr<TruthTable> > is_in_cache(const std::string& name, logics target_logic)
	{
		auto ub = _cache.size();
		while (0 < ub) {
			--ub;
			if (auto x = _cache[ub].lock()) {
				if (name != x->name()) continue;
				if (!x->is_propositional_variable()) continue;
				if (is_sublogic(x->_logic, target_logic)) return x;
				return nullptr;
			} else {
				_cache[ub].swap(_cache.back());
				_cache.pop_back();
			}
		}
		return std::nullopt;
	}

	static std::optional<std::shared_ptr<TruthTable> > is_in_cache(const std::string& sym, const std::shared_ptr<TruthTable>& src)
	{
		auto ub = _cache.size();
		while (0 < ub) {
			--ub;
			if (auto x = _cache[ub].lock()) {
				if (sym != x->symbol) continue;
				if (1 != x->_args.size()) continue;
				if (!are_equivalent(x->_args.front(), src)) continue;
				return x;
			} else {
				_cache[ub].swap(_cache.back());
				_cache.pop_back();
			}
		}
		return std::nullopt;
	}

	static void request_reevaluations(std::shared_ptr<TruthTable> target, TruthValue src, std::shared_ptr<TruthTable> origin) {
		auto& audience = target->_watching;
		if (ptrdiff_t ub = audience.size()) {
			const auto authority = origin ? origin.get() : nullptr;
			while (0 <= --ub) {
				if (auto x = audience[ub].lock()) {
					if (authority == x.get()) continue;
					_inferred_reevaluations.push_back({ x , { target, src } });
				}
				else {
					audience[ub].swap(audience.back());
					audience.pop_back();
				}
			}
		}
	}

	static bool execute_reevaluation() {
		if (!_inferred_reevaluations.empty()) {
			auto& stage = _inferred_reevaluations.front();
			auto ret = stage.first->execute_reevaluation(stage.second);
			_inferred_reevaluations.erase(_inferred_reevaluations.begin());
		}
		return false;
	}

	bool execute_reevaluation(const logic_substitution_spec& src) {
		if (_args.empty()) throw std::logic_error(desc()+", execute substitution: no arguments");
		if (!src.first) throw std::logic_error(desc() + ", execute substitution: no rationale");
		ptrdiff_t ub = _args.size();
		while (0 <= --ub) {
			if (_args[ub].get() == src.first.get()) break;
		}
		if (0 > ub) throw std::logic_error(desc() + ", execute substitution: do not have required target");
		if (op) {
			// \todo should just prune already-calculated _var_enumerated rather than discard completely
			return true;
		}
		return false; // but already invalid syntax if we get here
	}

	// this does not handle the parsing aspects of syntactical entailment
	static TruthValue _core_syntactical_entailment(logics host, TruthValue lhs, TruthValue rhs) {
		if (TruthValue::True != lhs) return TruthValue::True; // no inference in this case
		if (TruthValue::True == rhs) return TruthValue::True; // ok (at truth value level, not necessarily as valid reasoning)
		return TruthValue::False;
	}
	static auto core_syntactical_entailment() {
		static std::shared_ptr<Connective> ooao(new Connective("&#9500;", &_core_syntactical_entailment));
		return ooao;
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

	/* constexpr? */ static void update_truth_bitmap(unsigned char& dest, TruthValue src) { dest |= (1ULL << (int)src); }

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
		auto to_append = decode_bitmap(src, display_range());
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

	static std::optional<std::vector<std::vector<TruthValue> > > toTruthVector(const decltype(_args)& src) {
		std::vector<std::vector<TruthValue> > ret(src.size());
		size_t n = 0;
		for (decltype(auto) x : src) {
			if (auto stage = x->possible_values()) ret[n] = std::move(*stage);
			else return std::nullopt;
			n++;
		}
		return ret;
	}

	static void _catalog_vars(TruthTable* origin, std::vector<TruthTable*>& ret) {
retry:
		// we almost certainly will be out of RAM long before this cast is undefined behavior
		const ptrdiff_t strict_ub = origin->_args.size();
		switch(strict_ub) {
		case 0: // base case
			if (ptrdiff_t ub2 = ret.size()) {
				while (0 <= --ub2) if (origin == ret[ub2]) return;
			}
			ret.push_back(origin);
			return;
		case 1:	// tail-recurse
			origin = origin->_args.front().get();
			goto retry;
		}
		ptrdiff_t n = strict_ub;
		while (0 <= --n) _catalog_vars(origin->_args[strict_ub -n-1].get(), ret);
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
