#ifndef ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP
#define ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP 1

#include "Kuroda.hpp"
#include "FormalWord.hpp"
#include "../cache.hpp"
#include "../COW.hpp"
#include <span>

namespace formal {

	// abstract interface.
	struct parsed {
		virtual ~parsed() = default;

		virtual std::unique_ptr<parsed> clone() const = 0;
		virtual void CopyInto(parsed*& dest) const = 0;	// polymorphic assignment
		virtual void MoveInto(parsed*& dest) = 0;	// polymorphic move

		virtual src_location origin() const = 0;

		virtual std::string to_s() const = 0;
		virtual unsigned int precedence() const = 0;

		// unclear if following belong in a sub-interface
		virtual std::optional<perl::scalar> is_not_legal_axiom(bool unconditional) const { return "does not evaluate to a truth value"; }
		virtual std::optional<perl::scalar> before_add_axiom_handler() const { return "does not evaluate to a truth value"; }
	};

	class lex_node final
	{
		kuroda::parser<lex_node>::symbols _prefix;
		kuroda::parser<lex_node>::symbols _infix;
		kuroda::parser<lex_node>::symbols _postfix;
		std::vector<kuroda::parser<lex_node>::symbols> _fragments;
		std::variant<zaimoni::COW<lex_node>,
			zaimoni::COW<word>,
			zaimoni::COW<parsed>,
			std::shared_ptr<const parsed>,
			std::shared_ptr<const lex_node>,
			std::shared_ptr<const word> > _anchor;
		decltype(_anchor) _post_anchor;
		unsigned long long _code; // usually used as a bitmap
		size_t _offset; // used as a linear offset
		mutable std::optional<perl::scalar> _cached_scalar;

		static std::vector<zaimoni::I_erase*> _caches;

		lex_node(kuroda::parser<lex_node>::sequence& dest, size_t lb, size_t ub, unsigned long long code);	// slicing constructor
		// "sinking" constructor
		lex_node(lex_node*& src) noexcept : _anchor(zaimoni::COW<lex_node>(src)), _code(src->_code), _offset(src->_offset) {
			src = nullptr;
		}
		// thin-wrapping constructors
		lex_node(word*& src, unsigned long long code = 0) noexcept : _anchor(zaimoni::COW<word>(src)), _code(code), _offset(0) {
			if (src->code() & Comment) _code |= Comment;
			src = nullptr;
		}


	public:
		using edit_span = kuroda::parser<lex_node>::edit_span;

		lex_node(parsed* src, unsigned long long code = 0) noexcept : _anchor(zaimoni::COW<parsed>(src)), _code(code), _offset(0) {}
		lex_node(parsed*& src, unsigned long long code = 0) noexcept : _anchor(zaimoni::COW<parsed>(src)), _code(code), _offset(0) {
			src = nullptr;
		}
		lex_node(const parsed* src, unsigned long long code = 0) noexcept : _anchor(std::shared_ptr<const parsed>(src)), _code(code), _offset(0) {}
		lex_node(const parsed*& src, unsigned long long code = 0) noexcept : _anchor(std::shared_ptr<const parsed>(src)), _code(code), _offset(0) {
			src = nullptr;
		}
		lex_node(std::shared_ptr<const parsed> src, unsigned long long code = 0) noexcept : _anchor(std::move(src)), _code(code), _offset(0) {}
		lex_node(std::shared_ptr<const lex_node> src, unsigned long long code = 0) noexcept : _anchor(std::move(src)), _code(code), _offset(0) {}
		lex_node(std::shared_ptr<const word> src, unsigned long long code = 0) noexcept : _anchor(std::move(src)), _code(code), _offset(0) {}

		lex_node() noexcept : _code(0), _offset(0) {}
		// \todo anchor constructor
		lex_node(const lex_node& src) = default;
		lex_node(lex_node&& src) = default;
		lex_node& operator=(const lex_node& src) = default;
		lex_node& operator=(lex_node&& src) = default;
		~lex_node() {
			for (decltype(auto) x : _caches) x->erase(this);
		}

		operator perl::scalar() const {
			if (1 == is_pure_anchor()) return c_anchor<formal::word>()->value();
			return to_s();
		}
		const perl::scalar& to_scalar() const {
			if (!_cached_scalar) _cached_scalar = (perl::scalar)(*this);
			return *_cached_scalar;
		}

