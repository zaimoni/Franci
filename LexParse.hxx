#ifndef LEXPARSE_HXX
#define LEXPARSE_HXX 1

#include "Class.hxx"

bool _improviseVar(MetaConcept*& Target, const AbstractClass* Domain);
bool ImproviseVar(MetaConcept*& Target, const AbstractClass* Domain);
bool CoerceArgType(MetaConcept*& Arg, const AbstractClass& ForceType);
bool CoerceArgType(MetaConcept* const& Arg, const AbstractClass& ForceType);
bool LookUpVar(MetaConcept*& Target, const AbstractClass* Domain);

#endif
