# GentzenW regression fixture (work item #7 of the cut-rule plan)

Status: **implemented 2026-11-02** as CTest `gentzenw_cut_rule` (tests/cut_rule.cmake,
driven by `cmake -P`): stages GentzenW.exe, its runtime DLLs, cfg/core.yaml and a copy of
the script under `<build>/ctest/cut_rule/`, runs the session with
`execute_process(INPUT_FILE tests/cut_rule_picks.txt)` (no shell, no BOM), and
byte-compares the theorem file against tests/cut_rule.theorem.yaml modulo line endings.
The second stage, `theorem_table_cut_rule`, renders that file to HTML and compares it
with tests/cut_rule.html (see GentzenW-theorem-table.md).  Run with `ctest --test-dir
<build> --output-on-failure`; Visual Studio's Test Explorer picks them up.  The
remaining anticipated reshaping (N-ary levels, "lemmas used", Fitch depth) will change
the expected files when it lands.  The design notes below are kept as the record of why
it is built this way.

## Purpose

Regression-test the menu-driven proof session end to end: proof script in, scripted
menu picks on stdin, theorem file out, compared against an expected fixture.  Must be
runnable from Visual Studio with minimal ceremony.

## Components

1. **Input proof script**: [tests/cut_rule.yaml](../tests/cut_rule.yaml) (exists).
2. **Stdin picks script**: one selection key per line.  Currently `0,0,0,0` — the
   cut-rule session offers exactly one menu entry per round (inference rounds 1-2,
   discharge rounds 3-4), so any regression that adds or removes an entry shifts the
   indices and fails loudly rather than silently proving something else.
3. **Expected theorem file**: the `theorem:` YAML (name, axioms used, result, proof
   table as array of arrays).  Generate from a known-good run once features settle.
4. **Optional expected stdout transcript**: the console output is now free of synthetic
   notation ids (menu entries print statements; the table preview prints rewritten row
   labels), so the full transcript is stable and comparable too.
5. **Runner**: see Mechanics.

## Mechanics and gotchas (all learned the hard way)

* **Stdin redirection**: use `cmd`-style `< picks.txt` with an ASCII (BOM-free) file.
  PowerShell pipelines can prepend a BOM to the first line, which the menu silently
  ignores as an unmapped key — the first pick is eaten and everything shifts.
* **cfg staging**: `cfg/core.yaml` must sit beside `GentzenW.exe`.  If missing, axiom
  loading throws before any output flushes: silent abort, exit code 3.  This is a known
  CMake gap, deliberately not automated — an automatic copy would overwrite the
  `test_lines` key in the Visual Studio debug build's staged copy.
* **Output location**: the theorem file is written to `<script>.theorem.yaml` next to
  the *input script*.  Run against a copy of the script in a scratch directory so the
  fixture run cannot dirty `tests/`.
* **Exit codes**: 0 on success (`Errors.count()` drives `EXIT_FAILURE`); the abandoned
  paths print explicit messages.
* **YAML comparison**: byte comparison modulo line endings (`cmake -E compare_files
  --ignore-eol`), which is exact because fkYAML serializes deterministically: it
  alphabetizes mapping keys, serializes the proof rows in block form (not the flow form
  sketched in planning), and writes empty cells as empty scalars.  Deserialize-and-compare
  was the original preference but **does not work**: the vendored fkYAML cannot
  deserialize its own output when a plain scalar contains "," (see
  GentzenW-theorem-table.md).  If the serializer ever changes, regenerate the expected
  file rather than loosening the comparison.

## Runner: CTest (ruled 2026-11-02)

Visual Studio wants CTest, so: `add_test` in CMakeLists.txt driving copy /
run-with-redirection / compare (note the existing "EXCLUDE_FROM_ALL means can't test"
comment there), plus a tiny compare helper since CTest has no YAML-aware diff.  The
TheoremTable tool (GentzenW-theorem-table.md) is the natural second stage of the same
test: theorem file -> HTML, byte-compared.  (A `.cmd` script was the rejected
alternative.)

## Known feature work expected to reshape the fixture

(Extend this list as decisions land; these are the anticipated ones.)

* Defined notations (see GentzenW-defined-notations.md): rationale strings become
  "modus ponens for &#9500;; (1) and (3)" etc., and the `axioms:` list gains the
  conditional-definition axiom once it is used.  **Landed 2026-11-02 (R6/R7).**
* Fitch modification (see GentzenW-theorem-table.md): the theorem writer emits the
  hypothesis nesting depth per row; max depth 1 for the cut rule.  **Landed 2026-11-02**:
  the fifth cell is in tests/cut_rule.theorem.yaml, and theorem_table_cut_rule also
  checks the `--fitch` rendering against tests/cut_rule_fitch.html.
* N-ary (>2 statements) `given:`/`hypothesis:` levels.
* A "lemmas used" key alongside `axioms:` once previously proven theorems can be
  applied as rules.
* Possible output-path override key in the proof script.
* Any change to the menu entry vocabulary or ordering invalidates the picks script.