		// factory function: slices a lex_node out of dest, then puts the lex_node at index lb
		static void slice(kuroda::parser<lex_node>::sequence& dest, size_t lb, size_t ub, unsigned long long code = 0);

		void set_fragment(kuroda::parser<lex_node>::symbols&& src) { _fragments.push_back(std::move(src)); }
		void set_fragments(decltype(_fragments) && src) { _fragments = std::move(src); }

		void set_prefix(kuroda::parser<lex_node>::symbols&& src) { _prefix = std::move(src); }
		void set_postfix(kuroda::parser<lex_node>::symbols&& src) { _postfix = std::move(src); }

		src_location origin() const { return origin(this); }

		auto code() const { return _code; }
		auto offset() const { return _offset; }
		void interpret(unsigned long long src) { _code = src; }
		void interpret(unsigned long long src, unsigned long long offset) { _code = src; _offset = offset; }
		void learn(unsigned long long src) { _code |= src; }
		void learn(unsigned long long src, unsigned long long offset) { _code |= src; _offset = offset; }
		void forget(unsigned long long src) { _code &= ~src; }

		template<class Val>
		Val* anchor() requires requires { std::get_if<zaimoni::COW<Val> >(&_anchor); }
		{
			if (auto x = std::get_if<zaimoni::COW<Val> >(&_anchor)) return x->get();
			return nullptr;
		}

		template<class Val>
		const Val* c_anchor() const requires requires { std::get_if<zaimoni::COW<Val> >(&_anchor); std::get_if<std::shared_ptr<const Val> >(&_anchor); }
		{
			if (auto x = std::get_if<zaimoni::COW<Val> >(&_anchor)) return x->get_c();
			if (auto x = std::get_if<std::shared_ptr<const Val> >(&_anchor)) return x->get();
			return nullptr;
		}

		template<class Val>
		Val* post_anchor() requires requires { std::get_if<zaimoni::COW<Val> >(&_post_anchor); }
		{
			if (auto x = std::get_if<zaimoni::COW<Val> >(&_post_anchor)) return x->get();
			return nullptr;
		}

		template<class Val>
		const Val* c_post_anchor() const requires requires { std::get_if<zaimoni::COW<Val> >(&_post_anchor); std::get_if<std::shared_ptr<const Val> >(&_anchor); }
		{
			if (auto x = std::get_if<zaimoni::COW<Val> >(&_post_anchor)) return x->get_c();
			if (auto x = std::get_if<std::shared_ptr<const Val> >(&_post_anchor)) return x->get();
			return nullptr;
		}

		template<class T>
		std::shared_ptr<const T> shared_anchor() = delete;

		// 2025-06-19: fully specializing template doesn't work; the concept gets called instead
		std::shared_ptr<const parsed> shared_anchor_is_parsed() {
			if (auto x = std::get_if<std::shared_ptr<const parsed> >(&_anchor)) return *x;
			if (auto x = std::get_if<zaimoni::COW<parsed> >(&_anchor)) return x->get_shared();
			return nullptr;
		}

		std::shared_ptr<const lex_node> shared_anchor_is_lex_node() {
			if (auto x = std::get_if<std::shared_ptr<const lex_node> >(&_anchor)) return *x;
			if (auto x = std::get_if<zaimoni::COW<lex_node> >(&_anchor)) return x->get_shared();
			return nullptr;
		}

		std::shared_ptr<const word> shared_anchor_is_word() {
			if (auto x = std::get_if<std::shared_ptr<const word> >(&_anchor)) return *x;
			if (auto x = std::get_if<zaimoni::COW<word> >(&_anchor)) return x->get_shared();
			return nullptr;
		}

		// 2025-05-26: Gentzen.exe regresses badly if shared_anchor<parsed>() is called here
		template<std::derived_from<formal::parsed> T>
		std::shared_ptr<const T> shared_anchor() {
			if (auto x = std::get_if<std::shared_ptr<const parsed> >(&_anchor)) {
				if (auto test = dynamic_cast<const T*>(x->get())) {
					return std::shared_ptr<const T>(*x, test);
				}
			}
			return nullptr;
		}

		template<class T>
		std::shared_ptr<const T> shared_post_anchor() = delete;

		template<std::derived_from<formal::parsed> T>
		std::shared_ptr<const T> shared_post_anchor() {
			if (auto x = std::get_if<std::shared_ptr<const parsed> >(&_post_anchor)) {
				if (auto test = dynamic_cast<const T*>(x->get())) {
					return std::shared_ptr<const T>(*x, test);
				}
			}
			return nullptr;
		}

