#include "Zaimoni.STL/LexParse/LexNode.hpp"

#ifdef GENTZEN_DRIVER
#include <filesystem>
#include <memory>
#include <fstream>
#include <iostream>
#include <span>
#include <initializer_list>
#include <any>
#include <concepts>
#include <map>
#include <tuple>
#include <stacktrace>

// https://github.com/fktn-k/fkYAML; MIT license
#include "fkYAML/node.hpp"

#include "HTMLtag.hpp"
#include "Zaimoni.STL/LexParse/string_view.hpp"
#include "Zaimoni.STL/stack.hpp"
#include "Zaimoni.STL/observer.hpp"
#include "test_driver.h"
#include "Zaimoni.STL/Pure.C/comptest.h"

#include "errcount.hpp"

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
#include <io.h>
#else
#include <unistd.h>
#endif

static error_counter<size_t> Errors(100, "too many errors");
static error_counter<size_t> Warnings(1000, "too many warnings");

// Diagnostic capture: while a diagnostic_capture is alive, the two reporting hooks below
// append to it instead of printing and counting.  Every report funnels through those hooks
// -- the lex_node overloads (record, then report), the formal::parsed overloads, and plain
// locations -- so a capture sees all of them, including a message on a node that is deleted
// right after, and one flushed just before a formal::parsed is built over it; a walk for
// pending messages sees neither.  This is the mechanism behind try_instantiate (SFINAE for
// uniform substitution).
// Single-threaded by assumption: the target is a file-static.  Captures nest; each restores
// the one it displaced.  Errors.count() does not move while capturing, so code that tests
// its delta for control flow does not see captured errors -- the same blind spot the
// deferred (record_error) diagnostics already have.
struct captured_diagnostics {
	std::vector<std::pair<formal::src_location, perl::scalar> > errors;
	std::vector<std::pair<formal::src_location, perl::scalar> > warnings;
};

class diagnostic_capture final {
	captured_diagnostics _dest;
	captured_diagnostics* _displaced;

	static captured_diagnostics*& target() {
		static captured_diagnostics* ooao = nullptr;
		return ooao;
	}
public:
	diagnostic_capture() : _displaced(target()) { target() = &_dest; }
	~diagnostic_capture() { target() = _displaced; }
	diagnostic_capture(const diagnostic_capture&) = delete;
	diagnostic_capture(diagnostic_capture&&) = delete;
	diagnostic_capture& operator=(const diagnostic_capture&) = delete;
	diagnostic_capture& operator=(diagnostic_capture&&) = delete;

	const captured_diagnostics& get() const { return _dest; }
	static captured_diagnostics* active() { return target(); }
};

// stub for more sophisticated error reporting
void error_report(const formal::src_location& loc, const perl::scalar& err) {
	if (auto dest = diagnostic_capture::active()) {
		dest->errors.push_back(std::pair(loc, err));
		return;
	}
	if (loc.path) std::wcerr << loc.path->native();
	else std::cerr << "<unknown>";
	std::cerr << loc.to_s() << ": error : " << err.view() << '\n';
	++Errors;
}

void warning_report(const formal::src_location& loc, const perl::scalar& warn) {
	if (auto dest = diagnostic_capture::active()) {
		dest->warnings.push_back(std::pair(loc, warn));
		return;
	}
	if (loc.path) std::wcerr << loc.path->native();
	else std::cerr << "<unknown>";
	std::cerr << loc.to_s() << ": warning : " << warn.view() << '\n';
	++Warnings;
}

// re-issues captured diagnostics through the hooks: printed and counted, or captured again
// by an enclosing diagnostic_capture
static void replay(const captured_diagnostics& src) {
	for (decltype(auto) x : src.errors) error_report(x.first, x.second);
	for (decltype(auto) x : src.warnings) warning_report(x.first, x.second);
}

static auto relevant_stacktrace(size_t delta = 0) {
	// \todo(_MSC_VER) compiler migration checklist: everything below is calibrated to MSVC
	// (VS 18, x64, Debug).  On any new compiler, CRT, or optimization level, set
	// implementation_test = 1, force an exception, and re-verify:
	// * after_main -- how many entries follow main in the stacktrace; platform-specific
	//   (here 6 = 4 CRT startup frames + 2 OS thread-start frames).
	// * "!main+" -- the spelling of main within to_string(stacktrace_entry), which is
	//   implementation-defined (here module!symbol+offset).
	// * current(1 + delta) -- the skip count assumes relevant_stacktrace is not inlined into
	//   its caller, nor (delta = 1) report_exception into its own.
	// Verified 2026-09-19 for both call paths: delta = 0 starts at the calling function
	// (a throw site), delta = 1 at the catch funclet (main$catch$NN); both end on main.
#ifndef _MSC_VER
#pragma message("relevant_stacktrace: not MSVC -- work through the \\todo(_MSC_VER) compiler migration checklist")
#endif
	auto preview = std::stacktrace::current(1 + delta);
	enum { after_main = 6, implementation_test = 0 };
	auto actual = std::stacktrace::current(1 + delta, preview.size() - after_main);
	if constexpr (implementation_test) {
		if (   actual.empty()
			|| std::string::npos == to_string(*actual.rbegin()).find("!main+")) {
			std::cout << "stacktrace does not appear to end on main (platform-dependent)\n";
		}
	}

	return to_string(actual);
}

