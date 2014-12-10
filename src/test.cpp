/* test.cpp - this module will server as the entry point to a program
   that tests the compilation stages of the Ramsey compiler; try to
   build with the RAMSEY_DEBUG macro defined (otherwise you will get
   compile errors) */
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

    // attempt to compile the file, print out intermediate results
    try {
        // parser will open the file, lex it and generate an abstract syntax tree
        parser parse(argv[1]);
        const lexer& lex = parse.get_lexer();
        const ast_node* ast = parse.get_ast();
        stable symtable;
        code_generator codegen(cout);

        // display intermediate results
        cout << "[Lexical Tokens]\n";
        lex.output(cout);
        cout << "\n\nParsed successfully\n\n[Abstract Syntax Tree]\n";
        ast->output(cout); cout << '\n';
        symtable.addScope(); // add global scope
        ast->check_semantics(symtable);
        symtable.remScope();
        cout << "Passed semantic checks\n\n[Code Generation: Intel x86]\n";
        symtable.addScope();
        ast->generate_code(symtable,codegen);
        symtable.remScope();
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
