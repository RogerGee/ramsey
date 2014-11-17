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
void ast_builder::collapse(ast_builder& builder)
{
    /*_elems.swap(builder._elems);
    while ( !builder._elems.empty() ) {
        // maintain the correct ordering by popping off the front
        _elems.push_back(builder._elems.front());
        builder._elems.pop_front();
        }*/
    while ( !builder._elems.empty() ) {
        _elems.push_back(builder._elems.front());
        builder._elems.pop_front();
    }
}
const token* ast_builder::pop_token()
{
#ifdef RAMSEY_DEBUG
    if (_elems.size() == 0)
        throw ast_exception("ast_builder::pop_token: stack is empty");
    if ( !is_next_token() )
        throw ast_exception("ast_builder::pop_token: top of stack was not a token");
#endif
    ast_element thing = _elems.back();
    _elems.pop_back();
    return thing.ptok;
}
ast_node* ast_builder::pop_node()
{
#ifdef RAMSEY_DEBUG
    if (_elems.size() == 0)
        throw ast_exception("ast_builder::pop_node: stack is empty!");
    if ( !is_next_node() )
        throw ast_exception("ast_builder::pop_token: top of stack was not a node");
#endif
    ast_element thing = _elems.back();
    _elems.pop_back();
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
ast_node::ast_node()
{
}
#ifdef RAMSEY_DEBUG
void ast_node::output(ostream& stream) const
{
    output_at_level(stream,0);
}
void ast_node::output_at_level(ostream& stream,int level) const
{
    for (int i = 0;i < level;++i)
        stream.put('\t');
    output_impl(stream,level+1);
}
#endif

// ast_function_node, ast_function_builder
ast_function_node::ast_function_node()
    : _id(NULL), _param(NULL), _typespec(NULL), _statements(NULL)
{
}
ast_function_node::~ast_function_node()
{
    if (_param != NULL)
        delete _param;
    if (_statements != NULL)
        delete _statements;
}
void ast_function_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "function:id=" << *_id << ",type=";
    if (_typespec != NULL)
        stream << *_typespec;
    else
        stream << "[default in]";
    stream << '\n';
    if (_param != NULL)
        _param->output_at_level(stream,nlevel);
    if (_statements != NULL)
        _statements->output_at_level(stream,nlevel);
}
ast_function_node* ast_function_builder::build()
{
#ifdef RAMSEY_DEBUG
    if ( is_empty() )
        throw ast_exception("ast_function_builder: element stack was empty");
#endif
    ast_function_node* node = get_next();
    while ( !is_empty() ) {
        ast_function_node* n = get_next();
        node->append(n);
        node = n;
    }
    return node;
}
ast_function_node* ast_function_builder::get_next()
{
    ast_function_node* node = new ast_function_node;
    node->_statements = static_cast<ast_statement_node*>(pop_node());
    node->_typespec = pop_token();
    node->_param = static_cast<ast_parameter_node*>(pop_node());
    node->_id = pop_token();
    return node;
}

// ast_parameter_node, ast_parameter_builder
ast_parameter_node::ast_parameter_node()
    : _typespec(NULL), _id(NULL)
{
}
void ast_parameter_node::output_impl(ostream& stream,int) const
{
    stream << "parameter:id=" << *_id << ",type=" << *_typespec << '\n';
}
ast_parameter_node* ast_parameter_builder::build()
{
    if ( is_empty() )
        return NULL; // parameter_declaration is potentially nullable
    ast_parameter_node* node = get_next();
    while ( !is_empty() ) {
        ast_parameter_node* n = get_next();
        node->append(n);
        node = n;
    }
    return node;
}
ast_parameter_node* ast_parameter_builder::get_next()
{
    ast_parameter_node* node = new ast_parameter_node;
    node->_id = pop_token();
    node->_typespec = pop_token();
    return node;
}

