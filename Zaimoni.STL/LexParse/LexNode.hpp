#ifndef ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP
#define ZAIMONI_STL_LEXPARSE_LEX_NODE_HPP 1

#include "Kuroda.hpp"
#include "FormalWord.hpp"
#include "../cache.hpp"
#include "../COW.hpp"
#include <span>

namespace formal {

	class lex_node;
	struct placeholder_match;	// defined after lex_node

	// abstract interface.
	struct parsed {
		virtual ~parsed() = default;

		virtual std::unique_ptr<parsed> clone() const = 0;
		virtual void CopyInto(parsed*& dest) const = 0;	// polymorphic assignment
		virtual void MoveInto(parsed*& dest) = 0;	// polymorphic move

		virtual src_location origin() const = 0;

		virtual std::string to_s() const = 0;
//		virtual void diagnose(std::vector<std::string>& dest) const = 0;
		virtual void diagnose(std::vector<std::string>& dest) const {}
		std::vector<std::string> diagnose() const {
			std::vector<std::string> ret;
			diagnose(ret);
			return ret;
		}

		// returns std::nullopt when the parsed object has no recognizable placeholder structure.
		// when non-null, the returned placeholder_match owns a freshly-allocated, non-aliased
		// lex_node tree, and the handles in groups point into that tree.
		virtual std::optional<placeholder_match> get_placeholder_variables() const;

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
	public:
		// thin-wrapping constructors
		lex_node(word*& src, unsigned long long code = 0) noexcept : _anchor(zaimoni::COW<word>(src)), _code(code), _offset(0) {
			if (src->code() & Comment) _code |= Comment;
			src = nullptr;
		}


		using edit_span = kuroda::parser<lex_node>::edit_span;

		lex_node(parsed* src, unsigned long long code = 0) noexcept : _anchor(zaimoni::COW<parsed>(src)), _code(code), _offset(0) {}
		lex_node(parsed*& src, unsigned long long code = 0) noexcept : _anchor(zaimoni::COW<parsed>(src)), _code(code), _offset(0) {
			src = nullptr;
		}
		lex_node(const parsed* src, unsigned long long code = 0) noexcept : _anchor(std::shared_ptr<const parsed>(src)), _code(code), _offset(0) {}
		lex_node(const parsed*& src, unsigned long long code = 0) noexcept : _anchor(std::shared_ptr<const parsed>(src)), _code(code), _offset(0) {
			src = nullptr;
		}

		// There is an implicit conversion from std::unique_ptr to std::shared_ptr.
		// We don't want that here: hard crash during YAML configuration loading
		explicit lex_node(std::shared_ptr<const parsed> src, unsigned long long code = 0) noexcept : _anchor(std::move(src)), _code(code), _offset(0) {}
		explicit lex_node(std::shared_ptr<const lex_node> src, unsigned long long code = 0) noexcept : _anchor(std::move(src)), _code(code), _offset(0) {}
		explicit lex_node(std::shared_ptr<const word> src, unsigned long long code = 0) noexcept : _anchor(std::move(src)), _code(code), _offset(0) {}

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

		void set_fragment(kuroda::parser<lex_node>::symbols&& src) { _fragments.push_back(std::move(src)); _invalidate_scalar(); }
		void set_fragments(decltype(_fragments) && src) { _fragments = std::move(src); _invalidate_scalar(); }

		void set_prefix(kuroda::parser<lex_node>::symbols&& src) { _prefix = std::move(src); _invalidate_scalar(); }
		void set_postfix(kuroda::parser<lex_node>::symbols&& src) { _postfix = std::move(src); _invalidate_scalar(); }

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
			if (auto x = std::get_if<zaimoni::COW<Val> >(&_anchor)) {
				_invalidate_scalar();
				return x->get();
			}
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
			if (auto x = std::get_if<zaimoni::COW<Val> >(&_post_anchor)) {
				_invalidate_scalar();
				return x->get();
			}
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
		std::shared_ptr<const T> shared_anchor() {
			if (auto x = std::get_if<std::shared_ptr<const T> >(&_anchor)) return *x;
			if (auto x = std::get_if<zaimoni::COW<T> >(&_anchor)) return x->get_shared();
			return nullptr;
		}

#if 0
		// need a free adapter, rather than this
		template<std::derived_from<formal::parsed> T>
		std::shared_ptr<const T> shared_anchor() {
			if (auto x = std::get_if<std::shared_ptr<const parsed> >(&_anchor)) {
				if (auto test = dynamic_cast<const T*>(x->get())) {
					return std::shared_ptr<const T>(*x, test);
				}
			}
			return nullptr;
		}
#endif

