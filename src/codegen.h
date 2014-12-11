/* codegen.h */
#ifndef CODEGEN_H
#define CODEGEN_H
#include <cstdarg>
#include <stdio.h>
#include <sstream>
#include <queue>
#include <stack>
#include "lexer.h"

namespace ramsey
{
    class code_generator
    {
    public:
        enum _register
        { // just use the general registers for results
            reg_invalid = -1,
            reg_EAX,
            reg_EBX,
            reg_ECX,
            reg_EDX,
            reg_end
        };

        code_generator(std::ostream& output);

        // handle function scheduling
        void begin_function(const char* name); // begin new stack frame following C calling convention
        void end_function(); // end stack frame; writes assembly code to output stream

        // handle generic assembly text processing
        void instruction(const char* format, ...); // write generic instruction to "function body"
        void instruction_before(const char* format, ...); // write generic instruction to area before "function body"
        void writeline(const char* format, ...); // write assembly code line with no special formatting to "function body"

        // handle memory offsets for variables and arguments
        int next_variable_offset(token_t type);
        int next_argument_offset();

        // handle result register allocation
        bool expects_result() const
        { return _regcnt >= 0; }
        void allocate_result_register();
        const char* current_result_register(token_t type = token_big) const
        { return register_to_string(_reghead,type); }
        _register current_result_register_flag() const
        { return _reghead; }
        bool register_in_use(_register reg) const
        { return _regcnt >= reg; }
        void deallocate_result_register();
        void save_registers(); // given config, push registers on stack
        void restore_registers(); // given config, pop registers from stack

        // handle unique label allocation
        int get_unique_label()
        { return _lbl++; }
        int get_return_label();
        void add_store_label(int lbl)
        { _storlbls.push(lbl); }
        int get_store_label()
        { return _storlbls.top(); }
        void remove_store_label()
        { _storlbls.pop(); }
    private:
        std::ostream& _output;
        std::stringstream _before, _body;
        int _alloc; // function stack allocation amount
        int _narg; // function argument counter
        std::queue<int> _allocations[3]; // for the stack allocator
        _register _reghead; // current available register
        int _regcnt; // number of outstanding result registers
        int _lbl, _retlbl; // current available local label, return label
        std::stack<int> _storlbls; // stack of stored local labels

        static void instruction_impl(std::ostream&,const char*);
        static void instruction_impl(std::ostream&,const char*,va_list);
        static const char* register_to_string(_register,token_t);
    };
}

#endif
