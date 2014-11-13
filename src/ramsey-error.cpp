/* ramsey-error.cpp */
#include "ramsey-error.h"
using namespace ramsey;

void compiler_error_generic::_cstor(const char* format,va_list vargs)
{
    static const size_t BUFMAX = 4096;
    char buffer[BUFMAX];
    vsnprintf(buffer,BUFMAX,format,vargs);
    const_cast<std::string&>(_err) = buffer;
}
