// PhraseN.hxx
// header for PhraseNArg, the metatype for n-ary phrases

#ifndef PHRASE_NARG_DEF
#define PHRASE_NARG_DEF

#include "MetaCon2.hxx"

class PhraseNArg;
namespace zaimoni {

template<>
struct is_polymorphic_final<PhraseNArg> : public std::true_type {};

}

class PhraseNArg final : public MetaConceptWithArgArray
{
private:
	typedef	bool (PhraseNArg::*SyntaxOKAuxFunc)(void) const;
	typedef	size_t (PhraseNArg::*LengthOfSelfNameAuxFunc)(void) const;
	typedef	void (PhraseNArg::*ConstructSelfNameAuxFunc)(char* const Name) const;

	struct PhraseNArgVFT	{
							SyntaxOKAuxFunc SyntaxOK;
							LengthOfSelfNameAuxFunc LengthOfSelfName;
							ConstructSelfNameAuxFunc ConstructSelfName;
							};


	const char* PhraseKeyword;	// this controls the intended semantics
								// PhraseNArg does *NOT* own this!
	static LengthOfSelfNameAuxFunc LengthOfSelfNameAuxArray[(MaxPhraseNIdx_MC-MinPhraseNIdx_MC)+1];
	static ConstructSelfNameAuxFunc ConstructSelfNameAuxArray[(MaxPhraseNIdx_MC-MinPhraseNIdx_MC)+1];
	static SyntaxOKAuxFunc SyntaxOKAuxArray[(MaxPhraseNIdx_MC-MinPhraseNIdx_MC)+1];
public:
	PhraseNArg() = delete;
	PhraseNArg(MetaConcept**& src, size_t& KeywordIdx);
	PhraseNArg(const PhraseNArg& src) = default;
	PhraseNArg(PhraseNArg&& src) = default;
	PhraseNArg& operator=(const PhraseNArg & src) = default;
	PhraseNArg& operator=(PhraseNArg&& src) = default;
	virtual ~PhraseNArg() = default;

	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void CopyInto(PhraseNArg*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this),dest); }
	void MoveInto(PhraseNArg*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }

//  Type ID functions
	const AbstractClass* UltimateType() const override { return 0; }
//  Evaluation functions
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName() const;
	virtual const char* ViewKeyword() const;
// type-specific functions
	static ExactType_MC CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx);
	static ExactType_MC CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx);
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	void _forceStdForm() override;

	virtual void DiagnoseInferenceRules() const;
	virtual bool InvokeEqualArgRule() const;
private:
	bool SyntaxOKAuxCommaListVarNames() const;

	size_t LengthOfSelfNamePrefixOrPostfixCommaListVarNames(void) const;	// start at 0 to get length
	void ConstructSelfNamePrefixCommaListVarNames(char* const Name) const;		// overwrites what is already there
	void ConstructSelfNamePostfixCommaListVarNames(char* const Name) const;		// overwrites what is already there

	static ExactType_MC CanConstruct(const MetaConcept* const * src, size_t KeywordIdx);
	void ExtractPrefixCommaListVarNames(MetaConcept**& Target, size_t& KeywordIdx);
	void ExtractPostfixCommaListVarNames(MetaConcept**& Target, size_t& KeywordIdx);
};

#endif
