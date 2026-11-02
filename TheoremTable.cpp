// TheoremTable: renders the theorem file GentzenW's proof session writes
// (<script>.theorem.yaml) as the docs folder's Gentzen proof table.
// Specification: Notes/GentzenW-theorem-table.md.  The file format is the only contract
// with GentzenW; nothing here depends on it, on Zaimoni.STL, or on fkYAML.
//
// Why not fkYAML: the vendored version's lexer treats the "," inside a plain scalar such
// as "(P &#9500; Q) , P &#9500; Q" as a flow-sequence separator and asserts
// (m_flow_context_depth > 0), so it cannot read back what its own serializer wrote.  The
// writer's output is a small, fixed YAML subset, read here directly: a "theorem:" mapping
// with keys axioms (block sequence of scalars), name, proof (block sequence of four- or
// five-cell block sequences: the fifth cell, when present, is the hypothesis nesting depth
// the Fitch rendering draws) and result; scalars plain or double-quoted; empty cells as
// "- " with nothing after.  Keys may come in any order.

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct options {
	std::string out;	// empty: stdout
	std::string align = "center";
	bool page = false;
	bool axioms = true;
	bool name = true;
	bool fitch = false;
};

struct proof_row {
	std::array<std::string, 4> cells;	// row label, inference-rule column, proposition column, rationale
	size_t depth = 0;	// hypothesis nesting; 0 when the file predates the fifth cell
};

struct theorem {
	std::string name;
	std::string result;
	std::vector<std::string> axioms;
	std::vector<proof_row> proof;
};

// one physical line of the theorem file, split into indent and content
struct line {
	size_t indent;
	std::string text;	// trailing CR/whitespace removed
};

std::vector<line> read_lines(std::istream& in)
{
	std::vector<line> ret;
	std::string raw;
	while (std::getline(in, raw)) {
		while (!raw.empty() && (' ' == raw.back() || '\t' == raw.back() || '\r' == raw.back())) raw.pop_back();
		const auto first = raw.find_first_not_of(' ');
		if (std::string::npos == first) continue;	// blank
		if ('#' == raw[first]) continue;	// comment line
		ret.push_back(line{first, raw.substr(first)});
	}
	return ret;
}

// a scalar as the writer emits it: plain, or double-quoted with \" and \\ escapes
std::string scalar(std::string_view src)
{
	if (2 <= src.size() && '"' == src.front() && '"' == src.back()) {
		std::string ret;
		for (size_t i = 1; i + 1 < src.size(); ++i) {
			if ('\\' == src[i] && i + 2 < src.size()) ++i;
			ret += src[i];
		}
		return ret;
	}
	return std::string(src);
}

// "- x" -> x; "-" -> ""; nullopt when the line is not a sequence item
std::optional<std::string> item(const line& src)
{
	if (src.text.empty() || '-' != src.text[0]) return std::nullopt;
	if (1 == src.text.size()) return std::string();
	if (' ' != src.text[1]) return std::nullopt;
	const auto first = src.text.find_first_not_of(' ', 1);
	if (std::string::npos == first) return std::string();
	return scalar(std::string_view(src.text).substr(first));
}

// "key: value" -> (key, value); value may be empty (a nested block follows)
std::optional<std::pair<std::string, std::string> > key_value(const line& src)
{
	const auto colon = src.text.find(':');
	if (std::string::npos == colon || 0 == colon) return std::nullopt;
	if (colon + 1 < src.text.size() && ' ' != src.text[colon + 1]) return std::nullopt;
	const auto key = src.text.substr(0, colon);
	if (std::string::npos != key.find(' ')) return std::nullopt;	// "a b: c" is a scalar, not a key
	const auto first = src.text.find_first_not_of(' ', colon + 1);
	return std::pair(key, std::string::npos == first ? std::string() : scalar(std::string_view(src.text).substr(first)));
}

// the content is entity-encoded already and the docs write a bare & for conjunction,
// so only the two characters that would start a tag are escaped
std::string escape(std::string_view src)
{
	std::string ret;
	ret.reserve(src.size());
	for (char c : src) {
		switch (c) {
		case '<': ret += "&lt;"; break;
		case '>': ret += "&gt;"; break;
		default: ret += c;
		}
	}
	return ret;
}