// for the most-general exception handlers.  std::stacktrace::current() here is the
// *handler's* stack: unwinding has already discarded the throw point's frames, so this
// identifies which handler caught and the call chain above it.  A throw-point trace has
// to be captured where the exception is constructed (cf. kuroda::parser<T>::finite_parse).
static void report_exception(const char* context, const std::exception& e)
{
	std::cout << context << ": " << e.what() << "\ncaught at:\n" << relevant_stacktrace(1) << "\n" << std::flush;
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

// test-harness UI, parallel to Iskandria Form1.AskUserToPickOne (C# WinForms).
// Blocking: the C++ standard library has no non-blocking console read.

constexpr char selection_key(size_t i) { return (10 > i) ? char('0' + i) : char('A' + (i - 10)); }

constexpr ptrdiff_t index_from_key(char k) {
	if ('0' <= k && k <= '9') return k - '0';
	if ('A' <= k && k <= 'Z') return 10 + (k - 'A');
	if ('a' <= k && k <= 'z') return 10 + (k - 'a');
	return -1;
}

// returns std::nullopt when cancelled (blank line stands in for the Escape key; likewise stdin close/failure)
// without a verdict from on_pick; a std::nullopt return from on_pick keeps the menu up and keeps asking
std::optional<bool> ask_user_to_pick_one(const std::string& title, const std::vector<std::string>& entries, const std::function<std::optional<bool>(size_t)>& on_pick)
{
	if (entries.empty())
#ifndef NDEBUG
		throw std::logic_error("tracing");
#else
		return false;
#endif

	const size_t max_direct = (36 >= entries.size()) ? entries.size() : 36;	// \todo paging past 36

	auto render = [&]() {
		std::cout << title << '\n';
		for (size_t i = 0; i < max_direct; ++i) std::cout << selection_key(i) << ") " << entries[i] << '\n';
	};
	render();

	std::optional<bool> outcome;
	std::string buffer;
	// std::cin is tied to std::cout, so the menu flushes before we block
	while (std::getline(std::cin, buffer)) {
		if (buffer.empty()) break;
		const auto idx = index_from_key(buffer.front());
		if (0 <= idx && size_t(idx) < max_direct) {
			outcome = on_pick(idx);
			if (outcome) break;
			render();	// the popup stays visible in the source implementation
		}
	}
	return outcome;
}

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

// Returns the number of grouping layers stripped when the loop terminates normally.
// When the ok predicate terminates the loop after c layers, returns -((ptrdiff_t)(c+1)),
// so a negative value signals termination by predicate: the caller decides whether that
// is an error condition (e.g. "(A,B) &#9500; C" must not gain a hypothesis, while the
// ordered pair (1,2) is fine), and any further detail is the function object's to
// remember.  std::bad_alloc fires long before the ptrdiff_t conversion could overflow.
ptrdiff_t remove_grouping_parentheses(formal::lex_node*& src, std::function<bool(const formal::lex_node& contents)> ok = nullptr)
{
	size_t c = 0;
	if (!src) return 0;
restart:
	auto anchor = src->c_anchor<formal::word>();
	if (!anchor) return ptrdiff_t(c);
	if (anchor->value() != reserved_atomic[0].first) return ptrdiff_t(c);
	auto post_anchor = src->c_post_anchor<formal::word>();
	if (!post_anchor) return ptrdiff_t(c);
	if (post_anchor->value() != reserved_atomic[1].first) return ptrdiff_t(c);
	if (!src->prefix().empty()) return ptrdiff_t(c);
	if (!src->postfix().empty()) return ptrdiff_t(c);
	if (!src->fragments().empty()) return ptrdiff_t(c);
	if (1 != src->infix().size()) return ptrdiff_t(c);
	// C++-side invariant violation (the grammar cannot produce this): std::logic_error
	// in all build modes.  Do not to_s() the node here -- the null is in its infix.
	if (!src->infix().front()) throw std::logic_error(std::string("remove_grouping_parentheses: found nullptr inside () at ") + src->origin().to_s());
	if (ok && !ok(*src->infix().front())) return -ptrdiff_t(c + 1);

	// simulate tail recursion
	auto relay = src->infix().front(); // this has to be a copy, not a reference
	src->infix().front() = nullptr;
	delete src;
	src = relay;
	++c;
	goto restart;
}

// For typical formal languages, comma is both the lowest precedence binary infix symbol,
// and generally has right-fold semantics when parsing.
bool detect_comma(const formal::lex_node& src) {
	if (const auto x = src.c_anchor<formal::word>()) return comma == x->value();
	return false;
}

// useful to prevent parentheses unwrapping when arity would be increased,
// or when an ordered tuple is to be parsed.
// \todo(__cpp_reflection) evaluate conversion to a template function in
// Zaimoni.STL/flat_alg.h: C++23 standard syntax could not extract detect_comma's
// parameter type for interpretation as a stack-efficient compatible parameter;
// static reflection (P2996) can.
#ifdef __cpp_reflection
#pragma message("not_detect_comma: reflection available -- revisit the flat_alg.h template conversion")
#endif
bool not_detect_comma(const formal::lex_node& src) { return !detect_comma(src); }

bool is_strict_binary_node(const formal::lex_node& src) {
	if (1 != src.postfix().size()) return false;
	if (1 != src.prefix().size()) return false;
	if (!src.infix().empty()) return false;
	if (!src.fragments().empty()) return false;
	if (0 != src.post_anchor_code()) return false;
	return true;
}

bool is_binary_node(const formal::lex_node& src) {
	if (src.postfix().empty()) return false;
	if (src.prefix().empty()) return false;
	if (!src.infix().empty()) return false;
	if (!src.fragments().empty()) return false;
	if (0 != src.post_anchor_code()) return false;
	return true;
}

// while the semantics of left-fold, right-fold, and generalized-associativity are different,
// they conserve RAM for n-ary expressions comparably
bool is_lfold_rfold_gassoc_node(const formal::lex_node& src) {
	if (src.fragments().empty()) return false;
	if (!src.postfix().empty()) return false;
	if (!src.prefix().empty()) return false;
	if (!src.infix().empty()) return false;
	if (0 != src.post_anchor_code()) return false;
	return true;
}

bool are_fragments_superficially_parsed(const formal::lex_node& src) {
	if (src.fragments().empty()) return false;
	for (decltype(auto) x : src.fragments()) if (1 != x.size() || !x.front()) return false;
	return true;
}

// Interior node of a same-word fold chain: same anchor word, strict binary, both
// children present.  Shared by the sizing and commit traversals below so they cannot
// disagree about the operand count.
static bool _word_fold_interior(const formal::lex_node& node, const std::string_view& w)
{
	auto nw = node.c_anchor<formal::word>();
	if (!nw || nw->value() != w) return false;
	if (!is_strict_binary_node(node)) return false;
	return node.prefix().front() && node.postfix().front();
}

static size_t _word_fold_operand_count(const formal::lex_node& node, const std::string_view& w)
{
	size_t ret = 0;
	const formal::lex_node* const children[2] = { node.prefix().front(), node.postfix().front() };
	for (decltype(auto) c : children) {
		if (_word_fold_interior(*c, w)) ret += _word_fold_operand_count(*c, w);
		else ++ret;
	}
	return ret;
}

// commit half: nothrow.  Harvests operands left-to-right into pre-populated
// single-entry slots; interior chain nodes are deleted once their subtrees are moved.
static void _word_fold_harvest(formal::lex_node& node, const std::string_view& w, std::vector<kuroda::parser<formal::lex_node>::symbols>& stage, size_t& i)
{
	formal::lex_node** const slots[2] = { &node.prefix().front(), &node.postfix().front() };
	for (decltype(auto) s : slots) {
		if (_word_fold_interior(**s, w)) {
			_word_fold_harvest(**s, w, stage, i);
			delete *s;
		} else {
			stage[i++].front() = *s;
		}
		*s = nullptr;
	}
}

bool normalize_word_strict_binary_node_to_lfold_r_fold_gassoc_node(formal::lex_node& src) {
	if (formal::Error & src.code()) return false;
	auto w = src.c_anchor<formal::word>();
	if (!w) return false;
	if (!is_strict_binary_node(src)) return false;
	// C++-side invariant violations (the grammar cannot produce these), so
	// std::logic_error in all build modes: error-marking the line and continuing
	// would presume the corruption is line-local, which an internal invariant
	// breach specifically undermines
	if (!src.prefix().front()) throw std::logic_error("normalize_word_strict_binary_node: null prefix.front() in " + src.to_s());
	if (!src.postfix().front()) throw std::logic_error("normalize_word_strict_binary_node: null postfix.front() in " + src.to_s());

	const auto w_v = w->value();
	// interior-ness, not mere anchor-word equality, is what makes a child flattenable
	if (   !_word_fold_interior(*src.prefix().front(), w_v)
		&& !_word_fold_interior(*src.postfix().front(), w_v)) return false;

	// ACID (Atomic Commit Insert Delete): all allocation happens against local staging
	// before src is mutated, so a std::bad_alloc from sizing or pre-population leaves
	// src unchanged; the second traversal and the container swaps are nothrow.
	std::remove_reference_t<decltype(src.fragments())> stage(_word_fold_operand_count(src, w_v));
	for (decltype(auto) frag : stage) frag.push_back(nullptr);
	std::remove_reference_t<decltype(src.prefix())> empty_prefix;
	std::remove_reference_t<decltype(src.postfix())> empty_postfix;

	// commit: nothrow from here on
	size_t i = 0;
	_word_fold_harvest(src, w_v, stage, i);
	stage.swap(src.fragments());
	empty_prefix.swap(src.prefix());
	empty_postfix.swap(src.postfix());
	return true;
}

bool is_reserved_atomic(const formal::lex_node& src)
{
	if (const auto word = src.c_anchor<formal::word>()) {
		const auto text = word->value();
		for (decltype(auto) x : reserved_atomic) {
			if (text.starts_with(x.first)) return true;
		}
	}
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

static std::string to_string(const kuroda::parser<formal::lex_node>::edit_span& src)
{
	std::vector<perl::scalar> stage;
	ptrdiff_t n = -1;
	while (++n < src.size()) {
		stage.push_back((perl::scalar)(*src[n]));
	}
	return join(stage, " ");
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
static kuroda::parser<formal::lex_node>& TokenGrammar();

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

	// due to typical HTML layout, we don't want these as symbols
	static constexpr const char* const HTML_binds_to_predecessor[] = {
		"sub",
		"sup"
	};

	// could lose HTML formatting
	std::optional<std::pair<std::vector<const HTMLtag*>, std::string_view> > anchor_is_symbol_like(const formal::lex_node* src) {
		std::pair<std::vector<const HTMLtag*>, std::string_view> ret;
		do {
			if (auto word_code = src->anchor_code(); 1 == word_code || 6 == word_code) {
				if (!src->infix().empty()) return std::nullopt;
				if (0 != src->post_anchor_code()) return std::nullopt;
				ret.second = src->c_anchor<formal::word>()->value();
				return ret;
			}
			auto is_tag = dynamic_cast<const HTMLtag*>(src->c_anchor<formal::parsed>());
			if (!is_tag) return std::nullopt;
			if (1 != src->infix().size()) return std::nullopt;
			for (decltype(auto) x : HTML_binds_to_predecessor) {
				if (is_tag->tag_name() == x) return std::nullopt;
			}
			// HTML has already been lexed, so we're actually a balanced tag
			ret.first.push_back(is_tag);
		} while (src = src->infix().front()); // simulate tail recursion
		return std::nullopt;
	}

	std::optional<std::pair<std::vector<const HTMLtag*>, std::string_view> > is_symbol_like(const formal::lex_node* src) {
		if (auto ret = anchor_is_symbol_like(src)) {
			if (!src->prefix().empty()) return std::nullopt;
			if (!src->postfix().empty()) return std::nullopt;
			if (!src->fragments().empty()) return std::nullopt;
			return ret;
		}
		return std::nullopt;
	}

	bool is_placeholder_syntax_symbol(const formal::lex_node* x) {
		if (!x) return false;
	restart:
		if (!x->prefix().empty()) return false;
		if (!x->infix().empty()) return false;
		if (!x->postfix().empty()) return false;
		if (!x->fragments().empty()) return false;
		if (auto node = x->c_anchor<formal::lex_node>()) {
			if (0 != x->post_anchor_code()) return false;
			x = node;	// simulate tail recursion
			goto restart;
		}
		auto word = x->c_anchor<formal::word>();
		if (!word) return false;
		const auto text = word->value();
		if (1 != text.size()) return false;
		// assuming ASCII-friendly encoding
		auto c = text.front();
		if ('A' > c || 'Z' < c) return false;

		if (auto html_ish = x->c_post_anchor<formal::lex_node>()) {
			if (!html_ish->prefix().empty()) return false;
			if (!html_ish->postfix().empty()) return false;
			if (!html_ish->fragments().empty()) return false;
			if ( html_ish->infix().empty()) return false;
			// the axiom is vague on what counts as a subscripted placeholder substitution
			const auto tag = HTMLtag::is_balanced_pair(*html_ish);
			if (!tag) return false;
			return std::string_view(*tag) == std::string_view("sub");
		}
		return 0 == x->post_anchor_code();
	}

	bool is_symbol_placeholder_syntax_symbol(const formal::lex_node* x) {
		if (!x) return false;
	restart:
		if (!x->prefix().empty()) return false;
		if (!x->infix().empty()) return false;
		if (!x->fragments().empty()) return false;
		if (!x->postfix().empty()) return false;
		if (auto node = x->c_anchor<formal::lex_node>()) {
			if (0 != x->post_anchor_code()) return false;
			x = node;	// simulate tail recursion
			goto restart;
		}
		auto word = x->c_anchor<formal::word>();
		if (!word) return false;
		const auto text = word->value();
		if (1 != text.size()) return false;
		if ('_' != text.front()) return false;

		if (auto html_ish = x->c_post_anchor<formal::lex_node>()) {
			if (!html_ish->prefix().empty()) return false;
			if (!html_ish->postfix().empty()) return false;
			if (!html_ish->fragments().empty()) return false;
			if ( html_ish->infix().empty()) return false;
			// the axiom is vague on what counts as a subscripted placeholder substitution
			const auto tag = HTMLtag::is_balanced_pair(*html_ish);
			if (!tag) return false;
			return std::string_view(*tag) == std::string_view("sub");
		}
		return 0 == x->post_anchor_code();
	}

	static constexpr const unsigned long long used_as_placeholder = (1ULL << 8); // reserve this flag for lex_node

	class symbol_catalog {
	private:
		std::vector<std::pair<std::shared_ptr<const formal::lex_node>, unsigned int > > _symbols;
		std::vector<std::shared_ptr<const formal::lex_node> > _constant_symbols;

		enum {
			// these are the kinds of symbols we can declare
			Prefix = 1U << 0,	// prefix symbol
			Infix = 1U << 1,	// infix symbol
			Postfix = 1U << 2,	// postfix symbol
			// Infix operations can support recursive-definition syntax.  Cf. https://wiki.haskell.org/Fold
//			Infix_LFold = 1U << 3,	// left-fold: A _ B _ C := (A _ B) _ C
//			Infix_GAssoc = 1U << 4, // generalized associativity: A _ B _ C = A _ (B _ C) = (A _ B) _ C
//			Infix_RFold = 1U << 5,	// right-fold: A _ B _ C := A _ (B _ C)
		};
	public:
		static constexpr const unsigned long long anchor_is_symbol = (1ULL << 5); // reserve this flag for lex_node
		static constexpr const unsigned long long anchor_is_const_symbol = (1ULL << 6); // reserve this flag for lex_node
		static constexpr const unsigned long long anchor_is_not_symbol = (1ULL << 7); // reserve this flag for lex_node

		static_assert(!(anchor_is_symbol & formal::Comment));
		static_assert(!(anchor_is_symbol & formal::Error));
		static_assert(!(anchor_is_symbol & formal::Inert_Token));
		static_assert(!(anchor_is_symbol & formal::Tokenized));
		static_assert(!(anchor_is_symbol & formal::RequestNormalization));
		static_assert(!(anchor_is_symbol & HTMLtag::Entity));
		static_assert(!(anchor_is_symbol & HTMLtag::Tag));
		static_assert(!(anchor_is_const_symbol & formal::Comment));
		static_assert(!(anchor_is_const_symbol & formal::Error));
		static_assert(!(anchor_is_const_symbol & formal::Inert_Token));
		static_assert(!(anchor_is_const_symbol & formal::Tokenized));
		static_assert(!(anchor_is_const_symbol & formal::RequestNormalization));
		static_assert(!(anchor_is_const_symbol & HTMLtag::Entity));
		static_assert(!(anchor_is_const_symbol & HTMLtag::Tag));
		static_assert(!(anchor_is_const_symbol & anchor_is_symbol));
		static_assert(!(anchor_is_not_symbol & formal::Comment));
		static_assert(!(anchor_is_not_symbol & formal::Error));
		static_assert(!(anchor_is_not_symbol & formal::Inert_Token));
		static_assert(!(anchor_is_not_symbol & formal::Tokenized));
		static_assert(!(anchor_is_not_symbol & formal::RequestNormalization));
		static_assert(!(anchor_is_not_symbol & HTMLtag::Entity));
		static_assert(!(anchor_is_not_symbol & HTMLtag::Tag));
		static_assert(!(anchor_is_not_symbol & anchor_is_symbol));
		static_assert(!(anchor_is_not_symbol & anchor_is_const_symbol));

		symbol_catalog() = default;
		symbol_catalog(const symbol_catalog&) = delete;
		symbol_catalog(symbol_catalog&&) = delete;
		symbol_catalog& operator=(const symbol_catalog&) = delete;
		symbol_catalog& operator=(symbol_catalog&&) = delete;
		~symbol_catalog() = default;
		static symbol_catalog& get() {
			static symbol_catalog ooao;
			return ooao;
		}

		// a constant symbol may not be any other kind of symbol
		std::optional<perl::scalar> declare_prefix(const std::shared_ptr<const formal::lex_node>& x)
		{
			enum { trace_parse = 0 };

			if constexpr (trace_parse) std::cerr << "declare_prefix: " << x->to_s() << "\n";

			if (is_placeholder_syntax_symbol(x.get())) {
				warning_report(*x, x->to_s() + ": is placeholder-syntax symbol");
				return std::nullopt;
			}
			if (is_symbol_placeholder_syntax_symbol(x.get())) {
				warning_report(*x, x->to_s() + ": is symbol-placeholder-syntax symbol");
				return std::nullopt;
			}
			if (is_reserved_atomic(*x)) {
				warning_report(*x, x->to_s() + ": is a reserved atomic symbol");
				return std::nullopt;
			}

			auto x_view = x->to_scalar().view();
			for (decltype(auto) symbol : _constant_symbols) {
				if (symbol->to_scalar().view() == x_view) return "already declared as constant symbol";
			}
			if constexpr (trace_parse) std::cerr << "declare_prefix: not a constant symbol\n";
			for (decltype(auto) symbol : _symbols) {
				if (symbol.first->to_scalar().view() == x_view) {
					if (symbol.second & Prefix) {
						warning_report(*x, x->to_s() + ":" + symbol.first->to_s() + " already declared as prefix symbol");
						return std::nullopt;
					}
					symbol.second |= Prefix;
					return std::nullopt;
				}
			}
			if constexpr (trace_parse) std::cerr << "declare_prefix: not a prior symbol\n";
			_symbols.push_back(std::make_pair(x, Prefix));
			return std::nullopt;
		}

		std::optional<perl::scalar> declare_infix(const std::shared_ptr<const formal::lex_node>& x)
		{
			enum { trace_parse = 0 };

			if constexpr (trace_parse) std::cerr << "declare_infix: " << x->to_s() << "\n";

			if (is_placeholder_syntax_symbol(x.get())) {
				warning_report(*x, x->to_s() + ": is placeholder-syntax symbol");
				return std::nullopt;
			}
			if (is_symbol_placeholder_syntax_symbol(x.get())) {
				warning_report(*x, x->to_s() + ": is symbol-placeholder-syntax symbol");
				return std::nullopt;
			}
			if (is_reserved_atomic(*x) && !detect_comma(*x)) {
				warning_report(*x, x->to_s() + ": is a reserved atomic symbol");
				return std::nullopt;
			}

			auto x_view = x->to_scalar().view();
			for (decltype(auto) symbol : _constant_symbols) {
				if (symbol->to_scalar().view() == x_view) return "already declared as constant symbol";
			}
			if constexpr (trace_parse) std::cerr << "declare_infix: not a constant symbol\n";
			for (decltype(auto) symbol : _symbols) {
				if (symbol.first->to_scalar().view() == x_view) {
					if (symbol.second & Infix) {
						warning_report(*x, std::string(x_view) + ":" + std::string(symbol.first->to_scalar().view()) + " already declared as infix symbol");
						return std::nullopt;
					}
					symbol.second |= Infix;
					return std::nullopt;
				}
			}
			if constexpr (trace_parse) std::cerr << "declare_infix: not a prior symbol\n";
			_symbols.push_back(std::make_pair(x, Infix));
			return std::nullopt;
		}

		std::optional<perl::scalar> declare_postfix(const std::shared_ptr<const formal::lex_node>& x)
		{
			enum { trace_parse = 0 };

			if constexpr (trace_parse) std::cerr << "declare_postfix: " << x->to_s() << "\n";

			if (is_placeholder_syntax_symbol(x.get())) {
				warning_report(*x, x->to_s() + ": is placeholder-syntax symbol");
				return std::nullopt;
			}
			if (is_symbol_placeholder_syntax_symbol(x.get())) {
				warning_report(*x, x->to_s() + ": is symbol-placeholder-syntax symbol");
				return std::nullopt;
			}
			if (is_reserved_atomic(*x)) {
				warning_report(*x, x->to_s() + ": is a reserved atomic symbol");
				return std::nullopt;
			}

			auto x_view = x->to_scalar().view();
			for (decltype(auto) symbol : _constant_symbols) {
				if (symbol->to_scalar().view() == x_view) return "already declared as constant symbol";
			}
			for (decltype(auto) symbol : _symbols) {
				if (symbol.first->to_scalar().view() == x_view) {
					if (symbol.second & Postfix) {
						warning_report(*x, x->to_s() + ":" + symbol.first->to_s() + " already declared as postfix symbol");
						return std::nullopt;
					}
					symbol.second |= Postfix;
					return std::nullopt;
				}
			}
			_symbols.push_back(std::make_pair(x, Postfix));
			return std::nullopt;
		}

		std::optional<perl::scalar> declare_constant(const std::shared_ptr<const formal::lex_node>& x)
		{
			enum { trace_parse = 0 };

			if constexpr (trace_parse) std::cerr << "declare_constant: " << x->to_s() << "\n";

			auto x_view = x->to_scalar().view();
			for (decltype(auto) symbol : _symbols) {
				if (symbol.first->to_scalar().view() == x_view) return "already declared as non-constant symbol";
			}
			for (decltype(auto) symbol : _constant_symbols) {
				if (symbol->to_scalar().view() == x_view) {
					warning_report(*x, "already declared as constant symbol");
					return std::nullopt;
				}
			}
			_constant_symbols.push_back(x);
			return std::nullopt;
		}

		std::optional<std::pair<std::shared_ptr<const formal::lex_node>, unsigned int > > is_nonconstant_symbol_offset(size_t n) {
			if (n >= _symbols.size()) return std::nullopt;
			return _symbols[n];
		}

		static std::optional<std::pair<ptrdiff_t, bool> > recognize(const formal::lex_node& src) {
			enum { trace_parse = 0 };
			const auto& catalog = get();
			if (catalog._symbols.empty() && catalog._constant_symbols.empty()) {
				if constexpr (trace_parse) std::cerr << "catalog._symbols.empty() && catalog._constant_symbols.empty()\n";
				return std::nullopt;
			}
			if constexpr (trace_parse) std::cerr << "entering anchor_symbol_parse\n";
			if (auto test = anchor_is_symbol_like(&src)) {
				if constexpr (trace_parse) std::cerr << src.to_s() + ": anchor_is_symbol_like(&src)\n";
				ptrdiff_t n = -1;
				while (++n < catalog._symbols.size()) {
					decltype(auto) symbol = catalog._symbols[n].first;
					auto test2 = *is_symbol_like(symbol.get());
					auto test_size = test->first.size();
					if (test2.first.size() != test_size) continue;
					if (test->second != test2.second) continue;
					bool matched = true;
					while (0 < test_size) {
						--test_size;
						if (test->first[test_size]->tag_name() != test2.first[test_size]->tag_name()) {
							matched = false;
							break;
						}
					}
					if (!matched) continue;
					return std::pair(n, true);
				}
				n = -1;
				while (++n < catalog._constant_symbols.size()) {
					decltype(auto) symbol = catalog._constant_symbols[n];
					auto test2 = *is_symbol_like(symbol.get());
					auto test_size = test->first.size();
					if (test2.first.size() != test_size) continue;
					if (test->second != test2.second) continue;
					bool matched = true;
					while (0 < test_size) {
						--test_size;
						if (test->first[test_size]->tag_name() != test2.first[test_size]->tag_name()) {
							matched = false;
							break;
						}
					}
					if (!matched) continue;
					return std::pair(n, false);
				}
			}
			return std::nullopt;
		}

		static bool anchor_symbol_parse(formal::lex_node& src) {
			if (src.code() & (anchor_is_symbol | anchor_is_const_symbol)) return true; // \todo predicate target: any encoding using a linear index is already parsed
			if (src.code() & anchor_is_not_symbol) return false; // we already looked at this
			if (src.code() & formal::Error) return false;
			if (auto code = recognize(src)) {
				src.learn(code->second ? anchor_is_symbol : anchor_is_const_symbol, code->first);
				return true;
			}
			src.learn(anchor_is_not_symbol);
			return false;
		}

		static bool global_parse(kuroda::parser<formal::lex_node>::edit_span& tokens, kuroda::parser<formal::lex_node>& grammar)
		{
			if (1 >= tokens.size()) return false; // no parse target

			if (auto target = find_global_parse_target(tokens)) {
				// for now, assume left-associativity in the parse tree
				// need this to handle format-bound subscripts, etc.
				formal::lex_node::force_empty_prefix_postfix_fragments(tokens[target->first]);

				const auto postfix_ub = tokens.size() - 1 - target->first;

				kuroda::parser<formal::lex_node>::symbols stage_prefix(target->first);
				kuroda::parser<formal::lex_node>::symbols stage_suffix(postfix_ub);

				std::unique_ptr<formal::lex_node> dest(tokens[target->first]);
				tokens[target->first] = nullptr;

				if (tokens.size() - 1 > target->first && (target->second & (Infix | Prefix))) {
					const auto suffix_origin = tokens.begin() + target->first + 1;
					std::copy_n(suffix_origin, postfix_ub, stage_suffix.begin());
					std::fill_n(suffix_origin, postfix_ub, nullptr);
					dest.get()->set_postfix(std::move(stage_suffix));
					kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, postfix_ub, target->first + 1);
				}
				if (0 < target->first && (target->second & (Infix | Postfix))) {
					const auto prefix_origin = tokens.begin();
					std::copy_n(prefix_origin, target->first, stage_prefix.begin());
					std::fill_n(prefix_origin, target->first, nullptr);
					dest.get()->set_prefix(std::move(stage_prefix));
					kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, target->first, 0);
					target->first = 0;
				}

				tokens[target->first] = dest.release();

				// recurse the grammar on both prefix and postfix
				if (!tokens[target->first]->prefix().empty()) {
					grammar.complete_parse(tokens[target->first]->prefix());
				}
				if (!tokens[target->first]->postfix().empty()) {
					grammar.complete_parse(tokens[target->first]->postfix());
				}

				// tag isolated placeholder variables here
				if (1 == tokens[target->first]->prefix().size()) {
					if (is_placeholder_syntax_symbol(tokens[target->first]->prefix().front()) || is_symbol_placeholder_syntax_symbol(tokens[target->first]->prefix().front())) {
						tokens[target->first]->prefix().front()->learn(used_as_placeholder);
					}
				};
				if (1 == tokens[target->first]->postfix().size()) {
					if (is_placeholder_syntax_symbol(tokens[target->first]->postfix().front()) || is_symbol_placeholder_syntax_symbol(tokens[target->first]->postfix().front())) {
						tokens[target->first]->postfix().front()->learn(used_as_placeholder);
					}
				};

				// interpreting phrases/clauses is for other grammar rules.
				return true;
			}

			return false;
		}

private:
		static std::optional<std::pair<ptrdiff_t, unsigned int> > find_global_parse_target(const kuroda::parser<formal::lex_node>::edit_span& tokens) {
			enum { trace_parse = 0 };

			std::vector<std::pair<ptrdiff_t, size_t> > symbol_indexes;
			size_t lowest_priority_symbol = std::numeric_limits<size_t>::max();
			ptrdiff_t n = -1;
			for (formal::lex_node* x : tokens) {
				++n;
				// \todo ideally we would let superscripts and subscripts through
				if (!x->prefix().empty()) continue;
				if (!x->postfix().empty()) continue;
				if (!x->fragments().empty()) continue;
				if (x->code() & anchor_is_symbol) {
					symbol_indexes.push_back(std::pair(n, x->offset()));
					if (lowest_priority_symbol > symbol_indexes.back().second) lowest_priority_symbol = symbol_indexes.back().second;
				}
				else if (anchor_symbol_parse(*x)) {
					// invariant violation
					if constexpr (trace_parse) std::cerr << "symbol_catalog::global_parse: anchor_symbol_parse: " << x->to_s() << "\n";

					if (x->code() & anchor_is_symbol) {
						symbol_indexes.push_back(std::pair(n, x->offset()));
						if (lowest_priority_symbol > symbol_indexes.back().second) lowest_priority_symbol = symbol_indexes.back().second;
					}
				}
			}
			if (symbol_indexes.empty()) {
				if constexpr (trace_parse) std::cerr << "symbol_catalog::global_parse: No symbols: " << to_string(tokens) << "\n";
				return std::nullopt;
			}

			const auto& catalog = get();
			std::vector<ptrdiff_t> stage;
			for (const auto& x : symbol_indexes) {
				if (x.second == lowest_priority_symbol) stage.push_back(x.first);
			}
			const auto syntax_code = catalog._symbols[lowest_priority_symbol].second;
			if (syntax_code & Infix) {
				if (1 < stage.size()) {
					// if we don't have one of associativity or transitivity, this is an error
					bool already_error = false;
					for (const auto x : stage) {
						if (tokens[x]->code() & formal::Error) {
							already_error = true;
							break;
						}
					}
					if (!already_error) {
						tokens[stage.front()]->record_error("infix symbol with no associativity or transitivity: " + tokens[stage.front()]->to_s());
					}
					for (const auto x : stage) {
						tokens[x]->learn(formal::Error);
					}
				}
			}
			if (0 == stage.front()) {
				if (!(syntax_code & Prefix)) {
					if (tokens[stage.front()]->prefix().empty()) {
						if (!(tokens[stage.front()]->code() & formal::Error)) {
							tokens[stage.front()]->record_error("non-prefix symbol has no argument to left: " + tokens[stage.front()]->to_s());
						}
					}
				}
			}
			if (tokens.size() - 1 == stage.back()) {
				if (!(syntax_code & Postfix)) {
					if (tokens[stage.back()]->postfix().empty()) {
						if (!(tokens[stage.back()]->code() & formal::Error)) {
							tokens[stage.back()]->record_error("non-postfix symbol has no argument to right: " + tokens[stage.back()]->to_s());
						}
					}
				}
			}

			return std::pair(stage.front(), syntax_code);
		}
	};

	static bool needs_grouping_parens(const formal::lex_node& node) {
		return (node.code() & symbol_catalog::anchor_is_symbol) && 1 >= node.offset();
	}

	// Consumes the intended postcondition of symbol_catalog::anchor_symbol_parse: exactly
	// one of anchor_is_symbol / anchor_is_const_symbol / anchor_is_not_symbol set.
	// require_contract=true throws on a node that never went through anchor_symbol_parse
	// (e.g. a freshly constructed tree); the default treats it as a non-match.  The
	// contract test runs first, so enforcement is uniform across all node shapes.
	template<bool require_contract = false>
	static bool is_unary_syntactical_entailment(const formal::lex_node& node) {
		// we need syntactical entailment, not a Gentzen axiom (both use the same symbol)
		if (node.code() & symbol_catalog::anchor_is_symbol) {
			if (1 != node.offset()) return false;
		} else if (node.code() & (symbol_catalog::anchor_is_const_symbol | symbol_catalog::anchor_is_not_symbol)) return false;
		else {
			// precondition failure
			if constexpr (require_contract) throw std::logic_error(std::string("symbol_catalog::anchor_symbol_parse not called on node ") + node.to_s());
			else return false;
		}
		if (!is_strict_binary_node(node)) return false;
		if (detect_comma(*node.prefix().front())) return false;	// n-ary hypothesis, not unary
		if (detect_comma(*node.postfix().front())) return false;	// n-ary conclusion is not intuitionistic -- classical logical or, hard to work with
		// we do want the arguments to be truth-valued statements, but the logic is a parameter
		return true;
	}

	// Detects a syntactical entailment expression still in lex_node form: A &#9500; B,
	// or A, B &#9500; C etc. with a comma-fold hypothesis arm.  Deliberately false for
	// a lex_node wrapping a formal::parsed instance (in particular an
	// already-converted syntactical_entailment_2ary); that test precedes the contract
	// trichotomy so converted wrappers, which never go through anchor_symbol_parse,
	// cannot trip the throwing variant.
	// \todo the 3+ arity normal form (anchor with non-empty fragments) is not set up
	// yet; extend the shape tests when it is.
	template<bool require_contract = false>
	static bool is_syntactical_entailment(const formal::lex_node& node) {
		if (node.c_anchor<formal::parsed>()) return false;
		if (node.code() & symbol_catalog::anchor_is_symbol) {
			if (1 != node.offset()) return false;
		} else if (node.code() & (symbol_catalog::anchor_is_const_symbol | symbol_catalog::anchor_is_not_symbol)) return false;
		else {
			// precondition failure
			if constexpr (require_contract) throw std::logic_error(std::string("symbol_catalog::anchor_symbol_parse not called on node ") + node.to_s());
			else return false;
		}
		if (!is_strict_binary_node(node)) return false;
		if (detect_comma(*node.postfix().front())) return false;	// reject non-Intuitionistic syntactical entailments
		return true;
	}

	template<bool require_contract = false>
	static bool is_definition_node(const formal::lex_node& node) {
		// we need syntactical entailment, not a Gentzen axiom (both use the same symbol)
		if (node.code() & symbol_catalog::anchor_is_symbol) {
			if (0 != node.offset()) return false;
		}
		else if (node.code() & (symbol_catalog::anchor_is_const_symbol | symbol_catalog::anchor_is_not_symbol)) return false;
		else {
			// precondition failure
			if constexpr (require_contract) throw std::logic_error(std::string("symbol_catalog::anchor_symbol_parse not called on node ") + node.to_s());
			else return false;
		}
		if (!is_binary_node(node)) return false;
		// this is credibly under-specified
		return true;
	}

	// builds: lhs op rhs as a lex_node tree
	formal::lex_node* make_binary_node(std::string_view op, unsigned long long code, std::unique_ptr<formal::lex_node>&& lhs, std::unique_ptr<formal::lex_node>&& rhs) {
		formal::word* w = new formal::word(op, formal::src_location(), code);
		std::unique_ptr<formal::lex_node> ret(new formal::lex_node(w, code));
		ret->prefix().push_back(nullptr);
		ret->postfix().push_back(nullptr);
		ret->prefix().front() = lhs.release();
		ret->postfix().front() = rhs.release();
		return ret.release();
	}

	struct statement_t : public std::variant<std::shared_ptr<const formal::parsed>, std::shared_ptr<const formal::lex_node>> {
		using base = std::variant<std::shared_ptr<const formal::parsed>, std::shared_ptr<const formal::lex_node>>;
		using base::base;
		using base::operator=;

		explicit statement_t(formal::lex_node*& src) : base(_from_node(src)) {}

		std::string to_s() const {
			return std::visit([](const auto& x) { return x->to_s(); }, *this);
		}

		formal::src_location origin() const {
			return std::visit([](const auto& x) { return x->origin(); }, *this);
		}

		void diagnose(std::vector<std::string>& dest) const {
			return std::visit([&](const auto& x) { return x->diagnose(dest); }, *this);
		}

		std::optional<formal::placeholder_match> get_placeholder_variables() const;

	private:
		static base _from_node(formal::lex_node*& src) {
			if (!src) throw std::logic_error("!src");
			if (auto stage = src->shared_anchor<formal::parsed>()) {
				delete src;
				src = nullptr;
				return stage;
			}
			std::shared_ptr<const formal::lex_node> ret(src);
			src = nullptr;
			return ret;
		}
	};

	using weak_statement_t = std::variant<std::weak_ptr<const formal::parsed>, std::weak_ptr<const formal::lex_node>>;

	bool needs_grouping_parens(const statement_t& stmt) {
		struct visitor {
			bool operator()(const std::shared_ptr<const formal::parsed>&) { return false; }
			bool operator()(const std::shared_ptr<const formal::lex_node>& x) { return needs_grouping_parens(*x); }
		};
		return std::visit(visitor(), stmt);
	}

	static weak_statement_t weaken(const statement_t& src) {
		struct visitor {
			weak_statement_t operator()(const std::shared_ptr<const formal::parsed>& x) { return std::weak_ptr(x); }
			weak_statement_t operator()(const std::shared_ptr<const formal::lex_node>& x) { return std::weak_ptr(x); }
		};
		return std::visit(visitor(), src);
	}

	static statement_t lock(const weak_statement_t& src) {
		struct visitor {
			statement_t operator()(const std::weak_ptr<const formal::parsed>& x) { return x.lock(); }
			statement_t operator()(const std::weak_ptr<const formal::lex_node>& x) { return x.lock(); }
		};
		return std::visit(visitor(), src);
	}

	formal::lex_node* node_from(const statement_t& src) {
		struct visitor {
			auto operator()(const std::shared_ptr<const formal::parsed>& x) { return new formal::lex_node(x); }	// formal::parsed has no code() to punt to
			// propagate code and offset so the wrapper reads like what it wraps (cf. the sinking constructor)
			auto operator()(const std::shared_ptr<const formal::lex_node>& x) { return new formal::lex_node(x, x->code(), x->offset()); }
		};
		return std::visit(visitor(), src);
	}

	// freshly-allocated, non-aliased deep copy of the underlying lex_node tree.
	formal::lex_node* deep_clone_to_lex_node(const statement_t& src) {
		struct visitor {
			formal::lex_node* operator()(const std::shared_ptr<const formal::parsed>& x) {
				if (!x) return nullptr;
				formal::parsed* stage = nullptr;
				x->CopyInto(stage);
				return new formal::lex_node(zaimoni::COW(stage));
			}
			formal::lex_node* operator()(const std::shared_ptr<const formal::lex_node>& x) {
				if (!x) return nullptr;
				return new formal::lex_node(*x);
			}
		};
		return std::visit(visitor(), src);
	}

	bool valid_placeholder_symbol_content(const statement_t& src) {
		struct visitor {
			bool operator()(const std::shared_ptr<const formal::parsed>& x) { return (bool)x; }
			bool operator()(const std::shared_ptr<const formal::lex_node>& x) { return x && !detect_comma(*x); }
		};
		return std::visit(visitor(), src);
	}

	struct fact_database {
		const char * const rationale_oob = "*::rationale(size_t) out of bounds";

		virtual ~fact_database() = default;
		virtual size_t size() const noexcept = 0;
		virtual std::shared_ptr<fact_database> parent() const = 0;
		virtual std::optional<size_t> offset(size_t notation_id) const = 0;
		virtual std::pair<std::pair<statement_t, perl::scalar>, size_t> rationale(size_t n) const = 0;
	};

	// pass by value: the loop re-seats db (manual tail recursion, a stack micro-optimization),
	// so a caller's shared_ptr must not be mutated and const reference cannot work
	template<class T> requires (std::derived_from<T, fact_database> && !std::same_as<T, fact_database>)
	T* find_ancestor(std::shared_ptr<fact_database> db) {
		while (db = db->parent()) {
			if (auto p = dynamic_cast<T*>(db.get())) return p;
		}
		return nullptr;
	}

	template<class T> requires (std::derived_from<T, fact_database> && !std::same_as<T, fact_database>)
	std::vector<T*> find_ancestors(std::shared_ptr<fact_database> db) {
		std::vector<T*> ret;

		while (db = db->parent()) {
			if (auto p = dynamic_cast<T*>(db.get())) ret.push_back(p);
		}
		return ret;
	}

	std::optional<std::pair<std::shared_ptr<fact_database>, size_t> > offset(std::shared_ptr<fact_database> tip, const size_t notation_id) {
		while (tip) {
			if (auto test = tip->offset(notation_id)) return std::pair(tip, *test);
			tip = tip->parent();
		}
		return std::nullopt;
	}

	class axioms : public fact_database, public zaimoni::observed<statement_t> {
	private:
		std::vector<statement_t> _axioms;

		static const constexpr std::string_view str_axiom = std::string_view("axiom");
		static const constexpr std::string_view str_schema = std::string_view("axiom schema");
	public:
		axioms() = default;
		axioms(const axioms&) = delete;
		axioms(axioms&&) = delete;
		axioms& operator=(const axioms&) = delete;
		axioms& operator=(axioms&&) = delete;
		~axioms() = default;

		static std::shared_ptr<axioms> get() {
			static std::shared_ptr<axioms> oaoo;
			if (!oaoo) oaoo = std::shared_ptr<axioms>(new axioms());
			return oaoo;
		}

		// fact_database interface
		size_t size() const noexcept override { return _axioms.size(); }
		std::shared_ptr<fact_database> parent() const noexcept { return nullptr; }

		std::optional<size_t> offset(size_t notation_id) const override {
			if (0 >= notation_id) return std::nullopt;
			if (_axioms.size() < notation_id) return std::nullopt;
			return notation_id - 1;
		}

		std::pair<std::pair<statement_t, perl::scalar>, size_t> rationale(size_t n) const override {
			if (_axioms.size() <= n) throw std::logic_error(rationale_oob);
			return std::pair(std::pair(_axioms[n], str_axiom), n+1);
		}

		// observed interface
		void watched_by(const std::shared_ptr<zaimoni::observer<statement_t>>& src) override {
			// this is inappropriate in many contexts, but ok for our use case
			for (decltype(auto) axiom : _axioms) {
				if (!src->onNext(axiom)) return;
			}

			seen_by(src);
		}
		// end observed interface

		void add_axiom(std::shared_ptr<const formal::parsed> src) {
			enum { trace_parse = 0 };

			if (auto err = src->is_not_legal_axiom(true)) {
				error_report(*src, *err);
				return;
			}
			if (auto err = src->before_add_axiom_handler()) {
				error_report(*src, *err);
				return;
			}
			if constexpr (trace_parse) std::cerr << "axiom: " << src->to_s() << "\n";
			_axioms.push_back(src);

			notify(_axioms.back());
		}

		void add_axiom(std::shared_ptr<const formal::lex_node> src) {
			enum { trace_parse = 0 };

			if (!(formal::Recognized & src->code())) {
				error_report(src->origin(), "axiom not tagged as recognized");
				return;
			}
			// other legality tests are specific to the Gentzen syntax.  A conditional's
			// conclusion may not alter syntax: the parsed statement kinds carry that rule as
			// is_not_legal_axiom(false) (phrase_postfix: "cannot conditionally alter syntax").
			// The hypothesis is an assumption, not an assertion, and is not tested --
			// "_ is a binary infix symbol" is a legal hypothesis, not a legal axiom.
			if (is_unary_syntactical_entailment(*src)) {
				if (auto conclusion = src->postfix().front()->c_anchor<formal::parsed>()) {
					if (auto err = conclusion->is_not_legal_axiom(false)) {
						error_report(src->postfix().front()->origin(), *err);
						return;
					}
				}
			}

			// "before add axiom" tests go here (both validation, and side effects)

			if constexpr (trace_parse) std::cerr << "axiom: " << src->to_s() << "\n";
			_axioms.push_back(src);

			notify(_axioms.back());
		}

		std::optional<std::pair<statement_t, size_t> > disambiguate(const std::string& notation) {
			// notational deduplication check

			// \todo use lex_node::to_scalar when relevant
			ptrdiff_t ub = _axioms.size();
			while (0 <= --ub) {
				if (notation == _axioms[ub].to_s()) return std::pair(_axioms[ub], ub+1);
			}

			return std::nullopt;
		}
	};

	// these synthetic ids are supposed to be in bijection with statement notations actually used,
	// rather than the fact_database class.
	class notation_ids final {
	private:
		size_t _next;
		std::vector<std::pair<size_t, weak_statement_t> > _registry;

		notation_ids() : _next(std::numeric_limits<size_t>::max()) {}
	public:
		notation_ids(const notation_ids&) = delete;
		notation_ids(notation_ids&&) = delete;
		notation_ids& operator=(const notation_ids&) = delete;
		notation_ids& operator=(notation_ids&&) = delete;

		static notation_ids& get() {
			static notation_ids ooao;
			return ooao;
		}

		std::pair<statement_t, size_t> allocate(const statement_t& src) {
			// notational deduplication check
			auto notation = src.to_s();

			// \todo use lex_node::to_scalar when relevant
			ptrdiff_t ub = _registry.size();
			while (0 <= --ub) {
				// an expired entry -- its statement died with a discharged conditional-reasoning
				// scope, or was a temporary -- cannot be deduplicated against, and lock() on it
				// yields a null statement whose to_s() dereferences null.  Prune it instead
				// (swap-with-back is safe here: the back was already visited).
				if (std::visit([](const auto& w) { return w.expired(); }, _registry[ub].second)) {
					_registry[ub].swap(_registry.back());
					_registry.pop_back();
					continue;
				}
				auto s = lock(_registry[ub].second);
				if (notation == s.to_s()) return std::pair(s, _registry[ub].first);
			}

			// axioms are special cases
			if (auto ret = axioms::get()->disambiguate(notation)) return *ret;

			size_t id = _next--;
			_registry.push_back(std::pair(id, weaken(src)));
			return std::pair(src ,id);
		}

		std::optional<statement_t> lookup(size_t notation_id) {
			struct _lookup {
				std::optional<statement_t> operator()(const std::weak_ptr<const formal::parsed>& x) {
					auto ret = x.lock();
					if (ret) return ret;
					return std::nullopt;
				}
				std::optional<statement_t> operator()(const std::weak_ptr<const formal::lex_node>& x) {
					auto ret = x.lock();
					if (ret) return ret;
					return std::nullopt;
				}
			};

			static _lookup ooao;

			size_t n = std::numeric_limits<size_t>::max() - notation_id;
			if (_registry.size() <= n) {
				// axioms are special cases
				if (0 < notation_id && axioms::get()->size() >= notation_id) {
					return axioms::get()->rationale(notation_id - 1).first.first;
				}
				return std::nullopt;
			}

			return std::visit(ooao, _registry[n].second);
		}
	};

//	lifted above class symbol_catalog to allow symbol_catalog::global_parse to use it there
//	static constexpr const unsigned long long used_as_placeholder = (1ULL << 8); // reserve this flag for lex_node

	static_assert(!(used_as_placeholder & formal::Comment));
	static_assert(!(used_as_placeholder & formal::Error));
	static_assert(!(used_as_placeholder & formal::Inert_Token));
	static_assert(!(used_as_placeholder & formal::Tokenized));
	static_assert(!(used_as_placeholder & formal::RequestNormalization));
	static_assert(!(used_as_placeholder & HTMLtag::Entity));
	static_assert(!(used_as_placeholder & HTMLtag::Tag));
	static_assert(!(used_as_placeholder & symbol_catalog::anchor_is_symbol));
	static_assert(!(used_as_placeholder & symbol_catalog::anchor_is_const_symbol));
	static_assert(!(used_as_placeholder & symbol_catalog::anchor_is_not_symbol));

	// covers all four substitution variants: uniform/non-uniform x simultaneous/non-simultaneous
	static constexpr const unsigned long long parsed_substitution = (1ULL << 9); // reserve this flag for lex_node

	static_assert(!(parsed_substitution & formal::Comment));
	static_assert(!(parsed_substitution & formal::Error));
	static_assert(!(parsed_substitution & formal::Inert_Token));
	static_assert(!(parsed_substitution & formal::Tokenized));
	static_assert(!(parsed_substitution & formal::RequestNormalization));
	static_assert(!(parsed_substitution & HTMLtag::Entity));
	static_assert(!(parsed_substitution & HTMLtag::Tag));
	static_assert(!(parsed_substitution & symbol_catalog::anchor_is_symbol));
	static_assert(!(parsed_substitution & symbol_catalog::anchor_is_const_symbol));
	static_assert(!(parsed_substitution & symbol_catalog::anchor_is_not_symbol));
	static_assert(!(parsed_substitution & used_as_placeholder));

	struct tag_placeholder_syntax {
		void operator()(formal::lex_node* x) {
			if (is_placeholder_syntax_symbol(x)) {
				x->learn(used_as_placeholder);
				return;
			}
			if (is_symbol_placeholder_syntax_symbol(x)) {
				x->learn(used_as_placeholder);
				return;
			}
			// when looking into parsed statements, the undefined terminology regarding placeholder syntax symobls, is problematic
			// A is a placeholder syntax symbol: do not tag A (we bypass this when entering hard-coded axioms)
			// _ is a placeholder syntax symbol: do tag _, which is a symbol placeholder syntax symbol.  Result cannot be an axiom.
			// _ is a symbol placeholder syntax symbol: do not tag the axiom, but do tag when used as a hypothesis

			(*this)(x->prefix());
			(*this)(x->infix());
			(*this)(x->postfix());
			(*this)(x->fragments());
		}
		void operator()(kuroda::parser<formal::lex_node>::symbols& x) {
			if (!x.empty()) for (decltype(auto) arg : x) (*this)(arg);
		}
		void operator()(std::vector<kuroda::parser<formal::lex_node>::symbols>& x) {
			if (!x.empty()) for (decltype(auto) arg : x) (*this)(arg);
		}
	};

	// preconditions: tree is freshly-allocated and not aliased.  Recursively walks the
	// tree, producing lex_node** handles into the parent slot for each placeholder
	// occurrence and appending them to dest.groups (merged by string equality on
	// to_scalar()).  Untagged placeholder-syntax symbols are tagged on the fly.
	// Parsed-arm anchors delegate to parsed::get_placeholder_variables(dest), which
	// must attach any freshly-allocated sub-trees to dest.aux_trees.
	struct collect_placeholder_handles {
		formal::placeholder_match* dest_ = nullptr;

		void note(formal::lex_node** slot) {
			if (!slot || !*slot || !dest_) return;
			perl::scalar key = (*slot)->to_scalar();
			const auto key_view = key.view();
			for (decltype(auto) g : dest_->groups) {
				if (g.first.view() == key_view) {
					g.second.emplace_back(slot);
					return;
				}
			}
			std::vector<formal::placeholder_handle> initial;
			initial.emplace_back(slot);
			dest_->groups.emplace_back(std::move(key), std::move(initial));
		}

		void operator()(kuroda::parser<formal::lex_node>::symbols& x) {
			enum { trace_parse = 0,
				   backstop_placeholder = 0};

			if (x.empty()) return;
			for (size_t i = 0; i < x.size(); ++i) {
				formal::lex_node** slot = &(x[i]);
				if (!*slot) continue;
				if ((*slot)->code() & used_as_placeholder) {
					note(slot);
					continue;
				}
				if constexpr (backstop_placeholder) {
					if (is_placeholder_syntax_symbol(*slot) || is_symbol_placeholder_syntax_symbol(*slot)) {
						// if this were to process, some other inference rule isn't doing its job
						if constexpr (trace_parse) std::cerr << "collect_placeholder_handles: tagging " << (*slot)->to_s() << "\n";
						(*slot)->learn(used_as_placeholder);
						note(slot);
						continue;
					}
				}
				if (auto parsed_ptr = (*slot)->shared_anchor<formal::parsed>()) {
					if (dest_) parsed_ptr->get_placeholder_variables(*dest_);
					continue;
				}
				(*this)(**slot);
			}
		}

		void operator()(std::vector<kuroda::parser<formal::lex_node>::symbols>& x) {
			if (x.empty()) return;
			for (decltype(auto) arg : x) (*this)(arg);
		}

		void operator()(formal::lex_node& x) {
			(*this)(x.prefix());
			(*this)(x.infix());
			(*this)(x.postfix());
			(*this)(x.fragments());
		}

		// primary entry: walks dest.tree, appending into dest.
		void operator()(formal::placeholder_match& dest) {
			dest_ = &dest;
			if (dest.tree) (*this)(*dest.tree);
			dest_ = nullptr;
		}

		// helper for parsed-arm overrides: walks an externally-owned root, appending into dest.
		// caller is responsible for keeping `root` alive (typically by attaching to dest.aux_trees).
		void walk_into(formal::lex_node& root, formal::placeholder_match& dest) {
			auto* saved = dest_;
			dest_ = &dest;
			(*this)(root);
			dest_ = saved;
		}

		static std::optional<formal::placeholder_match> operator()(const formal::lex_node* x) {
			if (!x) return std::nullopt;
			formal::placeholder_match match;
			match.tree.reset(new formal::lex_node(*x));
			collect_placeholder_handles walker;
			walker(match);
			if (match.groups.empty()) return std::nullopt;
			return match;
		}
	};

	// builds [ <comma-fold of destinations> \ <comma-fold of sources> ] as strictly
	// binary lex_nodes.  Sets parsed_substitution on the root.  destinations and
	// sources must be the same nonzero size; ownership of all input pointers is
	// taken on success.
	formal::lex_node* make_substitution_node(std::vector<formal::lex_node*> destinations,
		std::vector<formal::lex_node*> sources)
	{
		if (destinations.size() != sources.size()) throw std::logic_error("make_substitution_node: arity mismatch");
		if (destinations.empty()) throw std::logic_error("make_substitution_node: empty");

		auto comma_fold = [](std::vector<formal::lex_node*>& v) -> std::unique_ptr<formal::lex_node> {
			std::unique_ptr<formal::lex_node> acc(v.back());
			v.back() = nullptr;
			ptrdiff_t i = v.size() - 1;
			while (0 <= --i) {
				std::unique_ptr<formal::lex_node> lhs(v[i]);
				v[i] = nullptr;
				acc.reset(make_binary_node(comma, 0, std::move(lhs), std::move(acc)));
			}
			return acc;
		};

		std::unique_ptr<formal::lex_node> dest_tree = comma_fold(destinations);
		std::unique_ptr<formal::lex_node> src_tree = comma_fold(sources);

		std::unique_ptr<formal::lex_node> backslash(make_binary_node(std::string_view("/"), 0,
			std::move(dest_tree), std::move(src_tree)));

		// wrap with [ ... ]
		formal::word* l_bracket = new formal::word(std::string_view("["), formal::src_location(), 0);
		l_bracket->learn(formal::Inert_Token);
		formal::word* r_bracket = new formal::word(std::string_view("]"), formal::src_location(), 0);
		r_bracket->learn(formal::Inert_Token);
		std::unique_ptr<formal::lex_node> root(new formal::lex_node(l_bracket, 0));
		root->infix().push_back(backslash.release());
		root->set_null_post_anchor(r_bracket);
		root->learn(parsed_substitution);
		return root.release();
	}

	// inverse of make_substitution_node.  Faithful only when no destination or source is
	// itself a comma-anchored strict binary node (holds for placeholder variables, the
	// current use case).  Returned pointers are views into src.
	std::optional<std::pair<std::vector<const formal::lex_node*>, std::vector<const formal::lex_node*> > >
	decode_substitution_node(const formal::lex_node& src)
	{
		if (!(src.code() & parsed_substitution)) return std::nullopt;
		if (1 != src.infix().size()) return std::nullopt;
		const auto slash = src.infix().front();
		if (!slash || !is_strict_binary_node(*slash)) return std::nullopt;
		const auto op = slash->c_anchor<formal::word>();
		if (!op || "/" != op->value()) return std::nullopt;

		auto comma_unfold = [](const formal::lex_node* tree) {
			std::vector<const formal::lex_node*> ret;
			while (tree && detect_comma(*tree)) {
				if (   is_lfold_rfold_gassoc_node(*tree)
					&& are_fragments_superficially_parsed(*tree)) {
					for (decltype(auto) x : tree->fragments()) ret.push_back(x.front());
					return ret;
				}
				// typical recursive right-fold semantics
				if (!is_strict_binary_node(*tree)) break;	// non-conforming comma node: take it whole
				ret.push_back(tree->prefix().front());
				tree = tree->postfix().front();
			}
			ret.push_back(tree);
			return ret;
		};

		auto dests = comma_unfold(slash->prefix().front());
		auto srcs = comma_unfold(slash->postfix().front());
		if (dests.size() != srcs.size()) return std::nullopt;
		return std::pair(std::move(dests), std::move(srcs));
	}

	// with_token_grammar is a pure routing switch: it prefixes the TokenGrammar stage,
	// whose enforcement terminals (argument_enforcer, is_wff) GentzenGrammar lacks.  The
	// caller selects it from what is being substituted *from*: a source that is a
	// symbol-placeholder-syntax variable with used_as_placeholder set (e.g. _ -> &#9500;
	// in "modus ponens for _") needs enforcement to re-run on the substituted run.
	// Scope <true> to loose leaf runs (a symbols container such as the := prefix), not
	// structured trees: TokenGrammar's captures are not idempotent on parsed input
	// (cf. the parse_inside_balanced note).
	template<bool with_token_grammar = false>
	void reparse(kuroda::parser<formal::lex_node>::symbols& x) {
		if (1 < x.size()) {
			const auto prior_errors = Errors.count();
			if constexpr (with_token_grammar) {
				x = TokenGrammar().apply(x);
				if (prior_errors < Errors.count()) return;
				TokenGrammar().complete_parse(x);
				if (prior_errors < Errors.count()) return;
			}
			x = GentzenGrammar().apply(x);
			if (prior_errors == Errors.count()) GentzenGrammar().complete_parse(x);
		}
	}

	template<bool with_token_grammar = false>
	void reparse(std::vector<kuroda::parser<formal::lex_node>::symbols>& fragments) {
		for (decltype(auto) frag : fragments) reparse<with_token_grammar>(frag);
	}

	// Post-substitution reparse (fork B): replacing an operator-position placeholder can
	// leave a loose infix run (e.g. (P _ Q) -> a paren over [P, &#9500;, Q]) that must
	// fold into a proper binary node.  Recurses deepest-first, then reparses every
	// container (prefix/infix/postfix/fragments) via reparse(symbols&), mirroring
	// parse_inside_balanced; an already-folded (size 1) container is a no-op there.
	template<bool with_token_grammar = false>
	void reparse(formal::lex_node& node) {
		for (decltype(auto) x : node.prefix()) if (x) reparse<with_token_grammar>(*x);
		reparse<with_token_grammar>(node.prefix());
		for (decltype(auto) x : node.infix()) if (x) reparse<with_token_grammar>(*x);
		reparse<with_token_grammar>(node.infix());
		for (decltype(auto) x : node.postfix()) if (x) reparse<with_token_grammar>(*x);
		reparse<with_token_grammar>(node.postfix());
		for (decltype(auto) frag : node.fragments()) {
			for (decltype(auto) x : frag) if (x) reparse<with_token_grammar>(*x);
			reparse<with_token_grammar>(frag);
		}
	}

	// a symbol-placeholder-syntax variable that has been used as a placeholder.  Substituting
	// *from* one of these changes the parse of the containers it sat in (a symbol arrives
	// where a variable was), so those containers need reparsing; substituting placeholder
	// to placeholder does not.  Also the caller-side diagnosis for reparse<true>.
	bool is_symbol_placeholder_variable(const formal::lex_node& source) {
		return (source.code() & used_as_placeholder) && is_symbol_placeholder_syntax_symbol(&source);
	}

	// A decoded uniform substitution [d1, ..., dn \ s1, ..., sn] whose destinations have been
	// prepared *once*, here, rather than at each place that copies one in.  Every consumer
	// copies a destination with new lex_node(*view()[i].second) -- the handle loops, the
	// bare-placeholder cases, phrase_postfix's subject -- and each copy needs its own bit
	// flags, since they update independently once reparsed.  So the prototypes are owned
	// here (one allocation per entry, not per occurrence) and the consumers' copies inherit
	// the preparation without any of them having to remember it.  Consumers take
	// const prepared_rename&, not the raw view, and decode is the only way to make one:
	// an unprepared rename cannot be passed.
	// The preparation: symbol_catalog::anchor_symbol_parse will short-circuit and do nothing
	// whatsoever, if we don't clear these flags on a destination the catalog recognizes.
	class prepared_rename final {
	public:
		using view_t = std::vector<std::pair<perl::scalar, const formal::lex_node*> >;
	private:
		std::vector<std::unique_ptr<formal::lex_node> > _owned;	// view()'s pointers target these
		view_t _view;

		prepared_rename() = default;
	public:
		prepared_rename(const prepared_rename&) = delete;	// _view would alias the source's prototypes
		prepared_rename(prepared_rename&&) = default;
		prepared_rename& operator=(const prepared_rename&) = delete;
		prepared_rename& operator=(prepared_rename&&) = default;
		~prepared_rename() = default;

		const view_t& view() const { return _view; }

		// nullopt when subst is not a substitution node, any source is not a placeholder-family
		// variable, or any destination is null.  symbol_sources, when requested, receives the
		// names of the sources that are symbol-placeholder variables: replacing one of those
		// changes the parse of the container it sat in.
		static std::optional<prepared_rename> decode(const formal::lex_node& subst, std::vector<perl::scalar>* symbol_sources = nullptr)
		{
			enum { test_preconditions = 0 };

			const auto decoded = decode_substitution_node(subst);
			if (!decoded) return std::nullopt;
			const auto& dests = decoded->first;
			const auto& srcs = decoded->second;

			prepared_rename ret;
			ptrdiff_t i = srcs.size();
			while (0 <= --i) {
				if (!srcs[i] || !(srcs[i]->code() & used_as_placeholder)) return std::nullopt;
				if (!dests[i]) return std::nullopt;

				std::unique_ptr<formal::lex_node> prototype(new formal::lex_node(*dests[i]));
				if (symbol_catalog::recognize(*prototype)) {
					if constexpr (test_preconditions) {
						if (symbol_catalog::anchor_is_not_symbol & prototype->code()) {
							std::cerr << relevant_stacktrace() << "\n";
							throw std::logic_error("symbol tagged as non-symbol: " + prototype->to_s());
						}
					}
					prototype->forget(symbol_catalog::anchor_is_symbol | symbol_catalog::anchor_is_const_symbol | symbol_catalog::anchor_is_not_symbol);
				} else if constexpr (test_preconditions) {
					if ((symbol_catalog::anchor_is_symbol | symbol_catalog::anchor_is_const_symbol) & prototype->code()) {
						std::cerr << relevant_stacktrace() << "\n";
						throw std::logic_error("non-symbol tagged as symbol: " + prototype->to_s());
					}
				}

				ret._view.push_back(std::pair(srcs[i]->to_scalar(), prototype.get()));
				ret._owned.push_back(std::move(prototype));
				if (symbol_sources && is_symbol_placeholder_variable(*srcs[i])) symbol_sources->push_back(srcs[i]->to_scalar());
			}
			return ret;
		}
	};

	// step-4 diagnostic walks (Notes/GentzenW-defined-notations.md): driven through
	// lex_node::recursive_rewrite with a self-recursing op.  recursive_rewrite is one level
	// and only applies op where the test passes, so the test has to be *deep* (this node or
	// any descendant has a pending message): a shallow test never descends through a clean
	// parent, and a deferred error two levels down is silently missed.  The deep test also
	// remains the termination guard -- op clears the node's own message and recurses, which
	// clears every descendant's, so recursive_rewrite's re-visit of that node (it includes
	// self) fails the test and stops.  That argument requires these to traverse exactly the
	// edges recursive_rewrite does (in particular: the lex anchor only when there are no
	// fragments, and never a parsed anchor); an extra edge here would never be cleared.
	template<bool errors_only>
	static bool _has_pending(const formal::lex_node& x) {
		if (x.last_error()) return true;
		if constexpr (!errors_only) {
			if (x.last_warning()) return true;
		}
		for (decltype(auto) y : x.prefix()) if (y && _has_pending<errors_only>(*y)) return true;
		for (decltype(auto) y : x.infix()) if (y && _has_pending<errors_only>(*y)) return true;
		for (decltype(auto) y : x.postfix()) if (y && _has_pending<errors_only>(*y)) return true;
		if (!x.fragments().empty()) {
			for (decltype(auto) frag : x.fragments()) {
				for (decltype(auto) y : frag) if (y && _has_pending<errors_only>(*y)) return true;
			}
		} else if (auto y = x.c_anchor<formal::lex_node>()) {
			if (_has_pending<errors_only>(*y)) return true;
		}
		if (auto y = x.c_post_anchor<formal::lex_node>()) {
			if (_has_pending<errors_only>(*y)) return true;
		}
		return false;
	}

	bool has_pending_diagnostic(const formal::lex_node& x) { return _has_pending<false>(x); }

	// for recognizer gates: a deferred error anywhere beneath src must stop recognition,
	// since building a formal::parsed over it would put the message out of the walks' reach
	bool has_pending_error(const formal::lex_node& x) { return _has_pending<true>(x); }

	// flush every pending message in the subtree; run immediately before an error-count
	// test so the count reflects the deferred diagnostics
	void report_diagnostics(formal::lex_node& root) {
		std::function<void(formal::lex_node&)> op = [&op](formal::lex_node& x) {
			x.report_error();
			x.report_warning();
			x.recursive_rewrite(has_pending_diagnostic, op);
		};
		root.recursive_rewrite(has_pending_diagnostic, op);
	}

	void report_diagnostics(const kuroda::parser<formal::lex_node>::sequence& src) {
		for (decltype(auto) x : src) report_diagnostics(*x);
	}

	// cancel every pending message in the subtree (e.g. the LHS of :=)
	void discard_diagnostics(formal::lex_node& root) {
		std::function<void(formal::lex_node&)> op = [&op](formal::lex_node& x) {
			x.discard_error();
			x.discard_warning();
			x.recursive_rewrite(has_pending_diagnostic, op);
		};
		root.recursive_rewrite(has_pending_diagnostic, op);
	}

	std::optional<formal::placeholder_match> statement_t::get_placeholder_variables() const {
		struct visitor {
			std::optional<formal::placeholder_match> operator()(const std::shared_ptr<const formal::parsed>& x) const {
				return x->get_placeholder_variables();
			}
			std::optional<formal::placeholder_match> operator()(const std::shared_ptr<const formal::lex_node>& x) const {
				if (!x) return std::nullopt;
				formal::placeholder_match match;
				match.tree.reset(new formal::lex_node(*x));
				collect_placeholder_handles walker;
				walker(match);
				if (match.groups.empty()) return std::nullopt;
				return match;
			}
		};
		return std::visit(visitor(), *this);
	}

	class syntactical_entailment_2ary final : public formal::parsed {
	private:
		statement_t hypothesis_1;
		statement_t hypothesis_2;
		statement_t conclusion;

		syntactical_entailment_2ary(formal::lex_node*& hyp_1, formal::lex_node*& hyp_2, formal::lex_node*& con)
			: hypothesis_1(hyp_1), hypothesis_2(hyp_2), conclusion(con) {}

		syntactical_entailment_2ary(formal::lex_node*& hyp_1, formal::lex_node*& hyp_2, const statement_t& con)
			: hypothesis_1(hyp_1), hypothesis_2(hyp_2), conclusion(con) {}
	public:
		syntactical_entailment_2ary() = default;
		syntactical_entailment_2ary(const syntactical_entailment_2ary&) = default;
		syntactical_entailment_2ary(syntactical_entailment_2ary&&) = default;
		syntactical_entailment_2ary& operator=(const syntactical_entailment_2ary&) = default;
		syntactical_entailment_2ary& operator=(syntactical_entailment_2ary&&) = default;
		~syntactical_entailment_2ary() = default;

		std::unique_ptr<parsed> clone() const override { return std::unique_ptr<formal::parsed>(new syntactical_entailment_2ary(*this)); }
		void CopyInto(formal::parsed*& dest) const override {
			if (dest) {
				if (auto x = dynamic_cast<syntactical_entailment_2ary*>(dest)) {
					*x = *this;
					return;
				}
				delete dest;
			}
			dest = new syntactical_entailment_2ary(*this);
		}
		void MoveInto(formal::parsed*& dest) override {
			if (dest) {
				if (auto x = dynamic_cast<syntactical_entailment_2ary*>(dest)) {
					*x = std::move(*this);
					return;
				}
				delete dest;
			}
			dest = new syntactical_entailment_2ary(*this);
		}

		formal::src_location origin() const override { return hypothesis_1.origin(); }

		std::string to_s() const override {
			std::vector<perl::scalar> ret(5);
			ret[0] = hypothesis_1.to_s();
			ret[1] = comma;
			ret[2] = hypothesis_2.to_s();
			ret[3] = std::string_view("&#9500;");
			ret[4] = conclusion.to_s();

			// use grouping parentheses on the hypotheses, and conclusion, if needed
			unsigned int code = 0;
			if (needs_grouping_parens(hypothesis_1)) code |= 1U;
			if (needs_grouping_parens(hypothesis_2)) code |= 2U;
			if (needs_grouping_parens(conclusion)) code |= 4U;
			if (0 < code) {
				std::string l_paren("(");
				std::string r_paren(")");
				if (1 & code) ret[0] = l_paren + std::string(ret[0].view()) + r_paren;
				if (2 & code) ret[2] = l_paren + std::string(ret[2].view()) + r_paren;
				if (4 & code) ret[4] = l_paren + std::string(ret[4].view()) + r_paren;
			}

			return join(ret, std::string_view(" "));
		}

		void diagnose(std::vector<std::string>& dest) const override {
			hypothesis_1.diagnose(dest);
			hypothesis_2.diagnose(dest);
			conclusion.diagnose(dest);
		}

		void get_placeholder_variables(formal::placeholder_match& dest) const override {
			std::unique_ptr<formal::lex_node> h1(deep_clone_to_lex_node(hypothesis_1));
			std::unique_ptr<formal::lex_node> h2(deep_clone_to_lex_node(hypothesis_2));
			std::unique_ptr<formal::lex_node> c(deep_clone_to_lex_node(conclusion));
			if (!h1 || !h2 || !c) return;

			std::unique_ptr<formal::lex_node> lhs(make_binary_node(comma, 0, std::move(h1), std::move(h2)));
			std::unique_ptr<formal::lex_node> tree(make_binary_node(std::string_view("&#9500;"), 0,
				std::move(lhs), std::move(c)));

			formal::lex_node& root = *tree;
			dest.aux_trees.push_back(std::move(tree));
			collect_placeholder_handles().walk_into(root, dest);
		}

		// unclear if following belong in a sub-interface
		std::optional<perl::scalar> is_not_legal_axiom(bool unconditional) const override { return std::nullopt; }
		std::optional<perl::scalar> before_add_axiom_handler() const override { return std::nullopt; }

		static bool can_construct_hypothesis(const formal::lex_node& lhs) {
			// we are checking for A, B
			if (!detect_comma(lhs)) return false;
			if (!is_strict_binary_node(lhs)) return false;

			if (detect_comma(*lhs.prefix().front())) return false;
			if (detect_comma(*lhs.postfix().front())) return false;

			return true;
		}

		static bool can_construct(const formal::lex_node& src) {
			// we are checking for A, B &#9500; C

			if (!(src.code() & symbol_catalog::anchor_is_symbol)) return false;
			if (1 != src.offset()) return false;	// need &#9500; ; only higher priority symbol is :=
			if (!is_strict_binary_node(src)) return false;

			if (detect_comma(*src.postfix().front())) return false;

			return can_construct_hypothesis(*src.prefix().front());
		}

		static syntactical_entailment_2ary* construct(formal::lex_node& src) {
			if (formal::Error & src.code()) return nullptr;
			if (has_pending_error(src)) return nullptr;	// a pending (deferred) error, at any depth, also stops recognition
			if (!can_construct(src)) return nullptr;

			decltype(auto) lhs = *src.prefix().front();

			unsigned int err = 0;
			// if grouping parentheses are actually needed, they'll be restored by to_s()
			if (0 > remove_grouping_parentheses(lhs.prefix().front(), not_detect_comma)) {
				err |= 1U;
				error_report(*lhs.prefix().front(), "hypothesis arity would be increased");
			}
			if (0 > remove_grouping_parentheses(lhs.postfix().front(), not_detect_comma)) {
				err |= 2U;
				error_report(*lhs.postfix().front(), "hypothesis arity would be increased");
			}
			if (0 > remove_grouping_parentheses(src.postfix().front(), not_detect_comma)) {
				err |= 4U;
				error_report(*src.postfix().front(), "conclusion arity would be increased");
			}
			if (err) {
				src.learn(formal::Error);
				return nullptr;
			}

			// tag placeholder-syntax symbols now
			tag_placeholder_syntax tag_this;
			tag_this(lhs.prefix().front());
			tag_this(lhs.postfix().front());
			tag_this(src.postfix().front());

			return new syntactical_entailment_2ary(lhs.prefix().front(), lhs.postfix().front(), src.postfix().front());
		}

		static syntactical_entailment_2ary* construct(formal::lex_node& lhs, const statement_t& conclusion) {
			if (!can_construct_hypothesis(lhs)) return nullptr;
			if (!valid_placeholder_symbol_content(conclusion)) return nullptr;

			return new syntactical_entailment_2ary(lhs.prefix().front(), lhs.postfix().front(), conclusion);
		}

		// returns an empty unique_ptr when no match (wrong shape, parsed-arm hypothesis, or
		// hypothesis_idx out of range).  On match, returns a freshly-allocated [d1, d2 \ s1, s2]
		// lex_node tree tagged with parsed_substitution.  hypothesis_idx selects which hypothesis
		// pattern to match against; hypotheses whose entire content is a single placeholder
		// (e.g. hypothesis_2 = P in modus ponens) silently yield an empty result.
		std::unique_ptr<formal::lex_node> get_uniform_simultaneous_substitution(const statement_t& candidate, size_t hypothesis_idx) const {
			if (1 < hypothesis_idx) return nullptr;

			// we do not yet want to adjust solely for a placeholder variable
			auto cand_node = std::get_if<std::shared_ptr<const formal::lex_node>>(&candidate);
			if (!cand_node || !*cand_node) return nullptr;	// we don't handle parsed-arm yet
			const formal::lex_node& cand_ref = **cand_node;
			if (cand_ref.code() & used_as_placeholder) return nullptr;	// unhandled
			// \todo do we want a parsing invariant test here
			if (!(cand_ref.code() & symbol_catalog::anchor_is_symbol)) return nullptr;	// reject what the prototype isn't looking for
			if (!is_strict_binary_node(cand_ref)) return nullptr;	// reject what the prototype isn't looking for

			const statement_t& hyp = (0 == hypothesis_idx) ? hypothesis_1 : hypothesis_2;

			// 7.1: parsed-arm hypothesis is unsupported for now
			auto hyp_node = std::get_if<std::shared_ptr<const formal::lex_node>>(&hyp);
			if (!hyp_node || !*hyp_node) return nullptr;
			const formal::lex_node& hyp_ref = **hyp_node;
			if (!(hyp_ref.code() & symbol_catalog::anchor_is_symbol)) return nullptr;
			if (!is_strict_binary_node(hyp_ref)) return nullptr;

			// check for same kind of binary node
			if (hyp_ref.offset() != cand_ref.offset()) return nullptr;
			// placeholder variables match anything (the symbol-syntax ones ultimately won't work here)
			if (!(hyp_ref.prefix().front()->code() & used_as_placeholder)) return nullptr;	// unhandled
			if (!(hyp_ref.postfix().front()->code() & used_as_placeholder)) return nullptr;	// unhandled

			// while we do not want to record the identity substitution, that does document that this is a relevant
			// inference rule for the candidate.

			std::unique_ptr<formal::lex_node> dest_pre(new formal::lex_node(*cand_ref.prefix().front()));
			std::unique_ptr<formal::lex_node> dest_post(new formal::lex_node(*cand_ref.postfix().front()));
			std::unique_ptr<formal::lex_node> src_pre(new formal::lex_node(*hyp_ref.prefix().front()));
			std::unique_ptr<formal::lex_node> src_post(new formal::lex_node(*hyp_ref.postfix().front()));

			tag_placeholder_syntax tag_this;
			tag_this(dest_pre.get());
			tag_this(dest_post.get());

			std::vector<formal::lex_node*> destinations;
			std::vector<formal::lex_node*> sources;
			destinations.reserve(2);
			sources.reserve(2);
			destinations.push_back(dest_pre.release());
			destinations.push_back(dest_post.release());
			sources.push_back(src_pre.release());
			sources.push_back(src_post.release());

			return std::unique_ptr<formal::lex_node>(make_substitution_node(std::move(destinations), std::move(sources)));
		}

		const statement_t& hypothesis(size_t n) const noexcept { return (0 == n) ? hypothesis_1 : hypothesis_2; }
		const statement_t& consequent() const noexcept { return conclusion; }

		// Uniform substitution per a parsed_substitution node [d1, ... \ s1, ...]: each
		// si (which must be a placeholder-family variable) is replaced by di throughout
		// the rule.  Destinations may be placeholder variables, symbols, or statement
		// subtrees; the combinatoric-explosion guard lives at the call sites (menu
		// enumeration keeps its restrictions; directed instantiation uses this path).
		// Formally an undefined operation (implemented in C++ this iteration).
		// Returns null if the node does not decode, any si is not a placeholder-family
		// variable, any di is null, or a hypothesis/conclusion delegates to a
		// parsed-arm sub-tree (substitution would not propagate there; see
		// placeholder_match::aux_trees).
		std::unique_ptr<syntactical_entailment_2ary> apply_uniform_substitution(const formal::lex_node& subst) const
		{
			const auto rename = prepared_rename::decode(subst);
			if (!rename) return nullptr;
			return apply_uniform_substitution(*rename);
		}

		// rename core: prepared_rename::decode has already validated that each source is a
		// placeholder-family variable and each destination non-null.  Shared with the
		// deep parsed-arm applier so a nested definiens 2ary substitutes through the
		// same path.  Reparses each arm (fork B), so an operator-position substitution
		// such as (P _ Q) -> (P &#9500; Q) folds correctly.
		std::unique_ptr<syntactical_entailment_2ary> apply_uniform_substitution(const prepared_rename& rename) const
		{
			auto instantiate = [&](const statement_t& src) -> formal::lex_node* {
				std::unique_ptr<formal::lex_node> stage(deep_clone_to_lex_node(src));
				if (!stage) return nullptr;

				// a bare placeholder variable has no parent slot for a handle
				if (stage->code() & used_as_placeholder) {
					const auto key = stage->to_scalar();
					for (decltype(auto) x : rename.view()) {
						if (x.first.view() == key.view()) return new formal::lex_node(*x.second);
					}
					return stage.release();
				}

				formal::placeholder_match match;
				match.tree = std::move(stage);
				collect_placeholder_handles()(match);
				if (!match.aux_trees.empty()) return nullptr;	// nested parsed-arm descent still unhandled
				for (decltype(auto) g : match.groups) {
					const formal::lex_node* to = nullptr;
					for (decltype(auto) x : rename.view()) {
						if (x.first.view() == g.first.view()) {
							to = x.second;
							break;
						}
					}
					if (!to) continue;
					for (decltype(auto) h : g.second) h.replace(new formal::lex_node(*to));	// to is a prepared_rename prototype
				}
				reparse(*match.tree);
				// the arms are about to go behind a formal::parsed membrane, out of the
				// diagnostic walks' reach; an arm is never a definiendum, so nothing
				// here is cancellable -- flush now
				report_diagnostics(*match.tree);

				// same normalization as construct: an arm that was an unparseable group at
				// schema time -- (P _ Q) -- parses once the symbol arrives, and its grouping
				// parentheses are then redundant.  Left in place they change the notation
				// ("( P &#9500; Q )" vs "(P &#9500; Q)"), which breaks notational lookup
				// against the configured axiom.  to_s() restores any that are needed.
				formal::lex_node* ret = match.tree.release();
				if (0 > remove_grouping_parentheses(ret, not_detect_comma)) {
					error_report(*ret, "arity would be increased");
					delete ret;
					return nullptr;
				}
				return ret;
			};

			formal::lex_node* h1 = instantiate(hypothesis_1);
			formal::lex_node* h2 = instantiate(hypothesis_2);
			formal::lex_node* c = instantiate(conclusion);
			if (!h1 || !h2 || !c) {
				delete h1;
				delete h2;
				delete c;
				return nullptr;
			}
			return std::unique_ptr<syntactical_entailment_2ary>(new syntactical_entailment_2ary(h1, h2, c));
		}

		// requires gentzen::symbol_catalog::global_parse to have run first, which is a global_rewriter
		// so doesn't process as a reformat_to_terminal
		static bool recognize(formal::lex_node*& src) {
			enum { trace_parse = 0 };

			std::unique_ptr<formal::parsed> test(gentzen::syntactical_entailment_2ary::construct(*src));
			if (test) {
				if constexpr (trace_parse) std::cerr << "syntactical_entailment_2ary::recognize: processing\n";
				std::unique_ptr<formal::lex_node> relay(new formal::lex_node(std::shared_ptr<const formal::parsed>(test.release())));
				delete src;
				src = relay.release();
				return true;
			}
			return false;
		}

		static bool global_parse(kuroda::parser<formal::lex_node>::edit_span& tokens, kuroda::parser<formal::lex_node>& grammar) {
			ptrdiff_t n = tokens.size();
			bool ret = false;
			while (0 <= --n) {
				if (recognize(tokens[n])) ret = true;
			}
			return ret;
		}
	};

	// functional analog to syntactical_entailment_2ary::global_parse
	// schedule this global_parse after syntactical_entailment_2ary::global_parse
	struct tag_syntactical_entailment {
		static bool can_recognize(const formal::lex_node& src) {
			// we are checking for A &#9500; B or A, B, C &#9500; D

			if (!(src.code() & symbol_catalog::anchor_is_symbol)) return false;
			if (1 != src.offset()) return false;	// need &#9500; ; only higher priority symbol is :=
			if (!is_strict_binary_node(src)) return false;

			if (detect_comma(*src.postfix().front())) return false;	// reject non-Intuitionistic syntactical entailments

			// unlike syntactical_entailment_2ary::can_construct_hypothesis, we aren't finicky about commas in the hypothesis arm
			return true;
		}

		// requires gentzen::symbol_catalog::global_parse to have run first, which is a global_rewriter
		// so doesn't process as a reformat_to_terminal
		static bool recognize(formal::lex_node& src) {
			if ((formal::Error | formal::Recognized) & src.code()) return false;
			if (has_pending_error(src)) return false;	// a pending (deferred) error, at any depth, also stops recognition
			if (!can_recognize(src)) return false;

			unsigned int err = 0;
			// if grouping parentheses are actually needed, they'll be restored by to_s()
			if (0 > remove_grouping_parentheses(src.prefix().front(), not_detect_comma)) {
				err |= 1U;
				error_report(*src.prefix().front(), "hypothesis arity would be increased");
			}
			if (0 > remove_grouping_parentheses(src.postfix().front(), not_detect_comma)) {
				err |= 2U;
				error_report(*src.postfix().front(), "conclusion arity would be increased");
			}
			if (   detect_comma(*src.prefix().front())
				&& normalize_word_strict_binary_node_to_lfold_r_fold_gassoc_node(*src.prefix().front())) {
				// the normalization put the fragments on the hypothesis arm, not on src
				for (decltype(auto) x : src.prefix().front()->fragments()) {
					if (0 > remove_grouping_parentheses(x.front(), not_detect_comma)) {
						err |= 4U;
						error_report(*x.front(), "hypothesis arity would be increased");
					}
				}
			}
			if (err) {
				src.learn(formal::Error);
				return false;
			}

			// tag placeholder-syntax symbols now
			tag_placeholder_syntax tag_this;
			tag_this(src.prefix().front());
			tag_this(src.postfix().front());
			src.learn(formal::Recognized);
			return true;
		}

		static bool global_parse(kuroda::parser<formal::lex_node>::edit_span& tokens, kuroda::parser<formal::lex_node>& grammar) {
			ptrdiff_t n = tokens.size();
			bool ret = false;
			while (0 <= --n) {
				if (recognize(*tokens[n])) ret = true;
			}
			return ret;
		}
	};

	// forward direction.  Modus tollens has too many logic dependencies to handle this cleanly.
	// W4: the technical-English name a defined notation gives a schema, if one does; defined
	// after defined_SVO.  Resolved notationally from the *schema* (the base rule), never from
	// the instantiated rule, whose notation is not the definiens.
	std::optional<perl::scalar> defined_name_for(const statement_t& schema);

	class syntactical_entailment_2ary_infer final : public fact_database {
	private:
		std::shared_ptr<fact_database> prior;
		std::shared_ptr<syntactical_entailment_2ary> rule;
		std::vector<size_t> hypothesis_ids;
		std::pair<statement_t, size_t> inferred;
		perl::scalar display;	// how the rule is named in the rationale: its defined name, else its notation

		syntactical_entailment_2ary_infer(
			std::shared_ptr<fact_database> p,
			std::shared_ptr<syntactical_entailment_2ary> r,
			decltype(hypothesis_ids)&& hids,
			statement_t inf,
			const statement_t& schema)
			: prior(std::move(p)), rule(std::move(r)),
			  hypothesis_ids(std::move(hids)), inferred(notation_ids::get().allocate(inf)) {
			// resolved at construction (open decision 2): the node carries its display name,
			// degrading to the rule notation when no definition names the schema
			if (auto name = defined_name_for(schema)) display = std::move(*name);
			else display = rule->to_s();
		}
	public:
		syntactical_entailment_2ary_infer() = default;
		syntactical_entailment_2ary_infer(const syntactical_entailment_2ary_infer&) = default;
		syntactical_entailment_2ary_infer(syntactical_entailment_2ary_infer&&) = default;
		syntactical_entailment_2ary_infer& operator=(const syntactical_entailment_2ary_infer&) = default;
		syntactical_entailment_2ary_infer& operator=(syntactical_entailment_2ary_infer&&) = default;
		~syntactical_entailment_2ary_infer() = default;

		// fact_database interface
		size_t size() const noexcept override { return 1; }
		std::shared_ptr<fact_database> parent() const { return prior; }
		std::optional<size_t> offset(size_t notation_id) const override {
			if (0 >= notation_id) return std::nullopt;
			if (axioms::get()->size() >= notation_id) return std::nullopt;

			if (inferred.second == notation_id) return 0;
			return std::nullopt;
		}

		std::pair<std::pair<statement_t, perl::scalar>, size_t> rationale(size_t n) const override {
			if (1 <= n) throw std::logic_error(rationale_oob);

			std::vector<perl::scalar> stage2(3);
			stage2[0] = std::string_view("label(");
			stage2[2] = std::string_view(")");

			std::vector<perl::scalar> labels(hypothesis_ids.size());
			ptrdiff_t i = hypothesis_ids.size();
			while (0 <= --i) {
				stage2[1] = std::to_string(hypothesis_ids[i]);
				labels[i] = join(stage2, "");
			}

			// design document format: <rule>; label(...) and label(...).  The row ledger
			// is responsible for rewriting each label(id) expression to a row label.
			std::vector<perl::scalar> stage(2);
			stage[0] = std::string(display.view()) + ";";
			stage[1] = join(labels, " and ");

			return std::pair(std::pair(inferred.first, join(stage, " ")), inferred.second);
		}

#if 2
		static syntactical_entailment_2ary_infer* construct(std::shared_ptr<syntactical_entailment_2ary> r, statement_t infer,
			decltype(hypothesis_ids) hyp_ids, std::shared_ptr<fact_database> prior, const statement_t& schema)
		{
			if (!r || !prior) return nullptr;
			return new syntactical_entailment_2ary_infer(std::move(prior), std::move(r), std::move(hyp_ids), std::move(infer), schema);
		}
#endif
	};

	class lemmas;
	std::shared_ptr<lemmas> infer_dest_for(std::shared_ptr<fact_database> src);

	class syntactical_entailment_introduction_start final : public fact_database {
	private:
		std::shared_ptr<fact_database> prior;
		std::vector<std::pair<statement_t, size_t> > hypotheses;

		static const constexpr std::string_view str_hypothesis = std::string_view("hypothesis");

		syntactical_entailment_introduction_start(std::shared_ptr<fact_database> assume, decltype(hypotheses)&& extra)
			: prior(assume), hypotheses(std::move(extra)) {}
	public:
		syntactical_entailment_introduction_start() = default;
		syntactical_entailment_introduction_start(const syntactical_entailment_introduction_start&) = default;
		syntactical_entailment_introduction_start(syntactical_entailment_introduction_start&&) = default;
		syntactical_entailment_introduction_start& operator=(const syntactical_entailment_introduction_start&) = default;
		syntactical_entailment_introduction_start& operator=(syntactical_entailment_introduction_start&&) = default;
		~syntactical_entailment_introduction_start() = default;

		// fact_database interface
		size_t size() const noexcept override { return hypotheses.size(); }
		std::shared_ptr<fact_database> parent() const { return prior; }
		std::optional<size_t> offset(size_t notation_id) const override {
			if (0 >= notation_id) return std::nullopt;
			if (axioms::get()->size() >= notation_id) return std::nullopt;

			ptrdiff_t i = -1;
			for (decltype(auto) x : hypotheses) {
				++i;
				if (x.second == notation_id) return i;
			}
			return std::nullopt;
		}

		std::pair<std::pair<statement_t, perl::scalar>, size_t> rationale(size_t n) const override {
			if (hypotheses.size() <= n) throw std::logic_error(rationale_oob);
			return std::pair(std::pair(hypotheses[n].first, str_hypothesis), hypotheses[n].second);
		}
		// end fact_database inferface

		// self must be a shared_ptr managing *this; caller keeps the returned tip alive
		std::shared_ptr<lemmas> infer_dest(const std::shared_ptr<fact_database>& self) const {
			return infer_dest_for(self);
		}

		formal::lex_node* hypothesis_node() const {
			std::vector<std::unique_ptr<formal::lex_node> > stage;
			for (decltype(auto) x : hypotheses) stage.push_back(std::unique_ptr<formal::lex_node>(node_from(x.first)));

			const auto stage_s = stage.size();
			if (1 == stage_s) return stage.front().release();

			if (2 == stage_s) return make_binary_node(comma, formal::Inert_Token, std::move(stage.front()), std::move(stage.back()));

			// this requires generalized rearrangement of logical and
			formal::word* w = new formal::word(comma, formal::src_location(), formal::Inert_Token);
			std::unique_ptr<formal::lex_node> ret(new formal::lex_node(w, formal::Inert_Token));

			ptrdiff_t i = -1;
			ret->fragments().reserve(stage_s);
			for (decltype(auto) x : stage) {
				decltype(auto) dest = ret->fragments()[++i];
				dest.push_back(nullptr);
				dest.front() = x.release();
			}

			return ret.release();
		}

		std::vector<size_t> hypothesis_ids() const {
			const size_t n = prior->size();
			ptrdiff_t i = hypotheses.size();

			std::vector<size_t> ret(hypotheses.size());
			while (0 <= --i) ret[i] = hypotheses[i].second;

			return ret;
		}

		static bool can_construct(const statement_t& src) { return valid_placeholder_symbol_content(src); }

	private:
		static std::shared_ptr<syntactical_entailment_introduction_start> finish_construct(decltype(hypotheses) && stage, std::shared_ptr<fact_database> assume) {
			if (!assume) assume = axioms::get();
			return std::shared_ptr<syntactical_entailment_introduction_start>(new syntactical_entailment_introduction_start(assume, std::move(stage)));
		}

	public:
		static std::shared_ptr<syntactical_entailment_introduction_start> construct(statement_t src, std::shared_ptr<fact_database> assume = nullptr) {
			if (!can_construct(src)) return nullptr;

			decltype(hypotheses) stage;
			stage.push_back(notation_ids::get().allocate(src));

			return finish_construct(std::move(stage), assume);
		}

		static std::shared_ptr<syntactical_entailment_introduction_start> construct(statement_t src, statement_t src2, std::shared_ptr<fact_database> assume = nullptr) {
			if (!can_construct(src)) return nullptr;
			if (!can_construct(src2)) return nullptr;

			decltype(auto) reg = notation_ids::get();
			decltype(hypotheses) stage;
			stage.push_back(reg.allocate(src));
			stage.push_back(reg.allocate(src2));

			return finish_construct(std::move(stage), assume);
		}
	};

	class syntactical_entailment_introduction final : public fact_database {
	private:
		std::shared_ptr<fact_database> prior;
		const syntactical_entailment_introduction_start* const hypotheses;
		std::pair<statement_t, size_t> inferred;

		static const constexpr std::string_view str_introduction = std::string_view("&#9500;-introduction,");

		// C++-side precondition: construct() has normalized the inferred tree's root through
		// symbol_catalog::anchor_symbol_parse.  Not NDEBUG-guarded by design: this is
		// foundations-of-math code, so correctness outranks CPU and there is no safe default.
		static std::shared_ptr<const formal::lex_node> verified(std::unique_ptr<formal::lex_node>& infer) {
			if (!infer || !(infer->code() & symbol_catalog::anchor_is_symbol)) throw std::logic_error("syntactical_entailment_introduction: inferred statement not symbol-anchored");
			return std::shared_ptr<const formal::lex_node>(infer.release());
		}

		syntactical_entailment_introduction(std::unique_ptr<formal::lex_node>& infer, const syntactical_entailment_introduction_start* hyp, const std::shared_ptr<fact_database>& assume)
			: prior(assume), hypotheses(hyp), inferred(notation_ids::get().allocate(verified(infer)))
			{}
	public:
		syntactical_entailment_introduction() = default;
		syntactical_entailment_introduction(const syntactical_entailment_introduction&) = default;
		syntactical_entailment_introduction(syntactical_entailment_introduction&&) = default;
		syntactical_entailment_introduction& operator=(const syntactical_entailment_introduction&) = default;
		syntactical_entailment_introduction& operator=(syntactical_entailment_introduction&&) = default;
		~syntactical_entailment_introduction() = default;

		// fact_database interface
		size_t size() const noexcept override { return 1; }
		std::shared_ptr<fact_database> parent() const { return prior; }
		std::optional<size_t> offset(size_t notation_id) const override {
			if (0 >= notation_id) return std::nullopt;
			if (axioms::get()->size() >= notation_id) return std::nullopt;

			if (inferred.second == notation_id) return 0;
			return std::nullopt;
		}

		std::pair<std::pair<statement_t, perl::scalar>, size_t> rationale(size_t n) const override {
			if (1 <= n) throw std::logic_error(rationale_oob);

			std::vector<perl::scalar> stage2(3);
			stage2[0] = std::string_view("label(");
			stage2[2] = std::string_view(")");

			auto ids = hypotheses->hypothesis_ids();
			std::vector<perl::scalar> stage(ids.size() + 1);

			stage[0] = str_introduction;
			ptrdiff_t i = ids.size();
			while (0 <= --i) {
				stage2[1] = std::to_string(ids[i]);
				stage[i + 1] = join(stage2, "");
			}

			return std::pair(std::pair(inferred.first, join(stage, " ")), inferred.second);
		}

		static std::optional<std::variant<std::unique_ptr<syntactical_entailment_introduction>, std::unique_ptr<syntactical_entailment_2ary> > > construct(std::shared_ptr<fact_database> src) {
			if (!src) return std::nullopt;
			if (dynamic_cast<syntactical_entailment_introduction_start*>(src.get())) return std::nullopt;

			auto hypotheses = find_ancestor<syntactical_entailment_introduction_start>(src);
			if (!hypotheses) return std::nullopt;

			// general case
			std::unique_ptr<formal::lex_node> lhs(hypotheses->hypothesis_node());
			auto conclusion = src->rationale(src->size() - 1);

			// construct a syntactical_entailment_2ary object, if we assumed 2 hypotheses
			if (auto ary2 = syntactical_entailment_2ary::construct(*lhs, conclusion.first.first)) return std::unique_ptr<syntactical_entailment_2ary>(ary2);

			// base case
			std::unique_ptr<formal::lex_node> rhs(node_from(conclusion.first.first));
			std::unique_ptr<formal::lex_node> ret(make_binary_node(std::string_view("&#9500;"), formal::Tokenized, std::move(lhs), std::move(rhs)));

			// the minted &#9500; word never went through the grammar: consult the symbol
			// catalog for the real code and offset rather than hard-coding them
			if (!symbol_catalog::anchor_symbol_parse(*ret)) return std::nullopt;

			return std::unique_ptr<syntactical_entailment_introduction>(new syntactical_entailment_introduction(ret, hypotheses, src));
		}
	};

	// R4: the "before add lemma" stage (validation and side effects), parallel to add_axiom's
	// is_not_legal_axiom / before_add_axiom_handler pair.  Defined after defined_SVO, which it
	// feeds; an error refuses the lemma.
	std::optional<perl::scalar> before_add_lemma(const statement_t& src);

	class lemmas : public fact_database, public zaimoni::observed<statement_t> {
	private:
		struct lemma {
			statement_t stmt;
			size_t id;
			perl::scalar rationale;	// how it was derived, as the recorder states it
		};

		std::shared_ptr<fact_database> prior;
		std::vector<lemma> _lemmas;
		std::vector<std::weak_ptr<syntactical_entailment_introduction_start> > conditional_reasoning;	// weak registry: prune expired entries when traversing

		// pure cache of the mint's live output: ownership stays with the reasoning tips.
		// a live value pins its key via prior, so value expiry covers key expiry
		static inline std::vector<std::pair<std::weak_ptr<fact_database>, std::weak_ptr<lemmas> > > _canonical;

		lemmas(decltype(prior) assumed) : prior(assumed) {}
	public:
		lemmas() = default;
		lemmas(const lemmas&) = delete;
		lemmas(lemmas&&) = delete;
		lemmas& operator=(const lemmas&) = delete;
		lemmas& operator=(lemmas&&) = delete;
		~lemmas() = default;

		static std::shared_ptr<lemmas> get() {
			static std::shared_ptr<lemmas> oaoo;	// interim strong owner of the axioms slot; migrates to the scheduler/session
			if (!oaoo) oaoo = infer_dest_for(axioms::get());
			return oaoo;
		}

		// fact_database interface
		size_t size() const noexcept override { return _lemmas.size(); }
		std::shared_ptr<fact_database> parent() const override { return prior; }
		std::optional<size_t> offset(size_t notation_id) const override {
			if (0 >= notation_id) return std::nullopt;
			if (axioms::get()->size() >= notation_id) return std::nullopt;

			ptrdiff_t i = -1;
			for (decltype(auto) x : _lemmas) {
				++i;
				if (x.id == notation_id) return i;
			}
			return std::nullopt;
		}

		std::pair<std::pair<statement_t, perl::scalar>, size_t> rationale(size_t n) const override {
			if (_lemmas.size() <= n) throw std::logic_error(rationale_oob);
			return std::pair(std::pair(_lemmas[n].stmt, _lemmas[n].rationale), _lemmas[n].id);
		}

		// observed interface
		void watched_by(const std::shared_ptr<zaimoni::observer<statement_t>>& src) override {
			// this is inappropriate in many contexts, but ok for our use case
			for (decltype(auto) lemma : _lemmas) {
				if (!src->onNext(lemma.stmt)) return;
			}

			seen_by(src);
		}
		// end observed interface

		// pure notational lookup of a lemma recorded here -- unlike record, which allocates a
		// notation id as a side effect.  Cf. axioms::disambiguate.
		std::optional<std::pair<statement_t, size_t> > disambiguate(const std::string& notation) const {
			for (decltype(auto) x : _lemmas) if (notation == x.stmt.to_s()) return std::pair(x.stmt, x.id);
			return std::nullopt;
		}

		// the root instance, over the axioms themselves: the only one whose lemmas are
		// unconditional, hence the only one where a definition can be registered (one made
		// under a hypothesis is scoped to it, and its fate at discharge is undesigned)
		bool is_root() const { return nullptr != std::dynamic_pointer_cast<axioms>(prior); }

		// Records a derived statement, notifying watchers.  Returns the recorded
		// (statement, notation id); a notation already present is returned without
		// duplication, and an axiom's notation is returned unrecorded.  The rationale is the
		// recorder's statement of how it was derived.  Before recording, the root instance
		// runs the before-add-lemma stage (R4); a refusal is reported and nothing is recorded
		// (the notation id allocated for it is simply unused).
		std::pair<statement_t, size_t> record(const statement_t& src, perl::scalar rationale = perl::scalar(std::string_view("prototyping lemma"))) {
			auto stage = notation_ids::get().allocate(src);
			if (0 < stage.second && axioms::get()->size() >= stage.second) return stage;
			if (auto have = offset(stage.second)) return std::pair(_lemmas[*have].stmt, _lemmas[*have].id);
			if (is_root()) {
				if (auto err = before_add_lemma(stage.first)) {
					error_report(stage.first.origin(), *err);
					return stage;
				}
			}
			_lemmas.push_back(lemma{stage.first, stage.second, std::move(rationale)});
			notify(_lemmas.back().stmt);
			return stage;
		}

		static std::shared_ptr<lemmas> infer_dest_for(const std::shared_ptr<fact_database>& src) {
			ptrdiff_t ub = _canonical.size();
			while (0 <= --ub) {
				auto& e = _canonical[ub];
				if (auto have = e.second.lock()) {
					if (!e.first.owner_before(src) && !src.owner_before(e.first)) return have;
				} else {
					e.swap(_canonical.back());
					_canonical.pop_back();
				}
			}
			auto ret = std::shared_ptr<lemmas>(new lemmas(src));
			_canonical.push_back(std::pair(src, ret));
			return ret;
		}

	};

	std::shared_ptr<lemmas> infer_dest_for(std::shared_ptr<fact_database> src) {
		return lemmas::infer_dest_for(src);
	}

	// \todo adjust this further, we actually need per-id results

	// Checks whether all notation ids in hypothesis_ids are known to some ancestor
	// of the given fact_database. Intended to be called from code controlled by
	// syntactical_entailment_2ary_infer::construct.
	// Returns the ancestor that contains them all, or nullptr if not found.
	const fact_database* hypothesis_ids_contained_by_ancestor(const std::shared_ptr<fact_database>& db, const std::vector<size_t>& hypothesis_ids) {
		if (hypothesis_ids.empty()) return nullptr;

		auto cursor = db;
		while (cursor) {
			auto s = cursor->size();
			bool all_found = true;
			for (auto id : hypothesis_ids) {
				if (id >= s) { all_found = false; break; }
			}
			if (all_found) return cursor.get();
			cursor = cursor->parent();
		}
		return nullptr;
	}

	std::partial_ordering reachable(std::shared_ptr<lemmas> lhs, std::shared_ptr<lemmas> rhs) {
		if (!lhs) return std::partial_ordering::unordered;
		if (!rhs) return std::partial_ordering::unordered;
		if (lhs.get() == rhs.get()) return std::partial_ordering::equivalent;
		decltype(auto) anchor = lemmas::get();
		if (anchor.get() == lhs.get()) return std::partial_ordering::less;
		if (anchor.get() == rhs.get()) return std::partial_ordering::greater;

		auto ancestors = find_ancestors<lemmas>(lhs);
		for(auto x : ancestors) if (x == rhs.get()) return std::partial_ordering::greater;
		ancestors = find_ancestors<lemmas>(rhs);
		for(auto x : ancestors) if (x == lhs.get()) return std::partial_ordering::less;

		return std::partial_ordering::unordered;
	}

	std::shared_ptr<lemmas> infer_to(std::shared_ptr<lemmas> lhs, std::shared_ptr<lemmas> rhs) {
		const auto test = reachable(lhs, rhs);
		if (test == std::partial_ordering::unordered) return nullptr;
		if (test == std::partial_ordering::greater) return rhs;
		return lhs;
	}

	std::shared_ptr<lemmas> inference_destination(std::shared_ptr<fact_database> db) {
		if (auto p = dynamic_cast<lemmas*>(db.get())) return std::shared_ptr<lemmas>(db, p);
		if (auto p = dynamic_cast<axioms*>(db.get())) return lemmas::get();
		if (auto p = dynamic_cast<syntactical_entailment_introduction_start*>(db.get())) return p->infer_dest(db);

		// arguably an invariant violation
		return nullptr;
	}

	// applicability enumeration (work item #4 of the cut-rule plan)

	struct visible_statement {
		statement_t stmt;
		std::string notation;
		size_t notation_id;
	};

	// tip-first walk of every statement the spine can see, deduplicated by notation id
	std::vector<visible_statement> visible_statements(std::shared_ptr<fact_database> tip)
	{
		std::vector<visible_statement> ret;
		while (tip) {
			const size_t s = tip->size();
			for (size_t n = 0; n < s; ++n) {
				auto stage = tip->rationale(n).first.first;
				auto id = notation_ids::get().allocate(stage);	// idempotent: reads back the registered id
				bool seen = false;
				for (decltype(auto) x : ret) {
					if (x.notation_id == id.second) {
						seen = true;
						break;
					}
				}
				if (!seen) ret.push_back(visible_statement{id.first, id.first.to_s(), id.second});
			}
			tip = tip->parent();
		}
		return ret;
	}

	struct applicable_2ary {
		std::shared_ptr<syntactical_entailment_2ary> rule;	// instantiated for this application
		statement_t inferred;	// the instantiated consequent
		std::vector<size_t> hypothesis_ids;	// notation ids matched, in hypothesis order
		statement_t schema;	// the base rule statement this application instantiates
	};

	// Enumerates the 2-ary rule applications available at tip: for each visible 2ary
	// rule, match hypothesis 0 against each visible statement (yielding the uniform
	// substitution), then require the instantiated hypothesis 1 to be visible
	// notationally.  Applications whose consequent is already visible are suppressed.
	//
	// Hard-coded assumptions, correct for the cut-rule test case:
	// * A statement never pattern-matches a bare placeholder variable (both directions
	//   are rejected by get_uniform_simultaneous_substitution).  Deliberate: a bare
	//   placeholder would match every visible statement, and binding placeholders to
	//   arbitrary statements is combinatoric explosion, not inference guidance.  The
	//   instantiated hypothesis 1 is therefore checked by literal notation only.
	// * Matching hypothesis 0 binds every placeholder variable of the rule at once
	//   (true for the modus ponens schema: {P, Q} covers hypothesis 1 and the
	//   conclusion).  A rule whose hypothesis 1 or conclusion uses placeholders not
	//   bound by hypothesis 0 cannot match this iteration.
	// * get_uniform_simultaneous_substitution is only ever called with hypothesis_idx
	//   0 here; 1 is implemented but unexercised, and matching against the conclusion
	//   (a hypothetical index 2) is unimplemented.
	std::vector<applicable_2ary> enumerate_applicable_2ary(const std::shared_ptr<fact_database>& tip)
	{
		std::vector<applicable_2ary> ret;
		const auto visible = visible_statements(tip);

		auto find_visible = [&visible](const std::string& notation) -> std::optional<size_t> {
			for (decltype(auto) v : visible) {
				if (notation == v.notation) return v.notation_id;
			}
			return std::nullopt;
		};

		for (decltype(auto) r : visible) {
			auto arm = std::get_if<std::shared_ptr<const formal::parsed> >(&r.stmt);
			if (!arm) continue;
			auto rule = dynamic_cast<const syntactical_entailment_2ary*>(arm->get());
			if (!rule) continue;

			for (decltype(auto) c : visible) {
				// The legacy Franci interpreters would consider rule to "want both of its hypotheses"
				auto subst = rule->get_uniform_simultaneous_substitution(c.stmt, 0);
				if (!subst) continue;
				auto instantiated = rule->apply_uniform_substitution(*subst);
				if (!instantiated) continue;
				// At this point, rule has been given its major hypothesis.
				// The Rete algorithm would be tracking instantiated as a rule instance.

				if (find_visible(instantiated->consequent().to_s())) continue;	// already derived
				// The legacy Franci interpreters would track instantiated->consequent().to_s(), as "provided by instantiated"
				// The Rete algorithm would consider the rule instance, to be "suppressed".

				auto h2 = find_visible(instantiated->hypothesis(1).to_s());
				// The Rete algorithm tracks partially activated rule instances (those that take the continue statement)
				if (!h2) continue;
				// The rule instance also has its minor hypothesis.

				// The Franci legacy interpreters, have many hard-coded special cases of matching hypotheses that
				// are not provided, with conclusions that provide the hypothesis.  These hard-codings are
				// used to prevent combinatoric explosion.  Inferences that are "not simplifying" are only executed
				// if some other inference rule, wants the provided conclusion, as a hypothesis.  This assumes
				// the cut rule for syntactical entailment.

				// Proceed with forward inference.
				std::shared_ptr<syntactical_entailment_2ary> stage(instantiated.release());
				std::vector<size_t> ids;
				ids.reserve(2);
				ids.push_back(c.notation_id);
				ids.push_back(*h2);
				ret.push_back(applicable_2ary{stage, stage->consequent(), std::move(ids), r.stmt});
			}
		}
		return ret;
	}

	// scaffold support: locate a syntactical_entailment_2ary among the loaded axioms
	std::pair<const syntactical_entailment_2ary*, statement_t> find_2ary_axiom()
	{
		decltype(auto) ax = axioms::get();
		size_t ub = ax->size();
		while (0 < ub) {
			auto stmt = ax->rationale(--ub).first.first;
			if (auto p = std::get_if<std::shared_ptr<const formal::parsed> >(&stmt)) {
				if (auto q = dynamic_cast<const syntactical_entailment_2ary*>(p->get())) return std::pair(q, stmt);
			}
		}
		return std::pair(nullptr, statement_t());
	}

	// scaffold: exercises the substitution and record primitives against the core.yaml
	// axioms.  Referenced from main() behind a compile-time switch.
	void self_test_substitution()
	{
		std::cout << std::unitbuf;

		const auto [rule, rule_stmt] = find_2ary_axiom();
		if (!rule) {
			std::cout << "self-test: no syntactical_entailment_2ary axiom found\n";
			return;
		}
		std::cout << "self-test rule: " << rule->to_s() << "\n";

		// identity substitution: match the rule's own first hypothesis
		auto subst = rule->get_uniform_simultaneous_substitution(rule->hypothesis(0), 0);
		std::cout << "self-test identity subst: " << (subst ? subst->to_s() : "(null)") << "\n";
		if (subst) {
			auto identity = rule->apply_uniform_substitution(*subst);
			std::cout << "self-test identity apply: " << (identity ? identity->to_s() : "(null)") << "\n";
		}

		// rename substitution: [Q, R \ P, Q]
		auto leaf = [](const char* name) {
			auto ret = new formal::lex_node(new formal::word(std::string_view(name), formal::src_location(), 0), 0);
			ret->learn(used_as_placeholder);
			return ret;
		};
		std::vector<formal::lex_node*> dests;
		std::vector<formal::lex_node*> srcs;
		dests.push_back(leaf("Q"));
		dests.push_back(leaf("R"));
		srcs.push_back(leaf("P"));
		srcs.push_back(leaf("Q"));
		std::unique_ptr<formal::lex_node> rename(make_substitution_node(std::move(dests), std::move(srcs)));
		std::cout << "self-test rename: " << rename->to_s() << "\n";
		auto minor = rule->apply_uniform_substitution(*rename);
		std::cout << "self-test rename apply: " << (minor ? minor->to_s() : "(null)") << "\n";
		if (!minor) return;

		// record: watcher sees the new lemma; duplicates and axioms do not re-record
		decltype(auto) L = lemmas::get();
		auto print_new = std::shared_ptr<zaimoni::observer<statement_t> >(new zaimoni::lambda_observer<statement_t>(
			[](const statement_t& x) {
				std::cout << "self-test watcher: " << x.to_s() << "\n";
				return true;
			}));
		L->watched_by(print_new);

		statement_t minor_stmt(std::shared_ptr<const formal::parsed>(minor.release()));
		const auto first = L->record(minor_stmt);
		const auto again = L->record(minor_stmt);
		std::cout << "self-test record: id " << first.second << (first.second == again.second ? ", stable" : ", UNSTABLE") << ", size " << L->size() << "\n";
		const auto ax_rec = L->record(rule_stmt);
		std::cout << "self-test record axiom: id " << ax_rec.second << ", size " << L->size() << "\n";

		// destination-kind relaxation: a statement subtree as the destination
		{
			std::vector<formal::lex_node*> dests;
			std::vector<formal::lex_node*> srcs;
			dests.push_back(deep_clone_to_lex_node(rule->hypothesis(0)));	// P &#9500; Q
			srcs.push_back(leaf("P"));
			std::unique_ptr<formal::lex_node> statement_subst(make_substitution_node(std::move(dests), std::move(srcs)));
			auto derived = rule->apply_uniform_substitution(*statement_subst);
			std::cout << "statement-destination apply: " << (derived ? derived->to_s() : "(null)") << "\n";
		}
	}

	// scaffold: drives rows 3-5 of the cut-rule proof (hypothesis P; infer Q; infer R)
	// through enumerate_applicable_2ary, entirely from the core.yaml axioms.
	// Referenced from main() behind a compile-time switch.
	void self_test_enumeration()
	{
		std::cout << std::unitbuf;

		const auto [rule, rule_stmt] = find_2ary_axiom();
		if (!rule) {
			std::cout << "enumeration self-test: no syntactical_entailment_2ary axiom found\n";
			return;
		}

		// statement inventory for the cut rule, built from the axiom itself
		auto leaf = [](const char* name) {
			auto ret = new formal::lex_node(new formal::word(std::string_view(name), formal::src_location(), 0), 0);
			ret->learn(used_as_placeholder);
			return ret;
		};
		std::vector<formal::lex_node*> dests;
		std::vector<formal::lex_node*> srcs;
		dests.push_back(leaf("Q"));
		dests.push_back(leaf("R"));
		srcs.push_back(leaf("P"));
		srcs.push_back(leaf("Q"));
		std::unique_ptr<formal::lex_node> rename(make_substitution_node(std::move(dests), std::move(srcs)));
		auto minor = rule->apply_uniform_substitution(*rename);
		if (!minor) {
			std::cout << "enumeration self-test: could not instantiate rule\n";
			return;
		}

		statement_t given1 = rule->hypothesis(0);	// P &#9500; Q
		statement_t given2 = minor->hypothesis(0);	// Q &#9500; R
		statement_t hyp = rule->hypothesis(1);	// P

		// exercise both contract modes of is_unary_syntactical_entailment on real data
		if (auto h = std::get_if<std::shared_ptr<const formal::lex_node> >(&given1)) {
			std::cout << "unary entailment (default): " << is_unary_syntactical_entailment(**h) << "\n";
			std::cout << "unary entailment (contract): " << is_unary_syntactical_entailment<true>(**h) << "\n";
			std::cout << "entailment, lex form: " << is_syntactical_entailment<true>(**h) << "\n";
		}

		// a lex_node wrapping a converted syntactical_entailment_2ary is not a
		// lex_node-form entailment, even under the throwing contract
		{
			formal::lex_node wrapper(std::shared_ptr<const formal::parsed>(rule->clone().release()));
			std::cout << "entailment, parsed wrapper: " << is_syntactical_entailment<true>(wrapper) << "\n";
		}

		// exercise the ACID fold-chain normalization: (P , Q) , R flattens to arity 3;
		// a leaf-only binary node refuses
		{
			std::unique_ptr<formal::lex_node> chain(make_binary_node(comma, 0,
				std::unique_ptr<formal::lex_node>(make_binary_node(comma, 0,
					std::unique_ptr<formal::lex_node>(leaf("P")),
					std::unique_ptr<formal::lex_node>(leaf("Q")))),
				std::unique_ptr<formal::lex_node>(leaf("R"))));
			std::cout << "normalize fold chain: " << normalize_word_strict_binary_node_to_lfold_r_fold_gassoc_node(*chain)
				<< ", gassoc " << is_lfold_rfold_gassoc_node(*chain)
				<< ", superficially parsed " << are_fragments_superficially_parsed(*chain)
				<< ", arity " << chain->fragments().size() << "\n";

			std::unique_ptr<formal::lex_node> flat(make_binary_node(comma, 0,
				std::unique_ptr<formal::lex_node>(leaf("P")),
				std::unique_ptr<formal::lex_node>(leaf("Q"))));
			std::cout << "normalize leaf-only binary: " << normalize_word_strict_binary_node_to_lfold_r_fold_gassoc_node(*flat) << "\n";
		}

		// exercise the 3+ary normalization wired into tag_syntactical_entailment
		// (unreachable from statement files until comma gains associativity config):
		// A, B, C |- D normalizes cleanly; a tuple operand (A, B) must be rejected
		{
			auto wrap_parens = [](formal::lex_node* content) {
				formal::word* l = new formal::word(std::string_view("("), formal::src_location(), formal::Inert_Token);
				formal::word* r = new formal::word(std::string_view(")"), formal::src_location(), formal::Inert_Token);
				auto ret = new formal::lex_node(l, formal::Inert_Token);
				ret->infix().push_back(content);
				ret->set_null_post_anchor(r);
				return ret;
			};
			auto chain3 = [](formal::lex_node* a, formal::lex_node* b, formal::lex_node* c) {
				return make_binary_node(comma, 0,
					std::unique_ptr<formal::lex_node>(a),
					std::unique_ptr<formal::lex_node>(make_binary_node(comma, 0,
						std::unique_ptr<formal::lex_node>(b),
						std::unique_ptr<formal::lex_node>(c))));
			};

			std::unique_ptr<formal::lex_node> good(make_binary_node(std::string_view("&#9500;"), 0,
				std::unique_ptr<formal::lex_node>(chain3(leaf("A"), leaf("B"), leaf("C"))),
				std::unique_ptr<formal::lex_node>(leaf("D"))));
			symbol_catalog::anchor_symbol_parse(*good);
			auto prior = Errors.count();
			bool recognized = tag_syntactical_entailment::recognize(*good);
			std::cout << "3ary entailment: recognized " << recognized
				<< ", arm gassoc " << is_lfold_rfold_gassoc_node(*good->prefix().front())
				<< ", arity " << good->prefix().front()->fragments().size()
				<< ", errors +" << (Errors.count() - prior) << "\n";

			std::unique_ptr<formal::lex_node> bad(make_binary_node(std::string_view("&#9500;"), 0,
				std::unique_ptr<formal::lex_node>(chain3(
					wrap_parens(make_binary_node(comma, 0,
						std::unique_ptr<formal::lex_node>(leaf("A")),
						std::unique_ptr<formal::lex_node>(leaf("B")))),
					leaf("C"), leaf("D"))),
				std::unique_ptr<formal::lex_node>(leaf("E"))));
			symbol_catalog::anchor_symbol_parse(*bad);
			prior = Errors.count();
			recognized = tag_syntactical_entailment::recognize(*bad);
			std::cout << "tuple-operand entailment: recognized " << recognized
				<< ", errors +" << (Errors.count() - prior) << "\n";
		}

		// exercise the remove_grouping_parentheses return encoding
		{
			auto wrap_parens = [](formal::lex_node* content) {
				formal::word* l = new formal::word(std::string_view("("), formal::src_location(), formal::Inert_Token);
				formal::word* r = new formal::word(std::string_view(")"), formal::src_location(), formal::Inert_Token);
				auto ret = new formal::lex_node(l, formal::Inert_Token);
				ret->infix().push_back(content);
				ret->set_null_post_anchor(r);
				return ret;
			};
			formal::lex_node* probe = wrap_parens(deep_clone_to_lex_node(given1));
			std::cout << "strip, no predicate: " << remove_grouping_parentheses(probe) << "\n";
			delete probe;
			probe = wrap_parens(deep_clone_to_lex_node(given1));
			std::cout << "strip, veto: " << remove_grouping_parentheses(probe, [](const formal::lex_node&) { return false; }) << "\n";
			delete probe;
		}

		auto outer = syntactical_entailment_introduction_start::construct(given1, given2);
		if (!outer) {
			std::cout << "enumeration self-test: outer start construction failed\n";
			return;
		}
		auto inner = syntactical_entailment_introduction_start::construct(hyp, outer);
		if (!inner) {
			std::cout << "enumeration self-test: inner start construction failed\n";
			return;
		}

		std::shared_ptr<fact_database> tip = inner;

		int round = 0;
		while (++round <= 4) {
			auto apps = enumerate_applicable_2ary(tip);
			std::cout << "enumeration round " << round << ": " << apps.size() << " applicable\n";
			for (decltype(auto) a : apps) {
				std::cout << "  " << a.rule->to_s() << " => " << a.inferred.to_s() << "\n";
			}
			if (apps.empty()) break;
			decltype(auto) a = apps.front();
			auto next = syntactical_entailment_2ary_infer::construct(a.rule, a.inferred, a.hypothesis_ids, tip, a.schema);
			if (!next) {
				std::cout << "enumeration self-test: 2ary_infer construction failed\n";
				return;
			}
			tip = std::shared_ptr<fact_database>(next);
			std::cout << "  applied: " << tip->rationale(0).first.second.view() << "\n";
		}
	}

} // end namespace gentzen

