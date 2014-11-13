/* ast.cpp */
#include "ast.h"
using namespace std;
using namespace ramsey;

// ast_error, ast_exception
ast_error::ast_error(const char* format, ...)
{
    va_list args;
    va_start(args,format);
    _cstor(format,args);
    va_end(args);
}
ast_exception::ast_exception(const char* format, ...)
{
    va_list args;
    va_start(args,format);
    _cstor(format,args);
    va_end(args);
}

// ast_builder
const token* ast_builder::pop_token()
{
#ifdef RAMSEY_DEBUG
    if (_elems.size() == 0)
        throw ast_exception("ast_builder::pop_token: stack is empty!");
#endif
    ast_element thing = _elems.top();
    _elems.pop();
    return thing.ptok;
}
ast_node* ast_builder::pop_node()
{
#ifdef RAMSEY_DEBUG
    if (_elems.size() == 0)
        throw ast_exception("ast_builder::pop_node: stack is empty!");
#endif
    ast_element thing = _elems.top();
    _elems.pop();
    return thing.pnode;
}

// ast_builder::ast_element
ast_builder::ast_element::ast_element()
    : pnode(NULL), flag(0)
{
}
ast_builder::ast_element::ast_element(const token* tok)
    : ptok(tok), flag(ast_element_tok)
{
}
ast_builder::ast_element::ast_element(ast_node* node)
    : pnode(node), flag(ast_element_node)
{
}

// ast_node

// ast_function_node, ast_function_builder
ast_function_node::ast_function_node()
    : _id(NULL), _param(NULL), _statements(NULL), _typespec(NULL)
{
}
ast_function_node::~ast_function_node()
{
    if (_param != NULL) {
        delete _param;
        _param = NULL;
    }
    if (_statements != NULL) {
        delete _statements;
        _statements = NULL;
    }
}
ast_function_node* ast_function_builder::build()
{
#ifdef RAMSEY_DEBUG
    if ( is_empty() )
        throw ast_exception("ast_function_builder: element stack was empty");
#endif
    ast_function_node* node = get_next(), *cur;
    cur = node;
    while ( !is_empty() ) {
        ast_function_node* n = get_next();
        cur->append(n);
        cur = n;
    }
    return node;
}
ast_function_node* ast_function_builder::get_next()
{
    ast_function_node* node = new ast_function_node;
#ifdef RAMSEY_DEBUG
    if ( !is_next_token() )
        throw ast_exception("ast_function_builder: expected token for type specifier");
#endif
    node->_typespec = pop_token();
#ifdef RAMSEY_DEBUG
    if ( !is_next_node() )
        throw ast_exception("ast_function_builder: expected node for statements");
#endif
    node->_statements = static_cast<ast_statement_node*>(pop_node());
#ifdef RAMSEY_DEBUG
    if ( !is_next_node() )
        throw ast_exception("ast_function_builder: expected node for parameter");
#endif
    node->_param = static_cast<ast_parameter_node*>(pop_node());
#ifdef RAMSEY_DEBUG
    if ( !is_next_token() )
        throw ast_exception("ast_function_builder: expected token for identifier");
#endif
    node->_id = pop_token();
    return node;
}

// ast_parameter_node, ast_parameter_builder
ast_parameter_node::ast_parameter_node()
    : _typespec(NULL), _id(NULL)
{
}
ast_parameter_node* ast_parameter_builder::build()
{
#ifdef RAMSEY_DEBUG
    if ( is_empty() )
        throw ast_exception("ast_parameter_builder: element stack was empty");
#endif
    ast_parameter_node* node = get_next(), *cur;
    cur = node;
    while ( !is_empty() ) {
        ast_parameter_node* n = get_next();
        cur->append(n);
        cur = n;
    }
    return node;
}
ast_parameter_node* ast_parameter_builder::get_next()
{
    ast_parameter_node* node = new ast_parameter_node;
#ifdef RAMSEY_DEBUG
    if ( !is_next_token() )
        throw ast_exception("ast_parameter_builder: expected token for identifier");
#endif
    node->_id = pop_token();
#ifdef RAMSEY_DEBUG
    if ( !is_next_token() )
        throw ast_exception("ast_parameter_builder: expected token for type specifier");
#endif
    node->_typespec = pop_token();
    return node;
}
