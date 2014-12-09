/* codegen.cpp */
#include "codegen.h"
#include "ast.h"
#include <cctype>
using namespace std;
using namespace ramsey;

// code_generator
code_generator::code_generator(ostream& output)
    : _output(output), _alloc(0), _narg(0)
{
    _before.flags(ios_base::left | _before.flags());
    _body.flags(ios_base::left | _body.flags());
}
void code_generator::begin_function(const char* name)
{
#ifdef RAMSEY_WIN32
    // MSWindows needs prefix underscore on symbol name
    instruction_before(".globl _%s, @function",name);
    _before << '_' << name << ":\n";
#else
    instruction_before(".globl %s, @function",name);
    _before << name << ":\n";
#endif
    instruction_impl(_before,"push\0%ebp");
    instruction_impl(_before,"movl\0%esp, %ebp");
}
void code_generator::end_function()
{
    if (_alloc > 0) // do stack allocation; this value should be aligned at a 4-byte boundry
        instruction_before("subl $%d, %%esp",_alloc);
    // do function stack cleanup
    instruction_impl(_body,"leave\0");
    instruction_impl(_body,"ret\0");
    // place preamble into output, then rest of function body
    _output << _before.str() << _body.str() << endl;
    // reset stringstream objects
    _before.str(string()); _body.str(string());
}
void code_generator::instruction(const char* format, ...)
{
    va_list vargs;
    va_start(vargs,format);
    instruction_impl(_body,format,vargs);
    va_end(vargs);
}
void code_generator::instruction_before(const char* format, ...)
{
    va_list vargs;
    va_start(vargs,format);
    instruction_impl(_before,format,vargs);
    va_end(vargs);
}
int code_generator::next_variable_offset(token_t type)
{
    /* variables are allocated on negative offsets from
       the stack base pointer; the allocator allocates
       three different kinds of variables:
        - longs: 4 bytes
        - words: 2 bytes
        - bytes: 1 byte
       the allocator will queue up available allocations by
       type and use them as needed; the allocation count will
       be kept at a 4-byte boundry */
    // grab iterator to queue of available allocations by type
    int iter, offset;
    static const int widths[] = {4,2,1};
    if (type == token_boo)
        iter = 2;
    else if (type == token_small)
        iter = 1;
    else if (type==token_in || type==token_big)
        iter = 0;
#ifdef RAMSEY_DEBUG
    else
        throw ramsey_exception();
#endif
    if (_allocations[iter].empty()) {
        int i = iter-1;
        while (i>=0 && _allocations[i].empty())
            --i;
        if (i < 0) {
            // increase overall allocation by 16 bytes
            int old = _alloc+4;
            _alloc += 16;
            for (;old <= _alloc;old+=4)
                _allocations[0].push(old);
            i = 0;
        }
        // take allocation space from other allocation area
        for (;i < iter;++i) {
            int off = _allocations[i].front();
            _allocations[i].pop();
            _allocations[i+1].push(off-widths[i+1]);
            _allocations[i+1].push(off);
        }
    }
    offset = _allocations[iter].front();
    _allocations[iter].pop();
    return -offset;
}
int code_generator::next_argument_offset()
{
    /* arguments are always allocated in 4-byte chunks on 
       positive offsets from 8+stack base pointer */
    return 8 + 4 * _narg++;
}
/*static*/ void code_generator::instruction_impl(ostream& output,const char* source)
{
    // 'source' must have 2 null bytes
    output.put('\t');
    output.width(7);
    output << source; // output instruction
    while (*source++); // find optional argument
    output << source << '\n';
}
/*static*/ void code_generator::instruction_impl(ostream& output,const char* format,va_list vargs)
{
    static const size_t BUFMAX = 4098;
    int len;
    char buffer[BUFMAX];
    len = vsnprintf(buffer,BUFMAX-2,format,vargs);
    buffer[len+1] = 0; // write the (potential) second null byte
    // section off the instruction 
    int iter = 0;
    while (buffer[iter] && !isspace(buffer[iter]))
        ++iter;
    buffer[iter++] = 0;
    instruction_impl(output,buffer);
}

// code generation implementation for AST node types
void ast_function_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    symtable.add(this);
    if ( !end() )
        get_next()->codegen_impl(symtable,cgen);
    // do visitor pattern
    symtable.addScope();
    symtable.enterFunction(this);
    cgen.begin_function(_id->source_string());
    { ast_parameter_node* n = _param;
        while (n != NULL) {
            n->generate_code(symtable,cgen);
            n = n->get_next();
        }
    }
    { ast_statement_node* n = _statements;
        while (n != NULL) {
            n->generate_code(symtable,cgen);
            n = n->get_next();
        }
    }
    cgen.end_function();
    symtable.exitFunction();
    symtable.remScope();
}
void ast_parameter_node::codegen_impl(stable&,code_generator&) const
{

}
void ast_declaration_statement_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_selection_statement_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_elf_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_iterative_statement_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_jump_statement_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_assignment_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_logical_or_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_logical_and_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_equality_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_relational_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_additive_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_multiplicative_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_prefix_expression_node::codegen_impl(stable&,code_generator&) const
{
}
void ast_postfix_expression_node::codegen_impl(stable&,code_generator&) const
{
}