// end prototype class

// lex_node::to_s policy: a node anchored at := (binary infix priority index 0)
// rendered as a direct argument of any other binary infix symbol must regenerate its
// grouping parentheses -- otherwise reparsing lets :=, the highest-priority split,
// seize the top level.  Applied to both argument sides; the mis-parse is symmetric.
// Installed via std::function so the parser library stays ignorant of Gentzen types.
static bool gentzen_interpolate_grouping_parens(const formal::lex_node& parent, const formal::lex_node& child)
{
	// currently, higher precedence corresponds to lower symbol index i.e. lower offset()
	// \todo restrict to binary infix symbols (use symbol_catalog for lookup)
	if (!(child.code() & gentzen::symbol_catalog::anchor_is_symbol)) return false;
	if (!(parent.code() & gentzen::symbol_catalog::anchor_is_symbol)) return false;
	if (parent.offset() < child.offset()) return false;
	if (parent.offset() > child.offset()) return true;
	// \todo this should rely on lfold/rfold/generalized associativity.  With current configuration ordering:
	// := (0) and &#9500; (1) are disqualified; & and , are qualified
	return 1 >= parent.offset();
}

// LG_* are word-level bit indices for the line-grammar phase only.  Their bit
// positions deliberately overlap HTMLtag::Entity and undefined_SVO::postfix/infix
// (bits 1-3): line-phase codes are assigned to whole-line words before tokenization,
// the overlapping flags only to sub-line words and lex_nodes during or after it, and
// the lex_node thin-wrap constructor propagates only Comment — so the two bit spaces
// never meet on one object.  Only Comment and Error, which do cross phases, are
// statically asserted distinct; LG_* is therefore excluded from the all_distinct_bits
// master list below.
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
			std::unique_ptr<const std::string> next(new std::string(x));
			LineGrammar().append_to_parse(ret, new formal::word(std::shared_ptr<const std::string>(next.release()), origin));
		}

		origin.line_pos.first++;
		origin.line_pos.second = 0;
	}
	return ret;
}

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
				formal::word* raw_word = new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size()));
				std::unique_ptr<formal::lex_node> node(new formal::lex_node(raw_word));
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
			formal::word* raw_word = new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size()));
			std::unique_ptr<formal::lex_node> node(new formal::lex_node(raw_word));
			src.insertNSlotsAt(1, viewpoint + 1);
			src[viewpoint + 1] = node.release();
		};

		*w = formal::word(std::shared_ptr<const std::string>(new std::string(*html_entity)), w->origin(), w->code());
		w->learn(HTMLtag::Entity);
		x->learn(HTMLtag::Entity | formal::Inert_Token);
		return ret;
	}

	if (auto tag = HTMLtag::parse(src, viewpoint)) {
		std::unique_ptr<formal::lex_node> stage(new formal::lex_node(tag.release(), HTMLtag::Tag | formal::Tokenized));
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
				formal::word* raw_word = new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size()));
				std::unique_ptr<formal::lex_node> node(new formal::lex_node(raw_word));
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
				formal::word* raw_word = new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size()));
				std::unique_ptr<formal::lex_node> node(new formal::lex_node(raw_word));
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

