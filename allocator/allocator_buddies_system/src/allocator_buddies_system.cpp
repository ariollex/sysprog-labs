#include <cstddef>
#include "../include/allocator_buddies_system.h"

//region Helpers
inline unsigned char* allocator_buddies_system::k(void* trusted) noexcept {
    return reinterpret_cast<unsigned char*>(reinterpret_cast<std::byte*>(trusted)
        + sizeof(allocator_dbg_helper*)
        + sizeof(allocator_with_fit_mode::fit_mode));
}

inline std::pmr::memory_resource** allocator_buddies_system::parent_allocator(void* trusted) noexcept {
    return reinterpret_cast<std::pmr::memory_resource**>(trusted);
}

inline allocator_with_fit_mode::fit_mode* allocator_buddies_system::fit_mode(void* trusted) noexcept {
    return reinterpret_cast<allocator_with_fit_mode::fit_mode*>(
        reinterpret_cast<std::byte*>(trusted) + sizeof(allocator_dbg_helper*));
}

inline void* allocator_buddies_system::forward(void* b_m) noexcept {
    return reinterpret_cast<std::byte*>(b_m) + block_size(b_m);
}

inline size_t allocator_buddies_system::space_size(void* trusted) noexcept {
    return (size_t{1} << *k(trusted));
}

inline std::mutex* allocator_buddies_system::mtx(void* trusted) noexcept {
    return reinterpret_cast<std::mutex*>(reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size - sizeof(std::mutex));
}

inline size_t allocator_buddies_system::block_size(void* b_m) noexcept {
    return size_t{1} << reinterpret_cast<block_metadata*>(b_m)->size;
}

inline void* allocator_buddies_system::block_data(void* b_m) noexcept {
    return reinterpret_cast<std::byte*>(b_m) + occupied_block_metadata_size;
}

inline void* allocator_buddies_system::first_block(void* trusted) noexcept {
    return reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size;
}

inline void** allocator_buddies_system::parent(void* b_m) noexcept {
    return reinterpret_cast<void**>(reinterpret_cast<std::byte*>(b_m) + occupied_block_metadata_size - sizeof(void*));
}

inline void* allocator_buddies_system::trusted_end(void* trusted) noexcept {
    return reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size + space_size(trusted);
}

inline void* allocator_buddies_system::buddy(void* b_m) const noexcept {
    auto* base = reinterpret_cast<std::byte*>(first_block(_trusted_memory));
    const auto* block = reinterpret_cast<std::byte*>(b_m);
    return base + (block - base ^ block_size(b_m));
}
//endregion

allocator_buddies_system::~allocator_buddies_system()
{
    if (!_trusted_memory) return;
    mtx(_trusted_memory)->~mutex();
    (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory,
        allocator_metadata_size + space_size(_trusted_memory));
}

allocator_buddies_system::allocator_buddies_system(
    allocator_buddies_system &&other) noexcept: _trusted_memory(nullptr)
{
    if (other._trusted_memory) {
        std::lock_guard lock(*mtx(other._trusted_memory));
        _trusted_memory = std::exchange(other._trusted_memory, nullptr);
    }
}

allocator_buddies_system &allocator_buddies_system::operator=(
    allocator_buddies_system &&other) noexcept
{
    if (this == &other) return *this;

    if (_trusted_memory)
    {
        mtx(_trusted_memory)->~mutex(); // because mutex in _trusted_memory
        (*parent_allocator(_trusted_memory))->deallocate(_trusted_memory,
            allocator_metadata_size + space_size(_trusted_memory));
    }
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;

    return *this;
}

allocator_buddies_system::allocator_buddies_system(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (!parent_allocator) parent_allocator = std::pmr::get_default_resource();

    if (space_size < (size_t{1} << min_k)) throw std::logic_error("space size is too small");

    const auto k_ = __detail::nearest_greater_k_of_2(space_size);

    _trusted_memory = parent_allocator->allocate(allocator_metadata_size + (size_t{1} << k_));

    *allocator_buddies_system::parent_allocator(_trusted_memory) = parent_allocator;
    set_fit_mode(allocate_fit_mode);
    *k(_trusted_memory) = k_;

    new (mtx(_trusted_memory)) std::mutex();

    auto* f_b = reinterpret_cast<block_metadata*>(first_block(_trusted_memory));
    f_b->occupied = false;
    f_b->size = k_;
    *parent(first_block(_trusted_memory)) = _trusted_memory;
}