// ast_statement_node, ast_statement_builder
ast_statement_node::ast_statement_node(ast_statement_kind dec)
    : _kind(dec)
{
}
ast_statement_node* ast_statement_builder::build()
{
    if ( is_empty() )
        return NULL; // statement_list is potentially nullable
    // ast_statement_builder is a generic builder type; other builders have already
    // contructed a object derived from ast_statement_node and placed it in this
    // object's stack; just pop them off, link them up and return the head of the list
    ast_statement_node* node = static_cast<ast_statement_node*>(pop_node());
    while ( !is_empty() ) {
        ast_statement_node* nxt = static_cast<ast_statement_node*>(pop_node());
        node->append(nxt);
        node = nxt;
    }
    return node;
}

// ast_declaration_statement_node, ast_declaration_statement_builder
ast_declaration_statement_node::ast_declaration_statement_node()
    : ast_statement_node(ast_declaration_statement), _typespec(NULL), _id(NULL), _initializer(NULL)
{
}
ast_declaration_statement_node::~ast_declaration_statement_node()
{
    if (_initializer != NULL)
        delete _initializer;
}
#ifdef RAMSEY_DEBUG
void ast_declaration_statement_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "declaration-statement:id=" << *_id << ",type=" << *_typespec << '\n';
    if (_initializer != NULL)
        _initializer->output_at_level(stream,nlevel);
}
#endif
ast_declaration_statement_node* ast_declaration_statement_builder::build()
{
    ast_declaration_statement_node* node = new ast_declaration_statement_node;
    node->_initializer = static_cast<ast_expression_node*>(pop_node());
    node->_id = pop_token();
    node->_typespec = pop_token();
    return node;
}

// ast_selection_statement_node, ast_selection_statement_builder
ast_selection_statement_node::ast_selection_statement_node()
    : ast_statement_node(ast_selection_statement), _condition(NULL),
      _body(NULL), _elf(NULL), _else(NULL)
{
}
ast_selection_statement_node::~ast_selection_statement_node()
{
    if (_condition != NULL)
        delete _condition;
    if (_body != NULL)
        delete _body;
    if (_elf != NULL)
        delete _elf;
    if (_else != NULL)
        delete _else;
}
#ifdef RAMSEY_DEBUG
void ast_selection_statement_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "selection-statement\n";
    _condition->output_at_level(stream,nlevel);
    if (_body != NULL)
        _body->output_at_level(stream,nlevel);
    if (_elf != NULL)
        _elf->output_at_level(stream,nlevel);
    if (_else != NULL)
        _else->output_at_level(stream,nlevel);
}
#endif
ast_selection_statement_node* ast_selection_statement_builder::build()
{
    ast_selection_statement_node* node = new ast_selection_statement_node;
    node->_else = static_cast<ast_statement_node*>(pop_node());
    node->_elf = static_cast<ast_elf_node*>(pop_node());
    node->_body = static_cast<ast_statement_node*>(pop_node());
    node->_condition = static_cast<ast_expression_node*>(pop_node());
    return node;
}

// ast_elf_node, ast_elf_builder
ast_elf_node::ast_elf_node()
    : _condition(NULL), _body(NULL), _elf(NULL)
{
}
ast_elf_node::~ast_elf_node()
{
    if (_condition != NULL)
        delete _condition;
    if (_body != NULL)
        delete _body;
    if (_elf != NULL)
        delete _elf;
}
#ifdef RAMSEY_DEBUG
void ast_elf_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "elf-statement\n";
    _condition->output_at_level(stream,nlevel);
    _body->output_at_level(stream,nlevel);
    if (_elf != NULL)
        _elf->output_at_level(stream,nlevel);
}
#endif
ast_elf_node* ast_elf_builder::build()
{
    ast_elf_node* node = new ast_elf_node;
    node->_elf = static_cast<ast_elf_node*>(pop_node());
    node->_body = static_cast<ast_statement_node*>(pop_node());
    node->_condition = static_cast<ast_expression_node*>(pop_node());
    return node;
}

