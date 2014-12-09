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

        void begin_function(const char* name); // begin new stack frame following C calling convention
        void end_function(); // end stack frame; writes assembly code to output stream

        void instruction(const char* format, ...); // write generic instruction to "function body"
        void instruction_before(const char* format, ...); // write generic instruction to area before "function body"

        int next_variable_offset(token_t type);
        int next_argument_offset();
    private:
        std::stringstream _before;
        std::stringstream _body;
        std::ostream& _output;
        int _alloc; // function stack allocation amount
        int _narg; // function argument counter
        std::queue<int> _allocations[3]; // for the stack allocator

        static void instruction_impl(std::ostream&,const char*);
        static void instruction_impl(std::ostream&,const char*,va_list);
    };
}

#endif
