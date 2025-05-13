#include "Zaimoni.STL/LexParse/LexNode.hpp"

#ifdef GENTZEN_DRIVER
// https://github.com/fktn-k/fkYAML; MIT license
#include "fkYAML/node.hpp"

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

const std::filesystem::path& self_path(const char* const _arg = nullptr)
{
	static std::filesystem::path ooao;
	if (!_arg || !*_arg) return ooao;
	if (!ooao.empty()) return ooao;	// XXX invariant failure \todo debug mode should hard-error
	// should be argv[0], which exists as it is our name
	ooao = _arg;

	// work around test driver issue
	if (!std::filesystem::exists(ooao)) ooao = std::string("../../") + _arg;

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

static std::optional<std::string_view> interpret_word(const formal::lex_node& src)
{
	if (1 != src.is_pure_anchor()) return std::nullopt;
	return src.c_anchor<formal::word>()->value();
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

static auto apply_grammar(kuroda::parser<formal::lex_node>& grammar, std::unique_ptr<formal::lex_node> wrapped)
{
	kuroda::parser<formal::lex_node>::symbols stage;
	grammar.append_to_parse(stage, wrapped.release());
	return stage;
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

	// could lose HTML formatting
	std::optional<std::pair<const HTMLtag*, std::string_view> > could_be_symbol(const formal::lex_node* src) {
		std::pair<const HTMLtag*, std::string_view> ret(nullptr, std::string_view());
		do {
			auto is_word = interpret_word(*src);
			if (is_word) {
				ret.second = *is_word;
				return ret;
			}
			auto is_tag = dynamic_cast<const HTMLtag*>(src->c_anchor<formal::parsed>());
			if (!is_tag) return std::nullopt;
			if (1 != src->infix().size()) return std::nullopt;
			if (!ret.first) ret.first = is_tag;
		} while (src = src->infix().front());
		return std::nullopt;
	}

	const HTMLtag* is_token_sequence_var(const formal::lex_node* src) {
		auto ret = could_be_symbol(src);
		if (ret && ret->first) {
			if (auto test = ret->first->attr("tokensequence")) return ret->first;
		}
		return nullptr;
	}

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

//template<bool C_Shell_Line_Continue = true>
static auto to_lines(std::vector<std::string>& in, formal::src_location& origin)
{
	kuroda::parser<formal::word>::symbols ret;
	for (const auto& x : in) {
		if (!x.empty()) {
			std::unique_ptr<const std::string> next(new std::string(std::move(x)));
			LineGrammar().append_to_parse(ret, new formal::word(std::shared_ptr<const std::string>(next.release()), origin));
		}

		origin.line_pos.first++;
		origin.line_pos.second = 0;
	}
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
//			gentzen::preaxiomatic::parse_lex(src[ub]);
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

		// \todo need local test cases for these
		ooao->register_build_nonterminal(gentzen::argument_enforcer::reject_adjacent);
		ooao->register_left_edge_build_nonterminal(gentzen::argument_enforcer::reject_left_edge);
		ooao->register_right_edge_build_nonterminal(gentzen::argument_enforcer::reject_right_edge);

		ooao->register_right_edge_build_nonterminal(check_for_gentzen_wellformed);
	};
	return *ooao;
}

class undefined_SVO {
private:
	std::vector<formal::lex_node> _postfix;

	static std::unique_ptr<undefined_SVO>& _get()
	{
		static std::unique_ptr<undefined_SVO> oaoo;
		if (!oaoo) {
			oaoo = decltype(oaoo)(new decltype(oaoo)::element_type());
			oaoo->load();
		}
		return oaoo;
	}
public:
	static constexpr const unsigned long long postfix = (1ULL << 2); // reserve this flag for both word and lex_node

	static_assert(!(formal::Comment& postfix));
	static_assert(!(formal::Error& postfix));
	static_assert(!(formal::Inert_Token& postfix));
	static_assert(!(formal::Tokenized& postfix));
	static_assert(!(HTMLtag::Entity& postfix));

	undefined_SVO() = default;

	static undefined_SVO& get() { return *_get(); }

private:
	void load() {
		std::ifstream ifs(std::filesystem::path(self_path()).replace_filename("cfg/core.yaml"));

		fkyaml::node root = fkyaml::node::deserialize(ifs);
		std::vector<std::string> dest;

		if (root.contains("SVO_postfix_undefined")) {
			fkyaml::from_node(root["SVO_postfix_undefined"], dest);

			formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path("cfg/core.yaml:SVO_postfix_undefined")));
			auto lines = to_lines(dest, src);
			while (!lines.empty()) {
				try {
					const auto prior_errors = Errors.count();
					auto stage = apply_grammar(TokenGrammar(), formal::lex_node::pop_front(lines));
					src.line_pos.first++;
					src.line_pos.second = 0;
					if (prior_errors < Errors.count()) continue;
					if (1 == stage[0]->is_pure_anchor()) {
						formal::lex_node tmp(*stage[0]);
						stage.DeleteIdx(0);
						if (!stage.empty()) {
							bool ok = true;
							for (decltype(auto) x : stage) {
								if (1 != x->is_pure_anchor()) {
									error_report(*x, "need more sophisticated parser");
									ok = false;
									break;
								}
							}
							tmp.set_postfix(std::move(stage));
						}
						_postfix.push_back(std::move(tmp));
					}
					else {
						error_report(*stage[0], "not a lexical word");
					}
				}
				catch (std::exception& e) {
					std::cout << "line iteration body: " << e.what() << "\n";
					return;
				}
			}
		}

		(decltype(dest)()).swap(dest);

		// test driver
		if (root.contains("test_lines")) {
			fkyaml::from_node(root["test_lines"], dest);

			formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path("cfg/core.yaml:test_lines")));
			auto lines = to_lines(dest, src);
			while (!lines.empty()) {
				try {
					const auto prior_errors = Errors.count();
					auto stage = apply_grammar(TokenGrammar(), formal::lex_node::pop_front(lines));
					src.line_pos.first++;
					src.line_pos.second = 0;
					if (prior_errors < Errors.count()) continue;

					kuroda::parser<formal::lex_node>::edit_span scan(stage);

					global_parse(scan);
					formal::lex_node::to_s(std::cout, stage);
					std::cout << "\n";
				}
				catch (std::exception& e) {
					std::cout << "line iteration body: " << e.what() << "\n";
					return;
				}
			}
		}
	}

