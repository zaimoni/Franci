// CSVTable.cpp
// implementation for CSVTable class

#include "CSVTable.hpp"
#include "../cstdio"
#include "../fstream"

using namespace std;
using namespace zaimoni;

#ifndef ZAIMONI_FORCE_ISO
#define TEXT_LEN(A) _msize(A)
#define CONST_TEXT_LEN(A) _msize(const_cast<char*>(A))
#else
#define TEXT_LEN(A) ZAIMONI_LEN_WITH_NULL(strlen(A))
#define CONST_TEXT_LEN(A) ZAIMONI_LEN_WITH_NULL(strlen(A))
#endif

// _split is inspired by the Perl split function
// Note: it can resume a split (this is why it works strictly last-to-first).
// In that case, it needs the Target array to "look compatible" with a restarted split
// The completed Target array can have NULL entries, which correspond to empty lines.
static size_t _predict_split_chunk_count(const char SplitOn, const char* src)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/28/2005
	if (!src || !*src) return 0;

	size_t LineCount = 1;
	size_t i = strlen(src);
	do	if (SplitOn==src[--i]) ++LineCount;
	while(0<i);
	return LineCount;
}

//! \todo delimited version
static bool _find_undelimited_CSV_cell_in_line(const char* const Line, size_t Col, size_t& i, size_t& Idx2, char FieldSeparator)
{
	const size_t Length = CONST_TEXT_LEN(Line);
	i = 0;
	size_t CellCount = 0;
	if (CellCount<Col)
		{
		do	if (Line[i]==FieldSeparator && ++CellCount>=Col) break;
		while(++i<Length);
		if (i>=Length) return false;
		}
	Idx2 = i;
	while(++Idx2<Length && Line[Idx2]!=FieldSeparator);
	return true;
}

static bool _split(const char SplitOn, char*& src, char**& dest)
{
	if (!src || !*src) return false;

	size_t i = strlen(src);
	size_t TerminateString = i;
	size_t LineCount = _predict_split_chunk_count(SplitOn,src);
	if (dest)
		{
		if (ArraySize(dest)<=LineCount) return false;
		size_t j = LineCount;
		do	if (dest[--j]) return false;
		while(0<j);
		}
	else{
		dest = _new_buffer<char*>(LineCount);
		if (!dest) return false;
		}

	size_t LeadingTrivialLines = 0;
	while(SplitOn==src[LeadingTrivialLines] && ++LeadingTrivialLines<i);
	if (LeadingTrivialLines>=LineCount)
		{
		FREE_AND_NULL(src);
		return true;
		}

	if (LeadingTrivialLines+1<LineCount)
		{
		i = TerminateString;
		do	if (SplitOn==src[--i])
				{
				--LineCount;
				if (i+1<TerminateString)
					{
					size_t destSize = TerminateString-(i+1);
					dest[LineCount] = _new_buffer<char>(destSize);
					if (!dest[LineCount] && TerminateString<strlen(src))
						{
						src = REALLOC(src,TerminateString);
						dest[LineCount] = _new_buffer<char>(destSize);
						}
					if (!dest[LineCount]) return false;
					memmove(dest[LineCount],src+i+1,destSize);
					}
				TerminateString = i;
				}
		while(0<i && LeadingTrivialLines+1<LineCount);
		}

	if (0<TerminateString)
		{
		if (0<LeadingTrivialLines)
			memmove(src,src+LeadingTrivialLines,(TerminateString-=LeadingTrivialLines));
		dest[LeadingTrivialLines] = REALLOC(src,TerminateString);
		}
	else
		free(src);
	src = NULL;
	return true;
}

