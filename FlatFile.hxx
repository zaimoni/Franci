// FlatFile.hxx
// Definitions for the FlatFile class

#ifndef FLATFILE_DEF
#define FLATFILE_DEF

#include "MetaCon2.hxx"

class FlatFile;
namespace zaimoni {

template<>
struct is_polymorphic_final<FlatFile> : public boost::true_type {};

}

class FlatFile : public MetaConceptWithArgArray
{
protected:
	static char** Filenames;
	static size_t* FilenameRefCount;
	static size_t LastFilenameIdx;

	const FlatFile& operator=(const FlatFile& src);
public:
	FlatFile() {};
	FlatFile(const FlatFile& src);
	virtual ~FlatFile();
	virtual void CopyInto(MetaConcept*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void CopyInto(FlatFile*& dest) const {CopyInto_ForceSyntaxOK(*this,dest);};	// can throw memory failure
	virtual void MoveInto(MetaConcept*& dest) {zaimoni::MoveInto(*this,dest);};	// can throw memory failure.  If it succeeds, it destroys the source.
	virtual void MoveInto(FlatFile*& dest);	// can throw memory failure.  If it succeeds, it destroys the source.
	virtual const AbstractClass* UltimateType() const;
	virtual bool SyntaxOK() const;
// text I/O functions
	virtual size_t LengthOfSelfName(void) const;
	// type-specific functions
	bool AppendBlankLine() {return InsertSlotAt(ArgArray.size(),NULL);};
	void RemoveLineBlock(size_t NonStrictLB, size_t NonStrictUB);
	bool ReadASCIIFile(const char* const Filename);	// reads entire ASCII file into object
	void unshift(char*& Line,size_t& LineNumber, const char*& SourceFileName);	// named after Perl unshift
//	void LinewiseFilter();	// unary action; must return false to preserve line, true to delete line
	bool DumpASCIIFile(const char* const Filename);	// writes entire ASCII file from object
protected:
	virtual void ConstructSelfNameAux(char* Name) const;		// overwrites what is already there
	virtual void _forceStdForm();

	virtual void DiagnoseInferenceRules() const;	// This is *not* the Interface!
	virtual bool InvokeEqualArgRule() const;
private:
	bool SplitLineIntoCache(char*& Buffer, const char* const Filename);
	bool ValidateFilename(const char* const Filename) const;
};

namespace zaimoni
{

template<>
struct has_invalid_assignment_but_copyconstructable<FlatFile> : public boost::true_type {};

}

#endif
