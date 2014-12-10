/* codegen.h */
#ifndef CODEGEN_H
#define CODEGEN_H
#include <cstdarg>
#include <sstream>
#include <queue>
#include "lexer.h"

namespace ramsey
{
    class code_generator
    {
    public:
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
        const char* current_result_register(token_t type = token_big) const;
        void deallocate_result_register();

        // handle unique label allocation
        int get_unique_label()
        { return _lbl++; }
    private:
        enum _register
        { // just use the general registers
            reg_invalid = -1,
            reg_EAX,
            reg_EBX,
            reg_ECX,
            reg_EDX,
            reg_end
        };

        std::ostream& _output;
        std::stringstream _before, _body;
        int _alloc; // function stack allocation amount
        int _narg; // function argument counter
        std::queue<int> _allocations[3]; // for the stack allocator
        _register _reghead; // current available register
        int _regcnt; // number of outstanding result registers
        int _lbl; // current available local label

        static void instruction_impl(std::ostream&,const char*);
        static void instruction_impl(std::ostream&,const char*,va_list);
    };
}

#endif
