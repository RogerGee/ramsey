#include "stable.h"
using namespace ramsey;

void stable::addScope()
{
    table.emplace_back();
}

void stable::remScope()
{
    table.pop_back();
}

bool stable::add(std::string name, stype type)
{
    if (table.back().find(name) != table.back().end())
        return false;
    symbol s;
    s.name = name;
    s.type = type;
    table.back().insert(s);
    return true;
}

bool stable::hasSymbol(std::string name)
{
    if (!find(name))
        return true;
    return false;
}

stype stable::typeOf(std::string name)
{
    if (!find(name))
        return stype_invalid;
    return find(name)->type;
}

const symbol* stable::find(std::string name)
{
    for (auto it = table.rbegin(); it != table.rend(); it++)
    {
        auto it2 = it->find(name);
        if (it2 != it->end())
            return &*it2;
    }
    return NULL;
}

