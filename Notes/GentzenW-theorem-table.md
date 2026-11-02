# TheoremTable: `*.theorem.yaml` -> HTML proof table (specification)

Status: **specified 2026-11-02, not implemented**.  Do not implement until Kenneth
green-lights it.

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
	<tr><th colspan="4">cut rule for &#9500;</th></tr>
</table>
```

* Header row as above; the theorem **name** is the closing `<th colspan="4">` row, which
  is how the docs caption their tables.
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
  --axioms         precede the table with "<p>Axioms used:</p><ul>...</ul>"
  --result         follow the table with "<p>Result: ...</p>"
```

Several inputs produce several tables in one output, in argument order.  `--axioms` and
`--result` default off: the docs state axioms in their own tables and the result is the
last row anyway; both exist for stand-alone pages.

Exit codes: 0; 1 for a file that does not parse, lacks `theorem:`/`proof:`, or has a
row that is not exactly four cells (reported with file and row index, then the next
file is still processed).  Null cells are empty strings.

## Fitch scoping -- deferred, with a decision

The docs' Fitch-style tables (LogicV2, "Gentzen format schema") draw hypothesis scopes
with a leading cell carrying `border-left` (and `border-top` / `border-bottom` on the
first / last row of the scope), plus labels like `(a)`.  The theorem file carries **no
scope information**, so the tool cannot draw them from the data.  Two ways to get there:

1. Infer from rationale text: "Hypothesis" opens a scope, "&#9500;-introduction, (n)"
   closes the one opened at row n.  Works for everything the session writes today, but
   couples the tool to rationale wording that the defined-notations work has already
   changed once.
2. Have the theorem writer emit scope depth per row (a fifth cell, or a parallel
   `scopes:` key).  Robust; a GentzenW change, so it belongs to a later GentzenW commit.

**Decision: v1 emits the flat Gentzen table only.**  Option 2 is the intended path to
`--fitch`; the fixture note's "anticipated reshaping" list should gain it.

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
  for HTML, with newline normalization.  This doubles as the first half of the deferred
  regression fixture.
