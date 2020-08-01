// OS.hxx
// OS-specific routines, header

// Keyboard information
extern const long KeyBoardType;
extern const long KeyBoardSubType;
extern const long FunctionKeyCount;

// Mouse information, etc.
extern const long MousePresent;
extern const long NumOfMouseButtons;

extern const char* const OS_ID;

// System interrogation
void ExtractSystemInfo();	// call this at startup; nothing else works until then

unsigned long RAMPageSize();


#ifdef _WIN32
// window class names
extern const char* MainWindowClassName;
#endif

#ifndef NDEBUG
// does a low-level system test
void SystemTest(void);
#endif
