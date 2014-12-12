/* gccbuild.h */
#ifndef GCCBUILD_H
#define GCCBUILD_H
#include <ostream>
#include "ramsey-error.h"

namespace ramsey
{
    class gccbuilder_error : public compiler_error_generic // errors reported to user
    {
    public:
        gccbuilder_error(const char* format, ...);
        virtual ~gccbuilder_error() throw() {}
    };

    // invoke GCC tools to build binary executable file
    // given a command-line and compiled ramsey source (as
    // a stream of ASM code)
    class gccbuilder
    {
    public:
        gccbuilder(int argc,const char* argv[]);
        ~gccbuilder(); // wait for the process to build

        const char* ramfile() const
        { return _ramfile; }
        const char* cfile() const
        { return _cfile; }

        void execute();
        std::ostream& get_code_stream()
        { return _stream; }
    private:
        enum gccbuilder_flags
        {
            gccbuilder_object,
            gccbuilder_asm = 2
        };

        std::streambuf* _buf;
        std::ostream _stream;
        const char* _ramfile;
        const char* _cfile;
        short _flags;
        void* _pi;
    };
}

#endif
