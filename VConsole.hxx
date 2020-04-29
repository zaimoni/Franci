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
	~VConsole() = default;

protected:
	void LinkInScripting(void) override;
	bool ScanForStartLogFileBlock(void) override;
	void ShrinkBlock(unsigned long StartBlock, unsigned long EndBlock, unsigned long& ReviewedPoint,size_t LogLength) override;
};

#endif
