/* ast.h - CS355 Compiler Project */
#ifndef AST_H
#define AST_H
#include "ramsey-error.h" // gets <ostream>, <string>
#include "lexer.h"
#include <deque>

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

    // forward declare types used in this module before their definition
    class ast_node; class ast_function_node; class ast_parameter_node;
    class ast_statement_node; class ast_elf_node; class ast_expression_node;
    class ast_expression_builder;

    // provide a base type for handling AST construction; a subtype will
    // be created to handle each construct in the AST
    class ast_builder
    {
    public:
        void add_token(const token* tok)
        { _elems.push_back(tok); }
        void add_node(ast_node* pnode)
        { _elems.push_back(pnode); }
        bool is_empty() const
        { return _elems.empty(); }
        int size() const
        { return int(_elems.size()); }
        void collapse(ast_builder& builder);
    protected:
        ast_builder() {}

        bool is_next_token() const
        { return _elems.back().flag == ast_element::ast_element_tok; }
        bool is_next_node() const
        { return _elems.back().flag == ast_element::ast_element_node; }
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
                int decore;
            };
            short flag;
        };

        std::deque<ast_element> _elems;
    };

    // provide a generic node type (with optional debug information)
    class ast_node
    {
    public:
        ast_node();
        virtual ~ast_node() {}

        // analyze semantics
        // generate code

#ifdef RAMSEY_DEBUG
        void output(std::ostream&) const;
        virtual void output_at_level(std::ostream&,int level) const;
#endif
    protected:
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const = 0;
        static void output_annot(std::ostream&,int level,const char* annot);
#endif
        // virtual semantic analysis
        // virtual code generation
    };

    // provide a generic node that can form a linked-list; any construct
    // that can appear in sequence should inherit from this base
    template<typename T> // 'T' must be a derivation of ast_linked_node
    class ast_linked_node : public ast_node
    {
    public:
        ~ast_linked_node();
        T* get_next();
        const T* get_next() const;
        T* get_prev();
        const T* get_prev() const;
        bool beg() const; // at beginning of sequence?
        bool end() const; // at end of sequence?

#ifdef RAMSEY_DEBUG
        virtual void output_at_level(std::ostream&,int level) const;
#endif
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

        // elements
        const token* _id; // identifier that names function
        ast_parameter_node* _param; // OPTIONAL list of parameter declarations
        const token* _typespec; // OPTIONAL type of function
        ast_statement_node* _statements; // OPTIONAL list of statements

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif

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
    private:
        ast_parameter_node();

        // elements
        const token* _typespec; // type of parameter
        const token* _id; // identifier that names parameter

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_parameter_builder : public ast_builder
    {
    public:
        ast_parameter_node* build();
    private:
        ast_parameter_node* get_next();
    };

    class ast_statement_node : public ast_linked_node<ast_statement_node>
    { // abstract class for statements; this shouldn't implement any virtual functions directly
        friend class ast_statement_builder;
    public:
        virtual ~ast_statement_node() {}
    protected:
        enum ast_statement_kind
        {
            ast_declaration_statement,
            ast_expression_statement,
            ast_selection_statement,
            ast_selection_elf_statement,
            ast_iterative_statement,
            ast_jump_statement
        };

        ast_statement_node(ast_statement_kind kind);
    private:
        const ast_statement_kind _kind; // decorate what kind of statement node this is
    };
    class ast_statement_builder : public ast_builder
    {
    public:
        ast_statement_node* build();
    };

    class ast_declaration_statement_node : public ast_statement_node
    {
        friend class ast_declaration_statement_builder;
    public:
        ~ast_declaration_statement_node();
    private:
        ast_declaration_statement_node();

        // elements
        const token* _typespec; // type specifier
        const token* _id; // identifier (name) of declaration
        ast_expression_node* _initializer; // OPTIONAL initializer for declaration

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_declaration_statement_builder : public ast_builder
    {
    public:
        ast_declaration_statement_node* build();
    };

    /* an expression-statement is syntactically identical to
       an expression-list, so they can be the same AST node type */
    typedef ast_expression_node ast_expression_statement_node;
    typedef ast_expression_builder ast_expression_statement_builder;

    class ast_selection_statement_node : public ast_statement_node
    {
        friend class ast_selection_statement_builder;
    public:
        ~ast_selection_statement_node();
    private:
        ast_selection_statement_node();

        // elements
        ast_expression_node* _condition;
        ast_statement_node* _body; // OPTIONAL statement block that matches condition
        ast_elf_node* _elf; // OPTIONAL statement block for elf clause
        ast_statement_node* _else; // OPTIONAL statement block for else clause

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_selection_statement_builder : public ast_builder
    {
    public:
        ast_selection_statement_node* build();
    };

    class ast_elf_node : public ast_node
    {
        friend class ast_elf_builder;
    public:
        ~ast_elf_node();
    private:
        ast_elf_node();

        // elements
        ast_expression_node* _condition;
        ast_statement_node* _body; // OPTIONAL statement block for body
        ast_elf_node* _elf; // OPTIONAL elf-statement block

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_elf_builder : public ast_builder
    {
    public:
        ast_elf_node* build();
    };

    class ast_iterative_statement_node : public ast_statement_node
    {
        friend class ast_iterative_statement_builder;
    public:
        ~ast_iterative_statement_node();
    private:
        ast_iterative_statement_node();

        // elements
        ast_expression_node* _condition;
        ast_statement_node* _body; // OPTIONAL

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_iterative_statement_builder : public ast_builder
    {
    public:
        ast_iterative_statement_node* build();
    };

    class ast_jump_statement_node : public ast_statement_node
    {
        friend class ast_jump_statement_builder;
    public:
        ~ast_jump_statement_node();
    private:
        ast_jump_statement_node();

        // elements
        const token* _kind; // 'toss' or 'smash'
        ast_expression_node* _expr; // OPTIONAL used only for 'toss' statement

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_jump_statement_builder : public ast_builder
    {
    public:
        ast_jump_statement_node* build();
    };

    class ast_expression_node : public ast_linked_node<ast_expression_node>
    { // abstract class for expressions; shouldn't implement any virtual functions directly
        friend class ast_expression_builder;
    public:
        virtual ~ast_expression_node() {};
    protected:
        enum ast_expression_kind
        {
            ast_assignment_expression,
            ast_logical_or_expression,
            ast_logical_and_expression,
            ast_equality_expression,
            ast_relational_expression,
            ast_additive_expression,
            ast_multiplicative_expression,
            ast_prefix_expression,
            ast_postfix_expression,
            ast_primary_expression
        };

        ast_expression_node(ast_expression_kind kind);

        struct operand
        { // an operand could be another node or simplified to an operand
            operand();
            operand(const token*);
            operand(ast_expression_node*);
            ~operand();

#ifdef RAMSEY_DEBUG
            void output_at_level(std::ostream&,int level) const;
#endif

            enum {
                operand_tok,
                operand_node
            };
            union { 
                const token* tok_operand;
                ast_expression_node* node_operand;
            };
            short flag;
        private:
            operand(const operand&);
        };

    private:
        ast_expression_kind _kind; // decorate what kind of expression node this is
    };
    class ast_expression_builder : public ast_builder
    {
    public:
        ast_expression_node* build();
    };

    class ast_assignment_expression_node : public ast_expression_node
    {
        friend class ast_assignment_expression_builder;
    private:
        ast_assignment_expression_node();

        // elements
        operand _ops[2];

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_assignment_expression_builder : public ast_builder
    {
    public:
        ast_assignment_expression_node* build();
    };

    class ast_logical_or_expression_node : public ast_expression_node
    {
        friend class ast_logical_or_expression_builder;
    private:
        ast_logical_or_expression_node();

        // elements: the operator is implied
        std::deque<operand> _ops;

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_logical_or_expression_builder : public ast_builder
    {
    public:
        ast_logical_or_expression_node* build();
    };

    class ast_logical_and_expression_node : public ast_expression_node
    {
        friend class ast_logical_and_expression_builder;
    private:
        ast_logical_and_expression_node();

        // elements: the operator is implied
        std::deque<operand> _ops;

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_logical_and_expression_builder : public ast_builder
    {
    public:
        ast_logical_and_expression_node* build();
    };

    class ast_equality_expression_node : public ast_expression_node
    {
        friend class ast_equality_expression_builder;
    private:
        ast_equality_expression_node();

        // elements
        std::deque<operand> _operands;
        std::deque<const token*> _operators;

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_equality_expression_builder : public ast_builder
    {
    public:
        ast_equality_expression_node* build();
    };

    class ast_relational_expression_node : public ast_expression_node
    {
        friend class ast_relational_expression_builder;
    private:
        ast_relational_expression_node();

        // elements
        std::deque<operand> _operands;
        std::deque<const token*> _operators;

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_relational_expression_builder : public ast_builder
    {
    public:
        ast_relational_expression_node* build();
    };

    class ast_additive_expression_node : public ast_expression_node
    {
        friend class ast_additive_expression_builder;
    private:
        ast_additive_expression_node();

        // elements
        std::deque<operand> _operands;
        std::deque<const token*> _operators;

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_additive_expression_builder : public ast_builder
    {
    public:
        ast_additive_expression_node* build();
    };

    class ast_multiplicative_expression_node : public ast_expression_node
    {
        friend class ast_multiplicative_expression_builder;
    private:
        ast_multiplicative_expression_node();

        // elements
        std::deque<operand> _operands;
        std::deque<const token*> _operators;

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_multiplicative_expression_builder : public ast_builder
    {
    public:
        ast_multiplicative_expression_node* build();
    };

    class ast_prefix_expression_node : public ast_expression_node
    {
        friend class ast_prefix_expression_builder;
    public:

    private:
        ast_prefix_expression_node();

        // elements
        const token* _operator;
        operand _operand;

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_prefix_expression_builder : public ast_builder
    {
    public:
        ast_prefix_expression_node* build();
    };

    class ast_postfix_expression_node : public ast_expression_node
    {
        friend class ast_postfix_expression_builder;
    public:
        ~ast_postfix_expression_node();
    private:
        ast_postfix_expression_node();

        // elements
        operand _operand; // operand modifying expression list
        ast_expression_node* _expList; // OPTIONAL expression list

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
    class ast_postfix_expression_builder : public ast_builder
    {
    public:
        ast_postfix_expression_node* build();
    };

    class ast_primary_expression_node : public ast_expression_node
    { // this type is created by ast_expression_builder
        friend class ast_expression_builder;
    private:
        ast_primary_expression_node(const token* tok);

        // elements
        const token* _tok; // in the AST, a primary expression is only a single token expression

        // virtual functions
#ifdef RAMSEY_DEBUG
        virtual void output_impl(std::ostream&,int nlevel) const;
#endif
    };
}

// include out-of-line implementation
#include "ast.tcc"

#endif
