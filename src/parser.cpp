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

// declare recursive functions
void program(lexer& lex);
void function_list(lexer& lex);
void function(lexer& lex);
void function_declaration(lexer& lex);
void function_type_specifier(lexer& lex);
void parameter_declaration(lexer& lex);
void parameter(lexer& lex);
void parameter_list(lexer& lex);
void statement(lexer& lex);
void statement_list(lexer& lex);
void declaration_statement(lexer& lex);
void type_name(lexer& lex);
void initializer(lexer& lex);
void assignment_operator(lexer& lex);
void expression_statement(lexer& lex);
void expression(lexer& lex);
void expression_list(lexer& lex);
void expression_list_item(lexer& lex);
void assignment_expression(lexer& lex);
void assignment_expression_opt(lexer& lex);
void logical_or_expression(lexer& lex);
void logical_or_expression_opt(lexer& lex);
void logical_and_expression(lexer& lex);
void logical_and_expression_opt(lexer& lex);
void equality_expression(lexer& lex);
void equality_expression_opt(lexer& lex);
void relational_expression(lexer& lex);
void relational_expression_opt(lexer& lex);
void additive_expression(lexer& lex);
void additive_expression_opt(lexer& lex);
void multiplicative_expression(lexer& lex);
void multiplicative_expression_opt(lexer& lex);
void prefix_expression(lexer& lex);
void postfix_expression(lexer& lex);
void postfix_expression_opt(lexer& lex);
void primary_expression(lexer& lex);
void selection_statement(lexer& lex);
void if_body(lexer& lex);
void elf_body(lexer& lex);
void if_concluder(lexer& lex);
void iterative_statement(lexer& lex);
void jump_statement(lexer& lex);

parser::parser(const char* file) : lex(file)
{
    program(lex);
}

// recursive call definitions

void program(lexer& lex)
{
    function_list(lex);
}

void function_list(lexer& lex)
{
    if (lex.endtok())
        return;
    else if (lex.curtok().type() == token_fun)
    {
        function(lex);
        function_list(lex);
    }
    else
        throw parser_error("stray token outside function");
}

void function(lexer& lex)
{
    function_declaration(lex);
    statement_list(lex);
    if (lex.curtok().type() == token_endfun)
        lex++;
    else
        throw parser_error("endfun expected.");
    if (lex.curtok().type() == token_eol)
        lex++;
    else
        throw parser_error("endline expected.");
}

void function_declaration(lexer& lex)
{
    lex++;
    if (lex.curtok().type() == token_id)
        lex++;
    else
        throw parser_error("identifier expected.");
    if (lex.curtok().type() == token_oparen)
        lex++;
    else
        throw parser_error("open paren expected.");
    parameter_declaration(lex);
    if (lex.curtok().type() == token_cparen)
        lex++;
    else
        throw parser_error("close paren expected.");
    function_type_specifier(lex);
    if (lex.curtok().type() == token_eol)
        lex++;
    else
        throw parser_error("end line expected.");
}

void function_type_specifier(lexer& lex)
{
    if (lex.curtok().type() == token_as)
    {
        lex++;
        type_name(lex);
    }
    else if (lex.curtok().type() == token_eol)
        return;
    else
        throw parser_error("Improperly formatted type specifier.");
}

void parameter_declaration(lexer& lex)
{
    if (lex.curtok().type() == token_in or lex.curtok().type() == token_boo)
    {
        parameter(lex);
        parameter_list(lex);
    }
    else if (lex.curtok().type() == token_cparen)
        return;
    else
        throw parser_error("Improperly formatted parameter declaration.");
}

void parameter(lexer& lex)
{
    type_name(lex);
    if (lex.curtok().type() == token_id)
        lex++;
    else
        throw parser_error("Missing parameter name.");
}

void parameter_list(lexer& lex)
{
    if (lex.curtok().type() == token_comma)
    {
        parameter(lex);
        parameter_list(lex);
    }
    else if (lex.curtok().type() == token_cparen)
        return;
    else
        throw parser_error("Unexpected token in parameter list");
}

