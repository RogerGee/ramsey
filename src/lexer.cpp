/* lexer.cpp */
#include "lexer.h"
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <cstring>
#include <cctype>
using namespace std;
using namespace ramsey;

// exception types
lexer_error::lexer_error(const char* format, ...)
{
    static const size_t BUFMAX = 4096;
    va_list args;
    char buffer[BUFMAX];
    va_start(args,format);
    vsnprintf(buffer,BUFMAX,format,args);
    va_end(args);
    _cstor(buffer);
}
lexer_exception::lexer_exception(const char* format, ...)
{
    static const size_t BUFMAX = 4096;
    va_list args;
    char buffer[BUFMAX];
    va_start(args,format);
    vsnprintf(buffer,BUFMAX,format,args);
    va_end(args);
    _cstor(buffer);
}

// token
token::token(token_t kind)
    : _tkind(kind), _tsource(NULL)
{
}
token::token(token_t kind,const char* source)
    : _tkind(kind)
{
    _alloc(source,strlen(source));
}
token::token(token_t kind,const char* source,int length)
    : _tkind(kind)
{
    if (length < 0)
        throw lexer_exception("negative length specified for token source string");
    _alloc(source,length);
}
token::token(const token& obj)
    : _tkind(obj._tkind)
{
    if (obj._tsource != NULL)
        _alloc(obj._tsource,strlen(obj._tsource));
    else
        _tsource = NULL;
}
token::~token()
{
    if (_tsource != NULL)
        delete[] _tsource;
}
token& token::operator =(const token& obj)
{
    if (this != &obj) {
        if (_tsource != NULL)
            delete[] _tsource;
        _tkind = obj._tkind;
        if (obj._tsource == NULL)
            _tsource = NULL;
        else
            _alloc(obj._tsource,strlen(obj._tsource));
    }
    return *this;
}
#ifdef RAMSEY_DEBUG
const char* token::to_string_kind() const
{
    switch (_tkind) {
    case token_invalid:
        return "token_invalid";
    case token_id:
        return "token_id";
    case token_number:
        return "token_number";
    case token_number_hex:
        return "token_number_hex";
    case token_string:
        return "token_string";
    case token_bool_true:
        return "token_bool_true";
    case token_bool_false:
        return "token_bool_false";
    case token_add:
        return "token_add";
    case token_subtract:
        return "token_subtract";
    case token_multiply:
        return "token_multiply";
    case token_divide:
        return "token_divide";
    case token_assign:
        return "token_assign";
    case token_oparen:
        return "token_oparen";
    case token_cparen:
        return "token_cparen";
    case token_equal:
        return "token_equal";
    case token_nequal:
        return "token_nequal";
    case token_less:
        return "token_less";
    case token_greater:
        return "token_greater";
    case token_le:
        return "token_le";
    case token_ge:
        return "token_ge";
    /*case token_lscript: // removed from language
        return "token_lscript";
    case token_rscript:
        return "token_rscript";*/
    case token_comma:
        return "token_comma";
    case token_in:
        return "token_in";
    case token_boo:
        return "token_boo";
    case token_if:
        return "token_if";
    case token_elf:
        return "token_elf";
    case token_endif:
        return "token_endif";
    case token_while:
        return "token_while";
    case token_smash:
        return "token_smash";
    case token_endwhile:
        return "token_endwhile";
    case token_fun:
        return "token_fun";
    case token_as:
        return "token_as";
    case token_endfun:
        return "token_endfun";
    case token_toss:
        return "token_toss";
    /*case token_take: // removed from language
        return "token_take";
    case token_give:
        return "token_give";*/
    case token_mod:
        return "token_mod";
    case token_or:
        return "token_or";
    case token_and:
        return "token_and";
    case token_not:
        return "token_not";
    case token_eol:
        return "token_eol";
    }
    return "token_bad";
}
string token::to_string() const
{
    string result = to_string_kind();
    if (_tsource != NULL) {
        result += " { ";
        result += _tsource;
        result += " } ";
    }
    return result;
}
#endif
void token::_alloc(const char* source,int length)
{
    int i;
    // allocate a buffer to store the source string
    _tsource = new char[length+1]; // (account for the null character)
    // copy the source string to the buffer
    for (i = 0;i < length;++i)
        _tsource[i] = source[i];
    _tsource[i] = 0;
}

