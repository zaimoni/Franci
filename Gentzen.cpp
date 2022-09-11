#include "Zaimoni.STL/LexParse/LexNode.hpp"

#ifdef GENTZEN_DRIVER
#include "HTMLtag.hpp"
#include "Zaimoni.STL/LexParse/string_view.hpp"
#include "Zaimoni.STL/stack.hpp"
#include "test_driver.h"
#include "Zaimoni.STL/Pure.C/comptest.h"
#include <filesystem>
#include <memory>
#include <fstream>
#include <iostream>
#include <span>
#include <initializer_list>
#include <any>
#include <tuple>

#include <algorithm>

#include "errcount.hpp"

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
#include <io.h>
#else
#include <unistd.h>
#endif

static error_counter<size_t> Errors(100, "too many errors");
static error_counter<size_t> Warnings(1000, "too many warnings");

// stub for more sophisticated error reporting
void error_report(const formal::src_location& loc, const std::string& err) {
	if (loc.path) std::wcerr << loc.path->native();
	else std::cerr << "<unknown>";
	std::cerr << loc.to_s() << ": error : " << err << '\n';
	++Errors;
}

void error_report(formal::lex_node& fail, const std::string& err) {
	error_report(fail.origin(), err);
	fail.learn(formal::Error);
}

void warning_report(const formal::src_location& loc, const std::string& warn) {
	if (loc.path) std::wcerr << loc.path->native();
	else std::cerr << "<unknown>";
	std::cerr << loc.to_s() << ": warning : " << warn << '\n';
	++Warnings;
}

const std::filesystem::path& self_path(const char* const _arg)
{
	static std::filesystem::path ooao;
	if (!_arg || !*_arg) return ooao;
	if (!ooao.empty()) return ooao;	// XXX invariant failure \todo debug mode should hard-error
	// should be argv[0], which exists as it is our name
	ooao = _arg;
	return ooao;
}

static void help(void)
{
	STRING_LITERAL_TO_STDOUT("Gentzen\n");
	STRING_LITERAL_TO_STDOUT("usage: gentzen infile\n");
	exit(EXIT_SUCCESS);
}

static bool process_option(std::string_view arg)
{
	static bool no_more = false;
	static const constexpr std::string_view longopt_prefix("--"); // Cf. GNU software
	if (no_more) return false;
	if (!arg.starts_with(longopt_prefix)) return false;
	if (arg == longopt_prefix) {
		no_more = true;
		return true;
	}
	// \todo actually implement
	return true;
}

// from C:Z zero.h

// https://devblogs.microsoft.com/oldnewthing/20200413-00/?p=103669 [Raymond Chen/"The Old New Thing"]
template<typename T, typename...>
using unconditional_t = T;

template<typename T, T v, typename...>
inline constexpr T unconditional_v = v;

// https://artificial-mind.net/blog/2020/11/14/cpp17-consteval
template <auto V>
static constexpr const auto force_consteval = V;

// end from C:Z zero.h

// prototype lookup -- extract to own header when stable

constexpr const std::pair<const char*, char32_t> HTML_entities[] = {
	{"forall", 0x2200ULL},
	{"exist", 0x2203ULL},
	{"empty", 0x2205ULL},
	{"isin", 0x2208ULL},
	{nullptr, 0x2209ULL} // := ~(... &isin; ...) when ... &isin; ... is well-formed
};

constexpr const std::pair<const char*, char32_t>* lookup_HTML_entity(const std::string_view& name)
{
	for (decltype(auto) x : HTML_entities) {
		if (x.first && name == x.first) return &x;
	}
	return nullptr;
}

constexpr const std::pair<const char*, char32_t>* lookup_HTML_entity(char32_t codepoint)
{
	for (decltype(auto) x : HTML_entities) {
		if (x.second == codepoint) return &x;
	}
	return nullptr;
}

static_assert(0x2200UL == lookup_HTML_entity("forall")->second);
static_assert(0x2200UL == lookup_HTML_entity(0x2200UL)->second);

// end prototype lookup

std::optional<std::variant<std::string_view, char32_t> > interpret_HTML_entity(std::string_view src) {
	if (3 > src.size()) return std::nullopt;
	if (';' != src.back()) return std::nullopt;
	src.remove_suffix(1);
	if ('&' != src.front()) return std::nullopt;
	src.remove_prefix(1);
	if ('#' != src.front()) return src; // non-numeric
	src.remove_prefix(1);
	if (src.empty()) return std::nullopt;
	if ('x' == src.front()) { // hexadecimal
		src.remove_prefix(1);
		if (src.empty()) return std::nullopt;
		auto text = kleene_star(src, is_hex_digit);
		if (!text || text->first != src) return std::nullopt;
		try {
			return std::stoul(std::string(src), nullptr, 16);
		} catch (const std::exception& e) { // just eat the exception
			return std::nullopt;
		}
	}
	// decimal
	auto text = kleene_star(src, is_digit);
	if (!text || text->first != src) return std::nullopt;
	try {
		return std::stoul(std::string(src), nullptr, 10);
	} catch (const std::exception& e) { // just eat the exception
		return std::nullopt;
	}
}

bool interpret_HTML_entity(std::string_view src, const std::string_view& ref) {
	if (const auto test = interpret_HTML_entity(src)) {
		if (auto x = std::get_if<std::string_view>(&(*test))) return ref == *x;
		if (auto transcode = lookup_HTML_entity(std::get<char32_t>(*test))) return ref == transcode->first;
	}
	return false;
}

formal::word* interpret_HTML_entity(const formal::lex_node& src)
{
	if (src.code() & HTMLtag::Entity) {
		if (1 != src.is_pure_anchor()) return nullptr; // \todo invariant violation
		const auto ret = src.anchor<formal::word>();
		if (ret->code() & HTMLtag::Entity) return ret;
		// \todo invariant violation
	}
	return nullptr;
}

bool interpret_HTML_entity(const formal::lex_node& src, const std::string_view& ref)
{
	if (const auto w = interpret_HTML_entity(src)) return interpret_HTML_entity(w->value(), ref);
	return false;
}

template<size_t n>
const std::string_view* interpret_HTML_entity(const formal::lex_node& src, const std::array<std::string_view, n>& ref)
{
	if (const auto w = interpret_HTML_entity(src)) {
		for (decltype(auto) x : ref) {
			if (auto test = interpret_HTML_entity(w->value(), x)) return &x;
		}
	}
	return nullptr;
}

static std::string wrap_to_HTML_entity(const std::string_view& src) {
	std::string ret;
	if (!src.starts_with('&')) ret += '&';
	ret += src;
	if (!src.ends_with(';')) ret += ';';
	return ret;
};

// prototype class -- extract to own files when stable

namespace formal {

	// many programming languages have the notion of a token, i.e. formal word, that is a syntax error by construction
	class is_wff { // is well-formed formula
	public:
		// offset, change target, constraint (usually some sort of std::function using language-specific types)
		using subsequence = std::tuple<size_t, std::span<formal::lex_node*>, std::any >;
		using change_target = std::pair<size_t, std::span<formal::lex_node*> >;
		using parse_t = std::function<change_target()>;

		// true, "": no comment, ok
		// true, std::string: warning
		// false, std:: string: error
		// false, "": no comment, error
		using wff_t = std::pair<bool, std::string>;
		using ret_parse_t = std::pair<wff_t, parse_t>;

	private:
		std::vector<std::function<std::optional<ret_parse_t>(subsequence src)> > _well_formed_span;
		std::vector<std::function<std::optional<wff_t>(const formal::lex_node& src)> > _well_formed_lex_node;
		std::vector<std::function<std::optional<wff_t>(const formal::parsed& src)> > _well_formed_parsed;
		std::vector<std::function<std::optional<wff_t>(const formal::word& src)> > _well_formed_word;

	public:
		is_wff() = default;
		is_wff(const is_wff& src) = default;
		is_wff(is_wff&& src) = default;
		is_wff& operator=(const is_wff& src) = default;
		is_wff& operator=(is_wff&& src) = default;
		~is_wff() = default;

