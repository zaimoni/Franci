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
void error_report(const formal::src_location& loc, const perl::scalar& err) {
	if (loc.path) std::wcerr << loc.path->native();
	else std::cerr << "<unknown>";
	std::cerr << loc.to_s() << ": error : " << err.view() << '\n';
	++Errors;
}

void warning_report(const formal::src_location& loc, const perl::scalar& warn) {
	if (loc.path) std::wcerr << loc.path->native();
	else std::cerr << "<unknown>";
	std::cerr << loc.to_s() << ": warning : " << warn.view() << '\n';
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
	{"forall", 0x2200UL},
	{"exist", 0x2203UL},
	{"empty", 0x2205UL},
	{"isin", 0x2208UL},
	{nullptr, 0x2209UL}, // := ~(... &isin; ...) when ... &isin; ... is well-formed
	{nullptr, 9500UL} // syntactically entails
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

constexpr auto lookup_HTML_entity(const std::variant<std::string_view, char32_t>& name)
{
	if (auto text = std::get_if<std::string_view>(&name)) return lookup_HTML_entity(*text);
	return lookup_HTML_entity(std::get<char32_t>(name));
}


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
		if (auto transcode = lookup_HTML_entity(std::get<char32_t>(*test))) return transcode->first && ref == transcode->first;
	}
	return false;
}

bool interpret_HTML_entity(std::string_view src, char32_t ref) {
	if (const auto test = interpret_HTML_entity(src)) {
		if (auto x = std::get_if<char32_t>(&(*test))) return ref == *x;
		if (auto transcode = lookup_HTML_entity(std::get<std::string_view>(*test))) return ref == transcode->second;
	}
	return false;
}

const formal::word* is_parsed_HTML_entity(const formal::word* x)
{
	if (!x || !(x->code() & HTMLtag::Entity)) return nullptr;
	return x;
}

const formal::word* is_parsed_HTML_entity(const formal::lex_node& src)
{
	if (1 != src.is_pure_anchor()) return nullptr;
	if (auto test = is_parsed_HTML_entity(src.c_anchor<formal::word>())) return test;
	return nullptr;
}

bool is_parsed_HTML_entity(const formal::lex_node& src, const std::string_view& ref)
{
	if (const auto w = is_parsed_HTML_entity(src)) return interpret_HTML_entity(w->value(), ref);
	return false;
}

bool is_parsed_HTML_entity(const formal::lex_node& src, char32_t ref)
{
	if (const auto w = is_parsed_HTML_entity(src)) return interpret_HTML_entity(w->value(), ref);
	return false;
}