// lexer
lexer::lexer(const char* file)
{
    vector<ptoken> ptoks;
    // open input file and decompose it into preprocessing tokens
    _preprocess(file,ptoks);
    // convert preprocessing tokens into lexical tokens
    _convert(ptoks);
    // set iterator at beginning
    _iter = _stream.begin();
}
lexer& lexer::operator ++()
{
    ++_iter;
    return *this;
}
lexer lexer::operator ++(int)
{
    lexer tmp = *this;
    _iter++;
    return tmp;
}
/*static*/ void lexer::_preprocess(const char* file,vector<ptoken>& ptoks)
{
    /* preprocess the input stream; this involves partial token
       decomposition; the tokens produced here will be converted
       into lexical tokens; note: a single ptoken may be converted
       into more than one lexical token (e.g. through maximal munch) */
    FILE* input;
    // open input file
    input = fopen(file,"r");
    if (input == NULL)
        throw lexer_error("can't read input file: %s",strerror(errno));
    while (true) {
        ptoken tok;
        int ch = fgetc(input);
        // end of file
        if (ch == EOF)
            break;
        if (isspace(ch)) {
            // handle whitespace
            if (ch == '\n')
                // only count '\n' for logical lines
                tok.kind = ptoken_eol;
            else
                // ignore all other whitespace
                continue;
        }
        else if (ch == '#') {
            // '#' denotes a comment; strip it out
            while (true) {
                ch = fgetc(input);
                if (ch == '\n') {
                    ungetc(ch,input);
                    break;
                }
            }
            continue;
        }
        else if (isdigit(ch)) {
            // number token
            tok.kind = ptoken_number;
            if (ch == '0') {
                // check for hexadecimal literal
                int chnext = fgetc(input);
                if (chnext=='x' || chnext=='X')
                    tok.kind = ptoken_number_hex;
                else
                    ungetc(chnext,input);
            }
            do {
                tok.payload.push_back(ch);
                ch = fgetc(input);
            } while (isdigit(ch) || (tok.kind==ptoken_number_hex && ishexletter(ch)));
            ungetc(ch,input);
        }
        else if (isalpha(ch) || ch=='_') {
            // identifier token
            tok.kind = ptoken_identifier;
            do {
                tok.payload.push_back(ch);
                ch = fgetc(input);
            } while (isalpha(ch) || ch=='_');
            ungetc(ch,input);
        }
        else if (ch == '\"') {
            // string token
            tok.kind = ptoken_string;
            while (true) {
                ch = fgetc(input);
                // check for end of string
                if (ch == '\"')
                    break;
                if (ch=='\r' || ch=='\n')
                    throw lexer_error("found newline in string literal");
                // handle escape characters
                if (ch == '\\') {
                    // translate the escape character to its actual value
                    //  valid escape characters include: \\ \" \n \r \t \0
                    ch = fgetc(input);
                    if (ch == 'n')
                        ch = '\n';
                    else if (ch == 'r')
                        ch = '\r';
                    else if (ch == 't')
                        ch = '\t';
                    else if (ch == '0')
                        ch = 0;
                    else if (ch!='\\' && ch!='\"') // anything else (excluding as-is escape characters)
                        // should this be a warning?
                        throw lexer_error("escape character '\\%c' is not supported",ch);
                }
                tok.payload.push_back(ch);
            }
        }
        else if (isoppunc(ch)) {
            // operator-punctuator token
            tok.kind = ptoken_puncop;
            do {
                tok.payload.push_back(ch);
                ch = fgetc(input);
            } while (isoppunc(ch));
            ungetc(ch,input);
        }
        else
            throw lexer_error("stray '%c' character in program text",ch);
        // add the preprocessing token
        ptoks.push_back(tok);
    }
    fclose(input);
}
void lexer::_convert(vector<ptoken>& ptoks)
{
    // convert preprocessor tokens into lexical tokens
    for (auto iter = ptoks.begin();iter != ptoks.end();++iter) {
        switch (iter->kind) {
            // an identifier could be a keyword of some sort (or boolean-literal)
        case ptoken_identifier:
        {
            token_t t;
            bool isKeyword = true;
            // if the identifier is a keyword then flag it
            if (iter->payload == "in")
                t = token_in;
            else if (iter->payload == "boo")
                t = token_boo;
            else if (iter->payload == "if")
                t = token_if;
            else if (iter->payload == "elf")
                t = token_elf;
            else if (iter->payload == "else")
                t = token_else;
            else if (iter->payload == "endif")
                t = token_endif;
            else if (iter->payload == "while")
                t = token_while;
            else if (iter->payload == "smash")
                t = token_smash;
            else if (iter->payload == "endwhile")
                t = token_endwhile;
            else if (iter->payload == "fun")
                t = token_fun;
            else if (iter->payload == "as")
                t = token_as;
            else if (iter->payload == "endfun")
                t = token_endfun;
            else if (iter->payload == "toss")
                t = token_toss;
            /*else if (iter->payload == "take") // removed from language
                t = token_take;
            else if (iter->payload == "give")
                t = token_give;*/
            else if (iter->payload == "mod")
                t = token_mod;
            else if (iter->payload == "or")
                t = token_or;
            else if (iter->payload == "and")
                t = token_and;
            else if (iter->payload == "not")
                t = token_not;
            else if (iter->payload == "true")
                t = token_bool_true;
            else if (iter->payload == "false")
                t = token_bool_false;
            else {
                isKeyword = false;
                t = token_id;
            }

            if (isKeyword)
                _stream.emplace_back(t);
            else
                // save payload for non-keyword identifier
                _stream.emplace_back(t,iter->payload.c_str());
            break;
        }
        // simply convert number, number-hex, string, and eol tokens to 
        // their lexical counterparts
        case ptoken_number:
            _stream.emplace_back(token_number,iter->payload.c_str());
            break;
        case ptoken_number_hex:
            _stream.emplace_back(token_number_hex,iter->payload.c_str());
            break;
        case ptoken_string:
            _stream.emplace_back(token_string,iter->payload.c_str());
            break;
        case ptoken_eol:
            _stream.emplace_back(token_eol);
            break;
            // a punctuator-operator at the preprocessing level is a sequence of potential
            // operator punctuators; here the sequence must be separated into valid lexical
            // tokens of the longest valid token kind (maximal munch)
        case ptoken_puncop:
        {
            char swp = 0;
            char* str = &iter->payload[0];
            size_t sz = iter->payload.length();
            size_t i = sz;
            while (i > 0) {
                while (true) {
                    token_t t = token_invalid;
                    if (strcmp(str,"+") == 0)
                        t = token_add;
                    else if (strcmp(str,"-") == 0)
                        t = token_subtract;
                    else if (strcmp(str,"*") == 0)
                        t = token_multiply;
                    else if (strcmp(str,"/") == 0)
                        t = token_divide;
                    else if (strcmp(str,"<") == 0)
                        t = token_less;
                    else if (strcmp(str,">") == 0)
                        t = token_greater;
                    else if (strcmp(str,"<=") == 0)
                        t = token_le;
                    else if (strcmp(str,">=") == 0)
                        t = token_ge;
                    else if (strcmp(str,"=") == 0)
                        t = token_equal;
                    else if (strcmp(str,"!=") == 0)
                        t = token_nequal;
                    else if (strcmp(str,"<-") == 0)
                        t = token_assign;
                    else if (strcmp(str,"(") == 0)
                        t = token_oparen;
                    else if (strcmp(str,")") == 0)
                        t = token_cparen;
                    /*else if (strcmp(str,"[") == 0) // removed from language
                        t = token_lscript;
                    else if (strcmp(str,"]") == 0)
                        t = token_rscript;*/
                    else if (strcmp(str,",") == 0)
                        t = token_comma;
                    else if (i == 0) {
                        str[i] = swp;
                        throw lexer_exception("couldn't process puncop preprocessing token: '%s'",iter->payload.c_str());
                    }
                    str[i] = swp;
                    if (t != token_invalid) {
                        _stream.emplace_back(t);
                        break;
                    }
                    --i;
                    swp = str[i];
                    str[i] = 0;
                }
                str += i;
                sz -= i;
                i = sz;
                swp = str[i];
            }
            break;
        }
        default:
            throw lexer_exception("bad preprocessing token in ptoken stream");
        }
    }
}
/*static*/ inline bool lexer::ishexletter(int ch)
{
    return (ch>='A' && ch<='F') || (ch>='a' && ch<='f');
}
/*static*/ inline bool lexer::isoppunc(int ch)
{
    static const string acceptChars("+-*/<>=!,()");
    return acceptChars.find(char(ch)) != string::npos;
}
