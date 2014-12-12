/* parser.h - CS355 Compiler Project */
#ifndef PARSER_H
#define PARSER_H
#include <stack>
#include "lexer.h" // gets "ramsey-error.h"
#include "ast.h"

namespace ramsey
{
    // exception types
    class parser_error : public compiler_error_generic // errors reported to user
    {
    public:
        parser_error(const char* format, ...);
        virtual ~parser_error() throw() {}
    };
    class parser_exception : public compiler_error_generic // errors used internally
    {
    public:
        parser_exception(const char* format, ...);
        virtual ~parser_exception() throw() {}
    };
    
    class parser
    {
    public:
        parser(const char* file);
        ~parser();

        int sloc() const
        { return linenumber; }
        const ast_node* get_ast() const
        { return ast; } // if NULL then source file was empty
        const lexer& get_lexer() const
        { return lex; }
    private:
        int linenumber;
        lexer lex;

        // abstract syntax tree (root node)
        ast_node* ast;

        // helper functions for parser
        bool eol();

        // declare recursive-descent functions
        std::stack<ast_builder*> builders;
        void program();
        void function_list();
        void function();
        void function_declaration();
        void function_type_specifier();
        void parameter_declaration();
        void parameter();
        void parameter_list();
        void statement();
        void statement_list();
        void declaration_statement();
        void type_name();
        void initializer();
        void assignment_operator();
        void expression_statement();
        void expression();
        void expression_list();
        void expression_list_item();
        void assignment_expression();
        void assignment_expression_opt();
        void logical_or_expression();
        void logical_or_expression_opt();
        void logical_and_expression();
        void logical_and_expression_opt();
        void equality_expression();
        void equality_expression_opt();
        void relational_expression();
        void relational_expression_opt();
        void additive_expression();
        void additive_expression_opt();
        void multiplicative_expression();
        void multiplicative_expression_opt();
        void prefix_expression();
        void postfix_expression();
        void postfix_expression_opt();
        void function_call();
        void primary_expression();
        void selection_statement();
        void if_body();
        void elf_body();
        void if_concluder();
        void iterative_statement();
        void jump_statement();
    };
    
}

#endif
