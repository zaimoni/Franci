// MetaCon7.hxx
// header for MetaConceptExternal

#ifndef METACONCEPT_EXTERNAL
#define METACONCEPT_EXTERNAL

#include "MetaCon1.hxx"
#include "Zaimoni.STL/Logging.h"

// master for these is Class.hxx
class AbstractClass;
extern const AbstractClass TruthValues;

template<class T>
struct MetaConcept_lookup
{	// 
	// enum {
	// 	return_code = ...;
	//  return_code = 1	// &TruthValues
	// }

	// always need these five
	// static ExactType_MC exact_type();
	// static bool syntax_ok(const T& x);
	// static size_t length_of_self_name(const T& x)
	// static void construct_self_name_aux(char* dest,const T& x)
	// static void lt_aux(const T& lhs, const T& rhs)
	// static bool read(T& dest, const char* src)

	// these two needed for ultimate type &TruthValues
	// static void SelfLogicalNOT(T& x);
	// static bool isAntiIdempotentTo(const T& lhs,const T& rhs);
};

template<class T> class MetaConceptExternal;

namespace zaimoni {

template<class T>
struct is_polymorphic_final<MetaConceptExternal<T> > : public std::true_type {};

}

// we'd like to replace most derivations from MetaConceptZeroAry (MetaCon6.hxx/cxx) with this
template<class T>
class MetaConceptExternal final : public MetaConcept
{
public:
	T _x;	// we would provide full accessors anyway so may as well be public

	MetaConceptExternal() noexcept(std::is_nothrow_default_constructible_v<T>) : MetaConcept(MetaConcept_lookup<T>::exact_type()) {}
	MetaConceptExternal(const T& src) noexcept(std::is_nothrow_copy_constructible_v<T>) : MetaConcept(MetaConcept_lookup<T>::exact_type()),_x(src) {}
	MetaConceptExternal(const MetaConceptExternal& src) = default;
	MetaConceptExternal(MetaConceptExternal&& src) = default;
	virtual ~MetaConceptExternal() {}	// as a template doesn't go in Destruct.cxx
	MetaConceptExternal& operator=(const MetaConceptExternal& src) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		_x = src._x;	// presumably this is ACID
		MetaConcept::operator=(src);
	};
	MetaConceptExternal& operator=(MetaConceptExternal&& src) = default;
	MetaConceptExternal& operator=(const T& src) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		_x = src;	// presumably this is ACID
	};

	void CopyInto(MetaConcept*& dest) const override {zaimoni::CopyInto(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(MetaConceptExternal*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	virtual typename std::enable_if<MetaConcept_lookup<T>::return_code==1,const AbstractClass*>::type UltimateType() const {return &TruthValues;};	// specialize or else
//	Arity functions
	virtual size_t size() const {return 0;};
	virtual const MetaConcept* ArgN(size_t n) const {return 0;};
	virtual MetaConcept* ArgN(size_t n) {return 0;};
// Syntactical equality and inequality
	virtual bool IsAbstractClassDomain() const {return true;};
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override { return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >(); }
	bool CanEvaluate() const override { return false; }
	bool CanEvaluateToSameType() const override { return false; }
	virtual bool SyntaxOK() const {return MetaConcept_lookup<T>::syntax_ok(_x);};	// actually should do a syntax check on its arg
	virtual bool Evaluate(MetaConcept*& dest) {return false;};		// same, or different type
	virtual bool DestructiveEvaluateToSameType() {return false;};	// overwrites itself iff returns true
// text I/O functions
	virtual size_t LengthOfSelfName() const {return MetaConcept_lookup<T>::length_of_self_name(_x);};
	static bool read(MetaConcept*& dest,const char* Text)
	{
		assert(!dest);
		T tmp;
		SUCCEED_OR_DIE(MetaConcept_lookup<T>::read(tmp,Text));
		dest = new(nothrow) MetaConceptExternal(tmp);
		return dest;
	}
// Formal manipulation functions
	virtual typename std::enable_if<MetaConcept_lookup<T>::return_code==1,void>::type SelfLogicalNOT() {MetaConcept_lookup<T>::SelfLogicalNOT(this->_x);};
	virtual void ConvertVariableToCurrentQuantification(MetaQuantifier& src) {};
	virtual bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const {return false;};
	virtual bool UsesQuantifierAux(const MetaQuantifier& x) const {return false;};
	virtual bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) {return true;};
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const {return _x==static_cast<const MetaConceptExternal&>(rhs)._x;};
	virtual bool InternalDataLTAux(const MetaConcept& rhs) const {return MetaConcept_lookup<T>::lt_aux(_x,static_cast<const MetaConceptExternal<T>&>(rhs)._x);};
	virtual void ConstructSelfNameAux(char* Name) const {return MetaConcept_lookup<T>::construct_self_name_aux(Name,_x);};		// overwrites what is already there
	void _forceStdForm() override {};
	virtual bool _IsExplicitConstant() const {return true;};
private:
	virtual typename std::enable_if<MetaConcept_lookup<T>::return_code==1,bool>::type isAntiIdempotentTo(const MetaConcept& rhs) const {return MetaConcept_lookup<T>::isAntiIdempotentTo(this->_x,static_cast<const MetaConceptExternal&>(rhs)._x);};
};

#endif
