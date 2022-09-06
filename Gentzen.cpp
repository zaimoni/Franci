#include "Zaimoni.STL/LexParse/LexNode.hpp"
#include "Zaimoni.STL/LexParse/FormalWord.hpp"
#include "Zaimoni.STL/LexParse/Kuroda.hpp"

#ifdef GENTZEN_DRIVER
#include "HTMLtag.hpp"
#include "Zaimoni.STL/LexParse/string_view.hpp"
#include "test_driver.h"
#include "Zaimoni.STL/Pure.C/comptest.h"
#include <filesystem>
#include <memory>
#include <fstream>
#include <iostream>

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

// prototype class -- extract to own files when stable

namespace gentzen {

	// still abstract
	class domain {
	protected:
		domain() = default;

	public:
		domain(const domain& src) = default;
		domain(domain&& src) = default;
		domain& operator=(const domain& src) = default;
		domain& operator=(domain&& src) = default;
		~domain() = default;
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

		std::string to_s() const {
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

	// deferred: uniqueness quantification (unclear what data representation should be)
	class var {
		unsigned long long _quant_code;
		std::shared_ptr<const formal::parsed> _var;
		std::shared_ptr<const domain> _domain;

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
			if (src.code() & HTMLtag::Entity) {
				if (1 != src.is_pure_anchor()) return 0; // \todo invariant violation
				return legal_quantifier(*src.anchor<formal::word>());
			}

			// \todo? visually, it is true that a span that rotates A or E 180-degrees "works" and we could accept that as an alternate encoding
			// e.g, <span style="transform:rotate(180deg);display:inline-block">E</span>
			return 0;
		}

		static unsigned int legal_quantifier(const formal::word& src) {
			struct is_quantifier_entity {
				int operator()(std::string_view src) {
					if (lookup_HTML_entity("forall")->first == src) return (unsigned int)(quantifier::ForAll);
					if (lookup_HTML_entity("exist")->first == src) return (unsigned int)(quantifier::ThereIs);
					return 0;
				}
				int operator()(char32_t src) {
					if (lookup_HTML_entity("forall")->second == src) return (unsigned int)(quantifier::ForAll);
					if (lookup_HTML_entity("exist")->second == src) return (unsigned int)(quantifier::ThereIs);
					return 0;
				}
			};

			if (src.code() & HTMLtag::Entity) {
				if (decltype(auto) test = interpret_HTML_entity(src.value())) {
					return std::visit(is_quantifier_entity(), *test);
				}
				return 0;
			}
			return 0;
		}
	};

	class inference_rule {
		std::string _name;
		std::vector<std::shared_ptr<const formal::parsed> > _vars;
		std::vector<std::shared_ptr<const formal::parsed> > _hypotheses;
		std::vector<std::shared_ptr<const formal::parsed> > _conclusions;

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
	LG_MAX
};

static_assert(sizeof(unsigned long long)*CHAR_BIT >= LG_MAX);
static_assert(!(formal::Comment & (1ULL << LG_PP_like)));
static_assert(!(formal::Comment & (1ULL << LG_CPP_like)));
static_assert(!(formal::Error & (1ULL << LG_PP_like)));
static_assert(!(formal::Error & (1ULL << LG_CPP_like)));

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

// first stage is a very simple line-finder (pre-preprocessor, shell script, ...)
static auto& LineGrammar() {
	static std::unique_ptr<kuroda::parser<formal::word> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());
		ooao->register_terminal(IsOneLineComment);
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
		std::cout << std::to_string(stage.size()) << "\n";

		formal::lex_node::to_s(std::cout, stage) << "\n";
	}

//	if (!to_console) STRING_LITERAL_TO_STDOUT("<pre>\n");

//	STRING_LITERAL_TO_STDOUT("End testing\n");
//	if (!to_console) STRING_LITERAL_TO_STDOUT("</pre>\n");
	return Errors.count() ? EXIT_FAILURE : EXIT_SUCCESS;
};

#endif
