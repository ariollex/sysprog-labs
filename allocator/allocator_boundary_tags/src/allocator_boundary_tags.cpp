#include "../include/allocator_boundary_tags.h"

//region Helpers
inline allocator_with_fit_mode::fit_mode* allocator_boundary_tags::fit_mode(void* trusted) noexcept {
    return reinterpret_cast<allocator_with_fit_mode::fit_mode*>(
        reinterpret_cast<std::byte*>(trusted)
        + sizeof(memory_resource*)
        );
}

inline void** allocator_boundary_tags::forward(void* b_m) noexcept {
    return reinterpret_cast<void**>(reinterpret_cast<std::byte*>(b_m) + sizeof(size_t) + sizeof(void*));
}

inline void** allocator_boundary_tags::back(void* b_m) noexcept {
    return reinterpret_cast<void**>(reinterpret_cast<std::byte*>(b_m) + sizeof(size_t));
}

inline size_t* allocator_boundary_tags::space_size(void* trusted) noexcept {
    return reinterpret_cast<size_t*>(
        reinterpret_cast<std::byte*>(trusted)
        + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode));
}

inline std::mutex* allocator_boundary_tags::mtx(void* trusted) noexcept {
    return reinterpret_cast<std::mutex*>(
    reinterpret_cast<std::byte*>(trusted)
        + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode)
        + sizeof(size_t));
}

inline std::pmr::memory_resource** allocator_boundary_tags::parent_allocator(void* trusted) noexcept {
    return reinterpret_cast<memory_resource**>(trusted);
}

inline size_t* allocator_boundary_tags::block_size(void* b_m) noexcept {
    return reinterpret_cast<size_t*>(b_m);
}

inline void** allocator_boundary_tags::first_occupied(void* trusted) noexcept {
    return reinterpret_cast<void**>(reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size - sizeof(void*));
}

inline void* allocator_boundary_tags::block_metadata(void* block) noexcept {
    return reinterpret_cast<std::byte*>(block) - occupied_block_metadata_size;
}

inline std::byte* allocator_boundary_tags::block_end(void* b_m) noexcept {
    return reinterpret_cast<std::byte*>(b_m) + occupied_block_metadata_size + *block_size(b_m);
}

inline void* allocator_boundary_tags::block_data(void* b_m) noexcept {
    return reinterpret_cast<std::byte*>(b_m) + occupied_block_metadata_size;
}

inline void* allocator_boundary_tags::first_block(void* trusted) noexcept {
    return reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size;
}

inline void* allocator_boundary_tags::trusted_end(void* trusted) noexcept {
    return reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size + *space_size(trusted);
}

inline void** allocator_boundary_tags::parent(void* b_m) noexcept {
    return reinterpret_cast<void**>(reinterpret_cast<std::byte*>(b_m) + sizeof(size_t) + sizeof(void*) + sizeof(void*));
}
//endregion

allocator_boundary_tags::~allocator_boundary_tags()
{
    if (!_trusted_memory) return;
    mtx(_trusted_memory)->~mutex();
    (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory,
        *space_size(_trusted_memory) + allocator_metadata_size);
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (do_is_equal(other)) return *this;

    if (_trusted_memory)
    {
        mtx(_trusted_memory)->~mutex(); // because mutex in _trusted_memory
        (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory,
            *space_size(_trusted_memory) + allocator_metadata_size);
    }
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;

    return *this;
}


/** If parent_allocator* == nullptr you should use std::pmr::get_default_resource()
 */
allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (!parent_allocator) parent_allocator = std::pmr::get_default_resource();

    _trusted_memory = parent_allocator->allocate(allocator_metadata_size + space_size);

    *allocator_boundary_tags::parent_allocator(_trusted_memory) = parent_allocator;
    *allocator_boundary_tags::space_size(_trusted_memory) = space_size;
    new (mtx(_trusted_memory)) std::mutex();
    set_fit_mode(allocate_fit_mode);
    *first_occupied(_trusted_memory) = nullptr;
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
    size_t size)
{
    if (!_trusted_memory) throw std::logic_error("Allocator is empty");
    std::lock_guard lock(*mtx(_trusted_memory));

    void* best_prev_occupied = nullptr;
    void* best_current_free = nullptr;
    void* best_next_occupied = nullptr;
    size_t best_size = 0;

    const auto mode = *fit_mode(_trusted_memory);

    size_t size_free = 0;

    auto it = begin();
    if (!it.occupied()) {
        size_free = *space_size(_trusted_memory);
        if (size_free >= occupied_block_metadata_size + size) {
            best_size = size_free;
            best_current_free = first_block(_trusted_memory);
        }
    } else {
        void* current_free = nullptr;
        void* prev_occupied = nullptr;

        while (true) {
            current_free = prev_occupied ? block_end(prev_occupied) : first_block(_trusted_memory);

            void* right = it == end() ? trusted_end(_trusted_memory) : *it;
            size_free = reinterpret_cast<std::byte*>(right) - reinterpret_cast<std::byte*>(current_free);

            if (size_free >= occupied_block_metadata_size + size) {
                if (!best_current_free ||
                mode == fit_mode::first_fit ||
                (mode == fit_mode::the_best_fit && size_free < best_size) ||
                (mode == fit_mode::the_worst_fit && size_free > best_size)
                ) {
                    best_size = size_free;
                    best_current_free = current_free;
                    best_prev_occupied = prev_occupied;
                    best_next_occupied = (it == end()) ? nullptr : *it;
                    if (mode == fit_mode::first_fit) break;
                }
            }
            if (it == end()) break;

            prev_occupied = *it++;
        }
    }

    if (!best_current_free) throw std::bad_alloc();

    if (best_size >= size + occupied_block_metadata_size + sizeof(void*)) { // so, we can reserve not all space
        *block_size(best_current_free) = size;
    } else { // if < sizeof(void), lets occupy all memory
        *block_size(best_current_free) = best_size - occupied_block_metadata_size;
    }

    *parent(best_current_free) = _trusted_memory;
    if (best_prev_occupied) {
        *forward(best_prev_occupied) = best_current_free;
        *back(best_current_free) = best_prev_occupied;
    } else {
        *first_occupied(_trusted_memory) = best_current_free;
        *back(best_current_free) = nullptr;
    }

    if (best_next_occupied) {
        *back(best_next_occupied) = best_current_free;
        *forward(best_current_free) = best_next_occupied;
    } else {
        *forward(best_current_free) = nullptr;
    }

    return block_data(best_current_free);
}

void allocator_boundary_tags::do_deallocate_sm(
    void *at)
{
    if (!at) return;
    const auto block = block_metadata(at);

    if (_trusted_memory != *parent(block)) return;

    std::lock_guard lock(*mtx(*parent(block)));

    void* prev = *back(block);
    void* next = *forward(block);

    if (prev) {
        *forward(prev) = next;
    } else {
        *first_occupied(*parent(block)) = next;
    }

    if (next) *back(next) = prev;
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (!_trusted_memory) return;
    *fit_mode(_trusted_memory) = mode;
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    if (!_trusted_memory) return {};
    try {
        std::lock_guard lock(*mtx(_trusted_memory));
        return get_blocks_info_inner();
    } catch (...) {
        return {};
    }
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return {_trusted_memory};
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    return {nullptr};
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
    std::vector<block_info> result;

    auto it = begin();

    if (!it.occupied()) {
        result.push_back({
            static_cast<size_t>(reinterpret_cast<std::byte*>(trusted_end(_trusted_memory))
                - reinterpret_cast<std::byte*>(first_block(_trusted_memory))), false});
        return result;
    }

    void* p_o = nullptr; // prev occupied

    while (true) {
        const std::byte* right = it == end() ?
            reinterpret_cast<std::byte*>(trusted_end(_trusted_memory)) :
            reinterpret_cast<std::byte*>(*it);
        std::byte* free_begin = p_o ? block_end(p_o) : reinterpret_cast<std::byte*>(first_block(_trusted_memory));
        if (right != free_begin) {
            result.push_back({static_cast<size_t>(right - free_begin), false});
        }
        if (it == end()) break;

        result.push_back({occupied_block_metadata_size + *block_size(*it), true});

        p_o = *it;
        ++it;
    }

    return result;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}

bool allocator_boundary_tags::boundary_iterator::operator==(
        const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _occupied_ptr == other._occupied_ptr;
}

bool allocator_boundary_tags::boundary_iterator::operator!=(
        const allocator_boundary_tags::boundary_iterator & other) const noexcept
{
    return !(*this == other);
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (_occupied_ptr == nullptr) return *this;

    _occupied_ptr = *forward(_occupied_ptr);
    _occupied = _occupied_ptr != nullptr;

    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (_occupied_ptr == nullptr) return *this;

    _occupied_ptr = *back(_occupied_ptr);
    _occupied = _occupied_ptr != nullptr;

    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    const auto tmp = *this;
    ++*this;
    return tmp;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    const auto tmp = *this;
    --*this;
    return tmp;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (!_occupied_ptr) return 0;
    return *block_size(_occupied_ptr);
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    return _occupied;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    return _occupied_ptr;
}

allocator_boundary_tags::boundary_iterator::boundary_iterator():
    _occupied_ptr(nullptr), _occupied(false), _trusted_memory(nullptr) {}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted): boundary_iterator()
{
    if (!trusted) return;

    _occupied_ptr = *first_occupied(trusted);
    _occupied = _occupied_ptr != nullptr;
    _trusted_memory = trusted;
}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    return _occupied_ptr;
}
