#ifndef INPARSE2_HXX
#define INPARSE2_HXX 1

#include "MetaCon1.hxx"
#include "Zaimoni.STL/LexParse/Kuroda.hpp"

// #define KURODA_GRAMMAR 1

kuroda::parser<MetaConcept>& Franci_parser();
bool force_parse(kuroda::parser<MetaConcept>::sequence& symbols);

#endif
