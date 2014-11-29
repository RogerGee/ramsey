#ifndef STABLE_H
#define STABLE_H
#include <unordered_map>
#include <deque>
#include <string>
#include "lexer.h"

namespace ramsey
{
    // abstract base class for symbols; this only contains an interface
    class symbol
    {
    public:
        enum skind {
            skind_function,
            skind_variable
        };

        const char* get_name() const
        { return get_name_impl(); }
        token_t get_type() const
        { return get_type_impl(); }
        skind get_kind() const
        { return get_kind_impl(); }
    private:
        // virtual interface
        virtual const char* get_name_impl() const = 0;
        virtual token_t get_type_impl() const = 0;
        virtual skind get_kind_impl() const = 0;
    };

    bool operator==(const symbol&, const symbol&);
}

/*namespace std
{
    template<>
    struct hash<const ramsey::symbol*>{
        size_t operator()(const ramsey::symbol* a) const
        {return hash<std::string>()(a->get_name());}
    };
}*/

namespace ramsey
{
    class stable
    {
    public:
        void addScope();
        void remScope();
        bool add(const symbol* symb);
        const symbol* getSymbol(const char* id) const; // returns NULL if symbol not found
    private:
        std::deque<std::unordered_map<const char*,const symbol*> > table;
        //const symbol* find(std::string name) const;
    };
}

#endif
