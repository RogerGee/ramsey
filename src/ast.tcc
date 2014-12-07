/* ast.tcc - ast.h out-of-line implementation */

// ast_linked_node
template<typename T>
ramsey::ast_linked_node<T>::ast_linked_node()
    : _nxt(NULL), _prv(NULL)
{
}
template<typename T>
ramsey::ast_linked_node<T>::~ast_linked_node()
{
    if (_nxt != NULL)
        delete _nxt;
}
template<typename T>
T* ramsey::ast_linked_node<T>::get_next()
{ return static_cast<T*>(_nxt); }
template<typename T>
const T* ramsey::ast_linked_node<T>::get_next() const
{ return static_cast<const T*>(_nxt); }
template<typename T>
T* ramsey::ast_linked_node<T>::get_prev()
{ return static_cast<T*>(_prv); }
template<typename T>
const T* ramsey::ast_linked_node<T>::get_prev() const
{ return static_cast<const T*>(_prv); }
template<typename T>
bool ramsey::ast_linked_node<T>::beg() const
{ return _prv == NULL; }
template<typename T>
bool ramsey::ast_linked_node<T>::end() const
{ return _nxt == NULL; }
template<typename T>
void ramsey::ast_linked_node<T>::append(T* node)
{
    // append here means prepend to the front of the list
    _prv = node;
    // the static_cast checks to make sure that 'T' is
    // a derivation of 'ast_linked_node'
    static_cast<ast_linked_node*>(node)->_nxt = this;
}
#ifdef RAMSEY_DEBUG
template<typename T>
void ramsey::ast_linked_node<T>::output_at_level(std::ostream& stream,int level) const
{
    stream.width(4);
    stream << get_lineno() << ' ';
    for (int i = 0;i < level;++i)
        stream.put('\t');
    output_impl(stream,level+1);
    // perform the operation on the next node in the list (if any)
    if (_nxt != NULL)
        _nxt->output_at_level(stream,level);
}
#endif
