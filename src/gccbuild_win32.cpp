/* gccbuild_win32.cpp */
#include "gccbuild.h"
#include <streambuf>
#include <cstring>
#include <windows.h>
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

class pipebuf : public streambuf
{
	public:
		pipebuf();
		~pipebuf();
		
		HANDLE get_read()
		{ return io[0]; }
		
		void close_write();
		void close_read();
	private:
		static const ptrdiff_t BUFSIZE = 4096;
	
		virtual int_type overflow(int_type);
		virtual int_type sync();
		
		// disallow copying
		pipebuf(const pipebuf&);
		pipebuf& operator =(const pipebuf&);
		
		HANDLE io[2];
		char _buffer[BUFSIZE+1];
};

pipebuf::pipebuf()
{
	if(!CreatePipe(&io[0], &io[1], NULL, 0))
		throw ramsey_exception("CreatePipe() failure");
	// set only the read end as inheritable
	if (!SetHandleInformation(io[0],HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT))
		throw ramsey_exception("SetHandleInformation() failure");
	setp(_buffer,_buffer+BUFSIZE);
}
pipebuf::~pipebuf()
{
	for (short i = 0;i < 2;i++)
		if (io[i] != INVALID_HANDLE_VALUE)
			CloseHandle(io[i]);
}
void pipebuf::close_write()
{
	if (io[1] != INVALID_HANDLE_VALUE)
	{
		CloseHandle(io[1]);
		io[1] = INVALID_HANDLE_VALUE;
	}
}
void pipebuf::close_read()
{
	if (io[0] != INVALID_HANDLE_VALUE)
	{
		CloseHandle(io[0]);
		io[0] = INVALID_HANDLE_VALUE;
	}
}
streambuf::int_type pipebuf::overflow(streambuf::int_type ch)
{
	if (ch == traits_type::eof())
        return traits_type::eof();
    // write all data in streambuf to pipe
    char* base = pbase(), *e = pptr();
    ptrdiff_t n = e - base;
   	DWORD doofus;
    *e = ch; // guarenteed to be present at end of buffer
    if (!WriteFile(io[1], base, n+1, &doofus, NULL))
        throw ramsey_exception("WriteFile() failure");
    pbump(-n);
    return ch;
}
int pipebuf::sync()
{
    // write all data in streambuf to pipe
    char* base = pbase(), *e = pptr();
    ptrdiff_t n = e - base;
    if (n > 0) {
		DWORD doofus;
    	if (!WriteFile(io[1], base, n, &doofus, NULL))
        	throw ramsey_exception("WriteFile() failure");
    	pbump(-n);
    }
    return 0;
}

struct proc{
	PROCESS_INFORMATION procinf;
	STARTUPINFO startinf;
};

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
    //_pi = new pid_t(-1);
	proc* p = new proc;
	ZeroMemory(&p->procinf,    sizeof(p->procinf));
	ZeroMemory(&p->startinf, sizeof(p->startinf));
	p->startinf.cb         = sizeof(p->startinf);
	p->startinf.hStdInput = static_cast<pipebuf*>(_buf)->get_read();
	p->startinf.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	p->startinf.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	p->startinf.dwFlags    = STARTF_USESTDHANDLES;
	_pi = p;
}
gccbuilder::~gccbuilder()
{
	// flush any remaining data in the stream buffer and close the pipe
    _buf->pubsync();
    static_cast<pipebuf*>(_buf)->close_write();
    delete _buf;
	// wait on the child processes
	proc p = *reinterpret_cast<proc*>(_pi);
	delete reinterpret_cast<proc*>(_pi);
	WaitForSingleObject(p.procinf.hProcess, INFINITE);
	CloseHandle(p.procinf.hProcess);
	CloseHandle(p.procinf.hThread);
}
void gccbuilder::execute()
{
	proc* p = reinterpret_cast<proc*>(_pi);
	string command = "gcc -m32 -O0 ";
	string prog = _ramfile;
	size_t n = prog.length();
    while (prog[n] != '.')
        --n;
    prog.resize(n);
	command += "-o " + prog + " -xassembler - -xc " + _cfile;
	if (!CreateProcess(NULL, &command[0], NULL, NULL, true, 0, NULL, NULL, 
		&p->startinf, &p->procinf))
		throw gccbuilder_error("CreateProcess() failure; is GCC installed?");
	static_cast<pipebuf*>(_buf)->close_read();
}
