/* codegen.cpp */
#include "codegen.h"
#include "ast.h"
#include <cstring>
#include <cctype>
using namespace std;
using namespace ramsey;

// code_generator
code_generator::code_generator(ostream& output)
    : _output(output), _alloc(0), _narg(0), _reghead(reg_invalid), _regcnt(-1),
      _lbl(1), _retlbl(0)
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
    instruction_impl(_before,"pushl\0%ebp");
    instruction_impl(_before,"movl\0%esp, %ebp");
}
void code_generator::end_function()
{
    if (_retlbl > 0) {
        _body << "lbl" << _retlbl << ":\n";
        _retlbl = 0;
    }
    if (_alloc > 0) { // do stack allocation; this value should be aligned at a 4-byte boundry
        instruction_before("subl $%d, %%esp",_alloc);
        // do function stack cleanup
        instruction_impl(_body,"leave\0");
    }
    else
        // do function stack cleanup (ESP does not need adjustment)
        instruction("popl %%ebp");
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
        throw ramsey_exception("code_generator::next_variable_offset");
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
void code_generator::deallocate_result_register()
{
#ifdef RAMSEY_DEBUG
    if (_regcnt < 0)
        throw ramsey_exception("code_generator::deallocate_result_register");
#endif
    if (_regcnt >= reg_end)
        instruction("popl %%%s",current_result_register());
    --_regcnt;
    _reghead = _regcnt<0 ? reg_invalid : (_register)(_regcnt % reg_end);
}
void code_generator::save_registers()
{
    // save all but the current register
    int i;
    for (i = 0;i < _reghead;++i)
        instruction("pushl %%%s",register_to_string((_register)i,token_big));
    if (_regcnt >= reg_end)
        for (i += 1;i < reg_end;++i)
            instruction("pushl %%%s",register_to_string((_register)i,token_big));
}
void code_generator::restore_registers()
{
    // restore all but the current register
    if (_regcnt >= reg_end)
        for (int i = reg_end-1;i > _reghead;--i)
            instruction("popl %%%s",register_to_string((_register)i,token_big));
    for (int i = _reghead-1;i >= 0;--i)
        instruction("popl %%%s",register_to_string((_register)i,token_big));
}
int code_generator::get_return_label()
{
    if (_retlbl <= 0)
        _retlbl = get_unique_label();
    return _retlbl;
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
/*static*/ const char* code_generator::register_to_string(_register r,token_t t)
{
#ifdef RAMSEY_DEBUG
    if (r == reg_invalid)
        throw ramsey_exception("code_generator::register_to_string");
#endif
    static const char* LONG_REGISTERS[] = {"eax","ebx","ecx","edx"};
    static const char* WORD_REGISTERS[] = {"ax","bx","cx","dx"};
    static const char* BYTE_REGISTERS[] = {"al","bl","cl","dl"};
    if (t==token_in || t==token_big)
        return LONG_REGISTERS[r];
    else if (t == token_small)
        return WORD_REGISTERS[r];
    // type == token_boo
    return BYTE_REGISTERS[r];
}

// code generation implementation for AST node types

/*static*/ const char* ast_expression_node::load_operand(const ast_expression_node::operand& op,stable& symtable,code_generator& cgen,bool alloc)
{
    // loading an operand into a register is a very common operation performed by expression nodes
    if (alloc) // allocate a new register for the operand's evaluation
        cgen.allocate_result_register();
    op.node->generate_code(symtable,cgen);
    // return the register into which the operand was loaded
    return !cgen.expects_result() ? "eax" : cgen.current_result_register();
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
void ast_selection_statement_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    int lbltrue = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    // load condition into a register
    if (_condition->get_kind()==ast_expression_node::ast_primary_expression && static_cast<ast_primary_expression_node*>(_condition)->is_identifier()) {
        // if the condition is just an identifier, then it can be used directly
        const symbol* sym = symtable.getSymbol(static_cast<ast_primary_expression_node*>(_condition)->name());
        if (sym->get_type()==token_in || sym->get_type()==token_big)
            cgen.instruction("cmpl $0, %d(%%ebp)",sym->get_offset());
        else if (sym->get_type() == token_small)
            cgen.instruction("cmpw $0, %d(%%ebp)",sym->get_offset());
        else // token_boo
            cgen.instruction("cmpb $0, %d(%%ebp)",sym->get_offset());
    }
    else {
        const char* reg;
        cgen.allocate_result_register(); // allocate a register for the result of the expression
        reg = cgen.current_result_register(); // this should always be EAX
        _condition->generate_code(symtable,cgen);
        cgen.deallocate_result_register();
        // the register value is still good since this is an if-statement condition
        cgen.instruction("cmpl $0, %%%s",reg);
    }
    // jump to the true block if condition was non-zero
    cgen.instruction("jne lbl%d",lbltrue);
    // otherwise the control falls through to hit an elf or else block (if any)
    cgen.add_store_label(lbldone); // store done label so elf block can jump over other case blocks
    if (_elf != NULL)
        _elf->generate_code(symtable,cgen);
    cgen.remove_store_label();
    if (_else != NULL)
        _else->generate_code(symtable,cgen);
    cgen.instruction("jmp lbl%d",lbldone); // jump over true block to done label
    cgen.writeline("lbl%d:",lbltrue);
    if (_body == NULL)
        cgen.instruction("nop"); // empty statement body, issue nop
    else {
        // insert code for function body
        ast_statement_node* n = _body;
        do {
            n->generate_code(symtable,cgen);
            n = n->get_next();
        } while (n != NULL);
    }
    // define done label past true block
    cgen.writeline("lbl%d:",lbldone);
}
void ast_elf_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    int lbldone = cgen.get_store_label(); // get jump location from parent node
    int lblfalse = cgen.get_unique_label();
    // load condition into a register
    if (_condition->get_kind()==ast_expression_node::ast_primary_expression && static_cast<ast_primary_expression_node*>(_condition)->is_identifier()) {
        // if the condition is just an identifier, then it can be used directly
        const symbol* sym = symtable.getSymbol(static_cast<ast_primary_expression_node*>(_condition)->name());
        if (sym->get_type()==token_in || sym->get_type()==token_big)
            cgen.instruction("cmpl $0, %d(%%ebp)",sym->get_offset());
        else if (sym->get_type() == token_small)
            cgen.instruction("cmpw $0, %d(%%ebp)",sym->get_offset());
        else // token_boo
            cgen.instruction("cmpb $0, %d(%%ebp)",sym->get_offset());
    }
    else {
        const char* reg;
        cgen.allocate_result_register(); // allocate a register for the result of the expression
        reg = cgen.current_result_register(); // this should always be EAX
        _condition->generate_code(symtable,cgen);
        cgen.deallocate_result_register();
        // the register value is still good since this is an if-statement condition
        cgen.instruction("cmpl $0, %%%s",reg);
    }
    // jump to the false block if value was zero
    cgen.instruction("je lbl%d",lblfalse);
    ast_statement_node* n = _body;
    while (n != NULL) {
        n->generate_code(symtable,cgen);
        n = n->get_next();
    }
    cgen.instruction("jmp lbl%d",lbldone);
    // otherwise test another elf (if any) and let control fall through
    cgen.writeline("lbl%d:",lblfalse);
    if (_elf != NULL)
        _elf->generate_code(symtable,cgen);
}
void ast_iterative_statement_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    int lbltop = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    // add label for top of loop body
    cgen.writeline("lbl%d:",lbltop);
    // load condition into a register
    if (_condition->get_kind()==ast_expression_node::ast_primary_expression && static_cast<ast_primary_expression_node*>(_condition)->is_identifier()) {
        // if the condition is just an identifier, then it can be used directly
        const symbol* sym = symtable.getSymbol(static_cast<ast_primary_expression_node*>(_condition)->name());
        if (sym->get_type()==token_in || sym->get_type()==token_big)
            cgen.instruction("cmpl $0, %d(%%ebp)",sym->get_offset());
        else if (sym->get_type() == token_small)
            cgen.instruction("cmpw $0, %d(%%ebp)",sym->get_offset());
        else // token_boo
            cgen.instruction("cmpb $0, %d(%%ebp)",sym->get_offset());
    }
    else {
        const char* reg;
        cgen.allocate_result_register(); // allocate a register for the result of the expression
        reg = cgen.current_result_register(); // this should always be EAX
        _condition->generate_code(symtable,cgen);
        cgen.deallocate_result_register();
        // the register value is still good since this is a statement node
        cgen.instruction("cmpl $0, %%%s",reg);
    }
    // if the condition was zero (false), then jump to the done label
    cgen.instruction("je lbl%d",lbldone);
    // otherwise control falls through to execute the while loop body
    ast_statement_node* n = _body;
    cgen.add_store_label(lbldone); // this store label is used to break from the loop
    while (n != NULL) {
        n->generate_code(symtable,cgen);
        n = n->get_next();
    }
    cgen.remove_store_label();
    // jump back up to the top to reiterate the loop
    cgen.instruction("jmp lbl%d",lbltop);
    // add done label
    cgen.writeline("lbl%d:",lbldone);
}
void ast_jump_statement_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    if (_expr != NULL) { // _kind->type() == token_toss
        // load return value into EAX
        cgen.allocate_result_register(); // allocate a register for the result of the expression (this will be EAX)
        _expr->generate_code(symtable,cgen);
        cgen.deallocate_result_register();
        // the register value is still good since this is at the statement level; so jump to the function return label
        cgen.instruction("jmp lbl%d",cgen.get_return_label());
    }
    else // _kind->type() == token_smash
        // jump to loop end (iterative-statement parent set this on top of the store label stack)
        cgen.instruction("jmp lbl%d",cgen.get_store_label());
}
void ast_assignment_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    bool alloc = !cgen.expects_result();
    token_t type = get_type();
    const symbol* obj = symtable.getSymbol( static_cast<ast_primary_expression_node*>(_ops[0].node)->name() );
    // generate code for the right-hand expression
    if (alloc)
        cgen.allocate_result_register();
    _ops[1].node->generate_code(symtable,cgen);
    // assign the right-hand expression to the left hand identifier; semantic analysis guarentees lvalue; make
    // sure that zero bits are extended when assigning to a function argument
    int offset = obj->get_offset();
    if (type==token_in || type==token_big)
        cgen.instruction("movl %%%s, %d(%%ebp)",cgen.current_result_register(type),offset);
    else if (type == token_small)
        cgen.instruction("%s %%%s, %d(%%ebp)",offset>=0 ? "movswl" : "movw",cgen.current_result_register(type),offset);
    else // token_boo
        cgen.instruction("%s %%%s, %d(%%ebp)",offset>=0 ? "movsbl" : "movb",cgen.current_result_register(type),offset);
    if (alloc)
        cgen.deallocate_result_register();
    // this expression returns the value of its left-operand, so if a result is expected, assign it to the 
    // current result register
    if (!alloc)
        cgen.instruction("movl %d(%%ebp), %%%s",offset,cgen.current_result_register());
}
void ast_logical_or_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement logic-OR, we test to see if a term is non-zero; if so, the control assigns
       will jump to a block that assigns 1 to the result register; otherwise control falls through
       to test the next term; 0 is assigned in the default case */
    bool alloc = cgen.expects_result();
    int lbltrue = cgen.get_unique_label(), lblfalse = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    const char* reg = load_operand(_ops[0],symtable,cgen,alloc); // process the first term independently to effectively handle the else case
    // process the rest of terms; the grammar guarantees at least 2 (so at least 1 for the below loop)
    for (size_t i = 1;i < _ops.size();++i) {
        // do comparison for previous term; if non-zero then jump to true block
        cgen.instruction("cmpl $0, %%%s",reg);
        cgen.instruction("jne lbl%d",lbltrue);
        // process the next term in the sequence
        reg = load_operand(_ops[i],symtable,cgen); // use the same result register
    }
    // insert jump for false case when none of the terms are non-zero
    cgen.instruction("cmpl $0, %%%s",reg);
    cgen.instruction("je lbl%d",lblfalse);
    // define lbltrue
    cgen.writeline("lbl%d:",lbltrue);
    cgen.instruction("movl $1, %%%s",reg);
    cgen.instruction("jmp lbl%d",lbldone);
    // define lblfalse
    cgen.writeline("lbl%d:",lblfalse);
    cgen.instruction("movl $0, %%%s",reg);
    // define lbldone
    cgen.writeline("lbl%d:",lbldone);
    if (alloc)
        cgen.deallocate_result_register();
}
void ast_logical_and_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement logic-AND, we test each term to see if it is zero; if so then control
       jumps to a block that assigns 0 to the result register; otherwise control falls through
       to test each term until the success case is hit at the bottom */
    bool alloc = !cgen.expects_result();
    int lblfalse = cgen.get_unique_label(), lbltrue = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    const char* reg = load_operand(_ops[0],symtable,cgen,alloc); // process the first term independently to effectively handle the else case
    // process the rest of terms; the grammar guarantees at least 2 (so at least 1 for the below loop)
    for (size_t i = 1;i < _ops.size();++i) {
        // do comparison for previous term; if non-zero then jump to true block
        cgen.instruction("cmpl $0, %%%s",reg);
        cgen.instruction("je lbl%d",lblfalse);
        // process the next term in the sequence
        reg = load_operand(_ops[i],symtable,cgen); // use the same result register
    }
    // insert jump for false case when none of the terms are non-zero
    cgen.instruction("cmpl $0, %%%s",reg);
    cgen.instruction("jne lbl%d",lbltrue);
    // define lblfalse
    cgen.writeline("lbl%d:",lblfalse);
    cgen.instruction("movl $0, %%%s",reg);
    cgen.instruction("jmp lbl%d",lbldone);
    // define lbltrue
    cgen.writeline("lbl%d:",lbltrue);
    cgen.instruction("movl $1, %%%s",reg);
    // define lbldone
    cgen.writeline("lbl%d:",lbldone);
    if (alloc)
        cgen.deallocate_result_register();
}
void ast_equality_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement EQUALITY, we load up the result of the left and right operands and compare them; based
       on which equality operator is used, a jump instruction takes control to either a success or fail block */
    bool alloc = !cgen.expects_result();
    const char* regA = load_operand(_operands[0],symtable,cgen,alloc), *regB = load_operand(_operands[1],symtable,cgen,true);
    int lbltrue = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    // note: regA will be assign-to register if hasResult==true
    cgen.instruction("cmp %%%s, %%%s",regA,regB);
    cgen.deallocate_result_register();
    if (alloc)
        cgen.deallocate_result_register();
    cgen.instruction("%s lbl%d",_operator->type()==token_equal ? "je" : "jne",lbltrue);
    cgen.instruction("movl $0, %%%s",regA);
    cgen.instruction("jmp lbl%d",lbldone);
    cgen.writeline("lbl%d:",lbltrue);
    cgen.instruction("movl $1, %%%s",regA);
    cgen.writeline("lbl%d:",lbldone);
}
void ast_relational_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    /* to implement RELATIONAL, we load up the result of the left and right operands and compare them; based
       on which relational operator is used, a jump instruction takes control to either a success or fail block */
    bool alloc = !cgen.expects_result();
    const char* regA = load_operand(_operands[0],symtable,cgen,alloc), *regB = load_operand(_operands[1],symtable,cgen,true);
    int lbltrue = cgen.get_unique_label(), lbldone = cgen.get_unique_label();
    const char* jmp;
    // note: regA will be assign-to register if hasResult==true
    cgen.instruction("cmp %%%s, %%%s",regA,regB);
    cgen.deallocate_result_register();
    if (alloc)
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
    cgen.instruction("movl $0, %%%s",regA);
    cgen.instruction("jmp lbl%d",lbldone);
    cgen.writeline("lbl%d:",lbltrue);
    cgen.instruction("movl $1, %%%s",regA);
    cgen.writeline("lbl%d:",lbldone);
}
void ast_additive_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    // load up first operand; accumulate result in 'reg'
    bool alloc = !cgen.expects_result();
    const char* reg = load_operand(_operands[0],symtable,cgen,alloc);
    // process the rest of the operands in the expression (grammar guarantees at least 1 more)
    for (size_t i = 1,j = 0;i < _operands.size();++i,++j) {
        load_operand(_operands[i],symtable,cgen,true); // allocate register and grab next operand
        if (_operators[j]->type() == token_add)
            cgen.instruction("addl %%%s, %%%s",cgen.current_result_register(),reg);
        else // token_subtract
            cgen.instruction("subl %%%s, %%%s",cgen.current_result_register(),reg);
        cgen.deallocate_result_register();
    }
    if (alloc)
        cgen.deallocate_result_register();
}
void ast_multiplicative_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    // load up first operand; accumulate result in 'reg'
    bool alloc = !cgen.expects_result();
    const char* reg = load_operand(_operands[0],symtable,cgen,alloc);
    // process the rest of the operands in the expression (grammar guarantees at least 1 more)
    for (size_t i = 1,j = 0;i < _operands.size();++i,++j) {
        load_operand(_operands[i],symtable,cgen,true); // allocate register and grab next operand
        // do signed operations
        if (_operators[j]->type() == token_multiply)
            cgen.instruction("imull %%%s, %%%s",cgen.current_result_register(),reg);
        else { // token_multiply or token_mod
            // this is really nasty...
            bool saveEAX, saveEDX;
            saveEAX = strcmp(reg,"eax") != 0; // true if eax is not the result register; eax must be in use
            saveEDX = cgen.register_in_use(code_generator::reg_EDX); // is EDX holding an intermediate result?
            if (saveEAX) {
                cgen.instruction("pushl %%eax");
                cgen.instruction("movl %%%s, %%eax",reg);
            }
            if (saveEDX)
                cgen.instruction("pushl %%edx");
            cgen.instruction("cdq"); // sign-extend eax into edx
            if (cgen.current_result_register_flag() == code_generator::reg_EDX)
                cgen.instruction("idivl (%%esp)");
            else
                cgen.instruction("idivl %%%s",cgen.current_result_register());
            if (_operators[j]->type() == token_mod)
                cgen.instruction("movl %%edx, %%%s",reg); // move remainder into result register
            else if (saveEAX) // and token_divide
                cgen.instruction("movl %%eax, %%%s",reg); // move quotient into result register
            if (saveEDX)
                cgen.instruction("popl %%edx");
            if (saveEAX)
                cgen.instruction("popl %%eax");
        }
        cgen.deallocate_result_register();
    }
    if (alloc)
        cgen.deallocate_result_register();
}
void ast_prefix_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    // load up the single operand into a result register
    bool alloc = !cgen.expects_result();
    const char* reg = load_operand(_operand,symtable,cgen,alloc);
    if (_operator->type() == token_not) {
        // do comparison on the operand; if non-zero then set 0 to the
        // low byte register, 1 otherwise; then zero-extend the low-byte
        // register into the long (extended) register
        const char* regLow;
        regLow = cgen.expects_result() ? cgen.current_result_register(token_boo)/*get low-byte version*/ : "al";
        cgen.instruction("cmp $0, %%%s",reg);
        cgen.instruction("sete %%%s",regLow); // set 1 if equal, 0 otherwise
        cgen.instruction("movzbl %%%s, %%%s",regLow,reg);
    }
    else // token_subtract (meaning unary negation)
        cgen.instruction("negl %%%s",reg);
    if (alloc)
        cgen.deallocate_result_register();
}
void ast_postfix_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    bool alloc;
    ast_expression_node* n;
    const symbol* sym = symtable.getSymbol(static_cast<ast_primary_expression_node*>(_op.node)->name());
    stack<ast_expression_node*> params;
    int nargs;
    // insert all parameter expressions into the stack
    n = _expList;
    while (n != NULL) {
        params.push(n);
        n = n->get_next();
    }
    nargs = (int)params.size();
    // save any intermediate values on the stack
    cgen.save_registers();
    // generate code for each expression list, pushing the result value on the stack; the stack
    // of expression nodes guarantees correct argument ordering
    alloc = !cgen.expects_result();
    if (alloc)
        cgen.allocate_result_register();
    while ( !params.empty() ) {
        n = params.top();
        params.pop();
        n->generate_code(symtable,cgen);
        cgen.instruction("pushl %%%s",cgen.current_result_register());
    }
    if (alloc)
        cgen.deallocate_result_register();
    // call the function