static bool _interpret_line_as_csv(const char FieldSeparator, const char FieldDelimiter, char*& Line, char**& CSV)
{
	if (!FieldSeparator || !Line || !*Line) return false;
	if (CSV) BLOCKDELETEARRAY_AND_NULL(CSV);
	if (!FieldDelimiter) return _split(FieldSeparator,Line,CSV);

	size_t ApparentCells = _predict_split_chunk_count(FieldSeparator,Line);
	CSV = _new_buffer<char*>(ApparentCells);
	if (!CSV) return false;

	size_t CellIdx = 0;
	size_t Offset = 0;
	size_t CellStart = 0;
	const size_t LineLength = TEXT_LEN(Line);
	bool InDelimiter = false;

	while(Offset<LineLength)
		{
		if (InDelimiter)
			{
			if (FieldDelimiter==Line[Offset]) InDelimiter=false;
			}
		else{
			if 		(FieldDelimiter==Line[Offset]) InDelimiter=true;
			else if (FieldSeparator==Line[Offset])
				{
				if (CellStart<Offset)
					{
					CSV[CellIdx] = _new_buffer<char>(Offset-CellStart);
					if (!CSV[CellIdx])
						{
						BLOCKDELETEARRAY_AND_NULL(CSV);
						return false;
						}
					memmove(CSV[CellIdx],Line+CellStart,Offset-CellStart);
					}
				CellStart = Offset+1;
				CellIdx++;
				}
			}
		Offset++;
		};
	if (CellIdx<ApparentCells)
#if ZAIMONI_REALLOC_TO_ZERO_IS_NULL
		CSV = REALLOC(CSV,CellIdx);
#else
		{
		if (0 == CellIdx)
			FREE_AND_NULL(CSV);
		else
			CSV = REALLOC(CSV,CellIdx);
		}
#endif
	FREE_AND_NULL(Line);
	return true;
}

static size_t _csv_length_as_line(char** CSV, char Delimiter, bool MandatoryDelimiter)
{
	if (!CSV) return 0;
	size_t Length = ArraySize(CSV);
	size_t i = Length;
	do	if (!CSV[--i])
			{
			if (Delimiter && MandatoryDelimiter) Length +=2;
			}
		else{
			Length += strlen(CSV[i]);
			if (Delimiter)
				{
				if (MandatoryDelimiter || strchr(CSV[i],Delimiter)) Length +=2;
				}
			}
	while(0<i);
	return Length;	
}

static bool _csv_as_line(char** CSV, char Separator, char Delimiter, bool MandatoryDelimiter, char*& Line)
{
	if (!CSV || !Separator) return false;
	if (!Line)
		{
		Line = _new_buffer<char>(_csv_length_as_line(CSV,Delimiter,MandatoryDelimiter));
		if (!Line) return false;
		}
	size_t i = ArraySize(CSV);
	size_t Offset = 0;
	do	{
		if (!CSV[--i])
			{
			if (Delimiter && MandatoryDelimiter)
				{
				Line[Offset++] = Delimiter;
				Line[Offset++] = Delimiter;
				};
			}
		else{
			bool NeedDelimiter = Delimiter && (MandatoryDelimiter || strchr(CSV[i],Delimiter));
			if (NeedDelimiter) Line[Offset++] = Delimiter;
			memmove(Line+Offset,CSV[i],TEXT_LEN(CSV[i]));
			Offset += TEXT_LEN(CSV[i]);
			if (NeedDelimiter) Line[Offset++] = Delimiter;
			}
		if (0<i) Line[Offset++] = Separator;
		}
	while(0<i);
	return true;
}

// start implementation proper
void _SuppressTrailingLines(char*& src)
{
	size_t src_len = TEXT_LEN(src);
	while('\n'==src[src_len-1]) src[--src_len]='\0';
	src = REALLOC(src,ZAIMONI_LEN_WITH_NULL(src_len));
}

CSVTable::CSVTable(char NewFieldSeparator, char NewFieldDelimiter, bool NewNoZLS)
:	FieldSeparator(NewFieldSeparator),
	FieldDelimiter(NewFieldDelimiter),
	NoZLS(NewNoZLS)
{
}

CSVTable::~CSVTable()
{	// default destructor had problems on GCC 3.4.5
	RawText.clear();
	RawLines.clear();
	ColHeaders.clear();
	CSVRawText.clear();
}

