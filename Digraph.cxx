// Digraph.cxx
// Implementation for Digraph class, which represents digraphs.

#include "Digraph.hxx"
#include "Class.hxx"
#include "TruthVal.hxx"
#include "LowRel.hxx"
#include "Keyword1.hxx"

#include "Zaimoni.STL/lite_alg.hpp"

#define DIGRAPH_SET_EDGE '\x01'
#define DIGRAPH_RESET_EDGE '\x02'
#define DIGRAPH_FORGET_EDGE '\x00'

Digraph::Digraph(MetaConcept**& NewArgList, bool OwnVertices, LowLevelBinaryRelation* RelationDefinition)
:	MetaConceptWithArgArray(Unknown_MC,NewArgList),
	DigraphFromToList(NULL),
	_RelationDefinition(RelationDefinition),
	RealUltimateType(NULL),
	_OwnVertices(OwnVertices)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/21/2006
	if (!ArgArray) return;

	DigraphFromToList = _new_buffer_nonNULL_throws<unsigned char*>(fast_size());

	size_t i = fast_size();
	do	{
		DigraphFromToList[--i] = _new_buffer<unsigned char>(fast_size());
		if (!DigraphFromToList[i])
			{
			BLOCKDELETEARRAY_AND_NULL(DigraphFromToList);
			throw std::bad_alloc();
			}
		}
	while(0<i);
}

Digraph::Digraph(const Digraph& src)
:	MetaConceptWithArgArray(src),
	DigraphFromToList(NULL),
	_RelationDefinition(src._RelationDefinition),
	RealUltimateType(NULL),	// redundant now...not so with dynamically compiled functions
	_OwnVertices(true)
{	// FORMALLY CORRECT: Kenneth Boyd, 4/21/2006
	if (src.DigraphFromToList)
		{
		const size_t TargetLength = ArraySize(src.DigraphFromToList);
		DigraphFromToList = _new_buffer_nonNULL_throws<unsigned char*>(TargetLength);

		// copy data
		size_t i = TargetLength;
		do	{
			--i;
			CopyDataFromPtrToPtr(DigraphFromToList[i],src.DigraphFromToList[i],TargetLength);
			if (NULL==DigraphFromToList[i])
				{
				BLOCKDELETEARRAY_AND_NULL(DigraphFromToList);
				throw std::bad_alloc();
				}
			}
		while(0<i);
		}
}

Digraph::~Digraph()
{	//! \todo relocate to Destruct.cxx
	//! \bug this code will leak if the function pointed to by _RelationDefinition is dynamically compiled.
	if (DigraphFromToList)
		BLOCKDELETEARRAY_AND_NULL(DigraphFromToList);
	if (!_OwnVertices)
		{	// default corrupts RAM if vertices not owned
		free(ArgArray);
		ArgArray.NULLPtr();
		}
}

const Digraph& Digraph::operator=(const Digraph& src)
{	//! \bug this code will cause undetected pinned pointers if the function pointed to by  _RelationDefinition is dynamically compiled.
	_RelationDefinition = src._RelationDefinition;
	if (NULL!=src.DigraphFromToList)
		{
		const size_t TargetLength = ArraySize(src.DigraphFromToList);
		if (!_resize(DigraphFromToList,TargetLength))
			{
			_flush(DigraphFromToList);
			UnconditionalRAMFailure();
			}

		// copy data
		size_t i = TargetLength;
		do	{
			--i;
			CopyDataFromPtrToPtr(DigraphFromToList[i],src.DigraphFromToList[i],TargetLength);
			if (!DigraphFromToList[i])
				{
				BLOCKDELETEARRAY_AND_NULL(DigraphFromToList);
				UnconditionalRAMFailure();
				}
			}
		while(0<i);
		}
	else{
		if (DigraphFromToList)
			BLOCKDELETEARRAY_AND_NULL(DigraphFromToList);			
		}
	// properly handle assignment of ArgArray
	MetaConceptWithArgArray::operator=(src);
	_OwnVertices = true;
	return *this;
}

void Digraph::MoveInto(Digraph*& dest)
{	
	if (dest->DigraphFromToList)
		BLOCKDELETEARRAY_AND_NULL(dest->DigraphFromToList);
	dest->DigraphFromToList = DigraphFromToList;
	DigraphFromToList = NULL;
	if (!dest->ArgArray.empty() && !dest->_OwnVertices)
		{
		free(ArgArray);
		ArgArray.NULLPtr();
		}
	ArgArray.MoveInto(dest->ArgArray);
	dest->_OwnVertices = _OwnVertices;
	dest->_RelationDefinition = _RelationDefinition;
}

//  Type ID functions
const AbstractClass* Digraph::UltimateType() const
{	//! \bug once we have function types; correct type is Vertex x Vertex |-> TruthValue
	return NULL;
}

void Digraph::_forceStdForm() {ForceStdFormAux();}

//  Evaluation functions
bool Digraph::SyntaxOK() const
{	//! \todo IMPLEMENT fully
	if (!DigraphFromToList) return false;
	return SyntaxOKAux();
}

// text I/O functions
// NOTE: what is the textual representation of a Digraph?
// #1: domain, matrix (requires enumerated set, matrix to parse)
// #2: list of edges (enumerated set of 2-tuples)
//! \todo move to LenName.cxx when ready
size_t Digraph::LengthOfSelfName() const
{	//! \todo IMPLEMENT correctly (placeholder now)
	return 10;
}

void Digraph::ConstructSelfNameAux(char* Name) const	// overwrites what is already there
{	//! \todo IMPLEMENT correctly (placeholder now)
	Name[0] = 'D';
	Name[1] = 'I';
	Name[2] = 'G';
	Name[3] = 'R';
	Name[4] = 'A';
	Name[5] = 'P';
	Name[6] = 'H';
	Name[7] = '(';
	Name[8] = ',';
	Name[9] = ')';
}

unsigned long Digraph::FunctionArity()
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	return 2;
}

bool
Digraph::EvaluateFunction(MetaConcept** const& ArgValList, unsigned long*& ArgList, MetaConcept*& Result)
{	//! \todo fix when restricted-arg functions are implemented
	if (!ValidateFunctionArgList(ArgValList,ArgList)) return false;
	if (2!=ArraySize(ArgValList)) return false;
	size_t Arg0Idx, Arg1Idx;
	if (   !IdxFromVertex(*ArgValList[0],Arg0Idx)
		|| !IdxFromVertex(*ArgValList[1],Arg1Idx))
		return false;
	try	{
		TruthValue* Temp = new TruthValue();
		if (0==ArgList[0])
			Temp->_x = EdgeTruthValue(Arg0Idx,Arg1Idx);	// 0, 1
		else	
			Temp->_x = EdgeTruthValue(Arg1Idx,Arg0Idx);	// 1, 0
		delete Result;
		Result = Temp;
		return true;	
		}
	catch(const bad_alloc&)
		{
		return false;
		}
}

std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > Digraph::canEvaluate() const // \todo obviate DiagnoseInferenceRules
{
	return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >();
}

void Digraph::DiagnoseInferenceRules() const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	if (DiagnoseStandardEvalRules()) return;
	IdxCurrentSelfEvalRule=SelfEvalSyntaxOKNoRules_SER;
}

