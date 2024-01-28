#include "LexNode.hpp"

namespace formal {

	// we only handle closed intervals
	lex_node::lex_node(kuroda::parser<lex_node>::sequence& dest, size_t lb, size_t ub, unsigned long long code)
		: _code(code)
	{
		assert(lb < ub);
		assert(dest.size() > ub);

		const size_t delta = ub - lb;
		if (2 <= delta) {
			decltype(_infix) staging(delta - 1);
			memmove(staging.c_array(), dest.data() + lb + 1, sizeof(lex_node*) * (delta - 1));
			std::fill_n(dest.c_array() + lb + 1, delta - 1, nullptr);
			staging.swap(_infix);
		}
		reset(_anchor, dest[lb]);
		reset(_post_anchor, dest[ub]);
		dest[lb] = nullptr;
	}

	void lex_node::slice(kuroda::parser<lex_node>::sequence& dest, size_t lb, size_t ub, unsigned long long code)
	{
		assert(lb < ub);
		assert(dest.size() > ub);

		const auto audit = dest.size();	// remove before commit
		const size_t delta = ub - lb;

		std::unique_ptr<lex_node> stage(new lex_node(dest, lb, ub, code));
		dest[lb] = stage.release();
		dest.DeleteNSlotsAt(delta, lb + 1);
		SUCCEED_OR_DIE(audit == dest.size() + delta);
	}

	// intentionally use tail recursion here.
	src_location lex_node::origin(const lex_node* src)
	{
		if (!src) return formal::src_location();
	restart:
		if (!src->_prefix.empty()) {
			src = src->_prefix.front();
			goto restart;
		}
		if (auto x = src->c_anchor<lex_node>()) {
			src = x;
			goto restart;
		}
		if (auto x = src->c_anchor<word>()) return x->origin();
		if (auto x = src->c_anchor<formal::parsed>()) return x->origin();
		// We could try to go on, if our syntax is bad.
		return formal::src_location();
	}

	src_location lex_node::origin(const decltype(_anchor)& src)
	{
		struct _origin {
			auto operator()(const zaimoni::COW<word>& x) {
				if (x) return x->origin();
				return src_location();
			}
			auto operator()(const zaimoni::COW<lex_node>& x) { return origin(x.get()); }
			auto operator()(const zaimoni::COW<parsed>& x) {
				if (x) return x->origin();
				return src_location();
			}
		};

		static _origin ooao;

		return std::visit(ooao, src);
	}

	bool lex_node::syntax_ok() const
	{
		// shallow test -- deep one is CPU-expensive
		if (!classify(_anchor)) return false; // we always have a syntactic anchor
		if (!_infix.empty() && !classify(_post_anchor)) return false; // if we have an infix sequence, we have a post-anchor token as well
		// \todo language-specific checks go here
		return true;
	}

	/// <returns>1 if word anchor; 2 if node anchor; 0 for other data fields; -1 for no data at all (invalid)</returns>
	int lex_node::is_pure_anchor() const
	{
		if (!_prefix.empty()) return 0;
		if (!_infix.empty()) return 0;
		if (!_postfix.empty()) return 0;
		if (classify(_post_anchor)) return 0;
		if (int code = classify(_anchor)) return code;
		return -1;
	}

	std::unique_ptr<lex_node> lex_node::pop_front(kuroda::parser<formal::word>::sequence& src)
	{
		std::unique_ptr<lex_node> ret;
		while (!src.empty() && !ret) {
			if (decltype(auto) x = src.front()) {
				ret = decltype(ret)(new lex_node(src.front()));
				src.front() = nullptr;
			}
			src.FastDeleteIdx(0);
		};
		return ret;
	}

	std::unique_ptr<lex_node> lex_node::pop_front(kuroda::parser<formal::lex_node>::sequence& src)
	{
		std::unique_ptr<lex_node> ret;
		while (!src.empty() && !ret) {
			if (decltype(auto) x = src.front()) {
				ret = decltype(ret)(x);
				src.front() = nullptr;
			}
			src.FastDeleteIdx(0);
		};
		return ret;
	}

	bool lex_node::recurse(kuroda::parser<formal::lex_node>& grammar) {
		if (_infix.empty()) return false;
		return grammar.finite_parse(_infix);
	}

	bool lex_node::rewrite(lex_node*& target, std::function<lex_node* (lex_node* target)> op)
	{
		if (decltype(auto) dest = op(target)) {
			if (target != dest) {
				delete target;
				target = dest;
			}
			return true;
		}
		return false;
	}

