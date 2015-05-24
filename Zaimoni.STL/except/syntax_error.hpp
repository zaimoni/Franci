// syntax_error.hpp
// header for syntax error exception

#ifndef ZAIMONI_STL_EXCEPT_SYNTAX_ERROR_HPP
#define ZAIMONI_STL_EXCEPT_SYNTAX_ERROR_HPP 1

#include <stdexcept>

namespace zaimoni {

class syntax_error : public std::runtime_error
{
public:
    explicit syntax_error(const std::string&  error_message)
	: std::runtime_error(error_message)
	{};
};

}

#endif
