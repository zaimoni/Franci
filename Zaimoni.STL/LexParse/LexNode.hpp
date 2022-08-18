#ifndef ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP
#define ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP 1

#include "Kuroda.hpp"
#include "FormalWord.hpp"

namespace formal {

	class lex_node
	{
		kuroda::parser<lex_node>::symbols _prefix;
		kuroda::parser<lex_node>::symbols _infix;
		kuroda::parser<lex_node>::symbols _postfix;
		std::variant<std::unique_ptr<lex_node>,
			std::unique_ptr<formal::word> > _anchor;
		std::variant<std::unique_ptr<lex_node>,
			std::unique_ptr<formal::word> > _post_anchor;
		unsigned long long _code; // usually used as a bitmap

		lex_node(kuroda::parser<lex_node>::sequence& dest, size_t lb, size_t ub, unsigned long long code);	// slicing constructor
		// thin-wrapping constructors
		lex_node(formal::word*& src, unsigned long long code = 0) noexcept : _anchor(std::unique_ptr<formal::word>(src)), _code(code) {
			if (src->code() & formal::Comment) _code |= formal::Comment;
			src = nullptr;
		}

	public:
		lex_node(std::unique_ptr<formal::word> src, unsigned long long code = 0) noexcept : _anchor(std::move(src)), _code(code) {
			if (std::get<std::unique_ptr<formal::word> >(_anchor)->code() & formal::Comment) _code |= formal::Comment;
		}

		lex_node() noexcept : _code(0) {}
		// \todo anchor constructor
		lex_node(const lex_node& src) = delete;
		lex_node(lex_node&& src) = default;
		lex_node& operator=(const lex_node& src) = delete;
		lex_node& operator=(lex_node&& src) = default;
		virtual ~lex_node() = default;

		// factory function: slices a lex_node out of dest, then puts the lex_node at index lb
		static void slice(kuroda::parser<lex_node>::sequence& dest, size_t lb, size_t ub, unsigned long long code = 0);

		formal::src_location origin() const { return origin(this); }

		auto code() const { return _code; }
		void interpret(unsigned long long src) { _code = src; }
		void learn(unsigned long long src) { _code |= src; }

		formal::word* anchor_word() const;
		lex_node* anchor_node() const;
		formal::word* post_anchor_word() const;
		lex_node* post_anchor_node() const;
		bool syntax_ok() const;
		int is_pure_anchor() const; // C error code convention

		static std::unique_ptr<lex_node> pop_front(kuroda::parser<formal::word>::sequence& src);

		void to_s(std::ostream& dest) const;
		static std::ostream& to_s(std::ostream& dest, const kuroda::parser<lex_node>::sequence& src);

	private:
		static formal::src_location origin(const lex_node* src);
		static formal::src_location origin(const std::variant<std::unique_ptr<lex_node>, std::unique_ptr<formal::word> >& src);
		static void to_s(std::ostream& dest, const lex_node* src, formal::src_location& track);
		static void to_s(std::ostream& dest, const kuroda::parser<lex_node>::sequence& src, formal::src_location& track);
		static void to_s(std::ostream& dest, const std::variant<std::unique_ptr<lex_node>, std::unique_ptr<formal::word> >& src, formal::src_location& track);

		static void reset(std::variant<std::unique_ptr<lex_node>, std::unique_ptr<formal::word> >& dest, lex_node*& src);
	};

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

