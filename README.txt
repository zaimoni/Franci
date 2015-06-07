Notes on coding idioms
=====
strlen of a string constant is a compile time constant that won't be 
automatically optimized away.  We want to optimize it, but catch 
misdefinitions at compile time.

The C++ standard mandates that a string constant's length is one less than 
its sizeof (null-termination).  So use a BOOST_STATIC_ASSERT to check that the 
hand-optimization is valid at compile time.

Instances:
LenName.cxx/UNNAMED_CLASS, etc.
=====
There appears to be no significant performance penalty for Franci's internal 
datatypes to have ACID assignment operators.  Keep.for now.
=====
GCC 4.3.3 won't work:
Equal.cxx: In member function 'bool EqualRelation::UseStdMultiplicationALLDISTIN
CT() const':
Equal.cxx:1876: internal compiler error: in initialize_flags_in_bb, at tree-into
-ssa.c:437