bool Digraph::InvokeEqualArgRule() const
{	//! \todo IMPLEMENT: this is an integrate-and-delete redundant vertex invocation.
	return false;
}

bool Digraph::IdxFromVertex(const MetaConcept& Vertex, size_t& Idx) const
{
	if (!ArgArray.empty())
		{
		size_t i = fast_size();
		do	if (Vertex==*ArgArray[--i])
				{
				Idx = i;
				return true;
				}
		while(0<i);
		}
	return false;
}

#define DECLARE_EDGE_CONTROL_MEMBERS(A,B)	\
void Digraph::A##Edge(const MetaConcept& From, const MetaConcept& To)	\
{	\
	size_t To2,From2;	\
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));	\
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));	\
	DigraphFromToList[From2][To2] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##Edge(size_t From, const MetaConcept& To)	\
{	\
	size_t To2;	\
	assert(size()>From);	\
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));	\
	DigraphFromToList[From][To2] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##Edge(const MetaConcept& From, size_t To)	\
{	\
	size_t From2;	\
	assert(size()>To);	\
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));	\
	DigraphFromToList[From2][To] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##Edge(size_t From, size_t To)	\
{	\
	assert(size()>From);	\
	assert(size()>To);	\
	DigraphFromToList[From][To] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##UndirectedEdge(const MetaConcept& From, const MetaConcept& To)	\
{	\
	size_t To2,From2;	\
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));	\
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));	\
	DigraphFromToList[From2][To2] = DIGRAPH_##B##_EDGE;	\
	DigraphFromToList[To2][From2] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##UndirectedEdge(size_t From, const MetaConcept& To)	\
{	\
	size_t To2;	\
	assert(size()>From);	\
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));	\
	DigraphFromToList[From][To2] = DIGRAPH_##B##_EDGE;	\
	DigraphFromToList[To2][From] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##UndirectedEdge(const MetaConcept& From, size_t To)	\
{	\
	size_t From2;	\
	assert(size()>To);	\
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));	\
	DigraphFromToList[From2][To] = DIGRAPH_##B##_EDGE;	\
	DigraphFromToList[To][From2] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##UndirectedEdge(size_t From, size_t To)	\
{	\
	assert(size()>From);	\
	assert(size()>To);	\
	DigraphFromToList[From][To] = DIGRAPH_##B##_EDGE;	\
	DigraphFromToList[To][From] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##Diagonal()	\
{	\
	size_t i = size();	\
	while(0<i)	\
		{	\
		--i;	\
		DigraphFromToList[i][i] = DIGRAPH_##B##_EDGE;	\
		};	\
}	\
	\
void Digraph::A##FromEdges(size_t From)	\
{	\
	assert(size()>From);	\
	size_t i = size();	\
	while(0<i) DigraphFromToList[From][--i] = DIGRAPH_##B##_EDGE;	\
}	\
	\
void Digraph::A##ToEdges(size_t To)	\
{	\
	assert(size()>To);	\
	size_t i = size();	\
	while(0<i) DigraphFromToList[--i][To] = DIGRAPH_##B##_EDGE;	\
}

DECLARE_EDGE_CONTROL_MEMBERS(Set,SET)
DECLARE_EDGE_CONTROL_MEMBERS(Reset,RESET)
DECLARE_EDGE_CONTROL_MEMBERS(Forget,FORGET)

#undef DECLARE_EDGE_CONTROL_MEMBERS

#define DECLARE_EXPLICIT_EDGE_MEMBERS(A,B)	\
bool Digraph::Explicit##A(const MetaConcept& From, const MetaConcept& To) const	\
{	\
	size_t To2,From2;	\
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));	\
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));	\
	return DIGRAPH_##B##_EDGE==DigraphFromToList[From2][To2];	\
}	\
	\
bool Digraph::Explicit##A(size_t From, const MetaConcept& To) const \
{	\
	size_t To2;	\
	assert(size()>From);	\
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));	\
	return DIGRAPH_##B##_EDGE==DigraphFromToList[From][To2];	\
}	\
	\
bool Digraph::Explicit##A(const MetaConcept& From, size_t To) const \
{	\
	size_t From2;	\
	assert(size()>To);	\
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));	\
	return DIGRAPH_##B##_EDGE==DigraphFromToList[From2][To];	\
}	\
	\
bool Digraph::Explicit##A(size_t From, size_t To) const \
{	\
	assert(size()>From);	\
	assert(size()>To);	\
	return DIGRAPH_##B##_EDGE==DigraphFromToList[From][To];	\
}

DECLARE_EXPLICIT_EDGE_MEMBERS(Edge,SET)
DECLARE_EXPLICIT_EDGE_MEMBERS(NoEdge,RESET)
DECLARE_EXPLICIT_EDGE_MEMBERS(AmbiguousEdge,FORGET)

#undef DECLARE_EXPLICIT_EDGE_MEMBERS

TVal Digraph::EdgeTruthValue(const MetaConcept& From, const MetaConcept& To)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	size_t From2,To2;
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));
	return EdgeTruthValueCore(From2,To2);
}

TVal Digraph::EdgeTruthValue(size_t From, const MetaConcept& To)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	size_t To2;
	assert(size()>From);
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));
	return EdgeTruthValueCore(From,To2);
}

TVal Digraph::EdgeTruthValue(const MetaConcept& From, size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	size_t From2;
	assert(size()>To);
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));
	return EdgeTruthValueCore(From2,To);
}

TVal Digraph::EdgeTruthValue(size_t From, size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	assert(size()>From);
	assert(size()>To);
	return EdgeTruthValueCore(From,To);
}

// Efficiency problems
// Remove this define block if it proves necessary to support sparse graphs
#if 1
#define ExplicitEdge(A,B) (DIGRAPH_SET_EDGE==DigraphFromToList[A][B])
#define ExplicitNoEdge(A,B) (DIGRAPH_RESET_EDGE==DigraphFromToList[A][B])
#define ExplicitAmbiguousEdge(A,B) (DIGRAPH_FORGET_EDGE==DigraphFromToList[A][B])

#define SetEdge(A,B) DigraphFromToList[A][B]=DIGRAPH_SET_EDGE
#define ResetEdge(A,B) DigraphFromToList[A][B]=DIGRAPH_RESET_EDGE
#define ForgetEdge(A,B) DigraphFromToList[A][B]=DIGRAPH_FORGET_EDGE
#endif

TVal Digraph::EdgeTruthValueCore(size_t From, size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/17/2005
	if 		(ExplicitEdge(From,To)) return true;
	else if (ExplicitNoEdge(From,To)) return false;
	SUCCEED_OR_DIE(ExplicitAmbiguousEdge(From,To));
	if (!_RelationDefinition) return TVal();
	if (_RelationDefinition(*ArgArray[From],*ArgArray[To]))
		{	// NOTE: this is a target for properties of relations (symmetric, antisymmetric, etc)
		SetEdge(From,To);
		return true;
		}
	else{
		ResetEdge(From,To);
		return false;
		}
}

size_t Digraph::VertexFromEdgeCount(const MetaConcept& From)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	size_t From2;
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));
	return VertexFromEdgeCountCore(From2);
}

size_t Digraph::VertexFromEdgeCount(size_t From)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	assert(size()>From);
	return VertexFromEdgeCountCore(From);
}

