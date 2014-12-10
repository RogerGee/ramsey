/* semantics.cpp */
#include "ast.h" // gets stable.h
#include <vector>
using namespace std;
using namespace ramsey;

// determine if the two token type names are the same
static bool semantic_type_equality(token_t left,token_t right)
{
#ifdef RAMSEY_DEBUG
    // token types should be type names
    if ((left!=token_in && left!=token_big && left!=token_small && left!=token_boo)
        || (right!=token_in && right!=token_big && right!=token_small && right!=token_boo))
        throw ramsey_exception();
#endif
    // semantically "in" can mean either "big" or 'small'
    if ((left==token_big || left==token_small) && right==token_in)
        return true;
    if ((right==token_big || right==token_small) && left==token_in)
        return true;
    return left == right;
}

// get string form of specified type name
static const char* semantic_type_name(token_t type)
{
    switch (type) {
    case token_in:
        return "in";
    case token_boo:
        return "boo";
    case token_big:
        return "big";
    case token_small:
        return "small";
    default:
#ifdef RAMSEY_DEBUG
    throw ramsey_exception();
#endif
        return "bad";
    }
}

// symbol
symbol::match_parameters_result symbol::match_parameters(const token_t* kinds,int cnt) const
{
    if (_argtypes == NULL)
        _argtypes = get_argtypes_impl();
    token_t* p = _argtypes; bool b = true;
    for (int i = 0;i < cnt;++i,++p) {
        if (*p == token_invalid)
            return match_too_many;
        // match 
        if (!semantic_type_equality(*p,kinds[i]) && (kinds[i]!=token_small || *p!=token_big))
            b = false;
    }
    if (*p == token_invalid)
        return b ? match_okay : match_bad_types;
    return match_too_few;
}

// ast_node derivations: perform a post-order traversal of sorts; add the symbol
// immediately so that the object is in scope; perform needed analysis AFTER calling
// analysis routine for any children
void ast_function_node::semantics_impl(stable& symtable) const
{
    if ( !symtable.add(this) ) // if symbol exists then it must be a function
        throw semantic_error("redeclaration of function '%s'",_id->source_string());
    // we want all function to be in scope before analyzing any of their statement bodies
    if ( !end() )
        get_next()->semantics_impl(symtable);
    symtable.addScope(); // scope for function
    symtable.enterFunction(this); // let the symbol table track the function throughout its lifetime
    { ast_parameter_node* n = _param;
        while (n != NULL) {
            n->check_semantics(symtable);
            n = n->get_next();
        }
    }
    { ast_statement_node* n = _statements;
        while (n != NULL) {
            n->check_semantics(symtable);
            n = n->get_next();
        }
    }
    symtable.exitFunction();
    symtable.remScope();
}

void ast_parameter_node::semantics_impl(stable& symtable) const
{
    // add parameter decls to the symbol table
    if ( !symtable.add(this) )
        throw semantic_error("line %d: parameter name '%s' is already in use",get_lineno(),_id->source_string());
}

void ast_declaration_statement_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    if (_initializer != NULL) {
        _initializer->check_semantics(symtable);
        // check types
        token_t left = _typespec->type(), right = _initializer->get_type(symtable);
        if (!semantic_type_equality(left,right) && (right!=token_small || left!=token_big))
            throw semantic_error("line %d: declaration requires initializer of type '%s', not '%s'",get_lineno(),
                semantic_type_name(_typespec->type()),semantic_type_name(_initializer->get_type(symtable)));
    }
    if ( !symtable.add(this) )
        throw semantic_error("line %d: can't redeclare variable; name '%s' already in use",get_lineno(),_id->source_string());
}

void ast_selection_statement_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    symtable.addScope(); // begin new scope BEFORE condition
    _condition->check_semantics(symtable);
    if (_condition->get_type(symtable) != token_boo) // check type semantics on condition
        throw semantic_error("line %d: if-statement condition expression must be of type 'boo'",get_lineno());
    if (_body != NULL)
        _body->check_semantics(symtable);
    symtable.remScope(); // end scope AFTER if-body
    if (_elf != NULL)
        _elf->check_semantics(symtable);
    symtable.addScope(); // add scope for else-body
    if (_else != NULL)
        _else->check_semantics(symtable);
    symtable.remScope();
}

void ast_elf_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    symtable.addScope(); // begin new scope BEFORE condition
    _condition->check_semantics(symtable);
    if (_condition->get_type(symtable) != token_boo) // check type semantics on condition
        throw semantic_error("line %d: elf-statement condition expression must be of type 'boo'",get_lineno());
    if (_body != NULL)
        _body->check_semantics(symtable);
    symtable.remScope(); // end scope AFTER elf-body
    if (_elf != NULL)
        _elf->check_semantics(symtable);
}

void ast_iterative_statement_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    symtable.addScope(); // add scope BEFORE condition
    _condition->check_semantics(symtable);
    if (_condition->get_type(symtable) != token_boo)
        throw semantic_error("line %d: iterative-statement condition expression must be of type 'boo'",get_lineno());
    if (_body != NULL)
        _body->check_semantics(symtable);
    symtable.remScope(); // end scope after statement body
}