// works for GentzenGrammar.apply, not TokenGrammar.apply
auto parse_inside_balanced(const std::string_view& l_token, const std::string_view& r_token)
{
	return [=](formal::lex_node& src) {
		if (src.infix().empty()) return false;
		const auto in_size = src.infix().size();
		if (1 == in_size) return false;

		if (auto anchor = src.c_anchor<formal::word>()) {
			if (l_token != anchor->value()) return false;
		} else return false;

		if (auto post_anchor = src.c_post_anchor<formal::word>()) {
			if (r_token != post_anchor->value()) return false;
		} else return false;

		const auto prior_errors = Errors.count();

		src.infix() = GentzenGrammar().apply(src.infix());
		if (prior_errors == Errors.count()) GentzenGrammar().complete_parse(src.infix());

		return in_size != src.infix().size();	// not really
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
	for (decltype(auto) x : gentzen::HTML_binds_to_predecessor) {
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

static kuroda::parser<formal::lex_node>& TokenGrammar() {
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
	// we have seven theoretical kinds of undefined sentences, based on where the infix/postfix/prefix text is.
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

	undefined_SVO() = default;
public:
	static constexpr const unsigned long long postfix = (1ULL << 2); // reserve this flag for both word and lex_node
	static constexpr const unsigned long long infix = (1ULL << 3); // reserve this flag for both word and lex_node
	static constexpr const unsigned long long prefix = (1ULL << 4); // reserve this flag for both word and lex_nodel

	static_assert(!(postfix & formal::Comment));
	static_assert(!(postfix & formal::Error));
	static_assert(!(postfix & formal::Inert_Token));
	static_assert(!(postfix & formal::Tokenized));
	static_assert(!(postfix & formal::RequestNormalization));
	static_assert(!(postfix & HTMLtag::Entity));
	static_assert(!(postfix & HTMLtag::Tag));
	static_assert(!(postfix & gentzen::symbol_catalog::anchor_is_symbol));
	static_assert(!(postfix & gentzen::symbol_catalog::anchor_is_const_symbol));
	static_assert(!(postfix & gentzen::symbol_catalog::anchor_is_not_symbol));
	static_assert(!(postfix & gentzen::used_as_placeholder));
	static_assert(!(postfix & gentzen::parsed_substitution));

	static_assert(!(infix & formal::Comment));
	static_assert(!(infix & formal::Error));
	static_assert(!(infix & formal::Inert_Token));
	static_assert(!(infix & formal::Tokenized));
	static_assert(!(infix & formal::RequestNormalization));
	static_assert(!(infix & HTMLtag::Entity));
	static_assert(!(infix & HTMLtag::Tag));
	static_assert(!(infix & gentzen::symbol_catalog::anchor_is_symbol));
	static_assert(!(infix & gentzen::symbol_catalog::anchor_is_const_symbol));
	static_assert(!(infix & gentzen::symbol_catalog::anchor_is_not_symbol));
	static_assert(!(infix & postfix));
	static_assert(!(infix & gentzen::used_as_placeholder));
	static_assert(!(infix & gentzen::parsed_substitution));

	static_assert(!(prefix & formal::Comment));
	static_assert(!(prefix & formal::Error));
	static_assert(!(prefix & formal::Inert_Token));
	static_assert(!(prefix & formal::Tokenized));
	static_assert(!(prefix & formal::RequestNormalization));
	static_assert(!(prefix & HTMLtag::Entity));
	static_assert(!(prefix & HTMLtag::Tag));
	static_assert(!(prefix & gentzen::symbol_catalog::anchor_is_symbol));
	static_assert(!(prefix & gentzen::symbol_catalog::anchor_is_const_symbol));
	static_assert(!(prefix & gentzen::symbol_catalog::anchor_is_not_symbol));
	static_assert(!(prefix & postfix));
	static_assert(!(prefix & infix));
	static_assert(!(prefix & gentzen::used_as_placeholder));
	static_assert(!(prefix & gentzen::parsed_substitution));

	undefined_SVO(const undefined_SVO&) = delete;
	undefined_SVO(undefined_SVO&&) = delete;
	undefined_SVO& operator=(const undefined_SVO&) = delete;
	undefined_SVO& operator=(undefined_SVO&&) = delete;
	~undefined_SVO() = default;

	class phrase_postfix final : public formal::parsed {
		std::shared_ptr<const formal::lex_node> _subject;
		size_t _code;

/*
  - "is a unary prefix symbol"
  - "is a unary postfix symbol"
  - "is a binary infix symbol"
  - "is a constant symbol"
*/
		// this must agree with core.yaml:SVO_postfix_undefined
	public:
		enum class hard_code {
			prefix_symbol = 0,
			postfix_symbol,
			infix_symbol,
			constant_symbol,
			prefix_undefined,
			postfix_undefined,
			infix_undefined,
			placeholder_syntax,
			symbol_placeholder_syntax,
			balanced_symbol_pair
		};
	private:
		enum {
			PARSE_TREE_MAX = (int)hard_code::infix_symbol + 1,
			SYMBOL_MAX = (int)hard_code::constant_symbol + 1,
			HARD_CODE_MAX = (int)hard_code::balanced_symbol_pair + 1
		};
	public:
		phrase_postfix() = default;
		phrase_postfix(const phrase_postfix&) = default;
		phrase_postfix(phrase_postfix&&) = default;
		phrase_postfix& operator=(const phrase_postfix&) = default;
		phrase_postfix& operator=(phrase_postfix&&) = default;
		~phrase_postfix() = default;

		phrase_postfix(formal::lex_node*& sub, size_t code) : _subject(sub), _code(code) {
			sub = nullptr;
		}
		phrase_postfix(formal::lex_node*& sub, hard_code code) : _subject(sub), _code((size_t)code) {
			sub = nullptr;
		}

		const auto& subject() const { return _subject; }
		auto& subject() { return _subject; }

		std::unique_ptr<parsed> clone() const override { return std::unique_ptr<parsed>(new phrase_postfix(*this)); }
		void CopyInto(formal::parsed*& dest) const override {
			if (dest) {
				if (auto x = dynamic_cast<phrase_postfix*>(dest)) {
					*x = *this;
					return;
				}
				delete dest;
			}
			dest = new phrase_postfix(*this);
		}
		void MoveInto(formal::parsed*& dest) override {
			if (dest) {
				if (auto x = dynamic_cast<phrase_postfix*>(dest)) {
					*x = std::move(*this);
					return;
				}
				delete dest;
			}
			dest = new phrase_postfix(*this);
		}

		formal::src_location origin() const override { return _subject->origin(); }

		void get_placeholder_variables(formal::placeholder_match& dest) const override {
			if (!_subject) return;
			// mention, not use: the placeholder-syntax declaring axioms do not treat
			// their subject as a variable (cf. the tag_placeholder_syntax base-case notes)
			if ((size_t)hard_code::placeholder_syntax == _code || (size_t)hard_code::symbol_placeholder_syntax == _code) return;
			// the subject is typically a pure-anchor leaf, and placeholder handles attach
			// only via parent slots (cf. apply_uniform_substitution's bare-placeholder
			// case), so walk a carrier node around the tagged clone
			std::unique_ptr<formal::lex_node> stage(new formal::lex_node(*_subject));
			gentzen::tag_placeholder_syntax tag_this;
			tag_this(stage.get());
			std::unique_ptr<formal::lex_node> carrier(new formal::lex_node());
			carrier->infix().push_back(stage.release());
			formal::lex_node& root = *carrier;
			dest.aux_trees.push_back(std::move(carrier));
			gentzen::collect_placeholder_handles().walk_into(root, dest);
		}

		// Unifies this phrase (a hypothesis pattern whose subject is a
		// symbol-placeholder variable) with candidate (the same phrase kind, concrete
		// symbol subject), authorizing the uniform substitution
		// [candidate subject / our subject].  Step 2 of the instantiation mechanics in
		// Notes/GentzenW-defined-notations.md: the hypothesis "_ is a binary infix
		// symbol" against the axiom "&#9500; is a binary infix symbol" yields
		// [&#9500; / _].  Returns null when the phrases do not unify.
		std::unique_ptr<formal::lex_node> get_uniform_substitution(const phrase_postfix& candidate) const {
			if (_code != candidate._code) return nullptr;
			if (!_subject || !candidate._subject) return nullptr;
			if (!gentzen::is_symbol_placeholder_syntax_symbol(_subject.get())) return nullptr;
			// the candidate's subject must be a concrete symbol, not placeholder syntax
			if (!gentzen::is_symbol_like(candidate._subject.get())) return nullptr;
			if (gentzen::is_symbol_placeholder_syntax_symbol(candidate._subject.get())) return nullptr;
			if (gentzen::is_placeholder_syntax_symbol(candidate._subject.get())) return nullptr;

			std::unique_ptr<formal::lex_node> dest(new formal::lex_node(*candidate._subject));
			std::unique_ptr<formal::lex_node> src(new formal::lex_node(*_subject));
			gentzen::tag_placeholder_syntax tag_this;
			tag_this(src.get());	// the source must read as a placeholder-family variable downstream

			std::vector<formal::lex_node*> dests;
			std::vector<formal::lex_node*> srcs;
			dests.push_back(dest.release());
			srcs.push_back(src.release());
			return std::unique_ptr<formal::lex_node>(gentzen::make_substitution_node(std::move(dests), std::move(srcs)));
		}

		// Step-3 support: a freshly-allocated clone with the subject uniformly
		// substituted, when our subject is a placeholder-family variable named in
		// rename; null otherwise.  The declaring axiom kinds are exempt (mention, not
		// use), mirroring get_placeholder_variables.
		phrase_postfix* apply_uniform_substitution(const gentzen::prepared_rename& rename) const {
			if (!_subject) return nullptr;
			if ((size_t)hard_code::placeholder_syntax == _code || (size_t)hard_code::symbol_placeholder_syntax == _code) return nullptr;
			if (   !gentzen::is_symbol_placeholder_syntax_symbol(_subject.get())
				&& !gentzen::is_placeholder_syntax_symbol(_subject.get())) return nullptr;
			const auto key = _subject->to_scalar();
			for (decltype(auto) x : rename.view()) {
				if (x.first.view() == key.view()) {
					formal::lex_node* stage = new formal::lex_node(*x.second);
					return new phrase_postfix(stage, _code);
				}
			}
			return nullptr;
		}

		std::optional<perl::scalar> is_not_legal_axiom(bool unconditional) const override {
			if (SYMBOL_MAX > _code) {
				if (!gentzen::is_symbol_like(_subject.get())) return "not a syntactical symbol";
			}
			if (!unconditional) {
				if (PARSE_TREE_MAX > _code) return "cannot conditionally alter syntax";
			}
			return std::nullopt;
		}

		std::optional<perl::scalar> before_add_axiom_handler() const override {
			enum { trace_param = 0 };
			if constexpr (trace_param) std::cerr << "entering before_add_axiom_handler\n";

			decltype(auto) symbols = gentzen::symbol_catalog::get();

			std::optional<perl::scalar> err;
			switch (_code) {
			case (int)hard_code::prefix_symbol: err = symbols.declare_prefix(_subject); break;
			case (int)hard_code::postfix_symbol: err = symbols.declare_postfix(_subject); break;
			case (int)hard_code::infix_symbol: err = symbols.declare_infix(_subject); break;
			case (int)hard_code::constant_symbol: err = symbols.declare_constant(_subject); break;
			default:
				if constexpr (trace_param) std::cerr << "before_add_axiom_handler: no action\n";
				return std::nullopt;
			}
			if (err) return err;

			// The subject was classified while this statement was being parsed, i.e. before the
			// declaration above took effect: a symbol's *first* declaring axiom cached
			// anchor_is_not_symbol on the very symbol it declares, and every later copy of that
			// subject (uniform substitution destinations) inherited the stale cache, which
			// symbol_catalog::anchor_symbol_parse trusts.  Re-classify now that the catalog is
			// current.  The const_cast is deliberate and meant to look unusual: this is the one
			// mutation of an otherwise const-correct class, it touches only the parse-state
			// cache bits (never the notation), and the node was allocated non-const.
			auto& subject = const_cast<formal::lex_node&>(*_subject);
			subject.forget(gentzen::symbol_catalog::anchor_is_symbol | gentzen::symbol_catalog::anchor_is_const_symbol | gentzen::symbol_catalog::anchor_is_not_symbol);
			gentzen::symbol_catalog::anchor_symbol_parse(subject);
			return std::nullopt;
		}

		std::string to_s() const override {
			std::vector<perl::scalar> stage;
			stage.push_back((perl::scalar)(*_subject));
			const auto& test = get();
			if (_code >= test._postfix.size()) {
				stage.push_back("<error>");
				return join(stage, " ");
			}
			stage.push_back((perl::scalar)test._postfix[_code]);
			return join(stage, " ");
		}
	};

	static undefined_SVO& get() { return *_get(); }

private:
	void load() {
		enum { trace_load = 0 };

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
					auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
					src.line_pos.first++;
					src.line_pos.second = 0;
					gentzen::report_diagnostics(stage);
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
					report_exception("line iteration body", e);
					return;
				}
			}
		}

		(decltype(dest)()).swap(dest);

		// ok to preload axioms after all undefined notations are loaded
		{
		auto& instinct = *gentzen::axioms::get();
		// preload placeholder-syntax symbol axioms
		char tok[2] = "A";
		do {
			instinct.add_axiom(undefined_SVO::declare_placeholder(tok));
		} while ('Z' >= ++tok[0]);

		// preload symbol-placeholder-syntax symbol axioms
		instinct.add_axiom(undefined_SVO::declare_placeholder("_", true));

		// preload balanced symbol pair axioms
		instinct.add_axiom(undefined_SVO::declare_balanced_pair(reserved_atomic[0].first, reserved_atomic[1].first));
		instinct.add_axiom(undefined_SVO::declare_balanced_pair(reserved_atomic[2].first, reserved_atomic[3].first));
		instinct.add_axiom(undefined_SVO::declare_balanced_pair(reserved_atomic[4].first, reserved_atomic[5].first));


		if (root.contains("axioms")) {
			fkyaml::from_node(root["axioms"], dest);

			formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path("cfg/core.yaml:axioms")));
			auto lines = to_lines(dest, src);
			while (!lines.empty()) {
				try {
					const auto prior_errors = Errors.count();
					auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
					src.line_pos.first++;
					src.line_pos.second = 0;
					if (prior_errors < Errors.count()) continue;

					TokenGrammar().complete_parse(stage);
					if (prior_errors < Errors.count()) continue;

					stage = GentzenGrammar().apply(stage);
					gentzen::report_diagnostics(stage);	// balanced groups parse (and may defer errors) during apply
					if (prior_errors < Errors.count()) continue;

					GentzenGrammar().complete_parse(stage);

					gentzen::report_diagnostics(stage);
					if (1 == stage.size() && prior_errors == Errors.count()) {
						if constexpr (trace_load) std::cerr << "considering axiom\n";
						if (auto relay = stage[0]->shared_anchor<formal::parsed>()) {
							if constexpr (trace_load) std::cerr << "adding axiom " << relay->to_s() << "\n";
							instinct.add_axiom(relay);
						} else {
							// a statement still in lex_node form (the conditional definition
							// axiom is a &#9500;-anchored node): admit it as-is; add_axiom's
							// gates report an unrecognized or illegal one.  Ownership moves
							// out of the symbols container.
							formal::lex_node* raw = stage[0];
							stage[0] = nullptr;
							if constexpr (trace_load) std::cerr << "adding lex_node axiom " << raw->to_s() << "\n";
							instinct.add_axiom(std::shared_ptr<const formal::lex_node>(raw));
						}
					}
				} catch (std::exception& e) {
					report_exception("line iteration body", e);
					return;
				}
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
					auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
					src.line_pos.first++;
					src.line_pos.second = 0;
					if (prior_errors < Errors.count()) continue;

					TokenGrammar().complete_parse(stage);
					if (prior_errors < Errors.count()) continue;

					stage = GentzenGrammar().apply(stage);
					gentzen::report_diagnostics(stage);	// balanced groups parse (and may defer errors) during apply
					if (prior_errors < Errors.count()) continue;

					GentzenGrammar().complete_parse(stage);

					// tests of new parsing code go here
					gentzen::report_diagnostics(stage);
					if (1 == stage.size() && prior_errors == Errors.count()) {
						// dump placeholder grouping for any parsed object that exposes one
						if (auto relay = stage.front()->shared_anchor<formal::parsed>()) {
							if (auto match = relay->get_placeholder_variables()) {
								std::cout << "placeholder_variables of parsed " << relay->to_s() << ":\n";
								for (const auto& g : match->groups) {
									std::cout << "  " << g.first.view() << " x " << g.second.size() << "\n";
								}
							}
						} else {
							// this tests false against Q &#9500; R because the placeholder variable tagging hasn't been triggered
							if (auto match = gentzen::collect_placeholder_handles()(stage.front())) {
								std::cout << "placeholder_variables of lex_node " << stage.front()->to_s() << ":\n";
								for (const auto& g : match->groups) {
									std::cout << "  " << g.first.view() << " x " << g.second.size() << "\n";
								}
							}
						}
						// probe: if the test line itself looks like a candidate (lex_node arm),
						// try to match it against every loaded 2-ary entailment axiom and dump
						// the resulting substitution.
						// reject placeholder syntax symbols of both flavors
						if (   !gentzen::is_placeholder_syntax_symbol(stage.front())
							&& !gentzen::is_symbol_placeholder_syntax_symbol(stage.front())) {
							// Q &#9500; R parses as a binary node with anchor &9500; and placeholder syntax symbols R and S as prefix and postfix
							auto raw = stage.front()->shared_anchor<formal::lex_node>();
							std::cout << "stage.front(): " << stage.front()->to_s() << "\n";
							for (decltype(auto) str : diagnose(stage)) {
								std::cout << str << std::endl;
							}
							// this disables diagnose later one
							gentzen::statement_t candidate(std::shared_ptr<const formal::lex_node>(stage.front()));
							stage.front() = nullptr;
#if 2
							auto axiom_set = gentzen::axioms::get();
							for (size_t i = 0; i < axiom_set->size(); ++i) {
								const auto& axiom_stmt = axiom_set->rationale(i).first.first;
								auto p = std::get_if<std::shared_ptr<const formal::parsed>>(&axiom_stmt);
								if (!p || !*p) continue;
								auto rule = std::dynamic_pointer_cast<const gentzen::syntactical_entailment_2ary>(*p);
								if (!rule) continue;
								if (auto subst = rule->get_uniform_simultaneous_substitution(candidate, 0)) {
									std::cout << "substitution against axiom " << (i+1) << " hyp 0: "
										<< subst->to_s() << "\n";
								}
							}
#endif
							continue;
						}
					}

					for (decltype(auto) str : diagnose(stage)) {
						std::cout << str << std::endl;
					}
				}
				catch (std::exception& e) {
					report_exception("line iteration body", e);
					return;
				}
			}
		}
	}