public:
	static bool global_parse(kuroda::parser<formal::lex_node>::edit_span& tokens) {
		enum { trace_parse = 0 };
		if constexpr (trace_parse) std::cerr << "undefined_SVO::global_parse: enter\n";

		static constexpr const std::string_view symbol("symbol");
		const auto starting_errors = Errors.count();
		const auto& x = get();

		ptrdiff_t n = -1;
		for (const auto& phrase : x._postfix) {
			++n;
			const auto reverse_offset = 1 + phrase.postfix().size();
			if (tokens.size() <= reverse_offset) continue;	// won't match

			const auto phrase_origin = tokens.size() - reverse_offset;
			kuroda::parser<formal::lex_node>::edit_span stage(tokens, phrase_origin, reverse_offset);

			if (0 != phrase.token_compare(stage)) continue;	// doesn't match
			const auto last_word = interpret_word(*phrase.postfix().back());
			const bool lhs_is_symbol = last_word ? last_word.value() == symbol : false;

			// did match: attempt parse
			kuroda::parser<formal::lex_node>::symbols stage_prefix(phrase_origin);
			kuroda::parser<formal::lex_node>::symbols stage_suffix(reverse_offset - 1);
			std::unique_ptr<formal::lex_node> dest(tokens[phrase_origin]);
			tokens[phrase_origin] = nullptr;
			if (0 < phrase_origin) {
				const auto prefix_origin = tokens.begin();
				std::copy_n(prefix_origin, phrase_origin, stage_prefix.begin());
				std::fill_n(prefix_origin, phrase_origin, nullptr);
				dest.get()->set_prefix(std::move(stage_prefix));
			}
			if (const auto ub = stage_suffix.size()) {
				const auto suffix_origin = tokens.begin() + phrase_origin + 1;
				std::copy_n(suffix_origin, ub, stage_suffix.begin());
				std::fill_n(suffix_origin, ub, nullptr);
				dest.get()->set_postfix(std::move(stage_suffix));
			}
			dest.get()->interpret(postfix, n);
			if (lhs_is_symbol) {
				if (1 != phrase_origin) {
					error_report(*dest, "cannot declare a token-sequence as a symbol");
				} else if (!gentzen::could_be_symbol(dest->prefix().front())) {
					error_report(*dest, "symbol must be a word");
				}
			}

			tokens[0] = dest.release();
			kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, tokens.size() - 1, 1);
			return true;
		}

		return false;
	}
};

// main language syntax
static kuroda::parser<formal::lex_node>& GentzenGrammar() {
	static std::unique_ptr<kuroda::parser<formal::lex_node> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());

		// we do not register terminals for the Gentzen grammar.

		ooao->register_global_build(undefined_SVO::global_parse);
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
	auto who_am_i = self_path(argv[0]);
//	std::wcout << std::filesystem::exists(who_am_i) << "\n";
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
			const auto prior_errors = Errors.count();
			try {
			auto stage = apply_grammar(TokenGrammar(), formal::lex_node::pop_front(lines));
			try {
				if (0 >= Errors.count()) TokenGrammar().finite_parse(stage);
			} catch (std::exception& e) {
				std::cout << "Token finite parse: " << e.what() << "\n";
				return 3;
			}
			std::cout << std::to_string(stage.size()) << "\n";
			formal::lex_node::to_s(std::cout, stage) << "\n";
			if (prior_errors < Errors.count()) continue;

			try {
				do {
					GentzenGrammar().finite_parse(stage);
				} while (GentzenGrammar().finite_parse(kuroda::parser<formal::lex_node>::to_editspan(stage)));
			} catch (std::exception& e) {
				std::cout << "Gentzen finite parse: " << e.what() << "\n";
				return 3;
			}
			std::cout << std::to_string(stage.size()) << "\n";
			formal::lex_node::to_s(std::cout, stage) << "\n";
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
