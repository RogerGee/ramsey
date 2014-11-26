/* parser.cpp */
#include "parser.h"
#include <cerrno>
#include <cstring>
#include <cctype>
using namespace std;
using namespace ramsey;

// exception types
parser_error::parser_error(const char* format, ...)
{
    va_list args;
    va_start(args,format);
    _cstor(format,args);
    va_end(args);
}
parser_exception::parser_exception(const char* format, ...)
{
    va_list args;
    va_start(args,format);
    _cstor(format,args);
    va_end(args);
}

// ramsey::parser

parser::parser(const char* file)
    : linenumber(1), lex(file), ast(NULL)
{
    program();
}

parser::~parser()
{
    if (ast != NULL)
        delete ast;
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
    // parse the program and build the abstract syntax tree
    ast_function_builder builder;
    builders.push(&builder);
    function_list();
    builders.pop();
    ast = builder.build();
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
    // parse statement list and add statements to AST
    ast_statement_builder statementBuilder;
    builders.push(&statementBuilder);
    statement_list();
    builders.pop();
    builders.top()->add_node( statementBuilder.build() );
    if (lex.curtok().type() == token_endfun)
        ++lex;
    else
        throw parser_error("line %d: expected 'endfun' after function body", linenumber);
    if (!eol())
        throw parser_error("line %d: expected newline after function", linenumber);
}

void parser::function_declaration()
{
    ++lex; // move past 'fun' token
    if (lex.curtok().type() == token_id) {
        // keep the identifier in the AST
        builders.top()->add_token(&lex.curtok());
        ++lex;
    }
    else
        throw parser_error("line %d: expected identifier in function declaration", linenumber);
    if (lex.curtok().type() == token_oparen)
        ++lex;
    else
        throw parser_error("line %d: expected '(' after function name", linenumber);
    // parse the parameter declaration and put it in the AST
    ast_parameter_builder paramBuilder;
    builders.push(&paramBuilder);
    parameter_declaration();
    builders.pop();
    builders.top()->add_node( paramBuilder.build() );
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
    else if (lex.curtok().type() == token_eol) {
        // if no specifier is found, then a function defaults to type "in"; place
        // a NULL pointer in the builder to account for this
        builders.top()->add_token(NULL);
        return;
    }
    else
        throw parser_error("line %d: bad function type specifier", linenumber);
}

void parser::parameter_declaration()
{
    if (lex.curtok().type() == token_in || lex.curtok().type() == token_big
        || lex.curtok().type() == token_small || lex.curtok().type() == token_boo)
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
    if (lex.curtok().type() == token_id) {
        builders.top()->add_token(&lex.curtok());
        ++lex;
    }
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
    {
        // next token is an identifier, user needs comma
        if (lex.curtok().type() == token_in || lex.curtok().type() == token_big
            || lex.curtok().type() == token_small || lex.curtok().type() == token_boo)
            throw parser_error("line %d: expected ',' in parameter list", linenumber);
        // next token indicates end of parameters, user needs cparen
        else if (lex.curtok().type() == token_as || lex.curtok().type() == token_eol)
            throw parser_error("line %d: expected ')' after parameter list", linenumber);
        else
            throw parser_error("line %d: unexpected token in parameter list '%s'", linenumber, lex.curtok().to_string().c_str());
    }
}

void parser::statement()
{
    if (lex.curtok().type() == token_in || lex.curtok().type() == token_big || lex.curtok().type() == token_small
        || lex.curtok().type() == token_boo) {
        ast_declaration_statement_builder declStatBuilder;
        builders.push(&declStatBuilder);
        declaration_statement();
        builders.pop();
        builders.top()->add_node( declStatBuilder.build() );
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_not
        || lex.curtok().type() == token_id || lex.curtok().type() == token_bool_true
        || lex.curtok().type() == token_bool_false || lex.curtok().type() == token_number
        || lex.curtok().type() == token_number_hex || lex.curtok().type() == token_string
        || lex.curtok().type() == token_oparen) {
        ast_expression_statement_builder expStatBuilder;
        builders.push(&expStatBuilder);
        expression_statement();
        builders.pop();
        builders.top()->add_node( expStatBuilder.build() );
    }
    else if (lex.curtok().type() == token_if) {
        ast_selection_statement_builder selBuilder;
        builders.push(&selBuilder);
        selection_statement();
        builders.pop();
        builders.top()->add_node( selBuilder.build() );
    }
    else if (lex.curtok().type() == token_while) {
        ast_iterative_statement_builder iterBuilder;
        builders.push(&iterBuilder);
        iterative_statement();
        builders.pop();
        builders.top()->add_node( iterBuilder.build() );
    }
    else if (lex.curtok().type() == token_toss || lex.curtok().type() == token_smash) {
        ast_jump_statement_builder jumpBuilder;
        builders.push(&jumpBuilder);
        jump_statement();
        builders.pop();
        builders.top()->add_node( jumpBuilder.build() );
    }
    else
        throw parser_error("line %d: malformed statement", linenumber);
}

