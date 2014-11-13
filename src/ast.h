/* ast.h - CS355 Compiler Project */
#ifndef AST_H
#define AST_H
#include "ramsey-error.h"
#include "lexer.h"
#include <stack>

namespace ramsey
{
    // exception types
    class ast_error : public compiler_error_generic
    {
    public:
        ast_error(const char* format, ...);
        virtual ~ast_error() throw() {}
    };
    class ast_exception : public compiler_error_generic
    {
    public:
        ast_exception(const char* format, ...);
        virtual ~ast_exception() throw() {}
    };

    // forward declare types used in this module
    class ast_node;
    class ast_function_node;
    class ast_function_builder;
    class ast_parameter_node;
    class ast_statement_node;

    // provide a base type for handling AST construction; a subtype will
    // be created to handle each AST construct kind
    class ast_builder
    {
    public:
        void add_token(const token* tok)
        { _elems.push(tok); }
        void add_node(ast_node* pnode)
        { _elems.push(pnode); }
    protected:
        ast_builder() {}

        bool is_empty() const
        { return _elems.empty(); }
        bool is_next_token() const
        { return _elems.top().flag == ast_element::ast_element_tok; }
        bool is_next_node() const
        { return _elems.top().flag == ast_element::ast_element_node; }
        const token* pop_token();
        ast_node* pop_node();
    private:
        struct ast_element
        {
            ast_element();
            ast_element(const token* tok);
            ast_element(ast_node* pnode);

            enum {
                ast_element_tok,
                ast_element_node
            };
            union {
                const token* ptok;
                ast_node* pnode;
            };
            short flag;
        };

        std::stack<ast_element> _elems;
    };

    // provide a generic node type
    class ast_node
    {
        // virtual semantic analysis
        // virtual code generation
    };

    // provide a generic node that can form a linked-list
    template<typename T> // 'T' must be a derivation of ast_linked_node
    class ast_linked_node : public ast_node
    {
    public:
        T* get_next();
        const T* get_next() const;
        T* get_prev();
        const T* get_prev() const;
        bool beg() const; // at beginning of sequence?
        bool end() const; // at end of sequence?
    protected:
        ast_linked_node();
        void append(T* node);
    private:
        ast_linked_node<T>* _nxt, *_prv;
    };

    class ast_function_node : public ast_linked_node<ast_function_node>
    { // represents the root node node of the AST
        friend class ast_function_builder;
    public:
        ~ast_function_node();
    private:
        ast_function_node();

        const token* _id; // identifier that names function
        ast_parameter_node* _param; // linked-list of parameter declarations
        ast_statement_node* _statements; // linked-list of statements
        const token* _typespec; // type of function
    };
    class ast_function_builder : public ast_builder
    {
    public:
        ast_function_node* build();
    private:
        ast_function_node* get_next();
    };

    class ast_parameter_node : public ast_linked_node<ast_parameter_node>
    {
        friend class ast_parameter_builder;
    public:

    private:
        ast_parameter_node();

        const token* _typespec; // type of parameter
        const token* _id; // identifier that names parameter
    };
    class ast_parameter_builder : public ast_builder
    {
    public:
        ast_parameter_node* build();
    private:
        ast_parameter_node* get_next();
    };

    class ast_statement_node : public ast_linked_node<ast_statement_node>
    {
    };
}

// include out-of-line implementation
#include "ast.tcc"

#endif