size_t Digraph::VertexFromEdgeCountCore(size_t From)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/19/2005
	size_t Count = 0;
	size_t i = ArraySize(DigraphFromToList[From]);
	do	{
		if		(ExplicitAmbiguousEdge(From,--i))
			{
			if (_RelationDefinition)
				{
				if (_RelationDefinition(*ArgArray[From],*ArgArray[i]))
					{	//! \todo this is a target for properties of relations (symmetric, antisymmetric, etc)
					SetEdge(From,i);
					Count++;
					}
				else ResetEdge(From,i);
				}
			}
		else if (ExplicitEdge(From,i))
			Count++;
		else
			SUCCEED_OR_DIE(ExplicitNoEdge(From,i));
		}
	while(0<i);
	return Count;
}

size_t Digraph::VertexToEdgeCount(const MetaConcept& To)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/23/2003
	size_t To2;
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));
	return VertexToEdgeCountCore(To2);
}

size_t Digraph::VertexToEdgeCount(size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/23/2003
	assert(size()>To);
	return VertexToEdgeCountCore(To);
}

size_t Digraph::VertexToEdgeCountCore(size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/19/2005
	size_t Count = 0;
	size_t i = ArraySize(DigraphFromToList);
	do	{
		if		(ExplicitAmbiguousEdge(--i,To))
			{
			if (_RelationDefinition)
				{
				if (_RelationDefinition(*ArgArray[i],*ArgArray[To]))
					{	//! \todo this is a target for properties of relations (symmetric, antisymmetric, etc)
					SetEdge(i,To);
					Count++;
					}
				else ResetEdge(i,To);
				}
			}
		else if (ExplicitEdge(i,To))
			Count++;
		else
			SUCCEED_OR_DIE(ExplicitNoEdge(i,To));
		}
	while(0<i);
	return Count;
}

size_t Digraph::VertexUndirectedEdgeCount(const MetaConcept& To)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2003
	size_t To2;
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));
	return VertexUndirectedEdgeCountCore(To2,0,fast_size());
}

size_t Digraph::VertexUndirectedEdgeCount(size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2003
	assert(size()>To);
	return VertexUndirectedEdgeCountCore(To,0,fast_size());
}

size_t
Digraph::VertexUndirectedEdgeCountCore(size_t From, size_t lb, size_t strict_ub)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/19/2005
	size_t Count = 0;
	size_t i = strict_ub;
	do	{
		if		(ExplicitAmbiguousEdge(From,--i))
			{
			if (_RelationDefinition)
				{
				if (_RelationDefinition(*ArgArray[From],*ArgArray[i]))
					{	//! \todo this is a target for properties of relations (symmetric, antisymmetric, etc)
					SetUndirectedEdge(From,i);
					Count++;
					}
				else{
					ResetUndirectedEdge(From,i);
					}
				}
			}
		else if (ExplicitEdge(From,i))
			Count++;
		else
			SUCCEED_OR_DIE(ExplicitNoEdge(From,i));
		}
	while(lb<i);
	return Count;
}

size_t Digraph::VertexAmbiguousFromEdgeCount(const MetaConcept& From) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	size_t From2;
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));
	return VertexAmbiguousFromEdgeCountCore(From2);
}

size_t Digraph::VertexAmbiguousFromEdgeCount(size_t From) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	assert(size()>From);
	return VertexAmbiguousFromEdgeCountCore(From);
}

size_t Digraph::VertexAmbiguousFromEdgeCountCore(size_t From) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/19/2005
	if (_RelationDefinition) return 0;
	size_t Count = 0;
	size_t i = ArraySize(DigraphFromToList[From]);
	do	{
		if		(ExplicitAmbiguousEdge(From,--i))
			Count++;
		else
			SUCCEED_OR_DIE(ExplicitEdge(From,i) || ExplicitNoEdge(From,i));
		}
	while(0<i);
	return Count;
}

size_t Digraph::VertexAmbiguousToEdgeCount(const MetaConcept& From) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	size_t From2;
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));
	return VertexAmbiguousToEdgeCountCore(From2);
}

size_t Digraph::VertexAmbiguousToEdgeCount(size_t From) const
{	// FORMALLY CORRECT: Kenneth Boyd, 9/6/2001
	assert(size()>From);
	return VertexAmbiguousToEdgeCountCore(From);
}

size_t Digraph::VertexAmbiguousToEdgeCountCore(size_t To) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/19/2005
	if (_RelationDefinition) return 0;
	size_t Count = 0;
	size_t i = ArraySize(DigraphFromToList);
	do	{
		if		(ExplicitAmbiguousEdge(--i,To))
			Count++;
		else
			SUCCEED_OR_DIE(ExplicitEdge(i,To) || ExplicitNoEdge(i,To));
		}
	while(0<i);
	return Count;
}
	
// NOTE: use ArgN to get Nth vertex (origin 0).
bool Digraph::RemoveVertex(const MetaConcept& Vertex)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/27/2001
	size_t Vertex2;
	if (!IdxFromVertex(Vertex,Vertex2)) return false;
	RemoveVertexCore(Vertex2);
	return true;
}

bool Digraph::RemoveVertex(size_t Vertex)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/27/2001
	if (size()<=Vertex) return false;
	RemoveVertexCore(Vertex);
	return true;
}

void Digraph::RemoveVertexCore(size_t Vertex)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/16/2003
	// #1: remove target Idx array/row
	if (!_OwnVertices) ArgArray[Vertex]=NULL;
	ArgArray.FastDeleteIdx(Vertex);
	_delete_idx(DigraphFromToList,Vertex);
	// #2: remove n-1 target Idx array/col
	if (NULL!=DigraphFromToList)
		{
		size_t i = ArraySize(DigraphFromToList);
		do	_delete_idx(DigraphFromToList[--i],Vertex);
		while(0<i);
		}
}

bool Digraph::AddVertex(const MetaConcept& Vertex)
{	// FORMALLY CORRECT: 10/30/2005
	size_t Idx3;
	if (IdxFromVertex(Vertex,Idx3)) return true;
	if (!_OwnVertices) return false;
	MetaConcept* NewVertex = NULL;
	try	{
		Vertex.CopyInto(NewVertex);
		}
	catch(const bad_alloc&)
		{
		return false;
		}
	if (!InsertSlotAt(fast_size(),NewVertex))
		{
		delete NewVertex;
		return false;		
		}
	return AddVertexCore();
}

bool Digraph::AddVertex(MetaConcept*& Vertex)
{	// FORMALLY CORRECT: 10/25/2005
	size_t Idx3;
	if (IdxFromVertex(*Vertex,Idx3)) return true;
	if (!_OwnVertices) return false;
	if (!InsertSlotAt(fast_size(),Vertex)) return false;
	Vertex = NULL;
	return AddVertexCore();
}

