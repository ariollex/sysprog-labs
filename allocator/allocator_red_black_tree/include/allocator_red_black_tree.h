#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H

#include <pp_allocator.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <mutex>

class allocator_red_black_tree final:
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode
{

private:

    enum class block_color : unsigned char
    { RED, BLACK };

    struct block_data
    {
        bool occupied : 4;
        block_color color : 4;
    };

    void *_trusted_memory;

    static constexpr const size_t allocator_metadata_size = sizeof(allocator_dbg_helper*) + sizeof(fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void*);
    static constexpr const size_t occupied_block_metadata_size = sizeof(block_data) + 3 * sizeof(void*);
    static constexpr const size_t free_block_metadata_size = sizeof(block_data) + 5 * sizeof(void*);

// region helpers declaration
    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to parent allocator field
     */
    static inline memory_resource** parent_allocator(void* trusted) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to fit_mode field
     */
    static inline fit_mode* fit_mode(void* trusted) noexcept;


    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to root of Red-Black Tree
     */
    static inline std::byte** root(void* trusted) noexcept;

    /**
     *
     * @param b_m pointer to current block metadata
     * @return pointer to next block
     */
    static inline std::byte** next(void* b_m) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return pointer to prev block
     */
    static inline std::byte** prev(void* b_m) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return pointer to left node
     */
    static inline std::byte** left(void* b_m) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return pointer to right node
     */
    static inline std::byte** right(void* b_m) noexcept;

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
     * @param trusted pointer to trusted memory
     * @param b_m pointer to block metadata
     * @return block size
     */
    static inline size_t block_size(void* trusted, void* b_m) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return pointer to block data struct
     */
    static inline block_data* block_node(void* b_m) noexcept;

    /**
     *
     * @param block pointer to block data
     * @return pointer to block start
     */
    static inline std::byte* block_metadata(void* block) noexcept;

    /**
     *
     * @param b_m pointer to block metadata
     * @return size of block metadata
     */
    static inline size_t block_metadata_size(void* b_m) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to first block metadata
     */
    static inline std::byte* first_block(void* trusted) noexcept;

    /**
     *
     * @param trusted pointer to trusted memory
     * @return pointer to end of trusted memory
     */
    static inline std::byte* trusted_end(void* trusted) noexcept;

    /**
     *
     * @param b_m pointer to occupied block metadata
     * @return pointer to trusted memory parent of block if block occupied or pointer to node parent if block is free
     */
    static inline void** parent(void* b_m) noexcept;
// endregion helpers declaration

// region red-black tree declaration
    static inline bool is_red(void* node) noexcept;

    static inline bool is_black(void* node) noexcept;

    static bool is_left_child(void *node) noexcept;

    static bool is_right_child(void *node) noexcept;

    static std::byte *maximum(void *node) noexcept;

    char compare(void *u, void *v) const noexcept;

    void transplant(void* u, void* v) const noexcept;

    void rotate_right(void *y) const noexcept;

    void rotate_left(void *x) const noexcept;

    void rotate_big_right(void *y) const noexcept;

    void rotate_big_left(void *x) const noexcept;

    void swap_with_predecessor(void *node) const noexcept;

    void insert(void *new_node) const;

    void remove(void* node) const;
// endregion region red-black tree declaration
public:
    
    ~allocator_red_black_tree() override;
    
    allocator_red_black_tree(
        allocator_red_black_tree const &other) = delete;
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree const &other) = delete;
    
    allocator_red_black_tree(
        allocator_red_black_tree &&other) noexcept;
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree &&other) noexcept;

public:
    
    explicit allocator_red_black_tree(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

private:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t size) override;
    
    void do_deallocate_sm(
        void *at) override;

    bool do_is_equal(const std::pmr::memory_resource&) const noexcept override;

    std::vector<allocator_test_utils::block_info> get_blocks_info() const override;
    
    inline void set_fit_mode(allocator_with_fit_mode::fit_mode mode) override;

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    class rb_iterator
    {
        void* _block_ptr;
        void* _trusted;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const rb_iterator&) const noexcept;

        bool operator!=(const rb_iterator&) const noexcept;

        rb_iterator& operator++() & noexcept;

        rb_iterator operator++(int n);

        size_t size() const noexcept;

        void* operator*() const noexcept;

        bool occupied()const noexcept;

        rb_iterator();

        rb_iterator(void* trusted);
    };

    friend class rb_iterator;

    rb_iterator begin() const noexcept;
    rb_iterator end() const noexcept;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H