		// thin forwarders for std::visit
		std::optional<wff_t> operator()(std::unique_ptr<formal::word>& src) { return operator()(*src); }
		std::optional<wff_t> operator()(std::unique_ptr<formal::lex_node>& src) { return operator()(*src); }
		std::optional<wff_t> operator()(std::unique_ptr<formal::parsed>& src) { return operator()(*src); }

		void register_handler(std::function<std::optional<ret_parse_t>(subsequence src)> x) { _well_formed_span.push_back(x); }
		void register_handler(std::function<std::optional<wff_t>(const formal::lex_node& src)> x) { _well_formed_lex_node.push_back(x); }
		void register_handler(std::function<std::optional<wff_t>(const formal::parsed& src)> x) { _well_formed_parsed.push_back(x); }
		void register_handler(std::function<std::optional<wff_t>(const formal::word& src)> x) { _well_formed_word.push_back(x); }

		// base cases
		wff_t operator()(const formal::lex_node& src) {
			for (decltype(auto) h : _well_formed_lex_node) {
				if (auto ret = h(src)) return *ret;
			}
			return std::pair(false, std::string());
		}

		wff_t operator()(const formal::parsed& src) {
			for (decltype(auto) h : _well_formed_parsed) {
				if (auto ret = h(src)) return *ret;
			}
			return std::pair(false, std::string());
		}

		wff_t operator()(const formal::word& src) {
			for (decltype(auto) h : _well_formed_word) {
				if (auto ret = h(src)) return *ret;
			}
			return std::pair(true, std::string());
		}

		ret_parse_t operator()(subsequence src) {
			std::optional<ret_parse_t> fallback;
			for (decltype(auto) h : _well_formed_span) {
				if (auto ret = h(src)) {
					if (ret->first.first) return *ret;
					if (!fallback) fallback = std::move(*ret);
					else if (!fallback->second && ret->second) fallback = std::move(*ret);
				};
			}
			if (fallback) return *fallback;
			decltype(auto) target = std::get<1>(src);
			if (1 == target.size()) return ret_parse_t(operator()(**target.begin()), nullptr);
			return ret_parse_t(std::pair(false, std::string()), nullptr);
		}
	};

} // end namespace formal

namespace gentzen {

	formal::is_wff syntax_check;

	// this has to be formal::parsed because it has to handle set-builder notation
	class domain : public formal::parsed {
	protected:
		domain() = default;

	public:
		domain(const domain& src) = default;
		domain(domain&& src) = default;
		domain& operator=(const domain& src) = default;
		domain& operator=(domain&& src) = default;
		~domain() = default;

		virtual void MoveInto(domain*& dest) = 0;	// polymorphic move
	};

	class preaxiomatic final : public domain {
		unsigned int _code; // beware type promotion
		static constexpr const std::array<std::string_view, 5> names = {
				"<b>TruthValued</b>",
				"<b>TruthValues</b>",
				"<b>Set</b>",
				"<b>Ur</b>",
				"<b>Class</b>"
		};
		static constexpr const std::array<std::string_view, 5> parsing = substr(names, 3, -4);

	public:
		enum class Domain {
			TruthValued = 0,
			TruthValues,
			Set,
			Ur,
			Class
		};

		static constexpr const std::array<Domain, 5> Domain_values = {
			Domain::TruthValued,
			Domain::TruthValues,
			Domain::Set,
			Domain::Ur,
			Domain::Class
		};

		preaxiomatic() = delete; // empty variable doesn't make much sense
		preaxiomatic(const preaxiomatic& src) = delete;
		preaxiomatic(preaxiomatic&& src) = delete;
		preaxiomatic& operator=(const preaxiomatic& src) = delete;
		preaxiomatic& operator=(preaxiomatic&& src) = delete;
		~preaxiomatic() = default;

		static std::shared_ptr<const domain> get(Domain src) {
			static std::shared_ptr<const domain> ooao[5];

			decltype(auto) x = ooao[(unsigned int)src];
			if (!x) x = std::shared_ptr<const domain>(new preaxiomatic(src));
			return x;
		}

		static std::optional<Domain> get(domain* src) {
			if (!src) return std::nullopt;

			for (decltype(auto) x : Domain_values) if (src == get(x).get()) return x;
			return std::nullopt;
		}

		// these don't work
		std::unique_ptr<parsed> clone() const override { throw std::logic_error("no cloning preaxiomatic"); }
		void CopyInto(parsed*& dest) const override { throw std::logic_error("no CopyInto for preaxiomatic"); }
		void MoveInto(parsed*& dest) override { throw std::logic_error("no MoveInto for preaxiomatic"); }
		void MoveInto(domain*& dest) override { throw std::logic_error("no MoveInto for preaxiomatic"); }

		formal::src_location origin() const override { return formal::src_location(); }

		static constexpr auto to_s(Domain src) { return names[(size_t)src]; }

		std::string to_s() const override {
			if (names.size() > _code) return std::string(names[_code]);
			return "<buggy>";
		}

		static std::optional<Domain> parse(const formal::lex_node& src) {
			if (!HTMLtag::is_balanced_pair(src, "b")) return std::nullopt;
			if (0 < src.postfix_size()) return std::nullopt; // \todo extend to handle abstract algebraic category theory
			if (1 != src.infix_size()) return std::nullopt;

			decltype(auto) node = *src.infix(0);
			if (1 != node.is_pure_anchor()) return std::nullopt;

			const auto text = node.anchor<formal::word>()->value();
			ptrdiff_t i = -1;
			for (decltype(auto) x : parsing) {
				++i;
				if (text == x) return (Domain)(i);
			}

			return std::nullopt;
		}

	private:
		preaxiomatic(Domain src) noexcept : _code((unsigned int)src) {}

	};

	constexpr bool is_properly_contained_in(preaxiomatic::Domain lhs, preaxiomatic::Domain rhs) {
		switch (lhs) {
		case preaxiomatic::Domain::TruthValues: return preaxiomatic::Domain::TruthValued == rhs || preaxiomatic::Domain::Ur == rhs;
		case preaxiomatic::Domain::Set: return preaxiomatic::Domain::Class == rhs;
		default: return false;
		}
	}

	constexpr bool is_contained_in(preaxiomatic::Domain lhs, preaxiomatic::Domain rhs) {
		if (lhs == rhs) return true;
		return is_properly_contained_in(lhs, rhs);
	}

	constexpr bool are_disjoint(preaxiomatic::Domain lhs, preaxiomatic::Domain rhs) {
		if (lhs == rhs) return false;
		// to be constexpr this has to be duplicated; static auto lambda disallowed
		if (is_properly_contained_in(lhs, rhs) || is_properly_contained_in(rhs, lhs)) return false;

		// truth values pre-exist mathematical objects
		if (is_contained_in(lhs, preaxiomatic::Domain::TruthValues) && is_contained_in(rhs, preaxiomatic::Domain::Class)) return true;
		if (is_contained_in(rhs, preaxiomatic::Domain::TruthValues) && is_contained_in(lhs, preaxiomatic::Domain::Class)) return true;
		// but *notations* are truth-valued
		if (is_contained_in(lhs, preaxiomatic::Domain::TruthValued) && is_contained_in(preaxiomatic::Domain::Set, rhs)) return false;
		if (is_contained_in(rhs, preaxiomatic::Domain::TruthValued) && is_contained_in(preaxiomatic::Domain::Set, lhs)) return false;
		// no ur-element is a mathematical object
		if (is_contained_in(lhs, preaxiomatic::Domain::Ur) && is_contained_in(rhs, preaxiomatic::Domain::Class)) return true;
		if (is_contained_in(rhs, preaxiomatic::Domain::Ur) && is_contained_in(lhs, preaxiomatic::Domain::Class)) return true;

		// derivable from above:
		if (preaxiomatic::Domain::TruthValued == lhs && preaxiomatic::Domain::Ur == rhs) return false;
		if (preaxiomatic::Domain::TruthValued == rhs && preaxiomatic::Domain::Ur == lhs) return false;
		return false; // not really, but should not be reached
	}

