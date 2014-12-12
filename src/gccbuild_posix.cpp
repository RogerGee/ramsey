/* gccbuild_posix.cpp */
#include "gccbuild.h"
#include <streambuf>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;
using namespace ramsey;

// gccbuild_error
gccbuilder_error::gccbuilder_error(const char* format, ...)
{
    va_list args;
    va_start(args,format);
    _cstor(format,args);
    va_end(args);
}

// pipebuf
class pipebuf : public streambuf
{
public:
    pipebuf();
    ~pipebuf();

    int get_read_fd()
    { return _pfd[0]; }
    void close_write_fd();
    void close_read_fd();
private:
    static const ptrdiff_t BUFSIZE = 4096;

    virtual int_type overflow(int_type);
    virtual int sync();

    // disallow copying
    pipebuf(const pipebuf&);
    pipebuf& operator =(const pipebuf&);

    int _pfd[2];
    char _buffer[BUFSIZE+1];
};

pipebuf::pipebuf()
{
    if (pipe(_pfd) == -1)
        throw ramsey_exception("fail pipe()");
    setp(_buffer,_buffer+BUFSIZE);
}
pipebuf::~pipebuf()
{
    for (short i = 0;i < 2;++i)
        if (_pfd[i] != -1)
            close(_pfd[i]);
}
void pipebuf::close_write_fd()
{
    if (_pfd[1] != -1) {
        close(_pfd[1]);
        _pfd[1] = -1;
    }
}
void pipebuf::close_read_fd()
{
    if (_pfd[0] != -1) {
        close(_pfd[0]);
        _pfd[0] = -1;
    }
}
streambuf::int_type pipebuf::overflow(streambuf::int_type ch)
{
    if (ch == traits_type::eof())
        return traits_type::eof();
    // write all data in streambuf to pipe
    char* base = pbase(), *e = pptr();
    ptrdiff_t n = e - base;
    *e = ch; // guarenteed to be present at end of buffer
    if (write(_pfd[1],base,n+1) == -1)
        throw ramsey_exception("fail write()");
    pbump(-n);
    return ch;
}
int pipebuf::sync()
{
    // write all data in streambuf to pipe
    char* base = pbase(), *e = pptr();
    ptrdiff_t n = e - base;
    if (n > 0) {
        if (write(_pfd[1],base,n) == -1)
            throw ramsey_exception("fail write()");
        pbump(-n);
    }
    return 0;
}

// gccbuilder
gccbuilder::gccbuilder(int argc,const char* argv[])
    : _buf(new pipebuf), _stream(_buf)
{
    // read arguments; find exactly one .c file and 1 .ram file
    _cfile = NULL; _ramfile = NULL;
    for (int i = 0;i < argc;++i) {
        const char* ext;
        int n = strlen(argv[i]) - 1;
        while (n>=0 && argv[i][n]!='.')
            --n;
        if (n < 0)
            throw gccbuilder_error("bad argument '%s'",argv[i]);
        ext = argv[i] + n;
        if (strcmp(ext,".ram") == 0) {
            if (_ramfile != NULL)
                throw gccbuilder_error("too many .ram files");
            _ramfile = argv[i];
        }
        else if (strcmp(ext,".c") == 0) {
            if (_cfile != NULL)
                throw gccbuilder_error("too many .c files");
            _cfile = argv[i];
        } 
    }
    if (_cfile == NULL)
        throw gccbuilder_error("no .c file provided");
    if (_ramfile == NULL)
        throw gccbuilder_error("no .ram file provided");
    _flags = 0;
    _pi = new pid_t(-1);
}
gccbuilder::~gccbuilder()
{
    // flush any remaining data in the stream buffer and close the pipe
    _buf->pubsync();
    static_cast<pipebuf*>(_buf)->close_write_fd();
    delete _buf;
    // wait on the child process
    pid_t pidChild;
    pidChild = *reinterpret_cast<pid_t*>(_pi);
    delete reinterpret_cast<pid_t*>(_pi);
    while (pidChild != -1) {
        int stat;
        pid_t pid = wait(&stat);
        if (WIFEXITED(stat) || WIFSIGNALED(stat))
            pidChild = -1;
        else
            kill(pid,SIGKILL);
    }
}
void gccbuilder::execute()
{
    // spawn 'gcc'; it will invoke the assembler
    // and compile the .c driver program; eventually,
    // it will invoke the linker and build an executable
    pid_t gcc;
    pipebuf* pip = static_cast<pipebuf*>(_buf);
    gcc = fork();
    if (gcc == -1)
        throw ramsey_exception("fail fork()");
    if (gcc == 0) {
        // 'as' child process
        // prepare command-line parameters
        string prog = _ramfile; // name the executable after the .ram source (minus extension)
        size_t n = prog.length();
        while (prog[n] != '.')
            --n;
        prog.resize(n);
        prog = "-o"+prog;
        const char* args[25] = {
            "gcc", "-m32", "-O0", // 32-bit code, no optimizations
            prog.c_str(), // name output to 'prog'
            "-xassembler", "-", // process assembly input from stdin
            "-xc", _cfile, // compile .c file (this is the driver program)
            NULL
        };
        // redirect stdin to read from pipebuf; assembler code will be written to this pipe
        if (dup2(pip->get_read_fd(),STDIN_FILENO) != STDIN_FILENO)
            throw ramsey_exception("fail dup2()");
        pip->close_read_fd(); pip->close_write_fd();
        // execute the child process
        if (execvp("gcc",(char*const*)args) == -1)
            throw gccbuilder_error("cannot execute 'gcc'; is the software installed in the system PATH?");
        // control not in this program
    }
    // close pipe read end and save the pid for the child process
    pip->close_read_fd();
    *reinterpret_cast<pid_t*>(_pi) = gcc;
}
