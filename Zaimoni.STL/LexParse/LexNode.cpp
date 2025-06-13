#include "LexNode.hpp"

namespace formal {

	std::vector<zaimoni::I_erase*> lex_node::_caches;

	// we only handle closed intervals
	lex_node::lex_node(kuroda::parser<lex_node>::sequence& dest, size_t lb, size_t ub, unsigned long long code)
		: _code(code), _offset(0)
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
		if (!_fragments.empty()) {
			if (!_prefix.empty()) return false;
			if (!_infix.empty()) return false;
			if (!_postfix.empty()) return false;
			if (classify(_post_anchor)) return false;
		} else {
			if (!_infix.empty() && !classify(_post_anchor)) return false; // if we have an infix sequence, we have a post-anchor token as well
		}

		// \todo language-specific checks go here
		return true;
	}

	/// <returns>1 if word anchor; 2 if node anchor; 0 for other data fields; -1 for no data at all (invalid)</returns>
	int lex_node::is_pure_anchor() const
	{
		if (!_prefix.empty()) return 0;
		if (!_infix.empty()) return 0;
		if (!_postfix.empty()) return 0;
		if (!_fragments.empty()) return 0;
		if (classify(_post_anchor)) return 0;
		if (int code = classify(_anchor)) return code;
		return -1;
	}

	std::optional<int> lex_node::token_compare(const formal::lex_node& rhs) const
	{
		if (1 == is_pure_anchor() && 1 == rhs.is_pure_anchor()) {
			if (auto x = c_anchor<formal::word>()) {
				if (auto y = rhs.c_anchor<formal::word>()) return x->value().compare(y->value());
			}
		};
		return std::nullopt;
	}

	std::optional<int> lex_node::token_compare(const formal::word& rhs) const
	{
		if (1 == is_pure_anchor()) {
			if (auto x = c_anchor<formal::word>()) return x->value().compare(rhs.value());
		};
		return std::nullopt;
	}

	std::optional<int> lex_node::token_compare(kuroda::parser<formal::lex_node>::edit_span tokens) const
	{
		// implement these later
		if (!_prefix.empty()) {
			if (tokens.empty()) return 1;
			std::cout << "lex_node::token_compare wants prefix handling\n";
			return std::nullopt;
		}
		if (classify(_anchor)) {
			if (tokens.empty()) return 1;
			bool ok = false;
			if (auto x = c_anchor<formal::word>()) {
				if (const auto code = tokens.front()->token_compare(*x)) {
					if (*code) return -(*code);
					ok = true;
					tokens.pop_front();
				}
			}
			if (!ok) {
				std::cout << "lex_node::token_compare wants anchor handling\n";
				return std::nullopt;
			}
		}
		if (!_infix.empty()) {
			if (tokens.empty()) return 1;
			std::cout << "lex_node::token_compare wants infix handling\n";
			return std::nullopt;
		}
		if (classify(_post_anchor)) {
			if (tokens.empty()) return 1;
			std::cout << "lex_node::token_compare wants post-anchor handling\n";
			return std::nullopt;
		}
		if (!_postfix.empty()) {
			bool ok = true;
			for (auto test : _postfix) {
				if (tokens.empty()) return 1;
				if (1 != test->is_pure_anchor()) break;
				if (auto x = test->c_anchor<formal::word>()) {
					if (const auto code = tokens.front()->token_compare(*x)) {
					    if (*code) return -(*code);
						tokens.pop_front();
					} else {
						return std::nullopt;
					}
				} else {
					ok = false;
					break;
				}
			}
			if (!ok) {
				std::cout << "lex_node::token_compare wants postfix handling\n";
				return std::nullopt;
			}
		}
		if (!_fragments.empty()) {
			if (tokens.empty()) return 1;
			std::cout << "lex_node::token_compare wants fragments handling\n";
			return std::nullopt;
		}
		return tokens.empty() ? 0 : -1;
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

	bool lex_node::rewrite(lex_node** target, std::function<lex_node* (lex_node* target)> op)
	{
		if (decltype(auto) dest = op(*target)) {
			if (*target != dest) {
				delete *target;
				*target = dest;
			}
			return true;
		}
		return false;
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

	bool lex_node::recursive_rewrite(std::function<bool(const lex_node&)> test, std::function<void(lex_node&)> op)
	{
		bool ret = false;

		if (!_prefix.empty()) {
			for (decltype(auto) x : _prefix) {
				if (test(*x)) {
					op(*x);
					ret = true;
				}
			}
		}
		if (!_infix.empty()) {
			for (decltype(auto) x : _infix) {
				if (test(*x)) {
					op(*x);
					ret = true;
				}
			}
		}
		if (!_postfix.empty()) {
			for (decltype(auto) x : _postfix) {
				if (test(*x)) {
					op(*x);
					ret = true;
				}
			}
		}
		if (!_fragments.empty()) {
			for (decltype(auto) scan : _fragments) {
				for (decltype(auto) x : scan) {
					if (test(*x)) {
						op(*x);
						ret = true;
					}
				}
			}
		} else { // only check anchor if no fragments
			if (decltype(auto) x = c_anchor<formal::lex_node>()) {
				if (test(*x)) {
					op(*anchor<formal::lex_node>());
					ret = true;
				}
			}
		}
		if (decltype(auto) x = c_post_anchor<formal::lex_node>()) {
			if (test(*x)) {
				op(*post_anchor<formal::lex_node>());
				ret = true;
			}
		}
		if (test(*this)) {
			op(*this);
			ret = true;
		}
		return ret;
	}

	static void push_back(std::vector<perl::scalar>& dest, const kuroda::parser<lex_node>::symbols& src)
	{
		for (const formal::lex_node* x : src) {
			dest.push_back((perl::scalar)(*x));
		}
	}

	std::string lex_node::to_s() const
	{
		std::vector<perl::scalar> stage;
		if (!_fragments.empty()) {
			const auto sep = to_scalar(_anchor);

			bool is_first = true;
			for (decltype(auto) x : _fragments) {
				std::vector<perl::scalar> stage2;
				push_back(stage2, x);
				stage.push_back(join(stage2, " "));
			}
			return join(stage, sep.view());
		}

		if (!_prefix.empty()) push_back(stage, _prefix);
		stage.push_back(to_scalar(_anchor));
		if (!_infix.empty()) push_back(stage, _infix);
		if (classify(_post_anchor)) stage.push_back(to_scalar(_post_anchor));
		if (!_postfix.empty()) push_back(stage, _postfix);
		return join(stage, " ");
	}

	std::string to_string(const kuroda::parser<formal::lex_node>::symbols& src)
	{
		std::vector<perl::scalar> stage;
		for (const formal::lex_node* x : src) {
			stage.push_back((perl::scalar)(*x));
		}
		return join(stage, " ");
	}

	perl::scalar lex_node::to_scalar(const decltype(_anchor)& src)
	{
		struct _to_s {
			auto operator()(const zaimoni::COW<word>& w) {
				return perl::scalar(w->value());
			}
			auto operator()(const zaimoni::COW<lex_node>& x) {
				return perl::scalar(x->to_s());
			}
			auto operator()(const zaimoni::COW<parsed>& x) {
				return perl::scalar(x->to_s());
			}
		};

		return std::visit(_to_s(), src);
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

	lex_node** lex_node::find_binding_predecessor(lex_node** src) {
		if (!src) return nullptr;
restart:
		if (!(*src)->_postfix.empty()) {
			src = &((*src)->_postfix.back());
			goto restart;
		}
		return src;
	}

	bool lex_node::has_outer_parentheses() const
	{
		return  _prefix.empty() && _postfix.empty() && is_balanced_pair("(", ")");
	}

	bool lex_node::remove_outer_parentheses(kuroda::parser<formal::lex_node>::symbols& target) {
		bool ret = false;
		while (   1 == target.size() && !target.front()->infix().empty()
			   && target.front()->has_outer_parentheses()) {
			kuroda::parser<formal::lex_node>::symbols stage;

			stage.swap(target.front()->_infix);
			stage.swap(target);
			ret = true;
		}
		return ret;
	}

	std::vector<lex_node::edit_span> lex_node::split(kuroda::parser<lex_node>::sequence& src, std::function<bool(const lex_node&)> ok)
	{
		std::vector<edit_span> ret;
		std::vector<size_t> indexes;

		size_t n = 0;
		while (src.size() > n) {
			if (ok(*(src[n++]))) indexes.push_back(n-1);
		}
		if (indexes.empty()) return ret;

		size_t origin = 0;
		for (const auto i : indexes) {
			const auto local_delta = origin < i ? i - origin : 0;
			ret.emplace_back(&src, origin, local_delta);
			origin = i + 1;
		}

		(decltype(indexes)()).swap(indexes);

		const auto local_delta = origin < src.size() ? src.size() - origin : 0;
		ret.emplace_back(&src, origin, local_delta);
		return ret;
	}

	std::vector<lex_node::edit_span> lex_node::split(const edit_span& src, std::function<bool(const lex_node&)> ok)
	{
		enum { trace_parse = 0 };

		std::vector<lex_node::edit_span> ret;
		std::vector<size_t> indexes;
		const auto delta = where_is(src);

		if constexpr (trace_parse) {
			std::cout << "lex_node::split: src.size(): " << src.size() << " " << src.extent << "\n";
		}

		size_t n = 0;
		while (src.size() > n) {
			if (ok(*src[n++])) indexes.push_back(n - 1);
		}
		if constexpr (trace_parse) {
			std::cout << "lex_node::split: indexes.size(): " << indexes.size() << "\n";
		}

		if (indexes.empty()) return ret;

		size_t origin = 0;
		for (const auto i : indexes) {
			const auto local_delta = (origin < i) ? i - origin : 0;
			ret.emplace_back(src.src, delta + origin, local_delta);
			origin = i + 1;
		}

		(decltype(indexes)()).swap(indexes);

		const auto local_delta = (origin < src.size()) ? src.size() - origin : 0;
		ret.emplace_back(src.src, delta + origin, local_delta);
		return ret;
	}

	std::vector<kuroda::parser<lex_node>::symbols> lex_node::move_per_spec(const std::vector<edit_span>& src)
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

void error_report(formal::lex_node& fail, const perl::scalar& err) {
	error_report(fail.origin(), err);
	fail.learn(formal::Error);
}

void error_report(const formal::parsed& fail, const perl::scalar& err) { error_report(fail.origin(), err); }

void warning_report(const formal::lex_node& fail, const perl::scalar& err) { warning_report(fail.origin(), err); }
void warning_report(const formal::parsed& fail, const perl::scalar& err) { warning_report(fail.origin(), err); }
