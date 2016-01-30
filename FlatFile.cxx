// Flatfile.cxx
// Implementation for the FlatFile class

#include "Unparsed.hxx"
#include "FlatFile.hxx"
#include "LangCon_.hxx"
#include "Zaimoni.STL/LexParse/LangConf.hpp"
#include "Zaimoni.STL/cstdio"
#include "Zaimoni.STL/fstream"

// NOTE: FlatFile reuses filenames a lot.  This justifies a reference-count system.
// This does mean that FlatFile needs the AIMutex to be thread-safe.
// NOTE: FlatFile doesn't like UnparsedText's auto-whitespace cleanup.
// [this will cause spec violations in C/C++ compilation; Python and Fortran will be microwaved]

// filename cache: candidate for boost::shared_ptr, which requires RTTI
char** FlatFile::Filenames = NULL;
size_t* FlatFile::FilenameRefCount = NULL;
size_t FlatFile::LastFilenameIdx=0;

FlatFile::~FlatFile()
{	// TODO: VERIFY
	// dereference filename entries from the refcount system
	if (!ArgArray.empty())
		{
		size_t i = fast_size();
		do	{	//! \todo function target.  This unregisters a filename from the filename cache
			if (   ValidateFilename(static_cast<UnparsedText*>(ArgArray[--i])->SourceFileName)
				&& 0==--FilenameRefCount[LastFilenameIdx])
				{
				if (LastFilenameIdx+1==ArraySize(Filenames))
					{
					LastFilenameIdx = 0;
					}
				else{
					DELETE(Filenames[LastFilenameIdx]);
					memmove(&FilenameRefCount[LastFilenameIdx],&FilenameRefCount[LastFilenameIdx+1],sizeof(size_t)*(ArraySize(FilenameRefCount)-(LastFilenameIdx+1)));
					memmove(&Filenames[LastFilenameIdx],&Filenames[LastFilenameIdx+1],sizeof(char*)*(ArraySize(Filenames)-(LastFilenameIdx+1)));
					}
				// realloc is free-and-null for size 0
				Filenames = REALLOC(Filenames,_msize(Filenames)-sizeof(char*));
				FilenameRefCount = REALLOC(FilenameRefCount,_msize(FilenameRefCount)-sizeof(size_t));
				};
			}
		while(0<i);
		}
}

FlatFile::FlatFile(const FlatFile& src)
:	MetaConceptWithArgArray(src)
{	//! \todo: fixup reference counts, or convert to boost::shared_ptr
}

const FlatFile& FlatFile::operator=(const FlatFile& src)
{	//! \todo: fixup reference counts, or convert to boost::shared_ptr
	MetaConceptWithArgArray::operator=(src);
	return *this;
}

void FlatFile::MoveInto(FlatFile*& dest)		// can throw memory failure.  If it succeeds, it destroys the source.
{	//! \todo IMPLEMENT
	FATAL(AlphaMustDefineVFunction);
#if 0
	if (!dest)
		{
		MetaConcept** TmpArgArray = NULL;
		dest = new FlatFile(TmpArgArray,(MetaConnectiveModes)(ExactType()-LogicalAND_MC));
		};
	MoveIntoAux(*dest);
#endif
}

const AbstractClass* FlatFile::UltimateType() const {return NULL;}
bool FlatFile::SyntaxOK() const {return SyntaxOKAux();}
void FlatFile::_forceStdForm() {}

size_t FlatFile::LengthOfSelfName() const
{	//! \todo IMPLEMENT
	return 0;
}

void FlatFile::ConstructSelfNameAux(char* Name) const		// overwrites what is already there
{	//! \todo IMPLEMENT
}

void FlatFile::DiagnoseInferenceRules() const
{
	IdxCurrentSelfEvalRule = SelfEvalSyntaxOKNoRules_SER;
}

bool FlatFile::InvokeEqualArgRule() const {return false;}

//! \todo Relocate/Rename to MetaConceptWithArgArray
void FlatFile::RemoveLineBlock(size_t NonStrictLB, size_t NonStrictUB)
{	// FORMALLY CORRECT: Kenneth Boyd, 9/8/2002
	assert(NonStrictLB<=NonStrictUB);
	assert(fast_size()>NonStrictUB);
	size_t N = NonStrictUB-NonStrictLB+1;
	DeleteNSlotsAt(N,NonStrictLB);
}

bool FlatFile::ReadASCIIFile(const char* const Filename)
{	// FORMALLY CORRECT: 10/17/2004
	char* Buffer = NULL;
	// FlatFile is supposed to be an in-RAM representation anyway, so this isn't as inefficient as one would think
	if (!GetBinaryFileImage(Filename,Buffer)) return false;

	ConvertBinaryModeToTextMode(Buffer);

	//! \todo whitespace manipulation hooks
	//! \todo Flatfile::SplitLineIntoCache should take a LangConf object; ergo, FlatFile::ReadASCIIFile should take a LangConf object
	//! Start by assuming Interpreted true: CPlusPlus, Interpreted false: FranciScript
	//! <p>Apply the global filter *before* the line splitting!
	FranciLexer.ApplyGlobalFilters(Buffer,Filename);
	FranciLexer.FlattenComments(Buffer);

	while(Buffer)
		{
		if (!SplitLineIntoCache(Buffer,Filename))
			{	// NOTE: this may leave the FlatFile partially constructed.
			free(Buffer);
			return false;
			}
		};
	return true;
}

