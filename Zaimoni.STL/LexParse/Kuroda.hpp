#ifndef ZAIMONI_STL_LEXPARSE_KURODA_HPP
#define ZAIMONI_STL_LEXPARSE_KURODA_HPP 1

#include "../AutoPtr.hpp"
#include <vector>
#include <memory>
#include <functional>

/*
https://en.wikipedia.org/wiki/Kuroda_normal_form:

In formal language theory, a grammar is in Kuroda normal form if all production rules are of the form:

AB -> CD or
A -> BC or
A -> B or
A -> a
where A, B, C and D are nonterminal symbols and a is a terminal symbol. Some sources omit the A -> B pattern.

*/
// A -> BC: build
// AB -> CD: rearrange (but uses same function signature as build)
// left/right sequence edge rules may be more important

namespace kuroda {
	template<class T>
	class parser {
	public:
//		using sequence = std::vector<std::unique_ptr<T> >;
		using sequence = zaimoni::_meta_autoarray_ptr<T*>;
		using symbols = zaimoni::autovalarray_ptr_throws<T*>;
//		using symbols = std::vector<std::unique_ptr<T> >;
		using rewriter = std::function<std::vector<size_t>(sequence&, size_t)>;
		// hinting (using a return value of rewriter) looked interesting but in practice it doesn't work (many parse rules work from
		// the same rightmost token trigger for efficiency reasons)

	private:
		std::vector<std::function<bool(T&)> > label_terminal;
		std::vector<std::function<bool(T*&)> > reformat_to_terminal;

		std::vector<rewriter> build_nonterminal;
		std::vector<rewriter> left_edge_build_nonterminal;
		std::vector<rewriter> right_edge_build_nonterminal;

		bool (*sub_recursion_ok)(const T& src);

	public:
		parser() : sub_recursion_ok(nullptr) {};
		parser(decltype(sub_recursion_ok) sub_recursion_ok) : sub_recursion_ok(sub_recursion_ok) {};
		parser(const parser& src) = default;
		parser(parser && src) = default;
		parser& operator=(const parser& src) = default;
		parser& operator=(parser && src) = default;
		~parser() = default;

		// terminals -- these are to be processed "on presentation"
		void register_terminal(const std::function<bool(T&)>& x) { label_terminal.push_back(x); }
		void register_terminal(std::function<bool(T&)>&& x) { label_terminal.push_back(std::move(x)); }
		void register_terminal(const std::function<bool(T*&)>& x) { reformat_to_terminal.push_back(x); }
		void register_terminal(std::function<bool(T*&)>&& x) { reformat_to_terminal.push_back(std::move(x)); }

		// non-terminals
		void register_build_nonterminal(const rewriter& x) { build_nonterminal.push_back(x); }
		void register_build_nonterminal(rewriter&& x) { build_nonterminal.push_back(std::move(x)); }

		void register_left_edge_build_nonterminal(const rewriter& x) { left_edge_build_nonterminal.push_back(x); }
		void register_left_edge_build_nonterminal(rewriter&& x) { left_edge_build_nonterminal.push_back(std::move(x)); }

		void register_right_edge_build_nonterminal(const rewriter& x) { right_edge_build_nonterminal.push_back(x); }
		void register_right_edge_build_nonterminal(rewriter&& x) { right_edge_build_nonterminal.push_back(std::move(x)); }

		bool want_sub_recursion(const T& src) { return sub_recursion_ok && sub_recursion_ok(src); }

		void append_to_parse(sequence& dest, T* src) {
			if (!src) return;
			notice_terminal(src);
			if (!src) return;
			size_t viewpoint = dest.size();
			dest.push_back(src);
			do {
				auto check_these = refine_parse(dest, viewpoint);
				if (!check_these.empty()) {
					if (2 <= check_these.size()) std::sort(check_these.begin(), check_these.end());
					viewpoint = check_these.front() - 1; // will be correct after end-of-loop increment
					continue;
				}
			} while (dest.size() > ++viewpoint);
		}

		std::vector<size_t> left_edge_at(sequence& dest, size_t viewpoint) {
			std::vector<size_t> ret;
			for (decltype(auto) test : left_edge_build_nonterminal) {
				ret = test(dest, viewpoint);
				if (!ret.empty()) return ret;
			}
			return ret;
		}

		std::vector<size_t> right_edge_at(sequence& dest, size_t viewpoint) {
			std::vector<size_t> ret;
			for (decltype(auto) test : right_edge_build_nonterminal) {
				ret = test(dest, viewpoint);
				if (!ret.empty()) return ret;
			}
			return ret;
		}

		bool finite_parse(sequence& dest) {
			if (dest.empty()) return false;
			size_t viewpoint = 0;
			bool changed = false;
restart:
			do {
				auto check_these = refine_parse(dest, viewpoint);
				if (!check_these.empty()) {
					if (2 <= check_these.size()) std::sort(check_these.begin(), check_these.end());
					viewpoint = check_these.front() - 1; // will be correct after end-of-loop increment
					changed = true;
					continue;
				}
			} while (dest.size() > ++viewpoint);
			{
			auto check_these = left_edge_at(dest, 0);
			if (!check_these.empty()) {
				if (2 <= check_these.size()) std::sort(check_these.begin(), check_these.end());
				viewpoint = check_these.front();
				changed = true;
				goto restart;
			}
			}
			{
			auto check_these = right_edge_at(dest, dest.size()-1);
			if (!check_these.empty()) {
				if (2 <= check_these.size()) std::sort(check_these.begin(), check_these.end());
				viewpoint = check_these.front();
				changed = true;
				goto restart;
			}
			}
			return changed;
		}

		std::optional<size_t> recurse(sequence& tokens) {
			std::optional<size_t> ret;

			ptrdiff_t ub = tokens.size();
			while (0 <= --ub) {
				if (T::recurse(*this, tokens[ub])) ret = ub;
			}
			return ret;
		}

	private:
		void notice_terminal(T*& src) {
			for (decltype(auto) test : label_terminal) if (test(*src)) return;
			for (decltype(auto) test : reformat_to_terminal) if (test(src)) return;
		}
		std::vector<size_t> refine_parse(sequence& dest, size_t viewpoint) {
			std::vector<size_t> ret;
			for (decltype(auto) test : build_nonterminal) {
				ret = test(dest, viewpoint);
				if (!ret.empty()) return ret;
			}
			return ret;
		}
	};
}

#endif
