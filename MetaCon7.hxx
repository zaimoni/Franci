// MetaCon7.hxx
// header for MetaConceptExternal

#ifndef METACONCEPT_EXTERNAL
#define METACONCEPT_EXTERNAL

#include "MetaCon1.hxx"

template<class T>
struct MetaConcept_lookup
{	// 
	// exact_type: _MC value
	// static AbstractClass* ultimate_type(): 
	// static bool syntax_ok(const T& x);
	// static size_t length_of_self_name(const T& x)
	// static void construct_self_name_aux(char* dest,const T& x)
	// static void lt_aux(const T& lhs, const T& rhs)
};

template<class T> class MetaConceptExternal;

namespace zaimoni {

template<class T>
struct is_polymorphic_final<MetaConceptExternal<T> > : public std::true_type {};

}

// we'd like to replace most derivations from MetaConceptZeroAry (MetaCon6.hxx/cxx) with this
template<class T>
class MetaConceptExternal : public MetaConcept
{
public:
	T _x;	// we would provide full accessors anyway so may as well be public

	MetaConceptExternal(const T& src) : MetaConcept(MetaConcept_lookup<T>::exact_type),_x(src) {};
	MetaConceptExternal(const MetaConceptExternal& src)
		: MetaConcept(src),_x(src._x) {};
	virtual ~MetaConceptExternal() {};	// as a template doesn't go in Destruct.cxx
	void operator=(const MetaConceptExternal& src) {
		_x = src._x;	// presumably this is ACID
		MetaConcept::operator=(src);
	};
	void operator=(const T& src) {
		_x = src;	// presumably this is ACID
	};

	virtual void CopyInto(MetaConcept*& dest) const {zaimoni::CopyInto(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(MetaConceptExternal*& dest) {zaimoni::CopyInto(*this,dest);};

//  Type ID functions
	virtual const AbstractClass* UltimateType() const {return MetaConcept_lookup<T>::ultimate_type();};	// specialize or else
//	Arity functions
	virtual size_t size() const {return 0;};
	virtual const MetaConcept* ArgN(size_t n) const {return NULL;};
	virtual MetaConcept* ArgN(size_t n) {return NULL;};
// Syntactical equality and inequality
	virtual bool IsAbstractClassDomain() const {return true;};
//  Evaluation functions
	virtual bool CanEvaluate() const {return false;};
	virtual bool CanEvaluateToSameType() const {return false;};
	virtual bool SyntaxOK() const {return MetaConcept_lookup<T>::syntax_ok(_x);};	// actually should do a syntax check on its arg
	virtual bool Evaluate(MetaConcept*& dest) {return false;};		// same, or different type
	virtual bool DestructiveEvaluateToSameType() {return false;};	// overwrites itself iff returns true
// text I/O functions
	virtual size_t LengthOfSelfName() const {return MetaConcept_lookup<T>::length_of_self_name(_x);};
// Formal manipulation functions
	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src) {};
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const {return false;};
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const {return false;};
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) {return true;};
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const {return _x==static_cast<const MetaConceptExternal&>(rhs)._x;};
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const {return MetaConcept_lookup<T>::lt_aux(_x,static_cast<const MetaConceptExternal<T>&>(rhs)._x);};
	virtual void ConstructSelfNameAux(char* Name) const {return MetaConcept_lookup<T>::construct_self_name_aux(Name,_x);};		// overwrites what is already there
	virtual void _forceStdForm() {};
	virtual bool _IsExplicitConstant() const {return true;};
};

#endif
