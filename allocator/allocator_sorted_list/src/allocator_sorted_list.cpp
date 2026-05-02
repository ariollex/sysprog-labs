#include "../include/allocator_sorted_list.h"

//region Helpers
inline allocator_with_fit_mode::fit_mode* allocator_sorted_list::fit_mode(void* trusted) noexcept {
    return reinterpret_cast<allocator_with_fit_mode::fit_mode*>(
        reinterpret_cast<std::byte*>(trusted)
        + sizeof(std::pmr::memory_resource*));
}

inline void** allocator_sorted_list::forward(void* b_m) noexcept {
    return reinterpret_cast<void**>(b_m);
}

inline void** allocator_sorted_list::parent(void* b_m) noexcept {
    // free block have "forward" first field, and occupied - "parent", so helpers for these fields are same
    return forward(b_m);
}

inline size_t* allocator_sorted_list::space_size(void* trusted) noexcept {
    return reinterpret_cast<size_t*>(
        reinterpret_cast<std::byte*>(trusted)
        + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode));
}

inline std::mutex* allocator_sorted_list::mtx(void* trusted) noexcept {
    return reinterpret_cast<std::mutex*>(
        reinterpret_cast<std::byte*>(trusted)
        + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode)
        + sizeof(size_t));
}

inline std::pmr::memory_resource** allocator_sorted_list::parent_allocator(void* trusted) noexcept {
    return reinterpret_cast<std::pmr::memory_resource**>(trusted);
}

inline size_t* allocator_sorted_list::block_size(void* b_m) noexcept {
    return reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(b_m) + sizeof(void*));
}

inline void** allocator_sorted_list::free_first_block(void* trusted) noexcept {
    return reinterpret_cast<void**>(reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size - sizeof(void*));
}

inline void* allocator_sorted_list::first_block(void* trusted) noexcept {
    return reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size;
}

inline void* allocator_sorted_list::block_metadata(void* block) noexcept {
    return reinterpret_cast<std::byte*>(block) - block_metadata_size;
}

inline std::byte* allocator_sorted_list::block_end(void* b_m) noexcept {
    return reinterpret_cast<std::byte*>(b_m) + block_metadata_size + *block_size(b_m);
}

inline void* allocator_sorted_list::block_data(void* b_m) noexcept {
    return reinterpret_cast<std::byte*>(b_m) + block_metadata_size;
}
//endregion

allocator_sorted_list::~allocator_sorted_list()
{
    if (!_trusted_memory) return;
    mtx(_trusted_memory)->~mutex(); // because of placement new with mutex
    (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory, *space_size(_trusted_memory) + allocator_metadata_size);
}

allocator_sorted_list::allocator_sorted_list(
    allocator_sorted_list &&other) noexcept: _trusted_memory(nullptr)
{
    if (other._trusted_memory) {
        std::lock_guard lock(*mtx(other._trusted_memory));
        _trusted_memory = std::exchange(other._trusted_memory, nullptr);
    }
}

allocator_sorted_list &allocator_sorted_list::operator=(
    allocator_sorted_list &&other) noexcept
{
    if (do_is_equal(other)) return *this;

    if (_trusted_memory)
    {
        mtx(_trusted_memory)->~mutex(); // because mutex in _trusted_memory
        (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory, *space_size(_trusted_memory) + allocator_metadata_size);
    }
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;

    return *this;
}

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (!parent_allocator) parent_allocator = std::pmr::get_default_resource();
    _trusted_memory = parent_allocator->allocate(allocator_metadata_size + block_metadata_size + space_size);

    *allocator_sorted_list::parent_allocator(_trusted_memory) = parent_allocator;
    set_fit_mode(allocate_fit_mode);
    *allocator_sorted_list::space_size(_trusted_memory) = space_size + block_metadata_size;
    new (mtx(_trusted_memory)) std::mutex(); // placement new for mutex

    void* f_b = first_block(_trusted_memory);
    *free_first_block(_trusted_memory) = f_b;

    *forward(f_b) = nullptr;
    *block_size(f_b) = space_size;
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(
    size_t size)
{
    std::lock_guard lock(*mtx(_trusted_memory));

    void* prev = nullptr;

    void* best_prev = nullptr;
    void* best_current = nullptr;

    const auto mode = *fit_mode(_trusted_memory);

    for (auto it = free_begin(); it != free_end(); ++it) {
        if (*block_size(*it) >= size) {
            if (!best_current ||
            mode == fit_mode::first_fit ||
            (mode == fit_mode::the_best_fit && *block_size(*it) < *block_size(best_current)) ||
            (mode == fit_mode::the_worst_fit && *block_size(*it) > *block_size(best_current))) {
                best_prev = prev;
                best_current = *it;
                if (mode == fit_mode::first_fit) break;
            }
        }
        prev = *it;
    }

    if (!best_current) throw std::bad_alloc();

    // block_metadata_size + sizeof(void*) - minimal size of block
    if (*block_size(best_current) >= size + (block_metadata_size + sizeof(void*))) { // so, we can divide block
        auto* rest_block = reinterpret_cast<std::byte*>(best_current) + block_metadata_size + size;

        *forward(rest_block) = *forward(best_current);
        *block_size(rest_block) = *block_size(best_current) - size - block_metadata_size;

        *block_size(best_current) = size;

        if (best_prev) {
            *forward(best_prev) = rest_block;
        } else {
            *free_first_block(_trusted_memory) = rest_block;
        }
    } else {
        if (best_prev) {
            *forward(best_prev) = *forward(best_current);
        } else {
            *free_first_block(_trusted_memory) = *forward(best_current);
        }
    }

    *parent(best_current) = _trusted_memory;

    return block_data(best_current);
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}

