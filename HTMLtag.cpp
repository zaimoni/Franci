#include "HTMLtag.hpp"

#include "Zaimoni.STL/LexParse/string_view.hpp"

const std::string* HTMLtag::is_balanced_pair(const formal::lex_node& src) {
	auto leading_tag = dynamic_cast<HTMLtag*>(src.anchor<formal::parsed>());
	if (!leading_tag) return nullptr;
	if (HTMLtag::mode::opening != leading_tag->tag_type()) return nullptr;
	auto trailing_tag = dynamic_cast<HTMLtag*>(src.post_anchor<formal::parsed>());
	if (!trailing_tag) return nullptr;
	if (HTMLtag::mode::opening != trailing_tag->tag_type() || trailing_tag->tag_name() != leading_tag->tag_name()) return nullptr;
	return &trailing_tag->tag_name();
}

bool HTMLtag::is_balanced_pair(const formal::lex_node& src, const std::string& target) {
	auto leading_tag = dynamic_cast<HTMLtag*>(src.anchor<formal::parsed>());
	if (!leading_tag) return false;
	if (HTMLtag::mode::opening != leading_tag->tag_type() || leading_tag->tag_name() != target) return false;
	auto trailing_tag = dynamic_cast<HTMLtag*>(src.post_anchor<formal::parsed>());
	if (!trailing_tag) return false;
	if (HTMLtag::mode::opening != trailing_tag->tag_type() || trailing_tag->tag_name() != target) return false;
	return true;
}

std::string HTMLtag::to_s() const {
	static constexpr const char* start_tag[] = { "<", "</", "<" };
	static constexpr const char* end_tag[] = { ">", ">", " />" };

	auto index = (_bitmap & (Start | End)) - 1;

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

std::unique_ptr<HTMLtag> HTMLtag::parse(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint) {
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

	working.remove_prefix(would_be_tag->first.size());
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
			}
			else {
				src.insertNSlotsAt(1, viewpoint + 1);
				src[viewpoint + 1] = node.release();
			}
		}
		else if (viewpoint < scan) src.DeleteIdx(scan);

		while (!doomed.empty()) {
			auto gone = doomed.back();
			doomed.pop_back();
			if (viewpoint < gone) src.DeleteIdx(gone);
		};
	};

	while (can_parse()) {
		bool stop_now = false;
		if (working.starts_with("/>")) {
			// self-closing tag.  Formal syntax error if both closing and self-closing.
			if (End == code) {
				warning_report(x->origin(), "HTML-like tag is both closing, and self-closing");
			}
			else if (Start == code) {
				warning_report(x->origin(), "HTML-like tag is both opening, and self-closing");
			}
			stop_now = true;
			working.remove_prefix(2);
		}
		else if (working.starts_with(">")) {
			stop_now = true;
			working.remove_prefix(1);
			if ((Start | End) == code) code = Start;
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
			}
			else
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
				}
				else {
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
				}
				else {
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