// false with err set: not parseable, or not shaped like a theorem file
bool load(const std::filesystem::path& path, theorem& dest, std::string& err)
{
	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		err = "cannot open";
		return false;
	}
	const auto lines = read_lines(ifs);
	size_t i = 0;

	// the root: a single "theorem:" key
	if (lines.empty()) {
		err = "empty file";
		return false;
	}
	{
		auto kv = key_value(lines[0]);
		if (!kv || "theorem" != kv->first || 0 != lines[0].indent) {
			err = "no theorem: key";
			return false;
		}
		i = 1;
	}
	if (lines.size() <= i) {
		err = "no proof: sequence";
		return false;
	}
	const size_t key_indent = lines[i].indent;
	bool have_proof = false;

	while (i < lines.size()) {
		if (lines[i].indent != key_indent) {
			err = "line " + std::to_string(i) + ": unexpected indentation";
			return false;
		}
		auto kv = key_value(lines[i]);
		if (!kv) {
			err = "line " + std::to_string(i) + ": expected a key";
			return false;
		}
		++i;
		if ("name" == kv->first) dest.name = kv->second;
		else if ("result" == kv->first) dest.result = kv->second;
		else if ("axioms" == kv->first) {
			while (i < lines.size() && key_indent < lines[i].indent) {
				auto x = item(lines[i]);
				if (!x) {
					err = "line " + std::to_string(i) + ": expected an axiom item";
					return false;
				}
				dest.axioms.push_back(std::move(*x));
				++i;
			}
		} else if ("proof" == kv->first) {
			have_proof = true;
			// each row: a bare "-" line, then four cells deeper than it
			while (i < lines.size() && key_indent < lines[i].indent) {
				const size_t row_indent = lines[i].indent;
				auto head = item(lines[i]);
				if (!head || !head->empty()) {
					err = "line " + std::to_string(i) + ": expected a proof row";
					return false;
				}
				++i;
				proof_row row;
				size_t n = 0;
				while (i < lines.size() && row_indent < lines[i].indent) {
					auto x = item(lines[i]);
					if (!x) {
						err = "line " + std::to_string(i) + ": expected a cell";
						return false;
					}
					if (4 > n) row.cells[n] = std::move(*x);
					else if (4 == n) {
						// the depth cell: a non-negative integer
						if (x->empty() || std::string::npos != x->find_first_not_of("0123456789")) {
							err = "proof row " + std::to_string(dest.proof.size()) + ": depth cell is not a non-negative integer";
							return false;
						}
						row.depth = std::stoull(*x);
					} else {
						err = "proof row " + std::to_string(dest.proof.size()) + " has more than five cells";
						return false;
					}
					++n;
					++i;
				}
				if (4 > n) {
					err = "proof row " + std::to_string(dest.proof.size()) + " has fewer than four cells";
					return false;
				}
				dest.proof.push_back(std::move(row));
			}
		} else {
			// an unknown key: skip its block
			while (i < lines.size() && key_indent < lines[i].indent) ++i;
		}
	}
	if (!have_proof) {
		err = "no proof: sequence";
		return false;
	}
	return true;
}

// the docs' page framing, copied rather than linked so the output stands alone
void emit_head(std::ostream& out, const std::string& title)
{
	out << "<!DOCTYPE html>\n<html>\n<head>\n"
		<< "\t<meta http-equiv=\"content-type\" content=\"text/html; charset=windows-1252\"/>\n"
		<< "\t<title>" << escape(title) << "</title>\n"
		<< "\t<style type=\"text/css\">\n"
		<< "\t\t@page { margin: 0.79in }\n"
		<< "\t\tp { margin-bottom: 0.1in; line-height: 120% }\n"
		<< "\t\ta:link { so-language: zxx }\n"
		<< "\t\t* {\n"
		<< "            font-family: \"Liberation Serif\", serif\n"
		<< "        }\n"
		<< "\t</style>\n"
		<< "</head>\n<body>\n";
}