void allocator_sorted_list::do_deallocate_sm(
    void *at)
{
    if (!at) return;

    std::lock_guard lock(*mtx(_trusted_memory));

    const auto block = block_metadata(at);

    if (_trusted_memory != *parent(block)) return;

    void* prev = nullptr; // left block
    auto curr = free_begin(); // right block

    for (; curr != free_end() && *curr < block; ++curr) prev = *curr;

    *forward(block) = *curr;
    if (prev) { // left block
        *forward(prev) = block;
    } else {
        *free_first_block(_trusted_memory) = block;
    }

    if (*curr && block_end(block) == *curr) {
        *block_size(block) += block_metadata_size + *block_size(*curr);
        *forward(block) = *forward(*curr);
    }

    if (prev && block_end(prev) == block) {
        *block_size(prev) += block_metadata_size + *block_size(block);
        *forward(prev) = *forward(block);
    }
}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (!_trusted_memory) return;
    *fit_mode(_trusted_memory) = mode;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
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

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const
{
    std::vector<block_info> result;
    for (auto it = begin(); it != end(); ++it) result.push_back({it.size(), it.occupied()});
    return result;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_begin() const noexcept
{
    return {*free_first_block(_trusted_memory)};
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_end() const noexcept
{
    return {nullptr};
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::begin() const noexcept
{
    return {_trusted_memory};
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::end() const noexcept
{
    return {nullptr};
}

bool allocator_sorted_list::sorted_free_iterator::operator==(
        const allocator_sorted_list::sorted_free_iterator & other) const noexcept
{
    return _free_ptr == other._free_ptr;
}

bool allocator_sorted_list::sorted_free_iterator::operator!=(
        const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_free_iterator &allocator_sorted_list::sorted_free_iterator::operator++() & noexcept
{
    if (_free_ptr) _free_ptr = *forward(_free_ptr);
    return *this;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::sorted_free_iterator::operator++(int n)
{
    const sorted_free_iterator tmp = *this;
    ++*this;
    return tmp;
}

size_t allocator_sorted_list::sorted_free_iterator::size() const noexcept
{
    return _free_ptr ? *block_size(_free_ptr) : 0;
}

void *allocator_sorted_list::sorted_free_iterator::operator*() const noexcept
{
    return _free_ptr;
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(): _free_ptr(nullptr) {}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(void *trusted): _free_ptr(trusted) {}

bool allocator_sorted_list::sorted_iterator::operator==(const allocator_sorted_list::sorted_iterator & other) const noexcept
{
    return _current_ptr == other._current_ptr;
}

bool allocator_sorted_list::sorted_iterator::operator!=(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_iterator &allocator_sorted_list::sorted_iterator::operator++() & noexcept
{
    if (!_current_ptr) return *this; // iterator already on end
    if (_free_ptr == _current_ptr) _free_ptr = *forward(_free_ptr);

    auto* next = block_end(_current_ptr);
    const auto* end = reinterpret_cast<std::byte*>(_trusted_memory) + *space_size(_trusted_memory) + allocator_metadata_size;

    _current_ptr = (next >= end ? nullptr : next);

    return *this;
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::sorted_iterator::operator++(int n)
{
    const sorted_iterator tmp = *this;;
    ++*this;
    return tmp;
}

size_t allocator_sorted_list::sorted_iterator::size() const noexcept
{
    if (!_current_ptr) return 0;
    return *block_size(_current_ptr);
}

void *allocator_sorted_list::sorted_iterator::operator*() const noexcept
{
    return _current_ptr;
}

allocator_sorted_list::sorted_iterator::sorted_iterator(): _free_ptr(nullptr), _current_ptr{nullptr}, _trusted_memory(nullptr)
{}

allocator_sorted_list::sorted_iterator::sorted_iterator(void *trusted): sorted_iterator() {
    if (!trusted) return;

    _trusted_memory = trusted;
    _free_ptr = *free_first_block(_trusted_memory);
    _current_ptr = first_block(_trusted_memory);
}

bool allocator_sorted_list::sorted_iterator::occupied() const noexcept
{
    if (!_current_ptr) return false; // otherwise we can touch memory not in _trusted_memory if _current_ptr in end
    return _current_ptr != _free_ptr;
}