void statement(lexer& lex)
{
    if (lex.curtok().type() == token_in or lex.curtok().type() == token_boo)
        declaration_statement(lex);
    else if (lex.curtok().type() == token_cparen or lex.curtok().type() == token_not
        or lex.curtok().type() == token_id or lex.curtok().type() == token_bool_true
        or lex.curtok().type() == token_bool_false or lex.curtok().type() == token_number
        or lex.curtok().type() == token_number_hex or lex.curtok().type() == token_string
        or lex.curtok().type() == token_oparen)
        expression_statement(lex);
    else if (lex.curtok().type() == token_if)
        selection_statement(lex);
    else if (lex.curtok().type() == token_while)
        iterative_statement(lex);
    else if (lex.curtok().type() == token_toss or lex.curtok().type() == token_smash)
        jump_statement(lex);
    else
        throw parser_error("Incorrect statement start.");
}

void statement_list(lexer& lex)
{
    if (lex.curtok().type() == token_in or lex.curtok().type() == token_boo or
        lex.curtok().type() == token_id or lex.curtok().type() == token_number or
        lex.curtok().type() == token_number_hex or lex.curtok().type() == token_bool_true or
        lex.curtok().type() == token_bool_false or lex.curtok().type() == token_string or
        lex.curtok().type() == token_oparen or lex.curtok().type() == token_if or
        lex.curtok().type() == token_while or lex.curtok().type() == token_toss or
        lex.curtok().type() == token_smash)
    {
        statement(lex);
        statement_list(lex);
    }
    else if (lex.curtok().type() == token_else or lex.curtok().type() == token_elf or
        lex.curtok().type() == token_endif or lex.curtok().type() == token_endfun or
        lex.curtok().type() == token_endwhile)
        return;
    else
        throw parser_error("Incorrect statement end.");
}

void declaration_statement(lexer& lex)
{
    type_name(lex);
    if (lex.curtok().type() == token_id)
        lex++;
    else
        throw parser_error("Identifier expected.");
    initializer(lex);
    if (lex.curtok().type() == token_eol)
        lex++;
    else
        throw parser_error("End line expected.");
}

void type_name(lexer& lex)
{
    if (lex.curtok().type() == token_in or lex.curtok().type() == token_boo)
        lex++;
    else
        throw parser_error("Typename specifier expected.");
}

void initializer(lexer& lex)
{
    if (lex.curtok().type() == token_assign)
    {
        assignment_operator(lex);
        expression(lex);
    }
    else if (lex.curtok().type() == token_eol)
        return;
    else
        throw parser_error("Improperly formatted initializer.");
}

void assignment_operator(lexer& lex)
{
    if (lex.curtok().type() == token_assign)
        lex++;
    // others would go here, if we so decide to implement them
}

void expression_statement(lexer& lex)
{
    expression_list(lex);
    if (lex.curtok().type() == token_eol)
        lex++;
    else
        throw parser_error("End line expected.");
}

void expression(lexer& lex)
{
    assignment_expression(lex);
}

void expression_list(lexer& lex)
{
    expression(lex);
    expression_list_item(lex);
}

void expression_list_item(lexer& lex)
{
    if (lex.curtok().type() == token_comma)
    {
        lex++;
        expression_list(lex);
    }
    else if (lex.curtok().type() == token_eol)
        return;
    else
        throw parser_error("improperly formatted expression");
}

void assignment_expression(lexer& lex)
{
    logical_or_expression(lex);
    assignment_expression_opt(lex);
}

void assignment_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_assign)
    {
        assignment_operator(lex);
        assignment_expression(lex);
    }
    else if (lex.curtok().type() == token_comma)
        return;
    else
        throw parser_error("error");
}

void logical_or_expression(lexer& lex)
{
    logical_and_expression(lex);
    logical_or_expression_opt(lex);
}

void logical_or_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_or)
    {
        lex++;
        logical_or_expression(lex);
    }
    else if (lex.curtok().type() == token_comma or lex.curtok().type() == token_assign)
        return;
    else
        throw parser_error("error");
}