void FlatFile::unshift(char*& Line,size_t& LineNumber, const char*& SourceFileName)	// named after Perl unshift
{
	if (NULL!=Line) DELETEARRAY_AND_NULL(Line);
	if (NULL==ArgArray)
		{
		LineNumber = 0;
		SourceFileName = NULL;
		return;
		}
	{
	UnparsedText& Tmp = UnparsedText::up_reference(ArgArray[0]);
	LineNumber = Tmp.LogicalLineNumber;
	SourceFileName = Tmp.SourceFileName;
	Tmp.ExtractText(Line);
	}
	FastDeleteIdx(0);
}

bool FlatFile::SplitLineIntoCache(char*& Buffer, const char* const Filename)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	assert(Buffer);
	if (!ValidateFilename(Filename))
		{	// add filename to list
		if (Filenames)
			{
			char** Tmp = REALLOC(Filenames,_msize(Filenames)+sizeof(char*));
			if (!Tmp) return false;
			Filenames = Tmp;
			size_t* Tmp2 = REALLOC(FilenameRefCount,_msize(FilenameRefCount)+sizeof(size_t));
			if (!Tmp2)
				{
				Filenames = REALLOC(Filenames,_msize(Filenames)-sizeof(char*));
				return false;
				}
			Filenames[ArraySize(Filenames)-1]=_new_buffer<char>(strlen(Filename)+1);
			if (!Filenames[ArraySize(Filenames)-1])
				{
				FilenameRefCount = REALLOC(FilenameRefCount,_msize(Filenames)-sizeof(size_t));
				Filenames = REALLOC(Filenames,_msize(Filenames)-sizeof(char*));
				return false;
				}
			}
		else{
			Filenames = _new_buffer<char*>(1);
			if (!Filenames) return false;
			FilenameRefCount = _new_buffer<size_t>(1);
			if (!FilenameRefCount)
				{
				FREE_AND_NULL(Filenames);
				return false;
				};
			Filenames[ArraySize(Filenames)-1]=_new_buffer<char>(strlen(Filename)+1);
			if (!Filenames[ArraySize(Filenames)-1])
				{
				FREE_AND_NULL(FilenameRefCount);
				FREE_AND_NULL(Filenames);
				return false;
				}
			}
		LastFilenameIdx = ArraySize(Filenames)-1;
		strcpy(Filenames[LastFilenameIdx],Filename);
		FilenameRefCount[LastFilenameIdx]=0;
		};

	size_t StartingLogicalLineNumber = (NULL==ArgArray) ? 0
									 : static_cast<UnparsedText*>(ArgArray[fast_size()-1])->LogicalLineNumber;
	FlushLeadingBlankLinesFromTextBuffer(Buffer,StartingLogicalLineNumber);
	if (!Buffer) return true;
	// First, make space (as above)
	if (!AppendBlankLine()) return false;
	char* NewLine = NULL;
	//! \todo SplitLineIntoCache should use a LangConf object
	FranciLexer.ExtractLineFromTextBuffer(Buffer,NewLine);
	if (!NewLine)
		{
		FastDeleteIdx(fast_size()-1);
		return false;
		}

	{
	UnparsedText* Tmp = new(nothrow) UnparsedText(NewLine);
	if (!Tmp)
		{
		free(NewLine);
		FastDeleteIdx(fast_size()-1);
		return false;
		}
	Tmp->LogicalLineNumber=++StartingLogicalLineNumber;
	Tmp->SourceFileName=Filenames[LastFilenameIdx];
	ArgArray[fast_size()-1] = Tmp;
	}
	return true;
}

bool FlatFile::ValidateFilename(const char* const Filename) const
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	if (Filenames)
		{
		if (!strcmp(Filenames[LastFilenameIdx],Filename)) return true;
		size_t i = ArraySize(Filenames);
		do	if (   LastFilenameIdx!= --i
				&& !strcmp(Filenames[i],Filename))
				{
				LastFilenameIdx=i;
				return true;
				}
		while(0<i);
		}
	return false;
}

bool FlatFile::DumpASCIIFile(const char* const Filename)		// writes entire ASCII file from object
{	// FORMALLY CORRECT: Kenneth Boyd, 4/23/2006
	assert(Filename);
	if (NULL==ArgArray) return false;
	ofstream* TargetFile = OutfileText(Filename,"file could not be written");
	if (!TargetFile) return false;

	size_t i = 0;
	do	{
		char* Tmp = NULL;
		ArgArray[i]->ConstructSelfName(Tmp);
		if (Tmp)
			{
			TargetFile->write(Tmp,ArraySize(Tmp));
			free(Tmp);
			};
		TargetFile->put('\n');
		}
	while(++i<fast_size());
	CloseAndNULL(TargetFile);
	return true;
}

