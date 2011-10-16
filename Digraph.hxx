// Digraph.hxx
// Header for Digraph class, which represents digraphs over finite lists of MetaConcepts.
// This digraph must behave as if the

#ifndef DIGRAPH_DEF
#define DIGRAPH_DEF

#include "MetaCon2.hxx"

class Digraph;
namespace zaimoni {

template<>
struct is_polymorphic_final<Digraph> : public boost::true_type {};

}

class TruthValue;

// NOTE: The true type of a Digraph is VertexSet x VertexSet -> TruthValue.
// We want:
// class EvalFiniteAryFunctionAtArgs(MetaConcept& Function, MetaConcept**& ArgValList, unsigned long**& ArgList);
// This class provides a transparent interface for evaluating a function with certain args predefined.

class Digraph : public MetaConceptWithArgArray
{
private:
	unsigned char** DigraphFromToList;
	LowLevelBinaryRelation* _RelationDefinition;
	AbstractClass* RealUltimateType;
	bool _OwnVertices;
public:
	Digraph(MetaConcept**& NewArgList, bool OwnVertices, LowLevelBinaryRelation* RelationDefinition);
	Digraph(const Digraph& src);
	virtual ~Digraph();

	const Digraph& operator=(const Digraph& src);
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Digraph*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(Digraph*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual void ConstructSelfNameAux(char* Name) const;	// overwrites what is already there
//  defaults are 0 and fatal/false
	virtual unsigned long FunctionArity();
	virtual bool EvaluateFunction(MetaConcept** const& ArgValList, unsigned long*& ArgList, MetaConcept*& Result);
	bool IdxFromVertex(const MetaConcept& Vertex, size_t& Idx) const;

	// Following block is defined by the DECLARE_EDGE_CONTROL_MEMBERS macro
	void SetEdge(const MetaConcept& From, const MetaConcept& To);
	void SetEdge(size_t From, const MetaConcept& To);
	void SetEdge(const MetaConcept& From, size_t To);
	void SetEdge(size_t From, size_t To);
	void SetUndirectedEdge(const MetaConcept& From, const MetaConcept& To);
	void SetUndirectedEdge(size_t From, const MetaConcept& To);
	void SetUndirectedEdge(const MetaConcept& From, size_t To);
	void SetUndirectedEdge(size_t From, size_t To);
	void ResetEdge(const MetaConcept& From, const MetaConcept& To);
	void ResetEdge(size_t From, const MetaConcept& To);
	void ResetEdge(const MetaConcept& From, size_t To);
	void ResetEdge(size_t From, size_t To);
	void ResetUndirectedEdge(const MetaConcept& From, const MetaConcept& To);
	void ResetUndirectedEdge(size_t From, const MetaConcept& To);
	void ResetUndirectedEdge(const MetaConcept& From, size_t To);
	void ResetUndirectedEdge(size_t From, size_t To);
	void ForgetEdge(const MetaConcept& From, const MetaConcept& To);
	void ForgetEdge(size_t From, const MetaConcept& To);
	void ForgetEdge(const MetaConcept& From, size_t To);
	void ForgetEdge(size_t From, size_t To);
	void ForgetUndirectedEdge(const MetaConcept& From, const MetaConcept& To);
	void ForgetUndirectedEdge(size_t From, const MetaConcept& To);
	void ForgetUndirectedEdge(const MetaConcept& From, size_t To);
	void ForgetUndirectedEdge(size_t From, size_t To);
	void SetDiagonal();
	void ResetDiagonal();
	void ForgetDiagonal();
	void SetFromEdges(size_t From);
	void ResetFromEdges(size_t From);
	void ForgetFromEdges(size_t From);
	void SetToEdges(size_t To);
	void ResetToEdges(size_t To);
	void ForgetToEdges(size_t To);
	// end of block of member functions defined by the DECLARE_EDGE_CONTROL_MEMBERS macro

	bool ExplicitEdge(const MetaConcept& From, const MetaConcept& To) const;
	bool ExplicitEdge(size_t From, const MetaConcept& To) const;
	bool ExplicitEdge(const MetaConcept& From, size_t To) const;
	bool ExplicitEdge(size_t From, size_t To) const;
	bool ExplicitNoEdge(const MetaConcept& From, const MetaConcept& To) const;
	bool ExplicitNoEdge(size_t From, const MetaConcept& To) const;
	bool ExplicitNoEdge(const MetaConcept& From, size_t To) const;
	bool ExplicitNoEdge(size_t From, size_t To) const;
	bool ExplicitAmbiguousEdge(const MetaConcept& From, const MetaConcept& To) const;
	bool ExplicitAmbiguousEdge(size_t From, const MetaConcept& To) const;
	bool ExplicitAmbiguousEdge(const MetaConcept& From, size_t To) const;
	bool ExplicitAmbiguousEdge(size_t From, size_t To) const;
	void EdgeTruthValue(const MetaConcept& From, const MetaConcept& To, TruthValue& dest);
	void EdgeTruthValue(size_t From, const MetaConcept& To, TruthValue& dest);
	void EdgeTruthValue(const MetaConcept& From, size_t To, TruthValue& dest);
	void EdgeTruthValue(size_t From, size_t To, TruthValue& dest);
	size_t VertexFromEdgeCount(const MetaConcept& From);
	size_t VertexFromEdgeCount(size_t From);
	size_t VertexToEdgeCount(const MetaConcept& To);
	size_t VertexToEdgeCount(size_t To);
	size_t VertexUndirectedEdgeCount(const MetaConcept& To);
	size_t VertexUndirectedEdgeCount(size_t To);
	size_t VertexAmbiguousFromEdgeCount(const MetaConcept& From) const;
	size_t VertexAmbiguousFromEdgeCount(size_t From) const;
	size_t VertexAmbiguousToEdgeCount(const MetaConcept& To) const;
	size_t VertexAmbiguousToEdgeCount(size_t To) const;
	void LogIncomingEdgeVertices(const MetaConcept& To);
	void LogIncomingEdgeVertices(size_t To);
	void LogLeavingEdgeVertices(const MetaConcept& From);
	void LogLeavingEdgeVertices(size_t From);
	// NOTE: use ArgN to get Nth vertex (origin 0).
	bool RemoveVertex(const MetaConcept& Vertex);
	bool RemoveVertex(size_t Vertex);
	bool AddVertex(const MetaConcept& Vertex);	// NOTE: AddVertex only works when the vertices are owned.
	bool AddVertex(MetaConcept*& Vertex);
	void FlushVerticesWithDirectedEdgeCountsLTE(size_t NonStrictToUB, size_t NonStrictFromUB);
	void FlushVerticesWithUndirectedEdgeCountLTE(size_t NonStrictUB);
	void FlushVerticesNotInStrictSquare();

	// allocates memory on success
	size_t* EnumerateFromEdgesNoLoops(size_t i, size_t EdgeCount,size_t nonstrict_lb, size_t strict_ub) const;
	void MirrorSourceVerticesForIdxNoLoopsNoOwnership(size_t i, size_t EdgeCount,size_t nonstrict_lb, size_t strict_ub, MetaConcept**& MirrorArgArray) const;

	void EnumerateHighestFromEdgesNoLoopsNoAllocation(size_t i, size_t EdgeCount,size_t nonstrict_lb, size_t strict_ub, size_t* IdxList) const;
	bool CompleteStrictGraphSquare(size_t Idx, size_t Idx2, size_t Idx3, size_t& Idx4) const;
	bool VertexEmbedsStrictNStar(size_t* SweepBand, size_t StarRays);
	bool VertexEmbedsCompleteGraph(size_t* SweepBand, size_t StarRays);
	size_t FindStrictHypercubeGraphFlushIrrelevantVertices();
	size_t FindCompleteGraphFlushIrrelevantVertices();

// Externally useful functions
	void InitForClauseAmplification();
	void MaxMinArityForAmplication(size_t& Idx, size_t& ToEdgeCount);

#if 0
	void GenerateIFFClauseList(MetaConcept**& ClauseList,const MetaConcept** const ArgArray);	// generates IFF clauses not implied by prior information
	void GenerateXORClauseList(MetaConcept**& ClauseList,const MetaConcept** const ArgArray);	// generates XOR clauses not implied by prior information
#endif
protected:
	virtual void _forceStdForm();

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
private:
	void EdgeTruthValueCore(size_t From, size_t To, TruthValue& dest);
	size_t VertexFromEdgeCountCore(size_t From);
	size_t VertexToEdgeCountCore(size_t From);
	size_t VertexUndirectedEdgeCountCore(size_t To, size_t lb, size_t strict_ub);
	size_t VertexAmbiguousFromEdgeCountCore(size_t From) const;
	size_t VertexAmbiguousToEdgeCountCore(size_t From) const;
	void LogIncomingEdgeVerticesCore(size_t To);
	void LogLeavingEdgeVerticesCore(size_t From);
	void RemoveVertexCore(size_t Vertex);
	bool AddVertexCore();

	bool CompleteStrictGraphSquareCore(size_t Idx, size_t Idx2, size_t Idx3, size_t& Idx4, size_t nonstrict_lb, size_t strict_ub) const;

	bool VertexEmbedsStrictNStar(size_t* SweepBand, size_t StarRays, size_t nonstrict_lb, size_t strict_ub);
	bool VertexEmbedsLocalHypercubeVertex(size_t* SweepBand, size_t StarRays, size_t nonstrict_lb, size_t strict_ub);
	bool FindStrictHypercubeGraphDimension(size_t Dimension);

	bool VertexEmbedsCompleteGraph(size_t* SweepBand, size_t StarRays, size_t nonstrict_lb, size_t strict_ub);
};

// Digraph can fail to construct, because of its critical dynamically allocated internal data
#define PROPERLY_INIT_DIGRAPH(VAR,ARGARRAY,VAR_OWNS,DECISION_RULE,CLEANUP,ARRAYCLEANUP,DAMAGE_CONTROL)	\
	try	{	\
		VAR = new Digraph(ARGARRAY,VAR_OWNS,DECISION_RULE);	\
		}	\
	catch(const bad_alloc&)	\
		{	\
		ARRAYCLEANUP(ARGARRAY);	\
		DAMAGE_CONTROL;	\
		}	\
	if (!VAR->SyntaxOK())	\
		{	\
		CLEANUP(VAR);	\
		DAMAGE_CONTROL;	\
		}
#endif