	// these are rejected by the general implementation
	static_assert(!are_disjoint(preaxiomatic::Domain::Class, preaxiomatic::Domain::Class));
	static_assert(!are_disjoint(preaxiomatic::Domain::Class, preaxiomatic::Domain::Set));
	static_assert(!are_disjoint(preaxiomatic::Domain::Set, preaxiomatic::Domain::Class));
	static_assert(!are_disjoint(preaxiomatic::Domain::Set, preaxiomatic::Domain::Set));
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::TruthValued));
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::TruthValues));
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValues, preaxiomatic::Domain::TruthValued));
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValues, preaxiomatic::Domain::TruthValues));
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValues, preaxiomatic::Domain::Ur));
	static_assert(!are_disjoint(preaxiomatic::Domain::Ur, preaxiomatic::Domain::TruthValues));
	static_assert(!are_disjoint(preaxiomatic::Domain::Ur, preaxiomatic::Domain::Ur));
	// these are allowed by the general implementation
	static_assert(!are_disjoint(preaxiomatic::Domain::Class, preaxiomatic::Domain::TruthValued));
	static_assert(!are_disjoint(preaxiomatic::Domain::Set, preaxiomatic::Domain::TruthValued));
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::Class));
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::Set));
	static_assert(are_disjoint(preaxiomatic::Domain::Class, preaxiomatic::Domain::Ur));
	static_assert(are_disjoint(preaxiomatic::Domain::Set, preaxiomatic::Domain::Ur));
	static_assert(are_disjoint(preaxiomatic::Domain::Ur, preaxiomatic::Domain::Class));
	static_assert(are_disjoint(preaxiomatic::Domain::Ur, preaxiomatic::Domain::Set));
	// above are a matter of how we formalize
	static_assert(!are_disjoint(preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::Ur));
	static_assert(!are_disjoint(preaxiomatic::Domain::Ur, preaxiomatic::Domain::TruthValued));
	// above is derivable
	static_assert(are_disjoint(preaxiomatic::Domain::Class, preaxiomatic::Domain::TruthValues));
	static_assert(are_disjoint(preaxiomatic::Domain::Set, preaxiomatic::Domain::TruthValues));
	static_assert(are_disjoint(preaxiomatic::Domain::TruthValues, preaxiomatic::Domain::Class));
	static_assert(are_disjoint(preaxiomatic::Domain::TruthValues, preaxiomatic::Domain::Set));

	// classical logic cross-check: lift general implementation to static assertion
	static constexpr const auto test1 = cartesian::product(preaxiomatic::Domain_values, preaxiomatic::Domain_values);
	static constexpr auto test2 = perl::grep<perl::count_if(test1, is_properly_contained_in)>(test1, is_properly_contained_in);
	static_assert(std::end(test2) != std::find(test2.begin(), test2.end(), std::pair(preaxiomatic::Domain::TruthValues, preaxiomatic::Domain::TruthValued)));
	static constexpr auto test3 = perl::count_if(preaxiomatic::Domain_values, preaxiomatic::Domain_values, is_properly_contained_in);
	static constexpr auto test4 = perl::enumerate<perl::count_if(preaxiomatic::Domain_values, preaxiomatic::Domain_values, is_properly_contained_in)>(preaxiomatic::Domain_values, preaxiomatic::Domain_values, is_properly_contained_in);
	static_assert(std::end(test4) != std::find(test4.begin(), test4.end(), std::pair(preaxiomatic::Domain::TruthValues, preaxiomatic::Domain::TruthValued)));

	class domain_param final : public std::variant<preaxiomatic::Domain, unsigned long, domain*> {
		using base = std::variant<preaxiomatic::Domain, unsigned long, domain*>;
		using listing = zaimoni::stack<preaxiomatic::Domain, preaxiomatic::Domain_values.size()>;

	public:
		constexpr domain_param(preaxiomatic::Domain src) noexcept : base{src} {}
		constexpr domain_param(domain* src) : base{src} {
			if (!src) throw std::invalid_argument("null src");
			if (auto ok = preaxiomatic::get(src)) *this = *ok;
		}
		constexpr domain_param(std::nullptr_t) noexcept = delete;
		constexpr domain_param(const domain_param&) = default;
		constexpr domain_param(domain_param&&) = default;
		constexpr domain_param& operator=(const domain_param&) = default;
		constexpr domain_param& operator=(domain_param&&) = default;
		constexpr ~domain_param() = default;

		constexpr domain_param(const std::initializer_list<preaxiomatic::Domain>& accept,
			const std::initializer_list<preaxiomatic::Domain>& reject = {})
		: base(0UL)
		{
			if (1 == accept.size() && std::empty(reject)) {
				base::operator=(*accept.begin());
				return;
			}
			base::operator=(encode(accept, reject));
		}

		// diagnostic
		constexpr unsigned long raw_code() const {
			if (auto ret = std::get_if<unsigned long>(this)) return *ret;
			return 0;
		}

		constexpr std::optional<bool> accept(const domain_param& src) const {
			auto my_code = raw_code();
			auto src_code = src.raw_code();

			const auto [my_require, my_reject] = decode();
			const auto [src_provides, src_does_not_provide] = src.decode();

			for (decltype(auto) have : src_provides) {
				for (decltype(auto) no : my_reject) if (no == have) return false;
				for (decltype(auto) want : my_require) if (want == have) return true;
			}

			for (decltype(auto) want : my_require) {
				for (decltype(auto) no : src_does_not_provide) if (want == no) return false;
			}

			for (decltype(auto) no : my_reject) {
				for (decltype(auto) want : my_require) {
					if (is_contained_in(want, no)) {
						for (decltype(auto) have : src_provides) {
							if (is_contained_in(have, want)) return true;
							if (is_contained_in(have, no)) return false;
						}
					}
					if (is_contained_in(no, want)) {
						for (decltype(auto) have : src_provides) {
							if (is_contained_in(have, no)) return false;
							if (is_contained_in(have, want)) return true;
						}
					}
				}
			}

			return std::nullopt;
		}

	private:
		static constexpr unsigned long encode_isin(preaxiomatic::Domain x) { return (unsigned long)x; }
		static constexpr unsigned long encode_not_isin(preaxiomatic::Domain x) { return (unsigned long)x + preaxiomatic::Domain_values.size(); }

		static constexpr unsigned long encode(const std::initializer_list<preaxiomatic::Domain>& accept,
			const std::initializer_list<preaxiomatic::Domain>& reject) {

			std::string err;
			if (2 <= accept.size()) {
				decltype(auto) x = std::data(accept);
				ptrdiff_t n = accept.size();
				while (1 <= --n) {
					ptrdiff_t i = n;
					while (0 <= --i) {
						if (is_contained_in(x[i], x[n])) {
							err = "Accepting ";
							err += preaxiomatic::to_s(x[n]);
							err += " makes accepting ";
							err = preaxiomatic::to_s(x[i]);
							err += " redundant.";
							throw std::invalid_argument(std::move(err));
						}
						if (is_contained_in(x[n], x[i])) {
							err = "Accepting ";
							err += preaxiomatic::to_s(x[i]);
							err += " makes accepting ";
							err = preaxiomatic::to_s(x[n]);
							err += " redundant.";
							throw std::invalid_argument(std::move(err));
						}
					}
				}
			}

			for (decltype(auto) want : accept) {
				for (decltype(auto) no : reject) {
					if (want == no) {
						err = "Both accepting and rejecting ";
						err += preaxiomatic::to_s(want);
						err += " is paradoxical.";
						throw std::invalid_argument(std::move(err));
					}

				}
			}

			unsigned long ret = 1;
			for (decltype(auto) x : accept) {
				ret *= force_consteval<2 * preaxiomatic::Domain_values.size()>;
				ret += encode_isin(x);
			};
			for (decltype(auto) x : reject) {
				ret *= force_consteval<2 * preaxiomatic::Domain_values.size()>;
				ret += encode_not_isin(x);
			};
			return ret;
		}
 
		constexpr std::pair<listing, listing> decode() const {
			struct interpret {
				constexpr auto operator()(preaxiomatic::Domain src) {
					listing ok;
					ok.push_back(src);
					return std::pair<listing, listing>(ok, listing());
				}
				constexpr auto operator()(unsigned long src) {
					listing ok, no;
					while (1 < src) {
						auto code = src % force_consteval<2 * preaxiomatic::Domain_values.size()>;
						src /= force_consteval<2 * preaxiomatic::Domain_values.size()>;
						if (preaxiomatic::Domain_values.size() > code) ok.push_back((preaxiomatic::Domain)code);
						else no.push_back((preaxiomatic::Domain)(code - preaxiomatic::Domain_values.size()));
					}
					return std::pair(ok, no);
				}
				constexpr auto operator()(domain* src) { return std::pair<listing, listing>(); }
			};

			return std::visit(interpret(), *this);
		}
	};

	// singleton
	class preaxiomatic_domain_of {
	private:
		std::vector<std::function<std::optional<preaxiomatic::Domain>(const formal::lex_node& src)> > _handlers_lex_node;
		std::vector<std::function<std::optional<preaxiomatic::Domain>(const formal::parsed& src)> > _handlers_parsed;
		std::vector<std::function<std::optional<preaxiomatic::Domain>(const formal::word& src)> > _handlers_word;

		preaxiomatic_domain_of() = default;
		preaxiomatic_domain_of(const preaxiomatic_domain_of&) = delete;
		preaxiomatic_domain_of(preaxiomatic_domain_of&&) = delete;
		preaxiomatic_domain_of& operator=(const preaxiomatic_domain_of&) = delete;
		preaxiomatic_domain_of& operator=(preaxiomatic_domain_of&&) = delete;
		~preaxiomatic_domain_of() = default;
	public:
		preaxiomatic_domain_of& get() {
			static preaxiomatic_domain_of ooao;

			return ooao;
		}

		// std::visit support
		std::optional<preaxiomatic::Domain> operator()(const std::unique_ptr<formal::lex_node>& src) {
			if (src) return operator()(*src);
			return std::nullopt;
		}
		std::optional<preaxiomatic::Domain> operator()(const std::unique_ptr<formal::parsed>& src) {
			if (src) return operator()(*src);
			return std::nullopt;
		}
		std::optional<preaxiomatic::Domain> operator()(const std::unique_ptr<formal::word>& src) {
			if (src) return operator()(*src);
			return std::nullopt;
		}

		std::optional<preaxiomatic::Domain> operator()(const formal::lex_node& src) {
			for (decltype(auto) h : _handlers_lex_node) {
				if (auto ret = h(src)) return ret;
			};
			return std::nullopt;
		}

		std::optional<preaxiomatic::Domain> operator()(const formal::parsed& src) {
			for (decltype(auto) h : _handlers_parsed) {
				if (auto ret = h(src)) return ret;
			};
			return std::nullopt;
		}

		std::optional<preaxiomatic::Domain> operator()(const formal::word& src) {
			for (decltype(auto) h : _handlers_word) {
				if (auto ret = h(src)) return ret;
			};
			return std::nullopt;
		}

		void register_handler(decltype(_handlers_lex_node)::value_type src) { _handlers_lex_node.push_back(std::move(src)); }
		void register_handler(decltype(_handlers_parsed)::value_type src) { _handlers_parsed.push_back(std::move(src)); }
		void register_handler(decltype(_handlers_word)::value_type src) { _handlers_word.push_back(std::move(src)); }

		bool force(const preaxiomatic::Domain target, formal::lex_node*& src) {
			if (auto test = operator()(*src)) { // already typed
				if (target == *test) return true;
			}
			return false;
		}
	};

	// deferred: uniqueness quantification (unclear what data representation should be)
	class var final : public formal::parsed {
		unsigned long long _quant_code;
		std::shared_ptr<const formal::lex_node> _var;
		std::shared_ptr<const domain> _domain;

		static constexpr const std::array<std::string_view, 3> to_s_aux = {
			std::string_view(),
			"&forall;",
			"&exist;"
		};
		static constexpr const auto quantifier_HTML_entities = substr(perl::shift<1>(to_s_aux).second, 1, -1);
		static constexpr const auto reserved_HTML_entities = perl::unshift(quantifier_HTML_entities, std::string_view("isin"));

	public:
		enum class quantifier {
			Term = 0,
			ForAll,
			ThereIs
		};

		var() = delete; // empty variable doesn't make much sense
		var(const var& src) = default;
		var(var&& src) = default;
		var& operator=(const var& src) = default;
		var& operator=(var&& src) = default;
		~var() = default;

		std::unique_ptr<parsed> clone() const override { return std::unique_ptr<parsed>(new var(*this)); }
		void CopyInto(parsed*& dest) const override { zaimoni::CopyInto(*this, dest); }
		void MoveInto(parsed*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }

		formal::src_location origin() const override { return _var->origin(); }

		std::string to_s() const override {
			std::string ret;
			if (force_consteval<std::end(to_s_aux) - std::begin(to_s_aux)> > _quant_code) { // invariant failure if this doesn't hold
				ret += to_s_aux[_quant_code];
			};
			ret += _var->to_s();
			ret += "&isin;";
			ret += _domain->to_s();
			return ret;
		}

		std::string name() const { return _var->to_s(); }

		static bool legal_varname(const formal::lex_node& src) {
			if (0 < src.prefix_size()) return false;  // no prefix in the parse, at all
			if (auto tag = HTMLtag::is_balanced_pair(src)) {
				if (*tag != "b" && *tag != "i") return false;
				if (1 != src.infix_size()) return false;
				decltype(auto) test = *src.infix(0);
				if (1 != test.is_pure_anchor()) return false;
				if (!legal_varname(*test.anchor<formal::word>())) return false;
				const auto post_size = src.postfix_size();
				if (1 < post_size) return false;
				if (0 == post_size) return true;
				if (decltype(auto) node = src.postfix(0)) return HTMLtag::is_balanced_pair(*node, "sub");
				return false;
			}
			// normal case: no italics, bold etc but subscripting generally ok
			if (0 < src.infix_size()) return false;
			if (0 < src.postfix_size()) return false;
			if (auto w = src.anchor<formal::word>()) {
				if (!legal_varname(*w)) return false;
			} else return false;
			if (auto node = src.post_anchor<formal::lex_node>()) return HTMLtag::is_balanced_pair(*node, "sub");
			if (src.post_anchor<formal::word>()) return false;
			if (src.post_anchor<formal::parsed>()) return false;
			return true;
		}

		static bool legal_varname(const formal::word& src) {
			if (src.code() & HTMLtag::Entity) return true; // \todo check for "alphabetic-ness" of the underlying UNICODE code point
			if (is_alphabetic(src.value())) return true; // first character only tested
			return false;
		}

		static unsigned int legal_quantifier(const formal::lex_node& src) {
			if (const auto w = interpret_HTML_entity(src)) return legal_quantifier(*w);

			// \todo? visually, it is true that a span that rotates A or E 180-degrees "works" and we could accept that as an alternate encoding
			// e.g, <span style="transform:rotate(180deg);display:inline-block">E</span>
			return 0;
		}

		static unsigned int legal_quantifier(const formal::word& src) {
			struct is_quantifier_entity {
				// std::ranges version false compile-errors (fake dangling iterator)
				unsigned int operator()(std::string_view src) {
					const auto origin = quantifier_HTML_entities.begin();
					const auto ub = quantifier_HTML_entities.end();
					// due to implementation of lookup_HTML_entity, identity lookup
					auto test = std::find(origin, ub, src);
					if (ub != test) return (test - origin) + 1;
					return 0;
				}
				unsigned int operator()(char32_t src) {
					const auto origin = quantifier_HTML_entities.begin();
					const auto ub = quantifier_HTML_entities.end();
					auto test = std::find_if(origin, ub, [&](const auto& x) { return lookup_HTML_entity(x)->second == src; });
					if (ub != test) return (test - origin) + 1;
					return 0;
				}
			};

			if (decltype(auto) test = interpret_HTML_entity(src.value())) {
				return std::visit(is_quantifier_entity(), *test);
			}
			return 0;
		}

		// our syntax is: [quantifier] varname &isin; domain
		static std::vector<size_t> reject_left_edge(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
			std::vector<size_t> ret;
			if (interpret_HTML_entity(*src[viewpoint], "isin")) {
				// no room for variable to left
				if (src[viewpoint]->code() & formal::Error) return ret;
				error_report(*src[viewpoint], "&isin; cannot match variable to its left");
			}
			return ret;
		}

		static std::vector<size_t> reject_right_edge(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
			std::vector<size_t> ret;
			if (src[viewpoint]->code() & formal::Error) return ret;

			if (decltype(auto) fail = interpret_HTML_entity(*src[viewpoint], reserved_HTML_entities)) {
				std::string err = wrap_to_HTML_entity(*fail);
				err += " cannot match to its right";
				error_report(*src[viewpoint], err);
			}

			return ret;
		}

		static std::vector<size_t> reject_adjacent(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
			std::vector<size_t> ret;

			if (2 > src.size()) return ret;
			if (1 > viewpoint) return ret;

			const auto origin = reserved_HTML_entities.begin();
			const auto ub = reserved_HTML_entities.end();

			auto leading = std::find_if(origin, ub, [&](const std::string_view& tag) { return interpret_HTML_entity(*src[viewpoint - 1], tag); });
			if (leading == ub) return ret;
			auto trailing = std::find_if(origin, ub, [&](const std::string_view& tag) { return interpret_HTML_entity(*src[viewpoint], tag); });
			if (trailing == ub) return ret;
			if ((src[viewpoint - 1]->code() & formal::Error) && (src[viewpoint]->code() & formal::Error)) return ret;

			std::string err = wrap_to_HTML_entity(*leading);
			err += ' ';
			err += wrap_to_HTML_entity(*trailing);
			err += " : cannot parse to variable declaration";
			error_report(*src[viewpoint-1], err);
			src[viewpoint]->learn(formal::Error);
			return ret;
		}

		static std::vector<size_t> parse(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
			// our syntax is: [quantifier] varname &isin; domain
			std::vector<size_t> ret;

			if (3 > src.size()) return ret;
			if (2 > viewpoint) return ret;
			if (!interpret_HTML_entity(*src[viewpoint - 1], "isin")) return ret;
			if (src[viewpoint - 1]->code() & formal::Error) return ret;
			if (!legal_varname(*src[viewpoint - 2])) return ret; // \todo handle more general well-formed expressions
			auto domain = preaxiomatic::parse(*src[viewpoint]); // \todo handle more general domains of discourse
			if (!domain) return ret;
			auto quant_code = 3 < src.size() ? legal_quantifier(*src[viewpoint - 3]) : 0;
			if (0 >= quant_code) {
				// a term variable.
				std::unique_ptr<var> stage(new var(src[viewpoint-2], preaxiomatic::get(*domain), quantifier::Term));
				std::unique_ptr<formal::lex_node> relay(new formal::lex_node(stage.release()));
				ret.push_back(viewpoint - 2);
				src.DeleteNSlotsAt(2, viewpoint - 1);
				delete src[viewpoint - 2];
				src[viewpoint - 2] = relay.release();
				return ret;
			}
			if ((decltype(quant_code))quantifier::ThereIs >= quant_code) {
				// a quantified variable.
				std::unique_ptr<var> stage(new var(src[viewpoint - 2], preaxiomatic::get(*domain), (quantifier)quant_code));
				std::unique_ptr<formal::lex_node> relay(new formal::lex_node(stage.release()));
				ret.push_back(viewpoint - 3);
				src.DeleteNSlotsAt(2, viewpoint - 1);
				src.DeleteIdx(viewpoint-3);
				delete src[viewpoint - 3];
				src[viewpoint - 3] = relay.release();
				return ret;
			}
			// \todo? more advanced quantifiers?
			return ret;
		}

		static std::optional<formal::is_wff::ret_parse_t> wff(formal::is_wff::subsequence src) {
			// We don't have a good idea of transitivity here : x &isin; y &isin; z should not parse.
			// (x &isin; y) &isin; <b>TruthValued</b> not only should parse, it should be an axiom built into the type system

			auto& [origin, target, ok] = src;

			constexpr auto isin_type = domain_param({ preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::Ur }, { preaxiomatic::Domain::TruthValues });
			constexpr auto domain_ok = domain_param({}, { preaxiomatic::Domain::Ur });
			constexpr auto element_ok = domain_param({ preaxiomatic::Domain::Set, preaxiomatic::Domain::Ur }, { preaxiomatic::Domain::Class });

			static_assert(domain_ok.accept(isin_type));
			static_assert(!*domain_ok.accept(isin_type));
			static_assert(element_ok.accept(isin_type));
			static_assert(*element_ok.accept(isin_type));

			// \todo need to be able to tell the testing function that we are:
			// * truth-valued
			// * not a truth value
			// * not a set (our notation is a set, but we ourselves are not)
			if (ok.has_value()) {
				if (auto test = std::any_cast<domain_param>(&ok)) {
					if (auto verify = test->accept(isin_type)) {
						if (!*verify) return std::nullopt;
					}
				}
			}

			ptrdiff_t anchor_at = -1;

			ptrdiff_t i = target.size();
			while (0 <= --i) {
				if (interpret_HTML_entity(*target[i], "isin")) {
					if (0 > anchor_at) anchor_at = i;
					// not an error: x &isin; y & w &isin; z should parse as a 2-ary conjunction
					else return std::nullopt;
				}
			};
			if (0 > anchor_at) return std::nullopt;
			const auto& anchor = *target[anchor_at];
			if (anchor.code() & formal::Error) {
				return formal::is_wff::ret_parse_t(std::pair(false, std::string()), nullptr);
			}

			if (0 == anchor_at) {
				// might want to hard error for "final" parsing
				size_t offset = origin;
				std::span<formal::lex_node*> dest = target.first(1);
				auto fail = [=]() {
					formal::is_wff::change_target ret(offset, dest);
					ret.second.front()->learn(formal::Error);
					return ret;
				};
				return formal::is_wff::ret_parse_t(std::pair(false, std::string("&isin; cannot match variable to its left")), fail);
			}
			if (target.size() - 1 == anchor_at) {
				size_t offset = origin + anchor_at;
				std::span<formal::lex_node*> dest = target.last(1);
				auto fail = [=]() {
					formal::is_wff::change_target ret(offset, dest);
					ret.second.front()->learn(formal::Error);
					return ret;
				};
				// might want to hard error for "final" parsing
				return formal::is_wff::ret_parse_t(std::pair(false, std::string("&isin; cannot match to its right")), fail);
			}
			// forward is the domain check
			// backwards is the variable check
			ptrdiff_t quantifier_at = anchor_at;
			unsigned int quant_code = 0;
			while (0 <= --quantifier_at) {
				quant_code = legal_quantifier(*target[quantifier_at]);
				if (0 < quant_code) break;
			};

			if (0 < quantifier_at) { // re-position
				size_t len = target.size() - quantifier_at;
				target = target.last(len);
				origin += quantifier_at;
				anchor_at -= quantifier_at;
				quantifier_at = 0;
			}

			std::optional<std::variant<domain*, preaxiomatic::Domain> > interpret_domain(nullptr);
			if (anchor_at + 2 == target.size()) {
				decltype(auto) domain_src = *target[anchor_at + 1];
				if (auto preax = preaxiomatic::parse(domain_src)) {
					interpret_domain = *preax;
				} else if (3 == domain_src.is_pure_anchor()) {
					if (auto test = dynamic_cast<domain*>(domain_src.anchor<formal::parsed>())) {
						interpret_domain = test;
					}
				}
			} /* else { // \todo ask gentzen::syntax_check
			} */
			if (!interpret_domain) return std::nullopt;

			formal::lex_node* elt = nullptr;
			if (2 == anchor_at - quantifier_at) {
				decltype(auto) element_src = *target[anchor_at - 1];
				if (legal_varname(element_src)) elt = &element_src;
				/* else {
				} */
			} /* else { // \todo ask gentzen::syntax_check
			} */

			if (!elt) return std::nullopt;

			struct decode_domain {
				std::shared_ptr<const domain> operator()(domain* src) {
					domain* stage = nullptr;
					src->MoveInto(stage);
					return std::shared_ptr<const domain>(src);
				}
				std::shared_ptr<const domain> operator()(preaxiomatic::Domain src) { return preaxiomatic::get(src); }
			};

			if (0 >= quant_code) {
				if (1 != anchor_at) throw new std::exception("tracing");
				// a term variable.
				auto rewrite = [=, d_src=*interpret_domain, origin=origin, target=target]() {
					std::unique_ptr<var> stage(new var(target[anchor_at - 1], std::visit(decode_domain(), d_src), quantifier::Term));
					std::unique_ptr<formal::lex_node> relay(new formal::lex_node(stage.release()));
					for (decltype(auto) x : target) {
						if (x) {
							delete x;
							x = nullptr;
						};
					}
					target.front() = relay.release();
					return formal::is_wff::change_target(origin, target);
				};
				return formal::is_wff::ret_parse_t(std::pair(true, std::string()), rewrite);
			}
			if ((decltype(quant_code))quantifier::ThereIs >= quant_code) {
				if (2 != anchor_at) throw new std::exception("tracing");
				// a quantified variable.
				auto rewrite = [=, d_src = *interpret_domain, origin = origin, target = target]() {
					std::unique_ptr<var> stage(new var(target[anchor_at - 1], std::visit(decode_domain(), d_src), (quantifier)quant_code));
					std::unique_ptr<formal::lex_node> relay(new formal::lex_node(stage.release()));
					for (decltype(auto) x : target) {
						if (x) {
							delete x;
							x = nullptr;
						};
					}
					target.front() = relay.release();
					return formal::is_wff::change_target(origin, target);
				};
				return formal::is_wff::ret_parse_t(std::pair(true, std::string()), rewrite);
			}

			return std::nullopt;
		}

private:
		var(formal::lex_node*& name, std::shared_ptr<const domain> domain, quantifier quant)
		: _quant_code((unsigned long long)quant), _var(name), _domain(domain) {
			name = nullptr;
		};
	};

	class var_ref final : public formal::parsed {
		std::shared_ptr<const var> _var;
		formal::src_location _origin;

	public:
		var_ref() = delete;
		var_ref(const var_ref& src) = default;
		var_ref(var_ref&& src) = default;
		var_ref& operator=(const var_ref& src) = default;
		var_ref& operator=(var_ref&& src) = default;
		~var_ref() = default;

		std::unique_ptr<parsed> clone() const override { return std::unique_ptr<parsed>(new var_ref(*this)); }
		void CopyInto(parsed*& dest) const override { zaimoni::CopyInto(*this, dest); }
		void MoveInto(parsed*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }

		formal::src_location origin() const override { return _origin; }
		std::string to_s() const override { return _var->name(); }
	};

	// \todo syntactical equivalence will be its own type, even though it's very similar
	class inference_rule {
		std::string _name;
		std::vector<std::shared_ptr<const var> > _vars;
		std::vector<std::shared_ptr<const formal::parsed> > _hypotheses;
		std::vector<std::shared_ptr<const formal::parsed> > _conclusions;

		using uniform_substitution_t = std::vector<std::pair<std::shared_ptr<const var>, std::weak_ptr<formal::parsed> > >;

		// fact, hypothesis/conclusion, var assignments
		using arg_match = std::tuple<std::weak_ptr<const formal::parsed>, std::shared_ptr<const formal::parsed>, std::vector<std::pair<std::shared_ptr<const formal::parsed>, std::weak_ptr<const formal::parsed> > > >;
		std::vector<arg_match> _rete_alpha_memory;

	public:
		inference_rule() = delete; // empty inference rule doesn't make much sense
		inference_rule(const inference_rule& src) = default;
		inference_rule(inference_rule&& src) = default;
		inference_rule& operator=(const inference_rule& src) = default;
		inference_rule& operator=(inference_rule&& src) = default;
		~inference_rule() = default;

		inference_rule(std::vector<std::shared_ptr<const formal::parsed> >&& hypotheses, std::vector<std::shared_ptr<const formal::parsed> >&& conclusions, std::string&& name = std::string())
			: _name(std::move(name)), _hypotheses(std::move(hypotheses)), _conclusions(std::move(conclusions)) {
		}
	};

	class proof {

	public:
		enum class rationale {
			Given = 0,
			Hypothesis
		};

		proof() = default;
		proof(const proof& src) = default;
		proof(proof&& src) = default;
		proof& operator=(const proof& src) = default;
		proof& operator=(proof&& src) = default;
		~proof() = default;


	};

} // end namespace gentzen

