# TheoremTable: `*.theorem.yaml` -> HTML proof table (specification)

Status: **v1 implemented 2026-11-02** (TheoremTable.cpp; CTest `theorem_table_cut_rule`
against tests/cut_rule.theorem.yaml and tests/cut_rule.html).  The Fitch modification
remains a later stage.

**Implementation deviation from this spec: no fkYAML.**  The vendored fkYAML's lexer
treats the "," inside a plain scalar such as `(P &#9500; Q) , P &#9500; Q` as a
flow-sequence separator and asserts (`m_flow_context_depth > 0`, node.hpp ~8111), so it
cannot read back what its own serializer wrote (core.yaml avoids this only because its
comma-bearing axioms are double-quoted).  TheoremTable therefore reads the writer's fixed
subset directly: a `theorem:` mapping with keys in any order, block sequences, plain or
double-quoted scalars, `- ` with nothing after for an empty cell.  About 80 lines, and
the tool now has no dependencies at all.  Consequence for the fixture note: its
"deserialize-and-compare" advice does not work with this fkYAML; byte comparison modulo
line endings is what the tests do, which is exact because the serializer is deterministic.

## Purpose

A command-line tool that takes the theorem file GentzenW's proof session writes
(`<script>.theorem.yaml`, cf. the theorem writer in run_proof_script) and emits the
corresponding HTML proof table in the format the docs folder already uses, so a proven
theorem can be pasted into (or generated for) the SubatomicPhysicsOfMath pages without
hand transcription.  It is also the natural consumer for the deferred regression fixture
(GentzenW-test-fixture.md): an expected HTML file is easier to eyeball than YAML.

## Input: the theorem file as actually written

fkYAML serializes it with alphabetized keys, block-form sequences, and empty cells as
empty scalars (which round-trip as **null, not ""**; see the fixture note).  Shape:

```yaml
theorem:
  axioms:
    - (P &#9500; Q) , P &#9500; Q
    - _ is a binary infix symbol &#9500; (modus ponens for _ := ( P _ Q ) , P &#9500; Q)
  name: cut rule for &#9500;
  proof:
    -
      - "1."
      - P &#9500; Q      # inference-rule column
      -                  # proposition column (empty)
      - Given            # rationale
    - ...
  result: (P &#9500; Q) , (Q &#9500; R) &#9500; (P &#9500; R)
```

Each proof row is exactly four cells: row label, inference-rule column, proposition
column, rationale.  Exactly one of the two middle cells is non-empty (the writer's
`is_entailment_statement` classifier decides which).  Row labels inside rationales are
already rewritten to `(n)` form by `rewrite_labels`; the tool does no label work.
Notation is stored with HTML entities verbatim (`&#9500;`), as the docs pages are.

## Output: the docs' Gentzen table

The established format (SubatomicPhysicsOfMath_ArithmeticTables.html and others):

```html
<table align="center">
	<tr><th></th><th>Inference rule</th><th>Proposition</th><th>Rationale</th></tr>
	<tr><td>1.</td><td>P &#9500; Q</td><td></td><td>Given</td></tr>
	<tr><td>2.</td><td>Q &#9500; R</td><td></td><td>Given</td></tr>
	<tr><td>3.</td><td></td><td>P</td><td>Hypothesis</td></tr>
	<tr><td>4.</td><td></td><td>Q</td><td>modus ponens for &#9500;; (1) and (3)</td></tr>
	<tr><td>5.</td><td></td><td>R</td><td>modus ponens for &#9500;; (2) and (4)</td></tr>
	<tr><td>6.</td><td>P &#9500; R</td><td></td><td>&#9500;-introduction, (3)</td></tr>
	<tr><th align="center" colspan="4">cut rule for &#9500;</th></tr>
	<tr><th align="center" colspan="4">(P &#9500; Q) , (Q &#9500; R) &#9500; (P &#9500; R)</th></tr>
</table>
```

preceded by the axioms used, as a single-column table in the docs' style:

```html
<table align="center">
	<tr><th>Axioms used</th></tr>
	<tr><td align="left">(P &#9500; Q) , P &#9500; Q</td></tr>
	<tr><td align="left">_ is a binary infix symbol &#9500; (modus ponens for _ := ( P _ Q ) , P &#9500; Q)</td></tr>
</table>
```