// ast_expression_node, ast_expression_node::operand, ast_expression_builder
ast_expression_node::ast_expression_node(ast_expression_kind kind)
    : _kind(kind)
{
}
ast_expression_node::operand::operand()
    : tok_operand(NULL), flag(-1)
{
}
ast_expression_node::operand::operand(const token* tok)
    : tok_operand(tok), flag(operand_tok)
{
}
ast_expression_node::operand::operand(ast_expression_node* node)
    : node_operand(node), flag(operand_node)
{
}
ast_expression_node::operand::~operand()
{
    if (node_operand!=NULL && flag==operand_node)
        delete node_operand;
}
#ifdef RAMSEY_DEBUG
void ast_expression_node::operand::output_at_level(ostream& stream,int level) const
{
    if (flag == operand_tok) {
        for (int i = 0;i < level;++i)
            stream.put('\t');
        stream << *tok_operand << '\n';
    }
    else
        node_operand->output_at_level(stream,level);
}
#endif
ast_expression_node* ast_expression_builder::build()
{
    /* if the builder has a token on top of its stack, then
       it will create an ast_primary_expression_node; else it
       is assumed that the element stack contains node pointers
       to ast_expression_node derivations */
    if ( is_next_token() ) {
#ifdef RAMSEY_DEBUG
        if (size() != 1)
            throw ast_exception("ast_expression_builder: expected single token in element stack");
#endif
        return new ast_primary_expression_node( pop_token() );
    }
    ast_expression_node* node = static_cast<ast_expression_node*>(pop_node());
    while ( !is_empty() ) {
        ast_expression_node* n = static_cast<ast_expression_node*>(pop_node());
        node->append(n);
        node = n;
    }
    return node;
}

// ast_assignment_expression_node, ast_assignment_expression_builder
ast_assignment_expression_node::ast_assignment_expression_node()
    : ast_expression_node(ast_assignment_expression)
{
}
#ifdef RAMSEY_DEBUG
void ast_assignment_expression_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "assignment-expression\n";
    for (short i = 0;i<2;++i)
        _ops[i].output_at_level(stream,nlevel);
}
#endif
ast_assignment_expression_node* ast_assignment_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() != 2)
        throw ast_exception("ast_assignment_expression_builder: element stack did not contain correct number of elements");
#endif
    ast_assignment_expression_node* node = new ast_assignment_expression_node;
    for (short i = 1;i >= 0;--i) {
        if ( is_next_token() ) {
            node->_ops[i].tok_operand = pop_token();
            node->_ops[i].flag = ast_expression_node::operand::operand_tok;
        }
        else {
            node->_ops[i].node_operand = static_cast<ast_expression_node*>(pop_node());
            node->_ops[i].flag = ast_expression_node::operand::operand_node;
        }
    }
    return node;
}

// ast_logical_or_expression_node, ast_logical_or_expression_node
ast_logical_or_expression_node::ast_logical_or_expression_node()
    : ast_expression_node(ast_logical_or_expression)
{
}
#ifdef RAMSEY_DEBUG
void ast_logical_or_expression_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "logical-or-expression\n";
    for (deque<operand>::const_iterator iter = _ops.begin();iter != _ops.end();++iter)
        iter->output_at_level(stream,nlevel);
}
#endif
ast_logical_or_expression_node* ast_logical_or_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() < 2)
        throw ast_exception("ast_logical_or_expression_builder: element stack did not contain correct number of elements");
#endif
    ast_logical_or_expression_node* node = new ast_logical_or_expression_node;
    while ( !is_empty() ) {
        if ( is_next_token() )
            node->_ops.emplace_front( pop_token() );
        else
            node->_ops.emplace_front( static_cast<ast_expression_node*>(pop_node()) );
    }
    return node;
}

// ast_logical_and_expression_node, ast_logical_and_expression_node
ast_logical_and_expression_node::ast_logical_and_expression_node()
    : ast_expression_node(ast_logical_and_expression)
{
}
#ifdef RAMSEY_DEBUG
void ast_logical_and_expression_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "logical-and-expression\n";
    for (deque<operand>::const_iterator iter = _ops.begin();iter != _ops.end();++iter)
        iter->output_at_level(stream,nlevel);
}
#endif
ast_logical_and_expression_node* ast_logical_and_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() < 2)
        throw ast_exception("ast_logical_and_expression_builder: element stack did not contain correct number of elements");
