#include "../include/allocator_red_black_tree.h"

// region helpers implementation
inline std::pmr::memory_resource** allocator_red_black_tree::parent_allocator(void* trusted) noexcept {
    return reinterpret_cast<memory_resource**>(trusted);
}

inline allocator_with_fit_mode::fit_mode* allocator_red_black_tree::fit_mode(void* trusted) noexcept {
    return reinterpret_cast<allocator_with_fit_mode::fit_mode*>(
        reinterpret_cast<std::byte*>(trusted) + sizeof(allocator_dbg_helper*));
}

inline std::byte** allocator_red_black_tree::root(void* trusted) noexcept {
    return reinterpret_cast<std::byte**>(reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size - sizeof(void*));
}

inline std::byte** allocator_red_black_tree::next(void* b_m) noexcept {
    return reinterpret_cast<std::byte**>(reinterpret_cast<std::byte*>(b_m) + sizeof(block_data) + sizeof(void*));
}

inline std::byte** allocator_red_black_tree::prev(void* b_m) noexcept {
    return reinterpret_cast<std::byte**>(reinterpret_cast<std::byte*>(b_m) + sizeof(block_data));
}

inline std::byte** allocator_red_black_tree::left(void* b_m) noexcept {
    return reinterpret_cast<std::byte**>(reinterpret_cast<std::byte*>(b_m) + sizeof(block_data) + 2 * sizeof(void*));
}

inline std::byte** allocator_red_black_tree::right(void* b_m) noexcept {
    return reinterpret_cast<std::byte**>(reinterpret_cast<std::byte*>(b_m) + sizeof(block_data) + 3 * sizeof(void*));
}

inline size_t* allocator_red_black_tree::space_size(void* trusted) noexcept {
    return reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(trusted)
        + sizeof(allocator_dbg_helper*) + sizeof(allocator_with_fit_mode::fit_mode));
}

