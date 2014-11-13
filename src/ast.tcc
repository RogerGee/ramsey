/* ast.tcc - ast.h out-of-line implementation */

// ast_linked_node
template<typename T>
ramsey::ast_linked_node<T>::ast_linked_node()
    : _nxt(NULL), _prv(NULL)
{
}
template<typename T>
T* ramsey::ast_linked_node<T>::get_next()
{ return _nxt; }
template<typename T>
const T* ramsey::ast_linked_node<T>::get_next() const
{ return _nxt; }
template<typename T>
T* ramsey::ast_linked_node<T>::get_prev()
{ return _prv; }
template<typename T>
const T* ramsey::ast_linked_node<T>::get_prev() const
{ return _prv; }
template<typename T>
bool ramsey::ast_linked_node<T>::beg() const
{ return _prv == NULL; }
template<typename T>
bool ramsey::ast_linked_node<T>::end() const
{ return _nxt == NULL; }
template<typename T>
void ramsey::ast_linked_node<T>::append(T* node)
{
    _nxt = node;
    // the static_cast checks to make sure that 'T' is
    // a derivation of 'ast_linked_node'
    static_cast<ast_linked_node*>(node)->_prv = this;
}
