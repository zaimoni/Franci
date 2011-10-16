// VConsole.hxx
// header for VConsole, which manages the text-window interface for Franci
// this is OS-sensitive

#ifndef VCONSOLE_DEF
#define VCONSOLE_DEF

#include "Zaimoni.STL/OS/console.hpp"

class VConsole : public Console
{
public:
	VConsole();
	virtual ~VConsole();

protected:
	virtual void LinkInScripting(void);
	virtual bool ScanForStartLogFileBlock(void);
	virtual void ShrinkBlock(unsigned long StartBlock, unsigned long EndBlock, unsigned long& ReviewedPoint,size_t LogLength);
};

#endif