[[nodiscard]] void *allocator_buddies_system::do_allocate_sm(
    size_t size)
{
    std::lock_guard lock(*mtx(_trusted_memory));

    void* best_current = nullptr;

    const auto mode = *fit_mode(_trusted_memory);

    for (auto it = begin(); it != end(); ++it) {
        if (!it.occupied() && block_size(*it) >= size + occupied_block_metadata_size) {
            if (!best_current ||
            mode == fit_mode::first_fit ||
            (mode == fit_mode::the_best_fit && block_size(*it) < block_size(best_current)) ||
            (mode == fit_mode::the_worst_fit && block_size(*it) > block_size(best_current))) {
                best_current = *it;
                if (mode == fit_mode::first_fit) break;
            }
        }
    }

    if (!best_current) throw std::bad_alloc();

    *parent(best_current) = _trusted_memory;
    auto* best_m = reinterpret_cast<block_metadata*>(best_current);

    while (size + occupied_block_metadata_size <= block_size(best_current) / 2) { // so, we can divide our blocks
        --best_m->size;

        auto* bud = reinterpret_cast<std::byte*>(buddy(best_current));
        auto* bud_m = reinterpret_cast<block_metadata*>(bud);

        bud_m->occupied = false;
        bud_m->size = best_m->size;
        *parent(bud) = _trusted_memory;
    }
    best_m->occupied = true;
    return block_data(best_current);
}

void allocator_buddies_system::do_deallocate_sm(void *at)
{
    if (!at) return;

    auto* block = reinterpret_cast<std::byte*>(at) - occupied_block_metadata_size;
    auto* block_m = reinterpret_cast<block_metadata*>(block);

    std::lock_guard lock(*mtx(*parent(block)));

    if (_trusted_memory != *parent(block)) return;
    if (!block_m->occupied) throw std::logic_error("Block is already free!");

    block_m->occupied = false;

    while (true) {
        if (block_size(block) == space_size(_trusted_memory)) return;

        const auto bud = reinterpret_cast<std::byte*>(buddy(block));
        const auto bud_m = reinterpret_cast<block_metadata*>(bud);

        if (_trusted_memory != *parent(bud)) return;
        if (bud_m->occupied || bud_m->size != block_m->size) return;

        if (bud < block) {
            block = bud;
            block_m = bud_m;
        }

        block_m->occupied = false;
        ++block_m->size;
    }
}

bool allocator_buddies_system::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}

inline void allocator_buddies_system::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (!_trusted_memory) return;
    *fit_mode(_trusted_memory) = mode;
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    if (!_trusted_memory) return {};
    try {
        std::lock_guard lock(*mtx(_trusted_memory));
        return get_blocks_info_inner();
    } catch (...) {
        return {};
    }
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info_inner() const
{
    std::vector<block_info> result;
    for (auto it = begin(); it != end(); ++it) result.push_back({it.size(), it.occupied()});
    return result;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::begin() const noexcept
{
    if (!_trusted_memory) return end();
    return {first_block(_trusted_memory)};
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::end() const noexcept
{
    if (!_trusted_memory) return {nullptr};
    return {trusted_end(_trusted_memory)};
}

bool allocator_buddies_system::buddy_iterator::operator==(const allocator_buddies_system::buddy_iterator &other) const noexcept
{
    return _block == other._block;
}

bool allocator_buddies_system::buddy_iterator::operator!=(const allocator_buddies_system::buddy_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_buddies_system::buddy_iterator &allocator_buddies_system::buddy_iterator::operator++() & noexcept
{
    if (_block == nullptr) return *this;

    _block = forward(_block);
    return *this;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::buddy_iterator::operator++(int n)
{
    const auto tmp = *this;
    ++*this;
    return tmp;
}

size_t allocator_buddies_system::buddy_iterator::size() const noexcept
{
    if (!_block) return 0;
    return block_size(_block);
}

bool allocator_buddies_system::buddy_iterator::occupied() const noexcept
{
    if (!_block) return false;
    return reinterpret_cast<block_metadata*>(_block)->occupied;
}

void *allocator_buddies_system::buddy_iterator::operator*() const noexcept
{
    return _block;
}

allocator_buddies_system::buddy_iterator::buddy_iterator(void *start): _block(start) {}

allocator_buddies_system::buddy_iterator::buddy_iterator(): _block(nullptr) {}