public:
	// these two are called during axiom loading, when the RAM pressure should be very low.  If these new operator 
	// calls were to throw, there would be no viable recovery strategy.
	static std::shared_ptr<const formal::parsed> declare_placeholder(std::string src, bool symbol = false) {
		formal::word* token = new formal::word(std::move(src), formal::src_location(), formal::Tokenized);
		formal::lex_node* subject = new formal::lex_node(token, formal::Tokenized);

		return std::shared_ptr<const formal::parsed>(new undefined_SVO::phrase_postfix(subject, symbol ? undefined_SVO::phrase_postfix::hard_code::symbol_placeholder_syntax : undefined_SVO::phrase_postfix::hard_code::placeholder_syntax));
	}

	static std::shared_ptr<const formal::parsed> declare_balanced_pair(const std::string_view& lhs, const std::string_view& rhs) {
		formal::word* token_lhs = new formal::word(lhs, formal::src_location(), formal::Inert_Token);
		formal::word* token_rhs = new formal::word(rhs, formal::src_location(), formal::Inert_Token);
		formal::lex_node* subject = new formal::lex_node(token_lhs, formal::Tokenized);
		subject->set_null_post_anchor(token_rhs);

		return std::shared_ptr<const formal::parsed>(new undefined_SVO::phrase_postfix(subject, undefined_SVO::phrase_postfix::hard_code::balanced_symbol_pair));
	}

	static bool global_parse(kuroda::parser<formal::lex_node>::edit_span& tokens, kuroda::parser<formal::lex_node>& grammar) {
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
			if (1 == phrase_origin) {
				const bool symbol_is_word = lhs_is_symbol ? (bool)gentzen::is_symbol_like(tokens[0]) : true;
				std::unique_ptr<formal::lex_node> dest(new formal::lex_node(new phrase_postfix(tokens[0], n)));
				if (!symbol_is_word) error_report(*dest, "symbol must be a word");
				tokens[0] = dest.release();
				kuroda::parser<formal::lex_node>::DeleteNSlotsAt(tokens, tokens.size() - 1, 1);
				return true;
			}

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
				} else if (!gentzen::is_symbol_like(dest->prefix().front())) {
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

// The per-declaration static_assert batteries above check pairwise distinctness by
// hand, which is O(n^2) to maintain; this fold does the n^2 work in the compiler
// while a human maintains the one list.  Every reserved word/lex_node code flag
// belongs in the list (word-only line-phase LG_* bits excepted; see LG_modes).
// \todo(__cpp_reflection) replace the hand-maintained list with a reflection-driven
// sweep of the declaring classes/namespaces (P2996): the declaration becomes the only
// mention, eliminating the forgotten-entry failure mode.  Prefer this over
// std::ranges views::concat of per-site fragments, which merely coarsens the list.
#ifdef __cpp_reflection
#pragma message("all_distinct_bits: reflection available -- replace the hand-maintained flag list with a reflected sweep")
#endif
constexpr bool all_distinct_bits(std::initializer_list<unsigned long long> src)
{
	unsigned long long seen = 0;
	for (auto x : src) {
		if (seen & x) return false;
		seen |= x;
	}
	return true;
}

static_assert(all_distinct_bits({
	formal::Comment,
	HTMLtag::Entity,
	undefined_SVO::postfix,
	undefined_SVO::infix,
	undefined_SVO::prefix,
	gentzen::symbol_catalog::anchor_is_symbol,
	gentzen::symbol_catalog::anchor_is_const_symbol,
	gentzen::symbol_catalog::anchor_is_not_symbol,
	gentzen::used_as_placeholder,
	gentzen::parsed_substitution,
	formal::Recognized,
	HTMLtag::Tag,
	formal::RequestNormalization,
	formal::Tokenized,
	formal::Inert_Token,
	formal::Error
	}), "reserved word/lex_node code flags collide");

class defined_SVO {
private:
	// we have seven theoretical kinds of undefined sentences, based on where the infix/postfix/prefix text is.
	static std::unique_ptr<defined_SVO>& _get()
	{
		static std::unique_ptr<defined_SVO> oaoo;
		if (!oaoo) {
			oaoo = decltype(oaoo)(new decltype(oaoo)::element_type());
		}
		return oaoo;
	}

	defined_SVO() = default;
public:
	// R5: a defined notation.  The phrase is stored as an undefined_SVO::_postfix entry is --
	// a lex_node anchored at its first word with the remaining tokens in postfix() -- and,
	// since a registered definiendum is a closed phrase (step 5: no placeholder variables),
	// there is no slot machinery.  Lookup is notational in both directions, never by id.
	struct entry {
		formal::lex_node phrase;
		perl::scalar phrase_notation;
		gentzen::statement_t definiens;
		perl::scalar definiens_notation;
		gentzen::statement_t definition;	// the := statement itself, as recorded
		std::optional<gentzen::statement_t> source;	// the axiom it was derived from, when the deriver says (saturation does)
	};
private:
	std::vector<entry> _entries;
public:
	static defined_SVO& get() { return *_get(); }

	defined_SVO(const defined_SVO&) = delete;
	defined_SVO(defined_SVO&&) = delete;
	defined_SVO& operator=(const defined_SVO&) = delete;
	defined_SVO& operator=(defined_SVO&&) = delete;
	~defined_SVO() = default;

	size_t size() const { return _entries.size(); }
	const entry& operator[](size_t n) const { return _entries[n]; }

	// phrase -> its definition (expansion)
	const entry* by_phrase(std::string_view notation) const {
		for (decltype(auto) x : _entries) if (notation == x.phrase_notation.view()) return &x;
		return nullptr;
	}

	// definiens notation -> the phrase that names it (W4's rationale direction)
	const entry* by_definiens(std::string_view notation) const {
		for (decltype(auto) x : _entries) if (notation == x.definiens_notation.view()) return &x;
		return nullptr;
	}

	// registers a definition whose left-hand side is a closed phrase; idempotent for the
	// same definiens, an error for a different one.  Defined after
	// definiendum_placeholder_census, which it relies on.
	std::optional<perl::scalar> declare(const gentzen::statement_t& definition);

	// records where a registered definition came from (the schema axiom, for saturation);
	// rendering its name then counts as a use of that axiom.  Keyed by the definition's
	// notation; false if no such definition is registered.
	bool attribute(const gentzen::statement_t& definition, const gentzen::statement_t& source) {
		const auto notation = definition.to_s();
		for (decltype(auto) x : _entries) {
			if (notation == x.definition.to_s()) {
				x.source = source;
				return true;
			}
		}
		return false;
	}
};

// step-3 support: substitutes through parsed-arm membranes the generic
// placeholder-handle walk cannot mutate (cf. placeholder_match::aux_trees).  Two arm
// kinds are dispatched (fork A(a); the A(b) unification into one virtual is deferred to
// stage 6): phrase_postfix subjects, and syntactical_entailment_2ary definiens (via its
// rename-vector apply, which reparses its arms).  The carrying node's code is preserved.
// Returns false when a substitution failed, leaving the tree partly substituted: the caller
// must then discard the whole tree.  The two arm kinds differ in what a null result means.
// phrase_postfix returns null when it has nothing to replace, which is not a failure.
// syntactical_entailment_2ary always rebuilds all three arms, so its null *is* a failure
// (an arm did not survive -- e.g. "arity would be increased"); falling through there would
// keep the unsubstituted arm in an otherwise substituted statement.
static bool substitute_parsed_anchors(formal::lex_node& node, const gentzen::prepared_rename& rename);

static bool substitute_parsed_anchors(kuroda::parser<formal::lex_node>::symbols& x, const gentzen::prepared_rename& rename)
{
	for (size_t i = 0; i < x.size(); ++i) {
		formal::lex_node*& s = x[i];
		if (!s) continue;
		if (auto anchor = s->c_anchor<formal::parsed>()) {
			if (auto pp = dynamic_cast<const undefined_SVO::phrase_postfix*>(anchor)) {
				if (auto replaced = pp->apply_uniform_substitution(rename)) {
					auto swap_in = new formal::lex_node(static_cast<formal::parsed*>(replaced), s->code());
					delete s;
					s = swap_in;
					continue;
				}
			} else if (auto e2 = dynamic_cast<const gentzen::syntactical_entailment_2ary*>(anchor)) {
				auto replaced = e2->apply_uniform_substitution(rename);
				if (!replaced) return false;
				auto swap_in = new formal::lex_node(static_cast<formal::parsed*>(replaced.release()), s->code());
				delete s;
				s = swap_in;
				continue;
			}
		}
		if (!substitute_parsed_anchors(*s, rename)) return false;
	}
	return true;
}

static bool substitute_parsed_anchors(formal::lex_node& node, const gentzen::prepared_rename& rename)
{
	if (!substitute_parsed_anchors(node.prefix(), rename)) return false;
	if (!substitute_parsed_anchors(node.infix(), rename)) return false;
	if (!substitute_parsed_anchors(node.postfix(), rename)) return false;
	for (decltype(auto) frag : node.fragments()) if (!substitute_parsed_anchors(frag, rename)) return false;
	return true;
}

// Step 4: after a uniform substitution, reparse each lex-side container that received a
// replacement for a symbol-placeholder variable (the := prefix [modus ponens for _] ->
// [modus ponens for &#9500;]); a placeholder-to-placeholder replacement leaves the parse
// unchanged and is not tracked.  Bottom-up, so an inner container settles before its
// parent's.  The default reparse routing is used: the Kuroda rule that flags &#9500; at a
// right edge is in GentzenGrammar, and its error is deferred (record_error), so callers
// flush with report_diagnostics.  Membership is by pointer identity against the
// replacement nodes; nothing in `replaced` is dereferenced.
//
// The left-hand side of a definition node is the exception (cf. C++ SFINAE: the failed
// substitution is dropped, not merely its message).  The reparse runs on a deep copy; if
// that leaves a pending (deferred) error, the copy is discarded along with its messages
// and the unparsed token run is kept, so a technical-English definiendum stays matchable
// as a phrase.  If the copy parses cleanly (a genuine formula, A _ B -> A &#9500; B) it is
// adopted.  This is sound here, where it would not be for freshly parsed text: the source
// statement parsed clean, so any error in this container was introduced by replacing a
// symbol-placeholder variable with a symbol.  An error reported *immediately* during the
// trial cannot be retracted; the copy is then adopted so the structure matches the report.
static void reparse_substituted_containers(formal::lex_node& node, const std::vector<const formal::lex_node*>& replaced)
{
	auto pass = [&](kuroda::parser<formal::lex_node>::symbols& x, bool is_definiendum) {
		bool want = false;
		for (decltype(auto) s : x) {
			if (!s) continue;
			reparse_substituted_containers(*s, replaced);
			if (!want) for (decltype(auto) r : replaced) if (r == s) { want = true; break; }
		}
		if (!want) return;
		if (!is_definiendum) {
			gentzen::reparse(x);
			return;
		}

		const auto prior_errors = Errors.count();
		kuroda::parser<formal::lex_node>::symbols trial(x);	// deep copy
		gentzen::reparse(trial);
		bool failed = false;
		if (prior_errors == Errors.count()) {
			for (decltype(auto) s : trial) {
				if (s && gentzen::has_pending_error(*s)) {
					failed = true;
					break;
				}
			}
		}
		if (failed) {
			// keep the token run; the replacements had their symbol classification cleared
			// for the reparse that is not happening, so classify them now
			for (decltype(auto) s : x) if (s) gentzen::symbol_catalog::anchor_symbol_parse(*s);
			return;
		}
		x = std::move(trial);
	};
	const bool is_definition = gentzen::is_definition_node(node);
	pass(node.prefix(), is_definition);
	pass(node.infix(), false);
	pass(node.postfix(), false);
	for (decltype(auto) frag : node.fragments()) pass(frag, false);
}

// Step 3 of the instantiation mechanics (Notes/GentzenW-defined-notations.md): apply
// [d \ s] uniformly to a freshly-cloned copy of src, covering lex-side placeholder
// occurrences AND parsed-arm membranes (phrase_postfix subjects, nested
// syntactical_entailment_2ary definiens).  Unlike
// syntactical_entailment_2ary::apply_uniform_substitution, a non-empty aux_trees is
// expected here (parsed-arm census descent); those handles point into discarded
// clones, and the parsed-anchor pass performs the real parsed-side substitution.
// Parallel implementation to the 2ary member; evaluate a shared base once stable
// (fork A(b), stage 6).
static formal::lex_node* apply_uniform_substitution_deep(const formal::lex_node& src, const formal::lex_node& subst)
{
	std::vector<perl::scalar> symbol_sources;	// sources whose replacement changes the parse
	const auto prepared = gentzen::prepared_rename::decode(subst, &symbol_sources);
	if (!prepared) return nullptr;
	decltype(auto) rename = prepared->view();	// destinations are prepared prototypes; copies need no further fixup

	std::unique_ptr<formal::lex_node> stage(new formal::lex_node(src));

	// a bare placeholder variable has no parent slot for a handle
	if (stage->code() & gentzen::used_as_placeholder) {
		const auto key = stage->to_scalar();
		for (decltype(auto) x : rename) {
			if (x.first.view() == key.view()) return new formal::lex_node(*x.second);
		}
		return stage.release();
	}

	formal::placeholder_match match;
	match.tree = std::move(stage);
	gentzen::collect_placeholder_handles()(match);
	std::vector<const formal::lex_node*> replaced;	// symbol-sourced replacements, for the container reparse
	for (decltype(auto) g : match.groups) {
		const formal::lex_node* to = nullptr;
		for (decltype(auto) x : rename) {
			if (x.first.view() == g.first.view()) {
				to = x.second;
				break;
			}
		}
		if (!to) continue;
		bool track = false;
		for (decltype(auto) x : symbol_sources) {
			if (x.view() == g.first.view()) {
				track = true;
				break;
			}
		}
		for (decltype(auto) h : g.second) {
			auto swap_in = new formal::lex_node(*to);
			if (track) replaced.push_back(swap_in);
			h.replace(swap_in);
		}
	}

	if (!substitute_parsed_anchors(*match.tree, *prepared)) return nullptr;	// partly substituted: discard
	if (!replaced.empty()) reparse_substituted_containers(*match.tree, replaced);
	return match.tree.release();
}

// SFINAE for uniform substitution: an instantiation that fails is not an error, it is no
// instance.  Runs apply_uniform_substitution_deep under a diagnostic_capture, flushes the
// result's deferred messages into the same capture, and keeps the instance only if no error
// was captured.  The diagnostics come back as a value either way: config-time saturation
// drops a discarded instance's, while a caller acting on a user's request can replay() them;
// for a kept instance they can only be warnings.  instance is null on failure, including a
// failure that reported nothing (e.g. subst is not a substitution node).
struct instantiation_result {
	std::unique_ptr<formal::lex_node> instance;
	captured_diagnostics diagnostics;
};

static instantiation_result try_instantiate(const formal::lex_node& src, const formal::lex_node& subst)
{
	instantiation_result ret;
	{
		diagnostic_capture capture;
		ret.instance.reset(apply_uniform_substitution_deep(src, subst));
		if (ret.instance) gentzen::report_diagnostics(*ret.instance);
		ret.diagnostics = capture.get();
	}
	if (!ret.diagnostics.errors.empty()) ret.instance.reset();
	return ret;
}

// Step 5 of the instantiation mechanics: modus ponens detachment.  From H |- C, where H is
// (notationally) an established axiom, infer C.  In the general case this is a value-copy
// of the conclusion, grouping parentheses stripped -- no substitution machinery runs.
// Returns null when src is not a unary syntactical entailment, carries a pending error, or
// its hypothesis is not established.  "Established" means a configured axiom for now:
// config-time saturation needs no more, and lemmas have no pure notational lookup yet
// (notation_ids::allocate mints an id as a side effect).
static formal::lex_node* detach_conclusion(const formal::lex_node& src)
{
	if (!gentzen::is_unary_syntactical_entailment(src)) return nullptr;
	if (gentzen::has_pending_error(src)) return nullptr;
	const auto hypothesis = src.prefix().front()->to_s();
	if (!gentzen::axioms::get()->disambiguate(hypothesis) && !gentzen::lemmas::get()->disambiguate(hypothesis)) return nullptr;

	formal::lex_node* ret = new formal::lex_node(*src.postfix().front());
	remove_grouping_parentheses(ret);
	return ret;
}

// How many distinct placeholder variables the left-hand side of a definition node mentions;
// nullopt when src is not a definition node.  Zero means the definiendum is a closed phrase
// -- the precondition for registering it as a constant defined notation.
static std::optional<size_t> definiendum_placeholder_census(const formal::lex_node& src)
{
	if (!gentzen::is_definition_node(src)) return std::nullopt;
	// the census walker takes mutable handles (dropped here unused), so it runs on a copy
	kuroda::parser<formal::lex_node>::symbols probe(src.prefix());	// deep copy
	formal::placeholder_match census;
	gentzen::collect_placeholder_handles walker;
	walker.dest_ = &census;
	walker(probe);
	return census.groups.size();
}

std::optional<perl::scalar> defined_SVO::declare(const gentzen::statement_t& definition)
{
	auto n = std::get_if<std::shared_ptr<const formal::lex_node> >(&definition);
	if (!n || !*n) return "definition is not a lex_node statement";
	const formal::lex_node& node = **n;
	auto census = definiendum_placeholder_census(node);
	if (!census) return "not a definition";
	if (0 < *census) return "definiendum is not a closed phrase";

	// the phrase in _postfix-entry form: first token anchors, the rest go to postfix()
	kuroda::parser<formal::lex_node>::symbols tokens(node.prefix());	// deep copy
	formal::lex_node phrase(*tokens[0]);
	tokens.DeleteIdx(0);
	if (!tokens.empty()) phrase.set_postfix(std::move(tokens));
	perl::scalar phrase_notation(phrase.to_s());

	gentzen::statement_t definiens(std::shared_ptr<const formal::lex_node>(new formal::lex_node(*node.postfix().front())));
	perl::scalar definiens_notation(definiens.to_s());

	if (auto prior = by_phrase(phrase_notation.view())) {
		if (prior->definiens_notation.view() == definiens_notation.view()) return std::nullopt;	// the same definition again
		return std::string(phrase_notation.view()) + " is already defined as " + std::string(prior->definiens_notation.view());
	}
	_entries.push_back(entry{std::move(phrase), std::move(phrase_notation), std::move(definiens), std::move(definiens_notation), definition});
	return std::nullopt;
}

std::optional<perl::scalar> gentzen::defined_name_for(const gentzen::statement_t& schema)
{
	if (auto e = defined_SVO::get().by_definiens(schema.to_s())) return e->phrase_notation;
	return std::nullopt;
}

// R4: what the root lemmas instance runs before recording.  A definition with a closed
// definiendum is registered as a defined notation; anything else passes through untouched.
std::optional<perl::scalar> gentzen::before_add_lemma(const gentzen::statement_t& src)
{
	auto n = std::get_if<std::shared_ptr<const formal::lex_node> >(&src);
	if (!n || !*n) return std::nullopt;
	auto census = definiendum_placeholder_census(**n);
	if (!census || 0 < *census) return std::nullopt;
	return defined_SVO::get().declare(src);
}

// R3: config-time saturation of conditional definition axioms, in both directions.  A
// *schema* is a unary syntactical entailment whose hypothesis is a phrase_postfix over a
// symbol-placeholder variable ("_ is a binary infix symbol &#9500; (...)"); a *declaration*
// is any phrase_postfix axiom ("&#9500; is a binary infix symbol").  Each new schema is
// saturated against every declaration seen so far, and each new declaration against every
// schema, so the outcome does not depend on their order in core.yaml; axioms::watched_by
// replays the axioms already loaded, so registering late equals registering early.
// Per pair: match (step 2; the matcher rejects a declaration of the wrong kind),
// try_instantiate (steps 3-4; SFINAE, so a failed instantiation is no instance, not an
// error), detach (step 5), and record into the root lemmas instance.  Every instance that
// instantiates cleanly is recorded, established definiens or not (resolved decision 4: the
// unestablished ones signal that provability should be checked).  R4's before-add-lemma
// stage, in lemmas::record, is what will hand a recorded definition to defined_SVO.
class saturation final : public zaimoni::observer<gentzen::statement_t> {
	std::vector<std::shared_ptr<const formal::lex_node> > _schemas;
	std::vector<std::shared_ptr<const undefined_SVO::phrase_postfix> > _declarations;

	saturation() = default;
public:
	saturation(const saturation&) = delete;
	saturation(saturation&&) = delete;
	saturation& operator=(const saturation&) = delete;
	saturation& operator=(saturation&&) = delete;
	~saturation() = default;

	static std::shared_ptr<saturation> get() {
		static std::shared_ptr<saturation> ooao(new saturation());
		return ooao;
	}

	// the hypothesis that makes x a schema, or null
	static const undefined_SVO::phrase_postfix* schema_hypothesis(const formal::lex_node& x) {
		if (!gentzen::is_unary_syntactical_entailment(x)) return nullptr;
		auto h = dynamic_cast<const undefined_SVO::phrase_postfix*>(x.prefix().front()->c_anchor<formal::parsed>());
		if (!h || !gentzen::is_symbol_placeholder_syntax_symbol(h->subject().get())) return nullptr;
		return h;
	}

	// one (schema, declaration) pair: the recorded lemma, if the pair yields one
	static std::optional<std::pair<gentzen::statement_t, size_t> > saturate(const formal::lex_node& schema, const undefined_SVO::phrase_postfix& decl) {
		enum { trace_saturation = 0 };
		if constexpr (trace_saturation) std::cerr << "saturate: " << decl.to_s() << "\n";
		auto hypothesis = schema_hypothesis(schema);
		if (!hypothesis) return std::nullopt;
		auto subst = hypothesis->get_uniform_substitution(decl);
		if (!subst) return std::nullopt;
		if constexpr (trace_saturation) std::cerr << "  subst: " << subst->to_s() << "\n";
		auto instance = try_instantiate(schema, *subst);
		if (!instance.instance) return std::nullopt;
		if constexpr (trace_saturation) std::cerr << "  instance: " << instance.instance->to_s() << "\n";
		std::shared_ptr<const formal::lex_node> conclusion(detach_conclusion(*instance.instance));
		if (!conclusion) return std::nullopt;
		if constexpr (trace_saturation) std::cerr << "  conclusion: " << conclusion->to_s() << "\n";
		// no conditional is introduced on the way, so this is the rationale
		auto ret = gentzen::lemmas::get()->record(gentzen::statement_t(conclusion), perl::scalar(std::string_view("immediate from non-explosive axioms")));
		if constexpr (trace_saturation) std::cerr << "  recorded " << ret.second << "\n";
		// the definition, if it registered, descends from the schema axiom: a theorem that
		// renders its name has used that axiom
		if (auto own = gentzen::axioms::get()->disambiguate(schema.to_s())) defined_SVO::get().attribute(ret.first, own->first);
		return ret;
	}

	bool onNext(const gentzen::statement_t& value) override {
		enum { trace_saturation = 0 };
		if constexpr (trace_saturation) std::cerr << "onNext: " << value.to_s() << "\n";
		if (auto p = std::get_if<std::shared_ptr<const formal::parsed> >(&value)) {
			auto decl = std::dynamic_pointer_cast<const undefined_SVO::phrase_postfix>(*p);
			if (!decl) return true;
			_declarations.push_back(decl);
			for (decltype(auto) schema : _schemas) saturate(*schema, *decl);
		} else if (auto n = std::get_if<std::shared_ptr<const formal::lex_node> >(&value)) {
			if (!*n || !schema_hypothesis(**n)) return true;
			if constexpr (trace_saturation) std::cerr << "  schema; declarations seen " << _declarations.size() << "\n";
			_schemas.push_back(*n);
			for (decltype(auto) decl : _declarations) saturate(**n, *decl);
		}
		return true;
	}
};

// scaffold: exercises phrase_postfix::get_uniform_substitution against the configured
// infix-symbol declaration axiom.  Referenced from main() behind the same
// compile-time switch as the other prototype self-tests; placed here because
// undefined_SVO must be complete.
static void self_test_phrase_matcher()
{
	const undefined_SVO::phrase_postfix* infix_decl = nullptr;
	decltype(auto) ax = gentzen::axioms::get();
	size_t ub = ax->size();
	while (0 < ub) {
		auto stmt = ax->rationale(--ub).first.first;
		if (auto p = std::get_if<std::shared_ptr<const formal::parsed> >(&stmt)) {
			if (auto q = dynamic_cast<const undefined_SVO::phrase_postfix*>(p->get())) {
				if ("&#9500; is a binary infix symbol" == q->to_s()) {
					infix_decl = q;
					break;
				}
			}
		}
	}
	if (!infix_decl) {
		std::cout << "phrase matcher: infix declaration axiom not found\n";
		return;
	}

	// symbol-classification bits on each declaring axiom's subject.  Observed 2026-09-19: the
	// *first* declaring axiom for a symbol (:=, &#9500; infix, /, &, ",") cached
	// anchor_is_not_symbol on its subject, because it is parsed before its own declaration
	// takes effect; a later one (&#9500; is a unary prefix symbol) cached anchor_is_symbol.
	// Fixed 2026-09-20 in phrase_postfix::before_add_axiom_handler, which re-classifies the
	// subject once the declaration has taken effect: every declaring subject now reads
	// "symbol", and the test_preconditions checks in prepared_rename::decode pass.
	// Kept as the regression probe for that fix.
	enum { trace_subject_codes = 0 };
	if constexpr (trace_subject_codes) {
		for (size_t i = 0; i < ax->size(); ++i) {
			auto stmt = ax->rationale(i).first.first;
			if (auto p = std::get_if<std::shared_ptr<const formal::parsed> >(&stmt)) {
				if (auto q = dynamic_cast<const undefined_SVO::phrase_postfix*>(p->get())) {
					if (!q->subject()) continue;
					const auto code = q->subject()->code();
					std::cout << "subject code " << i << ": " << q->to_s() << " -> " << std::hex << code << std::dec
						<< (code & gentzen::symbol_catalog::anchor_is_symbol ? " symbol" : "")
						<< (code & gentzen::symbol_catalog::anchor_is_const_symbol ? " const_symbol" : "")
						<< (code & gentzen::symbol_catalog::anchor_is_not_symbol ? " NOT_symbol" : "")
						<< ", catalog now says " << (gentzen::symbol_catalog::recognize(*q->subject()) ? "symbol" : "not a symbol") << "\n";
				}
			}
		}
	}

	auto subject_leaf = []() {
		return new formal::lex_node(new formal::word(std::string_view("_"), formal::src_location(), 0), 0);
	};

	formal::lex_node* subject = subject_leaf();
	undefined_SVO::phrase_postfix hyp(subject, undefined_SVO::phrase_postfix::hard_code::infix_symbol);
	auto subst = hyp.get_uniform_substitution(*infix_decl);
	std::cout << "phrase matcher [symbol / _]: " << (subst ? subst->to_s() : "(null)") << "\n";

	formal::lex_node* subject2 = subject_leaf();
	undefined_SVO::phrase_postfix wrong_kind(subject2, undefined_SVO::phrase_postfix::hard_code::prefix_symbol);
	std::cout << "phrase matcher kind mismatch: " << (wrong_kind.get_uniform_substitution(*infix_decl) ? 1 : 0) << "\n";

	// step 3: apply the matcher's own [&#9500; / _] to the parsed conditional axiom;
	// all three _ occurrences (phrase subject, definiendum, definiens) must substitute
	// Step 4: the := prefix is trial-reparsed (reparse_substituted_containers).  For the
	// technical-English definiendum the trial fails, so it is dropped with its deferred
	// error: errors +0 and the 4-token phrase run survives.  The formula-LHS control parses
	// cleanly, so its trial is adopted: errors +0 and the prefix folds to one node.
	auto instantiate = [&](const char* label, const char* text) {
		std::vector<std::string> src_lines;
		src_lines.push_back(text);
		formal::src_location loc(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(std::string("self-test:") + label)));
		auto lines = to_lines(src_lines, loc);
		if (!lines.empty()) {
			auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
			TokenGrammar().complete_parse(stage);
			stage = GentzenGrammar().apply(stage);
			GentzenGrammar().complete_parse(stage);
			if (1 == stage.size() && stage[0]) {
				const auto before = Errors.count();
				std::unique_ptr<formal::lex_node> instantiated(apply_uniform_substitution_deep(*stage[0], *subst));
				// all three _ occurrences substitute: hypothesis phrase subject,
				// definiendum token, and the definiens 2ary arm (reparsed to fold P |- Q).
				if (instantiated) gentzen::report_diagnostics(*instantiated);	// the pending error is in the result, not the source
				std::cout << label << " instantiation: " << (instantiated ? instantiated->to_s() : "(null)")
					<< ", errors +" << (Errors.count() - before);
				if (instantiated && 1 == instantiated->postfix().size() && gentzen::is_definition_node(*instantiated->postfix().front())) {
					std::cout << ", := prefix entries " << instantiated->postfix().front()->prefix().size();
				}
				std::cout << "\n";

				// step 5: the conditional axiom itself does not detach (its hypothesis
				// "_ is a binary infix symbol" is no axiom); the instantiation does, to a
				// definition whose left-hand side mentions no placeholder variables
				std::unique_ptr<formal::lex_node> undetachable(detach_conclusion(*stage[0]));
				std::unique_ptr<formal::lex_node> detached(instantiated ? detach_conclusion(*instantiated) : nullptr);
				std::cout << label << " detachment: schema " << (undetachable ? "DETACHED (wrong)" : "refused")
					<< "; instance " << (detached ? detached->to_s() : "(null)");
				if (detached) {
					if (auto census = definiendum_placeholder_census(*detached)) std::cout << ", definiendum placeholders " << *census;
					if (auto census = definiendum_placeholder_census(*stage[0]->postfix().front())) std::cout << " (schema's: " << *census << ")";
					// the registry resolves the definiens notationally, never by baked-in id
					if (gentzen::is_definition_node(*detached) && 1 == detached->postfix().size()) {
						if (auto ax = gentzen::axioms::get()->disambiguate(detached->postfix().front()->to_s())) std::cout << ", definiens is axiom #" << ax->second;
						else std::cout << ", definiens matches no axiom";
					}
				}
				std::cout << "\n";
				enum { trace_structure = 0 };	// per-node shape dump of source and result
				if constexpr (trace_structure) {
					auto shape = [](const formal::lex_node& n, unsigned depth, auto& self) -> void {
						std::string pad(2 * depth, ' ');
						std::cout << pad << "node anchor " << n.is_pure_anchor() << " code " << std::hex << n.code() << std::dec
							<< " prefix " << n.prefix().size() << " infix " << n.infix().size() << " postfix " << n.postfix().size()
							<< " fragments " << n.fragments().size() << ": " << n.to_s() << "\n";
						for (decltype(auto) x : n.prefix()) if (x) self(*x, depth + 1, self);
						for (decltype(auto) x : n.infix()) if (x) self(*x, depth + 1, self);
						for (decltype(auto) x : n.postfix()) if (x) self(*x, depth + 1, self);
						for (decltype(auto) f : n.fragments()) for (decltype(auto) x : f) if (x) self(*x, depth + 1, self);
					};
					std::cout << label << " source shape:\n";
					shape(*stage[0], 1, shape);
					if (instantiated) {
						std::cout << label << " result shape:\n";
						shape(*instantiated, 1, shape);
					}
				}
			} else {
				std::cout << label << ": conditional axiom did not parse to a single statement\n";
			}
		}
	};
	if (subst) {
		instantiate("step-3", "_ is a binary infix symbol &#9500; (modus ponens for _ := (P _ Q), P &#9500; Q)");
		instantiate("step-3 formula LHS", "_ is a binary infix symbol &#9500; (A _ B := (P _ Q), P &#9500; Q)");
	}

	// SFINAE, as R3's saturation will use it: try_instantiate the conditional axiom against
	// every declaring axiom its hypothesis matches.  A failed instantiation is discarded with
	// its diagnostics captured rather than reported, so errors +0 on every line.  The case
	// that motivates it is [, / _]: the definiens arm (P _ Q) becomes the tuple (P , Q), which
	// syntactical_entailment_2ary's rename-apply rejects ("arity would be increased").
	{
		std::vector<std::string> src_lines;
		src_lines.push_back("_ is a binary infix symbol &#9500; (modus ponens for _ := (P _ Q), P &#9500; Q)");
		formal::src_location loc(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path("self-test:sfinae")));
		auto lines = to_lines(src_lines, loc);
		auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
		TokenGrammar().complete_parse(stage);
		stage = GentzenGrammar().apply(stage);
		GentzenGrammar().complete_parse(stage);
		if (1 == stage.size() && stage[0]) {
			for (size_t i = 0; i < ax->size(); ++i) {
				auto stmt = ax->rationale(i).first.first;
				auto p = std::get_if<std::shared_ptr<const formal::parsed> >(&stmt);
				if (!p) continue;
				auto q = dynamic_cast<const undefined_SVO::phrase_postfix*>(p->get());
				if (!q) continue;
				auto with = hyp.get_uniform_substitution(*q);
				if (!with) continue;	// not a declaration the hypothesis matches

				const auto before = Errors.count();
				auto result = try_instantiate(*stage[0], *with);
				std::cout << "sfinae " << with->to_s() << ": ";
				if (result.instance) {
					std::unique_ptr<formal::lex_node> detached(detach_conclusion(*result.instance));
					std::cout << "kept, " << (detached ? detached->to_s() : "(not detachable)");
				} else std::cout << "discarded";
				std::cout << ", captured errors " << result.diagnostics.errors.size();
				if (!result.diagnostics.errors.empty()) std::cout << " (" << result.diagnostics.errors.front().second.view() << ")";

				// captures nest, and replay() re-issues into whichever is active
				if (!result.diagnostics.errors.empty()) {
					diagnostic_capture outer;
					replay(result.diagnostics);
					std::cout << ", replayed into an enclosing capture " << outer.get().errors.size();
				}
				std::cout << ", errors +" << (Errors.count() - before) << "\n";
			}
		}
	}
}

