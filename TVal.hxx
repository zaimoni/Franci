// TVal.hxx
#ifndef TVAL_HXX
#define TVAL_HXX

#include <stddef.h>
#include <string>

class TVal final
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

	static constexpr const char* const Names[] = { "CONTRADICTION",
												   "TRUE",
												   "FALSE",
												   "UNKNOWN" };

	constexpr TVal() noexcept : _x(Unknown) {}
	constexpr TVal(Flags src) noexcept : _x(src) {}
	constexpr TVal(bool src) noexcept : _x(src ? True : False) {}
	~TVal() = default;
	TVal(const TVal& src) = default;
	TVal(TVal&& src) = default;
	TVal& operator=(const TVal & src) = default;
	TVal& operator=(TVal&& src) = default;
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

	constexpr bool is(Flags x) const {return _x==x;};
	constexpr bool could_be(Flags x) const {return _x & x;};
	void force(Flags x) {_x &= x;};

	constexpr bool is(bool x) const {return _x==(x ? True : False);};
	constexpr bool could_be(bool x) const {return _x & (x ? True : False);};
	void force(bool x) {_x &= (x ? True : False);};

	constexpr size_t array_index() const {return _x;}
	constexpr size_t array_index(const TVal& rhs) const {return 4*_x+rhs._x;};

	std::string to_s() const;
	size_t LengthOfSelfName() const;
	void ConstructSelfNameAux(char* Name) const;

	void SelfLogicalNOT();
	bool isAntiIdempotentTo(const TVal& rhs) const;

	// to help parsing
	bool read(const char* src);
	static bool is_legal(const char* Text);
};

static_assert(sizeof(TVal) == sizeof(unsigned char));

#endif
