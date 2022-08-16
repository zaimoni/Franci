#include "Zaimoni.STL/LexParse/FormalWord.hpp"
#include "Zaimoni.STL/LexParse/Kuroda.hpp"

#ifdef GENTZEN_DRIVER
#include "test_driver.h"
#include "Zaimoni.STL/Pure.C/comptest.h"
#include <filesystem>
#include <memory>
#include <fstream>
#include <iostream>
#include <string_view>

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
#include <io.h>
#else
#include <unistd.h>
#endif

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

// whitespace trimming utilities
static void ltrim(std::string_view& src) {
	while (!src.empty() && isspace(static_cast<unsigned char>(src.front()))) src.remove_prefix(1);
}

static void rtrim(std::string_view& src) {
	while(!src.empty() && isspace(static_cast<unsigned char>(src.back()))) src.remove_suffix(1);
}

static void trim(std::string_view& src) {
	ltrim(src);
	rtrim(src);
}

enum LG_modes {
	LG_Comment = 0,
	LG_PP_like,	// #-format; could be interpreted later for C preprocessor directives if we were so inclined
	LG_CPP_like, // //-format
	LG_MAX
};

static_assert(sizeof(unsigned long long)*CHAR_BIT >= LG_MAX);

// note that we use # for set-theoretic cardinality, so this would not be correct at later stages
bool IsOneLineComment(formal::word*& x) {
	auto test = x->value();
	const auto o_size = test.size();
	ltrim(test);
	unsigned long long new_code = 0;
	if (test.starts_with('#')) {
		new_code = (1ULL << LG_Comment) | (1ULL << LG_PP_like);
	} else if (test.starts_with("//")) {
		new_code = (1ULL << LG_Comment) | (1ULL << LG_CPP_like);
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
		ooao = decltype(ooao)(new kuroda::parser<formal::word>());
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

// following class will need extraction once it is stable
class lex_node
{
	using lexeme = std::variant<formal::word, lex_node>;

private:
	kuroda::parser<lexeme>::symbols _prefix;
	kuroda::parser<lexeme>::symbols _infix;
	kuroda::parser<lexeme>::symbols _postfix;
	std::unique_ptr<lexeme> _anchor;
	std::unique_ptr<lexeme> _post_anchor;
	unsigned long long _code; // usually used as a bitmap

	lex_node(kuroda::parser<lexeme>::sequence& dest, size_t lb, size_t ub, unsigned long long code);	// slicing constructor

public:
	lex_node() noexcept : _code(0) {}
	// \todo anchor constructor
	lex_node(const lex_node& src) = delete;
	lex_node(lex_node&& src) = default;
	lex_node& operator=(const lex_node& src) = delete;
	lex_node& operator=(lex_node&& src) = default;
	virtual ~lex_node() = default;

	// factory function: slices a lex_node out of dest, then puts the lex_node at index lb
	void slice(kuroda::parser<lexeme>::sequence& dest, size_t lb, size_t ub, unsigned long long code = 0);

	formal::src_location origin() const;

	auto code() const { return _code; }
	void interpret(unsigned long long src) { _code = src; }

	std::string to_s() const;

private:
	static formal::src_location origin(const lexeme& src);
	std::string to_s(formal::src_location& track) const;
	static std::string to_s(const lexeme& src, formal::src_location& track);
	static std::string to_s(const kuroda::parser<lexeme>::sequence& src, formal::src_location& track);
};

// we only handle closed intervals
lex_node::lex_node(kuroda::parser<lexeme>::sequence& dest, size_t lb, size_t ub, unsigned long long code)
: _code(code)
{
	assert(lb < ub);
	assert(dest.size() > ub);

	const size_t delta = ub - lb;
	if (2 <= delta) {
		decltype(_infix) staging(delta - 1);
		memmove(staging.c_array(), dest.data() + lb + 1, sizeof(lexeme*) * (delta - 1));
		std::fill_n(dest.c_array() + lb + 1, delta - 1, nullptr);
		staging.swap(_infix);
	}
	_anchor.reset(dest[lb]);
	_post_anchor.reset(dest[ub]);
	dest[lb] = nullptr;
	dest[ub] = nullptr;
}

void lex_node::slice(kuroda::parser<lexeme>::sequence& dest, size_t lb, size_t ub, unsigned long long code)
{
	assert(lb < ub);
	assert(dest.size() > ub);

	const auto audit = dest.size();	// remove before commit
	const size_t delta = ub - lb;

	std::unique_ptr<lexeme> stage(new lexeme(lex_node(dest, lb, ub, code)));
	dest[lb] = stage.release();
	dest.DeleteNSlotsAt(delta, lb + 1);
	SUCCEED_OR_DIE(audit == dest.size() + delta);
}

formal::src_location lex_node::origin() const
{
	if (!_prefix.empty()) return origin(*_prefix.front());
	if (_anchor) return origin(*_anchor);
	if (!_infix.empty()) return origin(*_infix.front());
	if (_post_anchor) return origin(*_post_anchor);
	if (!_postfix.empty()) return origin(*_postfix.front());
	return formal::src_location();
}

formal::src_location lex_node::origin(const lexeme& src)
{
	if (auto x = std::get_if<formal::word>(&src)) return x->origin();
	return std::get<lex_node>(src).origin();
}

std::string lex_node::to_s() const
{
	auto track = origin();
	return to_s(track);
}

std::string lex_node::to_s(formal::src_location& track) const
{
	std::string ret;

	if (!_prefix.empty()) ret += to_s(_prefix, track);
	if (_anchor) ret += to_s(*_anchor, track);
	if (!_infix.empty()) ret += to_s(_infix, track);
	if (_post_anchor) ret += to_s(*_post_anchor, track);
	if (!_postfix.empty()) ret += to_s(_postfix, track);

	return ret;
}

std::string lex_node::to_s(const lexeme& src, formal::src_location& track)
{
	std::string ret;
	const auto start = origin(src);
	if (start.line_pos.first != track.line_pos.first) {
		// new line.  \todo Ignore indentation for one-line comments, but not normal source code
		ret += '\n';
	} else if (start.line_pos.first > track.line_pos.first) {
		// need whitespace to look like original code
		ret += std::string(start.line_pos.first- track.line_pos.first, ' ');
	}

	if (auto x = std::get_if<formal::word>(&src)) {
		ret += std::string(x->value());
		track = x->after();
	} else ret += std::get<lex_node>(src).to_s(track);

	return ret;
}

std::string lex_node::to_s(const kuroda::parser<lexeme>::sequence& src, formal::src_location& track)
{
	std::string ret;
	for (decltype(auto) x : src) ret += to_s(*x, track);
	return ret;
}

// lexing+preprocessing stage
// main language syntax

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

	lex_node test;

	if (2 > argc) help();

	int idx = 0;
	while (++idx < argc) {
		if (process_option(argv[idx])) continue;
		formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(argv[idx])));
		std::wcout << src.path->native() << "\n";
		auto to_interpret = std::ifstream(*src.path);
		if (!to_interpret.is_open()) continue;
		auto lines = to_lines(to_interpret, src);
		// debugging view
		std::cout << std::to_string(lines.size()) << "\n";
		for (decltype(auto) x : lines) {
			std::cout << x->code() << ":" << x->origin().line_pos.first << ":" << x->value() << "\n";
		}
	}

//	if (!to_console) STRING_LITERAL_TO_STDOUT("<pre>\n");

//	STRING_LITERAL_TO_STDOUT("End testing\n");
//	if (!to_console) STRING_LITERAL_TO_STDOUT("</pre>\n");
	return 0;	// success
};

#endif