bool CSVTable::SyntaxOK() const
{
	if (NULL!=RawText && NULL==RawLines && NULL==ColHeaders && NULL==CSVRawText) return true;
	if (NULL==RawText && NULL!=RawLines && NULL!=CSVRawText)
		{
		if (RawLines.ArraySize()!=CSVRawText.ArraySize()) return false;
		size_t i = RawLines.ArraySize();
		do	{
			--i;
			if (NULL!=RawLines[i] && NULL!=CSVRawText[i]) return false;
			}
		while(0<i);
		return true;
		}
	return false;
}
	
bool CSVTable::FirstRowIsColumnHeaders()
{
	if (NULL!=ColHeaders) return false;
	if (1>=RowCount()) return false;
	if (NULL!=RawText)
		{
		const char* Test = strchr(RawText,'\n');
		if (!Test || Test==RawText) return false;
		const size_t rowlen = Test-RawText;
		char* Line1 = _new_buffer<char>(rowlen);
		if (!Line1) return false;
		memmove(Line1,RawText,rowlen);
		if (_interpret_line_as_csv(FieldSeparator,FieldDelimiter,Line1,ColHeaders))
			{
			if (rowlen+1<RawText.size())
				{
				memmove(RawText,RawText+rowlen+1,RawText.size()-(rowlen+1));
				RawText.Shrink(RawText.size()-(rowlen+1));
				}
			else
				RawText.reset();
			return true;
			};
		free(Line1);
		return false;
		}
	
	if (NULL!=CSVRawText[0])
		{
		ColHeaders = std::move(CSVRawText[0]);
		CSVRawText.FastDeleteIdx(0);
		RawLines.FastDeleteIdx(0);
		return true;
		}
	if (	NULL!=RawLines[0]
		&& 	_interpret_line_as_csv(FieldSeparator,FieldDelimiter,RawLines[0],ColHeaders))
		{
		CSVRawText.FastDeleteIdx(0);
		RawLines.FastDeleteIdx(0);
		return true;
		}
	return false;
}

size_t CSVTable::ColumnCount() const
{
	if (NULL!=ColHeaders) return ColHeaders.ArraySize();
	if (NULL!=CSVRawText)
		{
		size_t i = CSVRawText.ArraySize();
		do	if (NULL!=CSVRawText[--i])
				return CSVRawText[i].ArraySize();
		while(0<i);
		}
	//! \todo more fallbacks
	return 0;
}

size_t CSVTable::RowCount() const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2005
	if (NULL!=RawLines) return RawLines.ArraySize();
	if (NULL!=CSVRawText) return CSVRawText.ArraySize();
	if (NULL!=RawText) return _predict_split_chunk_count('\n',RawText);
	return 0;
}

const char* CSVTable::ColumnHeaderFromColumn(size_t Col) const
{	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2005
	return (NULL!=ColHeaders && Col<ColHeaders.ArraySize()) ?  ColHeaders[Col] : NULL; 
}

ptrdiff_t CSVTable::ColumnFromColumnHeader(const char* const Col) const
{	// \return -1 on no-match, otherwise the internal array offset
	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2005
	if (!Col || !*Col || NULL==ColHeaders) return -1;
	size_t i = ColHeaders.ArraySize();
	do	if (NULL!=ColHeaders[--i] && 0==strcmp(Col,ColHeaders[i]))
			return i;
	while(0<i);
	return -1;
}

const char* CSVTable::RawLine(size_t Row)
{
	if (!ForceLinesRepresentation()) return NULL;
	if (NULL!=RawLines && Row<RawLines.ArraySize())
		{
		if (NULL!=RawLines[Row]) return RawLines[Row];
		if (NULL!=CSVRawText[Row])
			{	// autoconversion
			if (!_csv_as_line(CSVRawText[Row],FieldSeparator,FieldDelimiter,false,RawLines[Row])) return NULL;
			CSVRawText[Row].reset();
			return RawLines[Row];
			}
		}
	return NULL;
}