	bool lex_node::recurse(kuroda::parser<formal::lex_node>& grammar, formal::lex_node* src) {
		bool updated = false;
	restart:
		if ((formal::Error | formal::Inert_Token) & src->code()) return updated;

		if (grammar.want_sub_recursion(*src)) {
			if (src->recurse(grammar)) updated = true;
		} else {
			if (!src->_infix.empty() && grammar.recurse(src->_infix)) updated=true;
		}
		if (!src->_prefix.empty() && grammar.recurse(src->_prefix)) updated = true;
		if (!src->_postfix.empty() && grammar.recurse(src->_postfix)) updated = true;

		// tail recursion
		auto anchor = src->anchor<formal::lex_node>();
		if (auto p_anchor = src->post_anchor<formal::lex_node>()) {
			if (!anchor) {
				src = p_anchor;
				goto restart;
			}
			if (recurse(grammar, p_anchor)) updated = true;
		}
		if (anchor) {
			src = anchor;
			goto restart;
		}
		return updated;
	}

	std::string lex_node::to_s() const
	{
		std::ostringstream stage;
		to_s(stage);
		return stage.str();
	}

	void lex_node::to_s(std::ostream& dest) const
	{
		auto track = origin();
		to_s(dest, this, track);
	}

	std::ostream& lex_node::to_s(std::ostream& dest, const kuroda::parser<lex_node>::sequence& src)
	{
		auto track = origin(src.front());
		to_s(dest, src, track);
		return dest;
	};

	void lex_node::to_s(std::ostream& dest, const lex_node* src, src_location& track)
	{
		if (!src->_prefix.empty()) to_s(dest, src->_prefix, track);
		to_s(dest, src->_anchor, track);
		if (!src->_infix.empty()) to_s(dest, src->_infix, track);
		to_s(dest, src->_post_anchor, track);
		if (!src->_postfix.empty()) to_s(dest, src->_postfix, track);
	}

	void lex_node::to_s(std::ostream& dest, const kuroda::parser<lex_node>::sequence& src, src_location& track)
	{
		for (decltype(auto) x : src) to_s(dest, x, track);
	}

	void lex_node::to_s(std::ostream& dest, const decltype(_anchor)& src, src_location& track)
	{
		struct _to_s {
			std::ostream& dest;
			formal::src_location& track;

			_to_s(std::ostream& dest, formal::src_location& track) noexcept : dest(dest), track(track) {}

			auto operator()(const zaimoni::COW<word>& w) {
				const auto start = w->origin();
				if (start.line_pos.first != track.line_pos.first) {
					// new line.  \todo Ignore indentation for one-line comments, but not normal source code
					dest << '\n';
				} else if (start.line_pos.second > track.line_pos.second) {
					// need whitespace to look like original code
					dest << std::string(start.line_pos.second - track.line_pos.second, ' ');
				}
				dest << w->value();
				track = w->after();
			}
			auto operator()(const zaimoni::COW<lex_node>& x) {
				if (auto lex = x.get()) return to_s(dest, lex, track);
			}
			auto operator()(const zaimoni::COW<parsed>& x) {
				auto start = x->origin();
				if (!start.path && track.path) {
					// default value
					start = track;
					start += 1;
				}

				if (start.line_pos.first != track.line_pos.first) {
					// new line.  \todo Ignore indentation for one-line comments, but not normal source code
					dest << '\n';
				} else if (start.line_pos.second > track.line_pos.second) {
					// need whitespace to look like original code
					dest << std::string(start.line_pos.second - track.line_pos.second, ' ');
				}

				auto stage = x->to_s();
				if (!stage.empty()) {
					dest << stage;
					track += stage.size();
				}
			}
		};

		std::visit(_to_s(dest, track), src);
	}

	unsigned int lex_node::precedence() const
	{
		return 0;
	}

	bool lex_node::is_balanced_pair(const std::string_view& l_token, const std::string_view& r_token) const {
		auto leading_tag = c_anchor<formal::word>();
		if (!leading_tag || leading_tag->value() != l_token) return false;
		auto trailing_tag = c_post_anchor<formal::word>();
		if (!trailing_tag || trailing_tag->value() != r_token) return false;
		return true;
	}

