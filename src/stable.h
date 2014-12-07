#ifndef STABLE_H
#define STABLE_H
#include <unordered_map>
#include <deque>
#include <string>
#include "lexer.h" // gets exception and ramsey-error.h

namespace ramsey
{
    // abstract base class for symbols; this is mostly an interface with
    // some helpers for semantic analysis
    class symbol
    {
    public:
        symbol();
        ~symbol();

        enum skind {
            skind_function,
            skind_variable
        };

        enum match_parameters_result
        {
            match_okay,
            match_too_few,
            match_too_many,
            match_bad_types
        };

        // basic symbol interface
        const char* get_name() const
        { return get_name_impl(); }
        token_t get_type() const
        { return get_type_impl(); }
        skind get_kind() const
        { return get_kind_impl(); }

        // semantic analysis
        match_parameters_result match_parameters(const token_t* kinds,int cnt) const;
    private:
        mutable token_t* _argtypes; // cache parameter list (allocated on heap)

        // virtual interface
        virtual const char* get_name_impl() const = 0;
        virtual token_t get_type_impl() const = 0;
        virtual skind get_kind_impl() const = 0;
        virtual token_t* get_argtypes_impl() const { throw false; }
    };

    bool operator==(const symbol&, const symbol&);

    class stable
    {
    public:
        stable();

        void addScope();
        void remScope();
        bool add(const symbol* symb);
        const symbol* getSymbol(const char* id) const; // returns NULL if symbol not found

        void enterFunction(const symbol* symb);
        const symbol* getFunction() const // return the function symbol whose scope overlaps the current scope
        { return func; }
        void exitFunction();
    private:
        std::deque<std::unordered_map<std::string,const symbol*> > table;
        //const symbol* find(std::string name) const;

        const symbol* func;
    };
}

#endif
