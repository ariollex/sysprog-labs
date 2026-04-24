#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H

#include <pp_allocator.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <mutex>
#include <cmath>

namespace __detail
{
    constexpr size_t nearest_greater_k_of_2(size_t size) noexcept
    {
        int ones_counter = 0, index = -1;

        constexpr const size_t o = 1;

        for (int i = sizeof(size_t) * 8 - 1; i >= 0; --i)
        {
            if (size & (o << i))
            {
                if (ones_counter == 0)
                    index = i;
                ++ones_counter;
            }
        }

        return ones_counter <= 1 ? index : index + 1;
    }
}

class allocator_buddies_system final:
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode
{

private:


    struct block_metadata
    {
        bool occupied : 1;
        unsigned char size : 7;
    };

    void *_trusted_memory;

    static constexpr const size_t allocator_metadata_size =
        (sizeof(allocator_dbg_helper*) + sizeof(fit_mode)+ sizeof(unsigned char) +
            alignof(std::mutex) - 1) / alignof(std::mutex) * alignof(std::mutex)
        + sizeof(std::mutex);

    static constexpr const size_t occupied_block_metadata_size =
        (sizeof(block_metadata) + alignof(void*) - 1) / alignof(void*) * alignof(void*)
        + sizeof(void*);

    static constexpr const size_t free_block_metadata_size = sizeof(block_metadata);

    static constexpr const size_t min_k = __detail::nearest_greater_k_of_2(occupied_block_metadata_size);

//region Helpers
    /**
     *
     * @param trusted pointer to trusted memory
     * @return k - coefficient for size
     */
    static inline unsigned char* k(void* trusted) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to parent allocator field
     */
    static inline memory_resource** parent_allocator(void* trusted) noexcept;

    /**
     *
     * @param b_m pointer to current block metadata
     * @return pointer to next block
     */
    static inline void* forward(void* b_m) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to fit_mode field
     */
    static inline fit_mode* fit_mode(void* trusted) noexcept;

    /**
     *
     * @param trusted pointer to start of metadata
     * @return pointer to size field
     */
    static inline size_t space_size(void* trusted) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to field with mutex
     */
    static std::mutex* mtx(void* trusted) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return block size
     */
    static inline size_t block_size(void* b_m) noexcept;

    /**
     *
     * @param b_m pointer to memory
     * @return pointer to block data
     */
    static void* block_data(void* b_m) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to first block metadata
     */
    static inline void* first_block(void* trusted) noexcept;

    /**
     *
     * @param b_m pointer to occupied block metadata
     * @return pointer to trusted memory parent of block
     */
    static inline void** parent(void* b_m) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to end of trusted memory
     */
    static inline void* trusted_end(void* trusted) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return pointer to block buddy
     */
    inline void* buddy(void* b_m) const noexcept;
//endregion

public:

    explicit allocator_buddies_system(
            size_t space_size_power_of_two,
            std::pmr::memory_resource *parent_allocator = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

    allocator_buddies_system(
        allocator_buddies_system const &other) = delete;
    
    allocator_buddies_system &operator=(
        allocator_buddies_system const &other) = delete;
    
    allocator_buddies_system(
        allocator_buddies_system &&other) noexcept;
    
    allocator_buddies_system &operator=(
        allocator_buddies_system &&other) noexcept;

    ~allocator_buddies_system() override;

private:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t size) override;
    
    void do_deallocate_sm(
        void *at) override;

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    inline void set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) override;


    std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    /** TODO: Highly recommended for helper functions to return references */

    class buddy_iterator
    {
        void* _block;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const buddy_iterator&) const noexcept;

        bool operator!=(const buddy_iterator&) const noexcept;

        buddy_iterator& operator++() & noexcept;

        buddy_iterator operator++(int n);

        size_t size() const noexcept;

        bool occupied() const noexcept;

        void* operator*() const noexcept;

        buddy_iterator();

        buddy_iterator(void* start);
    };

    friend class buddy_iterator;

    buddy_iterator begin() const noexcept;

    buddy_iterator end() const noexcept;
    
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