* Header row as above.  **Two table-spanning, centered rows close the table (ruled
  2026-11-02): the theorem name, then the theorem itself** (`result:`).  The theorem is
  always the last row; the name is by default the row before it.  Both are
  `<th align="center" colspan="N">` with N = 4 before the Fitch modification; each extra
  Fitch column adds one to N.  (The docs' existing tables caption with the name alone;
  the result row is the addition.)  The last proof row is technically a syntactical
  entailment introduction against both givens at once -- the 2-ary entailment that the
  result states -- but that is adequately connoted and is not made explicit.
* The axioms-used table goes **above** the main table.
* Cells are emitted verbatim except `<` and `>` (escaped).  `&` is **not** escaped: the
  content is entity-encoded already, and the docs write a bare `&` for conjunction
  (`A & B &#9500; A & B`), which browsers tolerate.  Escaping it would break every
  entity.  (If a notation ever needs a literal `&amp;`, revisit.)
* Tabs for indentation, one row per line, as the docs do.

## Command line

```
TheoremTable [options] <file.theorem.yaml>...
  -o <file>        write here instead of stdout
  --page           wrap the table(s) in a complete page: the docs' <head> (charset,
                   the shared <style> block, <title> = theorem name), nothing else
  --align <a>      center (default) | left | right -- the docs float tables both ways
  --no-axioms      omit the axioms-used table (for pasting into a page that already
                   states them)
  --no-name        omit the theorem-name row (the theorem row is never omitted)
```

Several inputs produce several table pairs in one output, in argument order.

Exit codes: 0; 1 for a file that does not parse, lacks `theorem:`/`proof:`, or has a
row that is not exactly four cells (reported with file and row index, then the next
file is still processed).  Null cells are empty strings.

## Fitch modification -- a later stage, with the design settled

The docs' Fitch-style tables (LogicV2, "Gentzen format schema") draw hypothesis scopes
with one extra leading column per nesting level, the cell carrying `border-left` (and
`border-top` / `border-bottom` on the first / last row of the scope).  Each extra column
adds one to the colspan of the name and theorem rows.  The theorem file carries **no
scope information** today, so the tool cannot draw scopes from the data; the theorem
writer must emit the nesting depth per row (a fifth cell, or a parallel `scopes:` key)
-- a GentzenW change, so a later GentzenW commit.  Inferring depth from rationale text
("Hypothesis" opens, "&#9500;-introduction, (n)" closes) was considered and rejected:
it couples the tool to wording the defined-notations work has already changed once.

For the revised cut_rule.theorem.yaml the maximum nesting depth is 1: row 6's
syntactical entailment introduction for P &#9500; R takes the depth 1 -> 0, which forces
0 -> 1 at row 3, the hypothesis P.  Rows 1-2 and 6 are at depth 0, rows 3-5 at depth 1.

**v1 emits the flat Gentzen table only** (colspan 4).

## Implementation notes

* New CMake target: `add_executable(TheoremTable TheoremTable.cpp)`, added to the
  `foreach(tgt ...)` property loop.  C++23 is already the project standard.  It needs
  only fkYAML (header-only, already vendored) and iostream/fstream; no Zaimoni.STL
  libraries, no GentzenW code -- the file format is the contract.  Roughly 150 lines.
* Read with `fkyaml::node::deserialize(ifs)`; cells via `get_value<std::string>()` with
  the null-cell guard.  Do not reuse GentzenW's `from_node` idiom for sequences of
  sequences; index explicitly.
* The `--page` style block is copied from the docs (`@page`, `p`, `a:link`, the
  Liberation Serif `*` rule) -- copy, do not link, so the output stands alone.
* Test: `tests/cut_rule.theorem.yaml` (generate once from a known-good run -- all the
  features that were going to reshape it have now landed except N-ary levels and
  "lemmas used") and `tests/cut_rule.html` as expected output; byte comparison is fine
  for HTML, with newline normalization.  Driven by **CTest** (ruled 2026-11-02: Visual
  Studio wants CTest; see GentzenW-test-fixture.md), so it doubles as the first half of
  the deferred regression fixture.