// end prototype class

enum LG_modes {
	LG_PP_like = 1,	// #-format; could be interpreted later for C preprocessor directives if we were so inclined
	LG_CPP_like, // //-format
	LG_Command,	// to the inference engine
	LG_MAX
};

static_assert(sizeof(unsigned long long)*CHAR_BIT >= LG_MAX);
static_assert(!(formal::Comment & (1ULL << LG_PP_like)));
static_assert(!(formal::Error & (1ULL << LG_PP_like)));
static_assert(!(formal::Comment & (1ULL << LG_CPP_like)));
static_assert(!(formal::Error & (1ULL << LG_CPP_like)));
static_assert(!(formal::Comment & (1ULL << LG_Command)));
static_assert(!(formal::Error & (1ULL << LG_Command)));

// note that we use # for set-theoretic cardinality, so this would not be correct at later stages
bool IsOneLineComment(formal::word*& x) {
	auto test = x->value();
	const auto o_size = test.size();
	ltrim(test);
	unsigned long long new_code = 0;
	if (test.starts_with('#')) {
		new_code = formal::Comment | (1ULL << LG_PP_like);
	} else if (test.starts_with("//")) {
		new_code = formal::Comment | (1ULL << LG_CPP_like);
	}
	if (new_code) {
		const auto n_size = test.size();
		if (o_size <= n_size) {
			x->interpret(new_code);
			return true;
		}
		std::unique_ptr<formal::word> stage(new formal::word(*x, o_size - n_size, test.size(), new_code));
		delete x;
		x = stage.release();
		return true;
	};
	return false;
}

