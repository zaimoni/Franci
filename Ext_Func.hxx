// Ext_Func.hxx

// Defining functions by TSV:

// Line 1: Arity, argument types
// Line 2: blank for arity columns, then new reserved words as prefix functions
// next n lines: enumerated domain-value pairs (one value per reserved-word column)

#ifndef EXTERNAL_FUNCTION_DEF
#define EXTERNAL_FUNCTION_DEF

#include "Zaimoni.STL/math/relation.hpp"
#include "MetaCon2.hxx"

class ExternalFunction;
namespace zaimoni {

template<>
struct is_polymorphic_final<ExternalFunction> : public std::true_type {};

}

class ExternalFunction : public MetaConceptWithArgArray
{
private:
	// note: _range "owns" the master copy of the strings
	// reserved-word to return type (function)
	static zaimoni::math::relation<const char*, autoval_ptr<AbstractClass> > _range;
	// reserved-word to argument types (function)
	static zaimoni::math::relation<char*, autovalarray_ptr<AbstractClass*> > _domain;
	// reserved-word to enumerated definition (function)
	static zaimoni::math::relation<char*, zaimoni::math::relation<autovalarray_ptr<MetaConcept*>, autoval_ptr<MetaConcept> > > _definition;

	const char* ExternalKeyword;	// this controls the intended semantics
									// ExternalFunction does *NOT* own this!

	AbstractClass* _UltimateType;							// cache: want to use static data only during creation

public:
	ExternalFunction(const char* NewExternalKeyword);
	ExternalFunction(const char* NewExternalKeyword,MetaConcept**& NewArgList);
//	ExternalFunction(const ExternalFunction& Source);	// default OK
	virtual ~ExternalFunction();

	const ExternalFunction& operator=(const ExternalFunction& Source);
	virtual void CopyInto(MetaConcept*& Target) const {CopyInto_ForceSyntaxOK(*this,Target);};	// can throw memory failure
	void CopyInto(ExternalFunction*& Target) const {CopyInto_ForceSyntaxOK(*this,Target);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& Target) {zaimoni::MoveInto(*this,Target);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(ExternalFunction*& Target);	// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
	virtual const AbstractClass* UltimateType() const {return _UltimateType;};
//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName(void) const;

	static bool LoadFunctionsFromFile(const char* Filename);
	static bool UnloadFunction(const char* FuncName);
	static bool UnloadAllFunctions(void);
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	virtual void _forceStdForm();

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;

	void InitCachedDataFromKeyword(void);
};

#endif