void logical_and_expression(lexer& lex)
{
    equality_expression(lex);
    logical_and_expression_opt(lex);
}

void logical_and_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_and)
    {
        lex++;
        logical_and_expression(lex);
    }
    else if (lex.curtok().type() == token_comma or lex.curtok().type() == token_assign or
        lex.curtok().type() == token_or)
        return;
    else
        throw parser_error("error");
}

void equality_expression(lexer& lex)
{
    relational_expression(lex);
    equality_expression_opt(lex);
}

void equality_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_equal)
    {
        lex++;
        equality_expression(lex);
    }
    else if (lex.curtok().type() == token_nequal)
    {
        lex++;
        equality_expression(lex);
    }
    else if (lex.curtok().type() == token_comma or lex.curtok().type() == token_assign or
        lex.curtok().type() == token_or or lex.curtok().type() == token_and)
        return;
    else
        throw parser_error("error");
}

void relational_expression(lexer& lex)
{
    additive_expression(lex);
    relational_expression_opt(lex);
}

void relational_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_less)
    {
        lex++;
        relational_expression(lex);
    }
    else if (lex.curtok().type() == token_greater)
    {
        lex++;
        relational_expression(lex);
    }
    else if (lex.curtok().type() == token_le)
    {
        lex++;
        relational_expression(lex);
    }
    else if (lex.curtok().type() == token_ge)
    {
        lex++;
        relational_expression(lex);
    }
    else if (lex.curtok().type() == token_comma or lex.curtok().type() == token_assign or
        lex.curtok().type() == token_or or lex.curtok().type() == token_and or
        lex.curtok().type() == token_equal or lex.curtok().type() == token_nequal)
        return;
    else
        throw parser_error("error");
}

void additive_expression(lexer& lex)
{
    multiplicative_expression(lex);
    additive_expression(lex);
}

void additive_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_add)
    {
        lex++;
        additive_expression(lex);
    }
    else if (lex.curtok().type() == token_subtract)
    {
        lex++;
        additive_expression(lex);
    }
    else if (lex.curtok().type() == token_comma or lex.curtok().type() == token_assign or
        lex.curtok().type() == token_or or lex.curtok().type() == token_and or
        lex.curtok().type() == token_equal or lex.curtok().type() == token_nequal or
        lex.curtok().type() == token_less or lex.curtok().type() == token_greater or
        lex.curtok().type() == token_le or lex.curtok().type() == token_ge)
        return;
    else
        throw parser_error("error");
}

void multiplicative_expression(lexer& lex)
{
    prefix_expression(lex);
    multiplicative_expression_opt(lex);
}

void multiplicative_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_multiply)
    {
        lex++;
        multiplicative_expression(lex);
    }
    else if (lex.curtok().type() == token_divide)
    {
        lex++;
        multiplicative_expression(lex);
    }
    else if (lex.curtok().type() == token_comma or lex.curtok().type() == token_assign or
        lex.curtok().type() == token_or or lex.curtok().type() == token_and or
        lex.curtok().type() == token_less or lex.curtok().type() == token_greater or
        lex.curtok().type() == token_le or lex.curtok().type() == token_ge or
        lex.curtok().type() == token_add or lex.curtok().type() == token_subtract)
        return;
    else
        throw lexer_error("error");
}

void prefix_expression(lexer& lex)
{
    if (lex.curtok().type() == token_id or lex.curtok().type() == token_number or
        lex.curtok().type() == token_number_hex or lex.curtok().type() == token_bool_true or
        lex.curtok().type() == token_bool_false or lex.curtok().type() == token_oparen or
        lex.curtok().type() == token_not)
        postfix_expression(lex);
    else if (lex.curtok().type() == token_not)
    {
        lex++;
        prefix_expression(lex);
    }
}

void postfix_expression(lexer& lex)
{
    primary_expression(lex);
    postfix_expression_opt(lex);
}

