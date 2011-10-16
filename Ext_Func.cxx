// Ext_Func.cxx

// Defining functions by TSV:
// Line 1: Arity, argument types
// Line 2: blank for arity columns, then new reserved words as prefix functions
// next n lines: enumerated domain-value pairs (one value per reserved-word column)
// later versions will support more sophisticated formats.

#include "Ext_Func.hxx"
#include "Class.hxx"
#include "CSVTable.hxx"
#include "InParse.hxx"

#include "Zaimoni.STL/except/syntax_error.hpp"
#include "Zaimoni.STL/string.h"

// defined in LexParse.cxx
bool InitMetaConceptParserArray(autoarray_ptr<MetaConcept*>& ArgArray,char*& InputBuffer);

// test driver
#define ExternalFunction_MC Unknown_MC

ExternalFunction::ExternalFunction(const char* NewExternalKeyword)
	:	MetaConceptWithArgArray(ExternalFunction_MC),	//! \bug ExternalFunction_MC needs to be in the type enumeration
		ExternalKeyword(NewExternalKeyword),
		_UltimateType(NULL)
{	// constructor must initialize _UltimateType,_TypeSpec from static data
	InitCachedDataFromKeyword();
}

ExternalFunction::ExternalFunction(const char* NewExternalKeyword,MetaConcept**& NewArgList)
	:	MetaConceptWithArgArray(ExternalFunction_MC,NewArgList),
		ExternalKeyword(NewExternalKeyword),
		_UltimateType(NULL)
{	// constructor must initialize _UltimateType,_TypeSpec from static data
	InitCachedDataFromKeyword();
}

ExternalFunction::~ExternalFunction()
{	//! \todo move to Destruct.cxx
}

const ExternalFunction&
ExternalFunction::operator=(const ExternalFunction& Source)
{	//! FORMALLY CORRECT: 5/16/2006
	MetaConceptWithArgArray::operator=(Source);

	ExternalKeyword = Source.ExternalKeyword;
	_UltimateType = Source._UltimateType;
	
	return *this;
}

void
ExternalFunction::MoveInto(ExternalFunction*& Target)
{	//! FORMALLY CORRECT: 5/16/2006
	if (NULL==Target)
		{
		Target = new ExternalFunction(ExternalKeyword);
		};
	MoveIntoAux(*Target);
}

void
ExternalFunction::ForceStdForm(void)
{	//! \todo make more configurable
	ForceStdFormAux();
}

bool ExternalFunction::SyntaxOK() const
{	//! FORMALLY CORRECT: 5/16/2006
	if (NULL==ExternalKeyword) return false;
	if (!SyntaxOKAux()) return false;
	size_t i = _domain.size();
	while(0<i)
		if (0==strcmp(ExternalKeyword,_domain.domain_domain_order(--i)))
			{
			autovalarray_ptr<AbstractClass*>& _TypeSpec = _domain.range_domain_order(i);
			if (size()!=_TypeSpec.size()) return false;
			i = size();
			do	if (NULL!=_TypeSpec[--i])
					{
					if (NULL==ArgArray[i]->UltimateType()) return false;
					if (!_TypeSpec[i]->Superclass(*ArgArray[i]->UltimateType())) return false;
					}
			while(0<i);
			return true;
			};
	return false;
}

// text I/O functions
size_t
ExternalFunction::LengthOfSelfName(void) const
{	//! \todo move to LenName.cxx
	return strlen((NULL==ExternalKeyword) ? "(unnamed external function)" : ExternalKeyword)+LengthOfPrefixArgList();
}

void
ExternalFunction::ConstructSelfNameAux(char* Name) const
{	//! \todo move to LenName.cxx
	if (NULL==ExternalKeyword)
		{
		strcpy(Name,"(unnamed external function)");
		ConstructPrefixArgList(Name+strlen("(unnamed external function)"));
		}
	else{
		strcpy(Name,ExternalKeyword);
		ConstructPrefixArgList(Name+strlen(ExternalKeyword));
		}
}

