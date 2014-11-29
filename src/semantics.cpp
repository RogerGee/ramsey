/* semantics.cpp */
#include "ast.h"
using namespace ramsey;

void ast_node::check_semantics(stable& symtable) const
{
    symtable.addScope(); // create a default scope for the current level
    semantics_impl(symtable); // analyze semantics at current level
    symtable.remScope();
}

// implementations for each AST-node's 'semantics_impl' overload
void ast_function_node::semantics_impl(stable& symtable) const
{
    // TODO: function symbols need to store complete signiture
    if ( !symtable.add(this) )
        throw semantic_error("redeclaration of function '%s'",_id->source_string());
    while ( !end() )
        get_next()->semantics_impl(symtable);
    symtable.addScope(); // scope for function
    if (_param != NULL)
        _param->check_semantics(symtable);
    _statements->check_semantics(symtable);
    symtable.remScope();
}

void ast_parameter_node::semantics_impl(stable&) const
{

}

void ast_declaration_statement_node::semantics_impl(stable&) const
{
}

void ast_selection_statement_node::semantics_impl(stable&) const
{
}

void ast_elf_node::semantics_impl(stable&) const
{
}

void ast_iterative_statement_node::semantics_impl(stable&) const
{
}

void ast_jump_statement_node::semantics_impl(stable&) const
{
}

void ast_assignment_expression_node::semantics_impl(stable&) const
{
}

void ast_logical_or_expression_node::semantics_impl(stable&) const
{
}

void ast_logical_and_expression_node::semantics_impl(stable&) const
{
}

void ast_equality_expression_node::semantics_impl(stable&) const
{
}

void ast_relational_expression_node::semantics_impl(stable&) const
{
}

void ast_additive_expression_node::semantics_impl(stable&) const
{
}

void ast_multiplicative_expression_node::semantics_impl(stable&) const
{
}

void ast_prefix_expression_node::semantics_impl(stable&) const
{
}

void ast_postfix_expression_node::semantics_impl(stable&) const
{
}

void ast_primary_expression_node::semantics_impl(stable&) const
{
}