void postfix_expression_opt(lexer& lex)
{
    if (lex.curtok().type() == token_oparen)
    {
        lex++;
        expression_list(lex);
        if (lex.curtok().type() == token_cparen)
            lex++;
        else
            throw parser_error("Expected ')'.");
    }
    else if (lex.curtok().type() == token_comma or lex.curtok().type() == token_assign or
        lex.curtok().type() == token_or or lex.curtok().type() == token_and or
        lex.curtok().type() == token_equal or lex.curtok().type() == token_nequal or
        lex.curtok().type() == token_less or lex.curtok().type() == token_greater or
        lex.curtok().type() == token_le or lex.curtok().type() == token_ge or
        lex.curtok().type() == token_add or lex.curtok().type() == token_subtract or
        lex.curtok().type() == token_multiply or lex.curtok().type() == token_divide)
        return;
    else
        throw parser_error("error");
}

void primary_expression(lexer& lex)
{
    if (lex.curtok().type() == token_number or lex.curtok().type() == token_number_hex or
        lex.curtok().type() == token_bool_true or lex.curtok().type() == token_bool_false)
        lex++;
    else if (lex.curtok().type() == token_id)
        lex++;
    else if (lex.curtok().type() == token_oparen)
    {
        lex++;
        expression(lex);
        if (lex.curtok().type() == token_cparen)
            lex++;
        else
            throw parser_error("error lol");
    }
    else
        throw parser_error("not an expression");
}

void selection_statement(lexer& lex)
{
    lex++;
    if (lex.curtok().type() == token_oparen)
        lex++;
    else
        throw parser_error("lol");
    expression(lex);
    if (lex.curtok().type() == token_cparen)
        lex++;
    else
        throw parser_error("ha");
    if (lex.curtok().type() == token_eol)
        lex++;
    else
        throw parser_error("ha");
    if_body(lex);
    if_concluder(lex);
}

void if_body(lexer& lex)
{
    statement_list(lex);
    elf_body(lex);
}

void elf_body(lexer& lex)
{
    if (lex.curtok().type() == token_elf)
    {
        lex++;
        if (lex.curtok().type() == token_oparen)
            lex++;
        else
            throw parser_error("error");
        expression(lex);
        if (lex.curtok().type() == token_cparen)
            lex++;
        else
            throw parser_error("error");
        if (lex.curtok().type() == token_eol)
            lex++;
        else
            throw parser_error("error");
        if_body(lex);
    }
    else if (lex.curtok().type() == token_else or lex.curtok().type() == token_endif)
        return;
    else
        throw parser_error("hi");
}

void if_concluder(lexer& lex)
{
    if (lex.curtok().type() == token_endif)
    {
        lex++;
        if (lex.curtok().type() == token_eol)
            lex++;
        else
            throw parser_error("hi");
    }
    else if (lex.curtok().type() == token_else)
    {
        lex++;
        statement_list(lex);
        if (lex.curtok().type() == token_endif)
            lex++;
        else
            throw parser_error("hi");
        if (lex.curtok().type() == token_eol)
            lex++;
        else
            throw parser_error("hi");
    }
    else
        throw parser_error("hi");
}

void iterative_statement(lexer& lex)
{
    lex++;
    if (lex.curtok().type() == token_oparen)
        lex++;
    else
        throw parser_error("hi");
    expression(lex);
    if (lex.curtok().type() == token_cparen)
        lex++;
    else
        throw parser_error("hi");
    if (lex.curtok().type() == token_eol)
        lex++;
    else
        throw parser_error("hi");
    statement_list(lex);
    if (lex.curtok().type() == token_endwhile)
        lex++;
    else
        throw parser_error("hi");
    if (lex.curtok().type() == token_eol)
        lex++;
    else
        throw parser_error("hi");
}

void jump_statement(lexer& lex)
{
    if (lex.curtok().type() == token_toss)
    {
        lex++;
        expression_list(lex);
        if (lex.curtok().type() == token_eol)
            lex++;
        else
            throw parser_error("eh");
    }
    else if (lex.curtok().type() == token_smash)
    {
        lex++;
        if (lex.curtok().type() == token_eol)
            lex++;
        else
            throw parser_error("eh");
    }
    else
        throw parser_error("fffff");
}