void ExternalFunction::DiagnoseInferenceRules() const
{
	if (NULL!=ExternalKeyword)
		{
		size_t Idx = _definition.size();
		if (0<Idx)
			{
			do	if (0==strcmp(ExternalKeyword,_definition.domain_domain_order(--Idx)))
					{
					zaimoni::math::relation<autovalarray_ptr<MetaConcept*>, autoval_ptr<MetaConcept> >& function_def = _definition.range_domain_order(Idx);
					Idx = function_def.size();
					do	{
						const std::pair<autovalarray_ptr<MetaConcept*>, autoval_ptr<MetaConcept> >& candidate = function_def.domain_order_element(--Idx);
						if (candidate.first==ArgArray)
							{
							try	{
								candidate.second->CopyInto(InferenceParameterMC);
								}
							catch(const bad_alloc&)
								{
								UnconditionalRAMFailure();
								}
							IdxCurrentEvalRule = ForceValue_ER;
							return;
							}
						}
					while(0<Idx);
					break;
					}
			while(0<Idx);
			}
		}
	IdxCurrentSelfEvalRule=SelfEvalSyntaxOKNoRules_SER;
}

bool ExternalFunction::InvokeEqualArgRule() const
{	//! \todo hook into external definition
	return false;
}

void
ExternalFunction::InitCachedDataFromKeyword(void)
{
	size_t Idx = _range.size();
	if (0<Idx)
		do	if (0==strcmp(ExternalKeyword,_range.domain_domain_order(--Idx)))
				{	// match: use internal keyword
				_UltimateType = _range.range_domain_order(Idx);
				ExternalKeyword = _range.domain_domain_order(Idx);
				return;
				}
		while(0<Idx);
}

