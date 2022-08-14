// PhraseN.hxx
// header for PhraseNArg, the metatype for n-ary phrases

#ifndef PHRASE_NARG_DEF
#define PHRASE_NARG_DEF

#include "MetaCon2.hxx"

class PhraseNArg final : public MetaConceptWithArgArray
{
private:
	typedef	bool (PhraseNArg::*SyntaxOKAuxFunc)() const;

	const char* PhraseKeyword;	// this controls the intended semantics; not owned
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
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	virtual bool SyntaxOK() const;
// text I/O functions
	const char* ViewKeyword() const override { return PhraseKeyword; }
// type-specific functions
	static ExactType_MC CanConstructNonPostfix(const MetaConcept* const * src, size_t KeywordIdx);
	static ExactType_MC CanConstructPostfix(const MetaConcept* const * src, size_t KeywordIdx);

protected:
	std::string to_s_aux() const override;
	void _forceStdForm() override;

	virtual void DiagnoseInferenceRules() const;
	bool InvokeEqualArgRule() const override { return false; }

private:
	void _ForceArgSameImplementation(size_t n) override;
	bool SyntaxOKAuxCommaListVarNames() const;

	static ExactType_MC CanConstruct(const MetaConcept* const * src, size_t KeywordIdx);
	void ExtractPrefixCommaListVarNames(MetaConcept**& Target, size_t& KeywordIdx);
	void ExtractPostfixCommaListVarNames(MetaConcept**& Target, size_t& KeywordIdx);
};

#endif
