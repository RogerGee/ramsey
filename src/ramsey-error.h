/* ramsey-error.h - CS355 Compiler Project */
#ifndef RAMSEY_ERROR_H
#define RAMSEY_ERROR_H
#include <exception>
#include <string>
#include <cstdarg>

namespace ramsey
{
    class compiler_error_generic : public std::exception
    {
    public:
        virtual ~compiler_error_generic() throw() {}

        virtual const char* what() const throw()
        { return _err.c_str(); }
    protected:
        compiler_error_generic() {}

        void _cstor(const char* format,va_list vargs);
    private:
        const std::string _err;
    };

}

#endif