void CSVTable::RawLine(size_t Row, char*& Line)
{
	DELETE_AND_NULL(Line);
	ForceLinesRepresentation();
	if (NULL!=RawLines && Row<RawLines.ArraySize())
		{
		if (NULL!=RawLines[Row])
			{	// autocopy
			Line = REALLOC(Line,TEXT_LEN(RawLines[Row]));
			if (!Line) FATAL("RAM Failure: CSVTable text extraction");
			memmove(Line,RawLines[Row],TEXT_LEN(RawLines[Row]));
			return;
			}
		if (NULL!=CSVRawText[Row])
			{	// autocopy
			_csv_as_line(CSVRawText[Row],FieldSeparator,FieldDelimiter,false,Line);
			return;
			}
		}
}

const char* CSVTable::CellText_core(size_t Row, size_t Col)
{
	if (NULL!=RawLines[Row])
		{	// autoconversion
		if (!_interpret_line_as_csv(FieldSeparator,FieldDelimiter,RawLines[Row],CSVRawText[Row])) return NULL;
		if (Col<CSVRawText[Row].ArraySize()) return CSVRawText[Row][Col];
		return NULL;
		}
	if (NULL!=CSVRawText[Row])
		{	// return reference
		if (Col<CSVRawText[Row].ArraySize()) return CSVRawText[Row][Col];
		return NULL;
		}
	return NULL;
}

void CSVTable::CellText_core(size_t Row, size_t Col, char*& Text)
{
	if (NULL!=RawLines[Row])
		{	//! \todo: autocopy for delimited fields case
		if (!FieldDelimiter)
			{
			size_t Idx;
			size_t Idx2;
			if (_find_undelimited_CSV_cell_in_line(RawLines[Row],Col,Idx,Idx2,FieldSeparator))
				{
				size_t TargetSize = sizeof(char)*(Idx2-Idx);
				Text = REALLOC(Text,TargetSize);			
				if (!Text) FATAL("RAM Failure: CSVTable text extraction");	// if RAM failed, certainly can't do a row breakdown
				memmove(Text,&RawLines[Row][Idx],TargetSize);
				}
			return;
			}
		if (!_interpret_line_as_csv(FieldSeparator,FieldDelimiter,RawLines[Row],CSVRawText[Row])) return;
		}
	if (NULL!=CSVRawText[Row] && Col<CSVRawText[Row].ArraySize())
		{	// copy
		Text = REALLOC(Text,TEXT_LEN(CSVRawText[Row][Col]));
		if (!Text) FATAL("RAM Failure: CSVTable text extraction");
		memmove(Text,CSVRawText[Row][Col],TEXT_LEN(CSVRawText[Row][Col]));
		return;
		}
}

const char* CSVTable::CellText(size_t Row, size_t Col)
{
	if (!ForceLinesRepresentation()) return NULL;
	if (NULL!=RawLines && Row<RawLines.ArraySize())
		{
		const char* Tmp = CellText_core(Row,Col);
		if (NoZLS)
			while(!Tmp && 0<Row)
				Tmp = CellText_core(--Row,Col);
		return Tmp;
		}
	return NULL;
}

void CSVTable::CellText(size_t Row, size_t Col, char*& Text)
{
	DELETE_AND_NULL(Text);

	ForceLinesRepresentation();
	if (NULL!=RawLines && Row<RawLines.ArraySize())
		{
		CellText_core(Row,Col,Text);
		if (NoZLS)	
			while(!Text && 0<Row)
				CellText_core(--Row,Col,Text);		
		}
}

const char* CSVTable::CellText(size_t Row, const char* const Col)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2005
	ptrdiff_t ColIndex = ColumnFromColumnHeader(Col);
	if (0>ColIndex) return NULL;
	return CellText(Row,ColIndex);
}

void CSVTable::CellText(size_t Row, const char* const Col, char*& Text)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2005
	ptrdiff_t ColIndex = ColumnFromColumnHeader(Col);
	if (0>ColIndex)
		{
		DELETE_AND_NULL(Text);
		return;
		}
	CellText(Row,ColIndex,Text);
}