namespace gentzen {

	enum class command {
		Axiom = 0,
		Given,
		Hypothesis
	};

	static constexpr const std::array<std::string_view, 3> command_text = {
		// these configure the formal system
		"Axiom",
		// these feed lines into the current proof
		"Given",
		"Hypothesis"
	};

	static constexpr const auto command_first = substr(command_text, 0, 1);

}

bool IsGentzenCommand(formal::word*& x) {
	auto test = x->value();
	const size_t o_size = test.size();
	ltrim(test);

	if (!std::ranges::any_of(gentzen::command_first, [&](auto src) {return test.starts_with(src); })) return false;

	if (const auto candidate = kleene_star(test, is_alphabetic)) {
		if (!std::ranges::any_of(gentzen::command_first, [&](auto src) {return test == candidate->first; })) return false;
		test.remove_prefix(candidate->first.size());
		ltrim(test);
		if (!test.starts_with(':')) return false;
		test.remove_prefix(1);
		ltrim(test);
		if (test.empty()) { // no command content of note
			delete x;
			x = nullptr;
			return true;
		}

		size_t delta = o_size - test.size();
		delta -= candidate->first.size() + 1;

		std::unique_ptr<std::string> stage(new std::string(candidate->first));
		*stage += ':';
		*stage += test;

		static_assert(noexcept(noexcept(x->origin() + delta))); // would need a temporary if this failed

		*x = formal::word(std::shared_ptr<const std::string>(stage.release()), x->origin() + delta, LG_Command);
		return true;
	}
	return false;
}

