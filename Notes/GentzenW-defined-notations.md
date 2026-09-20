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
* 2026-09-19: step 3 landed (apply_uniform_substitution_deep applies [&#9500; / _]
  across all three occurrences: hypothesis phrase subject, definiendum token, and the
  definiens -- which is itself a syntactical_entailment_2ary parsed object, reached via
  fork A(a) parsed-arm dispatch in substitute_parsed_anchors calling the 2ary's
  rename-vector apply).  Fork B reparse (reparse_loose_infix) folds the operator-position
  result (P _ Q) -> (P &#9500; Q).  Verified end-to-end:
  `&#9500; is a binary infix symbol &#9500; (modus ponens for &#9500; := (P &#9500; Q), P
  &#9500; Q)`.  A(b) unification of the parsed-arm applier into one virtual is deferred
  to stage 6 (before the defined_SVO::get() wire-in).
* 2026-09-19: destination-kind relaxation landed (apply_uniform_substitution accepts
  placeholder, symbol, or statement destinations; sources stay placeholder-family;
  explosion guard at call sites) and phrase_postfix::get_uniform_substitution landed
  (step-2 matcher; verified [&#9500; / _] against the configured infix declaration).
  Also: phrase_postfix subjects now visible to placeholder collection (the census
  reports _ x 3), and to_s regenerates precedence-mandated grouping parentheses via
  the lex_node policy hook.
* 2026-09-08: **W0 spike executed and passed** (Kenneth, via local test_lines): the full
  conditional axiom parses, `_` does not error, and the machinery returns a
  simultaneous substitution for the line.  undefined_SVO made a compiler-enforced
  singleton — defined_SVO should copy that construction from birth.

Target axiom (cf. the schema tables in docs/SubatomicPhysicsOfMath_LogicV2.html; not yet
in cfg/core.yaml):

    _ is a binary infix symbol &#9500; (modus ponens for _ := (P _ Q), P &#9500; Q)

(Target revised 2026-09-19: := is higher precedence than &#9500;, so no grouping
parentheses are needed to the right of := for a correct parse.)

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

**Two-stage reporting flush contract (2026-09-17)**: the top-level parsing loops that
early-terminate iterations when the error count changes must trigger the second stage
of reporting (report_error/report_warning) before testing, so the counts they test are
correct.  Lower-level short-circuit loop tests are not expected to flush.

**remove_grouping_parentheses return encoding (2026-09-17)**: returns ptrdiff_t — the
layer count c on normal termination, -(c+1) on termination by the ok predicate, so a
negative value signals a predicate stop; the *caller* decides whether that is an error.
Motivating use: stripping must not increase an entailment's hypothesis count —
"(A) &#9500; B" strips to "A &#9500; B", "(A,B) &#9500; C" is an error at the
entailment-slot call site, while the ordered pair (1,2) is fine elsewhere.  Callers
needing more detail than the sign use a stateful function object; plain function
pointers stay acceptable as predicates.  The null-inside-parens path throws
std::logic_error in all build modes (normalized 2026-09-18 to the C++-side
invariant-violation doctrine; it previously error_report-ed in release).

**Spike outcome (2026-09-08): passed.**  The full conditional axiom line parses via
test_lines with no error from `_`, and a simultaneous substitution comes back for it.
The shielding assumption held; option (c) remains the plan of record, with the
two-stage diagnostics (record/report/discard) available if a corner case surfaces
during W1/W2 buildout.

## Instantiation mechanics (specified 2026-09-19; verification target for the next push)

The chain, when `_ is a binary infix symbol &#9500; (modus ponens for _ := (P _ Q), P
&#9500; Q)` is declared as an axiom:

1. The hypothesis parses through undefined_SVO::phrase_postfix with `_code ==
   hard_code::infix_symbol`; its subject `_` tests true for
   is_symbol_placeholder_syntax_symbol.
2. An axiom that parses the same way -- `&#9500; is a binary infix symbol` --
   authorizes the uniform substitution [&#9500; / _].  This requires a
   **phrase_postfix-level matcher** (new): same hard_code, one subject a
   symbol-placeholder binding to the other's concrete symbol.
3. The substitution replaces **all three** occurrences of `_` (hypothesis,
   definiendum "modus ponens for _", definiens "(P _ Q)"), yielding
   `&#9500; is a binary infix symbol &#9500; (modus ponens for &#9500; := (P &#9500;
   Q), P &#9500; Q)`.
4. Reparsing "modus ponens for &#9500;" is *expected* to error (a binary infix symbol
   in English-phrase position), and that error is **cancelled by being on the
   left-hand side of :=**: the sub-reparse records the enforcement error (two-stage
   diagnostics) and the := recognizer discards it on classifying the span as a
   definiendum -- or reports it when the LHS is a genuine formula (`A & B &#9500; C :=
   ...`).  This refines the enforcement sketch: balanced capture shields the
   conclusion from token-stage enforcement (option c), and the recognizer's own
   sub-reparse uses the deferral machinery (option a).
5. The modus ponens axiom identifies the simultaneous substitution deriving
   `modus ponens for &#9500; := (P &#9500; Q), P &#9500; Q`: matching hypothesis
   pattern (P &#9500; Q) against the instantiated conditional axiom binds P to the
   (configured) hypothesis axiom and Q to the definition, and detaches.

### Step 4 infrastructure — LANDED 2026-09-19

The three capabilities are in: gentzen::report_diagnostics and
gentzen::discard_diagnostics (self-recursing recursive_rewrite ops, guarded by
has_pending_diagnostic, which is also the termination condition -- op clears the node's
message before the self-revisit so the test then fails), and last_error() folded into
the syntactical_entailment_2ary::construct and tag_syntactical_entailment::recognize
gates.  Self-test verifies the cycle: record -> pending; discard -> cancelled, no
error-count change; report -> flushed, count +1, formal::Error set.  No corpus change
(record+report are atomic during normal parse, so last_error() is empty at the gates).

**Remaining for the end-to-end cancelled reparse (findings, 2026-09-19)**: the walks and
gates are ready, but two grammar-plumbing facts block the definiendum error from arising
and being cancellable, both discovered by probing a whole-result reparse of the step-3
instantiation (errors +0, structure unchanged):

1. **The reparse must reach the definiendum -- interim LANDED 2026-09-19.**
   apply_uniform_substitution_deep now tracks (by pointer) each lex-side replacement
   whose *source* is_symbol_placeholder_variable (symbol-placeholder-syntax symbol with
   used_as_placeholder; placeholder-to-placeholder replacements leave the parse unchanged
   and are not tracked) and, after the parsed-anchor pass, reparses every container
   holding one, bottom-up (reparse_substituted_containers), with the default reparse.
   The := node's 4-entry prefix [modus ponens for _] therefore reparses as
   [modus ponens for &#9500;] and self_test_phrase_matcher's step-3 line now reads
   "errors +1" with "non-postfix symbol has no argument to right: &#9500;" -- the
   broken intermediate stage, reported unconditionally until W1 cancels it.
   Two details found on the way: (a) the replacement clone must `forget` the
   destination's symbol-classification bits, because &#9500; in "&#9500; is a binary
   infix symbol" was classified anchor_is_not_symbol *before* that axiom put it in the
   catalog, and anchor_symbol_parse trusts the cached bit; (b) the error's reported
   location is the clone's, i.e. the declaring axiom's (cfg/core.yaml:axioms(2,1)),
   not the definiendum's -- W1 can decide whether the clone should take the source
   occurrence's origin.
2. **Routing (revised 2026-09-19, self_test_reparse_routing).**  The reparse family now
   takes `template<bool with_token_grammar = false>`, a pure routing switch that prefixes
   the TokenGrammar stage (apply + complete_parse, bail on new errors) before the
   GentzenGrammar stage.  The caller chooses it from what is being substituted *from*:
   `is_symbol_placeholder_variable(source)` = symbol-placeholder-syntax symbol with
   used_as_placeholder set (the same predicate that selects containers for reparse in
   item 1; the interim item-1 walk uses the default routing unconditionally).  The earlier premise that the right-edge error lives only in
   TokenGrammar was wrong: reparsing the isolated run `[modus ponens for &#9500;]` yields
   "non-postfix symbol has no argument to right: &#9500;" (+1) under *both* routings,
   from GentzenGrammar's symbol_catalog.  The TokenGrammar stage is silent on it --
   argument_enforcer's reserve_* tables and syntax_check's handlers are unpopulated (no
   callers anywhere), so `<true>` is not distinguishable from `<false>` by this test yet.
   What actually gates the error is *isolation*: in the whole step-3 result, &#9500; is
   followed by `:=`, so no right-edge condition exists until W1 splits the definiendum
   into its own run.  `<true>` stays scoped to loose leaf runs (the "works for
   GentzenGrammar.apply, not TokenGrammar.apply" caveat near check_for_gentzen_wellformed).
3. **Then the deferral -- LANDED 2026-09-19 for symbol_catalog.**  The three
   find_global_parse_target enforcement errors (no associativity/transitivity, no argument
   to left, no argument to right) now record_error instead of error_report, so they arise
   *pending* and discard_diagnostics can cancel them.  (argument_enforcer's reject_* still
   report atomically; its tables are unpopulated, so it is inert either way.)  Output is
   byte-identical to immediate reporting on the HTML differential and on a six-line
   malformed-statement probe (edge, nested-in-parens, inside :=, inside a 2ary arm).
   The counter-changes that parity required:
   * **The walks' test must be deep.**  recursive_rewrite is one level and applies op only
     where the test passes, so the original shallow has_pending_diagnostic never descended
     through a clean parent; the step-3 error sits at depth 2 (root -> := -> &#9500;).
     has_pending_diagnostic is now "this node or any descendant", traversing exactly
     recursive_rewrite's edges (the termination argument depends on that).
   * **Flush what you are about to test, in the tree that holds it.**  The pending error
     lives in the *instantiated* result, not the source statement.
   * **Flush after GentzenGrammar().apply too**, not only after complete_parse: balanced
     groups parse (and defer) during apply, and all four parse loops test the error count
     there.  Without it the statement's size/echo trace printed before its own error.
   * **Recognizer gates are deep** (has_pending_error): building a formal::parsed over a
     subtree with a pending error puts the message behind a membrane the walks cannot
     cross.  For the same reason syntactical_entailment_2ary's rename-apply flushes its
     reparsed arms before constructing (an arm is never a definiendum).
   * **Record-time location** (LexNode): origin() is the leftmost token, which moves once
     the parse attaches a prefix, so a deferred report cited the left argument instead of
     the offending symbol.  record_* now captures origin(); report_* cites it.
   Standing hazard: any new code that wraps lex_nodes in a formal::parsed, or tests
   Errors.count(), between a reparse and a flush inherits these obligations.

Item 3 belongs with the := definition recognizer (W1), which is not yet ready to be
attempted: it takes over the definiendum reparse from the interim item-1 walk, routes it
(per the substitution source) so enforcement runs in record mode, and after classifying
the LHS as a definiendum calls discard_diagnostics on it (or report_diagnostics + error-count test
when the LHS is a genuine formula such as A & B &#9500; C := ...).

### Step 4 prerequisites (infrastructure, specified 2026-09-19)

The cancelled reparse of "modus ponens for &#9500;" needs three lex_node-level
capabilities on top of the two-stage diagnostics already landed (record_error /
record_warning / last_error / last_warning / discard_error / discard_warning /
report_error / report_warning, two fields _last_error / _last_warning):

1. **Recursive error reporting**: call report_error() and report_warning() on every
   lex_node in a subtree, immediately before the parse loop tests the error count for
   an increase.  This is the second stage of the flush contract already recorded for
   top-level error-count-tested loops; recorded pending messages must be flushed so
   the count is correct when tested.
2. **Recursive error cancellation**: clear the pending _last_error / _last_warning
   fields (discard_error / discard_warning) on every lex_node controlled by the LHS of
   :=, so an expected error on a definiendum such as "modus ponens for &#9500;" is
   cancelled rather than reported.
3. **Pending-message parse stop**: recognizers currently gate on `formal::Error &
   code()`; they may also need to stop when an error message is *pending* (last_error()
   non-empty) but formal::Error is not yet set -- the record-but-not-yet-reported state.

Traversal shape: the recursion must cover _prefix, _postfix, _infix, _fragments, and
the lex_node-based variants of _anchor and _post_anchor -- exactly what
lex_node::recursive_rewrite(test, op) visits.  recursive_rewrite applies test/op one
level (immediate members, the lex anchor/post_anchor, and self), but it is legal and
prior practice for the op hook to call recursive_rewrite itself, driving genuine
subtree recursion; the op must know its own test hook, which has not been a problem
before.  So recursive_rewrite is the intended vehicle for the report and cancel walks
(currently unexercised code getting its first use).

Design choices settled: the report/cancel walks are self-recursing recursive_rewrite
ops (not new dedicated methods); the pending-message stop reuses the existing
last_error() (a std::optional reference, truthy when a pending error is recorded) at
the recognizer gates, rather than a new has_pending_error() predicate.  Step 4 is thus
mechanical: a report-all op run before each error-count test, a discard-all op over
the LHS-of-:= subtree, and last_error() folded into the formal::Error gates.

**Stop-line for this push**: verify the chain through step 5's derivation.
Automatically wiring the derived definition into defined_SVO::get() is a later commit,
after the above checks out.

Implementation observations against current code:

* get_uniform_simultaneous_substitution already *produces* both substitution nodes --
  it places no placeholder requirement on candidate children, so [&#9500; / _] and
  [H, (def) \ P, Q] with symbol/statement destinations build today.  The single
  blocker is apply_uniform_substitution's destination check.  Work item: relax
  destination kinds (placeholder | symbol | statement; sources stay
  placeholder-family), keeping the combinatoric-explosion guard at the *call sites*
  (menu enumeration keeps the restriction; directed config-time saturation uses the
  relaxed path), as the enumerate_applicable_2ary assumptions comment anticipated.
* The phrase_postfix matcher of step 2 is the first parsed-arm unification consumer.

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