bool CSVTable::DeleteRow(size_t Row)
{
	ForceLinesRepresentation();
	if (NULL!=RawLines && Row<RawLines.ArraySize())
		{
		if (NoZLS && Row+1<RawLines.ArraySize())
			{	// must protect NoZLS format
			if (NULL!=RawLines[Row] && !_interpret_line_as_csv(FieldSeparator,FieldDelimiter,RawLines[Row],CSVRawText[Row])) return false;
			if (NULL!=RawLines[Row+1] && !_interpret_line_as_csv(FieldSeparator,FieldDelimiter,RawLines[Row+1],CSVRawText[Row+1])) return false;
			size_t RowCols = CSVRawText[Row].ArraySize();
			while(NULL==CSVRawText[Row][--RowCols] && 0<RowCols);
			if (NULL!=CSVRawText[Row][RowCols])
				{
				RowCols++;
				if (RowCols>CSVRawText[Row+1].ArraySize()) CSVRawText[Row+1].Resize(RowCols);
				do	if (	NULL!=CSVRawText[Row][--RowCols]
						&&	NULL==CSVRawText[Row+1][RowCols])
						{
						CSVRawText[Row+1][RowCols] = CSVRawText[Row][RowCols];
						CSVRawText[Row][RowCols] = NULL;
						}
				while(0<RowCols);
				};
			}
		RawLines.FastDeleteIdx(Row);
		CSVRawText.DeleteIdx(Row);	// FastDeleteIdx can double-free if used here
		return true;
		}
	return false;
}

size_t CSVTable::SELECT_Filter(size_t Col, const char* const RHS,string_predicate_2ary StringRel)
{
	if (!ForceLinesRepresentation()) return 0;
	if (NULL!=RawLines)
		{
//		bool LastMatch = false;	// need this for forward-order, not reverse order
		size_t Row = RawLines.ArraySize();
		do	{
			const char* Text = CellText(--Row,Col);
			if (!StringRel(Text,RHS)) DeleteRow(Row);
			}
		while(0<Row);
		return RawLines.ArraySize();
		}
	return 0;
}

// nondestructive operations, returns true if any rows found; Row is first row found
// SQL: SELECT * FROM ... WHERE StringRel(Col,RHS)
bool CSVTable::SELECT_MinTerm(size_t Col, const char* const RHS,string_predicate_2ary StringRel,
					size_t& Row)
{
	if (!ForceLinesRepresentation()) return false;
	if (NULL!=RawLines)
		{
		const size_t MaxRow = RawLines.ArraySize();
		size_t AltRow = 0;
		do	if (StringRel(CellText(AltRow,Col),RHS))
				{
				Row = AltRow;
				return true;
				}
		while(++AltRow<MaxRow);
		}
	return false;
}

// SQL: SELECT * FROM ... WHERE StringRel(Col,RHS) AND StringRel2(Col2,RHS2)
bool CSVTable::SELECT_MinTerm(size_t Col, const char* const RHS,string_predicate_2ary StringRel,
					size_t Col2, const char* const RHS2,string_predicate_2ary StringRel2,
					size_t& Row)
{
	if (!ForceLinesRepresentation()) return false;
	if (NULL!=RawLines)
		{
		const size_t MaxRow = RawLines.ArraySize();
		size_t AltRow = 0;
		do	if (   StringRel(CellText(AltRow,Col),RHS)
				&& StringRel2(CellText(AltRow,Col2),RHS2))
				{
				Row = AltRow;
				return true;
				}
		while(++AltRow<MaxRow);
		}
	return false;
}