bool
ExternalFunction::LoadFunctionsFromFile(const char* const Filename)
{
	VERIFY(is_empty_string(Filename),AlphaCallAssumption);

	CSVTable FunctionReference('\t','\x00',true);
	FunctionReference.RAMTable(Filename);

	// Line 1: cell 0 is Arity of all functions (parses with atol)
	// cells 1..n are types (parses as Franci classes)
	const size_t Arity = atol(FunctionReference.CellText(0,(size_t)0));
	if (0>=Arity) return false;
	autovalarray_ptr<AbstractClass*> ParameterTypes(Arity);

	size_t Idx = Arity;
	do	{
		autoarray_ptr<MetaConcept*> TestItem(1);
		{
		autoarray_ptr<char> Tmp;
		FunctionReference.CellText(0,Idx--,Tmp);
		if (!InitMetaConceptParserArray(TestItem,Tmp)) return false;
		}
		// interpret TestItem as a Franci expression
		try	{
			while(InterpretOneStage(TestItem));
			}
		catch(const syntax_error& e)
			{
			FranciDialog.SaysError(e.what());
			const size_t StrictUB = TestItem.size();
			size_t Idx = 0;
			do	if (NULL==TestItem[Idx])
					INFORM("NULL");
				else
					INFORM(*TestItem[Idx]);
			while(StrictUB> ++Idx);
			return false;
			};
		// then check type of Tmp (fail if not AbstractClass)
		if (   1!=TestItem.size()
			|| NULL==TestItem[0]
			|| !TestItem[0]->SyntaxOK()
			|| !TestItem[0]->IsExactType(AbstractClass_MC))
			return false;
		// then install as AbstractClass
		ParameterTypes[Idx] = reinterpret_cast<AbstractClass*>(TestItem[0]);
		TestItem[0] = NULL;
		}
	while(0<Idx);
	FunctionReference.DeleteRow(0);

	// next: proceed to get names of prefix functions being defined
	// first Arity columns are domain
	const size_t Columns = FunctionReference.ColumnCount();
	const size_t Rows = FunctionReference.RowCount();
	if (Columns<=Arity) return false;
	if (1>=Rows) return false;

	const size_t FunctionIndexes = Columns-Arity;
	autoarray_ptr<char*> FunctionNames(FunctionIndexes);
	Idx = FunctionIndexes;
	do	{
		--Idx;
		FunctionReference.CellText(0,Arity+Idx,FunctionNames[Idx]);
		}
	while(0<Idx);

	// get domain instances at this time, then flush from table
	autoarray_ptr<autovalarray_ptr<MetaConcept*> > DomainInstances;
	Idx = Rows;
	do	{
		--Idx;
		if (!DomainInstances[Idx-1].Resize(Arity)) return false;
		size_t Idx2 = Arity;
		do	{
			autoarray_ptr<MetaConcept*> TestItem(1);
			{
			autoarray_ptr<char> Tmp;
			FunctionReference.CellText(Idx,--Idx2,Tmp);
			DEBUG_FAIL(NULL==ParameterTypes[Idx2]);	// data integrity
			if (!InitMetaConceptParserArray(TestItem,Tmp)) return false;
			}
			// interpret ArgArray as a Franci expression
			try	{
				while(InterpretOneStage(TestItem));
				}
			catch(const syntax_error& e)
				{
				FranciDialog.SaysError(e.what());
				const size_t StrictUB = TestItem.size();
				size_t Idx = 0;
				do	if (NULL==TestItem[Idx])
						INFORM("NULL");
					else
						INFORM(*TestItem[Idx]);
				while(StrictUB> ++Idx);
				return false;
				};
			// then type-check
			if (   1!=TestItem.size()
				|| NULL==TestItem[0]
				|| !TestItem[0]->SyntaxOK()
				|| !ParameterTypes[Idx2]->HasAsElement(*TestItem[0]))
				return false;
			// if type OK, accept
			DomainInstances[Idx-1][Idx2] = TestItem[0];
			TestItem[0] = NULL;
			}
		while(0<Idx2);
		}
	while(1<Idx);
	Idx = Arity;
	do	FunctionReference.DiscardColumn((size_t)0);
	while(0<--Idx);

	// init functions 
	Idx = FunctionIndexes;
	do	{
		--Idx;
		// init domain of function
		_domain.Enumerate(FunctionNames[Idx],ParameterTypes);
		autodel_ptr<AbstractClass> Supremum;
		Supremum = new AbstractClass(NULLSet);

		// init each function's enumeration and track the "supremum UltimateType", then init range
		zaimoni::math::relation<autovalarray_ptr<MetaConcept*>, autoval_ptr<MetaConcept> > TmpDef;
		size_t Idx2 = Rows;
		do	{
			--Idx2;
			autoarray_ptr<MetaConcept*> TestItem(1);
			{
			autoarray_ptr<char> Tmp;
			FunctionReference.CellText(--Idx2,Idx,Tmp);
			if (!InitMetaConceptParserArray(TestItem,Tmp)) return false;
			}
			// interpret ArgArray as a Franci expression
			try	{
				while(InterpretOneStage(TestItem));
				}
			catch(const syntax_error& e)
				{
				FranciDialog.SaysError(e.what());
				const size_t StrictUB = TestItem.size();
				size_t Idx = 0;
				do	if (NULL==TestItem[Idx])
						INFORM("NULL");
					else
						INFORM(*TestItem[Idx]);
				while(StrictUB> ++Idx);
				return false;
				};
			// then type-check
			if (   1!=TestItem.size()
				|| NULL==TestItem[0]
				|| !TestItem[0]->SyntaxOK())
				return false;
			// if type OK, accept
			if (NULL!=Supremum)
				{
				if (   NULL==TestItem[0]->UltimateType()
					|| !Supremum->UnionWith(*TestItem[0]->UltimateType()))
					Supremum.reset();
				};
			autoval_ptr<MetaConcept> Test(TestItem[0]);
			TestItem.clear();
			TmpDef.Enumerate(DomainInstances[Idx],Test);
			}
		while(1<Idx2);
		_definition.Enumerate(FunctionNames[Idx],TmpDef);

		// then flush from table
		FunctionReference.DiscardColumn(Idx);
		}
	while(0<Idx);
	return true;
}

bool
ExternalFunction::UnloadFunction(const char* FuncName)
{
	DEBUG_FAIL_OR_LEAVE(is_empty_string(FuncName),return false);
	size_t Idx = _range.size();
	if (0<Idx)
		{
		do	if (0==strcmp(FuncName,_range.domain_domain_order(--Idx)))
				{
				char* Tmp = const_cast<char*>(_range.domain_domain_order(Idx));
				_range.clear_Domain(Tmp);
				_domain.clear_Domain(Tmp);
				_definition.clear_Domain(Tmp);
				free(Tmp);
				}
		while(0<Idx);
		}
	return false;
}

bool
ExternalFunction::UnloadAllFunctions(void)
{
	size_t Idx = _range.size();
	if (0<Idx)
		do	free(const_cast<char*>(_range.domain_domain_order(--Idx)));
		while(0<Idx);
	_range.clear();
	_domain.clear();
	_definition.clear();
}