#endif
    ast_logical_and_expression_node* node = new ast_logical_and_expression_node;
    while ( !is_empty() ) {
        if ( is_next_token() )
            node->_ops.emplace_front( pop_token() );
        else
            node->_ops.emplace_front( static_cast<ast_expression_node*>(pop_node()) );
    }
    return node;
}

// ast_equality_expression_node, ast_equality_expression_node
ast_equality_expression_node::ast_equality_expression_node()
    : ast_expression_node(ast_equality_expression)
{
}
#ifdef RAMSEY_DEBUG
void ast_equality_expression_node::output_impl(ostream& stream,int nlevel) const
{
    deque<operand>::size_type i = 0, j = 0;
    stream << "equality-expression\n";
    while (j < _operators.size()) {
        _operands[i++].output_at_level(stream,nlevel);
        for (int cnt = 0;cnt < nlevel;++cnt)
            stream.put('\t');
        stream << *_operators[j++] << '\n';
    }
    _operands[i].output_at_level(stream,nlevel);
}
#endif
ast_equality_expression_node* ast_equality_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() < 2)
        throw ast_exception("ast_equality_expression_builder: element stack did not contain correct number of elements");
#endif
    bool state = true;
    ast_equality_expression_node* node = new ast_equality_expression_node;
    while ( !is_empty() ) {
        if (state) {
            if ( is_next_token() )
                node->_operands.emplace_front( pop_token() );
            else
                node->_operands.emplace_front( static_cast<ast_expression_node*>(pop_node()) );
        }
        else
            node->_operators.push_front( pop_token() );
        state = !state;
    }
    return node;
}

// ast_relational_expression_node, ast_relational_expression_node
ast_relational_expression_node::ast_relational_expression_node()
    : ast_expression_node(ast_relational_expression)
{
}
#ifdef RAMSEY_DEBUG
void ast_relational_expression_node::output_impl(ostream& stream,int nlevel) const
{
    deque<operand>::size_type i = 0, j = 0;
    stream << "relational-expression\n";
    while (j < _operators.size()) {
        _operands[i++].output_at_level(stream,nlevel);
        for (int cnt = 0;cnt < nlevel;++cnt)
            stream.put('\t');
        stream << *_operators[j++] << '\n';
    }
    _operands[i].output_at_level(stream,nlevel);
}
#endif
ast_relational_expression_node* ast_relational_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() < 2)
        throw ast_exception("ast_relational_expression_builder: element stack did not contain correct number of elements");
#endif
    bool state = true;
    ast_relational_expression_node* node = new ast_relational_expression_node;
    while ( !is_empty() ) {
        if (state) {
            if ( is_next_token() )
                node->_operands.emplace_front( pop_token() );
            else
                node->_operands.emplace_front( static_cast<ast_expression_node*>(pop_node()) );
        }
        else
            node->_operators.push_front( pop_token() );
        state = !state;
    }
    return node;
}

// ast_additive_expression_node, ast_additive_expression_node
ast_additive_expression_node::ast_additive_expression_node()
    : ast_expression_node(ast_additive_expression)
{
}
#ifdef RAMSEY_DEBUG
void ast_additive_expression_node::output_impl(ostream& stream,int nlevel) const
{
    deque<operand>::size_type i = 0, j = 0;
    stream << "additive-expression\n";
    while (j < _operators.size()) {
        _operands[i++].output_at_level(stream,nlevel);
        for (int cnt = 0;cnt < nlevel;++cnt)
            stream.put('\t');
        stream << *_operators[j++] << '\n';
    }
    _operands[i].output_at_level(stream,nlevel);
}
#endif
ast_additive_expression_node* ast_additive_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() < 2)
        throw ast_exception("ast_additive_expression_builder: element stack did not contain correct number of elements");
#endif
    bool state = true;
    ast_additive_expression_node* node = new ast_additive_expression_node;
    while ( !is_empty() ) {
        if (state) {
            if ( is_next_token() )
                node->_operands.emplace_front( pop_token() );
            else
                node->_operands.emplace_front( static_cast<ast_expression_node*>(pop_node()) );
        }
        else
            node->_operators.push_front( pop_token() );
        state = !state;
    }
    return node;
}