// first stage is a very simple line-finder (pre-preprocessor, shell script, ...)
static auto& LineGrammar() {
	static std::unique_ptr<kuroda::parser<formal::word> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());
		ooao->register_terminal(IsOneLineComment);
		ooao->register_terminal(IsGentzenCommand);
	};
	return *ooao;
}

template<bool C_Shell_Line_Continue=true>
static auto to_lines(std::istream& in, formal::src_location& origin)
{
	kuroda::parser<formal::word>::symbols ret;
	while (in.good()) {
		std::unique_ptr<std::string> next(new std::string());
		std::getline(in, *next);
		// if the target language uses the C/shell line continue character, we have to catch that here
		// use C++23 version
		std::optional<std::string_view> line_continue;
		if constexpr (C_Shell_Line_Continue) {
			if (!ret.empty()) {
				auto lc_probe = ret.back()->value();
				if (const auto idx = lc_probe.rfind('\\'); idx != std::string_view::npos) {
					bool ws_terminated = true;
					auto ws_test = lc_probe;
					ws_test.remove_prefix(idx + 1);
					ptrdiff_t ub = ws_test.size();
					while (0 <= --ub) {
						if (!isspace(static_cast<unsigned char>(ws_test[ub]))) {
							ws_terminated = false;
							break;
						}
					}
					if (ws_terminated) {
						line_continue = lc_probe;
						line_continue->remove_suffix(lc_probe.size() - idx);
					}
				}
			}
		}

		if (!next->empty()) {
			if constexpr (C_Shell_Line_Continue) {
				if (line_continue) {
					std::unique_ptr<std::string> stripped(new std::string(*line_continue));
					*stripped += *next;
					std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(stripped.release()), ret.back()->origin(), ret.back()->code()));
					delete ret.back();
					ret.back() = stage.release();
				} else
					LineGrammar().append_to_parse(ret, new formal::word(std::shared_ptr<const std::string>(next.release()), origin));
			} else
				LineGrammar().append_to_parse(ret, new formal::word(std::shared_ptr<const std::string>(next.release()), origin));
		} else if constexpr (C_Shell_Line_Continue) {
			if (line_continue) {
				std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(*line_continue)), ret.back()->origin(), ret.back()->code()));
				delete ret.back();
				ret.back() = stage.release();
			}
		};
		origin.line_pos.first++;
		origin.line_pos.second = 0;
	};
	return ret;
}

