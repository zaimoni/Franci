# GentzenW defined notations ("modus ponens for _") — plan

Status: **planned, not started** (plan agreed 2026-09-04).  This is the true form of the
dropped work item #1 of the cut-rule plan: the name "modus ponens for &#9500;" is not
configuration metadata but a theorem of the notation system, obtained by instantiating a
conditional definition.

Target axiom (cf. the schema tables in docs/SubatomicPhysicsOfMath_LogicV2.html; not yet
in cfg/core.yaml):

    _ is a binary infix symbol &#9500; (modus ponens for _ := ((P _ Q), P &#9500; Q))

Intent: define the technical-English **unary prefix notation** "modus ponens for", valid
only for binary infix symbols.  Instantiating _ := &#9500; discharges the hypothesis
(core.yaml configures "&#9500; is a binary infix symbol"), yielding
`modus ponens for &#9500; := ((P &#9500; Q), P &#9500; Q)` — whose definiens is
notationally the configured modus ponens axiom, closing the loop with current core.yaml.

Expected observable change: cut_rule.theorem.yaml rationales become
`modus ponens for &#9500;; (1) and (3)` and `modus ponens for &#9500;; (2) and (4)` in
place of the definiens notations.

## Structure of the axiom — three layers, each needing machinery

* **Top**: a 1-hypothesis entailment (binary infix &#9500;, no comma pair).  No current
  recognizer: syntactical_entailment_2ary requires the `A, B &#9500; C` shape.
* **Conclusion**: a definition — binary node anchored at `:=`, recognizable as
  `anchor_is_symbol && 0 == offset()` (highest-priority infix, catalog index 0),
  mirroring the 2ary's offset-1 test.
* **Definiendum**: the English phrase "modus ponens for" applied prefix-wise to the
  symbol-placeholder `_`.  Grammatical category (unary prefix) is *derived* from the
  definiendum's shape; the domain restriction (binary infix symbols only) is derived
  from the entailment hypothesis.  Neither is declared.

## Work items, in dependency order

* **W0 — parse spike (first, cheap)**: put the axiom line into a local `test_lines` in
  core.yaml (the test-driver hook with placeholder-dump probes exists) and see what the
  current grammar produces.  Known risks it exposes: whether `P _ Q` parses with a
  symbol-placeholder in *operator* position, and how the multi-word phrase
  "modus ponens for" tokenizes before any registration.  Design W1/W2 against the
  spike's findings.
* **W1 — definition recognition**: a new class in the syntactical_entailment_2ary style
  (can_construct / construct / recognize / global_parse) over `:=`-anchored binary
  nodes.  LHS analysis needs undefined_SVO-style phrase matching (token sequence with a
  distinguished slot).  Output: a definition record (definiendum phrase, slot shape,
  definiens statement).
* **W2 — conditional form + symbol substitution**: (a) recognize `H &#9500; (definition)`;
  (b) a new substitution primitive, symbol-placeholder -> symbol (`_` -> `&#9500;`) —
  deliberately excluded from the placeholder->placeholder primitive shipped with the
  cut-rule work.  Instantiation checks the hypothesis is an established axiom, then
  registers the instantiated definition.
* **W3 — technical-English tagging**: registry of defined notations plus (open decision)
  a reserved lex_node code bit set on *use* occurrences.  Mention/use rule copied from
  the placeholder base-case convention: the defining axiom's own definiendum occurrence
  does NOT tag; subsequent uses do.  Within the definiens, `_`, `P`, `Q` tag as the
  variables they already are; only the phrase is English.
* **W4 — rationale rendering**: the definiendum is a function of the *schema* and the
  symbol substitution, not the placeholder renaming — rows 4 and 5 of the cut-rule
  table both render "modus ponens for &#9500;", with renaming carried by the row labels.
  Caution: 2ary_infer::rationale renders from `rule` (the instantiated statement, e.g.
  `(Q &#9500; R), Q &#9500; R`), whose notation is NOT the definiens notation; the
  lookup must route through the schema (applicable_2ary::schema carries it; the node
  currently does not).
* **W5 — configuration and expectations**: add the axiom to core.yaml *after* the
  "&#9500; is a binary infix symbol" line it depends on (the axioms sequence is
  order-sensitive); update expected rationales; this feature is on the reshaping list
  in GentzenW-test-fixture.md.

## Resolved decisions

* **Phrase machinery**: parallel implementation first; evaluate what the shared base
  with undefined_SVO/phrase_postfix should be by comparing the two implementations once
  the new one is stable.
* **Theorem file `axioms:` key**: list axioms **used at least once**.  Rendering the
  defined name counts as use of the conditional-definition axiom, so it appears; the
  placeholder-syntax symbol axioms remain unlisted for the same reason (not used yet).
* **Instantiation timing**: config-time saturation is acceptable for non-recursive
  *syntax* axioms such as "is a binary infix symbol".  Other semantic kinds of axioms
  need review when they come up.

## Open decisions

1. Tagging representation: registry + reserved code bit (full placeholder parallel;
   bits 8/9 taken, 10 free) vs registry-only.  Lean: bit + registry.
2. Rendering hook: 2ary_infer carries the resolved display name from construction
   (degrading to the rule notation when no definition exists) vs session/ledger-side
   rewrite.  Lean: name on the node at construction.
3. Scope of the W2 recognizer: a general syntactical_entailment_1ary class now, or a
   narrower conditional-definition recognizer with generalization deferred.  Lean:
   narrow first.
