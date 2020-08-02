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
	typedef	bool (Phrase1Arg::*SyntaxOKAuxFunc)() const;

	struct Phrase1ArgVFT	{
							SyntaxOKAuxFunc SyntaxOK;
							};

	const char* PhraseKeyword;	// this controls the intended semantics
								// Phrase1Arg does *NOT* own this!
	static const Phrase1ArgVFT VFTable2Lookup[(MaxPhrase1Idx_MC-MinPhrase1Idx_MC)+1];
public:
	Phrase1Arg() = delete;
	Phrase1Arg(MetaConcept**& src, size_t KeywordIdx);	// constructor
													// NOTE: constructor fails by failing SyntaxOK()
	Phrase1Arg(const Phrase1Arg& src) = default;
	Phrase1Arg(Phrase1Arg&& src) = default;
	Phrase1Arg& operator=(const Phrase1Arg& src) = default;
	Phrase1Arg& operator=(Phrase1Arg&& src) = default;
	virtual ~Phrase1Arg() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(Phrase1Arg*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(Phrase1Arg*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	const AbstractClass* UltimateType() const override { return 0; }
//  Evaluation functions
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool CanEvaluate() const;
	bool CanEvaluateToSameType() const override { return false; }
	virtual bool SyntaxOK() const;
	virtual bool Evaluate(MetaConcept*& dest);		// same, or different type
	bool DestructiveEvaluateToSameType() override { return false; }
// text I/O functions
	const char* ViewKeyword() const override { return PhraseKeyword; }
// NOTE: we may need this further down
// Formal manipulation functions
	static ExactType_MC CanConstruct(const MetaConcept * const * TargetArray, size_t KeywordIdx);

protected:
	virtual bool EqualAux2(const MetaConcept& rhs) const;
	std::string to_s_aux() const override;

private:
	bool SyntaxOK_IN() const;
	bool SyntaxOK_FACTORIAL() const;
};

#endif
