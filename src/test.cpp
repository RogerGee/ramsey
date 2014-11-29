/* test.cpp - this module will server as the entry point to a program
   that tests some feature of the Ramsey compiler; try to build with
   RAMSEY_DEBUG macro defined (otherwise you will get compile errors) */
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include <iostream>
using namespace std;
using namespace ramsey;

int main(int argc,const char* argv[])
{
    if (argc <= 1) {
        cerr << "usage: " << argv[0] << " file\n";
        return 1;
    }

    // attempt to compile the file, print out intermediate results
    try {
        parser parse(argv[1]);
        const lexer& lex = parse.get_lexer();
        const ast_node* ast = parse.get_ast();

        lex.output(cout);
        cout << "\nParsed successfully\n\n[Abstract Syntax Tree]\n";
        ast->output(cout);
    }
    catch (lexer_error ex) {
        cerr << argv[0] << ": scan error: " << ex.what() << endl;
        return 1;
    }
    catch (parser_error ex) {
        cerr << argv[0] << ": syntax error: " << ex.what() << endl;
        return 1;
    }
    catch (semantic_error ex) {
        cerr << argv[0] << ": semantic error: " << ex.what() << endl;
        return 1;
    }
}
