// FlatFile.hxx
// Definitions for the FlatFile class

#ifndef FLATFILE_DEF
#define FLATFILE_DEF

#include "MetaCon2.hxx"

class FlatFile;
namespace zaimoni {

template<>
struct is_polymorphic_final<FlatFile> : public std::true_type {};

}

class FlatFile final : public MetaConceptWithArgArray
{
protected:
	static char** Filenames;
	static size_t* FilenameRefCount;
	static size_t LastFilenameIdx;

public:
	FlatFile() = default;
	FlatFile(const FlatFile& src) = default;
	FlatFile(FlatFile&& src) = default;
	FlatFile& operator=(const FlatFile & src) = default;
	FlatFile& operator=(FlatFile&& src) = default;
	virtual ~FlatFile();
	void CopyInto(MetaConcept*& dest) const override {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void CopyInto(FlatFile*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	void MoveInto(MetaConcept*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }
	void MoveInto(FlatFile*& dest) { zaimoni::MoveIntoV2(std::move(*this), dest); }
	const AbstractClass* UltimateType() const override { return 0; }
	std::pair<std::function<bool()>, std::function<bool(MetaConcept*&)> > canEvaluate() const override;
	bool SyntaxOK() const override { return SyntaxOKAux(); }
	// type-specific functions
	bool AppendBlankLine() {return InsertSlotAt(ArgArray.size(),NULL);};
	void RemoveLineBlock(size_t NonStrictLB, size_t NonStrictUB);
	bool ReadASCIIFile(const char* const Filename);	// reads entire ASCII file into object
	void unshift(char*& Line,size_t& LineNumber, const char*& SourceFileName);	// named after Perl unshift
//	void LinewiseFilter();	// unary action; must return false to preserve line, true to delete line
	bool DumpASCIIFile(const char* const Filename);	// writes entire ASCII file from object

protected:
	std::string to_s_aux() const override { return nullptr; }	// \todo implement
	void _forceStdForm() override {}

	virtual void DiagnoseInferenceRules() const;	// This is *not* the Interface!
	bool InvokeEqualArgRule() const override { return false; }

private:
	void _ForceArgSameImplementation(size_t n) override;

	bool SplitLineIntoCache(char*& Buffer, const char* const Filename);
	bool ValidateFilename(const char* const Filename) const;
};

namespace zaimoni
{

template<>
struct has_invalid_assignment_but_copyconstructable<FlatFile> : public std::true_type {};

}

#endif