// scaffold: exercises the step-4 diagnostic walks (record -> discard cancels,
// record -> report flushes) on a hand-built subtree, independent of any reparse site.
static void self_test_diagnostic_walks()
{
	auto leaf = [](const char* name) {
		return new formal::lex_node(new formal::word(std::string_view(name), formal::src_location(), 0), 0);
	};
	std::unique_ptr<formal::lex_node> root(gentzen::make_binary_node(std::string_view("&#9500;"), 0,
		std::unique_ptr<formal::lex_node>(leaf("A")),
		std::unique_ptr<formal::lex_node>(leaf("B"))));
	formal::lex_node* deep = root->prefix().front();	// the "A" leaf

	deep->record_error(perl::scalar(std::string_view("deferred test error")));
	std::cout << "diag: pending after record " << gentzen::has_pending_diagnostic(*deep) << "\n";

	auto before = Errors.count();
	gentzen::discard_diagnostics(*root);
	std::cout << "diag: pending after discard " << gentzen::has_pending_diagnostic(*deep)
		<< ", errors +" << (Errors.count() - before) << "\n";

	deep->record_error(perl::scalar(std::string_view("deferred test error")));
	before = Errors.count();
	gentzen::report_diagnostics(*root);
	std::cout << "diag: pending after report " << gentzen::has_pending_diagnostic(*deep)
		<< ", errors +" << (Errors.count() - before)
		<< ", Error flag " << ((deep->code() & formal::Error) ? 1 : 0) << "\n";

	// depth 2 under clean ancestors -- (A |- B) |- C with the message on A.  A shallow
	// walk test never descends through the clean (A |- B) node; this is the shape of the
	// step-3 definiendum error (root -> := -> &#9500;).
	std::unique_ptr<formal::lex_node> outer(gentzen::make_binary_node(std::string_view("&#9500;"), 0,
		std::move(root),
		std::unique_ptr<formal::lex_node>(leaf("C"))));
	deep->forget(formal::Error);
	deep->record_error(perl::scalar(std::string_view("deferred test error, depth 2")));
	std::cout << "diag depth 2: pending from root " << gentzen::has_pending_diagnostic(*outer)
		<< ", pending error from root " << gentzen::has_pending_error(*outer) << "\n";
	before = Errors.count();
	gentzen::discard_diagnostics(*outer);
	std::cout << "diag depth 2: pending after discard " << gentzen::has_pending_diagnostic(*outer)
		<< ", errors +" << (Errors.count() - before) << "\n";
	deep->record_error(perl::scalar(std::string_view("deferred test error, depth 2")));
	before = Errors.count();
	gentzen::report_diagnostics(*outer);
	std::cout << "diag depth 2: pending after report " << gentzen::has_pending_diagnostic(*outer)
		<< ", errors +" << (Errors.count() - before)
		<< ", Error flag " << ((deep->code() & formal::Error) ? 1 : 0) << "\n";
}