#ifdef RAMSEY_WIN32
    cgen.instruction("call _%s",sym->get_name());
#else
    cgen.instruction("call %s",sym->get_name());
#endif
    // move function return value into result register (if they are not the same register)
    if (cgen.expects_result() && cgen.current_result_register_flag()!=code_generator::reg_EAX)
        cgen.instruction("movl %%eax, %%%s",cgen.current_result_register());
    // unload the stack
    if (nargs > 0)
        cgen.instruction("addl $%d, %%esp",nargs*4);
    // restore registers 
    cgen.restore_registers();
}
void ast_primary_expression_node::codegen_impl(stable& symtable,code_generator& cgen) const
{
    // optimization: if no result is expected, then the operation is useless and can be discarded
    if ( !cgen.expects_result() )
        return;
    // default behavior is to load the value into the result register; this function should
    // not be called on every primary expression node; most expressions use the values directly
    if (_tok->type() == token_id) {
        token_t type;
        const symbol* sym = symtable.getSymbol(_tok->source_string());
        type = sym->get_type();
        if (type==token_in || type==token_big)
            cgen.instruction("movl %d(%%ebp), %%%s",sym->get_offset(),cgen.current_result_register());
        else if (type == token_small)
            // sign-extend to long
            cgen.instruction("movswl %d(%%ebp), %%%s",sym->get_offset(),cgen.current_result_register());
        else // token_boo
            // sign-extend to long
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
