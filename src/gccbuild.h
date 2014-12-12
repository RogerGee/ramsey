/* gccbuild.h */
#ifndef GCCBUILD_H
#define GCCBUILD_H
#include <streambuf>

namespace ramsey
{
    // invoke GCC tools to build binary executable file
    // given a command-line and compiled ramsey source (as
    // a stream of ASM code)
    class gccbuilder
    {
    public:
        gccbuilder(int argc,const char* argv);
        ~gccbuilder(); // wait for the process to build

        ostream& get_code_stream();
    private:
        enum gccbuilder_flags
        {
            gccbuilder_object,
            gccbuilder_asm = 2
        };

        std::ostream _stream;
        const char* _ramfile;
        const char* _cfile;
        short _flags;
    };
}

#endif
