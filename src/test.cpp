/* test.cpp - this module will server as the entry point to a program
   that tests some feature of the Ramsey compiler; try to build with
   RAMSEY_DEBUG macro defined */
#include "lexer.h"
#include "parser.h"
#include <iostream>
using namespace std;
using namespace ramsey;

int main(int argc,const char* argv[])
{
    if (argc <= 1) {
        cerr << "usage: " << argv[0] << " file\n";
        return 1;
    }

    try {
        lexer lex(argv[1]);

        while (!lex.endtok()) {
            cout << lex.curtok().to_string() << '\n';
            ++lex;
        }
    }
    catch (lexer_error ex) {
        cerr << argv[0] << ": error: " << ex.what() << endl;
        return 1;
    }
    try {
        parser parse(argv[1]);
        
        cout << "parsed successfully\n";
    }
    catch (parser_error ex) {
        cerr << argv[0] << ": error: " << ex.what() << endl;
        return 1;
    }
}