void emit(std::ostream& out, const theorem& src, const options& opt)
{
	// the Fitch modification adds one leading column per hypothesis nesting level, each
	// drawn as a cell with a left border (top / bottom borders on a scope's first / last
	// row); the table-spanning rows widen to match
	size_t scopes = 0;
	if (opt.fitch) for (const auto& row : src.proof) scopes = std::max(scopes, row.depth);
	const size_t columns = 4 + scopes;

	if (opt.axioms && !src.axioms.empty()) {
		out << "<table align=\"" << opt.align << "\">\n"
			<< "\t<tr><th>Axioms used</th></tr>\n";
		for (const auto& x : src.axioms) out << "\t<tr><td align=\"left\">" << escape(x) << "</td></tr>\n";
		out << "</table>\n";
	}

	// cellspacing="0" so the scope borders join, as the docs' Fitch tables have it
	out << "<table align=\"" << opt.align << "\"" << (scopes ? " cellspacing=\"0\"" : "") << ">\n\t<tr>";
	if (scopes) out << "<th colspan=\"" << (scopes + 1) << "\"></th>";
	else out << "<th></th>";
	out << "<th>Inference rule</th><th>Proposition</th><th>Rationale</th></tr>\n";
	for (size_t i = 0; i < src.proof.size(); ++i) {
		const auto& row = src.proof[i];
		out << "\t<tr>";
		for (size_t k = 1; k <= scopes; ++k) {
			if (row.depth < k) {
				out << "<td></td>";
				continue;
			}
			const bool opens = 0 == i || src.proof[i - 1].depth < k;
			const bool closes = i + 1 == src.proof.size() || src.proof[i + 1].depth < k;
			out << "<td style=\"border-left: 1px black solid";
			if (opens) out << "; border-top: 1px black solid";
			if (closes) out << "; border-bottom: 1px black solid";
			out << "\"></td>";
		}
		for (const auto& c : row.cells) out << "<td>" << escape(c) << "</td>";
		out << "</tr>\n";
	}
	// the theorem name, then the theorem itself: the theorem is always the last row
	if (opt.name && !src.name.empty()) {
		out << "\t<tr><th align=\"center\" colspan=\"" << columns << "\">" << escape(src.name) << "</th></tr>\n";
	}
	out << "\t<tr><th align=\"center\" colspan=\"" << columns << "\">" << escape(src.result) << "</th></tr>\n"
		<< "</table>\n";
}

int usage(std::ostream& out, int ret)
{
	out << "usage: TheoremTable [options] <file.theorem.yaml>...\n"
		<< "  -o <file>      write here instead of stdout\n"
		<< "  --page         wrap the tables in a complete page\n"
		<< "  --align <a>    center (default) | left | right\n"
		<< "  --no-axioms    omit the axioms-used table\n"
		<< "  --no-name      omit the theorem-name row\n"
		<< "  --fitch        draw hypothesis scopes (one leading column per nesting level)\n";
	return ret;
}

} // end anonymous namespace

int main(int argc, char* argv[])
{
	options opt;
	std::vector<std::filesystem::path> inputs;
	for (int i = 1; i < argc; ++i) {
		const std::string_view arg(argv[i]);
		if ("-o" == arg) {
			if (argc <= ++i) return usage(std::cerr, 1);
			opt.out = argv[i];
		} else if ("--page" == arg) opt.page = true;
		else if ("--align" == arg) {
			if (argc <= ++i) return usage(std::cerr, 1);
			opt.align = argv[i];
			if ("center" != opt.align && "left" != opt.align && "right" != opt.align) return usage(std::cerr, 1);
		} else if ("--no-axioms" == arg) opt.axioms = false;
		else if ("--no-name" == arg) opt.name = false;
		else if ("--fitch" == arg) opt.fitch = true;
		else if ("--help" == arg || "-h" == arg) return usage(std::cout, 0);
		else if (!arg.empty() && '-' == arg[0]) return usage(std::cerr, 1);
		else inputs.emplace_back(argv[i]);
	}
	if (inputs.empty()) return usage(std::cerr, 1);

	// a bad file is reported and the rest are still processed
	std::vector<theorem> theorems;
	int ret = 0;
	for (const auto& path : inputs) {
		theorem t;
		std::string err;
		if (load(path, t, err)) theorems.push_back(std::move(t));
		else {
			std::cerr << path.string() << ": " << err << "\n";
			ret = 1;
		}
	}

	std::ofstream file;
	if (!opt.out.empty()) {
		file.open(opt.out);
		if (!file.is_open()) {
			std::cerr << opt.out << ": cannot write\n";
			return 1;
		}
	}
	std::ostream& out = opt.out.empty() ? std::cout : file;

	if (opt.page) emit_head(out, theorems.empty() ? std::string("TheoremTable") : theorems.front().name);
	for (const auto& t : theorems) emit(out, t, opt);
	if (opt.page) out << "</body>\n</html>\n";
	return ret;
}
