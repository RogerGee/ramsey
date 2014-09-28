/* lexer.h - CS355 Compiler Project */
#ifndef LEXER_H
#define LEXER_H
#include <exception>
#include <string>
#include <vector>

namespace ramsey
{
    // exception types
    class lexer_error_generic : public std::exception
    {
    public:
        virtual ~lexer_error_generic() throw() {}

        virtual const char* what() const throw()
        { return _err.c_str(); }
    protected:
        lexer_error_generic() {}

        void _cstor(const char* s)
        { const_cast<std::string&>(_err) = s; }
    private:
        const std::string _err;
    };
    class lexer_error : public lexer_error_generic // errors reported to user
    {
    public:
        lexer_error(const char* format, ...);
        virtual ~lexer_error() throw() {}
    };
    class lexer_exception : public lexer_error_generic // errors used internally
    {
    public:
        lexer_exception(const char* format, ...);
        virtual ~lexer_exception() throw() {}
    };

    // define lexical token types
    enum token_t
    {
        token_invalid = -1,

        // basic types
        token_id, // user-defined identifier
        token_number, // integer literal
        token_number_hex, // integer literal (uses hexadecimal rep)
        token_string, // string literal
        token_bool_true, // boolean literals
        token_bool_false,

        // operators and punctuators
        token_add, // '+'
        token_subtract,  // '-'
        token_multiply, // '*'
        token_divide, // '/'
        token_assign, // '<-' (assignment operator)
        token_oparen, // '('
        token_cparen, // ')'
        token_equal, // '='
        token_nequal, // '!='
        token_less, // '<'
        token_greater, // '>'
        token_le, // '<='
        token_ge, // '>='
        //token_lscript, // '[' (left sub-scripting operator) (removed from language)
        //token_rscript, // ']' (right sub-scripting operator)
        token_comma, // ','

        // keywords
        token_in, // the typename 'in' (meaning integer)
        token_boo, // the typename 'boo' (meaning boolean)
        token_if, // 'if'
        token_elf, // 'elf' (meaning else if)
        token_endif, // 'endif'
        token_while, // 'while'
        token_smash, // 'smash'
        token_endwhile, // 'endwhile'
        token_fun, // 'fun' (function declaration)
        token_as, // 'as' (defines return type of function)
        token_endfun, // 'endfun' (end of function)
        token_toss, // 'toss' (return statement)
        //token_take, // 'take' (input operator) (removed from language)
        //token_give, // 'give' (output operator)
        token_mod, // the keyword operator 'mod'
        token_or, // the keyword operator 'or'
        token_and, // the keyword operator 'and'
        token_not, // the keyword operator 'not'

        // delimiter
        token_eol // '\n' (we have to denote the end of a statement)
    };

    // represents a lexical token
    class token
    {
    public:
        token(token_t kind);
        token(token_t kind,const char* source);
        token(token_t kind,const char* source,int length);
        token(const token&);
        ~token();

        token& operator =(const token&);

        token_t type() const
        { return _tkind; }
        const char* source_string() const
        { return _tsource; }

#ifdef RAMSEY_DEBUG
        // get human-readable strings describing the token (for testing)
        const char* to_string_kind() const; // just the kind
        std::string to_string() const; // kind plus any payload
#endif
    private:
        token_t _tkind;
        char* _tsource;

        void _alloc(const char* source,int length);
    };
    
    // the lexer class, reads the file and creates a stream of tokens
    /*
      (trying to follow your conventions as much as I can)
      this is just a starting point, it will certainly need more functions
    */
    class lexer
    {
    public:
        lexer(const char* file);
        void read(/*file*/);    //read in the file (or perhaps we do that on construction?)
        void tokenize();        //convert the char stream into a token stream / throw errors

        // provide means to access tokens
        const token& curtok() const
        { return *_iter; }
        bool endtok() const
        { return _iter == _stream.end(); }
        lexer& operator ++();
        lexer operator ++(int);
    private:
        //perhaps a list instead? we'll discuss at some point.
        //however, I do feel we ought to use stl where we can
        std::vector<token> _stream;
        std::vector<token>::const_iterator _iter;
        std::string _input;

        // preprocessing token flags
        enum {
            ptoken_identifier,
            ptoken_number,
            ptoken_number_hex,
            ptoken_string,
            ptoken_puncop,
            ptoken_eol
        };
        struct ptoken
        {
            int kind;
            std::string payload;
        };

        static void _preprocess(const char* file,std::vector<ptoken>& ptoks); // compile preprocessing tokens
        void _convert(std::vector<ptoken>& ptoks); // convert preprocessing tokens to lexical tokens

        // helpers for character classes
        static inline bool ishexletter(int ch);
        static inline bool isoppunc(int ch);
    };
}

#endif