inline std::mutex* allocator_red_black_tree::mtx(void* trusted) noexcept {
    return reinterpret_cast<std::mutex*>(
        reinterpret_cast<std::byte*>(trusted) + sizeof(allocator_dbg_helper*)
        + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
}

inline size_t allocator_red_black_tree::block_size(void* trusted, void* b_m) noexcept {
    auto n = *next(b_m);
    if (!n) n = trusted_end(trusted);
    return n - reinterpret_cast<std::byte*>(b_m);
}

inline allocator_red_black_tree::block_data* allocator_red_black_tree::block_node(void* b_m) noexcept {
    return reinterpret_cast<block_data*>(b_m);
}

inline std::byte* allocator_red_black_tree::block_metadata(void* block) noexcept {
    return reinterpret_cast<std::byte*>(block) - occupied_block_metadata_size;
}

inline size_t allocator_red_black_tree::block_metadata_size(void* b_m) noexcept {
    return block_node(b_m)->occupied ? occupied_block_metadata_size : free_block_metadata_size;
}

inline std::byte* allocator_red_black_tree::first_block(void* trusted) noexcept {
    return reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size;
}

inline std::byte* allocator_red_black_tree::trusted_end(void* trusted) noexcept {
    return reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size + *space_size(trusted);
}

inline void** allocator_red_black_tree::parent(void* b_m) noexcept {
    return reinterpret_cast<void**>(reinterpret_cast<std::byte*>(b_m) + block_metadata_size(b_m) - sizeof(void*));
}
// endregion helpers implementation

// region red-black tree implementation
inline bool allocator_red_black_tree::is_red(void* node) noexcept {
    return node && block_node(node)->color == block_color::RED;
}

inline bool allocator_red_black_tree::is_black(void* node) noexcept {
    return node == nullptr || block_node(node)->color == block_color::BLACK;
}

inline bool allocator_red_black_tree::is_left_child(void* node) noexcept {
    const auto p = *parent(node);
    return p && *left(p) == node;
}

inline bool allocator_red_black_tree::is_right_child(void* node) noexcept {
    const auto p = *parent(node);
    return p && *right(p) == node;
}

std::byte* allocator_red_black_tree::maximum(void* node) noexcept {
    auto curr = reinterpret_cast<std::byte*>(node);
    while (*right(curr)) curr = *right(curr);
    return curr;
}

char allocator_red_black_tree::compare(void *u, void *v) const noexcept {
    const auto size_a = block_size(_trusted_memory, u);
    const auto size_b = block_size(_trusted_memory, v);

    if (size_a < size_b) return -1;
    if (size_a > size_b) return 1;

    if (u < v) return -1;
    if (u > v) return 1;

    return 0;
}

void allocator_red_black_tree::transplant(void* u, void* v) const noexcept {
    const auto p = *parent(u);
    if (p == nullptr) {
        *root(_trusted_memory) = reinterpret_cast<std::byte*>(v);
    } else if (is_left_child(u)) {
        *left(p) = reinterpret_cast<std::byte*>(v);
    } else {
        *right(p) = reinterpret_cast<std::byte*>(v);
    }

    if (v) *parent(v) = reinterpret_cast<std::byte*>(p);
}

void allocator_red_black_tree::rotate_left(void *x) const noexcept {
    if (!x) return;

    const auto p = *parent(x);
    const auto tmp = *left(x);

    if (p == nullptr || *right(p) != x) return;

    transplant(p, x);

    *left(x) = reinterpret_cast<std::byte*>(p);
    *parent(p) = reinterpret_cast<std::byte*>(x);

    *right(p) = tmp;
    if (tmp) *parent(tmp) = reinterpret_cast<std::byte*>(p);
}

void allocator_red_black_tree::rotate_right(void *y) const noexcept {
    if (!y) return;

    const auto p = *parent(y);
    const auto tmp = *right(y);

    if (p == nullptr || *left(p) != y) return;

    transplant(p, y);

    *right(y) = reinterpret_cast<std::byte*>(p);
    *parent(p) = reinterpret_cast<std::byte*>(y);

    *left(p) = tmp;
    if (tmp) *parent(tmp) = reinterpret_cast<std::byte*>(p);
}

void allocator_red_black_tree::rotate_big_left(void *x) const noexcept {
    if (!x) return;

    rotate_right(x);
    rotate_left(x);
}

void allocator_red_black_tree::rotate_big_right(void *y) const noexcept {
    if (!y) return;

    rotate_left(y);
    rotate_right(y);
}

void allocator_red_black_tree::swap_with_predecessor(void *node) const noexcept {
    const auto r = maximum(*left(node));
    if (node == r) return;

    const auto node_l = *left(node);
    const auto node_r = *right(node);
    const auto node_color = block_node(node)->color;

    const auto r_p = *parent(r);
    const auto r_l = *left(r);
    const auto r_color = block_node(r)->color;

    const bool r_was_left = r_p && *left(r_p) == r;

    if (r_p == node) {
        transplant(node, r);
        *right(r) = reinterpret_cast<std::byte*>(node);
        *parent(node) = r;
        *left(node) = nullptr;
    } else {
        transplant(r, r_l);
        transplant(node, r);
        *left(r) = node_l;
        if (node_l) *parent(node_l) = r;
        *right(r) = node_r;
        if (node_r) *parent(node_r) = r;

        r_was_left ? *left(r_p) = reinterpret_cast<std::byte*>(node) : *right(r_p) = reinterpret_cast<std::byte*>(node);
        *parent(node) = reinterpret_cast<std::byte*>(r_p);
        *left(node) = r_l;
        if (r_l) *parent(r_l) = reinterpret_cast<std::byte*>(node);
        *right(node) = nullptr;
    }

    block_node(node)->color = r_color;
    block_node(r)->color = node_color;
}

void allocator_red_black_tree::insert(void *new_node) const {
    block_node(new_node)->occupied = false;
    block_node(new_node)->color = block_color::RED;
    *left(new_node) = nullptr;
    *right(new_node) = nullptr;
    *parent(new_node) = nullptr;

    void *parent_node = nullptr;
    auto curr_node = *root(_trusted_memory);

    while (curr_node != nullptr) {
        parent_node = curr_node;

        const auto cmp = compare(new_node, curr_node);

        if (cmp < 0) {
            curr_node = *left(curr_node);
        } else if (cmp > 0) {
            curr_node = *right(curr_node);
        } else {
            return;
        }
    }

    if (parent_node == nullptr) {
        *root(_trusted_memory) = reinterpret_cast<std::byte*>(new_node);
    } else if (compare(new_node, parent_node) < 0) {
        *left(parent_node) = reinterpret_cast<std::byte*>(new_node);
        *parent(new_node) = reinterpret_cast<std::byte*>(parent_node);
    } else {
        *right(parent_node) = reinterpret_cast<std::byte*>(new_node);
        *parent(new_node) = reinterpret_cast<std::byte*>(parent_node);
    }

    while (true) {
        if (*root(_trusted_memory) == nullptr) return;

        if (*parent(new_node) == nullptr) {
            block_node(*root(_trusted_memory))->color = block_color::BLACK;
            return;
        }

        if (is_black(*parent(new_node))) return;

        const auto p = *parent(new_node);
        const auto g = *parent(p);

        if (g == nullptr) {
            block_node(p)->color = block_color::BLACK;
            return;
        }

        auto u = is_left_child(p) ? *right(g) : *left(g);

        if (is_red(u)) {
            block_node(p)->color = block_color::BLACK;
            block_node(u)->color = block_color::BLACK;
            block_node(g)->color = block_color::RED;

            new_node = g;
            continue;
        }

        if (is_left_child(new_node) && is_left_child(p)) {
            rotate_right(p);

            block_node(p)->color = block_color::BLACK;
            block_node(g)->color = block_color::RED;
        } else if (is_right_child(new_node) && is_right_child(p)) {
            rotate_left(p);

            block_node(p)->color = block_color::BLACK;
            block_node(g)->color = block_color::RED;
        } else if (is_right_child(new_node) && is_left_child(p)) {
            rotate_big_right(new_node);

            block_node(new_node)->color = block_color::BLACK;
            block_node(g)->color = block_color::RED;
        } else if (is_left_child(new_node) && is_right_child(p)) {
            rotate_big_left(new_node);

            block_node(new_node)->color = block_color::BLACK;
            block_node(g)->color = block_color::RED;
        }

        break;
    }

    if (*root(_trusted_memory)) block_node(*root(_trusted_memory))->color = block_color::BLACK;
}

void allocator_red_black_tree::remove(void* node) const {
    while (true) {
        if (is_red(node)) {
            if (*left(node) == nullptr && *right(node) == nullptr) {
                transplant(node, nullptr);
                return;
            } else if ((*left(node) == nullptr) != (*right(node) == nullptr)) {
                throw std::logic_error("Invalid red-black tree");
            } else {
                swap_with_predecessor(node);
            }
        } else {
            if (*left(node) == nullptr && *right(node) == nullptr) {
                bool deleted_was_left = is_left_child(node);
                auto p = *parent(node);
                transplant(node, nullptr);

                if (p == nullptr) {
                    if (*root(_trusted_memory)) block_node(*root(_trusted_memory))->color = block_color::BLACK;
                    return;
                }

                while (p != nullptr) {
                    const auto b = deleted_was_left ? *right(p) : *left(p);
                    const auto n = deleted_was_left ? (b ? *left(b) : nullptr) : b ? *right(b) : nullptr;
                    const auto fn = deleted_was_left ? (b ? *right(b) : nullptr) : b ? *left(b) : nullptr;

                    if (is_black(b)) {
                        // Case 1.1a
                        if (is_red(fn)) {
                            block_node(b)->color = block_node(p)->color;
                            block_node(p)->color = block_color::BLACK;
                            block_node(fn)->color = block_color::BLACK;

                            deleted_was_left ? rotate_left(b) : rotate_right(b);

                            if (*root(_trusted_memory)) block_node(*root(_trusted_memory))->color = block_color::BLACK;

                            return;
                        }

                        // Case 1.1b
                        if (is_red(n) && is_black(fn)) {
                            block_node(n)->color = block_color::BLACK;
                            block_node(b)->color = block_color::RED;

                            deleted_was_left ? rotate_right(n) : rotate_left(n);
                            continue;
                        }

                        // Case 1.2
                        if (is_black(n) && is_black(fn)) {
                            if (b) block_node(b)->color = block_color::RED;

                            if (is_red(p)) {
                                block_node(p)->color = block_color::BLACK;

                                if (*root(_trusted_memory)) block_node(*root(_trusted_memory))->color = block_color::BLACK;
                                return;
                            }

                            if (*parent(p) == nullptr) {
                                if (*root(_trusted_memory)) block_node(*root(_trusted_memory))->color = block_color::BLACK;
                                return;
                            }

                            deleted_was_left = is_left_child(p);
                            p = *parent(p);
                        }
                    } else {
                        // Case 2: sibling is red
                        deleted_was_left ? rotate_left(b) : rotate_right(b);

                        block_node(p)->color = block_color::RED;
                        block_node(b)->color = block_color::BLACK;
                    }
                }

                if (*root(_trusted_memory)) block_node(*root(_trusted_memory))->color = block_color::BLACK;
                return;
            } else if (*left(node) == nullptr != (*right(node) == nullptr)) {
                const auto child = *left(node) ? *left(node) : *right(node);
                transplant(node, child);
                block_node(child)->color = block_color::BLACK;
                return;
            } else {
                swap_with_predecessor(node);
            }
        }
    }
}
// endregion red-black tree implementation

allocator_red_black_tree::~allocator_red_black_tree()
{
    if (!_trusted_memory) return;
    mtx(_trusted_memory)->~mutex();
    (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory,
        *space_size(_trusted_memory) + allocator_metadata_size);
}

allocator_red_black_tree::allocator_red_black_tree(
allocator_red_black_tree &&other) noexcept: _trusted_memory(nullptr)
{
    if (!other._trusted_memory) return;

    std::lock_guard lock(*mtx(other._trusted_memory));
    _trusted_memory = std::exchange(other._trusted_memory, nullptr);
}

allocator_red_black_tree &allocator_red_black_tree::operator=(
    allocator_red_black_tree &&other) noexcept
{
    if (do_is_equal(other)) return *this;

    if (_trusted_memory) {
        mtx(_trusted_memory)->~mutex(); // because mutex in _trusted_memory
        (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory,
        *space_size(_trusted_memory) + allocator_metadata_size);
    }
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;

    return *this;
}

allocator_red_black_tree::allocator_red_black_tree(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (space_size < free_block_metadata_size) throw std::logic_error("too small size");
    if (!parent_allocator) parent_allocator = std::pmr::get_default_resource();

    _trusted_memory = parent_allocator->allocate(allocator_metadata_size + space_size);

    *allocator_red_black_tree::parent_allocator(_trusted_memory) = parent_allocator;
    *allocator_red_black_tree::space_size(_trusted_memory) = space_size;
    new (mtx(_trusted_memory)) std::mutex();
    set_fit_mode(allocate_fit_mode);

    const auto fb = first_block(_trusted_memory);
    *prev(fb) = nullptr;
    *next(fb) = nullptr;
    insert(fb);
}

bool allocator_red_black_tree::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}

