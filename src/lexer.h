/* lexer.h - CS355 Compiler Project */
#ifndef LEXER_H
#define LEXER_H
#include <exception>
#include <string>

namespace ramsey
{
    // exception type for lexer errors
    class lexer_error : public std::exception
    {
    public:
        lexer_error(const char* format, ...);
        virtual ~lexer_error() throw() {}

        virtual const char* what() const throw()
        { return _err.c_str(); }
    private:
        std::string _err;
    };

    // define token types
    enum token_t
    {
        token_invalid = -1,

        // basic types
        token_id, // user-defined identifier
        token_number, // integer literal
        token_string, // string literal

        // operators
        token_add, // '+'
        token_subtract,  // '-' 
        token_multiply, // '*'
        token_divide, // '/'
        token_mod, // the keyword operator 'mod'

        // keywords
        token_in, // the typename 'in' (meaning integer)
        token_boo, // the typename 'boo' (meaning boolean)
        token_if, // 'if'
        token_elf, // 'elf' (meaning else if)
        token_endif // 'endif'
    };

    // represents a lexical token
    class token
    {
    public:
        token(token_t kind);
        token(token_t kind,const char* source,int length);
        ~token();

        token_t type() const
        { return _tkind; }
        const char* source_string() const
        { return _tsource; }
    private:
        token_t _tkind;
        char* _tsource;
    };
}

#endif