bool Digraph::AddVertexCore()
{	// FORMALLY CORRECT: Kenneth Boyd, 4/21/2006
	const size_t TargetSize = fast_size();
	size_t i = TargetSize-1;	
	while(0<i)
		if (!_resize(DigraphFromToList[--i],TargetSize))
			{
			while(ArraySize(DigraphFromToList)> ++i)
				_shrink(DigraphFromToList[i],TargetSize-1);
			FastDeleteIdx(fast_size()-1);
			return false;
			};

	unsigned char* Tmp = _new_buffer<unsigned char>(TargetSize);
	if (NULL==Tmp)
		{
		size_t i = ArraySize(DigraphFromToList);	
		do	_shrink(DigraphFromToList[--i],TargetSize-1);
		while(0<i);
		FastDeleteIdx(fast_size()-1);
		return false;
		}
	if (!_resize(DigraphFromToList,TargetSize))
		{
		free(Tmp);
		size_t i = TargetSize;
		do	_shrink(DigraphFromToList[--i],TargetSize-1);
		while(0<i);
		FastDeleteIdx(fast_size()-1);
		return false;
		};
	DigraphFromToList[ArraySize(DigraphFromToList)-1] = Tmp;
	return true;
}

void
Digraph::FlushVerticesWithDirectedEdgeCountsLTE(size_t NonStrictToUB, size_t NonStrictFromUB)
{	// FORMALLY CORRECT: Kenneth Boyd, 8/18/2003
	if (!ArgArray.empty())
		{
		size_t i = fast_size();
		do	if (	NonStrictToUB>=VertexToEdgeCount(--i)
				&&	NonStrictFromUB>=VertexFromEdgeCount(i))
				RemoveVertex(i);
		while(0<i);
		}
}

void Digraph::FlushVerticesWithUndirectedEdgeCountLTE(size_t NonStrictUB)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/17/2003
	if (!ArgArray.empty())
		{
		size_t Memory;
		do	{
			Memory = fast_size();
			size_t i = fast_size();
			do	if (NonStrictUB>=VertexUndirectedEdgeCountCore(--i,0,fast_size()))
					RemoveVertex(i);
			while(0<i);
			}
		while(!ArgArray.empty() && fast_size()!=Memory && 0<NonStrictUB);
		}
}

void Digraph::FlushVerticesNotInStrictSquare()
{
	FlushVerticesWithUndirectedEdgeCountLTE(1);
	size_t VertexBuffer[7];
	VertexBuffer[0] = fast_size();
	do	{
		--VertexBuffer[0];
		if (!VertexEmbedsLocalHypercubeVertex(VertexBuffer,2,0,fast_size()))
			RemoveVertex(VertexBuffer[0]);
		}
	while(0<VertexBuffer[0]);
}

#if 0
void Digraph::FlushVerticesNotInCompleteGraph(size_t StarRays)
{
	FlushVerticesWithUndirectedEdgeCountLTE(1);
	size_t i = fast_size();
	do	if (!VertexEmbedsCompleteGraph(--i,StarRays)) RemoveVertex(i);
	while(0<Idx);
}
#endif

void Digraph::LogIncomingEdgeVertices(const MetaConcept& To)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/10/2003
	size_t To2;
	SUCCEED_OR_DIE(IdxFromVertex(To,To2));
	LogIncomingEdgeVerticesCore(To2);
}

void Digraph::LogIncomingEdgeVertices(size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/10/2003
	assert(size()>To);
	LogIncomingEdgeVerticesCore(To);
}

void Digraph::LogIncomingEdgeVerticesCore(size_t To)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/10/2003
	size_t IncomingVertices = VertexToEdgeCountCore(To);
	if (IncomingVertices)
		{
		size_t i = fast_size();
		do	if (ExplicitEdge(--i,To)) LOG(*ArgArray[i]);
		while(0<i);
		}
}

void Digraph::LogLeavingEdgeVertices(const MetaConcept& From)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/10/2003
	size_t From2;
	SUCCEED_OR_DIE(IdxFromVertex(From,From2));
	LogLeavingEdgeVerticesCore(From2);
}

void Digraph::LogLeavingEdgeVertices(size_t From)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/10/2003
	assert(size()>From);
	LogLeavingEdgeVerticesCore(From);
}

void Digraph::LogLeavingEdgeVerticesCore(size_t From)
{	// FORMALLY CORRECT: Kenneth Boyd, 3/10/2003
	size_t IncomingVertices = VertexFromEdgeCountCore(From);
	if (IncomingVertices)
		{
		size_t i = fast_size();
		do	if (ExplicitEdge(From,--i)) LOG(*ArgArray[i]);
		while(0<i);
		}
}

// NOTE: the enumeration is guaranteed to be in strictly increasing order.
size_t*
Digraph::EnumerateFromEdgesNoLoops(const size_t i, const size_t EdgeCount,const size_t nonstrict_lb, const size_t strict_ub) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2003
	assert(strict_ub>nonstrict_lb);
	size_t* const IdxList = EdgeCount ? _new_buffer<size_t>(EdgeCount): NULL;
	if (IdxList)
		{
		size_t TargetIdx = EdgeCount;
		size_t j = strict_ub;
		do	if (--j!=i && ExplicitEdge(i,j))
				{
				SUCCEED_OR_DIE(0<TargetIdx);
				IdxList[--TargetIdx] = j;
				}
		while(nonstrict_lb<j);
		SUCCEED_OR_DIE(0==TargetIdx);
		}
	return IdxList;
}

void
Digraph::MirrorSourceVerticesForIdxNoLoopsNoOwnership(const size_t i, const size_t EdgeCount,const size_t nonstrict_lb, const size_t strict_ub, MetaConcept**& MirrorArgArray) const
{	// FORMALLY CORRECT: Kenneth Boyd, 3/3/2006
	assert(!MirrorArgArray);
	assert(nonstrict_lb<strict_ub);
	if (0==EdgeCount) return;
	MirrorArgArray = _new_buffer<MetaConcept*>(EdgeCount);
	if (MirrorArgArray)
		{
		size_t TargetIdx = EdgeCount;
		size_t j = strict_ub;
		do	if (--j!=i && ExplicitEdge(j,i))
				{
				SUCCEED_OR_DIE(0<TargetIdx);
				MirrorArgArray[--TargetIdx] = ArgArray[j];
				}
		while(nonstrict_lb<j);
		SUCCEED_OR_DIE(0==TargetIdx);
		}
}

void
Digraph::EnumerateHighestFromEdgesNoLoopsNoAllocation(const size_t i, const size_t EdgeCount,const size_t nonstrict_lb, const size_t strict_ub, size_t* const IdxList) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2003
	assert(0<EdgeCount);
	assert(IdxList);
	size_t TargetIdx = EdgeCount;
	size_t j = strict_ub;
	do	if (--j!=i && ExplicitEdge(i,j))
			{
			IdxList[--TargetIdx] = j;
			if (0==TargetIdx) return;
			}
	while(nonstrict_lb<j);
	SUCCEED_OR_DIE(0==TargetIdx);
}

// NOTE: diagnosis of strict N-stars and complete graphs is very similar; consider
// using a common macro definition

// Strict N-star: N edges going out, no two external endpoints connected
// SweepBand must be large enough to hold the initial vertex [0] and the endpoints of the rays
// [1..StarRays].
bool Digraph::VertexEmbedsStrictNStar(size_t* SweepBand, size_t StarRays)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/24/2003
	return VertexEmbedsStrictNStar(SweepBand,StarRays,0,fast_size());
}

