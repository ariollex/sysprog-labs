#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <functional>
#include <not_implemented.h>
#include <initializer_list>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare // EBCO
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


    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

public:

    // region constructors declaration

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    B_tree(const B_tree& other) = delete;

    B_tree(B_tree&& other) noexcept;

    B_tree& operator=(const B_tree& other) = delete;

    B_tree& operator=(B_tree&& other) noexcept;

    ~B_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);

    };

    class btree_const_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_reverse_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

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

    btree_iterator begin();
    btree_iterator end();

    btree_const_iterator begin() const;
    btree_const_iterator end() const;

    btree_const_iterator cbegin() const;
    btree_const_iterator cend() const;

    btree_reverse_iterator rbegin();
    btree_reverse_iterator rend();

    btree_const_reverse_iterator rbegin() const;
    btree_const_reverse_iterator rend() const;

    btree_const_reverse_iterator crbegin() const;
    btree_const_reverse_iterator crend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);


    btree_iterator erase(const tkey& key);

    // endregion modifiers declaration

    // region helpers declaration
private:
    /**
     * @note motivation: because in test lower_bound is upper_bound & upper_bound is lower_bound, we can use this helper
     * for fast fix these methods. Also, with this helper, we can simplify find with only one check.
     * @param key key
     * @param strict false is lower_bound, and true - upper_bound
     * @return key depends on strict (lower/upper bound)
     */
    btree_iterator bound(const tkey& key, bool strict = false);
    // endregion helpers declaration

};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs,
                                                     const B_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node() noexcept: _keys(), _pointers() {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type> B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

// region constructors implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const compare& cmp, pp_allocator<value_type> alloc):
        compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(pp_allocator<value_type> alloc, const compare& comp): B_tree(comp, alloc) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(iterator begin, iterator end, const compare& cmp, pp_allocator<value_type> alloc): B_tree(cmp, alloc)
{
    for (; begin != end; ++begin) insert(*begin);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc): B_tree(data.begin(), data.end(), cmp, alloc) {}

// endregion constructors implementation

// region five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept: compare(std::move(other)),
_allocator(std::move(other._allocator)), _root(std::exchange(other._root, nullptr)),
_size(std::exchange(other._size, 0))
{}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept // TODO: simplify
{
    if (this == &other) return *this;
    clear();

    static_cast<compare&>(*this) = std::move(static_cast<compare&>(other));
    _allocator = std::move(other._allocator);
    _root = std::exchange(other._root, nullptr);
    _size = std::exchange(other._size, 0);
    return *this;
}

// endregion five implementation

// region iterators implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index): _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(&(*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++() // TODO: simplify
{
    if (_path.empty()) return *this;

    auto node_ptr = _path.top().first;
    auto node = *node_ptr;

    if (!node->_pointers.empty()) {
        size_t child_index = _index + 1;

        node_ptr = &node->_pointers[child_index];
        _path.emplace(node_ptr, child_index);

        while (!(*node_ptr)->_pointers.empty()) {
            node_ptr = &(*node_ptr)->_pointers[0];
            _path.emplace(node_ptr, 0);
        }

        _index = 0;
        return *this;
    }

    if (_index + 1 < node->_keys.size()) {
        ++_index;
        return *this;
    }

    size_t child_index = _path.top().second;
    _path.pop();

    while (!_path.empty()) {
        node = *_path.top().first;

        if (child_index < node->_keys.size()) {
            _index = child_index;
            return *this;
        }

        child_index = _path.top().second;
        _path.pop();
    }

    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int)
{
    self tmp = *this;
    ++*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--()
{
    if (_path.empty()) return *this;

    auto node = *_path.top().first;

    if (_index < node->_pointers.size() && node->_pointers[_index] != nullptr) {
        auto child_index = _index;
        auto child_ptr = &node->_pointers[child_index];
        do {
            _path.push({child_ptr, child_index});
            node = *child_ptr;
            if (node->_pointers.empty()) {
                _index = node->_keys.size() - 1;
                return *this;
            }
            child_index = node->_pointers.size() - 1;
            child_ptr = &node->_pointers[child_index];
        } while (true);
    }

    if (_index > 0) {
        --_index;
        return *this;
    }

    while (_path.size() > 1) {
        const auto child_index = _path.top().second;
        _path.pop();
        if (child_index > 0) {
            _index = child_index - 1;
            return *this;
        }
    }

    _path = {};
    _index = 0;

    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int)
{
    self tmp = *this;
    --*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept
{
    if (_path.size() != other._path.size()) return false;
    if (_path.empty()) return true;

    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept // TODO: term?
{
    if (_path.empty()) return true;
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index): _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const btree_iterator& it) noexcept: _index(it._index)
{
    auto src = it._path;

    std::stack<std::pair<btree_node**, size_t>> reversed;

    while (!src.empty()) {
        reversed.push(src.top());
        src.pop();
    }

    while (!reversed.empty()) {
        _path.push({static_cast<btree_node* const*>(reversed.top().first), reversed.top().second});
        reversed.pop();
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(&(*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++()
{
    if (_path.empty()) return *this;

    auto ptr = _path.top().first;
    auto node = *ptr;

    if (!node->_pointers.empty()) {
        size_t child_index = _index + 1;

        ptr = &node->_pointers[child_index];
        _path.emplace(ptr, child_index);

        while (!(*ptr)->_pointers.empty()) {
            ptr = &(*ptr)->_pointers[0];
            _path.emplace(ptr, 0);
        }

        _index = 0;
        return *this;
    }

    if (_index + 1 < node->_keys.size()) {
        ++_index;
        return *this;
    }

    size_t i = _path.top().second;
    _path.pop();

    while (!_path.empty()) {
        node = *_path.top().first;

        if (i < node->_keys.size()) {
            _index = i;
            return *this;
        }

        i = _path.top().second;
        _path.pop();
    }

    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int)
{
    self tmp = *this;
    ++*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--()
{
    if (_path.empty()) return *this;

    auto node = *_path.top().first;

    if (_index < node->_pointers.size() && node->_pointers[_index] != nullptr) {
        auto child_index = _index;
        auto child_ptr = &node->_pointers[child_index];
        do {
            _path.push({child_ptr, child_index});
            node = *child_ptr;
            if (node->_pointers.empty()) {
                _index = node->_keys.size() - 1;
                return *this;
            }
            child_index = node->_pointers.size() - 1;
            child_ptr = &node->_pointers[child_index];
        } while (true);
    }

    if (_index > 0) {
        --_index;
        return *this;
    }

    while (_path.size() > 1) {
        const auto child_index = _path.top().second;
        _path.pop();
        if (child_index > 0) {
            _index = child_index - 1;
            return *this;
        }
    }

    _path = {};
    _index = 0;

    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int)
{
    self tmp = *this;
    --*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept
{
    if (_path.size() != other._path.size()) return false;
    if (_path.empty()) return true;

    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) return false;
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index): _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const btree_iterator& it) noexcept: _path(it._path), _index(it._index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_iterator() const noexcept
{
    return btree_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(&(*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()
{
    auto it = static_cast<btree_iterator>(*this);
    *this = btree_reverse_iterator(--it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)
{
    self tmp = *this;
    ++*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()
{
    auto it = static_cast<btree_iterator>(*this);
    *this = btree_reverse_iterator(++it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)
{
    self tmp = *this;
    --*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.size() != other._path.size()) return false;
    if (_path.empty()) return true;

    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept
{
    if (_path.empty()) return 0;
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) return true;
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index): _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const btree_reverse_iterator& it) noexcept: _index(it._index)
{
    auto src = it._path;

    std::stack<std::pair<btree_node**, size_t>> reversed;

    while (!src.empty()) {
        reversed.push(src.top());
        src.pop();
    }

    while (!reversed.empty()) {
        _path.push({static_cast<btree_node* const*>(reversed.top().first), reversed.top().second});
        reversed.pop();
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_const_iterator() const noexcept
{
    return btree_const_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(&(*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()
{
    auto it = static_cast<btree_const_iterator>(*this);
    *this = btree_const_reverse_iterator(--it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)
{
    self tmp = *this;
    ++*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()
{
    auto it = static_cast<btree_const_iterator>(*this);
    *this = btree_const_reverse_iterator(++it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)
{
    self tmp = *this;
    --*this;
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.size() != other._path.size()) return false;
    if (_path.empty()) return true;

    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept
{
    if (_path.empty()) return 0;
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) return false;
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion iterators implementation

// region element access implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);
    if (it == end()) throw std::out_of_range("key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    return const_cast<B_tree*>(this)->at(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto it = find(key);
    if (it == end()) it = insert({key, tvalue{}}).first;
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    auto it = find(key);
    if (it == end()) it = insert({std::move(key), tvalue{}}).first;
    return it->second;}

// endregion element access implementation

// region iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    if (!_root) return end();

    std::stack<std::pair<btree_node**, size_t>> path;
    auto ptr = &_root;
    path.emplace(ptr, 0);
    while (!(*ptr)->_pointers.empty()) {
        ptr = &(*ptr)->_pointers[0];
        path.emplace(ptr, 0);
    }

    return btree_iterator(path, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    return btree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    if (!_root) return rend();

    std::stack<std::pair<btree_node**, size_t>> path;
    auto ptr = &_root;
    size_t i = 0;

    path.emplace(ptr, i);
    while (!(*ptr)->_pointers.empty()) {
        i = (*ptr)->_pointers.size() - 1;
        ptr = &(*ptr)->_pointers[i];
        path.emplace(ptr, i);
    }

    return btree_reverse_iterator(path, (*ptr)->_keys.size() - 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    return btree_reverse_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return crbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    return crend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    return btree_const_reverse_iterator(const_cast<B_tree*>(this)->rbegin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    return btree_const_reverse_iterator(const_cast<B_tree*>(this)->rend());
}

// endregion iterator begins implementation

// region lookup implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    auto it = bound(key);
    return it != end() && !compare_keys(key, it->first) && !compare_keys(it->first, key) ? it : end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    return bound(key, true);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    return bound(key, false);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

// endregion lookup implementation

// region modifiers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    if (_root == nullptr) {
        _size = 0;
        return;
    }

    std::stack<btree_node*> nodes;
    nodes.push(_root);

    while (!nodes.empty()) {
        auto current = nodes.top();
        nodes.pop();

        for (btree_node* child : current->_pointers) if (child != nullptr) nodes.push(child);

        _allocator.template delete_object<btree_node>(current);
    }

    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return insert(tree_data_type(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    static_assert(t >= 2, "B-tree minimum degree must be at least 2");

    const auto key_copy = data.first;

    auto existing = find(key_copy);
    if (existing != end()) return {existing, false};

    if (_root == nullptr) {
        _root = _allocator.template new_object<btree_node>();
        _root->_keys.push_back(std::move(data));
        ++_size;
        return {find(key_copy), true};
    }

    std::stack<btree_node*> path;

    auto current = _root;

    while (true) {
        path.push(current);

        size_t index = 0;

        while (index < current->_keys.size() && compare_keys(current->_keys[index].first, data.first)) ++index;

        if (current->_pointers.empty()) {
            current->_keys.insert(current->_keys.begin() + index, std::move(data));
            break;
        }

        current = current->_pointers[index];
    }

    while (!path.empty()) {
        auto node = path.top();
        path.pop();

        if (node->_keys.size() <= maximum_keys_in_node) break;

        auto right = _allocator.template new_object<btree_node>();

        tree_data_type middle = std::move(node->_keys[t]);

        for (size_t i = t + 1; i < node->_keys.size(); ++i) {
            right->_keys.push_back(std::move(node->_keys[i]));
        }

        node->_keys.resize(t);

        if (!node->_pointers.empty()) {
            for (size_t i = t + 1; i < node->_pointers.size(); ++i) {
                right->_pointers.push_back(node->_pointers[i]);
            }

            node->_pointers.resize(t + 1);
        }

        if (path.empty()) {
            auto new_root = _allocator.template new_object<btree_node>();

            new_root->_keys.push_back(std::move(middle));
            new_root->_pointers.push_back(node);
            new_root->_pointers.push_back(right);

            _root = new_root;
            break;
        }

        auto parent = path.top();

        size_t parent_index = 0;

        while (parent_index < parent->_pointers.size() && parent->_pointers[parent_index] != node) ++parent_index;

        parent->_keys.insert(parent->_keys.begin() + parent_index, std::move(middle));
        parent->_pointers.insert(parent->_pointers.begin() + parent_index + 1, right);
    }

    ++_size;

    return {find(key_copy), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    return insert(tree_data_type(std::forward<Args>(args)...));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    return emplace_or_assign(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    return emplace_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);

    auto it = find(data.first);
    return it != end() ? (it->second = std::move(data.second), it) : insert(std::move(data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    if (pos == end()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    if (pos == cend()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    if (beg == en) return beg;

    std::optional<tkey> end_key;

    if (en != end()) end_key.emplace(en->first);

    while (beg != end()) {
        if (end_key && !compare_keys(beg->first, *end_key)) break;
        beg = erase(beg);
    }

    return end_key ? lower_bound(*end_key) : end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    if (beg == en) return end();
    if (beg == cend()) return end();

    return erase(lower_bound(beg->first), en == cend() ? end() : lower_bound(en->first));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (!_root) return end();

    auto it = find(key);
    if (it == end()) return end();

    auto next = it;
    ++next;

    std::optional<tkey> next_key;
    if (next != end()) next_key.emplace(next->first);

    auto key_index = [this](btree_node* node, const tkey& k) {
        size_t i = 0;
        while (i < node->_keys.size() && compare_keys(node->_keys[i].first, k)) ++i;
        return i;
    };

    auto equal_keys = [this](const tkey& a, const tkey& b) {
        return !compare_keys(a, b) && !compare_keys(b, a);
    };

    auto merge = [this](btree_node* parent, size_t i) {
        btree_node* left = parent->_pointers[i];
        btree_node* right = parent->_pointers[i + 1];

        left->_keys.push_back(std::move(parent->_keys[i]));

        for (auto& item : right->_keys) {
            left->_keys.push_back(std::move(item));
        }

        for (auto child : right->_pointers) {
            left->_pointers.push_back(child);
        }

        parent->_keys.erase(parent->_keys.begin() + i);
        parent->_pointers.erase(parent->_pointers.begin() + i + 1);

        _allocator.template delete_object<btree_node>(right);
    };

    auto borrow_from_left = [](btree_node* parent, size_t i) {
        btree_node* child = parent->_pointers[i];
        btree_node* left = parent->_pointers[i - 1];

        child->_keys.insert(child->_keys.begin(), std::move(parent->_keys[i - 1]));
        parent->_keys[i - 1] = std::move(left->_keys.back());
        left->_keys.pop_back();

        if (!left->_pointers.empty()) {
            child->_pointers.insert(child->_pointers.begin(), left->_pointers.back());
            left->_pointers.pop_back();
        }
    };

    auto borrow_from_right = [](btree_node* parent, size_t i) {
        btree_node* child = parent->_pointers[i];
        btree_node* right = parent->_pointers[i + 1];

        child->_keys.push_back(std::move(parent->_keys[i]));
        parent->_keys[i] = std::move(right->_keys.front());
        right->_keys.erase(right->_keys.begin());

        if (!right->_pointers.empty()) {
            child->_pointers.push_back(right->_pointers.front());
            right->_pointers.erase(right->_pointers.begin());
        }
    };

    std::function<void(btree_node*, const tkey&)> erase_inner;

    erase_inner = [&](btree_node* node, const tkey& k) {
        size_t i = key_index(node, k);

        bool found = i < node->_keys.size() && equal_keys(node->_keys[i].first, k);

        if (found) {
            if (node->_pointers.empty()) {
                node->_keys.erase(node->_keys.begin() + i);
                --_size;
                return;
            }

            btree_node* left = node->_pointers[i];
            btree_node* right = node->_pointers[i + 1];

            if (left->_keys.size() >= t) {
                btree_node* pred = left;
                while (!pred->_pointers.empty()) pred = pred->_pointers.back();

                tkey pred_key = pred->_keys.back().first;
                node->_keys[i] = pred->_keys.back();

                erase_inner(left, pred_key);
                return;
            }

            if (right->_keys.size() >= t) {
                btree_node* succ = right;
                while (!succ->_pointers.empty()) succ = succ->_pointers.front();

                tkey succ_key = succ->_keys.front().first;
                node->_keys[i] = succ->_keys.front();

                erase_inner(right, succ_key);
                return;
            }

            merge(node, i);
            erase_inner(left, k);
            return;
        }

        if (node->_pointers.empty()) return;

        if (node->_pointers[i]->_keys.size() == minimum_keys_in_node) {
            if (i > 0 && node->_pointers[i - 1]->_keys.size() >= t) {
                borrow_from_left(node, i);
            } else if (i + 1 < node->_pointers.size() && node->_pointers[i + 1]->_keys.size() >= t) {
                borrow_from_right(node, i);
            } else {
                if (i + 1 < node->_pointers.size()) {
                    merge(node, i);
                } else {
                    merge(node, i - 1);
                    --i;
                }
            }

            i = key_index(node, k);

            if (i >= node->_pointers.size()) i = node->_pointers.size() - 1;
        }

        erase_inner(node->_pointers[i], k);
    };

    erase_inner(_root, key);

    if (_root && _root->_keys.empty()) {
        auto old_root = _root;

        if (_root->_pointers.empty()) {
            _root = nullptr;
        } else {
            _root = _root->_pointers.front();
        }

        get_allocator().template delete_object<btree_node>(old_root);
    }

    return next_key ? lower_bound(*next_key) : end();
}

// endregion modifiers implementation

// region helpers implementation
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::bound(const tkey& key, bool strict)
{
    if (!_root) return end();

    std::stack<std::pair<btree_node**, size_t>> path;
    std::stack<std::pair<btree_node**, size_t>> best_path;

    size_t best_index = 0;
    bool found = false;

    auto ptr = &_root;
    size_t i = 0;

    while (ptr && *ptr) {
        auto node = *ptr;
        size_t lo = 0;
        size_t hi = node->_keys.size();

        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;

            const auto mid_key = node->_keys[mid].first;
            if (strict ? !compare_keys(key, mid_key) : compare_keys(mid_key, key)) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        path.emplace(ptr, i);

        if (lo < node->_keys.size()) {
            best_path = path;
            best_index = lo;
            found = true;
        }

        if (node->_pointers.empty()) break;

        i = lo;
        ptr = &node->_pointers[lo];
    }

    return found ? btree_iterator(best_path, best_index) : end();
}
// endregion helpers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_pairs(const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &lhs,
                   const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &rhs)
{
    return compare_keys<tkey, tvalue, compare, t>(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_keys(const tkey &lhs, const tkey &rhs)
{
    return compare{}(lhs, rhs);
}


#endif