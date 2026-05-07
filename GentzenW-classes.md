# GentzenW class hierarchy

Scope: classes reachable from the GentzenW translation unit
([GentzenW.cpp](GentzenW.cpp), [HTMLtag.cpp](HTMLtag.cpp), and the
`z_kuroda` library). Classes used only by Franci/Franci_console/LogicC
are out of scope.

```mermaid
classDiagram
    direction LR

    namespace formal {
        class parsed {
            <<abstract>>
            +clone()*
            +to_s()*
            +origin()*
            +get_placeholder_variables()*
            +is_not_legal_axiom()*
            +before_add_axiom_handler()*
        }
        class lex_node
        class word
        class src_location
        class placeholder_match
    }
    namespace gentzen {
        class domain
        class statement_t
        class fact_database {
            <<abstract>>
            +size()*
            +parent()*
            +offset(notation_id)*
            +rationale(n)*
        }
        class axioms
        class lemmas
        class syntactical_entailment_2ary
        class syntactical_entailment_2ary_infer
        class syntactical_entailment_introduction_start
        class syntactical_entailment_introduction
        class phrase_postfix
    }
    namespace zaimoni {
        class observed_SRC {
            <<abstract>>
            +watched_by(observer)*
        }
    }
    class HTMLtag

    %% parsed-tree hierarchy
    parsed <|-- HTMLtag
    parsed <|-- domain
    parsed <|-- syntactical_entailment_2ary
    parsed <|-- phrase_postfix

    %% proof database hierarchy
    fact_database <|-- axioms
    fact_database <|-- lemmas
    fact_database <|-- syntactical_entailment_2ary_infer
    fact_database <|-- syntactical_entailment_introduction_start
    fact_database <|-- syntactical_entailment_introduction

    %% multiple-inheritance: axioms/lemmas are also observed
    observed_SRC <|-- axioms : SRC = statement_t
    observed_SRC <|-- lemmas : SRC = statement_t

    %% statement_t is a variant over the two parse-tree carriers
    statement_t ..> parsed : variant alt
    statement_t ..> lex_node : variant alt

    %% lex_node aggregates words, parsed nodes, and sub-lex_nodes via COW
    lex_node *-- word : COW
    lex_node *-- parsed : COW
    lex_node o-- lex_node : COW (recursive)

    %% positional metadata
    word *-- src_location
    HTMLtag *-- src_location

    %% placeholder substitution result
    placeholder_match o-- lex_node
```

## Standalone classes (no inheritance, included for completeness)

| Class | File | Role |
|-------|------|------|
| `formal::is_wff` | [GentzenW.cpp:330](GentzenW.cpp:330) | Well-formed-formula predicate |
| `gentzen::argument_enforcer` | [GentzenW.cpp:451](GentzenW.cpp:451) | Argument-shape validation (final) |
| `gentzen::symbol_catalog` | [GentzenW.cpp:859](GentzenW.cpp:859) | Symbol registry / global parse |
| `gentzen::notation_ids` | [GentzenW.cpp:1471](GentzenW.cpp:1471) | Notation ID lookup (final) |
| `gentzen::tag_placeholder_syntax` | [GentzenW.cpp:1561](GentzenW.cpp:1561) | Tags placeholder-variable syntax |
| `gentzen::collect_placeholder_handles` | [GentzenW.cpp:1595](GentzenW.cpp:1595) | Walks parse trees collecting placeholders |
| `undefined_SVO` | [GentzenW.cpp:2899](GentzenW.cpp:2899) | Subject-Verb-Object placeholder |
| `error_counter<T>` | [errcount.hpp:9](errcount.hpp:9) | Threshold-based error counting |
| `kuroda::parser<T>` | Zaimoni.STL/LexParse | Kuroda-normal-form parser (template) |

## Out of scope (utility infrastructure, not part of GentzenW's design)

`zaimoni::COW`, `zaimoni::cache` / `I_erase`, the `auto*ptr` family, and
`zaimoni::stack` — these are Zaimoni.STL containers/smart-pointers that
GentzenW *uses* but that aren't part of its class hierarchy. If a
separate Zaimoni.STL infrastructure diagram becomes useful later, it
can live in its own file.
