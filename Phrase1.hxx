// Phrase1.hxx
// header for Phrase1Arg, the metatype for 1-ary phrases

#ifndef PHRASE_1ARG_DEF
#define PHRASE_1ARG_DEF

#include "MetaCon5.hxx"

class Phrase1Arg;
namespace zaimoni {

template<>
struct is_polymorphic_final<Phrase1Arg> : public std::true_type {};

}

class Phrase1Arg final : public MetaConceptWith1Arg
{
private:
	typedef	bool (Phrase1Arg::*SyntaxOKAuxFunc)(void) const;
	typedef	size_t (Phrase1Arg::*LengthOfSelfNameAuxFunc)(void) const;
	typedef	void (Phrase1Arg::*ConstructSelfNameAuxFunc)(char* Name) const;

	struct Phrase1ArgVFT	{
							SyntaxOKAuxFunc SyntaxOK;
							LengthOfSelfNameAuxFunc LengthOfSelfName;
							ConstructSelfNameAuxFunc ConstructSelfName;
							};

	const char* PhraseKeyword;	// this controls the intended semantics
								// Phrase1Arg does *NOT* own this!
	static const Phrase1ArgVFT VFTable2Lookup[(MaxPhrase1Idx_MC-MinPhrase1Idx_MC)+1];
protected:
	Phrase1Arg() {};
public:
	Phrase1Arg(MetaConcept**& src, size_t KeywordIdx);	// constructor
													// NOTE: constructor fails by failing SyntaxOK()
//	Phrase1Arg(const Phrase1Arg& src);	// default OK
	virtual ~Phrase1Arg();
//	const Phrase1Arg& operator=(const Phrase1Arg& src);	// default OK
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Phrase1Arg*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	void MoveInto(Phrase1Arg*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.

//  Type ID functions
	virtual const AbstractClass* UltimateType() const;
//  Evaluation functions
	virtual bool CanEvaluate() const;
	virtual bool CanEvaluateToSameType() const;
	virtual bool SyntaxOK() const;
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	virtual bool DestructiveEvaluateToSameType();	// overwrites itself iff returns true
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual const char* ViewKeyword() const;
// NOTE: we may need this further down
// Formal manipulation functions
	static ExactType_MC CanConstruct(const MetaConcept * const * TargetArray, size_t KeywordIdx);
protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
private:
	bool SyntaxOK_IN() const;
	bool SyntaxOK_FACTORIAL() const;
	size_t LengthOfSelfName_Prefix() const;
	size_t LengthOfSelfName_FunctionLike() const;
	void ConstructSelfName_Prefix(char* Name) const;
	void ConstructSelfName_FunctionLike(char* Name) const;
};

#endif
