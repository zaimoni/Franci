#ifndef ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP
#define ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP 1

#include "Kuroda.hpp"
#include "FormalWord.hpp"

namespace formal {

	// abstract interface.
	struct parsed {
		virtual ~parsed() = default;

		virtual std::unique_ptr<parsed> clone() const = 0;
		virtual void CopyInto(parsed*& dest) const = 0;	// can throw memory failure
		virtual void MoveInto(parsed*& dest) = 0;	// can throw memory failure.  If it succeeds, it destroys the source.

		virtual std::string to_s() const = 0;
	};

	class lex_node
	{
		kuroda::parser<lex_node>::symbols _prefix;
		kuroda::parser<lex_node>::symbols _infix;
		kuroda::parser<lex_node>::symbols _postfix;
		std::variant<std::unique_ptr<lex_node>,
			std::unique_ptr<word>,
			std::unique_ptr<parsed> > _anchor;
		decltype(_anchor) _post_anchor;
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

		template<class Val>
		Val* anchor() const requires requires { std::get_if<std::unique_ptr<Val> >(&_anchor); }
		{
			if (auto x = std::get_if<std::unique_ptr<Val> >(&_anchor)) return x->get();
			return nullptr;
		}

		template<class Val>
		Val* post_anchor() const requires requires { std::get_if<std::unique_ptr<Val> >(&_post_anchor); }
		{
			if (auto x = std::get_if<std::unique_ptr<Val> >(&_post_anchor)) return x->get();
			return nullptr;
		}

		bool syntax_ok() const;
		int is_pure_anchor() const; // C error code convention

		static std::unique_ptr<lex_node> pop_front(kuroda::parser<formal::word>::sequence& src);

		void to_s(std::ostream& dest) const;
		static std::ostream& to_s(std::ostream& dest, const kuroda::parser<lex_node>::sequence& src);

	private:
		static formal::src_location origin(const lex_node* src);
		static formal::src_location origin(const decltype(_anchor)& src);
		static void to_s(std::ostream& dest, const lex_node* src, formal::src_location& track);
		static void to_s(std::ostream& dest, const kuroda::parser<lex_node>::sequence& src, formal::src_location& track);
		static void to_s(std::ostream& dest, const decltype(_anchor)& src, formal::src_location& track);

		static int classify(const decltype(_anchor)& src);
		static void reset(decltype(_anchor)& dest, lex_node*& src);
	};

} // namespace formal

#endif
