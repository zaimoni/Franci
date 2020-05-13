Notes on coding idioms

* This project started pre-C++98.  It needs enough test cases to identify and fix the current breakage from the "big leap" to C++17.  Last known good build was on TDM-MingW32 GCC 5.2; TDM-MingW32 GCC 5.3-5.5 verified to miscompile, badly.

* The C memory manager overrides are incompatible with MSVC++ Debug libraries (this affects both MSVC++ and CLang in Visual Studio).  Reference build compiler in Visual Studio is CLang, Release mode.

* Many re-implementations of C++11 and higher features are intended to go away.

Requirements imposed by calculation validity include:

* Real-time checks for out-of-bounds memory access.  The C memory manager replacement fulfils this requirement.  It is reasonable to expect that an ISO build with debug libraries would also work.

* ACID assignment operators (in general Franci needs the strongest implementable safety guarantees)