// forces unique results in ImageArray
// returns true iff at least one result
// grep returns common entries
// invgrep returns entries in one not in the other
// invgrep_A_B returns entries in reference_array
// invgrep_B_A returns entries in HelpTable
// Target doesn't own strings (that is the source's problem)
bool CSVTable::set_grep(char** reference_array,size_t Col,string_predicate_2ary StringRel,const char**& dest)
{
	FREE_AND_NULL(dest);
	if (reference_array && ForceLinesRepresentation())
		{
		size_t ActualHits = 0;
		dest = _new_buffer<const char*>(ArraySize(reference_array));

		size_t i = 0;
		do	{
			size_t j = RawLines.ArraySize();
			do	if (StringRel(reference_array[i],CellText(--j,Col)))
					{
					dest[ActualHits++] = reference_array[i];
					break;
					}
			while(0<j);
			}
		while(++i<ArraySize(reference_array));

#if ZAIMONI_REALLOC_TO_ZERO_IS_NULL
		dest = REALLOC(dest,ActualHits);
		return dest;		
#else
		if (0<ActualHits)
			{
			dest = REALLOC(dest,ActualHits);
			return true;
			}
		else{
			FREE_AND_NULL(dest);	
			return false;
			}
#endif
		}
	return false;
}

bool CSVTable::set_invgrep_A_B(char** reference_array,size_t Col,string_predicate_2ary StringRel,const char**& dest)
{
	FREE_AND_NULL(dest);
	if (reference_array && ForceLinesRepresentation())
		{
		size_t ActualHits = 0;
		dest = _new_buffer<const char*>(ArraySize(reference_array));

		size_t i = 0;
		do	{
			bool Keep = true;
			size_t j = RawLines.ArraySize();
			do	if (StringRel(reference_array[i],CellText(--j,Col)))
					{
					Keep = false;
					break;
					}
			while(0<j);
			if (Keep) dest[ActualHits++] = reference_array[i];
			}
		while(++i<ArraySize(reference_array));

#if ZAIMONI_REALLOC_TO_ZERO_IS_NULL
		dest = REALLOC(dest,sizeof(char*)*ActualHits);
		return dest;
#else
		if (0<ActualHits)
			{
			dest = REALLOC(dest,sizeof(char*)*ActualHits);
			return true;
			}
		else{
			FREE_AND_NULL(dest);	
			return false;
			}
#endif
		}
	return false;
}

bool CSVTable::set_invgrep_B_A(char** reference_array,size_t Col,string_predicate_2ary StringRel,const char**& dest)
{
	FREE_AND_NULL(dest);
	if (reference_array && ForceLinesRepresentation())
		{
		size_t ActualHits = 0;
		dest = _new_buffer<const char*>(RawLines.ArraySize());

		size_t i = RawLines.ArraySize();
		do	{
			const char* TextRef = CellText(--i,Col);
			if (NULL!=TextRef)
				{
				bool Keep = true;
				if (0<ActualHits)
					{
					size_t i = ActualHits;
					do	if (0==strcmp(dest[--i],TextRef))
							{
							Keep = false;
							break;
							}
					while(0<i);
					}
				if (Keep)
					{
					size_t i = ArraySize(reference_array);
					do	if (StringRel(reference_array[--i],TextRef))
							{
							Keep = false;
							break;
							}
					while(0<i);
					}
				if (Keep) dest[ActualHits++] = TextRef;
				}
			}
		while(0<i);

#if ZAIMONI_REALLOC_TO_ZERO_IS_NULL
		dest = REALLOC(dest,sizeof(char*)*ActualHits);
		return dest;
#else
		if (0<ActualHits)
			{
			dest = REALLOC(dest,sizeof(char*)*ActualHits);
			return true;
			}
		else{
			FREE_AND_NULL(dest);	
			return false;
			}
#endif
		}
	return false;
}


void CSVTable::DiscardColumn(const char* const Col)
{
	ptrdiff_t ColIndex = ColumnFromColumnHeader(Col);
	if (0<=ColIndex) DiscardColumn(ColIndex);
}