enum TG_modes {
	TG_HTML_tag =60,
	TG_MAX
};

static_assert(sizeof(unsigned long long)* CHAR_BIT >= TG_MAX);
static_assert(!(formal::Comment & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Error & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Inert_Token & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Tokenized & (1ULL << TG_HTML_tag)));

// action coding: offset to parse at next
static constexpr std::pair<std::string_view, int> reserved_atomic[] = {
	{"(", 1},
	{")", 0},
	{"{", 1},
	{"}", 0},
};

size_t issymbol(const std::string_view& src)
{
	if (isalnum(static_cast<unsigned char>(src[0]))) return 0;
	if (isspace(static_cast<unsigned char>(src[0]))) return 0;
	for (decltype(auto) x : reserved_atomic) {
		if (src.starts_with(x.first)) return 0;
	}
	return 1;
}

size_t HTML_EntityLike(const std::string_view& src)
{
	if (3 > src.size() || '&' != src[0]) return 0;

	auto working = src;
	working.remove_prefix(1);

	if (auto alphabetic_entity = kleene_star(working, is_alphabetic)) {
		working.remove_prefix(alphabetic_entity->first.size());
		if (working.empty() || ';' != working[0]) return 0;
		return 2 + alphabetic_entity->first.size();
	};

	if (3 > working.size() || '#' != working[0]) return 0;
	working.remove_prefix(1);

	if (auto decimal_entity = kleene_star(working, is_digit)) {
		working.remove_prefix(decimal_entity->first.size());
		if (working.empty() || ';' != working[0]) return 0;
		return 3 + decimal_entity->first.size();
	};
	if ('x' != working[0] || 2 > working.size()) return 0;
	working.remove_prefix(1);

	if (auto hexadecimal_entity = kleene_star(working, is_hex_digit)) {
		working.remove_prefix(hexadecimal_entity->first.size());
		if (working.empty() || ';' != working[0]) return 0;
		return 4 + hexadecimal_entity->first.size();
	};
	return 0;
}

// lexing+preprocessing stage
std::vector<size_t> tokenize(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint)
{
	std::vector<size_t> ret;

	const auto x = src[viewpoint];
	if (x->code() & (formal::Comment | formal::Tokenized | formal::Inert_Token)) return ret;	// do not try to lex comments, or already-tokenized
	if (1 != x->is_pure_anchor()) return ret;	// we only try to manipulate things that don't have internal syntax

	const auto w = x->anchor<formal::word>();
	auto text = w->value();

	auto text_size = text.size();
	// like most formal languages, we don't care about leading whitespace.
	ltrim(text);
	if (const auto new_size = text.size(); new_size < text_size) {
		if (0 == new_size) { // gone.
			src.DeleteIdx(viewpoint);
			ret.push_back(viewpoint);
			return ret;
		}
		*w = formal::word(std::string(text), w->origin() + (text_size - new_size), w->code());
		text = w->value();
		text_size = new_size;
	}

	for (decltype(auto) scan : reserved_atomic) {
		if (scan.first == text) {
			*w = formal::word(scan.first, w->origin(), w->code());	// force GC of std::shared_ptr
			x->learn(formal::Inert_Token);
			if (1 != scan.second) ret.push_back(viewpoint + scan.second);
			return ret;
		}
		if (text.starts_with(scan.first)) {
			auto remainder = text;
			remainder.remove_prefix(scan.first.size());
			ltrim(remainder);
			if (!remainder.empty()) {
				std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size())));
				std::unique_ptr<formal::lex_node> node(new formal::lex_node(std::move(stage)));
				src.insertNSlotsAt(1, viewpoint + 1);
				src[viewpoint + 1] = node.release();
			};
			*w = formal::word(scan.first, w->origin(), w->code());	// force GC of std::shared_ptr
			x->learn(formal::Inert_Token);
			if (1 != scan.second) ret.push_back(viewpoint + scan.second);
			return ret;
		}
	}

	if (auto html_entity = HTML_EntityLike(text)) {
		auto remainder = text;
		remainder.remove_prefix(html_entity);
		ltrim(remainder);
		if (!remainder.empty()) {
			std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size())));
			std::unique_ptr<formal::lex_node> node(new formal::lex_node(std::move(stage)));
			src.insertNSlotsAt(1, viewpoint + 1);
			src[viewpoint + 1] = node.release();
		};

		text.remove_suffix(text.size() - html_entity);
		*w = formal::word(std::shared_ptr<const std::string>(new std::string(text)), w->origin(), w->code());
		w->learn(HTMLtag::Entity);
		x->learn(HTMLtag::Entity | formal::Inert_Token);
		return ret;
	}

	if (auto tag = HTMLtag::parse(src, viewpoint)) {
		std::unique_ptr<formal::lex_node> stage(new formal::lex_node(tag.release(), TG_HTML_tag | formal::Tokenized));
		delete src[viewpoint];
		src[viewpoint] = stage.release();
		return ret;
	}

	// failover: alphanumeric blob
	if (auto test = kleene_star(text, is_alphanumeric)) {
		if (test->first.size() < text.size()) {
			auto remainder = text;
			remainder.remove_prefix(test->first.size());
			ltrim(remainder);
			if (!remainder.empty()) {
				std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size())));
				std::unique_ptr<formal::lex_node> node(new formal::lex_node(std::move(stage)));
				src.insertNSlotsAt(1, viewpoint + 1);
				src[viewpoint + 1] = node.release();
			};
			text.remove_suffix(text.size() - test->first.size());
			*w = formal::word(std::shared_ptr<const std::string>(new std::string(text)), w->origin());
			x->learn(formal::Tokenized);
			return ret;
		}
		x->learn(formal::Tokenized);
		return ret;
	}

	// failover: symbolic blob
	if (auto test = kleene_star(text, issymbol)) {
		if (test->first.size() < text.size()) {
			auto remainder = text;
			remainder.remove_prefix(test->first.size());
			ltrim(remainder);
			if (!remainder.empty()) {
				std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size())));
				std::unique_ptr<formal::lex_node> node(new formal::lex_node(std::move(stage)));
				src.insertNSlotsAt(1, viewpoint + 1);
				src[viewpoint + 1] = node.release();
			};
			text.remove_suffix(text.size() - test->first.size());
			*w = formal::word(std::shared_ptr<const std::string>(new std::string(text)), w->origin());
			x->learn(formal::Tokenized);
			return ret;
		}
		x->learn(formal::Tokenized);
		return ret;
	}

	// failover: if we didn't handle it, pretend it's tokenized so we don't re-scan it
	x->learn(formal::Tokenized);
	return ret;
}