bool
Digraph::VertexEmbedsStrictNStar(size_t* const SweepBand, const size_t StarRays, size_t nonstrict_lb, size_t strict_ub)
{	//! \todo VERIFY
	size_t i = SweepBand[0];
	size_t EdgeCount = VertexUndirectedEdgeCountCore(i,nonstrict_lb,strict_ub);
	if (StarRays>EdgeCount) return false;
	else if (1>=StarRays) return true;

	// Adjust nonstrict_lb for technical accuracy
	while(nonstrict_lb==i || !ExplicitEdge(i,nonstrict_lb))
		if (++nonstrict_lb+StarRays>strict_ub) return false;

	// Adjust strict_ub for technical accuracy
	while(strict_ub-1==i || !ExplicitEdge(i,strict_ub-1))
		if (nonstrict_lb+StarRays> --strict_ub) return false;

	if (2==EdgeCount)
		{	// also implies 2==StarRays.  Don't bother with dynamic allocation
		SweepBand[1] = nonstrict_lb;
		SweepBand[2] = strict_ub-1;
		if (ExplicitEdge(SweepBand[2],SweepBand[0]))
			return ExplicitNoEdge(SweepBand[2],SweepBand[1]);
		return false;
		}

	size_t* const VertexEnumeration = SweepBand+1;

	// enumerate edges explicitly.  This should run faster
	// because all of the ExplicitEdge(Idx,___) tests automatically succeed.
	if (StarRays==EdgeCount)
		{	// No tolerance, any cross-connects break the N-star.
		EnumerateHighestFromEdgesNoLoopsNoAllocation(SweepBand[0],EdgeCount,nonstrict_lb,strict_ub,VertexEnumeration);
		size_t j = StarRays;
		do	{
			size_t k = --j;
			do	if (ExplicitEdge(VertexEnumeration[j],VertexEnumeration[--k]))
					return false;
			while(0<k);
			}
		while(1<j);
		return true;
		}

	if (2*StarRays-1>EdgeCount)
		{	// Tolerance is less than our window.  We might be able to get away with deduction.
		size_t EdgesSeen = StarRays;
		size_t Tolerance = EdgeCount-EdgesSeen;
		EnumerateHighestFromEdgesNoLoopsNoAllocation(SweepBand[0],StarRays,nonstrict_lb,strict_ub,VertexEnumeration);

		bool Disqualified = false;
		size_t j = StarRays;
		do	{
			size_t Failures = 0;
			size_t k = StarRays;
			--j;
			do	if (   j!=--k
					&& ExplicitEdge(VertexEnumeration[j],VertexEnumeration[k]))
					{
					Disqualified = true;
					if (++Failures>Tolerance)
						{	// VertexEnumeration[j] is completely unacceptable.
							// We're going to have to eliminate it from consideration.
						if (++EdgesSeen>EdgeCount)
							return false;	// not recoverable
						if (1!=j)
							memmove(&VertexEnumeration[1],&VertexEnumeration[0],(j-1)*sizeof(size_t));
						EnumerateHighestFromEdgesNoLoopsNoAllocation(SweepBand[0],1,nonstrict_lb,VertexEnumeration[1],VertexEnumeration);
						if (j==StarRays-1)
							{	// just discarded highest vertex.  Adjust StrictUB, etc.
							strict_ub = VertexEnumeration[StarRays-1]+1;
							EdgesSeen--;
							EdgeCount--;
							}
						// Lose tolerance of errors
						// Tolerance = EdgeCount-EdgesSeen;
						Tolerance--;
						k = 0;
						j = StarRays;
						Disqualified = false;
						}
					}
			while(0<k);
			}
		while(0<j);
		if (!Disqualified) return true;
		// Heuristics failed.
		}

	size_t* const VertexIdxEnumeration = EnumerateFromEdgesNoLoops(i,EdgeCount,nonstrict_lb,strict_ub);
	if (VertexIdxEnumeration)
		{
		// This code scans for an arbitrary permutation that satisifies a pairwise constraint.
		size_t ScanPoint = StarRays-1;
		VertexEnumeration[StarRays-1] = ArraySize(VertexIdxEnumeration)-1;
		do	{
			bool Failed = false;
			size_t Validate = StarRays;
			while(ScanPoint<--Validate)
				if (ExplicitEdge(VertexIdxEnumeration[VertexEnumeration[ScanPoint]],VertexIdxEnumeration[VertexEnumeration[Validate]]))
					{
					Failed = true;
					break;
					}
			if (Failed)
				{
				while(ScanPoint==VertexEnumeration[ScanPoint])
					if (++ScanPoint==StarRays)
						{
						free(VertexIdxEnumeration);
						return false;
						}
				VertexEnumeration[ScanPoint]--;
				}
			else if (0<ScanPoint)
				{
				ScanPoint--;
				VertexEnumeration[ScanPoint] = VertexEnumeration[ScanPoint+1]-1;
				}
			else{
				do	VertexEnumeration[ScanPoint]=VertexIdxEnumeration[VertexEnumeration[ScanPoint]];
				while(++ScanPoint<StarRays);
				free(VertexIdxEnumeration);
				return true;
				}
			}
		while(true);
		// should never get here
		return false;
		}

	// This code scans for an arbitrary permutation that satisifies a pairwise constraint.
	size_t ScanPoint = StarRays-1;
	VertexEnumeration[StarRays-1] = strict_ub-1;
	do	{
		bool Failed = false;
		if (   i==VertexEnumeration[ScanPoint]
			|| ExplicitNoEdge(i,VertexEnumeration[ScanPoint]))
			Failed = true;

		if (!Failed)
			{
			size_t Validate = StarRays;
			while(ScanPoint<--Validate)
				if (ExplicitEdge(VertexEnumeration[ScanPoint],VertexEnumeration[Validate]))
					{
					Failed = true;
					break;
					}
			}

		if (Failed)
			{
			while(nonstrict_lb+ScanPoint==VertexEnumeration[ScanPoint])
				if (++ScanPoint==StarRays)
					return false;
			VertexEnumeration[ScanPoint]--;
			}
		else if (0<ScanPoint)
			{
			ScanPoint--;
			VertexEnumeration[ScanPoint] = VertexEnumeration[ScanPoint+1]-1;
			}
		else
			return true;
		}
	while(true);
	// should never get here
	return false;
}

bool Digraph::VertexEmbedsCompleteGraph(size_t* SweepBand, size_t StarRays)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2003
	return VertexEmbedsCompleteGraph(SweepBand,StarRays,0,fast_size());
}

