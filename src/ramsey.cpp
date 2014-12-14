/* ramsey.cpp - entry point implementation file for ramsey compiler */
#include "parser.h" // get ramsey compiler utilities
#include "gccbuild.h" // get GCC invoking utilities
#include <iostream>
using namespace std;
using namespace ramsey;

int main(int argc,const char* argv[])
{
    if (argc <= 1) {
        cerr << argv[0] << ": no input files\n";
        return 1;
    }

    try {
        gccbuilder gccBuilder(argc-1,argv+1);

        parser theParser(gccBuilder.ramfile());
        const ast_node* theAst = theParser.get_ast();
        if (theAst != NULL) {
            // check semantics on the abstract syntax tree
            stable theSymbolTable;
            theSymbolTable.addScope();
            theAst->check_semantics(theSymbolTable);
            theSymbolTable.remScope();

            // execute the gcc process
            gccBuilder.execute();

            // generate code from the abstract syntax tree
            code_generator theCodeGenerator(gccBuilder.get_code_stream());
            theSymbolTable.addScope();
            theAst->generate_code(theSymbolTable,theCodeGenerator);
            theSymbolTable.remScope();
        }
    } catch (gccbuilder_error& err) {
        cerr << argv[0] << ": error: " << err.what() << endl;
        return 1;
    } catch (lexer_error& err) {
        cerr << argv[0] << ": syntax error: " << err.what() << endl;
        return 1;
    } catch (parser_error& err) {
        cerr << argv[0] << ": syntax error: " << err.what() << endl;
        return 1;
    } catch (semantic_error& err) {
        cerr << argv[0] << ": semantic error: " << err.what() << endl;
    }
}