		template<class T>
		std::shared_ptr<const T> shared_post_anchor() {
			if (auto x = std::get_if<std::shared_ptr<const T> >(&_post_anchor)) return *x;
			if (auto x = std::get_if<zaimoni::COW<T> >(&_post_anchor)) return x->get_shared();
			return nullptr;
		}

#if 0
		// need a free adapter, rather than this
		template<std::derived_from<formal::parsed> T>
		std::shared_ptr<const T> shared_post_anchor() {
			if (auto x = std::get_if<std::shared_ptr<const parsed> >(&_post_anchor)) {
				if (auto test = dynamic_cast<const T*>(x->get())) {
					return std::shared_ptr<const T>(*x, test);
				}
			}
			return nullptr;
		}
#endif

		bool set_null_post_anchor(lex_node*& src) {
			if (c_post_anchor<word>()) return false;
			if (c_post_anchor<lex_node>()) return false;
			if (c_post_anchor<parsed>()) return false;
			_post_anchor = zaimoni::COW<lex_node>(src);
			src = nullptr;
			_invalidate_scalar();
			return true;
		}

		bool set_null_post_anchor(word*& src) {
			if (c_post_anchor<word>()) return false;
			if (c_post_anchor<lex_node>()) return false;
			if (c_post_anchor<parsed>()) return false;
			_post_anchor = zaimoni::COW<word>(src);
			src = nullptr;
			_invalidate_scalar();
			return true;
		}

		template<class T>
		T* release_post_anchor() = delete;

		template<>
		formal::lex_node* release_post_anchor<formal::lex_node>() {
			if (auto x = std::get_if<zaimoni::COW<formal::lex_node> >(&_post_anchor)) return x->release();
			return nullptr;
		}

		void diagnose(std::vector<std::string>& dest) const;
		std::vector<std::string> diagnose() const;

		auto anchor_code() const noexcept { return classify(_anchor); }
		auto post_anchor_code() const noexcept { return classify(_post_anchor); }

		bool syntax_ok() const noexcept;
		int is_pure_anchor() const noexcept; // C error code convention
		std::optional<int> token_compare(const formal::lex_node& rhs) const;
		std::optional<int> token_compare(const formal::word& rhs) const;
		std::optional<int> token_compare(kuroda::parser<formal::lex_node>::edit_span tokens) const;

		auto& prefix() const { return _prefix; }
		auto& infix() const { return _infix; }
		auto& postfix() const { return _postfix; }
		auto& fragments() const { return _fragments; }

		auto& prefix() { _invalidate_scalar(); return _prefix; }
		auto& infix() { _invalidate_scalar(); return _infix; }
		auto& postfix() { _invalidate_scalar(); return _postfix; }
		auto& fragments() { _invalidate_scalar(); return _fragments; }

		static std::unique_ptr<lex_node> pop_front(kuroda::parser<formal::word>::sequence& src);
		static std::unique_ptr<lex_node> pop_front(kuroda::parser<formal::lex_node>::sequence& src);

		template<class Ret>
		auto is_ok(std::function<Ret(const lex_node&)> ok) const { return ok(*this); }

		static bool rewrite(lex_node** target, std::function<lex_node* (lex_node* target)> op);
		static bool rewrite(lex_node*& target, std::function<lex_node*(lex_node* target)> op);
		bool recursive_rewrite(std::function<bool(const lex_node&)> test, std::function<void(lex_node&)> op);

		std::string to_s() const;

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
		void _invalidate_scalar() { _cached_scalar = std::nullopt; }

		static src_location origin(const lex_node* src);
		static src_location origin(const decltype(_anchor)& src);

		static perl::scalar to_scalar(const decltype(_anchor)& src);

		static int classify(const decltype(_anchor)& src) noexcept;
		static void reset(decltype(_anchor)& dest, lex_node*& src);

		static void diagnose(const decltype(_anchor)& src, std::vector<std::string>& dest, const std::string& pre);
		static void diagnose(const decltype(_prefix)& src, std::vector<std::string>& dest, const std::string& pre);
	};

	std::string to_string(const kuroda::parser<formal::lex_node>::sequence& src);
	std::vector<std::string> diagnose(const kuroda::parser<formal::lex_node>::sequence& src);

	// mutable handle into a parent's slot for a placeholder occurrence.
	// preconditions: the parent slot outlives this handle, and the underlying
	// subtree is freshly-allocated and not aliased elsewhere.  Lifetime should
	// not extend past the substitution call that consumed it.
	struct placeholder_handle : public std::variant<std::shared_ptr<const lex_node>*, lex_node**> {
		using base = std::variant<std::shared_ptr<const lex_node>*, lex_node**>;
		using base::base;
		using base::operator=;

		perl::scalar name() const;
		void replace(lex_node* src);
		void replace(std::shared_ptr<const lex_node> src);
	};

	// the freshly-allocated tree owns the storage that the handles reference.
	struct placeholder_match {
		std::unique_ptr<lex_node> tree;
		std::vector<std::pair<perl::scalar, std::vector<placeholder_handle>>> groups;
	};

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