[[nodiscard]] void *allocator_red_black_tree::do_allocate_sm(
    size_t size)
{
    std::lock_guard lock(*mtx(_trusted_memory));

    void *best_current = nullptr;
    const auto mode = *fit_mode(_trusted_memory);
    auto current = *root(_trusted_memory);

    if (mode == fit_mode::first_fit) {
        while (current) {
            if (block_size(_trusted_memory, current) >= size + occupied_block_metadata_size) {
                best_current = current;
                break;
            }
            current = *right(current);
        }
    } else if (mode == fit_mode::the_best_fit) {
        while (current) {
            const auto current_size = block_size(_trusted_memory, current);
            if (current_size >= size + occupied_block_metadata_size) {
                best_current = current;
                current = *left(current);
            } else {
                current = *right(current);
            }
        }
    } else if (mode == fit_mode::the_worst_fit) {
        if (current) {
            current = maximum(current);
            if (block_size(_trusted_memory, current) >= size + occupied_block_metadata_size) {
                best_current = current;
            }
        }
    }

    if (!best_current) throw std::bad_alloc();

    remove(best_current);

    if (block_size(_trusted_memory, best_current) >= size + occupied_block_metadata_size + free_block_metadata_size) {
        const auto rest_block = reinterpret_cast<std::byte*>(best_current) + size + occupied_block_metadata_size;

        *prev(rest_block) = reinterpret_cast<std::byte*>(best_current);
        *next(rest_block) = *next(best_current);
        if (*next(best_current)) *prev(*next(best_current)) = rest_block;
        *next(best_current) = rest_block;

        insert(rest_block);
    }

    block_node(best_current)->occupied = true;
    block_node(best_current)->color = block_color::BLACK;
    *parent(best_current) = _trusted_memory;

    return reinterpret_cast<std::byte*>(best_current) + occupied_block_metadata_size;
}