// ast_multiplicative_expression_node, ast_multiplicative_expression_node
ast_multiplicative_expression_node::ast_multiplicative_expression_node()
    : ast_expression_node(ast_multiplicative_expression)
{
}
#ifdef RAMSEY_DEBUG
void ast_multiplicative_expression_node::output_impl(ostream& stream,int nlevel) const
{
    deque<operand>::size_type i = 0, j = 0;
    stream << "multiplicative-expression\n";
    while (j < _operators.size()) {
        _operands[i++].output_at_level(stream,nlevel);
        for (int cnt = 0;cnt < nlevel;++cnt)
            stream.put('\t');
        stream << *_operators[j++] << '\n';
    }
    _operands[i].output_at_level(stream,nlevel);
}
#endif
ast_multiplicative_expression_node* ast_multiplicative_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() < 2)
        throw ast_exception("ast_multiplicative_expression_builder: element stack did not contain correct number of elements");
#endif
    bool state = true;
    ast_multiplicative_expression_node* node = new ast_multiplicative_expression_node;
    while ( !is_empty() ) {
        if (state) {
            if ( is_next_token() )
                node->_operands.emplace_front( pop_token() );
            else
                node->_operands.emplace_front( static_cast<ast_expression_node*>(pop_node()) );
        }
        else
            node->_operators.push_front( pop_token() );
        state = !state;
    }
    return node;
}

// ast_prefix_expression_node, ast_prefix_expression_builder
ast_prefix_expression_node::ast_prefix_expression_node()
    : ast_expression_node(ast_prefix_expression), _operator(NULL)
{
}
#ifdef RAMSEY_DEBUG
void ast_prefix_expression_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "prefix-expression:op=" << *_operator << '\n';
    _operand.output_at_level(stream,nlevel);
}
#endif
ast_prefix_expression_node* ast_prefix_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() != 2)
        throw ast_exception("ast_prefix_expression_builder: element stack did not contain correct number of elements");
#endif
    ast_prefix_expression_node* node = new ast_prefix_expression_node;
    if ( is_next_token() ) {
        node->_operand.tok_operand = pop_token();
        node->_operand.flag = ast_expression_node::operand::operand_tok;
    }
    else {
        node->_operand.node_operand = static_cast<ast_expression_node*>(pop_node());
        node->_operand.flag = ast_expression_node::operand::operand_node;
    }
    node->_operator = pop_token();
    return node;
}

// ast_postfix_expression_node, ast_postfix_expression_builder
ast_postfix_expression_node::ast_postfix_expression_node()
    : ast_expression_node(ast_postfix_expression), _expList(NULL)
{
}
ast_postfix_expression_node::~ast_postfix_expression_node()
{
    if (_expList != NULL)
        delete _expList;
}
#ifdef RAMSEY_DEBUG
void ast_postfix_expression_node::output_impl(ostream& stream,int nlevel) const
{
    stream << "postfix-expression\n";
    _operand.output_at_level(stream,nlevel);
    _expList->output_at_level(stream,nlevel);
}
#endif
ast_postfix_expression_node* ast_postfix_expression_builder::build()
{
#ifdef RAMSEY_DEBUG
    if (size() != 2)
        throw ast_exception("ast_postfix_expression_builder: element stack did not contain correct number of elements");
#endif
    ast_postfix_expression_node* node = new ast_postfix_expression_node;
    node->_expList = static_cast<ast_expression_node*>(pop_node());
    // operand is either a token or a node
    if ( is_next_token() ) {
        node->_operand.tok_operand = pop_token();
        node->_operand.flag = ast_expression_node::operand::operand_tok;
    }
    else {
        node->_operand.node_operand = static_cast<ast_expression_node*>(pop_node());
        node->_operand.flag = ast_expression_node::operand::operand_node;
    }
    return node;
}

// ast_primary_expression_node
ast_primary_expression_node::ast_primary_expression_node(const token* tok)
    : ast_expression_node(ast_primary_expression), _tok(tok)
{
}
#ifdef RAMSEY_DEBUG
void ast_primary_expression_node::output_impl(ostream& stream,int) const
{
    stream << "primary-expression:tok=" << *_tok << '\n';
}
#endif
