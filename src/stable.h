#ifndef STABLE_H
#define STABLE_H

#include <unordered_set>
#include <deque>
#include <string>

namespace ramsey
{
    enum stype {stype_invalid = -1, stype_in, stype_boo};
    struct symbol
    {
        symbol() {}
        symbol(std::string n) : name(n) {}
        std::string name;
        stype type;
    };
    bool operator==(symbol a, symbol b)
    {
        return a.name == b.name;
    }
}
namespace std{template<>
struct hash<ramsey::symbol>{
    size_t operator()(const ramsey::symbol &a ) const
    {return hash<std::string>()(a.name);}
};}
namespace ramsey
{
    class stable
    {
    public:
        void addScope();
        void remScope();
        bool add(std::string name, stype type);
        bool hasSymbol(std::string name);
        stype typeOf(std::string name);
    private:
        std::deque<std::unordered_set<symbol> > table;
        const symbol* find(std::string name);
    };
}

#endif
