/* lexer.cpp */
#include "lexer.h"
#include <cstdio>
#include <cstdarg>
using namespace std;
using namespace ramsey;

// lexer_error
lexer_error::lexer_error(const char* format, ...)
{
    static const size_t BUFMAX = 4096;
    va_list args;
    char buffer[BUFMAX];
    va_start(args,format);
    vsnprintf(buffer,BUFMAX,format,args);
    va_end(args);
    _err = buffer;
}

// token
token::token(token_t kind)
    : _tkind(kind), _tsource(NULL)
{
}
token::token(token_t kind,const char* source,int length)
    : _tkind(kind)
{
    int i;
    if (length < 0)
        throw lexer_error("exception: negative length specified for token source string");
    // allocate a buffer to store the source string
    _tsource = new char[length+1]; // (account for the null character)
    // copy the source string to the buffer
    for (i = 0;i < length;++i)
        _tsource[i] = source[i];
    _tsource[i] = 0;
}
token::~token()
{
    if (_tsource != NULL)
        delete[] _tsource;
}