// R2: lex_node axioms.  Exercises axioms::add_axiom(shared_ptr<const lex_node>)'s gates
// directly -- the loader's else-branch in undefined_SVO::get() is the same call, and is
// exercised by the configuration once the conditional axiom is appended (R7).  Runs last:
// a successful admission grows the global axiom list.
static void self_test_lex_axiom_admission()
{
	auto parse = [](const char* label, const char* text) -> std::shared_ptr<formal::lex_node> {
		std::vector<std::string> src_lines;
		src_lines.push_back(text);
		formal::src_location loc(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(std::string("self-test:") + label)));
		auto lines = to_lines(src_lines, loc);
		if (lines.empty()) return nullptr;
		auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
		TokenGrammar().complete_parse(stage);
		stage = GentzenGrammar().apply(stage);
		GentzenGrammar().complete_parse(stage);
		gentzen::report_diagnostics(stage);
		if (1 != stage.size() || !stage[0]) return nullptr;
		formal::lex_node* raw = stage[0];	// ownership moves out of the symbols container, as in the loader
		stage[0] = nullptr;
		return std::shared_ptr<formal::lex_node>(raw);
	};

	decltype(auto) ax = gentzen::axioms::get();
	decltype(auto) L = gentzen::lemmas::get();
	// dispatches as the loader does: a parsed statement through add_axiom(parsed), anything
	// else through add_axiom(lex_node).  R3: the saturation observer is live, so admitting a
	// schema, or a declaration after one, records lemmas; they are listed.
	auto admit = [&](const char* label, const char* text) {
		auto stmt = parse(label, text);
		if (!stmt) {
			std::cout << label << ": did not parse to a single statement\n";
			return;
		}
		const auto before = Errors.count();
		const auto size = ax->size();
		const auto lemmas = L->size();
		const auto defined = defined_SVO::get().size();
		if (auto relay = stmt->shared_anchor<formal::parsed>()) ax->add_axiom(relay);
		else ax->add_axiom(stmt);
		std::cout << label << ": " << stmt->to_s() << " -> axioms +" << (ax->size() - size) << ", errors +" << (Errors.count() - before);
		if (auto found = ax->disambiguate(stmt->to_s())) std::cout << ", notation id " << found->second;
		std::cout << ", lemmas +" << (L->size() - lemmas) << ", defined +" << (defined_SVO::get().size() - defined) << "\n";
		for (size_t i = lemmas; i < L->size(); ++i) {
			auto r = L->rationale(i);
			std::cout << "  lemma " << r.second << ": " << r.first.first.to_s() << " (" << r.first.second.view() << ")\n";
		}
		for (size_t i = defined; i < defined_SVO::get().size(); ++i) {
			decltype(auto) e = defined_SVO::get()[i];
			std::cout << "  defined: " << e.phrase.to_s() << " [" << e.phrase.postfix().size() << " postfix] := " << e.definiens.to_s() << "\n";
		}
	};
	admit("lex axiom, unrecognized", "A := B");
	admit("lex axiom, alters syntax", "A &#9500; (X is a binary infix symbol)");
	// since R7 this schema is configured, so this admission duplicates it: add_axiom does
	// not deduplicate (axioms +1), but saturation's lemmas and definitions do (+0, +0)
	admit("lex axiom, conditional definition (configured since R7)", "_ is a binary infix symbol &#9500; (modus ponens for _ := (P _ Q), P &#9500; Q)");
	// reverse direction: a declaration arriving after the schema; a plain word, so the
	// symbol catalog accepts it and nothing in the HTML fixture is affected
	admit("declaration after schema", "@ is a binary infix symbol");

	// R4/R5: the registry answers in both directions, notationally; re-recording the same
	// definition is idempotent (lemmas dedupe by notation before the stage even runs)
	decltype(auto) D = defined_SVO::get();
	if (auto e = D.by_phrase("modus ponens for &#9500;")) std::cout << "by phrase: " << e->phrase.to_s() << " := " << e->definiens.to_s() << "\n";
	else std::cout << "by phrase: not found\n";
	if (auto e = D.by_definiens("(P &#9500; Q) , P &#9500; Q")) std::cout << "by definiens: " << e->phrase.to_s() << " (definition: " << e->definition.to_s() << ")\n";
	else std::cout << "by definiens: not found\n";
	std::cout << "unknown phrase: " << (D.by_phrase("modus tollens for &#9500;") ? "found (wrong)" : "not found") << "\n";
	if (auto e = D.by_phrase("modus ponens for &#9500;")) {
		const auto before = Errors.count();
		const auto lemmas = L->size();
		const auto defined = D.size();
		auto again = L->record(e->definition);
		std::cout << "re-record: lemmas +" << (L->size() - lemmas) << ", defined +" << (D.size() - defined) << ", errors +" << (Errors.count() - before) << ", id " << again.second << "\n";
	}
}