	bool lex_node::remove_outer_parentheses(kuroda::parser<formal::lex_node>::symbols& target) {
		bool ret = false;
		while (   1 == target.size() && target.front()->prefix().empty()
			   && target.front()->postfix().empty() && !target.front()->infix().empty()
			   && target.front()->is_balanced_pair("(", ")")) {
			kuroda::parser<formal::lex_node>::symbols stage;

			stage.swap(target.front()->_infix);
			stage.swap(target);
			ret = true;
		}
		return ret;
	}

	std::pair<lex_node**, std::vector<std::span<lex_node*, std::dynamic_extent> > > lex_node::split(kuroda::parser<lex_node>::sequence& src, std::function<bool(const lex_node&)> ok)
	{
		std::pair<lex_node**, std::vector<std::span<lex_node*, std::dynamic_extent> > > ret;
		ret.first = src.begin();

		std::vector<size_t> indexes;

		size_t n = 0;
		while (src.size() > n) {
			if (ok(*(src[n++]))) indexes.push_back(n-1);
		}
		if (indexes.empty()) return ret;

		size_t origin = 0;
		for (const auto i : indexes) {
			if (origin < i) {
				const auto delta = i - origin;
				ret.second.emplace_back(&(src[origin]), delta);
			} else {
				ret.second.emplace_back(&(src[origin]), 0);
			}
			origin = i+1;
		}

		(decltype(indexes)()).swap(indexes);

		if (origin < src.size()) {
			const auto delta = src.size() - origin;
			ret.second.emplace_back(&(src[origin]), delta);
		} else {
			ret.second.emplace_back(&(src[origin]), 0);
		}
		return ret;
	}

	std::pair<lex_node**, std::vector<std::span<lex_node*, std::dynamic_extent> > > lex_node::split(const std::span<lex_node*, std::dynamic_extent>& src, lex_node** const start, std::function<bool(const lex_node&)> ok)
	{
		std::pair<lex_node**, std::vector<std::span<lex_node*, std::dynamic_extent> > > ret;
		ret.first = start;
		std::vector<size_t> indexes;

		size_t n = 0;
		while (src.size() > n) {
			if (ok(*(src[n++]))) indexes.push_back(n - 1);
		}
		if (indexes.empty()) return ret;

		size_t origin = 0;
		for (const auto i : indexes) {
			if (origin < i) {
				const auto delta = i - origin;
				ret.second.emplace_back(&(src[origin]), delta);
			} else {
				ret.second.emplace_back(&(src[origin]), 0);
			}
			origin = i + 1;
		}

		(decltype(indexes)()).swap(indexes);

		if (origin < src.size()) {
			const auto delta = src.size() - origin;
			ret.second.emplace_back(&(src[origin]), delta);
		} else {
			ret.second.emplace_back(&(src[origin]), 0);
		}
		return ret;
	}

	std::vector<kuroda::parser<lex_node>::symbols> lex_node::move_per_spec(const std::vector<std::span<lex_node*, std::dynamic_extent> >& src)
	{
		std::vector<kuroda::parser<lex_node>::symbols> ret;
		ret.reserve(src.size());

		for (decltype(auto) x : src) {
			if (x.empty()) continue;
			ret.emplace_back(x.size());
			std::copy_n(x.begin(), x.size(), ret.back().begin());
			std::fill_n(&(*x.begin()), x.size(), nullptr);
		}

		return ret;
	}


	int lex_node::classify(const decltype(_anchor)& src)
	{
		struct _encode_anchor {
			int operator()(const zaimoni::COW<word>& x) { return x ? 1 : 0; }
			int operator()(const zaimoni::COW<lex_node>& x) { return x ? 2 : 0; }
			int operator()(const zaimoni::COW<parsed>& x) { return x ? 3 : 0; }
			int operator()(const std::shared_ptr<const parsed>& x) { return x ? 4 : 0; }
		};

		static _encode_anchor ooao;
		return std::visit(ooao, src);
	}

	void lex_node::reset(decltype(_anchor)& dest, lex_node*& src)
	{
		assert(src);
		switch (src->is_pure_anchor()) {
		case 1:
			dest = std::move(std::get<zaimoni::COW<word> >(src->_anchor));
			break;
		case 2:
			dest = std::move(std::get<zaimoni::COW<lex_node> >(src->_anchor));
			break;
		case 3:
			dest = std::move(std::get<zaimoni::COW<parsed> >(src->_anchor));
			break;
		default: // has internal structure; just capture it as-is
			dest = std::unique_ptr<lex_node>(src);
			src = nullptr;
			return;
		}
		delete src;
		src = nullptr;
	}
}	// namespace formal
