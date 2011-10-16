// OS.cxx
// OS-specific routines, implementation

#ifdef _WIN32

#include <WINDOWS.H>
// #include <MAPIDEFS.H>
// #include <MAPINLS.H>

SYSTEM_INFO InitialHardwareSetup;
// wProcessorArchitecture:
// dwPageSize
// lpMinimumApplicationAddress
// lpMaximumApplicationAddress
// dwActiveProcessorMask
// dwNumberOfProcessors
// dwProcessorType
// dwAllocationGranularity
// wProcessorLevel
// wProcessorRevision
OSVERSIONINFO Win95Version;
// dwOSVersionInfoSize
// dwMajorVersion
// dwMinorVersion
// dwBuildNumber
// dwPlatformId
// szCSDVersion[128]

// Keyboard infomation
extern const long KeyBoardType = GetKeyboardType(0);
extern const long KeyBoardSubType = GetKeyboardType(1);
extern const long FunctionKeyCount = GetKeyboardType(2);

// NOTE: SystemsParameterInfo calls usually dynamically affect the *interface*.
// NOTE: GetWindowsDirectory is the Win32-standard place for initialization and help files.
//       Alternate is HOMEPATH variable.
// NOTE: GetSysColor dynamically affects the *interface*

// Mouse information, etc.
extern const long MousePresent = GetSystemMetrics(SM_MOUSEPRESENT);
extern const long NumOfMouseButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);

extern const char* MainWindowClassName = "FranciMainWindowClass";

//! \todo dynamically construct OS_ID
//! We want: Processor type, OS
extern const char* const OS_ID = "<br>(1 GHz Cyrix III, Win2000)<br>";

void 
ExtractSystemInfo(void)
{
	// Hardware information
	GetSystemInfo(&InitialHardwareSetup);
	// Extended version information
	Win95Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&Win95Version);
}

unsigned long
RAMPageSize(void)
{
	return InitialHardwareSetup.dwPageSize;
}

#define OS_IMPLEMENTED
#endif	// _WIN32

#if !defined(OS_IMPLEMENTED)
#error OS.cxx has not been implemented for the target platform.
#endif
