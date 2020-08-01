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

namespace kuroda {
	template<class T>
	class parser {
	public:
//		using sequence = std::vector<zaimoni::autoval_ptr<T> >;
		using sequence = zaimoni::_meta_autoarray_ptr<T*>;
		using rewriter = std::function<std::vector<size_t>(sequence&, size_t)>;

	private:
		std::vector<std::function<bool(T&)> > label_terminal;
		std::vector<std::function<bool(T*&)> > reformat_to_terminal;
		std::vector<std::function<rewriter (T&)> > label_terminal_hint;
		std::vector<std::function<rewriter (T*&)> > reformat_to_terminal_hint;

		std::vector<rewriter> build_nonterminal;
		std::vector<rewriter> rearrange_nonterminal;

	public:
		parser() = default;
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
		void register_terminal(const std::function<rewriter(T&)>& x) { label_terminal_hint.push_back(x); }
		void register_terminal(std::function<rewriter(T&)>&& x) { label_terminal_hint.push_back(std::move(x)); }
		void register_terminal(const std::function<rewriter(T*&)>& x) { reformat_to_terminal_hint.push_back(x); }
		void register_terminal(std::function<rewriter(T*&)>&& x) { reformat_to_terminal_hint.push_back(std::move(x)); }

		// non-terminals
		void register_build_nonterminal(const rewriter& x) { build_nonterminal.push_back(x); }
		void register_build_nonterminal(rewriter&& x) { build_nonterminal.push_back(std::move(x)); }

		void register_rearrange_nonterminal(const rewriter& x) { rearrange_nonterminal.push_back(x); }
		void register_rearrange_nonterminal(rewriter&& x) { rearrange_nonterminal.push_back(std::move(x)); }

		void append_to_parse(sequence& dest, T* src) {
			if (!src) return;
			std::vector<size_t> mutated;
			mutated.push_back(dest.size());
			{ // scoping brace
			auto hint = notice_terminal(src);
			dest.push_back(src);
			if (hint) {
				size_t viewpoint = mutated.back();
				mutated.pop_back();
				auto check_these = hint(dest, viewpoint);
				if (!check_these.empty()) {
					if (2 <= check_these.size()) std::sort(check_these.begin(), check_these.end());
					while (!mutated.empty() && mutated.back() >= check_these.front()) mutated.pop_back();
					for (decltype(auto) i : check_these) mutated.push_back(i);
				}
			}
			}	// scoping brace: force hint to destruct
			while (!mutated.empty()) {
				size_t viewpoint = mutated.back();
				mutated.pop_back();
				if (dest.size() <= viewpoint) continue;	// invalid
				auto check_these = refine_parse(dest, viewpoint);
				if (!check_these.empty()) {
					if (2 <= check_these.size()) std::sort(check_these.begin(), check_these.end());
					while (!mutated.empty() && mutated.back() >= check_these.front()) mutated.pop_back();
					for (decltype(auto) i : check_these) mutated.push_back(i);
				}
			}
		}

	private:
		rewriter notice_terminal(T*& src) {
			for (decltype(auto) test : label_terminal) if (test(*src)) return nullptr;
			for (decltype(auto) test : reformat_to_terminal) if (test(src)) return nullptr;
			for (decltype(auto) test : label_terminal_hint) if (auto ret = test(*src)) return ret;
			for (decltype(auto) test : reformat_to_terminal_hint) if (auto ret = test(src)) return ret;
			return nullptr;
		}
		std::vector<size_t> refine_parse(sequence& dest, size_t viewpoint) {
			std::vector<size_t> ret;
			for (decltype(auto) test : build_nonterminal) {
				ret = test(dest, viewpoint);
				if (!ret.empty()) return ret;
			}
			for (decltype(auto) test : rearrange_nonterminal) {
				ret = test(dest, viewpoint);
				if (!ret.empty()) return ret;
			}
			return ret;
		}
	};
}

#endif
