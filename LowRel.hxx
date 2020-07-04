// LowRel.hxx
// header for low-level relations

class MetaConcept;

// Arity 2, symmetric
bool IsAntiIdempotentTo(const MetaConcept& lhs, const MetaConcept& rhs);

bool
AreSyntacticallyEqual(const MetaConcept& LHS, const MetaConcept& RHS);

bool
AreSyntacticallyUnequal(const MetaConcept& LHS, const MetaConcept& RHS);

bool IsSyntacticallyEqualOrAntiIdempotentTo(const MetaConcept& lhs, const MetaConcept& rhs);

bool
IsStdAdditionInverseTo(const MetaConcept& LHS, const MetaConcept& RHS);

// Well, symmetric in the common cases.  Not all multiplication is commutative.
bool
IsStdMultiplicationInverseTo(const MetaConcept& LHS, const MetaConcept& RHS);

// Arity 2, antisymmetric
bool
StrictlyImplies(const MetaConcept& LHS, const MetaConcept& RHS);

bool StrictlyImpliesLogicalNOTOf(const MetaConcept& lhs, const MetaConcept& rhs);
bool LogicalNOTOfStrictlyImplies(const MetaConcept& lhs, const MetaConcept& rhs);

bool
CanStrictlyModify(const MetaConcept& LHS, const MetaConcept& RHS);

bool
CanDeepStrictlyModify(const MetaConcept& LHS, const MetaConcept& RHS);

bool
DeepLogicallyImplies(const MetaConcept& LHS, const MetaConcept& RHS);

bool NonStrictlyImpliesThisOrLogicalNOTOf(const MetaConcept& lhs, const MetaConcept& rhs);
bool NonStrictlyImplies(const MetaConcept& lhs, const MetaConcept& rhs);
bool NonStrictlyImpliesLogicalNOTOf(const MetaConcept& lhs, const MetaConcept& rhs);
bool LogicalNOTOfNonStrictlyImplies(const MetaConcept& lhs, const MetaConcept& rhs);

// Actions
void
SetLHSToRHS(MetaConcept*& Target, const MetaConcept& Inducer);

void
SetLHSToLogicalNOTOfRHS(MetaConcept*& Target, const MetaConcept& Inducer);

void
SetLHSToStdAdditionInverseOfRHS(MetaConcept*& Target, const MetaConcept& Inducer);

void
SetLHSToStdMultiplicationInverseOfRHS(MetaConcept*& Target, const MetaConcept& Inducer);