void allocator_red_black_tree::do_deallocate_sm(
    void *at)
{
    if (!at) return;

    std::lock_guard lock(*mtx(_trusted_memory));

    auto block = block_metadata(at);

    if (!block_node(block)->occupied) return;
    if (_trusted_memory != *parent(block)) return;

    const auto p = *prev(block);
    const auto n = *next(block);

    if (p && !block_node(p)->occupied) {
        remove(p);
        *next(p) = n;
        block = p;
        if (n) *prev(n) = p;
    }
    if (n && !block_node(n)->occupied) {
        remove(n);
        const auto nn = *next(n);
        *next(block) = nn;
        if (nn) *prev(nn) = block;
    }

    insert(block);
}

void allocator_red_black_tree::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    *fit_mode(_trusted_memory) = mode;
}


std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const
{
    if (!_trusted_memory) return {};
    try {
        std::lock_guard lock(*mtx(_trusted_memory));
        return get_blocks_info_inner();
    }
    catch (...) {
        return {};
    }
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info_inner() const
{
    std::vector<block_info> result;
    for (auto it = begin(); it != end(); ++it) result.push_back({it.size(), it.occupied()});
    return result;
}


allocator_red_black_tree::rb_iterator allocator_red_black_tree::begin() const noexcept
{
    return {_trusted_memory};
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::end() const noexcept
{
    return {};
}

bool allocator_red_black_tree::rb_iterator::operator==(const allocator_red_black_tree::rb_iterator &other) const noexcept
{
    return _block_ptr == other._block_ptr;
}

bool allocator_red_black_tree::rb_iterator::operator!=(const allocator_red_black_tree::rb_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_red_black_tree::rb_iterator &allocator_red_black_tree::rb_iterator::operator++() & noexcept
{
    if (_block_ptr) _block_ptr = *next(_block_ptr);
    return *this;
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::rb_iterator::operator++(int n)
{
    const auto tmp = *this;
    ++*this;
    return tmp;
}

size_t allocator_red_black_tree::rb_iterator::size() const noexcept
{
    return _block_ptr != nullptr ? block_size(_trusted, _block_ptr) : 0;
}

void *allocator_red_black_tree::rb_iterator::operator*() const noexcept
{
    return _block_ptr;
}

allocator_red_black_tree::rb_iterator::rb_iterator(): _block_ptr(nullptr), _trusted(nullptr) {}

allocator_red_black_tree::rb_iterator::rb_iterator(void *trusted): rb_iterator()
{
    if (!trusted) return;

    _trusted = trusted;
    _block_ptr = first_block(trusted);
}

bool allocator_red_black_tree::rb_iterator::occupied() const noexcept
{
    return _block_ptr && block_node(_block_ptr)->occupied;
}