		std::shared_ptr<const lex_node> shared_post_anchor_is_lex_node() {
			if (auto x = std::get_if<std::shared_ptr<const lex_node> >(&_post_anchor)) return *x;
			if (auto x = std::get_if<zaimoni::COW<lex_node> >(&_post_anchor)) return x->get_shared();
			return nullptr;
		}

		std::shared_ptr<const word> shared_post_anchor_is_word() {
			if (auto x = std::get_if<std::shared_ptr<const word> >(&_post_anchor)) return *x;
			if (auto x = std::get_if<zaimoni::COW<word> >(&_post_anchor)) return x->get_shared();
			return nullptr;
		}

		bool set_null_post_anchor(lex_node*& src) {
			if (c_post_anchor<word>()) return false;
			if (c_post_anchor<lex_node>()) return false;
			if (c_post_anchor<parsed>()) return false;
			_post_anchor = zaimoni::COW<lex_node>(src);
			src = nullptr;
			return true;
		}

		template<class T>
		T* release_post_anchor() = delete;

		template<>
		formal::lex_node* release_post_anchor<formal::lex_node>() {
			if (auto x = std::get_if<zaimoni::COW<formal::lex_node> >(&_post_anchor)) return x->release();
			return nullptr;
		}

		auto anchor_code() const { return classify(_anchor); }
		auto post_anchor_code() const { return classify(_post_anchor); }

		bool syntax_ok() const;
		int is_pure_anchor() const; // C error code convention
		std::optional<int> token_compare(const formal::lex_node& rhs) const;
		std::optional<int> token_compare(const formal::word& rhs) const;
		std::optional<int> token_compare(kuroda::parser<formal::lex_node>::edit_span tokens) const;

		auto& prefix() const { return _prefix; }
		auto& infix() const { return _infix; }
		auto& postfix() const { return _postfix; }
		auto& fragments() const { return _fragments; }

		auto& prefix() { return _prefix; }
		auto& infix() { return _infix; }
		auto& postfix() { return _postfix; }
		auto& fragments() { return _fragments; }

		static std::unique_ptr<lex_node> pop_front(kuroda::parser<formal::word>::sequence& src);
		static std::unique_ptr<lex_node> pop_front(kuroda::parser<formal::lex_node>::sequence& src);

		template<class Ret>
		auto is_ok(std::function<Ret(const lex_node&)> ok) const { return ok(*this); }

		static bool rewrite(lex_node** target, std::function<lex_node* (lex_node* target)> op);
		static bool rewrite(lex_node*& target, std::function<lex_node*(lex_node* target)> op);
		bool recursive_rewrite(std::function<bool(const lex_node&)> test, std::function<void(lex_node&)> op);

		std::string to_s() const;

		unsigned int precedence() const;

		bool is_balanced_pair(const std::string_view& l_token, const std::string_view& r_token) const;
		static lex_node** find_binding_predecessor(lex_node** src);

		bool has_outer_parentheses() const;
		static bool remove_outer_parentheses(kuroda::parser<formal::lex_node>::symbols& target);
		static std::vector<edit_span> split(kuroda::parser<lex_node>::sequence& src, std::function<bool(const lex_node&)> ok);
		static std::vector<edit_span> split(const edit_span& src, std::function<bool(const lex_node&)> ok);

		static auto where_is(const edit_span& view) { return view.offset; }

		static std::vector<kuroda::parser<lex_node>::symbols> move_per_spec(const std::vector<edit_span>& src);

		static void force_empty_prefix_postfix_fragments(lex_node*& dest);

	private:
		static src_location origin(const lex_node* src);
		static src_location origin(const decltype(_anchor)& src);

		static perl::scalar to_scalar(const decltype(_anchor)& src);

		static int classify(const decltype(_anchor)& src);
		static void reset(decltype(_anchor)& dest, lex_node*& src);
	};

	std::string to_string(const kuroda::parser<formal::lex_node>::sequence& src);

} // namespace formal

// hooks to be provided by the library user
void error_report(const formal::src_location& loc, const perl::scalar& err);
void warning_report(const formal::src_location& loc, const perl::scalar& warn);

// standard thin wrappers
void error_report(formal::lex_node& fail, const perl::scalar& err);
void error_report(const formal::parsed& fail, const perl::scalar& err);

void warning_report(const formal::lex_node& fail, const perl::scalar& err);
void warning_report(const formal::parsed& fail, const perl::scalar& err);

#endif