void parser::statement_list()
{
    if (lex.curtok().type() == token_in || lex.curtok().type() == token_big ||
        lex.curtok().type() == token_small || lex.curtok().type() == token_boo ||
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
    if (lex.curtok().type() == token_id) {
        // keep the identifier in the AST
        builders.top()->add_token(&lex.curtok());
        ++lex;
    }
    else
        throw parser_error("line %d: expected identifier in declaration statement", linenumber);
    initializer();
    if (!eol())
        throw parser_error("line %d: newline expected after declaration statement", linenumber);
}

void parser::type_name()
{
    if (lex.curtok().type()==token_in || lex.curtok().type() == token_big ||
        lex.curtok().type() == token_small || lex.curtok().type()==token_boo) {
        // keep the type name specifier in the AST
        builders.top()->add_token(&lex.curtok());
        ++lex;
    }
    else
        throw parser_error("line %d: expected typename specifier", linenumber);
}

void parser::initializer()
{
    if (lex.curtok().type() == token_assign)
    {
        assignment_operator();
        // parse the expression and put it in the AST
        ast_expression_builder expBuilder;
        builders.push(&expBuilder);
        expression();
        builders.pop();
        builders.top()->add_node( expBuilder.build() );
    }
    else if (lex.curtok().type() == token_eol) {
        builders.top()->add_node(NULL); // store empty initializer in AST
        return;
    }
    else
        throw parser_error("line %d: malformed initializer", linenumber);
}

void parser::assignment_operator()
{
    // note: at this point the assignment operator is not included in the AST
    if (lex.curtok().type() == token_assign)
        ++lex;
    else
        throw parser_error("line %d: expected assignment operator in expression",linenumber);
    /* note: if we wanted to add compound assignment operators to the grammar, then a new grammar
       rule would have to be created since compound assignment operators would not be usable in
       some contexts where an assignment operator is valid (e.g. declaration initializers) */
}

void parser::expression_statement()
{
    expression_list();
    if (!eol())
        throw parser_error("line %d: expected newline after expression statement", linenumber);
}

void parser::expression()
{
    if (lex.curtok().type() == token_id || lex.curtok().type() == token_number ||
        lex.curtok().type() == token_number_hex || lex.curtok().type() == token_bool_true ||
        lex.curtok().type() == token_bool_false || lex.curtok().type() == token_oparen ||
        lex.curtok().type() == token_subtract || lex.curtok().type() == token_not)
        assignment_expression();
    else
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::expression_list()
{
    // there should already be an expression builder on the stack
    // for building the list; this new expression builder builds
    // a single list item for the expression list
    ast_expression_builder expBuilder;
    builders.push(&expBuilder);
    expression();
    builders.pop();
    builders.top()->add_node( expBuilder.build() );
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
    // build assignment expression if it exists
    ast_assignment_expression_builder assignBuilder;
    builders.push(&assignBuilder);
    logical_or_expression();
    assignment_expression_opt();
    builders.pop();
    if (assignBuilder.size() > 1)
        builders.top()->add_node( assignBuilder.build() );
    else
        builders.top()->collapse(assignBuilder);
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
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::logical_or_expression()
{
    // build logical-or expression if it exists
    ast_logical_or_expression_builder orBuilder;
    builders.push(&orBuilder);
    logical_and_expression();
    logical_or_expression_opt();
    builders.pop();
    if (orBuilder.size() > 1)
        builders.top()->add_node( orBuilder.build() );
    else
        builders.top()->collapse(orBuilder);
}

void parser::logical_or_expression_opt()
{
    if (lex.curtok().type() == token_or)
    {
        ++lex;
        logical_and_expression();
        logical_or_expression_opt();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign)
        return;
    else
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::logical_and_expression()
{
    // build logical-and expression if it exists
    ast_logical_and_expression_builder andBuilder;
    builders.push(&andBuilder);
    equality_expression();
    logical_and_expression_opt();
    builders.pop();
    if (andBuilder.size() > 1)
        builders.top()->add_node( andBuilder.build() );
    else
        builders.top()->collapse(andBuilder);
}

void parser::logical_and_expression_opt()
{
    if (lex.curtok().type() == token_and)
    {
        ++lex;
        equality_expression();
        logical_and_expression_opt();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or)
        return;
    else
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::equality_expression()
{
    // build equality expression if it exists
    ast_equality_expression_builder equalBuilder;
    builders.push(&equalBuilder);
    relational_expression();
    equality_expression_opt();
    builders.pop();
    if (equalBuilder.size() > 1)
        builders.top()->add_node( equalBuilder.build() );
    else
        builders.top()->collapse(equalBuilder);
}

void parser::equality_expression_opt()
{
    if (lex.curtok().type()==token_equal || lex.curtok().type()==token_nequal)
    {
        // keep equality operator in AST
        builders.top()->add_token(&lex.curtok());
        ++lex;
        relational_expression();
        equality_expression_opt();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and)
        return;
    else
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::relational_expression()
{
    // build relational expression if it exists
    ast_relational_expression_builder relationBuilder;
    builders.push(&relationBuilder);
    additive_expression();
    relational_expression_opt();
    builders.pop();
    if (relationBuilder.size() > 1)
        builders.top()->add_node( relationBuilder.build() );
    else
        builders.top()->collapse(relationBuilder);
}

void parser::relational_expression_opt()
{
    if (lex.curtok().type() == token_less || lex.curtok().type()==token_greater
        || lex.curtok().type()==token_le || lex.curtok().type()==token_ge)
    {
        // keep relational operator in the AST
        builders.top()->add_token(&lex.curtok());
        ++lex;
        additive_expression();
        relational_expression_opt();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and ||
        lex.curtok().type() == token_equal || lex.curtok().type() == token_nequal)
        return;
    else
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::additive_expression()
{
    // build additive expression if it exists
    ast_additive_expression_builder additiveBuilder;
    builders.push(&additiveBuilder);
    multiplicative_expression();
    additive_expression_opt();
    builders.pop();
    if (additiveBuilder.size() > 1)
        builders.top()->add_node( additiveBuilder.build() );
    else
        builders.top()->collapse(additiveBuilder);
}

void parser::additive_expression_opt()
{
    if (lex.curtok().type()==token_add || lex.curtok().type()==token_subtract)
    {
        // keep additive operator in the AST
        builders.top()->add_token(&lex.curtok());
        ++lex;
        multiplicative_expression();
        additive_expression_opt();
    }
    else if (lex.curtok().type() == token_cparen || lex.curtok().type() == token_eol ||
        lex.curtok().type() == token_comma || lex.curtok().type() == token_assign ||
        lex.curtok().type() == token_or || lex.curtok().type() == token_and ||
        lex.curtok().type() == token_equal || lex.curtok().type() == token_nequal ||
        lex.curtok().type() == token_less || lex.curtok().type() == token_greater ||
        lex.curtok().type() == token_le || lex.curtok().type() == token_ge)
        return;
    else
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::multiplicative_expression()
{
    ast_multiplicative_expression_builder multipBuilder;
    builders.push(&multipBuilder);
    prefix_expression();
    multiplicative_expression_opt();
    builders.pop();
    if (multipBuilder.size() > 1)
        builders.top()->add_node( multipBuilder.build() );
    else
        builders.top()->collapse(multipBuilder);
}

void parser::multiplicative_expression_opt()
{
    if (lex.curtok().type()==token_multiply || lex.curtok().type()==token_divide || lex.curtok().type()==token_mod)
    {
        // keep multiplicative operator in the AST
        builders.top()->add_token(&lex.curtok());
        ++lex;
        prefix_expression();
        multiplicative_expression_opt();
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
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::prefix_expression()
{
    if (lex.curtok().type() == token_id || lex.curtok().type() == token_number ||
        lex.curtok().type() == token_number_hex || lex.curtok().type() == token_bool_true ||
        lex.curtok().type() == token_bool_false || lex.curtok().type() == token_oparen)
        postfix_expression();
    else if (lex.curtok().type()==token_subtract || lex.curtok().type()==token_not)
    {
        ast_prefix_expression_builder prefixBuilder;
        prefixBuilder.add_token(&lex.curtok());
        ++lex;
        builders.push(&prefixBuilder);
        prefix_expression();
        builders.pop();
        builders.top()->add_node( prefixBuilder.build() );
    }
    else
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::postfix_expression()
{
    // build a postfix expression if it exists
    ast_postfix_expression_builder postfixBuilder;
    builders.push(&postfixBuilder);
    primary_expression();
    postfix_expression_opt();
    builders.pop();
    if (postfixBuilder.size() > 1)
        builders.top()->add_node( postfixBuilder.build() );
    else
        builders.top()->collapse(postfixBuilder);
}

void parser::postfix_expression_opt()
{
    if (lex.curtok().type() == token_oparen)
    {
        ++lex;
        function_call();
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
        throw parser_error("line %d: unexpected token in expression: '%s'", linenumber, lex.curtok().to_string().c_str());
}

void parser::function_call()
{
    if (lex.curtok().type() == token_cparen) { // follow set
        builders.top()->add_node(NULL);
        return;
    }
    ast_expression_builder expBuild;
    builders.push(&expBuild);
    expression_list();
    builders.pop();
    builders.top()->add_node( expBuild.build() );
}

void parser::primary_expression()
{
    if (lex.curtok().type() == token_number || lex.curtok().type() == token_number_hex ||
        lex.curtok().type() == token_bool_true || lex.curtok().type() == token_bool_false ||
        lex.curtok().type() == token_id)
    {
        builders.top()->add_token(&lex.curtok());
        ++lex;
    }
    else if (lex.curtok().type() == token_oparen)
    {
        ++lex;
        ast_expression_builder expBuilder;
        builders.push(&expBuilder);
        expression();
        builders.pop();
        builders.top()->add_node( expBuilder.build() );
        if (lex.curtok().type() == token_cparen)
            ++lex;
        else
            throw parser_error("line %d: expected ')' in primary expression", linenumber);
    }
    else
        throw parser_error("line %d: expected primary expression", linenumber);
}

void parser::selection_statement()
{
    ++lex;
    if (lex.curtok().type() == token_oparen)
        ++lex;
    else
        throw parser_error("line %d: '(' must follow 'if'", linenumber);
    ast_expression_builder expBuilder;
    builders.push(&expBuilder);
    expression();
    builders.pop();
    builders.top()->add_node( expBuilder.build() );
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
    ast_statement_builder statBuilder;
    builders.push(&statBuilder);
    statement_list();
    builders.pop();
    builders.top()->add_node( statBuilder.build() );
    // build another selection statement node to handle elf-clause if it exists
    ast_elf_builder elfBuilder;
    builders.push(&elfBuilder);
    elf_body();
    builders.pop();
    builders.top()->add_node(elfBuilder.is_empty() ? NULL : elfBuilder.build());
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
        ast_expression_builder expBuild;
        builders.push(&expBuild);
        expression();
        builders.pop();
        builders.top()->add_node( expBuild.build() );
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
        builders.top()->add_node(NULL); // mark empty else-block
    }
    else if (lex.curtok().type() == token_else)
    {
        ++lex;
        if (!eol())
            throw parser_error("line %d: expected newline after 'else'", linenumber);
        ast_statement_builder statBuilder;
        builders.push(&statBuilder);
        statement_list();
        builders.pop();
        builders.top()->add_node( statBuilder.build() );
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
    ast_expression_builder expBuilder;
    builders.push(&expBuilder);
    expression();
    builders.pop();
    builders.top()->add_node( expBuilder.build() );
    if (lex.curtok().type() == token_cparen)
        ++lex;
    else
        throw parser_error("line %d: expected ')' after iterative condition", linenumber);
    if (!eol())
        throw parser_error("line %d: expected newline after iterative condition", linenumber);
    ast_statement_builder statBuilder;
    builders.push(&statBuilder);
    statement_list();
    builders.pop();
    builders.top()->add_node( statBuilder.build() );
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
        builders.top()->add_token(&lex.curtok());
        ++lex;
        ast_expression_builder expBuilder;
        builders.push(&expBuilder);
        expression_list();
        builders.pop();
        builders.top()->add_node( expBuilder.build() );
        if (!eol())
            throw parser_error("line %d: expected newline after 'toss'", linenumber);
    }
    else if (lex.curtok().type() == token_smash)
    {
        ++lex;
        builders.top()->add_token(&lex.curtok());
        if (!eol())
            throw parser_error("line %d: expected newline after 'smash'", linenumber);
    }
    // this shouldn't run since where jump-statement is called
    // we check curtok==token_smash || curtok==token_toss
    else
        throw parser_error("line %d: malformed jump statement", linenumber);
}