// scaffold: step-4 reparse routing.  The same loose definiendum run
// [modus ponens for &#9500;] goes through reparse<false> (GentzenGrammar only) and
// reparse<true> (TokenGrammar first); the run is built as the instantiation builds it,
// by tokenizing "modus ponens for _" and swapping the _ for a tokenizer-made &#9500;.
// Observed 2026-09-19: both routings report "non-postfix symbol has no argument to
// right: &#9500;" (errors +1) from GentzenGrammar's symbol_catalog; the TokenGrammar
// stage is silent because argument_enforcer's reservation tables and syntax_check's
// handlers are both unpopulated.  The isolated run is what triggers the error: in the
// whole step-3 result &#9500; is followed by :=, so the definiendum must be split off
// (W1) before it can be flagged.
static void self_test_reparse_routing()
{
	auto build_run = [](const char* label) {
		kuroda::parser<formal::lex_node>::symbols ret;
		std::vector<std::string> src_lines;
		src_lines.push_back("modus ponens for _");
		src_lines.push_back("P &#9500; Q");
		formal::src_location loc(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(label)));
		auto lines = to_lines(src_lines, loc);
		ret = TokenGrammar().apply(formal::lex_node::pop_front(lines));
		TokenGrammar().complete_parse(ret);
		auto donor = TokenGrammar().apply(formal::lex_node::pop_front(lines));
		TokenGrammar().complete_parse(donor);
		if (4 != ret.size() || 3 != donor.size()) {
			std::cout << "reparse routing: unexpected tokenization " << ret.size() << ", " << donor.size() << "\n";
			ret.clear();
			return ret;
		}
		delete ret[3];
		ret[3] = donor[1];
		donor[1] = nullptr;
		return ret;
	};
	auto show = [](const kuroda::parser<formal::lex_node>::symbols& run) {
		std::string ret;
		for (decltype(auto) x : run) {
			if (!ret.empty()) ret += ' ';
			ret += x ? x->to_s() : "(null)";
		}
		return ret;
	};

	auto placeholder = std::unique_ptr<formal::lex_node>(new formal::lex_node(new formal::word(std::string_view("_"), formal::src_location(), 0), 0));
	std::cout << "reparse routing: is_symbol_placeholder_variable before tagging " << gentzen::is_symbol_placeholder_variable(*placeholder);
	placeholder->learn(gentzen::used_as_placeholder);
	std::cout << ", after tagging " << gentzen::is_symbol_placeholder_variable(*placeholder) << "\n";

	if (auto run = build_run("self-test:reparse<false>"); !run.empty()) {
		const auto before = Errors.count();
		gentzen::reparse<false>(run);
		gentzen::report_diagnostics(run);	// flush contract: the enforcement error is deferred
		std::cout << "reparse<false>: errors +" << (Errors.count() - before) << ", " << run.size() << " symbols: " << show(run) << "\n";
	}
	if (auto run = build_run("self-test:reparse<true>"); !run.empty()) {
		const auto before = Errors.count();
		gentzen::reparse<true>(run);
		gentzen::report_diagnostics(run);
		std::cout << "reparse<true>: errors +" << (Errors.count() - before) << ", " << run.size() << " symbols: " << show(run) << "\n";
	}
}

// main language syntax
static kuroda::parser<formal::lex_node>& GentzenGrammar() {
	static std::unique_ptr<kuroda::parser<formal::lex_node> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());

		ooao->register_terminal(gentzen::symbol_catalog::anchor_symbol_parse);
		ooao->register_terminal(parse_inside_balanced(reserved_atomic[0].first, reserved_atomic[1].first));
		ooao->register_terminal(parse_inside_balanced(reserved_atomic[2].first, reserved_atomic[3].first));
		ooao->register_terminal(parse_inside_balanced(reserved_atomic[4].first, reserved_atomic[5].first));

		ooao->register_global_build(undefined_SVO::global_parse);
		ooao->register_global_build(gentzen::symbol_catalog::global_parse);
		ooao->register_global_build(gentzen::syntactical_entailment_2ary::global_parse);
		ooao->register_global_build(gentzen::tag_syntactical_entailment::global_parse);
	};
	return *ooao;
}

// GentzenW proof session (work item #5 of the cut-rule plan).  A proof script is a
// YAML file (extension .yaml/.yml) with keys:
//   theorem: display name
//   given: sequence of top-level hypothesis statements (one introduction_start)
//   hypothesis: sequence of subordinate hypothesis statements (one introduction_start)
// Inference stepping is menu-driven via ask_user_to_pick_one, so regression runs are
// ordinary console sessions with redirected stdin.

// parses statement lines through the same pipeline as the core.yaml axioms
static std::optional<std::vector<gentzen::statement_t> > parse_statements(std::vector<std::string>& src_lines, formal::src_location& src)
{
	std::vector<gentzen::statement_t> ret;
	auto lines = to_lines(src_lines, src);
	while (!lines.empty()) {
		const auto prior_errors = Errors.count();
		auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
		src.line_pos.first++;
		src.line_pos.second = 0;
		if (prior_errors < Errors.count()) return std::nullopt;

		TokenGrammar().complete_parse(stage);
		if (prior_errors < Errors.count()) return std::nullopt;

		stage = GentzenGrammar().apply(stage);
		gentzen::report_diagnostics(stage);	// balanced groups parse (and may defer errors) during apply
		if (prior_errors < Errors.count()) return std::nullopt;

		GentzenGrammar().complete_parse(stage);
		gentzen::report_diagnostics(stage);
		if (prior_errors < Errors.count()) return std::nullopt;

		if (1 != stage.size()) {
			error_report(src, "proof-script statement did not parse to a single statement");
			return std::nullopt;
		}
		ret.push_back(gentzen::statement_t(stage[0]));
	}
	return ret;
}

// column classifier for the Gentzen/Fitch table: entailments render in the
// "Inference rule" column, other statements in the "Proposition" column
static bool is_entailment_statement(const gentzen::statement_t& src)
{
	if (auto p = std::get_if<std::shared_ptr<const formal::parsed> >(&src)) {
		return nullptr != dynamic_cast<const gentzen::syntactical_entailment_2ary*>(p->get());
	}
	auto n = std::get_if<std::shared_ptr<const formal::lex_node> >(&src);
	if (!n || !*n) return false;
	const formal::lex_node& ref = **n;
	if (!(ref.code() & gentzen::symbol_catalog::anchor_is_symbol)) return false;
	if (1 != ref.offset()) return false;	// &#9500;: cf. syntactical_entailment_2ary::can_construct
	return is_strict_binary_node(ref);
}

struct proof_row {
	size_t notation_id;
	bool entailment_column;
	std::string notation;
	std::string rationale;	// label(...) references carry raw notation ids; renumbering is the theorem writer's job
};

// Rewrites each label(<notation id>) expression whose id has a ledger row, to the row
// label (<row #>).  Notation ids need not be unique in general (first ledger row wins),
// but are unique for Gentzen-format proofs structured like the cut rule.  An id with no
// ledger row is left as its label(...) expression rather than silently mislabeled.
static std::string rewrite_labels(const std::string& rationale, const std::vector<proof_row>& rows)
{
	static constexpr const char* tag = "label(";
	static constexpr const size_t tag_len = 6;

	std::string ret;
	size_t pos = 0;
	while (true) {
		const auto hit = rationale.find(tag, pos);
		if (std::string::npos == hit) break;
		auto scan = hit + tag_len;
		size_t id = 0;
		bool have_digits = false;
		while (scan < rationale.size() && '0' <= rationale[scan] && '9' >= rationale[scan]) {
			id = id * 10 + size_t(rationale[scan] - '0');
			have_digits = true;
			++scan;
		}
		if (!have_digits || scan >= rationale.size() || ')' != rationale[scan]) {
			ret += rationale.substr(pos, scan - pos);
			pos = scan;
			continue;
		}

		size_t row_no = 0;
		{
			size_t n = 0;
			for (decltype(auto) r : rows) {
				++n;
				if (r.notation_id == id) {
					row_no = n;
					break;
				}
			}
		}

		ret += rationale.substr(pos, hit - pos);
		if (0 < row_no) {
			ret += '(';
			ret += std::to_string(row_no);
			ret += ')';
		} else {
			ret += rationale.substr(hit, scan + 1 - hit);
		}
		pos = scan + 1;
	}
	ret += rationale.substr(pos);
	return ret;
}

static void run_proof_script(const std::filesystem::path& path)
{
	using namespace gentzen;

	std::ifstream in(path);
	if (!in.is_open()) {
		std::cerr << path << ": cannot open\n";
		return;
	}
	formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(path)));
	fkyaml::node root;
	try {
		root = fkyaml::node::deserialize(in);
	} catch (const std::exception& e) {
		error_report(src, e.what());
		return;
	}

	std::string theorem_name("(unnamed)");
	if (root.contains("theorem")) fkyaml::from_node(root["theorem"], theorem_name);

	std::vector<std::string> given_lines;
	std::vector<std::string> hypothesis_lines;
	if (root.contains("given")) fkyaml::from_node(root["given"], given_lines);
	if (root.contains("hypothesis")) fkyaml::from_node(root["hypothesis"], hypothesis_lines);

	auto given = parse_statements(given_lines, src);
	if (!given) return;
	auto hypotheses = parse_statements(hypothesis_lines, src);
	if (!hypotheses) return;
	if (given->empty() && hypotheses->empty()) {
		error_report(src, "proof script has no given: or hypothesis: statements");
		return;
	}

	std::vector<proof_row> rows;
	std::vector<std::shared_ptr<fact_database> > steps;	// pins derivation records for the theorem writer
	std::vector<std::pair<std::string, size_t> > schemas_used;	// base rules applied: (notation, notation id)

	std::shared_ptr<fact_database> tip = lemmas::get();	// previously proven theorems stay visible

	// this iteration handles what the introduction_start overloads do: 1 or 2 statements per level
	auto open_level = [&](std::vector<statement_t>& stmts, const char* label) -> bool {
		std::shared_ptr<syntactical_entailment_introduction_start> level;
		if (1 == stmts.size()) level = syntactical_entailment_introduction_start::construct(stmts[0], tip);
		else if (2 == stmts.size()) level = syntactical_entailment_introduction_start::construct(stmts[0], stmts[1], tip);
		if (!level) {
			error_report(src, "this iteration only supports 1 or 2 statements per given:/hypothesis: level");
			return false;
		}
		const size_t s = level->size();
		for (size_t n = 0; n < s; ++n) {
			auto row = level->rationale(n);
			rows.push_back(proof_row{row.second, is_entailment_statement(row.first.first), row.first.first.to_s(), label});
		}
		tip = level;
		return true;
	};

	if (!given->empty() && !open_level(*given, "Given")) return;
	if (!hypotheses->empty() && !open_level(*hypotheses, "Hypothesis")) return;

	const std::string title("prove: " + theorem_name);
	statement_t last_discharged;
	bool complete = false;

	while (!complete) {
		auto apps = enumerate_applicable_2ary(tip);

		// probe dischargeability up front (the unique_ptr variant makes an unpicked probe
		// leak-free), and suppress re-assuming a statement that is already visible, e.g.
		// discharging to the Given P &#9500; Q.  P &#9500; P stays offered: it is not a
		// configured axiom.
		auto discharge = syntactical_entailment_introduction::construct(tip);
		bool offer_discharge = false;
		if (discharge) {
			struct notation_of {
				std::string operator()(const std::unique_ptr<syntactical_entailment_introduction>& x) const { return x->rationale(0).first.first.to_s(); }
				std::string operator()(const std::unique_ptr<syntactical_entailment_2ary>& x) const { return x->to_s(); }
			};
			const auto notation = std::visit(notation_of(), *discharge);
			offer_discharge = true;
			for (decltype(auto) v : visible_statements(tip)) {
				if (v.notation == notation) {
					offer_discharge = false;
					break;
				}
			}
		}

		std::vector<std::string> entries;
		for (decltype(auto) a : apps) entries.push_back("infer " + a.inferred.to_s() + " via " + a.rule->to_s());
		if (offer_discharge) entries.push_back("discharge current hypothesis level");

		if (entries.empty()) {
			std::cout << "no applicable inferences or discharges; proof of " << theorem_name << " abandoned\n";
			return;
		}

		size_t picked = std::numeric_limits<size_t>::max();
		const auto choice = ask_user_to_pick_one(title, entries, [&picked](size_t i) {
			picked = i;
			return std::optional<bool>(true);
		});
		if (!choice) {
			std::cout << "proof of " << theorem_name << " abandoned\n";
			return;
		}

		if (picked < apps.size()) {	// forward inference
			decltype(auto) a = apps[picked];
			auto next = syntactical_entailment_2ary_infer::construct(a.rule, a.inferred, a.hypothesis_ids, tip, a.schema);
			if (!next) {
				error_report(src, "syntactical_entailment_2ary_infer::construct failed");
				return;
			}
			tip = std::shared_ptr<fact_database>(next);
			auto row = tip->rationale(0);
			rows.push_back(proof_row{row.second, is_entailment_statement(row.first.first), row.first.first.to_s(), std::string(row.first.second.view())});

			auto schema_id = notation_ids::get().allocate(a.schema);
			bool known = false;
			for (decltype(auto) s : schemas_used) {
				if (s.second == schema_id.second) {
					known = true;
					break;
				}
			}
			if (!known) schemas_used.push_back(std::pair(schema_id.first.to_s(), schema_id.second));

			std::cout << "inferred: " << row.first.first.to_s() << "\n";
			continue;
		}

		// discharge the nearest enclosing hypothesis level, probed above
		auto closed = find_ancestor<syntactical_entailment_introduction_start>(tip);
		// non-null: the discharge probe succeeded on the same walk
		auto dest = inference_destination(closed->parent());
		if (!dest) {
			error_report(src, "no inference destination for discharge");
			return;
		}

		if (auto p = std::get_if<std::unique_ptr<syntactical_entailment_introduction> >(&*discharge)) {
			std::shared_ptr<fact_database> node(std::move(*p));
			steps.push_back(node);	// pins the discharged branch as well, via its prior chain
			auto row = node->rationale(0);
			last_discharged = row.first.first;
			rows.push_back(proof_row{row.second, is_entailment_statement(last_discharged), last_discharged.to_s(), std::string(row.first.second.view())});
		} else {
			decltype(auto) q = std::get<std::unique_ptr<syntactical_entailment_2ary> >(*discharge);
			last_discharged = statement_t(std::shared_ptr<const formal::parsed>(q.release()));
		}

		dest->record(last_discharged);
		tip = dest;
		std::cout << "discharged: " << last_discharged.to_s() << "\n";

		complete = !find_ancestor<syntactical_entailment_introduction_start>(tip);
	}

	std::cout << "\nproven (" << theorem_name << "): " << last_discharged.to_s() << "\n";

	// theorem writer (work item #6): axioms used, the result, and the proof table as an
	// array of arrays [row label, inference-rule column, proposition column, rationale],
	// reformattable to the docs folder's Gentzen/Fitch HTML tables
	{
		fkyaml::node::sequence_type ax;
		std::vector<std::string> listed;
		for (decltype(auto) s : schemas_used) {
			if (0 < s.second && gentzen::axioms::get()->size() >= s.second) {
				ax.emplace_back(s.first);
				listed.push_back(s.first);
			}
			// a schema rendered under its defined name has also used the axiom that
			// conditionally defines it (the definition's source), once per such axiom
			if (auto e = defined_SVO::get().by_definiens(s.first)) {
				if (e->source) {
					const auto notation = e->source->to_s();
					if (listed.end() == std::find(listed.begin(), listed.end(), notation)) {
						ax.emplace_back(notation);
						listed.push_back(notation);
					}
				}
			}
		}

		fkyaml::node::sequence_type proof_table;
		size_t line_no = 0;
		for (decltype(auto) r : rows) {
			fkyaml::node::sequence_type cells;
			cells.emplace_back(std::to_string(++line_no) + ".");
			cells.emplace_back(r.entailment_column ? r.notation : std::string());
			cells.emplace_back(r.entailment_column ? std::string() : r.notation);
			cells.emplace_back(rewrite_labels(r.rationale, rows));
			proof_table.emplace_back(fkyaml::node::sequence(std::move(cells)));
		}

		fkyaml::node theorem = fkyaml::node::mapping();
		theorem["name"] = theorem_name;
		theorem["axioms"] = fkyaml::node::sequence(std::move(ax));
		theorem["result"] = last_discharged.to_s();
		theorem["proof"] = fkyaml::node::sequence(std::move(proof_table));

		fkyaml::node root = fkyaml::node::mapping();
		root["theorem"] = theorem;

		auto out_path = path;
		out_path.replace_extension(".theorem.yaml");
		std::ofstream out(out_path);
		if (out.is_open()) {
			out << fkyaml::node::serialize(root);
			std::wcout << L"theorem file: " << out_path.native() << L"\n";
		} else {
			error_report(src, "cannot write theorem file");
		}
	}

	std::cout << "proof table preview:\n";
	size_t line_no = 0;
	for (decltype(auto) r : rows) {
		std::cout << ++line_no << ".\t";
		if (r.entailment_column) std::cout << r.notation << "\t\t";
		else std::cout << "\t" << r.notation << "\t";
		std::cout << rewrite_labels(r.rationale, rows) << "\n";
	}
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
	formal::lex_node::interpolate_grouping_parens = gentzen_interpolate_grouping_parens;
	undefined_SVO::get();	// load the undefined SVOs, and the axioms
	gentzen::axioms::get()->watched_by(saturation::get());	// replays the loaded axioms, then watches

	enum { run_prototype_self_tests = 0 };
	if constexpr (run_prototype_self_tests) {
		try {
			gentzen::self_test_substitution();
			gentzen::self_test_enumeration();
			self_test_phrase_matcher();
			self_test_diagnostic_walks();
			self_test_reparse_routing();
			self_test_lex_axiom_admission();
		} catch (const std::exception& e) {
			report_exception("self-test exception", e);
		}
		std::cout << std::nounitbuf;
	}

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
		{
			const auto ext = src.path->extension();
			if (L".yaml" == ext || L".yml" == ext) {
				try {
					run_proof_script(*src.path);
				} catch (std::exception& e) {
					report_exception("proof session", e);
					return 3;
				}
				continue;
			}
		}
		auto to_interpret = std::ifstream(*src.path);
		if (!to_interpret.is_open()) continue;
		auto lines = to_lines(to_interpret, src);

		// intent is one line, one statement
		while (!lines.empty()) {
			const auto prior_errors = Errors.count();
			try {
			auto stage = TokenGrammar().apply(formal::lex_node::pop_front(lines));
			if (prior_errors < Errors.count()) continue;

			TokenGrammar().complete_parse<true>(stage);
			if (prior_errors < Errors.count()) continue;

			stage = GentzenGrammar().apply(stage);
			gentzen::report_diagnostics(stage);	// balanced groups parse (and may defer errors) during apply
			if (prior_errors < Errors.count()) continue;

			GentzenGrammar().complete_parse<true>(stage);
			gentzen::report_diagnostics(stage);
			} catch (std::exception& e) {
				report_exception("line iteration body", e);
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