//! \todo port StarRays==3 N-star code here
bool
Digraph::VertexEmbedsCompleteGraph(size_t* const SweepBand, const size_t StarRays, size_t nonstrict_lb, size_t strict_ub)
{	//! \todo VERIFY
	size_t i = SweepBand[0];
	size_t EdgeCount = VertexUndirectedEdgeCountCore(i,nonstrict_lb,strict_ub);
	if (StarRays>EdgeCount) return false;
	else if (1>=StarRays) return true;

	// Adjust nonstrict_lb for technical accuracy
	while(nonstrict_lb==i || !ExplicitEdge(i,nonstrict_lb))
		if (++nonstrict_lb+StarRays>strict_ub) return false;

	// Adjust strict_ub for technical accuracy
	while(strict_ub-1==i || !ExplicitEdge(i,strict_ub-1))
		if (nonstrict_lb+StarRays> --strict_ub) return false;

	if (2==EdgeCount)
		{	// also implies 2==StarRays.  Don't bother with dynamic allocation
		SweepBand[1] = nonstrict_lb;
		SweepBand[2] = strict_ub-1;
		if (ExplicitEdge(SweepBand[2],SweepBand[0]))
			return ExplicitEdge(SweepBand[2],SweepBand[1]);
		return false;
		}

	size_t* const VertexEnumeration = SweepBand+1;

	// enumerate edges explicitly.  This should run faster
	// because all of the ExplicitEdge(Idx,___) tests automatically succeed.
	if (StarRays==EdgeCount)
		{	// No tolerance, any failures to cross-connects break the complete graph.
		EnumerateHighestFromEdgesNoLoopsNoAllocation(SweepBand[0],EdgeCount,nonstrict_lb,strict_ub,VertexEnumeration);
		size_t j = StarRays;
		do	{
			size_t k = --j;
			do	if (ExplicitNoEdge(VertexEnumeration[j],VertexEnumeration[--k]))
					return false;
			while(0<k);
			}
		while(1<j);
		return true;
		}

	if (2*StarRays-1>EdgeCount)
		{	// Tolerance is less than our window.  We might be able to get away with deduction.
		size_t EdgesSeen = StarRays;
		size_t Tolerance = EdgeCount-EdgesSeen;
		EnumerateHighestFromEdgesNoLoopsNoAllocation(SweepBand[0],StarRays,nonstrict_lb,strict_ub,VertexEnumeration);

		bool Disqualified = false;
		size_t j = StarRays;
		do	{
			size_t Failures = 0;
			size_t k = StarRays;
			--j;
			do	if (   j!=--k
					&& ExplicitNoEdge(VertexEnumeration[j],VertexEnumeration[k]))
					{
					Disqualified = true;
					if (++Failures>Tolerance)
						{	// VertexEnumeration[j] is completely unacceptable.
							// We're going to have to eliminate it from consideration.
						if (++EdgesSeen>EdgeCount)
							return false;	// not recoverable
						if (1!=j)
							memmove(&VertexEnumeration[1],&VertexEnumeration[0],(j-1)*sizeof(size_t));
						EnumerateHighestFromEdgesNoLoopsNoAllocation(SweepBand[0],1,nonstrict_lb,VertexEnumeration[1],VertexEnumeration);
						if (j==StarRays-1)
							{	// just discarded highest vertex.  Adjust StrictUB, etc.
							strict_ub = VertexEnumeration[StarRays-1]+1;
							EdgesSeen--;
							EdgeCount--;
							}
						// Lose tolerance of errors
						// Tolerance = EdgeCount-EdgesSeen;
						Tolerance--;
						k = 0;
						j = StarRays;
						Disqualified = false;
						}
					}
			while(0<k);
			}
		while(0<j);
		if (!Disqualified) return true;
		// Heuristics failed.
		}

	// enumerate edges explicitly.  This should run faster
	// because all of the ExplicitEdge(Idx,___) tests automatically succeed.
	size_t* const VertexIdxEnumeration = EnumerateFromEdgesNoLoops(i,EdgeCount,nonstrict_lb,strict_ub);
	if (VertexIdxEnumeration)
		{
		// This code scans for an arbitrary permutation that satisifies a pairwise constraint.
		size_t ScanPoint = StarRays-1;
		VertexEnumeration[StarRays-1] = ArraySize(VertexIdxEnumeration)-1;
		do	{
			bool Failed = false;
			size_t Validate = StarRays;
			while(ScanPoint<--Validate)
				if (ExplicitNoEdge(VertexIdxEnumeration[VertexEnumeration[ScanPoint]],VertexIdxEnumeration[VertexEnumeration[Validate]]))
					{
					Failed = true;
					break;
					}
			if (Failed)
				{
				while(ScanPoint==VertexEnumeration[ScanPoint])
					if (++ScanPoint==StarRays)
						{
						free(VertexIdxEnumeration);
						return false;
						}
				VertexEnumeration[ScanPoint]--;
				}
			else if (0<ScanPoint)
				{
				ScanPoint--;
				VertexEnumeration[ScanPoint] = VertexEnumeration[ScanPoint+1]-1;
				}
			else{
				do	VertexEnumeration[ScanPoint]=VertexIdxEnumeration[VertexEnumeration[ScanPoint]];
				while(++ScanPoint<StarRays);
				free(VertexIdxEnumeration);
				return true;
				}
			}
		while(true);
		// should never get here
		return false;
		}

	// This code scans for an arbitrary permutation that satisifies a pairwise constraint.
	size_t ScanPoint = StarRays-1;
	VertexEnumeration[StarRays-1] = strict_ub-1;
	do	{
		bool Failed = false;
		if (   i==VertexEnumeration[ScanPoint]
			|| ExplicitNoEdge(i,VertexEnumeration[ScanPoint]))
			Failed = true;

		if (!Failed)
			{
			size_t Validate = StarRays;
			while(ScanPoint<--Validate)
				if (ExplicitNoEdge(VertexEnumeration[ScanPoint],VertexEnumeration[Validate]))
					{
					Failed = true;
					break;
					}
			}

		if (Failed)
			{
			while(nonstrict_lb+ScanPoint==VertexEnumeration[ScanPoint])
				if (++ScanPoint==StarRays) return false;
			VertexEnumeration[ScanPoint]--;
			}
		else if (0<ScanPoint)
			{
			ScanPoint--;
			VertexEnumeration[ScanPoint] = VertexEnumeration[ScanPoint+1]-1;
			}
		else return true;
		}
	while(true);
	// should never get here
	return false;
}

// Key properties of CompleteStrictGraphSquare:
// * If vertices V1,V2,V3,V4 are connected as:
// ** ExplicitEdge V1-V2,V1-V3,V2-V4,V3-V4,
// ** ExplicitNoEdge V1-V3,V2-V4
// ** presenting V1,V2,V3 will return true and V4; similarly for other three viewpoint vertices
// ** Idx2,Idx3 commute
// ** Idx1/Idx2 and Idx1/Idx3 are anti-symmetric
// ** If ExplicitEdge V1-V2,V1-V3 and CompleteStrictGraphSquare fails (no RAM problems), then 
//    CompleteStrictGraphSquare categorically fails for V2,V3 for all V1
bool
Digraph::CompleteStrictGraphSquare(size_t Idx, size_t Idx2, size_t Idx3, size_t& Idx4) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2003
	assert(ExplicitEdge(Idx ,Idx2));
	assert(ExplicitEdge(Idx ,Idx3));
	assert(ExplicitNoEdge(Idx2,Idx3));
	return CompleteStrictGraphSquareCore(Idx,Idx2,Idx3,Idx4,0,fast_size());
}

bool
Digraph::CompleteStrictGraphSquareCore(const size_t Idx, const size_t Idx2, const size_t Idx3, size_t& Idx4, const size_t nonstrict_lb, const size_t strict_ub) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/21/2003
	size_t i = strict_ub;
	do	if (   --i!=Idx && i!=Idx2 && i!=Idx3 && ExplicitEdge(i,Idx2)
			&& ExplicitEdge(i,Idx3) && ExplicitNoEdge(i,Idx))
			{
			Idx4 = i;
			return true;
			}
	while(nonstrict_lb<i);
	return false;
}

