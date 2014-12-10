/* codegen.cpp */
#include "codegen.h"
#include "ast.h"
#include <cctype>
using namespace std;
using namespace ramsey;

// code_generator
code_generator::code_generator(ostream& output)
    : _output(output), _alloc(0), _narg(0), _reghead(reg_invalid), _regcnt(-1),
      _lbl(1)
{
    _before.flags(ios_base::left | _before.flags());
    _body.flags(ios_base::left | _body.flags());
}
void code_generator::begin_function(const char* name)
{
#ifdef RAMSEY_WIN32
    // MSWindows needs prefix underscore on symbol name
    instruction_before(".globl _%s",name);
    instruction_before(".type _%s, @function",name);
    _before << '_' << name << ":\n";
#else
    instruction_before(".globl %s",name);
    instruction_before(".type %s, @function",name);
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
void code_generator::writeline(const char* format, ...)
{
    static const size_t BUFMAX = 4097;
    va_list vargs;
    char buffer[BUFMAX];
    va_start(vargs,format);
    vsnprintf(buffer,BUFMAX-1,format,vargs);
    _body << buffer << '\n';
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
void code_generator::allocate_result_register()
{
    ++_regcnt;
    _reghead = (_register)(_regcnt % reg_end);
    if (_regcnt >= reg_end)
        instruction("pushl %%%s",current_result_register());
}
const char* code_generator::current_result_register(token_t type) const
{
#ifdef RAMSEY_DEBUG
    if (_reghead == reg_invalid)
        throw ramsey_exception();
#endif
    static const char* LONG_REGISTERS[] = {"eax","ebx","ecx","edx"};
    static const char* WORD_REGISTERS[] = {"ax","bx","cx","dx"};
    static const char* BYTE_REGISTERS[] = {"al","bl","cl","dl"};
    if (type==token_in || type==token_big)
        return LONG_REGISTERS[_reghead];
    else if (type == token_small)
        return WORD_REGISTERS[_reghead];
    // type == token_boo
    return BYTE_REGISTERS[_reghead];
}
void code_generator::deallocate_result_register()
{
#ifdef RAMSEY_DEBUG
    if (_regcnt < 0)
        throw ramsey_exception();
#endif
    if (_regcnt >= reg_end)
        instruction("popl %%%s",current_result_register());
    --_regcnt;
    _reghead = _regcnt<0 ? reg_invalid : (_register)(_regcnt % reg_end);
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

/*static*/ const char* ast_expression_node::load_operand(const ast_expression_node::operand& op,stable& symtable,code_generator& cgen,bool alloc)
{
    // loading an operand into a register is a very common operation performed by expression nodes
    if (alloc)
        cgen.allocate_result_register();
    const char* reg = cgen.expects_result() ? cgen.current_result_register() : NULL;
    if (op.node->get_kind()!=ast_expression_node::ast_primary_expression || reg==NULL) {
        cgen.allocate_result_register();
        op.node->generate_code(symtable,cgen);
        // if no result register was allocated then keep the result in EAX; otherwise move the result to the current result register
        if (reg != NULL)
            cgen.instruction("movl %%%s, %%%s",cgen.current_result_register(),reg);
        cgen.deallocate_result_register();
    }
    else
        op.node->generate_code(symtable,cgen);
    // return the register into which the operand was loaded
    return reg==NULL ? "eax" : reg;
}

void ast_function_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    // add symbol and process all remaining functions
    symtable.add(this);
    if ( !end() )
        get_next()->codegen_impl(symtable,cgen);
    // generate code for function body
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
void ast_parameter_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    const_cast<ast_parameter_node*>(this)->set_offset( cgen.next_argument_offset() );
    symtable.add(this);
}
void ast_declaration_statement_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    // get stack address offset
    const_cast<ast_declaration_statement_node*>(this)->set_offset( cgen.next_variable_offset(_typespec->type()) );
    // do initializer assignment
    if (_initializer != NULL) {
        token_t type = get_type();
        cgen.allocate_result_register();
        _initializer->generate_code(symtable,cgen);
        if (type==token_in || type==token_big)
            cgen.instruction("movl %%%s, %d(%%ebp)",cgen.current_result_register(type),get_offset());
        else if (type == token_small)
            cgen.instruction("movw %%%s, %d(%%ebp)",cgen.current_result_register(type),get_offset());
        else // token_boo
            cgen.instruction("movb %%%s, %d(%%ebp)",cgen.current_result_register(type),get_offset());
        cgen.deallocate_result_register();
    }
    // add symbol after assignment
    symtable.add(this);
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
void ast_assignment_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    token_t type = get_type();
    const symbol* obj = symtable.getSymbol( static_cast<ast_primary_expression_node*>(_ops[0].node)->name() );
    // generate code for the right-hand expression
    cgen.allocate_result_register();
    _ops[1].node->generate_code(symtable,cgen);
    // assign the right-hand expression to the left hand identifier; semantic analysis guarentees lvalue
    if (type==token_in || type==token_big)
        cgen.instruction("movl %%%s, %d(%%ebp)",cgen.current_result_register(type),obj->get_offset());
    else if (type == token_small)
        cgen.instruction("movw %%%s, %d(%%ebp)",cgen.current_result_register(type),obj->get_offset());
    else // token_boo
        cgen.instruction("movb %%%s, %d(%%ebp)",cgen.current_result_register(type),obj->get_offset());
    cgen.deallocate_result_register();
    // this expression returns the value of its left-operand, so if a result is expected, assign it to the 
    // current result register
    if (cgen.expects_result())
        cgen.instruction("movl %d(%%ebp), %%%s",obj->get_offset(),cgen.current_result_register());
}
void ast_logical_or_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement logic-OR, we test to see if a term is non-zero; if so, the control assigns
       will jump to a block that assigns 1 to the result register; otherwise control falls through
       to test the next term; 0 is assigned in the default case; if no assignable space is available,
       then a nop is issued and a processor cycle is wasted */
    int lbltrue = cgen.get_unique_label(), lblfalse = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    const char* reg = load_operand(_ops[0],symtable,cgen); // process the first term independently to effectively handle the else case
    // process the rest of terms; the grammar guarantees at least 2 (so at least 1 for the below loop)
    for (size_t i = 1;i < _ops.size();++i) {
        // do comparison for previous term; if non-zero then jump to true block
        cgen.instruction("cmpl $0, %%%s",reg);
        cgen.instruction("jne lbl%d",lbltrue);
        // process the next term in the sequence
        reg = load_operand(_ops[i],symtable,cgen);
    }
    // insert jump for false case when none of the terms are non-zero
    cgen.instruction("cmpl $0, %%%s",reg);
    if ( cgen.expects_result() )
        cgen.instruction("je lbl%d",lblfalse);
    // define lbltrue
    cgen.writeline("lbl%d:",lbltrue);
    if ( !cgen.expects_result() )
        cgen.instruction("nop");
    else {
        cgen.instruction("movl $1, %%%s",reg);
        cgen.instruction("jmp lbl%d",lbldone);
        // define lblfalse
        cgen.writeline("lbl%d:",lblfalse);
        cgen.instruction("movl $0, %%%s",reg);
    }
    // define lbldone
    cgen.writeline("lbl%d:",lbldone);
}
void ast_logical_and_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement logic-AND, we test each term to see if it is zero; if so then control
       jumps to a block that assigns 0 to the result register; otherwise control falls through
       to test each term until the success case is hit at the bottom; do a nop if no result register is active */
    int lblfalse = cgen.get_unique_label(), lbltrue = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    const char* reg = load_operand(_ops[0],symtable,cgen); // process the first term independently to effectively handle the else case
    // process the rest of terms; the grammar guarantees at least 2 (so at least 1 for the below loop)
    for (size_t i = 1;i < _ops.size();++i) {
        // do comparison for previous term; if non-zero then jump to true block
        cgen.instruction("cmpl $0, %%%s",reg);
        cgen.instruction("je lbl%d",lblfalse);
        // process the next term in the sequence
        reg = load_operand(_ops[i],symtable,cgen);
    }
    // insert jump for false case when none of the terms are non-zero
    cgen.instruction("cmpl $0, %%%s",reg);
    if ( cgen.expects_result() )
        cgen.instruction("jne lbl%d",lbltrue);
    // define lblfalse
    cgen.writeline("lbl%d:",lblfalse);
    if ( !cgen.expects_result() )
        cgen.instruction("nop");
    else {
        cgen.instruction("movl $0, %%%s",reg);
        cgen.instruction("jmp lbl%d",lbldone);
        // define lbltrue
        cgen.writeline("lbl%d:",lbltrue);
        cgen.instruction("movl $1, %%%s",reg);
    }
    // define lbldone
    cgen.writeline("lbl%d:",lbldone);
}
void ast_equality_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement EQUALITY, we load up the result of the left and right operands and compare them; based
       on which equality operator is used, a jump instruction takes control to either a success or fail block;
       a nop is issued if no result register has been designated by a parent node */
    bool hasResult = cgen.expects_result();
    const char* regA = load_operand(_operands[0],symtable,cgen,!hasResult), *regB = load_operand(_operands[1],symtable,cgen,true);
    int lbltrue = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    // note: regA will be assign-to register if hasResult==true
    cgen.instruction("cmp %%%s, %%%s",regA,regB);
    cgen.deallocate_result_register();
    if (!hasResult)
        cgen.deallocate_result_register();
    cgen.instruction("%s lbl%d",_operator->type()==token_equal ? "je" : "jne",lbltrue);
    if (hasResult)
        cgen.instruction("movl $0, %%%s",regA);
    else
        cgen.instruction("nop");
    cgen.instruction("jmp lbl%d",lbldone);
    cgen.writeline("lbl%d:",lbltrue);
    if (hasResult)
        cgen.instruction("movl $1, %%%s",regA);
    else
        cgen.instruction("nop");
    cgen.writeline("lbl%d:",lbldone);
}
void ast_relational_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement RELATIONAL, we load up the result of the left and right operands and compare them; based
       on which relational operator is used, a jump instruction takes control to either a success or fail block;
       a nop is issued if no result register has been designated by a parent node */
    bool hasResult = cgen.expects_result();
    const char* regA = load_operand(_operands[0],symtable,cgen,!hasResult), *regB = load_operand(_operands[1],symtable,cgen,true);
    int lbltrue = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    const char* jmp;
    // note: regA will be assign-to register if hasResult==true
    cgen.instruction("cmp %%%s, %%%s",regA,regB);
    cgen.deallocate_result_register();
    if (!hasResult)
        cgen.deallocate_result_register();
    // decide which operator to use
    if (_operator->type() == token_less)
        jmp = "jl";
    else if (_operator->type() == token_greater)
        jmp = "jg";
    else if (_operator->type() == token_le)
        jmp = "jle";
    else // token_ge
        jmp = "jge";
    cgen.instruction("%s lbl%d",jmp,lbltrue);
    if (hasResult)
        cgen.instruction("movl $0, %%%s",regA);
    else
        cgen.instruction("nop");
    cgen.instruction("jmp lbl%d",lbldone);
    cgen.writeline("lbl%d:",lbltrue);
    if (hasResult)
        cgen.instruction("movl $1, %%%s",regA);
    else
        cgen.instruction("nop");
    cgen.writeline("lbl%d:",lbldone);
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
void ast_primary_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    // default behavior is to load the value into the result register; this function should
    // not be called on every primary expression node; most expressions use the values directly
    if (_tok->type() == token_id) {
        token_t type;
        const symbol* sym = symtable.getSymbol(_tok->source_string());
        type = sym->get_type();
        if (type==token_in || type==token_big)
            cgen.instruction("movl %d(%%ebp), %%%s",sym->get_offset(),cgen.current_result_register());
        else if (type == token_small)
            cgen.instruction("movswl %d(%%ebp), %%%s",sym->get_offset(),cgen.current_result_register());
        else // token_boo
            cgen.instruction("movsbl %d(%%ebp), %%%s",sym->get_offset(),cgen.current_result_register());
    }
    else {
        if (_tok->type() == token_number)
            cgen.instruction("movl $%s, %%%s",_tok->source_string(),cgen.current_result_register());
        else if (_tok->type() == token_bool_false) // use 0 for false
            cgen.instruction("movl $0, %%%s",cgen.current_result_register());
        else // token_bool_true (use 1 for true)
            cgen.instruction("movl $1, %%%s",cgen.current_result_register());
    }
}
