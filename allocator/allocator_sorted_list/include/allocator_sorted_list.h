#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H

#include <pp_allocator.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <iterator>
#include <mutex>

class allocator_sorted_list final:
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode
{

private:

    void *_trusted_memory;

    static constexpr const size_t allocator_metadata_size = sizeof(std::pmr::memory_resource *) + sizeof(fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void*);

    static constexpr const size_t block_metadata_size = sizeof(void*) + sizeof(size_t);

//region Helpers
    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to parent allocator field
     */
    static inline std::pmr::memory_resource** parent_allocator(void* trusted) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to fit_mode field
     */
    static inline fit_mode* fit_mode(void* trusted) noexcept;

    /**
     *
     * @param b_m pointer to current free block metadata
     * @return pointer to next free block
     */
    static inline void** forward(void* b_m) noexcept;

    /**
     *
     * @param b_m pointer to occupied block metadata
     * @return pointer to _trusted_memory parent of block
     */
    static inline void** parent(void* b_m) noexcept;

    /**
     *
     * @param trusted pointer to start of metadata
     * @return pointer to size field
     */
    static inline size_t* space_size(void* trusted) noexcept;

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
    static inline size_t* block_size(void* b_m) noexcept;

    /**
     *
     * @return pointer to field with first free block
     */
    static inline void** free_first_block(void* trusted) noexcept;

    /**
     *
     * @return pointer to first block
     */
    static inline void* first_block(void* trusted) noexcept;

    /**
     *
     * @param block pointer to block data
     * @return pointer to block start
     */
    static void* block_metadata(void* block) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return pointer to block end
     */
    static std::byte* block_end(void* b_m) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return pointer to block data
     */
    static inline void* block_data(void* b_m) noexcept;
//endregion

public:

    explicit allocator_sorted_list(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);
    
    allocator_sorted_list(
        allocator_sorted_list const &other) = delete;
    
    allocator_sorted_list &operator=(
        allocator_sorted_list const &other) = delete;

    allocator_sorted_list(
        allocator_sorted_list &&other) noexcept;
    
    allocator_sorted_list &operator=(
        allocator_sorted_list &&other) noexcept;

    ~allocator_sorted_list() override;

private:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t size) override;
    
    void do_deallocate_sm(
        void *at) override;

    bool do_is_equal(const std::pmr::memory_resource&) const noexcept override;
    
    inline void set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) override;

    std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    class sorted_free_iterator
    {
        void* _free_ptr;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const sorted_free_iterator&) const noexcept;

        bool operator!=(const sorted_free_iterator&) const noexcept;

        sorted_free_iterator& operator++() & noexcept;

        sorted_free_iterator operator++(int n);

        size_t size() const noexcept;

        void* operator*() const noexcept;

        sorted_free_iterator();

        sorted_free_iterator(void* trusted);
    };

    class sorted_iterator
    {
        void* _free_ptr;
        void* _current_ptr;
        void* _trusted_memory;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const sorted_iterator&) const noexcept;

        bool operator!=(const sorted_iterator&) const noexcept;

        sorted_iterator& operator++() & noexcept;

        sorted_iterator operator++(int n);

        size_t size() const noexcept;

        void* operator*() const noexcept;

        bool occupied()const noexcept;

        sorted_iterator();

        sorted_iterator(void* trusted);
    };

    friend class sorted_iterator;
    friend class sorted_free_iterator;

    sorted_free_iterator free_begin() const noexcept;
    sorted_free_iterator free_end() const noexcept;

    sorted_iterator begin() const noexcept;
    sorted_iterator end() const noexcept;
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H