// Towards this end, let's consider how to implement a convenient mapping:
// 0: viewpoint vertex
// 1..StarRays: distance 1
// We'd like the complementary vertex index to simply be
// POWER_OF_2(StarRays)-1-(current index)
// Use 3-d as a guide:
// 0
// 1	2 	3
// 23	13	12
// 123
// So the desired mapping is F(2,3)=0, F(1,3)=1, F(1,2)=2; use constant offset 4
// 4-d as a guide:
// 	0
//	1	2	3	4
//	34	24	14	23	13	12
//	234	134	124	123
//	1234
// Higher dimensions do not provide clearly useful heuristics.
// Desired mapping: F(3,4)=0, F(2,4)=1, F(1,4)=2, F(2,3)=3, F(1,3)=4, F(1,2)=5
// It seems that we need the codimension:
// F(1,0)=0, F(2,0)=1, F(2,1)=2
// F(1,0)=0, F(2,0)=1, F(3,0)=2, F(2,1)=3, F(3,1)=4, F(3,2)=5
// Correct the origin by subtracting 1 from the left codimension coordinate
// F(0,0)=0, F(1,0)=1, 			 F(1,1)=2
// F(0,0)=0, F(1,0)=1, F(2,0)=2, F(1,1)=3, F(2,1)=4, F(2,2)=5
// then F(X,0) = X
// then F(X,1) = X+(D-1)
// Correct the origin by subtracting 1+(right codimension coordinate) from the left codimension coordinate
// F(0,0)=0, F(1,0)=1, 					   F(0,1)=2
// F(0,0)=0, F(1,0)=1, F(2,0)=2,		   F(0,1)=3, F(1,1)=4,	 		 F(0,2)=5
// F(0,0)=0, F(1,0)=1, F(2,0)=2, F(3,0)=3, F(0,1)=4, F(1,1)=5, F(2,1)=6, F(0,2)=7
// Let TRIANGLE(K) be the K'th triangular number, K a positive integer.
// then F(X,0) = X
// then F(X,1) = X+(D-1) 		= X+TRIANGLE(D-1)-TRIANGLE(D-2)
// then F(X,2) = X+(D-1)+(D-2)	= X+TRIANGLE(D-1)-TRIANGLE(D-3)
// While a proper induction proof should be done, it appears that F should be defined as:
// X=D-LIdx-1
// Y=D-RIdx
// F(X,Y) = X+TRIANGLE(D-1)-TRIANGLE(D-1-Y)

#if 0
// 2-cycles: this is the inverse of lexical order.
// Invert it if useful
static size_t HypercubePermutationIndexArgRange1N(size_t Arg1,size_t Arg2,size_t Dimension)
{
	assert(1<=Arg1);
	assert(Arg1<Arg2);
	assert(Dimension>=Arg2);
	size_t X = Dimension-Arg1-1;
	size_t Y = Dimension-Arg2;
	Dimension--;

	X += (0==Dimension%2) ? (Dimension-1)*(Dimension/2)
						  : Dimension*((Dimension-1)/2);

	Dimension -= Y;
	return X+((0==Dimension%2) 	? (Dimension-1)*(Dimension/2)
							 	: Dimension*((Dimension-1)/2));
}
#endif

bool
Digraph::VertexEmbedsLocalHypercubeVertex(size_t* const SweepBand, const size_t StarRays, const size_t nonstrict_lb, const size_t strict_ub)
{	//! \todo IMPLEMENT
	// This routine imposes some requirements on the buffer SweepBand.
	// SweepBand[0] is the viewpoint index.
	// SweepBand[1]...SweepBand[StarRays] are used to hold the active permutation.
	// SweepBand[POWER_OF_2(StarRays)]...SweepBand[POWER_OF_2(StarRays)+StarRays+1] is used as a working buffer.
	// SweepBand[StarRays+1]...SweepBand[POWER_OF_2(StarRays)-1]  is intended to be used to hold auxilliary vertices.
	// We probably won't have to worry about 31-d hypercubes....really should be asking
	// the memory manager about maximum available memory.
	if (   31<StarRays
		|| !VertexEmbedsStrictNStar(SweepBand,StarRays,nonstrict_lb,strict_ub))
		return false;
	if (1==StarRays) return true;

	// NOTE: VertexEmbedsStrictNStar picks the earliest permutation in its sweep order
	// to initialize to...that is, SweepBand[StarRays] defines the practical strict_ub
	// for permutations.

	size_t i = SweepBand[0];
	size_t EffectiveEdgeLB = nonstrict_lb;
	size_t EffectiveEdgeStrictUB = SweepBand[StarRays]+1;

	// Adjust EffectiveEdgeLB for technical accuracy
	while(EffectiveEdgeLB==i || !ExplicitEdge(i,EffectiveEdgeLB))
		if (++EffectiveEdgeLB+StarRays>EffectiveEdgeStrictUB)
			return false;

	size_t EdgeCount = VertexUndirectedEdgeCountCore(i,EffectiveEdgeLB,EffectiveEdgeStrictUB);
	if (2==EdgeCount)
		{	// also know that 2==StarRays.  Don't bother with dynamic allocation
			// We also know there is no cross-connect, and that VertexEmbedsStrictNStar
			// has initialized SweepBand[1] and SweepBand[2].
		return CompleteStrictGraphSquareCore(SweepBand[0],SweepBand[1],SweepBand[2],SweepBand[3],nonstrict_lb,strict_ub);
		}

	// Currently can handle only StarRays 2 or less
	if (2<StarRays) return false;

	// enumerate edges explicitly.  This should run faster
	// because all of the ExplicitEdge(Idx,___) tests automatically succeed.
	size_t* const VertexIdxEnumeration = EnumerateFromEdgesNoLoops(i,EdgeCount,EffectiveEdgeLB,EffectiveEdgeStrictUB);
	if (VertexIdxEnumeration)
		{
		// we only want vertices that embed strict N-stars
		// We have a pre-filtering problem.  We need a 'safe space' to play in for 
		// VertexEmbedsStrictNStar.
		if (2==StarRays)
			{	// 2 edges that don't match OK
			size_t* const VertexEnumeration = SweepBand+1;

			VertexEnumeration[1] = ArraySize(VertexIdxEnumeration);
			do	{
				VertexEnumeration[0] = --VertexEnumeration[1];
				do	if (   ExplicitNoEdge(VertexIdxEnumeration[--VertexEnumeration[0]],VertexIdxEnumeration[VertexEnumeration[1]])
						&& CompleteStrictGraphSquareCore(SweepBand[0],VertexIdxEnumeration[VertexEnumeration[0]],VertexIdxEnumeration[VertexEnumeration[1]],SweepBand[3],nonstrict_lb,strict_ub))
						{
						VertexEnumeration[0] = VertexIdxEnumeration[VertexEnumeration[0]];
						VertexEnumeration[1] = VertexIdxEnumeration[VertexEnumeration[1]];
						free(VertexIdxEnumeration);
						return true;
						}
				while(0<VertexEnumeration[0]);
				}
			while(1<VertexEnumeration[1]);
			free(VertexIdxEnumeration);
			return false;
			}

		//! \todo port general permutation-stepping code from N-star

		free(VertexIdxEnumeration);
		return false;
		};

	// Low RAM situation
	if (2==StarRays)
		{	// 2 edges that don't match OK
		size_t* const VertexEnumeration = SweepBand+1;

		VertexEnumeration[1] = EffectiveEdgeStrictUB;
		do	if (   --VertexEnumeration[1]!=i
				&& ExplicitEdge(i,VertexEnumeration[1]))
				{
				VertexEnumeration[0] = VertexEnumeration[1];
				do	if (   --VertexEnumeration[0]!=i
						&& ExplicitEdge(VertexEnumeration[0],i)
						&& ExplicitNoEdge(VertexEnumeration[0],VertexEnumeration[1])
						&& CompleteStrictGraphSquareCore(i,VertexEnumeration[0],VertexEnumeration[1],SweepBand[3],nonstrict_lb,strict_ub))
						return true;
				while(EffectiveEdgeLB<VertexEnumeration[0]);
				}
		while(EffectiveEdgeLB+1<VertexEnumeration[1]);
		return false;
		};

	//! \todo port general permutation-stepping code from N-star
	return false;
}

