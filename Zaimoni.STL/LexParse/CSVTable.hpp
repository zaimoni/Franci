// CSVTable.hpp
// header for CSVTable class

#ifndef ZAIMONI_STL_LEXPARSE_CSVTABLE_HPP
#define ZAIMONI_STL_LEXPARSE_CSVTABLE_HPP

#include "../AutoPtr.hpp"
#include "../string2.h"

class CSVTable 
{
private:
	char FieldSeparator;
	char FieldDelimiter;
	bool NoZLS;	// NoZLS format assumes that all empty strings are to be 
		// interpolated from the first value (backwards) that is not 
		// an empty string.  Useful for human verifiability.

	zaimoni::autovalarray_ptr<char> RawText;
	zaimoni::autovalarray_ptr<char*> RawLines;
	zaimoni::autovalarray_ptr<char*> ColHeaders;
	zaimoni::autovalarray_ptr<zaimoni::autovalarray_ptr<char*> > CSVRawText;

	// not copyable, so ...
	CSVTable(const CSVTable& src);
	void operator=(const CSVTable& rhs);
public:
	CSVTable(char NewFieldSeparator, char NewFieldDelimiter, bool NewNoZLS);
	~CSVTable();	// default OK

	bool SyntaxOK() const;
	
	bool FirstRowIsColumnHeaders();	
	size_t ColumnCount() const;
	size_t RowCount() const;

	const char* ColumnHeaderFromColumn(size_t Col) const;
	ptrdiff_t ColumnFromColumnHeader(const char* const Col) const;

	const char* RawLine(size_t Row);
	void RawLine(size_t Row, char*& Line);

	const char* CellText(size_t Row, size_t Col);
	void CellText(size_t Row, size_t Col, char*& Text);
	const char* CellText(size_t Row, const char* const Col);
	void CellText(size_t Row, const char* const Col, char*& Text);

	bool DeleteRow(size_t Row);
	// destructive operations, returns number of rows that passed StringRel
	// SQL: SELECT * FROM ... WHERE StringRel(Col,RHS)
	size_t SELECT_Filter(size_t Col, const char* RHS,string_predicate_2ary StringRel);

	// nondestructive operations, returns true if any rows found; Row is first row found
	// SQL: SELECT * FROM ... WHERE StringRel(Col,RHS)
	bool SELECT_MinTerm(size_t Col, const char* RHS,string_predicate_2ary StringRel,
						size_t& Row);

	// SQL: SELECT * FROM ... WHERE StringRel(Col,RHS) AND StringRel2(Col2,RHS2)
	bool SELECT_MinTerm(size_t Col, const char* RHS,string_predicate_2ary StringRel,
						size_t Col2, const char* RHS2,string_predicate_2ary StringRel2,
						size_t& Row);

	// forces unique results in ImageArray
	// returns true iff at least one result
	// grep returns common entries
	// invgrep returns entries in one not in the other
	// invgrep_A_B returns entries in reference_array
	// invgrep_B_A returns entries in HelpTable
	// Target doesn't own strings (that is the source's problem)
	bool set_grep(char** reference_array,size_t Col,string_predicate_2ary StringRel,const char**& dest);
	bool set_invgrep_A_B(char** reference_array,size_t Col,string_predicate_2ary StringRel,const char**& dest);
	bool set_invgrep_B_A(char** reference_array,size_t Col,string_predicate_2ary StringRel,const char**& dest);

	// discard column
	void DiscardColumn(const char* Col);
	void DiscardColumn(size_t Col);

	bool RAMTable(const char* FileName);
	bool DumpToFlatfile(const char* FileName);

	bool ForceLinesRepresentation();	// irreversible

private:
	const char* CellText_core(size_t Row, size_t Col);
	void CellText_core(size_t Row, size_t Col, char*& Text);
};

#endif
