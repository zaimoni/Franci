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
	virtual ~MetaConceptExternal() = default;	// as a template doesn't go in Destruct.cxx
	MetaConceptExternal& operator=(const MetaConceptExternal& src) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		_x = src._x;	// presumably this is ACID
		MetaConcept::operator=(src);
		return *this;
	};
	MetaConceptExternal& operator=(MetaConceptExternal&& src) = default;
	MetaConceptExternal& operator=(const T& src) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		_x = src;	// presumably this is ACID
		return *this;
	};

	void CopyInto(MetaConcept*& dest) const override {zaimoni::CopyInto(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(MetaConceptExternal*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	std::enable_if_t<MetaConcept_lookup<T>::return_code==1,const AbstractClass*> UltimateType() const override {return &TruthValues;};	// specialize or else
//	Arity functions
	size_t size() const final {return 0;}
	const MetaConcept* ArgN(size_t n) const final {return nullptr;}
// Syntactical equality and inequality
	bool IsAbstractClassDomain() const override { return true; }
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override { return std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> >(); }
	bool CanEvaluate() const override { return false; }
	bool CanEvaluateToSameType() const override { return false; }
	bool SyntaxOK() const override {return MetaConcept_lookup<T>::syntax_ok(_x);}	// actually should do a syntax check on its arg
	bool Evaluate(MetaConcept*& dest) override {return false;}		// same, or different type
	bool DestructiveEvaluateToSameType() override {return false;}	// overwrites itself iff returns true
// text I/O functions
	static bool read(MetaConcept*& dest,const char* Text)
	{
		assert(!dest);
		T tmp;
		SUCCEED_OR_DIE(MetaConcept_lookup<T>::read(tmp,Text));
		dest = new(nothrow) MetaConceptExternal(tmp);
		return dest;
	}
// Formal manipulation functions
	std::enable_if_t<MetaConcept_lookup<T>::return_code==1,void> SelfLogicalNOT() override {MetaConcept_lookup<T>::SelfLogicalNOT(this->_x);};
	void ConvertVariableToCurrentQuantification(MetaQuantifier& src) override {}
	bool HasArgRelatedToThisConceptBy(const MetaConcept& Target, LowLevelBinaryRelation* TargetRelation) const override {return false;}
	bool UsesQuantifierAux(const MetaQuantifier& x) const override {return false;}
	bool ModifyArgWithRHSInducedActionWhenLHSRelatedToArg(const MetaConcept& lhs, const MetaConcept& rhs, LowLevelAction* RHSInducedActionOnArg, LowLevelBinaryRelation* TargetRelation) override {return true;}

protected:
	bool EqualAux2(const MetaConcept& rhs) const override {return _x==static_cast<const MetaConceptExternal&>(rhs)._x;}
	bool InternalDataLTAux(const MetaConcept& rhs) const override {return MetaConcept_lookup<T>::lt_aux(_x,static_cast<const MetaConceptExternal<T>&>(rhs)._x);}
	std::string to_s_aux() const override { return MetaConcept_lookup<T>::to_s_aux(_x); }
	void _forceStdForm() override {}
	bool _IsExplicitConstant() const override { return true; }

private:
	std::enable_if_t<MetaConcept_lookup<T>::return_code==1,bool> isAntiIdempotentTo(const MetaConcept& rhs) const override {return MetaConcept_lookup<T>::isAntiIdempotentTo(this->_x,static_cast<const MetaConceptExternal&>(rhs)._x);}
};

#endif
