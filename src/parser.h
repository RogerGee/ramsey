/* parser.h - CS355 Compiler Project */
#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"

namespace ramsey
{
    // exception types
    class parser_error_generic : public std::exception
    {
    public:
        virtual ~parser_error_generic() throw() {}

        virtual const char* what() const throw()
        { return _err.c_str(); }
    protected:
        parser_error_generic() {}

        void _cstor(const char* s)
        { const_cast<std::string&>(_err) = s; }
    private:
        const std::string _err;
    };
    class parser_error : public parser_error_generic // errors reported to user
    {
    public:
        parser_error(const char* format, ...);
        virtual ~parser_error() throw() {}
    };
    class parser_exception : public parser_error_generic // errors used internally
    {
    public:
        parser_exception(const char* format, ...);
        virtual ~parser_exception() throw() {}
    };
    
    class parser
    {
    public:
        parser(const char* file);
    private:
        lexer lex;
        //parse tree representation
    };
    
}

#endif
