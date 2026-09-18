# Deciding whether noexcept pessimizes the binary

(Recorded from design discussion, 2026-09-19.  Context: MSVC x64, /EHsc, CMake Ninja
builds; the project stance is that CPU is not a priority, so this is almost entirely a
binary-size question.)

A linker map file is one of the *worse* instruments for this question: on MSVC x64 the
cost of noexcept mostly does not land in the function bodies a map file itemizes.  The
practical toolkit, ordered from "predict without measuring" to "measure precisely":

## 1. Most cases are decidable by inspection, no tooling

The only situation where noexcept can pessimize is a function that *calls
potentially-throwing code*: the compiler must guarantee escape -> std::terminate, so
MSVC wraps the body in terminate scaffolding (an EH state, a funclet, unwind records).
A function whose body is transitively non-throwing -- leaf code, or callees that are
themselves noexcept/nothrow intrinsics -- costs nothing to annotate, and typically
*shrinks callers*, because they can drop landing pads and cleanup paths around calls
to it.  The lex_node thin-wrap constructors are in this free category (pointer stores
into a variant).  The cases worth measuring are noexcept on something that allocates
(cf. the MSVC debug _Container_proxy behavior: container constructors allocate under
_ITERATOR_DEBUG_LEVEL >= 1 while keeping their noexcept specification, so OOM there
terminates rather than throws).

## 2. Local question -- "did the compiler inject scaffolding?" -- diff disassembly, not the map

Compiler Explorer with the exact /std: and /EH flags, toggling the noexcept, answers it
quickly; for in-tree code, compile the one TU both ways and `dumpbin /DISASM` the
objects.  What to look for on MSVC x64: added `__ehhandler$`/`dtor$` funclets and
changed prologue EH state, not longer straight-line code.

## 3. Net question -- "did the binary grow?" -- compare section sizes, where map files fail

On x64 the enforcement machinery lives largely in .pdata/.xdata (unwind metadata) plus
funclets, and linker map files attribute unwind data poorly or not at all -- two maps
can diff clean while the binary grew.  The right tools: `dumpbin /HEADERS` for
.text/.pdata/.xdata sizes before and after the toggle (scriptable, works per-.obj), or
Microsoft's SizeBench, which attributes PE size to symbols *including* their unwind
info -- purpose-built for this question.  (ELF-side equivalents if ever needed:
bloaty, with .eh_frame/.gcc_except_table visibility.)

## 4. The sign can be negative, and per-function deltas mislead

noexcept on a hot callee can shrink every *caller* while growing the callee -- a
whole-image section comparison is the honest net measure; a single function's
map-delta is neither necessary nor sufficient in either direction.

## Project calibrations

* On x64, table-driven EH means the *runtime* cost of the scaffolding on the
  non-throwing path is essentially zero -- purely a binary-size question, close to a
  non-question here outside genuinely size-constrained targets.
* Debug builds already pay _ITERATOR_DEBUG_LEVEL costs that dwarf any noexcept effect:
  measure on Release or the measurement is noise.
* The enforcement is /EH-flag-sensitive: under plain /EHsc the compiler assumes
  extern "C" functions do not throw and elides some machinery; /EHr forces terminate
  checks everywhere and inflates exactly the scaffolding above.  Pin the /EH flag
  before measuring or the comparison crosses regimes.
