# GentzenW defined notations ("modus ponens for _") — plan

Status: **in progress** (plan agreed 2026-09-04).  This is the true form of the dropped
work item #1 of the cut-rule plan: the name "modus ponens for &#9500;" is not
configuration metadata but a theorem of the notation system, obtained by instantiating a
conditional definition.

Progress log:
* 2026-09-04: `is_unary_syntactical_entailment` (top-layer recognizer, contract-template
  over the anchor_symbol_parse postcondition, exercised by the enumeration self-test)
  and `is_definition_node` + `is_binary_node` (definition-layer recognizer; the relaxed
  binary shape admits multi-token sides such as the definiendum phrase) checkpointed.
* 2026-09-04: W0 premise revised — see W0 below; `_` must never be globally registered
  as a symbol.
* 2026-09-05: two-stage lex_node diagnostics landed (record_error/record_warning,
  report_*/discard_*), giving the enforcement sketch's deferral option real
  infrastructure; remove_grouping_parentheses gained the contents-inspecting callback.
* 2026-09-08: **W0 spike executed and passed** (Kenneth, via local test_lines): the full
  conditional axiom parses, `_` does not error, and the machinery returns a
  simultaneous substitution for the line.  undefined_SVO made a compiler-enforced
  singleton — defined_SVO should copy that construction from birth.

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
  current grammar produces.  Remaining question for the spike: how the multi-word
  phrase "modus ponens for" tokenizes before any registration.
  **Settled by design, not by the spike**: `P _ Q` must NOT parse via global symbol
  registration.  A symbol-placeholder-syntax symbol exists specifically for axiom
  schemata *about other kinds of symbols*; declaring `_` as a binary infix (or unary
  prefix/postfix, or constant) symbol would add `_` to the formal grammar being
  described axiomatically.  Instead, **the hypothesis about `_` controls how the
  conclusion is parsed**: the conditional axiom's hypothesis (`_ is a binary infix
  symbol`) licenses a hypothesis-scoped parse of the conclusion span, in which `_`
  parses as a binary infix operator.  W2's recognizer therefore cannot expect a fully
  parsed conclusion from the global grammar; it parses the hypothesis first, extracts
  the grammatical category the hypothesis asserts about `_`, and applies it as a scoped
  extension when parsing the conclusion.
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

## Enforcement scoping for the conditional axiom (sketch, 2026-09-05)

The argument_enforcer error reports are unconditional and fire at **token stage**:
reject_left_edge / reject_right_edge / reject_adjacent (and the boolean terminal
forms), plus the is_wff path via check_for_gentzen_wellformed, are all registered in
TokenGrammar — they run long before GentzenGrammar and therefore before any `:=` node
exists.  Under the hypothesis-scoped parse, `_` at the right edge of the definiendum
"modus ponens for _" would hard-error (binary infix, nothing to its right) even though
the definiendum is technical English where `_` is a phrase slot, not an operator.

**The discriminator is not "LHS of :=".**  The reversed classical-logic definition
`A & B &#9500; C := A, B &#9500; C` wants full Gentzen parsing and enforcement on its
LHS.  What separates the cases is token content: a span made entirely of grammatically
known material (registered symbols, placeholder variables) is a Gentzen formula —
parse and enforce; a span containing grammatically unknown (English) words is a
definiendum introducing notation — phrase-parse, no argument enforcement on slots.
This is the same test that classifies the axiom as notation-introducing, doing double
duty.

Three candidate change points:

* **(a) Deferral mode on the enforcement path**: violations mark nodes "pending"
  instead of error_report + formal::Error; the `:=` recognizer later clears
  (definiendum) or promotes (formula).  Precedent: is_wff's wff_t warning-vs-error
  bool.  Cost: touches the global enforcer; pending-state leaks into everything that
  tests formal::Error.  Fallback option.
* **(b) Scoping built into the W2 overlay**: `_`'s enforcement exists only where its
  scoped registration does; the overlay applies to the definiens sub-span, not the
  definiendum.  Requires push/pop (RAII) scoping of argument_enforcer's singleton
  reservation state.
* **(c) Keep the conclusion span out of the token-stage grammar — recommended.**  The
  axiom text already parenthesizes the whole conclusion, and TokenGrammar registers
  the balanced_atomic_handlers *before* the reject_* enforcers, so parenthesized
  content is captured into a balanced node's infix before top-level edge rejection
  sees it.  The conditional-definition recognizer then owns everything inside: it
  sub-parses the definiens under the `_` overlay *with* enforcement and the
  definiendum as a phrase *without*, using the unknown-token discriminator.  The
  global enforcer stays untouched and unconditional — correct for ordinary Gentzen
  text.  Deferred sub-span reparse has in-repo precedent (HTMLtag infix reparse;
  note its "works for GentzenGrammar.apply, not TokenGrammar.apply" caveat, which is
  the one constraint to design around).

Added W0 spike obligations: confirm enforcement genuinely does not reach inside a
balanced capture at token stage (registration order says no; parse_inside_balanced
drives inner spans at GentzenGrammar stage and the interaction wants an empirical
check), and observe how the unregistered phrase words tokenize inside the capture.
If the shielding assumption is falsified, fall back to (a).

**Spike outcome (2026-09-08): passed.**  The full conditional axiom line parses via
test_lines with no error from `_`, and a simultaneous substitution comes back for it.
The shielding assumption held; option (c) remains the plan of record, with the
two-stage diagnostics (record/report/discard) available if a corner case surfaces
during W1/W2 buildout.

## Resolved decisions

* **No hard-coded notation ids.**  The axiom to be defined as "modus ponens for
  &#9500;" currently carries notation id 37 — a load-order artifact (hard-coded axioms
  plus YAML sequence position).  The definition machinery resolves the definiens
  *notationally* (the notation_ids::allocate / axioms::disambiguate precedent), never
  by baked-in id.
* **Registry class**: `defined_SVO`, paralleling undefined_SVO (name acknowledged as
  approximate for what is stored); compiler-enforced singleton from birth, per the
  2026-09-08 undefined_SVO fix.
* **Definition granularity**: one "modus ponens for _" instance per binary infix
  symbol, rather than a single parameterized "modus ponens for" taking a
  binary-infix-symbol argument.  Less testing to build out; also the natural output
  shape of the already-resolved config-time saturation, which materializes per-symbol
  instances anyway.
* **Definitions are bidirectional.**  `A := B` formally justifies both C[A/B] and
  C[B/A]; this matters once the uniform substitutions the C++ implementation actually
  performs are themselves axiomatized.  For "modus ponens for &#9500;" neither
  direction is simplifying — rationale rewriting is a *policy* of favoring technical
  English in rationales over Gentzen-format notation, not a simplification step.  The
  defined_SVO registry API should expose both directions from the start even though
  rationale rewriting only consumes the English-ward one.
* **Post-saturation framing**: the instantiated "modus ponens for &#9500;" can be
  viewed as a *constant* defined notation on the left-hand side whose right-hand side
  is the fully-parsed "(P &#9500; Q), P &#9500; Q".  This collapses W4's lookup to a
  notational pair in defined_SVO (constant-name <-> definiens notation), sidestepping
  slot/parameter machinery in the rendering path entirely.
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