// A strict hypercube is considered to be one with no extraneous edges within its vertex set.
bool Digraph::FindStrictHypercubeGraphDimension(const size_t Dimension)
{	//! \todo IMPLEMENT
	// right now, we can't handle anything above 2
	// don't let this cutoff go above 31
	if (2<Dimension) return false;

	if (size()<POWER_OF_2(Dimension)) return false;
	if (0==Dimension) return true;

	if (1==Dimension)
		{
		size_t SweepBand[2];
		SweepBand[0] = 0;
		while(!VertexEmbedsStrictNStar(SweepBand,1,SweepBand[0],fast_size()))
			if (++SweepBand[0]+2>fast_size())
				return false;
		return true;
		};
	// precondition: all vertices are used by at least one edge
	if (2==Dimension)
		{
		// 4 to hold index 4-tuple
		// but scan buffer wants 6
		size_t SweepBand[7];
		SweepBand[0] = 0;
		while(!VertexEmbedsLocalHypercubeVertex(SweepBand,2,SweepBand[0],fast_size()))
			if (++SweepBand[0]+4>fast_size())
				return false;
		return true;
		}

#if 0
	bool UBChanged = false;
	do	{
		UBChanged = false;
		while(!VertexEmbedsLocalHypercubeVertex(NonStrictLB,Dimension,NonStrictLB,NonStrictUB+1))
			if (++NonStrictLB==NonStrictUB)
				return false;

		if (!VertexEmbedsLocalHypercubeVertex(NonStrictUB,Dimension,NonStrictLB,NonStrictUB+1))
			{
			UBChanged = true;
			if (NonStrictLB==--NonStrictUB) return false;
			while(!VertexEmbedsLocalHypercubeVertex(NonStrictUB,Dimension,NonStrictLB,NonStrictUB+1))
				if (NonStrictLB==--NonStrictUB) return false;
			}
		}
	// validate the lower bound before leaving the loop
	while(UBChanged && !VertexEmbedsLocalHypercubeVertex(NonStrictLB,Dimension,NonStrictLB,NonStrictUB+1));

	size_t TargetIndirectorSize = NonStrictUB-NonStrictLB+1;
	size_t IndirectorSize = TargetIndirectorSize;

	if (TargetIndirectorSize<POWER_OF_2(Dimension)) return false;

	// Alas, we have several approaches.
	size_t* ArgArrayIndirector = _new_buffer<size_t>(IndirectorSize);
	if (ArgArrayIndirector)
		{	// have enough space for something....
		size_t CandidateCount = 1;
		size_t Offset = 1;
		size_t Tolerance = (NonStrictUB-NonStrictLB+1)-PowerOf2[Dimension]+1;
		ArgArrayIndirector[0] = NonStrictLB;
		while(NonStrictLB+Offset<NonStrictUB)
			if (VertexEmbedsStrictNStarConnectedToEmbeddedStrictNStars(NonStrictLB+Offset++,Dimension,1))
				ArgArrayIndirector[CandidateCount++] = NonStrictLB+Offset-1;
			else if (0==--Tolerance)
				{
				free(NewArgArray);
				return false;
				}
		ArgArrayIndirector[CandidateCount++] = NonStrictUB;
		if (CandidateCount<NonStrictUB-NonStrictLB+1)
			NewArgArray = REALLOC(NewArgArray,CandidateCount*sizeof(MetaConcept*));

		// OK...enough candidates left		
		}

	if (CandidateCount<POWER_OF_2(Dimension))
		return false;

	//! \todo rest of code
#endif

	return false;
}

// Returns dimension of largest hypercube indexed
size_t Digraph::FindStrictHypercubeGraphFlushIrrelevantVertices()
{	//! \todo IMPLEMENT
	FlushVerticesWithUndirectedEdgeCountLTE(0);
	if (ArgArray.empty()) return 0;
	if (!FindStrictHypercubeGraphDimension(2)) return 1;
	FlushVerticesNotInStrictSquare();
#if 1
	return 2;
#else
	// want to support higher values first
	while(FindStrictHypercubeGraphDimension(InferenceParameter1+1))
		{
		FlushVerticesWithUndirectedEdgeCountLTE(InferenceParameter1);
		InferenceParameter1++;
		}
#endif
}

size_t Digraph::FindCompleteGraphFlushIrrelevantVertices()
{	//! \todo IMPLEMENT
	FlushVerticesWithUndirectedEdgeCountLTE(0);
	if (ArgArray.empty()) return 0;
#if 1
	return 2;
#else
	if (!FindCompleteGraphDimension(2))
		return 2;
	FlushVerticesNotInCompleteGraph(2);
	return 3;
#endif
}

// Externally useful functions
void Digraph::InitForClauseAmplification()
{	// FORMALLY CORRECT: Kenneth Boyd, 2/13/2005
	size_t i = fast_size();
	do	{	// we already screened out the logical and of these.
		if (!ArgArray[--i]->CanAmplifyClause()) ResetFromEdges(i);
		else if (!ArgArray[i]->WantToBeAmplified()) ResetToEdges(i);
		}
	while(0<i);
}

void Digraph::MaxMinArityForAmplication(size_t& i, size_t& ToEdgeCount)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/17/2005
	size_t j = i;
	size_t RHSArity = ArgArray[i]->size();
	size_t LHSArity;
	while(0<j)
		{
		size_t AltToEdgeCount = VertexToEdgeCount(--j);
		if (	0<AltToEdgeCount
			&&  (   (LHSArity = ArgArray[j]->size())<RHSArity
				 || (	AltToEdgeCount>ToEdgeCount && LHSArity==RHSArity)))
			{
			i = j;
			ToEdgeCount = AltToEdgeCount;
			RHSArity = LHSArity;
			};
		};
}

#if 0
void
Digraph::GenerateIFFClauseList(MetaConcept**& ClauseList,const MetaConcept** const ArgArray)	// generates IFF clauses not implied by prior information
{	//! \todo IMPLEMENT
	// ASSUMPTION: Digraph has been initialized with fragmentary implies based on ArgArray, which is from a LogicalAND
}

void
Digraph::GenerateXORClauseList(MetaConcept**& ClauseList,const MetaConcept** const ArgArray)	// generates XOR clauses not implied by prior information
{	//! \todo IMPLEMENT
	// ASSUMPTION: Digraph has been initialized with fragmentary implies based on ArgArray, which is from a LogicalAND
}
#endif

