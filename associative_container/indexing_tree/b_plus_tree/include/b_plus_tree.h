#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>

#ifndef SYS_PROG_B_PLUS_TREE_H
#define SYS_PROG_B_PLUS_TREE_H

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BP_tree final : private compare //EBCO
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration

    struct bptree_node_base
    {
        bool _is_terminate;

        bptree_node_base() noexcept;
        virtual ~bptree_node_base() =default;
    };

    struct bptree_node_term : public bptree_node_base
    {
        bptree_node_term* _next;

        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _data;
        bptree_node_term() noexcept;
    };

    struct bptree_node_middle : public bptree_node_base
    {
        boost::container::static_vector<tkey, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<bptree_node_base*, maximum_keys_in_node + 2> _pointers;
        bptree_node_middle() noexcept;
    };

    pp_allocator<value_type> _allocator;
    bptree_node_base* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

public:

    // region constructors declaration

    explicit BP_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit BP_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BP_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    BP_tree(const BP_tree& other);

    BP_tree(BP_tree&& other) noexcept;

    BP_tree& operator=(const BP_tree& other);

    BP_tree& operator=(BP_tree&& other) noexcept;

    ~BP_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class bptree_iterator;
    class bptree_const_iterator;

    class bptree_iterator final
    {
        bptree_node_term* _node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_iterator;

        friend class BP_tree;
        friend class bptree_const_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_iterator(bptree_node_term* node = nullptr, size_t index = 0);

    };

    class bptree_const_iterator final
    {
        const bptree_node_term* _node;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_const_iterator;

        friend class BP_tree;
        friend class bptree_iterator;

        bptree_const_iterator(const bptree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_const_iterator(const bptree_node_term* node = nullptr, size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

    bptree_iterator begin();
    bptree_iterator end();

    bptree_const_iterator begin() const;
    bptree_const_iterator end() const;

    bptree_const_iterator cbegin() const;
    bptree_const_iterator cend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    bptree_iterator find(const tkey& key);
    bptree_const_iterator find(const tkey& key) const;

    bptree_iterator lower_bound(const tkey& key);
    bptree_const_iterator lower_bound(const tkey& key) const;

    bptree_iterator upper_bound(const tkey& key);
    bptree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<bptree_iterator, bool> insert(const tree_data_type& data);
    std::pair<bptree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<bptree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    bptree_iterator insert_or_assign(const tree_data_type& data);
    bptree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    bptree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    bptree_iterator erase(bptree_iterator pos);
    bptree_iterator erase(bptree_const_iterator pos);

    bptree_iterator erase(bptree_iterator beg, bptree_iterator en);
    bptree_iterator erase(bptree_const_iterator beg, bptree_const_iterator en);


    bptree_iterator erase(const tkey& key);

    // endregion modifiers declaration

    // region helpers declaration
    size_t key_index(bptree_node_middle* node, const tkey& key) const;
    size_t key_index(bptree_node_term* node, const tkey& key, bool strict) const;

    void merge_nodes(bptree_node_middle* parent, size_t i);
    void erase_node(bptree_node_middle* node, const tkey& k);

    bptree_iterator bound(const tkey& key, bool strict = false);
    // endregion helpers declaration
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
BP_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BP_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BP_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_pairs(const BP_tree::tree_data_type &lhs,
                                                     const BP_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_base::bptree_node_base() noexcept: _is_terminate(false) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_term::bptree_node_term() noexcept: _next(nullptr)
{
    this->_is_terminate = true;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_middle::bptree_node_middle() noexcept = default;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BP_tree<tkey, tvalue, compare, t>::value_type> BP_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept
{
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::reference BP_tree<tkey, tvalue, compare, t>::
bptree_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::pointer BP_tree<tkey, tvalue, compare, t>::bptree_iterator
::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self & BP_tree<tkey, tvalue, compare, t>::bptree_iterator::
operator++()
{
    if (!_node) return *this;

    if (_index + 1 < _node->_data.size()) {
        ++_index;
        return *this;
    }

    _node = _node->_next;
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self BP_tree<tkey, tvalue, compare, t>::bptree_iterator::
operator++(int)
{
    self tmp = *this;
    ++*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator==(const self &other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator!=(const self &other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::current_node_keys_count() const noexcept
{
    return _node ? _node->_data.size() : 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::bptree_iterator(bptree_node_term *node, size_t index):
_index(index), _node(node) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_iterator &it) noexcept:
_index(it._index), _node(it._node) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::reference BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::pointer BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self & BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator++()
{
    if (!_node) return *this;

    if (_index + 1 < _node->_data.size()) {
        ++_index;
        return *this;
    }

    _node = _node->_next;
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator++(int)
{
    self tmp = *this;
    ++*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator==(const self &other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator!=(const self &other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::current_node_keys_count() const noexcept
{
    return _node ? _node->_data.size() : 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_node_term *node, size_t index):
_index(index), _node(node) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BP_tree<tkey, tvalue, compare, t>::at(const tkey &key)
{
    auto it = find(key);
    if (it == end()) throw std::out_of_range("key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue & BP_tree<tkey, tvalue, compare, t>::at(const tkey &key) const
{
    auto it = find(key);
    if (it == end()) throw std::out_of_range("key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue & BP_tree<tkey, tvalue, compare, t>::operator[](const tkey &key)
{
    auto it = find(key);
    if (it == end()) it = insert({key, tvalue{}}).first;
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue & BP_tree<tkey, tvalue, compare, t>::operator[](tkey &&key)
{
    auto it = find(key);
    if (it == end()) it = insert({std::move(key), tvalue{}}).first;
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool>
BP_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return insert(tree_data_type(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const compare& cmp, pp_allocator<value_type> alloc):
compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(pp_allocator<value_type> alloc, const compare& cmp): BP_tree(cmp, alloc) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BP_tree<tkey, tvalue, compare, t>::BP_tree(iterator begin, iterator end, const compare& cmp, pp_allocator<value_type> alloc):
BP_tree(cmp, alloc)
{
    for (; begin != end; ++begin) insert(*begin);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp, pp_allocator<value_type> alloc):
BP_tree(data.begin(), data.end(), cmp, alloc) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const BP_tree& other): compare(static_cast<const compare&>(other)),
_allocator(other._allocator), _root(nullptr), _size(0)
{
    try {
        for (auto it = other.cbegin(); it != other.cend(); ++it) insert({it->first, it->second});
    } catch (...) {
        clear();
        throw std::logic_error("Err while copy construct!");
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(BP_tree&& other) noexcept: compare(std::move(static_cast<compare&>(other))),
_allocator(std::move(other._allocator)), _root(std::exchange(other._root, nullptr)),
_size(std::exchange(other._size, 0)) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(const BP_tree& other)
{
    if (this == &other) return *this;

    BP_tree tmp(other);
    *this = std::move(tmp);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(BP_tree&& other) noexcept
{
    if (this == &other) return *this;

    BP_tree tmp(std::move(other));

    std::swap(static_cast<compare&>(*this), static_cast<compare&>(tmp));
    std::swap(_allocator, tmp._allocator);
    std::swap(_root, tmp._root);
    std::swap(_size, tmp._size);

    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::~BP_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::begin()
{
    if (!_root) return end();

    auto current = _root;
    while (!current->_is_terminate) {
        current = static_cast<bptree_node_middle*>(current)->_pointers.front();
    }

    auto leaf = static_cast<bptree_node_term*>(current);

    if (leaf->_data.empty()) return end();
    return bptree_iterator(leaf, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::end()
{
    return bptree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::begin() const
{
    return bptree_const_iterator(const_cast<BP_tree*>(this)->begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::end() const
{
    return bptree_const_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cbegin() const
{
    return bptree_const_iterator(const_cast<BP_tree*>(this)->begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cend() const
{
    return bptree_const_iterator(const_cast<BP_tree*>(this)->end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    auto it = bound(key);
    return it != end() && !compare_keys(key, it->first) && !compare_keys(it->first, key) ? it : end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    return bptree_const_iterator(const_cast<BP_tree*>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    return bound(key, false);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    return bptree_const_iterator(const_cast<BP_tree*>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    return bound(key, true);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    return bptree_const_iterator(const_cast<BP_tree*>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    if (!_root) return;

    std::stack<bptree_node_base*> nodes;
    nodes.push(_root);

    while (!nodes.empty()) {
        auto current = nodes.top();
        nodes.pop();


if (!current->_is_terminate)
            for (auto child : static_cast<bptree_node_middle*>(current)->_pointers)
                if (child) nodes.push(child);

        if (current->_is_terminate)
            _allocator.template delete_object<bptree_node_term>(static_cast<bptree_node_term*>(current));
        else
            _allocator.template delete_object<bptree_node_middle>(static_cast<bptree_node_middle*>(current));
    }

    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    if (!_root) {
        auto leaf = _allocator.template new_object<bptree_node_term>();
        leaf->_data.push_back(std::move(data));
        _root = leaf;
        ++_size;
        return {bptree_iterator(leaf, 0), true};
    }

    std::vector<std::pair<bptree_node_middle*, size_t>> path;

    auto current = _root;

    while (!current->_is_terminate) {
        auto middle = static_cast<bptree_node_middle*>(current);

        const size_t index = key_index(middle, data.first);

        path.emplace_back(middle, index);
        current = middle->_pointers[index];
    }

    auto leaf = static_cast<bptree_node_term*>(current);

    size_t insert_index = key_index(leaf, data.first, false);

    if (insert_index < leaf->_data.size()) {
        if (!compare_keys(data.first, leaf->_data[insert_index].first) && !compare_keys(leaf->_data[insert_index].first, data.first)) {
            return {bptree_iterator(leaf, insert_index), false};
        }
    }

    leaf->_data.insert(leaf->_data.begin() + insert_index, std::move(data));
    ++_size;

    auto result_leaf = leaf;
    size_t result_index = insert_index;

    if (leaf->_data.size() <= maximum_keys_in_node) {
        return {bptree_iterator(result_leaf, result_index), true};
    }

    auto new_leaf = _allocator.template new_object<bptree_node_term>();

    const size_t mid = leaf->_data.size() / 2;

    for (size_t i = mid; i < leaf->_data.size(); ++i) {
        new_leaf->_data.push_back(std::move(leaf->_data[i]));
    }

    leaf->_data.erase(leaf->_data.begin() + mid, leaf->_data.end());

    new_leaf->_next = leaf->_next;
    leaf->_next = new_leaf;

    if (insert_index >= mid) {
        result_leaf = new_leaf;
        result_index = insert_index - mid;
    }

    auto separator = new_leaf->_data.front().first;
    bptree_node_base* right_child = new_leaf;

    while (!path.empty()) {
        auto [parent, child_index] = path.back();
        path.pop_back();

        parent->_keys.insert(parent->_keys.begin() + child_index, std::move(separator));
        parent->_pointers.insert(parent->_pointers.begin() + child_index + 1, right_child);

        if (parent->_keys.size() <= maximum_keys_in_node) return {bptree_iterator(result_leaf, result_index), true};

        auto new_middle = _allocator.template new_object<bptree_node_middle>();
        const size_t middle_index = parent->_keys.size() / 2;
        separator = std::move(parent->_keys[middle_index]);

        for (size_t i = middle_index + 1; i < parent->_keys.size(); ++i) {
            new_middle->_keys.push_back(std::move(parent->_keys[i]));
        }
        for (size_t i = middle_index + 1; i < parent->_pointers.size(); ++i) {
            new_middle->_pointers.push_back(parent->_pointers[i]);
        }

        parent->_keys.erase(parent->_keys.begin() + middle_index, parent->_keys.end());
        parent->_pointers.erase(parent->_pointers.begin() + middle_index + 1, parent->_pointers.end());

        right_child = new_middle;
    }

    auto new_root = _allocator.template new_object<bptree_node_middle>();

    new_root->_keys.push_back(std::move(separator));
    new_root->_pointers.push_back(_root);
    new_root->_pointers.push_back(right_child);

    _root = new_root;

    return {bptree_iterator(result_leaf, result_index), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    return insert(tree_data_type(std::forward<Args>(args)...));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    return emplace_or_assign(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    return emplace_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);

    auto it = find(data.first);
    return it != end() ? (it->second = std::move(data.second), it) : insert(std::move(data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator pos)
{
    if (pos == end()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator pos)
{
    if (pos == cend()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator beg, bptree_iterator en)
{
    if (beg == en) return beg;

    std::optional<tkey> end_key = en != end() ? std::optional(en->first) : std::nullopt;

    while (beg != end() && (!end_key || compare_keys(beg->first, *end_key))) beg = erase(beg);

    return end_key ? lower_bound(*end_key) : end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator beg, bptree_const_iterator en)
{
    if (beg == en || beg == cend()) return end();

    return erase(lower_bound(beg->first), en == cend() ? end() : lower_bound(en->first));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (!_root) return end();

    auto it = find(key);
    if (it == end()) return end();

    auto next = std::next(it);
    std::optional<tkey> next_key = next != end() ? std::optional(next->first) : std::nullopt;

    if (_root->_is_terminate) {
        auto leaf = static_cast<bptree_node_term*>(_root);
        const size_t j = key_index(leaf, key, false);
        leaf->_data.erase(leaf->_data.begin() + j);
        --_size;
        if (leaf->_data.empty()) {
            _allocator.template delete_object<bptree_node_term>(leaf);
            _root = nullptr;
        }
    } else {
        erase_node(static_cast<bptree_node_middle*>(_root), key);

        if (static_cast<bptree_node_middle*>(_root)->_keys.empty()) {
            auto old_root = static_cast<bptree_node_middle*>(_root);
            _root = old_root->_pointers.empty() ? nullptr : old_root->_pointers.front();
            _allocator.template delete_object<bptree_node_middle>(old_root);
        }
    }

    return next_key ? lower_bound(*next_key) : end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::key_index(
    bptree_node_middle* node,
    const tkey& key
) const
{
    size_t lo = 0;
    size_t hi = node->_keys.size();

    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const auto& mid_key = node->_keys[mid];

        if (!compare_keys(key, mid_key)) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    return lo;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::key_index(
    bptree_node_term* node,
    const tkey& key,
    bool strict
) const
{
    size_t lo = 0;
    size_t hi = node->_data.size();

    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const auto& mid_key = node->_data[mid].first;

        if (strict ? !compare_keys(key, mid_key) : compare_keys(mid_key, key)) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    return lo;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::merge_nodes(bptree_node_middle* parent, size_t i)
{
    auto left_base = parent->_pointers[i];
    auto right_base = parent->_pointers[i + 1];

    if (left_base->_is_terminate) {
        auto left = static_cast<bptree_node_term*>(left_base);
        auto right = static_cast<bptree_node_term*>(right_base);

        for (auto& item : right->_data) left->_data.push_back(std::move(item));
        left->_next = right->_next;

        _allocator.template delete_object<bptree_node_term>(right);
    } else {
        auto left = static_cast<bptree_node_middle*>(left_base);
        auto right = static_cast<bptree_node_middle*>(right_base);

        left->_keys.push_back(std::move(parent->_keys[i]));
        for (auto& key : right->_keys) left->_keys.push_back(std::move(key));
        for (auto ptr : right->_pointers) left->_pointers.push_back(ptr);

        _allocator.template delete_object<bptree_node_middle>(right);
    }

    parent->_keys.erase(parent->_keys.begin() + i);
    parent->_pointers.erase(parent->_pointers.begin() + i + 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::erase_node(bptree_node_middle* node, const tkey& k)
{
    size_t i = key_index(node, k);
    auto child = node->_pointers[i];

    if (child->_is_terminate) {
        auto leaf = static_cast<bptree_node_term*>(child);

        if (leaf->_data.size() == minimum_keys_in_node) {
            if (i > 0 && static_cast<bptree_node_term*>(node->_pointers[i - 1])->_data.size() >= t) {
                auto left = static_cast<bptree_node_term*>(node->_pointers[i - 1]);
                leaf->_data.insert(leaf->_data.begin(), std::move(left->_data.back()));
                left->_data.pop_back();
                node->_keys[i - 1] = leaf->_data.front().first;
            } else if (i + 1 < node->_pointers.size() && static_cast<bptree_node_term*>(node->_pointers[i + 1])->_data.size() >= t) {
                auto right = static_cast<bptree_node_term*>(node->_pointers[i + 1]);
                leaf->_data.push_back(std::move(right->_data.front()));
                right->_data.erase(right->_data.begin());
                node->_keys[i] = right->_data.front().first;
            } else {
                if (i + 1 >= node->_pointers.size()) --i;
                merge_nodes(node, i);
            }
        }

        leaf = static_cast<bptree_node_term*>(node->_pointers[i]);
        const size_t j = key_index(leaf, k, false);
        if (j < leaf->_data.size() &&
            !compare_keys(leaf->_data[j].first, k) &&
            !compare_keys(k, leaf->_data[j].first)) {
            leaf->_data.erase(leaf->_data.begin() + j);
            --_size;
        }
        return;
    }

    auto child_middle = static_cast<bptree_node_middle*>(child);

    if (child_middle->_keys.size() == minimum_keys_in_node) {
        if (i > 0 && static_cast<bptree_node_middle*>(node->_pointers[i - 1])->_keys.size() >= t) {
            auto left = static_cast<bptree_node_middle*>(node->_pointers[i - 1]);

            child_middle->_keys.insert(child_middle->_keys.begin(), std::move(node->_keys[i - 1]));
            node->_keys[i - 1] = std::move(left->_keys.back());
            left->_keys.pop_back();

            if (!left->_pointers.empty()) {
                child_middle->_pointers.insert(child_middle->_pointers.begin(), left->_pointers.back());
                left->_pointers.pop_back();
            }
        } else if (i + 1 < node->_pointers.size() && static_cast<bptree_node_middle*>(node->_pointers[i + 1])->_keys.size() >= t) {
            auto right = static_cast<bptree_node_middle*>(node->_pointers[i + 1]);

            child_middle->_keys.push_back(std::move(node->_keys[i]));
            node->_keys[i] = std::move(right->_keys.front());
            right->_keys.erase(right->_keys.begin());

            if (!right->_pointers.empty()) {
                child_middle->_pointers.push_back(right->_pointers.front());
                right->_pointers.erase(right->_pointers.begin());
            }
        } else {
            if (i + 1 >= node->_pointers.size()) --i;
            merge_nodes(node, i);
            i = key_index(node, k);
            if (i >= node->_pointers.size()) i = node->_pointers.size() - 1;
        }
    }

    erase_node(static_cast<bptree_node_middle*>(node->_pointers[i]), k);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::bound(const tkey& key, bool strict)
{
    if (!_root) return end();

    auto current = _root;

    while (!current->_is_terminate) {
        auto middle = static_cast<bptree_node_middle*>(current);

        const size_t index = key_index(middle, key);
        current = middle->_pointers[index];
    }

    auto leaf = static_cast<bptree_node_term*>(current);

    const size_t index = key_index(leaf, key, strict);

    if (index < leaf->_data.size()) {
        return bptree_iterator(leaf, index);
    }

    if (leaf->_next) {
        return bptree_iterator(leaf->_next, 0);
    }

    return end();
}

#endif