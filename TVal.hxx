// TVal.hxx
#ifndef TVAL_HXX
#define TVAL_HXX

#include <stddef.h>

class TVal
{
private:
	unsigned char _x;
public:
	friend bool operator==(const TVal& lhs, const TVal& rhs) {return lhs._x==rhs._x;}
	friend bool operator<(const TVal& lhs, const TVal& rhs) {return lhs._x<rhs._x;}
	friend TVal operator&&(const TVal& lhs, const TVal& rhs);

	enum Flags	{
			Contradiction	= 0,
			True,
			False,
			Unknown
			};
	TVal() : _x(Unknown) {};
	TVal(Flags src) : _x(src) {};
	TVal(bool src) : _x(src ? True : False) {};
	// final class, so default destructor OK
	// likewise default assignment and copy constructors OK
	const TVal& operator=(bool src)
		{
		_x = (src ? True : False);
		return *this;
		};
	const TVal& operator=(Flags src)
		{
		_x = src;
		return *this;
		};

	bool is(Flags x) const {return _x==x;};
	bool could_be(Flags x) const {return _x & x;};
	void force(Flags x) {_x &= x;};

	bool is(bool x) const {return _x==(x ? True : False);};
	bool could_be(bool x) const {return _x & (x ? True : False);};
	void force(bool x) {_x &= (x ? True : False);};

	size_t array_index() const {return _x;}
	size_t array_index(const TVal& rhs) const {return 4*_x+rhs._x;};

	size_t LengthOfSelfName() const;
	void ConstructSelfNameAux(char* Name) const;

	void SelfLogicalNOT();
	bool isAntiIdempotentTo(const TVal& rhs) const;

	// to help parsing
	bool read(const char* src);
	static bool is_legal(const char* Text);
};

#endif