void ast_jump_statement_node::semantics_impl(stable& symtable) const
{
    if (_expr != NULL) {
        _expr->check_semantics(symtable);
        // check that return statement matches function return type
        token_t funcType, tossType;
        funcType = symtable.getFunction()->get_type();
        tossType = _expr->get_type(symtable);
        if (!semantic_type_equality(funcType,tossType) && (tossType!=token_small || funcType!=token_big))
            throw semantic_error("line %d: cannot convert '%s' to '%s' in return",get_lineno(),semantic_type_name(tossType),semantic_type_name(funcType));
    }
}

// expression node derivations
void ast_assignment_expression_node::semantics_impl(stable& symtable) const
{
    token_t left, right;
    // do visitor pattern
    for (short i = 0;i < 2;++i)
        _ops[i].node->check_semantics(symtable);
    // get types of operands
    left = get_type(symtable); // does some semantic analysis as well
    right = _ops[1].node->get_type(symtable);
    // make sure right hand type is assignable to left hand type
    if (!semantic_type_equality(left,right) && (right!=token_small || left!=token_big))
        throw semantic_error("line %d: cannot assign type '%s' to object of type '%s'",get_lineno(),semantic_type_name(right),semantic_type_name(left));
}
token_t ast_assignment_expression_node::get_ex_type_impl(const stable& symtable) const
{
    // type is determined by left operand (assigned-to operand)
    token_t type = _ops[0].node->get_type(symtable);
    if (_ops[0].node->get_kind() != ast_primary_expression)
        throw semantic_error("line: %d: cannot assign to expression",get_lineno());
    else if ( !static_cast<ast_primary_expression_node*>(_ops[0].node)->is_identifier() )
        throw semantic_error("line %d: cannot assign to non-identifier",get_lineno());
    return type;
}

void ast_logical_or_expression_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    for (auto iter = _ops.begin();iter != _ops.end();++iter)
        iter->node->check_semantics(symtable);
    // semantic analysis
    get_type(symtable);
}
token_t ast_logical_or_expression_node::get_ex_type_impl(const stable& symtable) const
{
    for (auto iter = _ops.begin();iter != _ops.end();++iter) {
        if (iter->node->get_type(symtable) != token_boo)
            throw semantic_error("line %d: or-operator must have operands of type 'boo'");
    }
    // should always be a 'boo' type expression
    return token_boo;
}

void ast_logical_and_expression_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    for (auto iter = _ops.begin();iter != _ops.end();++iter)
        iter->node->check_semantics(symtable);
    // semantic analysis
    get_type(symtable);
}
token_t ast_logical_and_expression_node::get_ex_type_impl(const stable& symtable) const
{
    for (auto iter = _ops.begin();iter != _ops.end();++iter) {
        if (iter->node->get_type(symtable) != token_boo)
            throw semantic_error("line %d: and-operator must have operands of type 'boo'");
    }
    // should always be a 'boo' type expression
    return token_boo;
}

void ast_equality_expression_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    for (short i = 0;i < 2;++i)
        _operands[i].node->check_semantics(symtable);
    // semantic analysis
    get_type(symtable);
}
token_t ast_equality_expression_node::get_ex_type_impl(const stable& symtable) const
{
    // note: equality is defined for numeric types only
    token_t types[2];
    for (short i = 0;i < 2;++i) {
        token_t t = _operands[i].node->get_type(symtable);
        if (t!=token_in && t!=token_small && t!=token_big)
            throw semantic_error("line %d: equality-operator requires numeric type operands",get_lineno());
        types[i] = t;
    }
    // since this operator will compare, demand type equality on the two operands
    if ( !semantic_type_equality(types[0],types[1]) )
        throw semantic_error("line %d: equality-operator types of '%s' and '%s' do not match",get_lineno(),
            semantic_type_name(types[0]),semantic_type_name(types[1]));
    // should always be a 'boo' type expression
    return token_boo;
}

void ast_relational_expression_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    for (short i = 0;i < 2;++i)
        _operands[i].node->check_semantics(symtable);
    // semantic analysis
    get_type(symtable);
}
token_t ast_relational_expression_node::get_ex_type_impl(const stable& symtable) const
{
    // note: relational operators are defined for numeric types only
    token_t types[2];
    for (short i = 0;i < 2;++i) {
        token_t t = _operands[i].node->get_type(symtable);
        if (t!=token_in && t!=token_small && t!=token_big)
            throw semantic_error("line %d: relational-operator requires numeric type operands",get_lineno());
        types[i] = t;
    }
    // since this operator will compare, demand type equality on the two operands
    if ( !semantic_type_equality(types[0],types[1]) )
        throw semantic_error("line %d: equality-operator operand types '%s' and '%s' do not match",get_lineno(),
            semantic_type_name(types[0]),semantic_type_name(types[1]));
    // should always be a 'boo' type expression
    return token_boo;
}

