/* parser.cpp */
#include "parser.h"
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <cstring>
#include <cctype>
using namespace std;
using namespace ramsey;

// exception types
parser_error::parser_error(const char* format, ...)
{
    static const size_t BUFMAX = 4096;
    va_list args;
    char buffer[BUFMAX];
    va_start(args,format);
    vsnprintf(buffer,BUFMAX,format,args);
    va_end(args);
    _cstor(buffer);
}
parser_exception::parser_exception(const char* format, ...)
{
    static const size_t BUFMAX = 4096;
    va_list args;
    char buffer[BUFMAX];
    va_start(args,format);
    vsnprintf(buffer,BUFMAX,format,args);
    va_end(args);
    _cstor(buffer);
}

// ramsey::parser

parser::parser(const char* file)
    : linenumber(1), lex(file)
{
    program();
}

bool parser::eol()    // eat all endlines and return whether there were any
{
    bool ans = false;
    while (!lex.endtok() && lex.curtok().type() == token_eol)
    {
        ans = true;
        ++lex;
        linenumber++;
    }
    return ans;
}

// recursive call definitions

void parser::program()
{
    function_list();
}

void parser::function_list()
{
    eol();
    if (lex.endtok())
        return;
    else if (lex.curtok().type() == token_fun)
    {
        function();
        function_list();
    }
    else
        throw parser_error("line %d: stray %s outside function body", linenumber, lex.curtok().to_string().c_str());
}

void parser::function()
{
    function_declaration();
    statement_list();
    if (lex.curtok().type() == token_endfun)
        ++lex;
    else
        throw parser_error("line %d: expected 'endfun' after function body", linenumber);
    if (!eol())
        throw parser_error("line %d: expected newline after function", linenumber);
}

void parser::function_declaration()
{
    ++lex;
    if (lex.curtok().type() == token_id)
        ++lex;
    else
        throw parser_error("line %d: expected identifier in function declaration", linenumber);
    if (lex.curtok().type() == token_oparen)
        ++lex;
    else
        throw parser_error("line %d: expected '(' after function name", linenumber);
    parameter_declaration();
    if (lex.curtok().type() == token_cparen)
        ++lex;
    else
        throw parser_error("line %d: expected ')' after function declaration", linenumber);
    function_type_specifier();
    if (!eol())
        throw parser_error("line %d: expected newline after function declaration", linenumber);
}

void parser::function_type_specifier()
{
    if (lex.curtok().type() == token_as)
    {
        ++lex;
        type_name();
    }
    else if (lex.curtok().type() == token_eol)
        return;
    else
        throw parser_error("line %d: bad function type specifier", linenumber);
}

void parser::parameter_declaration()
{
    if (lex.curtok().type() == token_in || lex.curtok().type() == token_boo)
    {
        parameter();
        parameter_list();
    }
    else if (lex.curtok().type() == token_cparen)
        return;
    else
        throw parser_error("line %d: bad parameter declaration", linenumber);
}

void parser::parameter()
{
    type_name();
    if (lex.curtok().type() == token_id)
        ++lex;
    else
        throw parser_error("line %d: missing parameter name", linenumber);
}