template<size_t n>
const std::string_view* is_parsed_HTML_entity(const formal::lex_node& src, const std::array<std::string_view, n>& ref)
{
	if (const auto w = is_parsed_HTML_entity(src)) {
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

static std::optional<std::string_view> interpret_inert_word(const formal::lex_node& src)
{
	if (1 != src.is_pure_anchor()) return std::nullopt;
	const auto w = src.c_anchor<formal::word>();
	if (w->code() & formal::Inert_Token) return w->value();
	return std::nullopt;
}

static constexpr const std::string_view comma(",");

// action coding: offset to parse at next
static constexpr std::pair<std::string_view, int> reserved_atomic[] = {
	{"(", 1},
	{")", 0},
	{"{", 1},
	{"}", 0},
	{"[", 1},
	{"]", 0},
	{",", 1},
};

bool detect_comma(const formal::lex_node& src) {
	if (const auto x = src.c_anchor<formal::word>()) return comma == x->value();
	return false;
}

size_t issymbol(const std::string_view& src)
{
	if (isalnum(static_cast<unsigned char>(src[0]))) return 0;
	if (isspace(static_cast<unsigned char>(src[0]))) return 0;
	for (decltype(auto) x : reserved_atomic) {
		if (src.starts_with(x.first)) return 0;
	}
	return 1;
}

// prototype class -- extract to own files when stable

namespace formal {

	// many programming languages have the notion of a token, i.e. formal word, that is a syntax error by construction
	class is_wff { // is well-formed formula
	public:
		// offset, change target, constraints (usually some sort of std::function using language-specific types)
		using subsequence = std::tuple<size_t, std::span<formal::lex_node*>, std::vector<std::any> >;
		using change_target = std::pair<size_t, std::span<formal::lex_node*> >;
		using parse_t = std::function<change_target()>;

		// true, "": no comment, ok
		// true, std::string: warning
		// false, std:: string: error
		// false, "": no comment, error
		using wff_t = std::pair<bool, std::pair<formal::src_location, std::string> >;
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

		static auto no_msg() {
			static const auto ooao = std::pair(formal::src_location(), std::string());
			return ooao;
		}

		static wff_t no_op() {
			static const wff_t ooao = wff_t(false, no_msg());
			return ooao;
		}

		static wff_t proceed() {
			static const wff_t ooao = wff_t(true, no_msg());
			return ooao;
		}

		static ret_parse_t no_parse() {
			static const ret_parse_t ooao = ret_parse_t(no_op(), nullptr);
			return ooao;
		}

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
			return no_op();
		}

		wff_t operator()(const formal::parsed& src) {
			for (decltype(auto) h : _well_formed_parsed) {
				if (auto ret = h(src)) return *ret;
			}
			return no_op();
		}

		wff_t operator()(const formal::word& src) {
			for (decltype(auto) h : _well_formed_word) {
				if (auto ret = h(src)) return *ret;
			}
			return no_op();
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
			return ret_parse_t(no_op(), nullptr);
		}
	};

} // end namespace formal

static kuroda::parser<formal::lex_node>& GentzenGrammar();

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

		static constexpr std::optional<Domain> get(const domain* src) {
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

		unsigned int precedence() const override { return 0; }

		static std::optional<Domain> parse(const formal::lex_node& src) {
			if (!HTMLtag::is_balanced_pair(src, "b")) return std::nullopt;
			if (!src.postfix().empty()) return std::nullopt; // \todo extend to handle abstract algebraic category theory
			if (1 != src.infix().size()) return std::nullopt;

			decltype(auto) node = *src.infix().front();
			if (1 != node.is_pure_anchor()) return std::nullopt;

			const auto text = node.c_anchor<formal::word>()->value();
			ptrdiff_t i = -1;
			for (decltype(auto) x : parsing) {
				++i;
				if (text == x) return (Domain)(i);
			}

			return std::nullopt;
		}

		static bool parse_lex(formal::lex_node*& src) {
			if (auto stage = parse(*src)) {
				auto relay = std::make_unique<formal::lex_node>(get(*stage), formal::Inert_Token);
				delete src;
				src = relay.release();
				return true;
			}
			return false;
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

	class domain_param final : public std::variant<preaxiomatic::Domain, unsigned long, const domain*> {
		using base = std::variant<preaxiomatic::Domain, unsigned long, const domain*>;
		using listing = zaimoni::stack<preaxiomatic::Domain, preaxiomatic::Domain_values.size()>;

	public:
		constexpr domain_param(preaxiomatic::Domain src) noexcept : base{src} {}
		constexpr domain_param(const domain* src) : base{src} {
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
				constexpr auto operator()(const domain* src) { return std::pair<listing, listing>(); }
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

	// should only need one of these per formal language
	// handles common issues with infix/prefix/postfix reserved keywords
	class argument_enforcer final
	{
		static constexpr const unsigned int require_left = (1U << 0);
		static constexpr const unsigned int require_right = (1U << 1);
		static_assert(!(require_left & require_right));

		// these always need arguments to either side, in some sense
		std::vector<std::pair<std::string_view, unsigned int> > _reserved_HTML_named_entities;
		std::vector<std::pair<char32_t, unsigned int> > _reserved_HTML_numeric_entities;
		std::vector<std::pair<std::string_view, unsigned int> > _reserved_tokens;

		// these don't reliably need arguments, but do stop parsing of arguments
		std::vector<std::pair<std::string_view, unsigned int> > _stop_parse_HTML_named_entities;
		std::vector<std::pair<char32_t, unsigned int> > _stop_parse_HTML_numeric_entities;
		std::vector<std::pair<std::string_view, unsigned int> > _stop_parse_tokens;

		argument_enforcer() = default;
	public:
		argument_enforcer(const argument_enforcer&) = delete;
		argument_enforcer(argument_enforcer&&) = delete;
		argument_enforcer& operator=(const argument_enforcer&) = delete;
		argument_enforcer& operator=(argument_enforcer&&) = delete;
		~argument_enforcer() = default;

		static argument_enforcer& get() {
			static argument_enforcer ooao;
			return ooao;
		}

		void reserve_HTML_entity(bool want_left, const std::string_view& src, bool want_right) {
			for (decltype(auto) x : _reserved_HTML_named_entities) if (src == x.first) continue;
			if (auto test = lookup_HTML_entity(src)) {
				ptrdiff_t i = _reserved_HTML_numeric_entities.size();
				while (0 <= --i) {
					if (test->second == _reserved_HTML_numeric_entities[i].first) {
						_reserved_HTML_numeric_entities.erase(_reserved_HTML_numeric_entities.begin() + i);
						break;
					}
				}
			}
			_reserved_HTML_named_entities.push_back(std::pair(src, (want_left ? require_left : 0U) | (want_right ? require_right : 0U)));
		}

		void reserve_HTML_entity(bool want_left, char32_t src, bool want_right) {
			for (decltype(auto) x : _reserved_HTML_numeric_entities) if (src == x.first) continue;
			if (auto test = lookup_HTML_entity(src)) {
				ptrdiff_t i = _reserved_HTML_named_entities.size();
				while (0 <= --i) {
					if (test->first == _reserved_HTML_named_entities[i].first) return;
				}
			}
			_reserved_HTML_numeric_entities.push_back(std::pair(src, (want_left ? require_left : 0U) | (want_right ? require_right : 0U)));
		}

		void reserve_token(bool want_left, const std::string_view& src, bool want_right) {
			for (decltype(auto) x : _reserved_tokens) if (src == x.first) continue;
			_reserved_tokens.push_back(std::pair(src, (want_left ? require_left : 0U) | (want_right ? require_right : 0U)));
		}

		void stop_parse_HTML_entity(bool want_left, const std::string_view& src, bool want_right) {
			for (decltype(auto) x : _stop_parse_HTML_named_entities) if (src == x.first) continue;
			if (auto test = lookup_HTML_entity(src)) {
				ptrdiff_t i = _stop_parse_HTML_numeric_entities.size();
				while (0 <= --i) {
					if (test->second == _stop_parse_HTML_numeric_entities[i].first) {
						_stop_parse_HTML_numeric_entities.erase(_stop_parse_HTML_numeric_entities.begin() + i);
						break;
					}
				}
			}
			_stop_parse_HTML_named_entities.push_back(std::pair(src, (want_left ? require_left : 0U) | (want_right ? require_right : 0U)));
		}

		void stop_parse_HTML_entity(bool want_left, char32_t src, bool want_right) {
			for (decltype(auto) x : _stop_parse_HTML_numeric_entities) if (src == x.first) continue;
			if (auto test = lookup_HTML_entity(src)) {
				ptrdiff_t i = _stop_parse_HTML_named_entities.size();
				while (0 <= --i) {
					if (test->first == _stop_parse_HTML_named_entities[i].first) return;
				}
			}
			_stop_parse_HTML_numeric_entities.push_back(std::pair(src, (want_left ? require_left : 0U) | (want_right ? require_right : 0U)));
		}

		void stop_parse_token(bool want_left, const std::string_view& src, bool want_right) {
			for (decltype(auto) x : _stop_parse_tokens) if (src == x.first) continue;
			_stop_parse_tokens.push_back(std::pair(src, (want_left ? require_left : 0U) | (want_right ? require_right : 0U)));
		}

		static bool rejectLeftEdge(formal::lex_node& src) {
			if (src.code() & formal::Error) return true;

			if (auto reserved = interpret_reserved(src)) {
				if (_reject_left_edge(reserved->first)) {
					std::string err(reserved->second);
					err += "cannot match to its left";
					error_report(src, err);
				}
				return true;
			}
			return false;
		}

		static std::vector<size_t> reject_left_edge(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
			std::vector<size_t> ret;
			if (src[viewpoint]->code() & formal::Error) return ret;

			if (auto reserved = interpret_reserved(*src[viewpoint])) {
				if (_reject_left_edge(reserved->first)) {
					std::string err(reserved->second);
					err += "cannot match to its left";
					error_report(*src[viewpoint], err);
				}
				return ret;
			}
			return ret;
		}

		static std::vector<size_t> reject_right_edge(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
			std::vector<size_t> ret;
			if (src[viewpoint]->code() & formal::Error) return ret;

			if (auto reserved = interpret_reserved(*src[viewpoint])) {
				if (_reject_right_edge(reserved->first)) {
					std::string err(reserved->second);
					err += "cannot match to its right";
					error_report(*src[viewpoint], err);
				}
				return ret;
			}

			return ret;
		}

		static bool rejectRightEdge(formal::lex_node& src) {
			if (src.code() & formal::Error) return true;

			if (auto reserved = interpret_reserved(src)) {
				if (_reject_right_edge(reserved->first)) {
					std::string err(reserved->second);
					err += "cannot match to its right";
					error_report(src, err);
				}
				return true;
			}
			return false;
		}

		static std::vector<size_t> reject_adjacent(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
			std::vector<size_t> ret;

			if (1 > viewpoint) return ret;
			if ((src[viewpoint]->code() & formal::Error) && (src[viewpoint - 1]->code() & formal::Error)) return ret;

			auto l_reserved = interpret_reserved(*src[viewpoint - 1]);
			if (!l_reserved) return ret;
			auto r_reserved = interpret_reserved(*src[viewpoint]);
			if (!r_reserved) return ret;

			const bool rejectLeftEdge = _reject_left_edge(r_reserved->first);

			if (   (_reject_right_edge(l_reserved->first) && (rejectLeftEdge || _no_parse_through_left_edge(r_reserved->first)))
				|| (rejectLeftEdge && _no_parse_through_right_edge(l_reserved->first))) {
				std::string err(l_reserved->second);
				err += ' ';
				err += r_reserved->second;
				err += " : ";
				err += "cannot parse";
				error_report(*src[viewpoint - 1], err);
				src[viewpoint]->learn(formal::Error);
			}
			return ret;
		}

	private:
		static std::optional<std::pair<std::variant<std::string_view, std::variant<std::string_view, char32_t> >, std::string_view> > interpret_reserved(const formal::lex_node& src) {
			if (auto inert = interpret_inert_word(src)) return std::pair(*inert, *inert);
			if (auto node = is_parsed_HTML_entity(src)) {
				if (auto tag = interpret_HTML_entity(node->value())) return std::pair(*tag, node->value());
			}
			return std::nullopt;
		}

		static bool _reject_left_edge(const std::string_view& tag) {
			const auto& ooao = get();
			for (decltype(auto) x : ooao._reserved_tokens) {
				if ((x.second & require_left) && x.first == tag) return true;
			};
			return false;
		}

		static bool _reject_left_edge(const std::variant<std::string_view, char32_t>& tag) {
			const auto& ooao = get();
			if (auto entity = lookup_HTML_entity(tag)) {
				if (entity->first) {
					for (decltype(auto) x : ooao._reserved_HTML_named_entities) {
						if ((x.second & require_left) && x.first == entity->first) return true;
					};
					return false;
				}
				for (decltype(auto) x : ooao._reserved_HTML_numeric_entities) {
					if ((x.second & require_left) && x.first == entity->second) return true;
				};
				return false;
			}
			return false;
		}

		static bool _reject_left_edge(const std::variant<std::string_view, std::variant<std::string_view, char32_t> >& src) {
			if (auto inert = std::get_if<0>(&src)) return _reject_left_edge(*inert);
			return _reject_left_edge(std::get<1>(src));
		}

		static bool _reject_right_edge(const std::variant<std::string_view, char32_t>& tag) {
			const auto& ooao = get();
			if (auto entity = lookup_HTML_entity(tag)) {
				if (entity->first) {
					for (decltype(auto) x : ooao._reserved_HTML_named_entities) {
						if ((x.second & require_right) && x.first == entity->first) return true;
					};
					return false;
				}
				for (decltype(auto) x : ooao._reserved_HTML_numeric_entities) {
					if ((x.second & require_right) && x.first == entity->second) return true;
				};
				return false;
			}
			return false;
		}

		static bool _reject_right_edge(const std::string_view& tag) {
			const auto& ooao = get();
			for (decltype(auto) x : ooao._reserved_tokens) {
				if ((x.second & require_right) && x.first == tag) return true;
			};
			return false;
		}

		static bool _reject_right_edge(const std::variant<std::string_view, std::variant<std::string_view, char32_t> >& src) {
			if (auto inert = std::get_if<0>(&src)) return _reject_right_edge(*inert);
			return _reject_right_edge(std::get<1>(src));
		}

		static bool _no_parse_through_left_edge(const std::string_view& tag) {
			const auto& ooao = get();
			for (decltype(auto) x : ooao._stop_parse_tokens) {
				if ((x.second & require_left) && x.first == tag) return true;
			};
			return false;
		}

		static bool _no_parse_through_left_edge(const std::variant<std::string_view, char32_t>& tag) {
			const auto& ooao = get();
			if (auto entity = lookup_HTML_entity(tag)) {
				if (entity->first) {
					for (decltype(auto) x : ooao._stop_parse_HTML_named_entities) {
						if ((x.second & require_left) && x.first == entity->first) return true;
					};
					return false;
				}
				for (decltype(auto) x : ooao._stop_parse_HTML_numeric_entities) {
					if ((x.second & require_left) && x.first == entity->second) return true;
				};
				return false;
			}
			return false;
		}

		static bool _no_parse_through_left_edge(const std::variant<std::string_view, std::variant<std::string_view, char32_t> >& src) {
			if (auto inert = std::get_if<0>(&src)) return _no_parse_through_left_edge(*inert);
			return _no_parse_through_left_edge(std::get<1>(src));
		}

		static bool _no_parse_through_right_edge(const std::variant<std::string_view, char32_t>& tag) {
			const auto& ooao = get();
			if (auto entity = lookup_HTML_entity(tag)) {
				if (entity->first) {
					for (decltype(auto) x : ooao._stop_parse_HTML_named_entities) {
						if ((x.second & require_right) && x.first == entity->first) return true;
					};
					return false;
				}
				for (decltype(auto) x : ooao._stop_parse_HTML_numeric_entities) {
					if ((x.second & require_right) && x.first == entity->second) return true;
				};
				return false;
			}
			return false;
		}

		static bool _no_parse_through_right_edge(const std::string_view& tag) {
			const auto& ooao = get();
			for (decltype(auto) x : ooao._stop_parse_tokens) {
				if ((x.second & require_right) && x.first == tag) return true;
			};
			return false;
		}

		static bool _no_parse_through_right_edge(const std::variant<std::string_view, std::variant<std::string_view, char32_t> >& src) {
			if (auto inert = std::get_if<0>(&src)) return _no_parse_through_right_edge(*inert);
			return _no_parse_through_right_edge(std::get<1>(src));
		}
	};

	// local refinement -- more API that we need, but a generic formal language doesn't
	class var;

	struct Gentzen : public formal::parsed {
		virtual domain_param element_of() const = 0;
		virtual domain_param syntax() const = 0;

		// not really const; just mostly used during construction
		[[nodiscard]] virtual bool normalize_vars(std::vector<std::shared_ptr<const var> >& catalog) const = 0;
	};

	// deferred: uniqueness quantification (unclear what data representation should be)
	class var final : public Gentzen {
	public:
		enum class quantifier {
			Term = 0,
			ForAll,
			ThereIs
		};

	private:
		unsigned long long _quant_code;
		std::shared_ptr<const formal::lex_node> _var;
		std::shared_ptr<const domain> _domain;

		static constexpr const unsigned long long LexQuantifier = (1ULL << 2); // reserve this flag for both word and lex_node

		static_assert(!(formal::Comment & LexQuantifier));
		static_assert(!(formal::Error & LexQuantifier));
		static_assert(!(formal::Inert_Token & LexQuantifier));
		static_assert(!(formal::Tokenized & LexQuantifier));
		static_assert(!(HTMLtag::Entity & LexQuantifier));

		static constexpr auto my_syntax = domain_param({ preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::Ur }, { preaxiomatic::Domain::TruthValues });

		static constexpr const std::array<std::string_view, 3> to_s_aux = {
			std::string_view(),
			"&forall;",
			"&exist;"
		};
		static constexpr const auto quantifier_HTML_entities = substr(perl::shift<1>(to_s_aux).second, 1, -1);
		static constexpr const auto reserved_HTML_entities = perl::unshift(quantifier_HTML_entities, std::string_view("isin"));

		var(quantifier code, std::shared_ptr<const formal::lex_node> name, std::shared_ptr<const domain> domain)
			: _quant_code((decltype(_quant_code))code), _var(name), _domain(domain) {}
	public:
		using cache_t = std::vector<std::shared_ptr<const var> >;
		using live_caches_t = std::vector<cache_t*>;

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

		unsigned int precedence() const override { return 0; }

		std::string name() const { return _var->to_s(); }
		domain_param element_of() const override { return domain_param(_domain.get()); }
		domain_param syntax() const override { return my_syntax; }
		bool normalize_vars(std::vector<std::shared_ptr<const var> >& catalog) const override { return false; }

		quantifier quantified() const { return (quantifier)_quant_code; }

		static bool legal_varname(const formal::lex_node& src) {
			if (!src.prefix().empty()) return false;  // no prefix in the parse, at all
			if (auto tag = HTMLtag::is_balanced_pair(src)) {
				if (*tag != "b" && *tag != "i") return false;
				if (1 != src.infix().size()) return false;
				decltype(auto) test = *src.infix().front();
				if (1 != test.is_pure_anchor()) return false;
				if (!legal_varname(*test.c_anchor<formal::word>())) return false;
				const auto post_size = src.postfix().size();
				if (1 < post_size) return false;
				if (0 == post_size) return true;
				return HTMLtag::is_balanced_pair(*src.postfix().front(), "sub");
			}
			// normal case: no italics, bold etc but subscripting generally ok
			if (!src.infix().empty()) return false;
			if (!src.postfix().empty()) return false;
			if (auto w = src.c_anchor<formal::word>()) {
				if (!legal_varname(*w)) return false;
			} else return false;
			if (auto node = src.c_post_anchor<formal::lex_node>()) return HTMLtag::is_balanced_pair(*node, "sub");
			if (src.c_post_anchor<formal::word>()) return false;
			if (src.c_post_anchor<formal::parsed>()) return false;
			return true;
		}

		static bool legal_varname(const formal::word& src) {
			if (src.code() & HTMLtag::Entity) return true; // \todo check for "alphabetic-ness" of the underlying UNICODE code point
			if (is_alphabetic(src.value())) return true; // first character only tested
			return false;
		}

		static unsigned int legal_quantifier(const formal::lex_node& src) {
			if (const auto w = is_parsed_HTML_entity(src)) return legal_quantifier(*w);

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
		static void init(argument_enforcer& dest) {
			for (decltype(auto) x : quantifier_HTML_entities) dest.reserve_HTML_entity(false, x, true);
			dest.reserve_HTML_entity(true, "isin", true);
		};

		static std::optional<formal::is_wff::ret_parse_t> wff(formal::is_wff::subsequence src) {
			// We don't have a good idea of transitivity here : x &isin; y &isin; z should not parse.
			// (x &isin; y) &isin; <b>TruthValued</b> not only should parse, it should be an axiom built into the type system

			auto& [origin, target, demand] = src;

			constexpr auto domain_ok = domain_param({}, { preaxiomatic::Domain::Ur });
			constexpr auto element_ok = domain_param({ preaxiomatic::Domain::Set, preaxiomatic::Domain::Ur }, { preaxiomatic::Domain::Class });

			static_assert(domain_ok.accept(my_syntax));
			static_assert(!*domain_ok.accept(my_syntax));
			static_assert(element_ok.accept(my_syntax));
			static_assert(*element_ok.accept(my_syntax));

			// need to be able to tell the testing function that we are:
			// * truth-valued
			// * not a truth value
			// * not a set (our notation is a set, but we ourselves are not)
			for (decltype(auto) ok : demand) {
				if (!ok.has_value()) continue;
				if (auto test = std::any_cast<domain_param>(&ok)) {
					if (auto verify = test->accept(my_syntax)) {
						if (!*verify) return std::nullopt;
					}
				}
			}

			ptrdiff_t anchor_at = -1;

			ptrdiff_t i = target.size();
			while (0 <= --i) {
				if (is_parsed_HTML_entity(*target[i], "isin")) {
					if (0 > anchor_at) anchor_at = i;
					// not an error: x &isin; y & w &isin; z should parse as a 2-ary conjunction
					else return std::nullopt;
				}
			};
			if (0 > anchor_at) return std::nullopt;
			const auto& anchor = *target[anchor_at];
			if (anchor.code() & formal::Error) {
				return formal::is_wff::no_parse();
			}

			if (0 == anchor_at) {
				// might want to hard error for "final" parsing
				auto fail = [offset=origin, dest = target.first(1)]() {
					formal::is_wff::change_target ret(offset, dest);
					error_report(*ret.second.front(), "&isin; cannot match variable to its left");
					ret.second.front()->learn(formal::Error);
					return ret;
				};
				return formal::is_wff::ret_parse_t(formal::is_wff::no_op(), fail);
			}
			if (target.size() - 1 == anchor_at) {
				auto fail = [offset = origin + anchor_at, dest = target.last(1)]() {
					formal::is_wff::change_target ret(offset, dest);
					ret.second.front()->learn(formal::Error);
					return ret;
				};
				// might want to hard error for "final" parsing
				return formal::is_wff::ret_parse_t(std::pair(false, std::pair(anchor.origin(), std::string("&isin; cannot match to its right"))), fail);
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

			std::optional<std::variant<domain*, preaxiomatic::Domain> > interpret_domain;
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
				return formal::is_wff::ret_parse_t(formal::is_wff::proceed(), rewrite);
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
				return formal::is_wff::ret_parse_t(formal::is_wff::proceed(), rewrite);
			}

			return std::nullopt;
		}

		static std::vector<size_t> quantifier_bind_global(kuroda::parser<formal::lex_node>::sequence& tokens, size_t n) {
			std::vector<size_t> ret;

			ptrdiff_t scan = -1;
			while (++scan < tokens.size()-1) {
				const auto q_code = legal_quantifier(*tokens[scan]);
				if (!q_code) continue;
				if (legal_varname(*tokens[scan + 1])) {
					if (tokens[scan]->set_null_post_anchor(tokens[scan + 1])) {
						tokens[scan]->learn(formal::Inert_Token | LexQuantifier);
						ret.push_back(scan);
						tokens.DeleteIdx(scan + 1);
					}
				}
			}

			return ret;
		}

		static bool global_parse(kuroda::parser<formal::lex_node>::edit_span& tokens) {
			enum { trace_parse = 0 };

			const auto starting_errors = Errors.count();

			auto args = formal::lex_node::split(tokens, [](const formal::lex_node& x) {
				return is_parsed_HTML_entity(x, "isin");
				});
			if (args.empty()) return false;

			decltype(auto) origin = *args.front().src; // backward compatibility
			const auto offset = formal::lex_node::where_is(tokens);

			if (2 < args.size()) {
				for (decltype(auto) fail : args) {
					if (auto loc = formal::lex_node::where_is(fail)) {
						formal::lex_node& err = *origin[loc - 1];
						if (!(err.code() & formal::Error)) error_report(err, "non-associative ambiguous parse: &isin;");
					}
				}
				return false;
			}

			decltype(auto) anchor = args[0].empty() ? origin : (&(args[0].back()) + 1);
			std::shared_ptr<const domain> have_domain;
			bool have_var = false;
			bool have_quantifier = false;
			unsigned int quantifier_code = 0;

			auto lhs = args[0].to_span();
			if (args[0].empty()) {
				if (!((*anchor)->code() & formal::Error)) error_report(**anchor, "&isin; cannot match to its left");
			} else {
				have_quantifier = (lhs.back()->code() & var::LexQuantifier);
				if (have_quantifier) {
					quantifier_code = legal_quantifier(*lhs.back()->c_anchor<formal::word>());
				} else {
					have_var = legal_varname(*lhs.back());
				}
			};
			auto rhs = args[1].to_span();
			if (args[0].empty()) {
				if (!((*anchor)->code() & formal::Error)) error_report(**anchor, "&isin; cannot match to its right");
			} else {
				// \todo would like to "see through" grouping parentheses, but not ordered tuples
				have_domain = rhs.front()->shared_anchor<domain>();
			}

			if constexpr (trace_parse) {
				std::cout << "var::global_parse: " << have_var << " " << have_quantifier << " " << quantifier_code << " " << (bool)have_domain << "\n";
			}

			if (starting_errors < Errors.count()) return false;

			if (have_domain && 1 == rhs.size() && 1 == lhs.size()) {
				if (have_var) {
					const auto rescan = offset + lhs.size() - 1;
					if constexpr (trace_parse) {
						std::cout << "var::global_parse: " << rescan << " " << offset << " " << lhs.size() << " " << rhs.size() << " " << tokens.src->size() << "\n";
					}
					auto relay = std::unique_ptr<const var>(new var(quantifier::Term, std::shared_ptr<const formal::lex_node>(lhs.back()), have_domain));
					lhs.back() = nullptr;
					auto relay2 = std::make_unique<formal::lex_node>(relay.release(), formal::RequestNormalization);
					decltype(auto) dest = (*tokens.src)[rescan];
					delete dest;
					dest = relay2.release();
					kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, 2, rescan + 1);
					if constexpr (trace_parse) {
						std::cout << "var::global_parse: term variable ok\n";
					}
					return true;
				} else if (0 < quantifier_code) {
					const auto rescan = offset + lhs.size() - 1;
					auto relay = std::unique_ptr<const var>(new var((quantifier)quantifier_code, std::shared_ptr<const formal::lex_node>(lhs.back()->release_post_anchor<formal::lex_node>()), have_domain));
					auto relay2 = std::make_unique<formal::lex_node>(relay.release(), formal::RequestNormalization);
					decltype(auto) dest = (*tokens.src)[rescan];
					delete dest;
					dest = relay2.release();
					kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, 2, rescan + 1);
					return true;
				}
			}

			return false;
		}

		static std::shared_ptr<const var> WantsToMakeVarRefs(formal::lex_node* x) {
			if (x->code() & formal::RequestNormalization) return x->shared_anchor<var>();
			return std::shared_ptr<const var>();
		}

		bool is_compatible(std::shared_ptr<const domain>& domain) const {
			if (_domain.get() == domain.get()) return true;
			// disjoint: hard-reject
			// _domain subset domain: ok, but replace domain with ourselves (stricter than requested)
			// * but: our one caller doesn't actually use domain
			// domain subset _domain: this is a constraint forcing: do not handle this here (would be invalid to constraint-force
			// outside of a a subproof, for instance
			// proper intersection: constraint forcing, see above
			return false;
		}

		static std::optional<std::shared_ptr<const var> > lookup(const formal::lex_node& src, const live_caches_t& catalog) {
			const std::string find_me = src.to_s();
			for (decltype(auto) ref : catalog) {
				for (decltype(auto) old_var : *ref) {
					if (find_me == old_var->name()) return old_var;
				}
			}
			return std::nullopt;
		}

		static std::unique_ptr<var> improvise(formal::lex_node*& name, std::shared_ptr<const domain> domain, quantifier quant) {
			if (!var::legal_varname(*name)) return nullptr;
			return std::unique_ptr<var>(new var(name, domain, quant));
		}

private:
		var(formal::lex_node*& name, std::shared_ptr<const domain> domain, quantifier quant)
		: _quant_code((unsigned long long)quant), _var(name), _domain(domain) {
			name = nullptr;
		};
	};

	class var_ref final : public Gentzen {
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

		unsigned int precedence() const override { return 0; }

		domain_param element_of() const override { return _var->element_of(); }
		domain_param syntax() const override { return _var->element_of(); }
		bool normalize_vars(std::vector<std::shared_ptr<const var> >& catalog) const override {
			const std::string my_name = _var->name();
			const domain_param my_domain = _var->element_of();
			var::quantifier my_q = _var->quantified();

			for (decltype(auto) x : catalog) {
				if (x.get() == _var.get()) return false;
				const std::string my_name = x->name();
				if (my_name == x->name()) {
					// lexical match but not same
					// as we improvise to term quantification, it is ok to update that to the catalog version
					if (var::quantifier::Term != my_q && x->quantified() != my_q) {
						std::string err("parse ambiguity: ");
						err += my_name;
						err += " has inconsistent quantifications";
						throw std::pair(_origin, err);
					}

					const domain_param x_domain = _var->element_of();
					if (my_domain == x_domain) {
						const_cast<std::shared_ptr<const var>&>(_var) = x;
						return false;
					}

					// \todo if our domain *contains* the reference domain, update as above
					// \todo if our domain *is contained* in the reference domain, update the catalog (and return true)
					// \todo if our domain *is disjoint*, hard error
					// \todo otherwise, take intersection of domains and update both us and the catalog (and return true)

					std::string err("don't know how to reconcile domains ");
					err += my_name;
					err += " and ";
					err += x->name();
					throw std::pair(_origin, err);
				}
			}

			// not in catalog.
			catalog.push_back(_var);
			return false;
		}

		static bool make(std::shared_ptr<const var>& src, formal::lex_node* x) {
			enum { trace_parse = 0 };

			if (auto test = x->c_anchor<formal::parsed>()) {
				if (dynamic_cast<const var*>(test)) return false;
				if (dynamic_cast<const var_ref*>(test)) return false;
			}

			auto rewrite_ok = [my_name=src->name()](const formal::lex_node& target) {
				if (auto test = target.c_anchor<formal::parsed>()) {
					if (dynamic_cast<const var*>(test)) return false;
					if (dynamic_cast<const var_ref*>(test)) return false;
				}
				if (!var::legal_varname(target)) return false;
				if constexpr (trace_parse) {
					std::cout << "checking for var_ref\n";
					std::cout << my_name << "\n";
					std::cout << target.to_s() << "\n";
				}
				return my_name == target.to_s();
			};

			auto exec_rewrite = [&src](formal::lex_node& target) {
				auto stage = std::unique_ptr<var_ref>(new var_ref(src, target.origin()));
				target = std::shared_ptr<const formal::parsed>(stage.release());

				if constexpr (trace_parse) {
					std::cout << "var_ref created\n";
				}
			};

			return x->recursive_rewrite(rewrite_ok, exec_rewrite);
		}

		static std::unique_ptr<var_ref> improvise(formal::lex_node*& name, std::shared_ptr<const domain> domain, const var::live_caches_t& catalog) {
			const auto origin = name->origin();
			if (decltype(auto) test = var::lookup(*name, catalog)) {
				// \todo: check for compatible domain
				if ((*test)->is_compatible(domain)) return std::unique_ptr<var_ref>(new var_ref(std::move(*test), origin));
			}

			decltype(auto) stage = var::improvise(name, domain, var::quantifier::Term);
			if (!stage) return nullptr;

			// catalog.back() needs to be unchanged if a memory allocation fails
			std::shared_ptr<const var> new_var(stage.release());
			std::unique_ptr<var_ref> ret(new var_ref(new_var, origin));
			catalog.back()->push_back(std::move(new_var));
			return ret;
		}

	private:
		var_ref(const std::shared_ptr<const var>& src, const formal::src_location& origin) noexcept : _var(src), _origin(origin) {}
		var_ref(std::shared_ptr<const var>&& src, const formal::src_location& origin) noexcept : _var(std::move(src)), _origin(origin) {}
	};

	// This type is related to the Malinkowski entailments as well
	// \todo syntactical equivalence will be its own type, even though it's very similar
	class inference_rule final : public Gentzen {
		std::string _name;
		var::cache_t _vars;
		std::vector<std::shared_ptr<const Gentzen> > _hypotheses;
		std::vector<std::shared_ptr<const Gentzen> > _conclusions;

		std::vector<kuroda::parser<formal::lex_node>::symbols> _lexical_hypotheses;
		std::vector<kuroda::parser<formal::lex_node>::symbols> _lexical_conclusions;

		// key: hypotheses/conclusions
		// values: statements (when sub-proof is destucted, its statements will go null)
		using pattern_t = std::vector<std::pair<std::shared_ptr<const Gentzen>, std::weak_ptr<Gentzen> > >;

		// key: placeholder variables
		// values: statements (when sub-proof is destucted, its statements will go null)
		using uniform_substitution_t = std::vector<std::pair<std::shared_ptr<const var>, std::weak_ptr<Gentzen> > >;

		// hypothesis pattern match, conclusion pattern match, uniform substitution used
		using arg_match = std::tuple<pattern_t, pattern_t, uniform_substitution_t >;
		std::vector<arg_match> _rete_alpha_memory;

		static constexpr auto my_syntax = domain_param({ preaxiomatic::Domain::TruthValued, preaxiomatic::Domain::Ur }, { preaxiomatic::Domain::TruthValues });
		static constexpr auto arg_spec = domain_param({ preaxiomatic::Domain::TruthValued }, { preaxiomatic::Domain::TruthValues });
		static constexpr const char32_t reserved_HTML_entities[] = { 9500UL };

		inference_rule(decltype(_lexical_hypotheses)&& hypotheses, decltype(_lexical_conclusions)&& conclusions)
			: _lexical_hypotheses(std::move(hypotheses)), _lexical_conclusions(std::move(conclusions)) {
		}

		inference_rule(decltype(_hypotheses)&& hypotheses, decltype(_conclusions)&& conclusions, decltype(_vars)&& vars)
			: _vars(std::move(vars)), _hypotheses(std::move(hypotheses)), _conclusions(std::move(conclusions)) {
		}

	public:
		inference_rule() = delete; // empty inference rule doesn't make much sense
		inference_rule(const inference_rule& src) = default;
		inference_rule(inference_rule&& src) = default;
		inference_rule& operator=(const inference_rule& src) = default;
		inference_rule& operator=(inference_rule&& src) = default;
		~inference_rule() = default;

		inference_rule(decltype(_hypotheses)&& hypotheses, decltype(_conclusions)&& conclusions, std::string&& name = std::string())
			: _name(std::move(name)), _hypotheses(std::move(hypotheses)), _conclusions(std::move(conclusions)) {
retry:
			bool want_retry = false;
			for (decltype(auto) p : _hypotheses) {
				want_retry = p->normalize_vars(_vars);
			}

			const size_t n = _vars.size();
			for (decltype(auto) q : _conclusions) {
				want_retry = q->normalize_vars(_vars);
			}

			if (0 < n && n < _vars.size()) {
				// let axioms through, but otherwise require relevance per Belnap
				std::string err("inference rule '");
				err += _name;
				err += "' lacks relevance: ";
				err += std::to_string(n);
				err += " variables in hypothes(es), and ";
				err += std::to_string(_vars.size() - n);
				err += " more variables in conclusion(s) ";
				throw std::pair(formal::src_location(), err);
			}
			if (want_retry) goto retry;
		}

		std::unique_ptr<parsed> clone() const override { return std::unique_ptr<parsed>(new inference_rule(*this)); }
		void CopyInto(parsed*& dest) const override { zaimoni::CopyInto(*this, dest); }
		void MoveInto(parsed*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
		formal::src_location origin() const override {
			if (!_hypotheses.empty()) return _hypotheses.front()->origin();
			if (!_conclusions.empty()) return _conclusions.front()->origin(); // \todo fix
			return formal::src_location();
		}

		std::string to_s() const override {
			std::string ret;
			enum { trace_to_s = 0 };

			std::vector<std::string> stage;
			if (!_hypotheses.empty()) {
				if constexpr (trace_to_s) std::cout << "hypotheses: " << _hypotheses.size() << "\n";
				ret += join(to_s(_hypotheses), ", ");
			}
			if (!_lexical_hypotheses.empty()) {
				if constexpr (trace_to_s) {
					std::cout << "lexical hypotheses: " << _lexical_hypotheses.size() << "\n";
					for (decltype(auto) text : _lexical_hypotheses) {
						overview(text, "lexical hypothesis");
					}
				}
				if (!_hypotheses.empty()) ret += ", ";
				ret += join(to_s(_lexical_hypotheses), ", ");
			}
			ret += ' ';
			ret += "&#9500;";
			if (!_conclusions.empty()) {
				if constexpr (trace_to_s) std::cout << "conclusions: " << _conclusions.size() << "\n";
				ret += ' ';
				ret += join(to_s(_conclusions), ", ");
			}
			if (!_lexical_conclusions.empty()) {
				if constexpr (trace_to_s) {
					std::cout << "lexical conclusions: " << _lexical_conclusions.size() << "\n";
					for (decltype(auto) text : _lexical_conclusions) {
						overview(text, "lexical conclusion");
					}
				}
				ret += (_conclusions.empty()) ? " " : ", ";
				ret += join(to_s(_lexical_conclusions), ", ");
			}

			return ret;
		}

		constexpr unsigned int precedence() const override { return (unsigned int)(-1); }

		static bool requires_parentheses(const parsed* src) {
			return src && (unsigned int)(-1) <= src->precedence();
		}

		domain_param element_of() const override { return my_syntax; }
		domain_param syntax() const override { return my_syntax; }

		// not really const; just mostly used during construction
		[[nodiscard]] bool normalize_vars(std::vector<std::shared_ptr<const var> >& catalog) const override {
			// \todo implement
			return false;
		}

/*
		&#9500; is an expression-stopper but doesn't actually need arguments on either side (it needs them on *one* side, but
		either would do.
		',' does require arguments on both sides, but is a plain character
*/
		static void init(argument_enforcer& dest) {
			dest.reserve_token(true, ",", true);
			// UNICODE thinks this is a box-drawing glyph;
			// looks like what Standard Encyclopedia of Philosophy uses (the UNICODE blessed entities are for provability,
			// not syntactical entailment)
			dest.stop_parse_HTML_entity(true, 9500UL, true);
		};

		// arguably should be elsewhere
		static bool interpret_substatement(formal::lex_node::edit_span& view)
		{
			enum { trace_parse = 0 };

			if constexpr (trace_parse) {
				std::cout << "inference_rule::interpret_substatement: " << view.size() << "\n";
			}

			switch (view.size()) {
			case 0: return false;
			case 1: {
				decltype(auto) dest = view.front();
redo_outer_parens:
				if (dest->has_outer_parentheses()) {
					decltype(auto) parsing = static_cast<kuroda::parser<formal::lex_node>::sequence&>(const_cast<kuroda::parser<formal::lex_node>::symbols&>(dest->infix()));
					switch (parsing.size()) {
					case 0: return false;
					case 1: {
						formal::lex_node* test = parsing.front();
						parsing.front() = nullptr;
						delete dest; // invalidates variable parsing
						dest = test;
						goto redo_outer_parens;
					}
					default: { // in this context, an ordered tuple would be a syntax error
						auto stage = kuroda::parser<formal::lex_node>::to_editspan(parsing);
						if (GentzenGrammar().finite_parse(stage)) return true;
					}
					}
				}
			}
				  break;
			default:
				if constexpr (trace_parse) {
					std::cout << "inference_rule::interpret_substatement: attemptng GentzenGrammar().finite_parse(view)\n";
				}
				if (GentzenGrammar().finite_parse(view)) {
					if constexpr (trace_parse) {
						std::cout << "inference_rule::interpret_substatement: GentzenGrammar().finite_parse(view) successful\n";
					}
					return true;
				}
				if constexpr (trace_parse) {
					std::cout << "inference_rule::interpret_substatement: GentzenGrammar().finite_parse(view) ok\n";
				}
			}
			return false;
		}

		static bool global_parse(kuroda::parser<formal::lex_node>::edit_span& tokens) {
			enum { trace_parse = 0, test_conditions = 0 };

			const auto starting_errors = Errors.count();

			auto args = formal::lex_node::split(tokens, [](const formal::lex_node& x) {
				return is_parsed_HTML_entity(x, 9500UL);
				});
			if constexpr (trace_parse) {
				std::cout << "inference_rule::global_parse: args.size(): " << args.size() << "\n";
			}

			if (args.empty()) return false;

			decltype(auto) origin = *tokens.src; // backward compatibility
			const auto offset = formal::lex_node::where_is(tokens);

			if (2 < args.size()) {
				for (decltype(auto) fail : args) {
					if (const auto loc = formal::lex_node::where_is(fail)) {
						formal::lex_node& err = *origin[loc - 1];
						if (!(err.code() & formal::Error)) error_report(err, "non-associative ambiguous parse: &#9500;");
					}
				}
				return false;
			}

			auto weak_hypothesis_like = formal::lex_node::split(args[0], detect_comma);
			if (weak_hypothesis_like.empty()) weak_hypothesis_like.push_back(args[0]);
			else {
				for (decltype(auto) x : weak_hypothesis_like) {
					if (x.empty()) {
						if (0 != x.offset) {
							formal::lex_node& err = **(x.begin() - 1);
							if (!(err.code() & formal::Error)) error_report(err, ", delimits missing argument for &#9500;");
						}
					}
				}
			}

			auto weak_conclusion_like = formal::lex_node::split(args[1], detect_comma);
			if (weak_conclusion_like.empty()) weak_conclusion_like.push_back(args[1]);
			else {
				for (decltype(auto) x : weak_conclusion_like) {
					if (x.empty()) {
						if (0 != x.offset) {
							formal::lex_node& err = **(x.begin() - 1);
							if (!(err.code() & formal::Error)) error_report(err, ", delimits missing argument for &#9500;");
						}
					}
				}
			}

			if (starting_errors < Errors.count()) return false;

			if constexpr (trace_parse) {
				std::cout << "inference_rule::global_parse: " << weak_hypothesis_like.size() << " " << weak_conclusion_like.size() << "\n";
			}

			var::cache_t local_vars;
			ptrdiff_t cumulative_delta = 0;
			for (decltype(auto) view : weak_hypothesis_like) {
				view.offset += cumulative_delta;
full_restart_hypothesis:
				const auto old_size = view.size();
restart_hypothesis:
				if (interpret_substatement(view)) {
					const ptrdiff_t delta = view.size() - old_size;
					tokens.extent += delta;
					cumulative_delta += delta;
					if constexpr (test_conditions) {
						if (auto err = view.bad_syntax()) {
							std::cerr << std::string("kuroda::parser<...>::finite_parse: post-interpret_substatement view: ") + std::move(*err) << "\n";
							throw std::logic_error(to_string(std::stacktrace::current()));
						}
						if (auto err = tokens.bad_syntax()) {
							std::cerr << std::string("kuroda::parser<...>::finite_parse: post-interpret_substatement tokens: ") + std::move(*err) << "\n";
							throw std::logic_error(to_string(std::stacktrace::current()));
						}
					}
					if constexpr (trace_parse) {
						std::cout << "inference_rule::global_parse: hypothesis substatement:" << view.size() << " " << old_size << "\n";
					}

					if (1 == view.size()) {
						if (auto test = var::WantsToMakeVarRefs(view[0])) {
							local_vars.push_back(test);
							for (decltype(auto) x : *tokens.src) {
								var_ref::make(test, x);
							}
							view[0]->forget(formal::RequestNormalization);
						}
					}

					if constexpr (trace_parse) {
						std::cout << "inference_rule::global_parse: interpret_substatement ok; local_vars.size(): " << local_vars.size() << "\n";
					}

					if (0 == delta) goto restart_hypothesis;
					goto full_restart_hypothesis;
				}
			}
			for (decltype(auto) view : weak_conclusion_like) {
				view.offset += cumulative_delta;
full_restart_conclusion:
				const auto old_size = view.size();
restart_conclusion:
				if (interpret_substatement(view)) {
					const ptrdiff_t delta = view.size() - old_size;
					tokens.extent += delta;
					cumulative_delta += delta;
					if constexpr (test_conditions) {
						if (auto err = view.bad_syntax()) {
							std::cerr << std::string("kuroda::parser<...>::finite_parse: post-interpret_substatement view: ") + std::move(*err) << "\n";
							throw std::logic_error(to_string(std::stacktrace::current()));
						}
						if (auto err = tokens.bad_syntax()) {
							std::cerr << std::string("kuroda::parser<...>::finite_parse: post-interpret_substatement tokens: ") + std::move(*err) << "\n";
							throw std::logic_error(to_string(std::stacktrace::current()));
						}
					}
					if constexpr (trace_parse) {
						std::cout << "inference_rule::global_parse: conclusion substatement:" << view.size() << " " << old_size << "\n";
					}

					if (1 == view.size()) {
						if (auto test = var::WantsToMakeVarRefs(view[0])) local_vars.push_back(test);
					}

					if constexpr (trace_parse) {
						std::cout << "inference_rule::global_parse: interpret_substatement ok; local_vars.size(): " << local_vars.size() << "\n";
					}

					if (0 == delta) goto restart_conclusion;
					goto full_restart_conclusion;
				}
			}

			decltype(_hypotheses) gentzen_hypotheses;
			decltype(_conclusions) gentzen_conclusions;

			while (!weak_hypothesis_like.empty()) {
				if (1 != weak_hypothesis_like.front().size()) break;
				decltype(auto) gentzen_test = dynamic_cast<const Gentzen*>(weak_hypothesis_like.front().front()->c_anchor<formal::parsed>());
				if (!gentzen_test) break;
				gentzen_hypotheses.push_back(weak_hypothesis_like.front().front()->shared_anchor<Gentzen>());
				weak_hypothesis_like.erase(weak_hypothesis_like.begin());
			}

			while (!weak_conclusion_like.empty()) {
				if (1 != weak_conclusion_like.front().size()) break;
				decltype(auto) gentzen_test = dynamic_cast<const Gentzen*>(weak_conclusion_like.front().front()->c_anchor<formal::parsed>());
				if (!gentzen_test) break;
				gentzen_conclusions.push_back(weak_conclusion_like.front().front()->shared_anchor<Gentzen>());
				weak_conclusion_like.erase(weak_conclusion_like.begin());
			}

			if (weak_hypothesis_like.empty() && weak_conclusion_like.empty()) {
				const bool no_args = gentzen_hypotheses.empty() && gentzen_conclusions.empty();
				auto stage = std::unique_ptr<const inference_rule>(new inference_rule(std::move(gentzen_hypotheses), std::move(gentzen_conclusions), std::move(local_vars)));

				auto stage2 = std::make_unique<formal::lex_node>(stage.release());
				decltype(auto) dest = tokens.front();
				delete dest;
				dest = stage2.release();
				if (no_args) error_report(*dest, "no-argument &#9500;");
				if constexpr (trace_parse) {
					std::cout << "inference_rule::global_parse: Gentzen exiting: " << tokens.extent << " " << dest->to_s() << "\n";
				}

				kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, tokens.size() - 1, 1);
				return true;
			}

			decltype(_lexical_hypotheses) hypothesis_like = formal::lex_node::move_per_spec(weak_hypothesis_like);
			decltype(_lexical_conclusions) conclusion_like = formal::lex_node::move_per_spec(weak_conclusion_like);

			const bool no_args = hypothesis_like.empty() && conclusion_like.empty();
			if constexpr (trace_parse) {
				std::cout << "inference_rule::global_parse: no_args: " << no_args << "\n";
			}

			auto stage = std::unique_ptr<inference_rule>(new inference_rule(std::move(hypothesis_like), std::move(conclusion_like)));
			while (stage->Gentzen_parse_args());

			auto stage2 = std::make_unique<formal::lex_node>(stage.release());
			decltype(auto) dest = tokens.front();
			delete dest;
			dest = stage2.release();
			if (no_args) error_report(*dest, "no-argument &#9500;");
			if constexpr (trace_parse) {
				std::cout << "inference_rule::global_parse: exiting: " << tokens.extent << " " << dest->to_s() << "\n";
			}

			kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, tokens.size() - 1, 1);
			if constexpr (trace_parse) {
				std::cout << "inference_rule::global_parse: exiting\n";
			}
			return true;
		}

	private:
		static void overview(const formal::lex_node& src, const std::string& label) {
			if (!src.prefix().empty()) overview(src.prefix(), label + " prefix");
			if (auto test = src.c_anchor<parsed>()) std::cout << "parsed anchor " << label << ": " << test->to_s() << "\n";
			if (auto test = src.c_anchor<formal::lex_node>()) {
				std::cout << "lex_node anchor: " << test->to_s() << "\n";
				overview(*test, label + " anchor");
			}
			if (auto test = src.c_anchor<formal::word>()) {
				std::cout << "word anchor: " << test->value() << "\n";
			}
			if (!src.infix().empty()) overview(src.infix(), label + " infix");
			if (auto test = src.c_post_anchor<parsed>()) std::cout << "parsed post-anchor " << label << ": " << test->to_s() << "\n";
			if (auto test = src.c_post_anchor<formal::lex_node>()) {
				std::cout << "lex_node post-anchor: " << test->to_s() << "\n";
				overview(*test, label + " post-anchor");
			}
			if (auto test = src.c_post_anchor<formal::word>()) {
				std::cout << "word post-anchor: " << test->value() << "\n";
			}
			if (!src.postfix().empty()) overview(src.postfix(), label + " postfix");
		}

		static void overview(const kuroda::parser<formal::lex_node>::sequence& src, const std::string& label) {
			std::cout << label << ": " << src.size() << "\n";
			for (decltype(auto) x : src) overview(*x, label);
		}

		static bool remove_outer_parentheses(formal::lex_node*& x) {
			if (x->is_balanced_pair("(", ")")) {
				if (x->prefix().empty() && x->postfix().empty() && 1 == x->infix().size()) {
					auto stage = x->infix()[0];
					x->infix()[0] = nullptr;
					delete x;
					x = stage;
					return true;
				}
			}
			return false;
		}

		bool Gentzen_parse_args() {
			bool updated = false;
			if (!_lexical_hypotheses.empty()) {
				for (decltype(auto) phrase : _lexical_hypotheses) {
					if (GentzenGrammar().finite_parse(phrase)) updated = true;
				}
				do {
					if (1 < _lexical_hypotheses.front().size()) break;
					if (!_lexical_hypotheses.front().front()->is_pure_anchor()) break;
					if (auto test = static_cast<const Gentzen*>(_lexical_hypotheses.front().front()->c_anchor<formal::parsed>())) {
						if (arg_spec.accept(test->syntax())) {
							auto stage = test->clone();
							std::shared_ptr<const Gentzen> stage2(static_cast<Gentzen*>(stage.release()));
							_hypotheses.push_back(stage2);
							_lexical_hypotheses.erase(_lexical_hypotheses.begin());
							continue;
						}
					}
					break;
				} while(!_lexical_hypotheses.empty());
			}
			if (!_lexical_conclusions.empty()) {
				for (decltype(auto) phrase : _lexical_conclusions) {
					if (GentzenGrammar().finite_parse(phrase)) updated = true;
				}
				do {
					if (1 < _lexical_conclusions.front().size()) break;
					if (!_lexical_conclusions.front().front()->is_pure_anchor()) break;
					if (auto test = static_cast<const Gentzen*>(_lexical_conclusions.front().front()->c_anchor<formal::parsed>())) {
						if (arg_spec.accept(test->syntax())) {
							auto stage = test->clone();
							std::shared_ptr<const Gentzen> stage2(static_cast<Gentzen*>(stage.release()));
							_conclusions.push_back(stage2);
							_lexical_conclusions.erase(_lexical_conclusions.begin());
							continue;
						}
					}
					break;
				} while (!_lexical_conclusions.empty());
			}
			return updated;
		}

		static std::string join(std::vector<std::string>&& src, const std::string_view& glue) {
			std::string ret;
			while (!src.empty()) {
				ret += std::move(src.front());
				if (1 == src.size()) {
					std::remove_reference_t<decltype(src)>().swap(src);
				} else {
					src.erase(src.begin());
					ret += glue;
				}
			}
			return ret;
		}

		static std::vector<std::string> to_s(const std::vector<std::shared_ptr<const Gentzen> >& src)
		{
			std::vector<std::string> ret;
			ret.reserve(src.size());
			for (decltype(auto) x : src) {
				if (0 == x.use_count()) {
					ret.push_back(std::string());
					continue;
				}
				if (requires_parentheses(x.get())) {
					std::string stage("(");
					stage += x->to_s();
					ret.push_back(stage+")");
				} else {
					ret.push_back(x->to_s());
				}
			}

			return ret;
		}

		static std::vector<std::string> to_s(const kuroda::parser<formal::lex_node>::symbols& src)
		{
			std::vector<std::string> ret;
			ret.reserve(src.size());
			for (decltype(auto) x : src) {
				if (requires_parentheses(x->c_anchor<parsed>())) {
					std::string stage("(");
					stage += x->to_s();
					ret.push_back(stage + ")");
				} else {
					ret.push_back(x->to_s());
				}
			}

			return ret;
		}

		static std::vector<std::string> to_s(const std::vector<kuroda::parser<formal::lex_node>::symbols>& src)
		{
			std::vector<std::string> ret;
			ret.reserve(src.size());

			for (decltype(auto) lexical : src) {
				std::string stage;
				auto tokens = to_s(lexical);
				if (!tokens.empty()) {
					stage = std::move(tokens.front());
					tokens.erase(tokens.begin());
					while (!tokens.empty()) {
						stage += ' ';
						stage += tokens.front();
						tokens.erase(tokens.begin());
					}
				}
				ret.push_back(stage);
			}
			return ret;
		}
	};

	class proof {
		enum class rationale {
			Given = 0,
			Hypothesis,
			Inference
		};

		var::cache_t _vars; // global working variable catalog
		std::vector<std::shared_ptr<proof> > _testing;  // sub-proofs that Franci has not yet committed to using

		// want to allow non-uniform simultaneious substitutions when linking up inference rule
		using line = std::pair<std::shared_ptr<Gentzen>, std::tuple<rationale, std::shared_ptr<inference_rule> /* , std::unique_ptr<inference_rule::pattern_t> */ > >;

		std::vector<std::pair<line, std::shared_ptr<proof> > > _proof; // proof is for implication introduction, syntactical entailment introduction, etc.

	public:
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
	const auto _origin = in.tellg();
	in.seekg(0, std::ios_base::end);
	const auto _final = in.tellg();
	in.seekg(_origin);

	while (in.good() && _final != in.tellg()) {
		std::unique_ptr<std::string> next(new std::string());
		std::getline(in, *next);

		if (next->empty()) {
			next.reset(); // try to trigger crash-out here if RAM corruption
			origin.line_pos.first++;
			origin.line_pos.second = 0;
			continue;
		}
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
	TG_HTML_tag = 59,
	TG_MAX
};

static_assert(sizeof(unsigned long long)* CHAR_BIT >= TG_MAX);
static_assert(!(formal::Comment & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Error & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Inert_Token & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Tokenized & (1ULL << TG_HTML_tag)));
static_assert(!(formal::RequestNormalization & (1ULL << TG_HTML_tag)));

std::optional<std::string_view> HTML_EntityLike(const std::string_view& src)
{
	if (3 > src.size() || '&' != src[0]) return std::nullopt;

	auto working = src;
	working.remove_prefix(1);

	if (auto alphabetic_entity = kleene_star(working, is_alphabetic)) {
		working.remove_prefix(alphabetic_entity->first.size());
		if (working.empty() || ';' != working[0]) return std::nullopt;
		return src.substr(0, 2 + alphabetic_entity->first.size());
	};

	if (3 > working.size() || '#' != working[0]) return std::nullopt;
	working.remove_prefix(1);

	if (auto decimal_entity = kleene_star(working, is_digit)) {
		working.remove_prefix(decimal_entity->first.size());
		if (working.empty() || ';' != working[0]) return std::nullopt;
		return src.substr(0, 3 + decimal_entity->first.size());
	};
	if ('x' != working[0] || 2 > working.size()) return std::nullopt;
	working.remove_prefix(1);

	if (auto hexadecimal_entity = kleene_star(working, is_hex_digit)) {
		working.remove_prefix(hexadecimal_entity->first.size());
		if (working.empty() || ';' != working[0]) return std::nullopt;
		return src.substr(0, 4 + hexadecimal_entity->first.size());
	};
	return std::nullopt;
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
		remainder.remove_prefix(html_entity->size());
		ltrim(remainder);
		if (!remainder.empty()) {
			std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size())));
			std::unique_ptr<formal::lex_node> node(new formal::lex_node(std::move(stage)));
			src.insertNSlotsAt(1, viewpoint + 1);
			src[viewpoint + 1] = node.release();
		};

		*w = formal::word(std::shared_ptr<const std::string>(new std::string(*html_entity)), w->origin(), w->code());
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

		auto close_text = closing->c_anchor<formal::word>()->value();

		if (r_token != close_text) return ret;
		ptrdiff_t ub = viewpoint;
		while (0 <= --ub) {
			decltype(auto) opening = src[ub];
			if (!(opening->code() & formal::Inert_Token)) continue;	// our triggers are inert tokens
//			if (x->code() & formal::Error) return ret;	// do not try to process error tokens
			if (1 != opening->is_pure_anchor()) continue;	// we only try to manipulate things that don't have internal syntax

			auto open_text = opening->c_anchor<formal::word>()->value();
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
	auto tag = dynamic_cast<const HTMLtag*>(closing->c_anchor<formal::parsed>());
	if (!tag) return ret;
	if (HTMLtag::mode::closing != tag->tag_type()) return ret;

	ptrdiff_t ub = viewpoint;
	while (0 <= --ub) {
		decltype(auto) opening = src[ub];
		auto open_tag = dynamic_cast<const HTMLtag*>(opening->c_anchor<formal::parsed>());
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
			gentzen::preaxiomatic::parse_lex(src[ub]);
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
					const auto text = target->c_anchor<formal::word>()->value();
					for (decltype(auto) x : reserved_atomic) {
						if (x.first == text) return (formal::lex_node*)nullptr;
					}
				}

				if (target->set_null_post_anchor(src[viewpoint])) return target;
				// \todo suffixing to () [] {} should be fine
				return (formal::lex_node *)nullptr;
			};

			decltype(auto) bind_to = formal::lex_node::find_binding_predecessor(src.begin()+(viewpoint-1));
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

auto check_for_gentzen_wellformed(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
	std::vector<size_t> ret;

// using subsequence = std::tuple<size_t, std::span<formal::lex_node*>, std::any >;
	auto test2 = gentzen::syntax_check(formal::is_wff::subsequence(0, std::span(src.begin(), viewpoint + 1), std::vector<std::any>()));

//		using change_target = std::pair<size_t, std::span<formal::lex_node*> >;
//		using parse_t = std::function<change_target()>;
//		using wff_t = std::pair<bool, std::pair<formal::src_location, std::string> >;
//	using ret_parse_t = std::pair<wff_t, parse_t>;
	if (!test2.first.second.second.empty()) {
		if (test2.first.first) {
			warning_report(test2.first.second.first, test2.first.second.second);
		} else {
			error_report(test2.first.second.first, test2.first.second.second);
		}
	}
	if (test2.second) {
		ret.push_back(0);
		auto changed = test2.second();
		ret[0] = changed.first;
		if (1 < changed.second.size()) {
			if (changed.second[0] == src[changed.first]) {
				src.DeleteNSlotsAt(changed.second.size()-1, changed.first + 1);
			} else {
				// invariant violation
			}
		}
	}
	return ret;
}

static auto& TokenGrammar() {
	static std::unique_ptr<kuroda::parser<formal::lex_node> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());
		ooao->register_build_nonterminal(tokenize);
		ooao->register_build_nonterminal(balanced_atomic_handler(reserved_atomic[0].first, reserved_atomic[1].first));
		ooao->register_build_nonterminal(balanced_atomic_handler(reserved_atomic[2].first, reserved_atomic[3].first));
		ooao->register_build_nonterminal(balanced_atomic_handler(reserved_atomic[4].first, reserved_atomic[5].first));
		ooao->register_build_nonterminal(balanced_html_tag);
		ooao->register_build_nonterminal(HTML_bind_to_preceding);

		gentzen::syntax_check.register_handler(gentzen::var::wff);
		gentzen::var::init(gentzen::argument_enforcer::get());
		gentzen::inference_rule::init(gentzen::argument_enforcer::get());

		// \todo need local test cases for these
		ooao->register_build_nonterminal(gentzen::argument_enforcer::reject_adjacent);
		ooao->register_left_edge_build_nonterminal(gentzen::argument_enforcer::reject_left_edge);
		ooao->register_right_edge_build_nonterminal(gentzen::argument_enforcer::reject_right_edge);

		ooao->register_right_edge_build_nonterminal(check_for_gentzen_wellformed);
	};
	return *ooao;
}