void ast_additive_expression_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    for (auto iter = _operands.begin();iter!=_operands.end();++iter)
        iter->node->check_semantics(symtable);
    // 'get_type' does symantic analysis
    get_type(symtable);
}
token_t ast_additive_expression_node::get_ex_type_impl(const stable& symtable) const
{
    token_t t = token_invalid;
    for (auto iter = _operands.begin();iter != _operands.end();++iter) {
        token_t curtype = iter->node->get_type(symtable);
        if (curtype!=token_in && curtype!=token_small && curtype!=token_big)
            throw semantic_error("line %d: additive-operator requires numeric type operands",get_lineno());
        // determine type of expression so far; promote small to big as needed; "in"
        // may go both ways (small or big)
        if (t==token_invalid || curtype==token_big || (t==token_in && curtype==token_small))
            t = curtype;
    }
    return t;
}

void ast_multiplicative_expression_node::semantics_impl(stable& symtable) const
{
    // do visitor pattern
    for (auto iter = _operands.begin();iter!=_operands.end();++iter)
        iter->node->check_semantics(symtable);
    // 'get_type' does symantic analysis
    get_type(symtable);
}
token_t ast_multiplicative_expression_node::get_ex_type_impl(const stable& symtable) const
{
    token_t t = token_invalid;
    for (auto iter = _operands.begin();iter != _operands.end();++iter) {
        token_t curtype = iter->node->get_type(symtable);
        if (curtype!=token_in && curtype!=token_small && curtype!=token_big)
            throw semantic_error("line %d: multiplicative-operator requires numeric type operands",get_lineno());
        // determine type of expression so far; promote small to big as needed; "in"
        // may go both ways (small or big)
        if (t==token_invalid || curtype==token_big || (t==token_in && curtype==token_small))
            t = curtype;
    }
    return t;
}

void ast_prefix_expression_node::semantics_impl(stable& symtable) const
{
    token_t t;
    // do visitor pattern
    _operand.node->check_semantics(symtable);
    t = _operand.node->get_type(symtable);
    if (_operator->type()==token_not && t!=token_boo) // operand must be 'boo' type
        throw semantic_error("line %d: not-operator requires 'boo' type operand",get_lineno());
    else if (_operator->type()==token_subtract && t!=token_in && t!=token_small 
        && t!=token_big) // operand must be numeric type
        throw semantic_error("line %d: negate-operator requires numeric type operand",get_lineno());
}
token_t ast_prefix_expression_node::get_ex_type_impl(const stable& symtable) const
{
    // the type of a prefix expression is the type of its operand (given that semantic checks have already
    // made sure that the types match up with what the operator expects)
    return _operand.node->get_type(symtable);
}

void ast_postfix_expression_node::semantics_impl(stable& symtable) const
{
    // make sure that '_op' is an identifier
    if (_op.node->get_kind()!=ast_primary_expression && !static_cast<ast_primary_expression_node*>(_op.node)->is_identifier())
        throw semantic_error("line %d: function name cannot be non-identifier",get_lineno());
    const symbol* sym;
    vector<token_t> args;
    ast_expression_node* p;
    symbol::match_parameters_result result;
    // lookup symbol based on '_op' identifier
    sym = symtable.getSymbol(static_cast<ast_primary_expression_node*>(_op.node)->name());
    if (sym == NULL)
        throw semantic_error("line %d: function '%s' is not declared",get_lineno(),static_cast<ast_primary_expression_node*>(_op.node)->name());
    // make sure that symbol is a function
    if (sym->get_kind() != symbol::skind_function)
        throw semantic_error("line %d: '%s' is not a function",get_lineno(),sym->get_name());
    // go through each expression and compile an argument type list
    p = _expList;
    while (p != NULL) {
        args.push_back(p->get_type(symtable));
        p = p->get_next();
    }
    // make sure function argument list is correct
    result = sym->match_parameters(&args[0],int(args.size()));
    if (result == symbol::match_too_few)
        throw semantic_error("line %d: too few arguments to function '%s'",get_lineno(),sym->get_name());
    if (result == symbol::match_too_many)
        throw semantic_error("line %d: too many arguments to function '%s'",get_lineno(),sym->get_name());
    if (result == symbol::match_bad_types)
        throw semantic_error("line %d: argument type mismatch to function '%s'",get_lineno(),sym->get_name());
}
token_t ast_postfix_expression_node::get_ex_type_impl(const stable& symtable) const
{
    // the function return type is the overall type for the expression
    return symtable.getSymbol(static_cast<ast_primary_expression_node*>(_op.node)->name())->get_type();
}

void ast_primary_expression_node::semantics_impl(stable& symtable) const
{
    get_type(symtable);
}
token_t ast_primary_expression_node::get_ex_type_impl(const stable& symtable) const
{
    if (_tok->type() == token_id) {
        const symbol* sym = symtable.getSymbol(_tok->source_string());
        if (sym == NULL)
            throw semantic_error("line %d: identifier '%s' is undeclared",get_lineno(),_tok->source_string());
        return sym->get_type();
    }
    if (_tok->type()==token_bool_true || _tok->type()==token_bool_false)
        return token_boo;
    if (_tok->type() == token_number)
        return token_in;
#ifdef RAMSEY_DEBUG
    throw ramsey_exception();
#endif
    return token_invalid;
}
