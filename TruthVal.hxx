// TruthVal.hxx
// TruthValue header

#ifndef TRUTHVAL_HXX
#define TRUTHVAL_HXX 1

#include "MetaCon7.hxx"
#include "TVal.hxx"

template<>
struct MetaConcept_lookup<TVal>
{
	enum {
		return_code = 1	// &TruthValues
	};
	
	// always need these five
	static ExactType_MC exact_type() {return TruthValue_MC;}
	static bool syntax_ok(const TVal& x) {return true;};
	static std::string to_s_aux(const TVal& x) { return x.to_s(); }
#ifndef USE_TO_S
	static size_t length_of_self_name(const TVal& x) {return x.LengthOfSelfName();};
	static void construct_self_name_aux(char* dest,const TVal& x) {x.ConstructSelfNameAux(dest);};
#endif
	static bool lt_aux(const TVal& lhs, const TVal& rhs) {return lhs<rhs;};
	static bool read(TVal& dest, const char* src) {return dest.read(src);}

	// these two needed for ultimate type &TruthValues
	static void SelfLogicalNOT(TVal& x) {x.SelfLogicalNOT();};
	static bool isAntiIdempotentTo(const TVal& lhs,const TVal& rhs) {return lhs.isAntiIdempotentTo(rhs);};
};

typedef MetaConceptExternal<TVal> TruthValue;

template<auto tval>
bool ForceTruth(MetaConcept*&)
{
	return new TruthValue(TVal(tval));
}

#endif