void parser::parameter_list()
{
    if (lex.curtok().type() == token_comma)
    {
        ++lex;
        parameter();
        parameter_list();
    }
    else if (lex.curtok().type() == token_cparen)
        return;
    else
        throw parser_error("line %d: unexpected token in parameter list '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::statement()
{
    if (lex.curtok().type() == token_in || lex.curtok().type() == token_boo)
        declaration_statement();
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_not
        || lex.curtok().type() == token_id || lex.curtok().type() == token_bool_true
        || lex.curtok().type() == token_bool_false || lex.curtok().type() == token_number
        || lex.curtok().type() == token_number_hex || lex.curtok().type() == token_string
        || lex.curtok().type() == token_oparen)
        expression_statement();
    else if (lex.curtok().type() == token_if)
        selection_statement();
    else if (lex.curtok().type() == token_while)
        iterative_statement();
    else if (lex.curtok().type() == token_toss || lex.curtok().type() == token_smash)
        jump_statement();
    else
        throw parser_error("line %d: malformed statement", linenumber);
}

void parser::statement_list()
{
    if (lex.curtok().type() == token_in || lex.curtok().type() == token_boo ||
        lex.curtok().type() == token_id || lex.curtok().type() == token_number ||
        lex.curtok().type() == token_number_hex || lex.curtok().type() == token_bool_true ||
        lex.curtok().type() == token_bool_false || lex.curtok().type() == token_string ||
        lex.curtok().type() == token_oparen || lex.curtok().type() == token_if ||
        lex.curtok().type() == token_while || lex.curtok().type() == token_toss ||
        lex.curtok().type() == token_smash)
    {
        statement();
        statement_list();
    }
    else if (lex.curtok().type() == token_else || lex.curtok().type() == token_elf ||
        lex.curtok().type() == token_endif || lex.curtok().type() == token_endfun ||
        lex.curtok().type() == token_endwhile)
        return;
    else
        throw parser_error("line %d: expected end-construct after statement", linenumber);
}

void parser::declaration_statement()
{
    type_name();
    if (lex.curtok().type() == token_id)
        ++lex;
    else
        throw parser_error("line %d: expected identifier in declaration statement", linenumber);
    initializer();
    if (!eol())
        throw parser_error("line %d: newline expected after declaration statement", linenumber);
}

void parser::type_name()
{
    if (lex.curtok().type() == token_in)
        ++lex;
    else if (lex.curtok().type() == token_boo)
        ++lex;
    else
        throw parser_error("line %d: expected typename specifier", linenumber);
}

void parser::initializer()
{
    if (lex.curtok().type() == token_assign)
    {
        assignment_operator();
        expression();
    }
    else if (lex.curtok().type() == token_eol)
        return;
    else
        throw parser_error("line %d: malformed initializer", linenumber);
}

void parser::assignment_operator()
{
    if (lex.curtok().type() == token_assign)
        ++lex;
    else
        throw parser_error("line %d: expected assignment operator in expression",linenumber);
    // others would go here, if we decide to implement them
}

void parser::expression_statement()
{
    expression_list();
    if (!eol())
        throw parser_error("line %d: expected newline after expression statement", linenumber);
}

void parser::expression()
{
    assignment_expression();
}

void parser::expression_list()
{
    expression();
    expression_list_item();
}

void parser::expression_list_item()
{
    if (lex.curtok().type() == token_comma)
    {
        ++lex;
        expression_list();
    }
    else if (lex.curtok().type() == token_eol || lex.curtok().type() == token_cparen)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::assignment_expression()
{
    logical_or_expression();
    assignment_expression_opt();
}

void parser::assignment_expression_opt()
{
    if (lex.curtok().type() == token_assign)
    {
        assignment_operator();
        assignment_expression();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::logical_or_expression()
{
    logical_and_expression();
    logical_or_expression_opt();
}

void parser::logical_or_expression_opt()
{
    if (lex.curtok().type() == token_or)
    {
        ++lex;
        logical_or_expression();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::logical_and_expression()
{
    equality_expression();
    logical_and_expression_opt();
}

void parser::logical_and_expression_opt()
{
    if (lex.curtok().type() == token_and)
    {
        ++lex;
        logical_and_expression();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::equality_expression()
{
    relational_expression();
    equality_expression_opt();
}

void parser::equality_expression_opt()
{
    if (lex.curtok().type() == token_equal)
    {
        ++lex;
        equality_expression();
    }
    else if (lex.curtok().type() == token_nequal)
    {
        ++lex;
        equality_expression();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::relational_expression()
{
    additive_expression();
    relational_expression_opt();
}

void parser::relational_expression_opt()
{
    if (lex.curtok().type() == token_less)
    {
        ++lex;
        relational_expression();
    }
    else if (lex.curtok().type() == token_greater)
    {
        ++lex;
        relational_expression();
    }
    else if (lex.curtok().type() == token_le)
    {
        ++lex;
        relational_expression();
    }
    else if (lex.curtok().type() == token_ge)
    {
        ++lex;
        relational_expression();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and ||
        lex.curtok().type() == token_equal || lex.curtok().type() == token_nequal)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::additive_expression()
{
    multiplicative_expression();
    additive_expression_opt();
}

void parser::additive_expression_opt()
{
    if (lex.curtok().type() == token_add)
    {
        ++lex;
        additive_expression();
    }
    else if (lex.curtok().type() == token_subtract)
    {
        ++lex;
        additive_expression();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and ||
        lex.curtok().type() == token_equal || lex.curtok().type() == token_nequal ||
        lex.curtok().type() == token_less || lex.curtok().type() == token_greater ||
        lex.curtok().type() == token_le || lex.curtok().type() == token_ge)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::multiplicative_expression()
{
    prefix_expression();
    multiplicative_expression_opt();
}

void parser::multiplicative_expression_opt()
{
    if (lex.curtok().type() == token_multiply)
    {
        ++lex;
        multiplicative_expression();
    }
    else if (lex.curtok().type() == token_divide)
    {
        ++lex;
        multiplicative_expression();
    }
    else if (lex.curtok().type() == token_mod)
    {
        ++lex;
        multiplicative_expression();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and ||
        lex.curtok().type() == token_equal || lex.curtok().type() == token_nequal ||
        lex.curtok().type() == token_less || lex.curtok().type() == token_greater ||
        lex.curtok().type() == token_le || lex.curtok().type() == token_ge ||
        lex.curtok().type() == token_add || lex.curtok().type() == token_subtract)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::prefix_expression()
{
    if (lex.curtok().type() == token_id || lex.curtok().type() == token_number ||
        lex.curtok().type() == token_number_hex || lex.curtok().type() == token_bool_true ||
        lex.curtok().type() == token_bool_false || lex.curtok().type() == token_oparen)
        postfix_expression();
    else if (lex.curtok().type() == token_not)
    {
        ++lex;
        prefix_expression();
    }
}

void parser::postfix_expression()
{
    primary_expression();
    postfix_expression_opt();
}

void parser::postfix_expression_opt()
{
    if (lex.curtok().type() == token_oparen)
    {
        ++lex;
        expression_list();
        if (lex.curtok().type() == token_cparen)
            ++lex;
        else
            throw parser_error("line %d: expected ')' in function call", linenumber);
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and ||
        lex.curtok().type() == token_equal || lex.curtok().type() == token_nequal ||
        lex.curtok().type() == token_less || lex.curtok().type() == token_greater ||
        lex.curtok().type() == token_le || lex.curtok().type() == token_ge ||
        lex.curtok().type() == token_add || lex.curtok().type() == token_subtract ||
        lex.curtok().type() == token_multiply || lex.curtok().type() == token_divide ||
        lex.curtok().type() == token_mod)
        return;
    else
        throw parser_error("line %d: malformed expression", linenumber);
}

void parser::primary_expression()
{
    if (lex.curtok().type() == token_number || lex.curtok().type() == token_number_hex ||
        lex.curtok().type() == token_bool_true || lex.curtok().type() == token_bool_false)
        ++lex;
    else if (lex.curtok().type() == token_id)
        ++lex;
    else if (lex.curtok().type() == token_oparen)
    {
        ++lex;
        expression();
        if (lex.curtok().type() == token_cparen)
            ++lex;
        else
            throw parser_error("line %d: expected ')' in primary expression", linenumber);
    }
    else
        throw parser_error("line %d: expected primary-id", linenumber);
}

void parser::selection_statement()
{
    ++lex;
    if (lex.curtok().type() == token_oparen)
        ++lex;
    else
        throw parser_error("line %d: '(' must follow 'if'", linenumber);
    expression();
    if (lex.curtok().type() == token_cparen)
        ++lex;
    else
        throw parser_error("line %d: expected ')' after if-statement condition", linenumber);
    if (!eol())
        throw parser_error("line %d: expected newline after if-statement condition", linenumber);
    if_body();
    if_concluder();
}

void parser::if_body()
{
    statement_list();
    elf_body();
}

void parser::elf_body()
{
    if (lex.curtok().type() == token_elf)
    {
        ++lex;
        if (lex.curtok().type() == token_oparen)
            ++lex;
        else
            throw parser_error("line %d: expected '(' after 'elf'", linenumber);
        expression();
        if (lex.curtok().type() == token_cparen)
            ++lex;
        else
            throw parser_error("line %d: expected ')' after elf-statement condition", linenumber);
        if (!eol())
            throw parser_error("line %d: expected newline after elf-statement condition", linenumber);
        if_body();
    }
    else if (lex.curtok().type() == token_else || lex.curtok().type() == token_endif)
        return;
    else
        throw parser_error("line %d: expected 'elf', 'else' or 'endif' after if-statement body", linenumber);
}

void parser::if_concluder()
{
    if (lex.curtok().type() == token_endif)
    {
        ++lex;
        if (!eol())
            throw parser_error("line %d: expected newline after 'endif'", linenumber);
    }
    else if (lex.curtok().type() == token_else)
    {
        ++lex;
        statement_list();
        if (lex.curtok().type() == token_endif)
            ++lex;
        else
            throw parser_error("line %d: expected 'endif' after else block", linenumber);
        if (!eol())
            throw parser_error("line %d: expected newline after 'endif'", linenumber);
    }
    else
        throw parser_error("line %d: expected 'else' or 'endif'", linenumber);
}

void parser::iterative_statement()
{
    ++lex;
    if (lex.curtok().type() == token_oparen)
        ++lex;
    else
        throw parser_error("line %d: expected '(' after iterative", linenumber);
    expression();
    if (lex.curtok().type() == token_cparen)
        ++lex;
    else
        throw parser_error("line %d: expected ')' after iterative condition", linenumber);
    if (!eol())
        throw parser_error("line %d: expected newline after iterative condition", linenumber);
    statement_list();
    if (lex.curtok().type() == token_endwhile)
        ++lex;
    else
        throw parser_error("line %d: expected 'endwhile'", linenumber);
    if (!eol())
        throw parser_error("line %d: expected newline after 'endwhile'", linenumber);
}

void parser::jump_statement()
{
    if (lex.curtok().type() == token_toss)
    {
        ++lex;
        expression_list();
        if (!eol())
            throw parser_error("line %d: expected newline after 'toss'", linenumber);
    }
    else if (lex.curtok().type() == token_smash)
    {
        ++lex;
        if (!eol())
            throw parser_error("line %d: expected newline after 'smash'", linenumber);
    }
    // this shouldn't run since where jump-statement is called
    // we check curtok==token_smash || curtok==token_toss
    else
        throw parser_error("line %d: malformed jump statement", linenumber);
}
