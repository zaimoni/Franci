// TVal.hxx
#ifndef TVAL_HXX
#define TVAL_HXX

class TVal
{
private:
	unsigned char _x;
public:
	enum Flags	{
			Contradiction	= 0,
			True,
			False,
			Unknown
			};
	TVal() : _x(Unknown) {};
	TVal(Flags src) : _x(src) {};
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
	bool read(const char* src);

	bool is(Flags x) const {return _x==x;};
	bool could_be(Flags x) const {return _x & x;};
	void force(Flags x) {_x &= x;};

	size_t array_index() const {return _x;}
	size_t array_index(const TVal& rhs) const {return 4*_x+rhs._x;};
};

#endif