// main language syntax
static kuroda::parser<formal::lex_node>& GentzenGrammar() {
	static std::unique_ptr<kuroda::parser<formal::lex_node> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());

		// we do not register terminals for the Gentzen grammar.

		ooao->register_right_edge_build_nonterminal(gentzen::var::quantifier_bind_global); // must happen after var names are decorated with HTML

		ooao->register_global_build(gentzen::inference_rule::global_parse); // early as these are extremely high precedence
		ooao->register_global_build(gentzen::var::global_parse);
	};
	return *ooao;
}

template<class T>
static auto apply_grammar(kuroda::parser<formal::lex_node>& grammar, typename kuroda::parser<T>::symbols& lines)
{
	kuroda::parser<formal::lex_node>::symbols stage;
	auto wrapped = formal::lex_node::pop_front(lines);
	while (wrapped) {
		grammar.append_to_parse(stage, wrapped.release());
		wrapped = formal::lex_node::pop_front(lines);
	};
	return stage;
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

		// intent is one line, one statement
		while (!lines.empty()) {
			try {
			auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
			try {
				if (0 >= Errors.count()) TokenGrammar().finite_parse(stage);
			} catch (std::exception& e) {
				std::cout << "Token finite parse: " << e.what() << "\n";
				return 3;
			}
			std::cout << std::to_string(stage.size()) << "\n";
			std::cout << to_string(stage) << std::endl;
			if (0 < Errors.count()) continue;

			GentzenGrammar().complete_parse(stage);
			} catch (std::exception& e) {
				std::cout << "line iteration body: " << e.what() << "\n";
				return 3;
			}
		}
	}

//	if (!to_console) STRING_LITERAL_TO_STDOUT("<pre>\n");

//	STRING_LITERAL_TO_STDOUT("End testing\n");
//	if (!to_console) STRING_LITERAL_TO_STDOUT("</pre>\n");
	return Errors.count() ? EXIT_FAILURE : EXIT_SUCCESS;
};

#endif