auto balanced_atomic_handler(const std::string_view& l_token, const std::string_view& r_token)
{
	return [=](kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
		std::vector<size_t> ret;

		decltype(auto) closing = src[viewpoint];
		if (!(closing->code() & formal::Inert_Token)) return ret;	// our triggers are inert tokens
		if (closing->code() & formal::Error) return ret;	// do not try to process error tokens
		if (1 != closing->is_pure_anchor()) return ret;	// we only try to manipulate things that don't have internal syntax

		auto close_text = closing->anchor<formal::word>()->value();

		if (r_token != close_text) return ret;
		ptrdiff_t ub = viewpoint;
		while (0 <= --ub) {
			decltype(auto) opening = src[ub];
			if (!(opening->code() & formal::Inert_Token)) continue;	// our triggers are inert tokens
//			if (x->code() & formal::Error) return ret;	// do not try to process error tokens
			if (1 != opening->is_pure_anchor()) continue;	// we only try to manipulate things that don't have internal syntax

			auto open_text = opening->anchor<formal::word>()->value();
			if (r_token == open_text) { // oops, consecutive unmatched
				error_report(*closing, std::string("unmatched '") + std::string(r_token) + "'");
				return ret;
			}
			if (l_token == open_text) { // ok
				formal::lex_node::slice(src, ub, viewpoint);
				ret.push_back(ub);
				return ret;
			}
		}

		error_report(*closing, std::string("unmatched '") + std::string(r_token) + "'");
		return ret;
	};
}

auto balanced_html_tag(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
	static constexpr const char* empty_visual_no_op[] = {
		"span",
		"sub",
		"sup",
		"b",
		"i",
	};

	std::vector<size_t> ret;

	decltype(auto) closing = src[viewpoint];
	auto tag = dynamic_cast<HTMLtag*>(closing->anchor<formal::parsed>());
	if (!tag) return ret;
	if (HTMLtag::mode::closing != tag->tag_type()) return ret;

	ptrdiff_t ub = viewpoint;
	while (0 <= --ub) {
		decltype(auto) opening = src[ub];
		auto open_tag = dynamic_cast<HTMLtag*>(opening->anchor<formal::parsed>());
		if (!open_tag) continue;
		auto open_mode = open_tag->tag_type();
		if (HTMLtag::mode::self_closing == open_mode) continue;
		if (open_tag->tag_name() != tag->tag_name()) continue;
		if (HTMLtag::mode::opening == open_mode) {
			// we understand that the following graphical tags no-op without content
			for (decltype(auto) must_see : empty_visual_no_op) {
				if (tag->tag_name() == must_see && 1 == viewpoint - ub) {
					ret.push_back(ub);
					src.DeleteIdx(viewpoint);
					src.DeleteIdx(ub);
					return ret;
				}
			}
			// typically want to do this
			formal::lex_node::slice(src, ub, viewpoint);
			ret.push_back(ub);
			return ret;
		}
		// assume an unmatched closing tag of our type has itself failed to match
		return ret;
	}

	error_report(*closing, std::string("unmatched </") + tag->tag_name() + ">");
	return ret;
}

auto HTML_bind_to_preceding(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
	static constexpr const char* binds_to_predecessor[] = {
		"sub",
		"sup"
	};

#if PROTOTYPE
	static constexpr const char* visually_merges_with_predecessor[] = {
		"b",
		"i"
	};
#endif

	std::vector<size_t> ret;

	if (0 >= viewpoint) return ret;

//	decltype(auto) balanced = src[viewpoint]; // lambda capture of reference from [], only works once
	const auto tag = HTMLtag::is_balanced_pair(*src[viewpoint]);
	if (!tag) return ret;
	for (decltype(auto) x : binds_to_predecessor) {
		if (*tag == x) {
			static auto relink = [&](formal::lex_node* target) {
				// don't mess with the balanced () {} parsing
				if ((target->code() & formal::Inert_Token) && 1 == target->is_pure_anchor()) {
					const auto text = target->anchor<formal::word>()->value();
					for (decltype(auto) x : reserved_atomic) {
						if (x.first == text) return (formal::lex_node*)nullptr;
					}
				}

				decltype(auto) last_postfix = target->postfix(-1);
				if (!last_postfix) {
					if (target->set_null_post_anchor(src[viewpoint])) return target;
					// \todo? warn when auto-repairing this?
					if (HTMLtag::is_balanced_pair(*target, *tag)) return (formal::lex_node*)nullptr;
					target->push_back_postfix(src[viewpoint]);
					return target;
				}
				// \todo unclear how to proceed here (tensors?)
				return (formal::lex_node*)nullptr;
			};

			decltype(auto) bind_to = src[viewpoint-1];
			if (formal::lex_node::rewrite(bind_to, relink)) {
				ret.push_back(viewpoint - 1);
				src.DeleteIdx(viewpoint);
			}
			return ret;
		}
	}

#if PROTOTYPE
	for (decltype(auto) x : visually_merges_with_predecessor) {
		if (*tag == x) {
			const auto merge_with = src[viewpoint - 1];
			if (!HTMLtag::is_balanced_pair(*merge_with, *tag)) return ret;
			// \todo consider blocking merge for style attribute differences?
			// ...
		}
	}
#endif
	return ret;
}

static auto& TokenGrammar() {
	static std::unique_ptr<kuroda::parser<formal::lex_node> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());
		ooao->register_build_nonterminal(tokenize);
		ooao->register_build_nonterminal(balanced_atomic_handler(reserved_atomic[0].first, reserved_atomic[1].first));
		ooao->register_build_nonterminal(balanced_atomic_handler(reserved_atomic[2].first, reserved_atomic[3].first));
		ooao->register_build_nonterminal(balanced_html_tag);
		ooao->register_build_nonterminal(HTML_bind_to_preceding);

		// \todo need local test cases for these
		ooao->register_build_nonterminal(gentzen::var::reject_adjacent);
		ooao->register_build_nonterminal(gentzen::var::parse);
		ooao->register_left_edge_build_nonterminal(gentzen::var::reject_left_edge);
		ooao->register_right_edge_build_nonterminal(gentzen::var::reject_right_edge);
	};
	return *ooao;
}

// main language syntax
static auto& GentzenGrammar() {
	static std::unique_ptr<kuroda::parser<formal::lex_node> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());
	};
	return *ooao;
}

int main(int argc, char* argv[], char* envp[])
{
#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
	const bool to_console = _isatty(_fileno(stdout));
#else
	const bool to_console = isatty(fileno(stdout));
#endif
	decltype(auto) who_am_i = self_path(argv[0]);
//	std::wcout << who_am_i.native() << "\n";
//	std::wcout << std::filesystem::canonical(who_am_i.native()) << "\n";

#ifndef MOCK_TEST
	if (2 > argc) help();
#endif

	int idx = 0;
#ifdef MOCK_TEST
	while (++idx < 2) {
	formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(MOCK_TEST)));
#else
	while (++idx < argc) {
		if (process_option(argv[idx])) continue;
		formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(argv[idx])));
#endif
		std::wcout << src.path->native() << "\n";
		auto to_interpret = std::ifstream(*src.path);
		if (!to_interpret.is_open()) continue;
		auto lines = to_lines(to_interpret, src);

		kuroda::parser<formal::lex_node>::symbols stage;
		auto wrapped = formal::lex_node::pop_front(lines);
		while (wrapped) {
			TokenGrammar().append_to_parse(stage, wrapped.release());
			wrapped = formal::lex_node::pop_front(lines);
		};
		if (0 >= Errors.count()) TokenGrammar().finite_parse(stage);
		std::cout << std::to_string(stage.size()) << "\n";

		formal::lex_node::to_s(std::cout, stage) << "\n";
	}

//	if (!to_console) STRING_LITERAL_TO_STDOUT("<pre>\n");

//	STRING_LITERAL_TO_STDOUT("End testing\n");
//	if (!to_console) STRING_LITERAL_TO_STDOUT("</pre>\n");
	return Errors.count() ? EXIT_FAILURE : EXIT_SUCCESS;
};

#endif
