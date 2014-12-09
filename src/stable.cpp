#include "stable.h"
#include <cstring>
using namespace std;
using namespace ramsey;

// symbol

symbol::symbol()
    : _argtypes(NULL), _offset(0)
{
}
symbol::~symbol()
{
    if (_argtypes != NULL)
        delete[] _argtypes;
}
// symbol::match_parameters is defined in 'semantics.cpp'
int symbol::get_offset() const
{
#ifdef RAMSEY_DEBUG
    if (get_kind_impl() != skind_variable)
        throw ramsey_exception();
#endif
    return _offset;
}
void symbol::set_offset(int offset)
{
#ifdef RAMSEY_DEBUG
    if (get_kind_impl() != skind_variable)
        throw ramsey_exception();
#endif
    _offset = offset;
}

bool ramsey::operator ==(const symbol& a,const symbol& b)
{
    return strcmp(a.get_name(),b.get_name()) == 0;
}

// stable

stable::stable()
    : func(NULL)
{
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
    auto r = table.back().insert( make_pair<string,const symbol*>(symb->get_name(),(const symbol*)symb) );
    return r.second;
}

const symbol* stable::getSymbol(const char* id) const
{
    // go through the scopes backwards, since the inner-scope shadows
    // any other scopes previously declared
    for (auto it = table.rbegin(); it != table.rend(); it++)
        if (it->count(id) > 0)
            return it->at(id);
    return NULL;
}

void stable::enterFunction(const symbol* symb)
{
#ifdef RAMSEY_DEBUG
    if (func != NULL)
        throw ramsey_exception();
#endif
    func = symb;
}

void stable::exitFunction()
{
    func = NULL;
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