		std::unique_ptr<lex_node> stage(new lex_node(lex_node(dest, lb, ub, code)));
		dest[lb] = stage.release();
		dest.DeleteNSlotsAt(delta, lb + 1);
		SUCCEED_OR_DIE(audit == dest.size() + delta);
	}

	// intentionally use tail recursion here.
	formal::src_location lex_node::origin(const lex_node* src)
	{
	restart:
		if (!src->_prefix.empty()) {
			src = src->_prefix.front();
			goto restart;
		}
		if (auto x = src->anchor_node()) {
			src = x;
			goto restart;
		}
		if (auto x = src->anchor_word()) return x->origin();
		// We could try to go on, if our syntax is bad.
		return formal::src_location();
	}

	formal::src_location lex_node::origin(const std::variant<std::unique_ptr<lex_node>, std::unique_ptr<formal::word> >& src)
	{
		if (auto x = std::get_if<std::unique_ptr<formal::word> >(&src)) {
			if (auto y = x->get()) return y->origin();
			return formal::src_location();
		}
		return origin(std::get<std::unique_ptr<lex_node> >(src).get());
	}

	formal::word* lex_node::anchor_word() const
	{
		if (auto x = std::get_if<std::unique_ptr<formal::word> >(&_anchor)) return x->get();
		return nullptr;
	}

	lex_node* lex_node::anchor_node() const
	{
		if (auto x = std::get_if<std::unique_ptr<lex_node> >(&_anchor)) return x->get();
		return nullptr;
	}

	formal::word* lex_node::post_anchor_word() const
	{
		if (auto x = std::get_if<std::unique_ptr<formal::word> >(&_anchor)) return x->get();
		return nullptr;
	}

	lex_node* lex_node::post_anchor_node() const
	{
		if (auto x = std::get_if<std::unique_ptr<lex_node> >(&_anchor)) return x->get();
		return nullptr;
	}

	bool lex_node::syntax_ok() const
	{
		// shallow test -- deep one is CPU-expensive
		if (!anchor_word() && !anchor_node()) return false; // we always have a syntactic anchor
		if (!_infix.empty() && !post_anchor_word() && !post_anchor_node()) return false; // if we have an infix sequence, we have a post-anchor token as well
		// \todo language-specific checks go here
		return true;
	}

	/// <returns>1 if word anchor; 2 if node anchor; 0 for other data fields; -1 for no data at all (invalid)</returns>
	int lex_node::is_pure_anchor() const
	{
		if (!_prefix.empty()) return 0;
		if (!_infix.empty()) return 0;
		if (!_postfix.empty()) return 0;
		if (post_anchor_word()) return 0;
		if (post_anchor_node()) return 0;
		if (anchor_word()) return 1;
		if (anchor_node()) return 2;
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

	void lex_node::to_s(std::ostream& dest, const lex_node* src, formal::src_location& track)
	{
		if (!src->_prefix.empty()) to_s(dest, src->_prefix, track);
		to_s(dest, src->_anchor, track);
		if (!src->_infix.empty()) to_s(dest, src->_infix, track);
		to_s(dest, src->_post_anchor, track);
		if (!src->_postfix.empty()) to_s(dest, src->_postfix, track);
	}

	void lex_node::to_s(std::ostream& dest, const kuroda::parser<lex_node>::sequence& src, formal::src_location& track)
	{
		for (decltype(auto) x : src) to_s(dest, x, track);
	}

	void lex_node::to_s(std::ostream& dest, const std::variant<std::unique_ptr<lex_node>, std::unique_ptr<formal::word> >& src, formal::src_location& track)
	{
		if (auto x = std::get_if<std::unique_ptr<formal::word> >(&src)) {
			if (auto w = x->get()) {
				const auto start = w->origin();
				if (start.line_pos.first != track.line_pos.first) {
					// new line.  \todo Ignore indentation for one-line comments, but not normal source code
					dest << '\n';
				}
				else if (start.line_pos.first > track.line_pos.first) {
					// need whitespace to look like original code
					dest << std::string(start.line_pos.first - track.line_pos.first, ' ');
				}
				dest << w->value();
			}
			return;
		}
		auto lex = std::get<std::unique_ptr<lex_node> >(src).get();
		if (lex) to_s(dest, lex, track);
	}


	void lex_node::reset(std::variant<std::unique_ptr<lex_node>, std::unique_ptr<formal::word> >& dest, lex_node*& src)
	{
		assert(src);
		switch (src->is_pure_anchor()) {
		case 1:
			dest = std::move(std::get<std::unique_ptr<formal::word> >(src->_anchor));
			break;
		case 2:
			dest = std::move(std::get<std::unique_ptr<lex_node> >(src->_anchor));
			break;
		default: // has internal structure; just capture it as-is
			dest = std::unique_ptr<lex_node>(src);
			src = nullptr;
			return;
		}
		delete src;
		src = nullptr;
	}
}

#endif