void CSVTable::DiscardColumn(size_t Col)
{
	if (!ForceLinesRepresentation()) return;
	if (NULL!=RawLines)
		{	
		size_t Row = RawLines.ArraySize();
		do	{
			if 		(NULL!=RawLines[--Row])
				{	//! \todo autocontract for delimited fields case
				if (!FieldDelimiter)
					{
					size_t Idx;
					size_t Idx2;
					if (_find_undelimited_CSV_cell_in_line(RawLines[Row],Col,Idx,Idx2,FieldSeparator))
						{
						const size_t Length = _msize(RawLines[Row]);
						if (Idx2<Length)
							memmove(RawLines[Row]+Idx,RawLines[Row]+Idx2,Length-Idx2);
						RawLines[Row] = REALLOC(RawLines[Row],TEXT_LEN(RawLines[Row])-(Idx2-Idx));
						}
					return;
					}
				if (!_interpret_line_as_csv(FieldSeparator,FieldDelimiter,RawLines[Row],CSVRawText[Row])) return;
				}
			CSVRawText[Row].DeleteIdx(Col);	// need limits check, so not-fast
			}
		while(0<Row);
		ColHeaders.DeleteIdx(Col);	// need limits check, so not-fast
		}
}

bool CSVTable::RAMTable(const char* const FileName)
{	// FORMALLY CORRECT: Kenneth Boyd, 2/22/2005
	char* RawText2 = NULL;
	if (!GetBinaryFileImage(FileName,RawText2)) return false;

//	RawText = NULL;	// not strictly necessary; nothing following this increases RAM loading
	RawLines.reset();
	ColHeaders.reset();
	CSVRawText.reset();

	ConvertBinaryModeToTextMode(RawText2);
	_SuppressTrailingLines(RawText2);
	RawText = RawText2;
	return true;
}

bool CSVTable::DumpToFlatfile(const char* const FileName)
{
	ofstream* Target = OutfileText(FileName,"cannot open file");
	if (NULL!=RawText)
		{
		Target->write(RawText,(streamsize)(_msize(RawText)));
		}
	else if (NULL!=RawLines)
		{
		if (NULL!=ColHeaders)
			{
			const size_t PhysicalRowLength = ColHeaders.ArraySize();
			size_t Idx2 = 0;
			do	{
				if (NULL!=ColHeaders[Idx2])
					{
					if ('\0'!=FieldDelimiter) Target->put(FieldDelimiter);
					Target->write(ColHeaders[Idx2],(streamsize)(_msize(ColHeaders[Idx2])));
					if ('\0'!=FieldDelimiter) Target->put(FieldDelimiter);
					}
				if (Idx2<PhysicalRowLength-1) Target->put(FieldSeparator);
				}
			while(++Idx2<PhysicalRowLength && Target->good());
			Target->put('\n');
			}
		size_t Idx = 0;
		do	{
			if (NULL!=RawLines[Idx])
				{
#ifndef ZAIMONI_FORCE_ISO
				Target->write(RawLines[Idx],(streamsize)(_msize(RawLines[Idx])));
#else
				Target->write(RawLines[Idx],(streamsize)(strlen(RawLines[Idx])));
#endif
				}
			else if (NULL!=CSVRawText[Idx])
				{
				const size_t PhysicalRowLength = CSVRawText[Idx].ArraySize();
				size_t Idx2 = 0;
				do	{
					if (NULL!=CSVRawText[Idx][Idx2])
						{
						if ('\0'!=FieldDelimiter) Target->put(FieldDelimiter);
#ifndef ZAIMONI_FORCE_ISO
						Target->write(CSVRawText[Idx][Idx2],(streamsize)(strlen(CSVRawText[Idx][Idx2])));
#else
						Target->write(CSVRawText[Idx][Idx2],(streamsize)(_msize(CSVRawText[Idx][Idx2])));
#endif
						if ('\0'!=FieldDelimiter) Target->put(FieldDelimiter);
						}
					if (Idx2<PhysicalRowLength-1) Target->put(FieldSeparator);
					}
				while(++Idx2<PhysicalRowLength && Target->good());
				}
			if (0<Idx) Target->put('\n');
			}
		while(++Idx<RawLines.ArraySize() && Target->good());
		}
	bool Success = Target->good();
	CloseAndNULL(Target);
	return Success;
}

bool CSVTable::ForceLinesRepresentation()
{
	if (!RawLines.empty()) return true;
	if (char* Tmp = RawText.release())
		{
		if (_split('\n',Tmp,RawLines))
			return CSVRawText.Resize(RawLines.ArraySize());
		}
	return false;	
}
