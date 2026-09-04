# GentzenW regression fixture (work item #7 of the cut-rule plan)

Status: **deferred by design**.  Several planned features will radically change the
fixture's appearance, so this note records the design well enough to implement after
they land.  Do not implement until Kenneth green-lights it.

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
* **YAML comparison**: prefer deserialize-and-compare over byte comparison.  fkYAML
  alphabetizes mapping keys, serializes the proof rows in block form (not the flow form
  sketched in planning), and writes empty cells as empty scalars, which round-trip as
  **null, not ""** — a comparator must treat null cells as empty strings.  Byte
  comparison also needs newline normalization (the repo is CRLF on checkout).

## Runner options (decision deferred)

* Simplest: a `.cmd` script doing copy / run-with-redirection / compare, invocable from
  a VS external tool or terminal.
* Better integrated: CTest `add_test` in CMakeLists.txt driving the same steps (note
  the existing "EXCLUDE_FROM_ALL means can't test" comment there), plus a tiny compare
  helper since CTest has no YAML-aware diff.

## Known feature work expected to reshape the fixture

(Extend this list as decisions land; these are the anticipated ones.)

* Defined notations (see GentzenW-defined-notations.md): rationale strings become
  "modus ponens for &#9500;; (1) and (3)" etc., and the `axioms:` list gains the
  conditional-definition axiom once it is used.
* N-ary (>2 statements) `given:`/`hypothesis:` levels.
* A "lemmas used" key alongside `axioms:` once previously proven theorems can be
  applied as rules.
* Possible output-path override key in the proof script.
* Any change to the menu entry vocabulary or ordering invalidates the picks script.
