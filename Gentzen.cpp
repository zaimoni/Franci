#include "Zaimoni.STL/LexParse/LexNode.hpp"
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

#include "errcount.hpp"

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
#include <io.h>
#else
#include <unistd.h>
#endif

static error_counter<size_t> Errors(100, "too many errors");
static error_counter<size_t> Warnings(1000, "too many warnings");

// stub for more sophisticated error reporting
static void error_report(const formal::src_location& loc, const std::string& err) {
	std::wcerr << loc.path->native();
	std::cerr << loc.to_s() << ": error : " << err << '\n';
	++Errors;
}

static void error_report(formal::lex_node& fail, const std::string& err) {
	error_report(fail.origin(), err);
	fail.learn(formal::Error);
}

static void warning_report(const formal::src_location& loc, const std::string& warn) {
	std::wcerr << loc.path->native();
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

size_t is_alphabetic(const std::string_view& src)
{
	if (isalpha(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

size_t is_alphanumeric(const std::string_view& src)
{
	if (isalnum(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

size_t is_digit(const std::string_view& src)
{
	if (isdigit(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

size_t is_hex_digit(const std::string_view& src)
{
	if (isxdigit(static_cast<unsigned char>(src[0]))) return 1;
	return 0;
}

static std::optional<std::pair<std::string_view, size_t> > kleene_star(const std::string_view& src, std::function<size_t(const std::string_view&)> ok) {
	size_t len = src.empty() ? 0 : ok(src);
	if (0 >= len) return std::nullopt;
	size_t matched = 0;
	auto working = src;
	while (0 < len) {
		++matched;
		working.remove_prefix(len);
		len = src.empty() ? 0 : ok(working);
	}

	std::pair<std::string_view, size_t> ret(src, matched);
	ret.first.remove_suffix(working.size());
	return ret;
}

// prototype class -- extract to own files when stable

class HTMLtag : public formal::parsed {
	using kv_pairs_t = std::vector < std::pair<std::string, std::string> >;
	std::string _tag_name;
	std::shared_ptr<const kv_pairs_t> kv_pairs; // could use std::map instead
	formal::src_location _origin;
	unsigned long long _bitmap;

	static constexpr const decltype(_bitmap) Start = (1ULL << 0);
	static constexpr const decltype(_bitmap) End = (1ULL << 1);

	static constexpr const char* start_tag[] = { "<", "</", "<" };
	static constexpr const char* end_tag[] = { ">", ">", " />" };

public:
	enum class mode : decltype(_bitmap) {
		opening = Start,
		closing = End,
		self_closing = Start | End
	};

	HTMLtag() = delete;
	HTMLtag(const HTMLtag& src) = default;
	HTMLtag(HTMLtag&& src) = default;
	HTMLtag& operator=(const HTMLtag& src) = default;
	HTMLtag& operator=(HTMLtag&& src) = default;
	~HTMLtag() = default;

	HTMLtag(const std::string& tag, mode code, formal::src_location origin) : _tag_name(tag), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}
	HTMLtag(std::string&& tag, mode code, formal::src_location origin) noexcept : _tag_name(std::move(tag)), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}
	HTMLtag(const std::string& tag, mode code, formal::src_location origin, decltype(kv_pairs) data) : _tag_name(tag), kv_pairs(data), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}
	HTMLtag(std::string&& tag, mode code, formal::src_location origin, decltype(kv_pairs) data) noexcept : _tag_name(std::move(tag)), kv_pairs(data), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}

	std::unique_ptr<parsed> clone() const override { return std::unique_ptr<parsed>(new HTMLtag(*this)); }
	void CopyInto(parsed*& dest) const override { zaimoni::CopyInto(*this, dest); }	// polymorphic assignment
	void MoveInto(parsed*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }	// polymorphic move

	formal::src_location origin() const override { return _origin; }

	std::string to_s() const override {
		auto index = (_bitmap & (Start | End))-1;

		std::string ret(start_tag[index]);
		ret += _tag_name;
		if (kv_pairs) {
			for (decltype(auto) kv : *kv_pairs) {
				ret += " ";
				ret += kv.first;
				if (!kv.second.empty()) {
					ret += "\"";
					ret += kv.second;
					ret += "\"";
				}
			}
		}
		ret += end_tag[index];
		return ret;
	}

	static std::unique_ptr<HTMLtag> parse(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
		// invariants -- would be ok to hard-fail these
		const auto x = src[viewpoint];
		if (x->code() & (formal::Comment | formal::Tokenized | formal::Inert_Token)) return nullptr;	// do not try to lex comments, or already-tokenized
		if (1 != x->is_pure_anchor()) return nullptr;	// we only try to manipulate things that don't have internal syntax
		// end invariants check

		const auto w = x->anchor<formal::word>();
		auto text = w->value();

		if (!text.starts_with("<")) return nullptr;
		auto working = text;
		size_t initial_len = 1;
		unsigned int code = Start | End;
		working.remove_prefix(1);
		if (working.starts_with("/")) {
			// terminal tag
			working.remove_prefix(1);
			code &= ~Start;
			initial_len++;
		}

		if (working.empty()) return nullptr;
		const auto would_be_tag = kleene_star(working, is_alphabetic);
		if (!would_be_tag) return nullptr;

		kv_pairs_t kv_pairs;
		std::vector<size_t> doomed;
		bool seen_equals = false;
		auto y = x;
		size_t scan = viewpoint;
		ltrim(working);

		static auto can_parse = [&]() {
			if (!working.empty()) return true;
			doomed.push_back(scan);

			while (src.size() > ++scan) {
				y = src[viewpoint];
				if (y->code() & formal::Comment) continue;	// ignore
				if ((y->code() & (formal::Tokenized | formal::Inert_Token))
					|| 1 != y->is_pure_anchor()) {
					std::string err("HTML tag ");
					err += would_be_tag->first;
					err += " parse stopped by already-parsed content";
					error_report(x->origin(), err);
					return false;
				}
				working = y->anchor<formal::word>()->value();
				ltrim(working);
				if (!working.empty()) return true;
				doomed.push_back(scan);
			}
			return false;
		};

		static auto chop_to_remainder = [&]() {
			auto remainder = y->anchor<formal::word>()->value();
			remainder.remove_prefix(remainder.size() - working.size());
			ltrim(remainder);
			if (!remainder.empty()) {
				std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size())));
				std::unique_ptr<formal::lex_node> node(new formal::lex_node(std::move(stage)));
				if (viewpoint < scan) {
					delete src[scan];
					src[scan] = node.release();
				} else {
					src.insertNSlotsAt(1, viewpoint + 1);
					src[viewpoint + 1] = node.release();
				}
			} else if (viewpoint < scan) src.DeleteIdx(scan);

			while (!doomed.empty()) {
				auto gone = doomed.back();
				doomed.pop_back();
				if (viewpoint < gone) src.DeleteIdx(gone);
			};
		};

		while(can_parse()) {
			bool stop_now = false;
			if (working.starts_with("/>")) {
				// self-closing tag.  Formal syntax error if both closing and self-closing.
				if (End == code) {
					warning_report(x->origin(), "HTML-like tag is both closing, and self-closing");
				} else if (Start == code) {
					warning_report(x->origin(), "HTML-like tag is both opening, and self-closing");
				}
				stop_now = true;
				working.remove_prefix(2);
			} else if (working.starts_with(">")) {
				stop_now = true;
				working.remove_prefix(1);
			}
			if (stop_now) {
				if (End == code) { // closing tag: discard all key-value pairs
					chop_to_remainder();
					return std::unique_ptr<HTMLtag>(new HTMLtag(std::string(would_be_tag->first), mode::closing, w->origin()));
				}
				if (kv_pairs.empty()) { // no key-value pairs
					chop_to_remainder();
					return std::unique_ptr<HTMLtag>(new HTMLtag(std::string(would_be_tag->first), (mode)code, w->origin()));
				};
				chop_to_remainder();
				return std::unique_ptr<HTMLtag>(new HTMLtag(std::string(would_be_tag->first), (mode)code, w->origin(), std::shared_ptr<const kv_pairs_t>(new kv_pairs_t(std::move(kv_pairs)))));
			}
			if (auto would_be_key = kleene_star(working, is_alphabetic)) {	// \todo not quite correct, but handles what is needed
				if (seen_equals) {
					kv_pairs.back().second = would_be_key->first;
					seen_equals = false;
				} else
					kv_pairs.push_back(std::pair(std::string(would_be_key->first), std::string()));
				working.remove_prefix(would_be_key->first.size());
				ltrim(working);
				continue;
			}
			if (working.starts_with("=")) {
				if (seen_equals) {
					warning_report(x->origin(), "HTML-like tag parse aborted: = =");
					return nullptr;
				}
				if (kv_pairs.empty() || !kv_pairs.back().second.empty()) {
					warning_report(x->origin(), "HTML-like tag parse aborted: key-less =");
					return nullptr;
				}
				seen_equals = true;
				working.remove_prefix(1);
				continue;
			}
			if (seen_equals) {
				// we don't handle multi-line values
				if (working.starts_with('"')) {
					auto n = working.find('"', 1);
					if (std::string_view::npos != n) {
						auto val = working;
						if (val.size() > (n + 1)) val.remove_suffix(val.size() - (n + 1));
						kv_pairs.back().second = std::string(val);
						working.remove_prefix(n + 1);
						seen_equals = false;
						continue;
					} else {
						warning_report(x->origin(), "HTML-like tag parse aborted: unterminated \"...\"");
						return nullptr;
					}
				}
				if (working.starts_with('\'')) {
					auto n = working.find('\'', 1);
					if (std::string_view::npos != n) {
						auto val = working;
						if (val.size() > (n + 1)) val.remove_suffix(val.size() - (n + 1));
						kv_pairs.back().second = std::string(val);
						working.remove_prefix(n + 1);
						seen_equals = false;
						continue;
					} else {
						warning_report(x->origin(), "HTML-like tag parse aborted: unterminated '...'");
						return nullptr;
					}
				}
			}
			warning_report(x->origin(), "HTML-like tag parse aborted: unclear how to proceed");
			return nullptr;
		}
		if (src.size() > scan) return nullptr;

		return nullptr;
	}

	static std::optional<std::variant<
		std::pair<std::unique_ptr<HTMLtag>, size_t>,
		std::pair<std::string_view, unsigned int>
	> > parse(const formal::word& w)
	{	// In practice, these tags can span multiple lines.  This is a prefilter
		const auto src = w.value();

		if (!src.starts_with("<")) return std::nullopt;
		auto working = src;
		size_t initial_len = 1;
		unsigned int code = Start | End;
		working.remove_prefix(1);
		if (working.starts_with("/")) {
			// terminal tag
			working.remove_prefix(1);
			code &= ~Start;
			initial_len++;
		}

		if (working.empty()) return std::nullopt;
		const auto would_be_tag = kleene_star(working, is_alphabetic);
		if (!would_be_tag) return std::nullopt;
		initial_len += would_be_tag->first.size();
		auto leading_fragment = src;
		leading_fragment.remove_suffix(src.size() - initial_len);
		ltrim(working);
		if (working.empty()) {
			// need full parse
			return std::pair(leading_fragment, code);
		}
		if (working.starts_with("/>")) {
			// self-closing tag.  Formal syntax error if both closing and self-closing.
			if (End == code) {
				warning_report(w.origin(), "HTML-like tag is both closing, and self-closing");
			}
			std::unique_ptr<HTMLtag> ret(new HTMLtag(std::string(would_be_tag->first), (mode)code, w.origin()));
			working.remove_prefix(2);
			return std::pair(std::move(ret), src.size() - working.size());
		}
		if (working.starts_with(">")) {
			// either start, or end tag.
			if ((Start | End) == code) code = Start;
			std::unique_ptr<HTMLtag> ret(new HTMLtag(std::string(would_be_tag->first), (mode)code, w.origin()));
			working.remove_prefix(1);
			return std::pair(std::move(ret), src.size() - working.size());
		}

		const auto would_be_first_key = kleene_star(working, is_alphabetic);
		if (!would_be_first_key) {
			error_report(w.origin(), "malformed HTML tag");
			return std::nullopt;
		}

		return std::pair(leading_fragment, Start);	// we punt on handling start tags with key-value pairs
	}

	template<auto hint_code> requires(Start <= hint_code && (Start | End) >= hint_code)
	static std::unique_ptr<HTMLtag> parse(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint, std::string_view hint) {
		// invariants -- would be ok to hard-fail these
		decltype(auto) x = src[viewpoint];
		if (x->code() & (formal::Comment | formal::Tokenized | formal::Inert_Token)) return nullptr;	// do not try to lex comments, or already-tokenized
		if (1 != x->is_pure_anchor()) return nullptr;	// we only try to manipulate things that don't have internal syntax

		const auto w = x->anchor<formal::word>();
		auto text = w->value();

		if constexpr (Start == hint_code) {
			if (!text.starts_with(hint)) return nullptr;
		} else {
			if (text != hint) return nullptr;
		}
		// end invariants check

		auto tagname = hint;
		if constexpr (End == hint_code) { tagname.remove_prefix(2); }
		else { tagname.remove_prefix(1); }

		decltype(viewpoint) scan = viewpoint;

		if constexpr (Start == hint_code) {
		} else if constexpr (End == hint_code) {
			while (++scan < src.size()) {
				decltype(auto) y = src[scan];
				if (y->code() & formal::Comment) continue;	// ignore comments
				if (y->code() & formal::Inert_Token) {
					error_report(*x, "HTML tag parse stopped by inert token");
					return nullptr;
				}
				if (y->code() & formal::Tokenized) {
					// this might need further work
					error_report(*x, "HTML tag parse stopped by already-parsed token");
					return nullptr;
				}
				if (1 != y->is_pure_anchor()) {
					error_report(*x, "HTML tag parse stopped by already-parsed content");
					return nullptr;	// we only try to manipulate things that don't have internal syntax
				}

				const auto z = y->anchor<formal::word>();
				auto working = z->value();
				ltrim(working);
				if (working.empty()) {
					src.DeleteIdx(scan--);
					continue;
				}

				if (working.starts_with("/>")) {
					// self-closing tag.  We incur a formal syntax error: both closing and self-closing.
					warning_report(w->origin(), "HTML-like tag is both closing, and self-closing");
					working.remove_prefix(2);
					ltrim(working);
					if (working.empty()) {
						src.DeleteIdx(scan--);
					} else {
						*z = formal::word(std::shared_ptr<const std::string>(new std::string(working)), y->origin() + (z->value.size() - working.size()), z->code());
					}
					return new HTMLtag(std::string(tagname), (mode)hint_code, w->origin());
				}
				if (working.starts_with(">")) {
					// either start, or end tag.
					if ((Start | End) == code) code = Start;
					working.remove_prefix(1);
					ltrim(working);
					if (working.empty()) {
						src.DeleteIdx(scan--);
					} else {
						*z = formal::word(std::shared_ptr<const std::string>(new std::string(working)), y->origin() + (z->value.size() - working.size()), z->code());
					}
					return new HTMLtag(std::string(tagname), (mode)hint_code, w->origin());
				}
			}
			warning_report(w->origin(), "incomplete HTML-like closing tag: completing");
			return new HTMLtag(std::string(tagname), (mode)hint_code, w->origin());
		} else /* if constexpr ((Start | End) == hint_code) */ {
		}

		return ret;
	}

};

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
	TG_HTML_tag =59,
	TG_HTML_entity,
	TG_MAX
};

static_assert(sizeof(unsigned long long)* CHAR_BIT >= TG_MAX);
static_assert(!(formal::Comment & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Comment & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Error & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Error & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Inert_Token & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Inert_Token & (1ULL << TG_HTML_tag)));
static_assert(!(formal::Tokenized & (1ULL << TG_HTML_tag)));
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

	decltype(auto) x = src[viewpoint];
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
		x->learn((1ULL << TG_HTML_entity) | formal::Inert_Token);
		return ret;
	}

	if (auto possible_tag = HTMLtag::parse(*w)) {
		if (auto tag = std::get_if<std::pair<std::unique_ptr<HTMLtag>, size_t> >(&(*possible_tag))) {
			auto prechop_size = text.size() - tag->second;
			if (0 < prechop_size) {
				auto remainder = text;
				remainder.remove_prefix(prechop_size);
				ltrim(remainder);
				if (!remainder.empty()) {
					std::unique_ptr<formal::word> stage(new formal::word(std::shared_ptr<const std::string>(new std::string(remainder)), w->origin() + (text.size() - remainder.size())));
					std::unique_ptr<formal::lex_node> node(new formal::lex_node(std::move(stage)));
					src.insertNSlotsAt(1, viewpoint + 1);
					src[viewpoint + 1] = node.release();
				};
			}
			std::unique_ptr<formal::lex_node> stage(new formal::lex_node(tag->first.release(), formal::Tokenized | TG_HTML_tag));
			delete src[viewpoint];
			src[viewpoint] = stage.release();
			return ret;
		}
		// \todo handle incomplete parse
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

static auto& TokenGrammar() {
	static std::unique_ptr<kuroda::parser<formal::lex_node> > ooao;
	if (!ooao) {
		ooao = decltype(ooao)(new decltype(ooao)::element_type());
		ooao->register_build_nonterminal(tokenize);
		ooao->register_build_nonterminal(balanced_atomic_handler(reserved_atomic[0].first, reserved_atomic[1].first));
		ooao->register_build_nonterminal(balanced_atomic_handler(reserved_atomic[2].first, reserved_atomic[3].first));
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

	if (2 > argc) help();

	int idx = 0;
	while (++idx < argc) {
		if (process_option(argv[idx])) continue;
		formal::src_location src(std::pair(1, 0), std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(argv[idx])));
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
	return 0;	// success
};

#endif
