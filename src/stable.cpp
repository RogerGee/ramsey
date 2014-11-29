#include "stable.h"
#include <cstring>
using namespace std;
using namespace ramsey;

bool ramsey::operator ==(const symbol& a,const symbol& b)
{
    return strcmp(a.get_name(),b.get_name()) == 0;
}

void stable::addScope()
{
    table.emplace_back();
}

void stable::remScope()
{
    table.pop_back();
}

bool stable::add(const symbol* symb)
{
    auto r = table.back().insert( make_pair<const char*,const symbol*>(symb->get_name(),(const symbol*)symb) );
    return r.second;
}

const symbol* stable::getSymbol(const char* id) const
{
    for (auto it = table.rbegin(); it != table.rend(); it++)
    {
        auto it2 = it->find(id);
        if (it2 != it->end())
            return it2->second;
    }
    return NULL;
}

/*const symbol* stable::find(std::string name) const
{
    for (auto it = table.rbegin(); it != table.rend(); it++)
    {
        auto it2 = it->find(name);
        if (it2 != it->end())
            return &*it2;
    }
    return NULL;
}*/
