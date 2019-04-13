/*
 * We use explitict segregated free lists with rounding to upper power of 2 as the class equivalence condition
 * Blocks within each class are sorted based on size in descending order
 *
 * Format of allocated block and free block are shown below
 ///////////////////////////////// Block information /////////////////////////////////////////////////////////
 /*
 A   : Allocated? (1: true, 0:false)
 < Allocated Block >
 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 bp --->     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | A|
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |                                                                                               |
 |                                                                                               |
 .                              Payload                                                            .
 .                                                                                               .
 .                                                                                               .
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | A|
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 < Free block >
 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 bp --->    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | A|
 bp+4 --->  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |                        pointer to its predecessor in Segregated list                          |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |                        pointer to its successor in Segregated list                            |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 .                                                                                               .
 .                                                                                               .
 .                                                                                               .
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | A|
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 ///////////////////////////////// End of Block information /////////////////////////////////////////////////////////
 // This visual text-based description is taken from: https://github.com/mightydeveloper/Malloc-Lab/blob/master/mm.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "qhv200+vr697",
    /* First member's full name */
    "Vasily Rudchenko",
    /* First member's email address */
    "vr697@nyu.edu",
    /* Second member's full name (leave blank if none) */
    "Quan Vuong",
    /* Second member's email address (leave blank if none) */
    "qhv200@nyu.edu"
};

#define MAX_POWER 50
#define TAKEN 1
#define FREE 0

#define WORD_SIZE 4 /* bytes */
#define D_WORD_SIZE 8
#define CHUNK ((1<<12)/WORD_SIZE) /* extend heap by this amount (words) */
#define STATUS_BIT_SIZE 3 // bits
#define HDR_FTR_SIZE 2 // in words
#define HDR_SIZE 1 // in words
#define FTR_SIZE 1 // in words
#define PRED_FIELD_SIZE 1 // in words
#define EPILOG_SIZE 2 // in words

// Read and write a word at address p
#define GET_BYTE(p) (*(char *)(p))
#define GET_WORD(p) (*(unsigned int *)(p))
#define PUT_WORD(p, val) (*(char **)(p) = (val))

// Get a bit mask where the lowest size bit is set to 1
#define GET_MASK(size) ((1 << size) - 1)

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

// Pack a size and allocated bit into a BIT_word
#define PACK(size, status) ((size<<STATUS_BIT_SIZE) | (status))

/* Round up to even */
#define EVENIZE(x) ((x + 1) & ~1)

// Read the size and allocation bit from address p
#define GET_SIZE(p)  ((GET_WORD(p) & ~GET_MASK(STATUS_BIT_SIZE)) >> STATUS_BIT_SIZE)
#define GET_STATUS(p) (GET_WORD(p) & 0x1)

// Address of block's footer
// Take in a pointer that points to the header
#define FTRP(header_p) ((char **)(header_p) + GET_SIZE(header_p) + HDR_SIZE)

// Get total size of a block
// Size indicates the size of the free space in a block
// Total size = size + size_of_header + size_of_footer = size + D_WORD_SIZE
// p must point to a header
#define GET_TOTAL_SIZE(p) (GET_SIZE(p) + HDR_FTR_SIZE)

// Define this so later when we move to store the list in heap,
// we can just change this function
#define GET_FREE_LIST_PTR(i) (*(free_lists+i))
#define SET_FREE_LIST_PTR(i, ptr) (*(free_lists+i) = ptr)

// Set pred or succ for free blocks
#define SET_PTR(p, ptr) (*(char **)(p) = (char *)(ptr))

// Get pointer to the word containing the address of pred and succ for a free block
// ptr should point to the start of the header
#define GET_PTR_PRED_FIELD(ptr) ((char **)(ptr) + HDR_SIZE)
#define GET_PTR_SUCC_FIELD(ptr) ((char **)(ptr) + HDR_SIZE + PRED_FIELD_SIZE)

// Get the pointer that points to the succ of a free block
// ptr should point to the header of the free block
#define GET_PRED(bp) (*(GET_PTR_PRED_FIELD(bp)))
#define GET_SUCC(bp) (*(GET_PTR_SUCC_FIELD(bp)))

// Given pointer to current block, return pointer to header of previous block
#define PREV_BLOCK_IN_HEAP(header_p) ((char **)(header_p) - GET_TOTAL_SIZE((char **)(header_p) - FTR_SIZE))

// Given pointer to current block, return pointer to header of next block
#define NEXT_BLOCK_IN_HEAP(header_p) (FTRP(header_p) + FTR_SIZE)

// Global variables
static char **free_lists;
static char **heap_ptr;

// Function Declarations
static size_t find_free_list_index(size_t words);

static void *extend_heap(size_t words);

static void *coalesce(void *bp);
static void *find_free_block(size_t words);
static void alloc_free_block(void *bp, size_t words);
static void place_block_into_free_list(char **bp);
static void remove_block_from_free_list(char **bp);
void *mm_realloc_wrapped(void *ptr, size_t size, int buffer_size);
static int round_up_power_2(int x);

int mm_check();

/// Round up to next higher power of 2 (return x if it's already a power
/// of 2).
/// Borrowed from http://stackoverflow.com/questions/364985/algorithm-for-finding-the-smallest-power-of-two-thats-greater-or-equal-to-a-giv
static int round_up_power_2 (int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

/*
 Find the index of the free list which given size belongs to.
 Returns index.
 Index can be from 0 to MAX_POWER.
 */
static size_t find_free_list_index(size_t words) {
    int index = 0;
    
    while ((index <= MAX_POWER) && (words > 1))
    {
        words >>= 1;
        index++;
    }
    
    return index;
}

/*
 The function combines the current block in
 physical memory with neigboring free blocks.
 Returns the pointer to the beginning of this
 new free block.
 Coalesce is only called on a block that is not in the free list.
 As such, coalesce does not set pointer values.
 */
static void *coalesce(void *bp) {
    char **prev_block = PREV_BLOCK_IN_HEAP(bp);
    char **next_block = NEXT_BLOCK_IN_HEAP(bp);
    size_t prev_status = GET_STATUS(prev_block);
    size_t next_status = GET_STATUS(next_block);
    size_t new_size = GET_SIZE(bp);
    
    if (prev_status == TAKEN && next_status == TAKEN) {
        return bp;
    } else if (prev_status == TAKEN && next_status == FREE) {
        remove_block_from_free_list(next_block);
        new_size += GET_TOTAL_SIZE(next_block);
        
        PUT_WORD(bp, PACK(new_size, FREE));
        PUT_WORD(FTRP(next_block), PACK(new_size, FREE));
    } else if (prev_status == FREE && next_status == TAKEN) {
        remove_block_from_free_list(prev_block);
        new_size += GET_TOTAL_SIZE(prev_block);
        
        PUT_WORD(prev_block, PACK(new_size, FREE));
        PUT_WORD(FTRP(bp), PACK(new_size, FREE));
        bp = prev_block;
    } else if (prev_status == FREE && next_status == FREE) {
        remove_block_from_free_list(prev_block);
        remove_block_from_free_list(next_block);
        new_size += GET_TOTAL_SIZE(prev_block) + GET_TOTAL_SIZE(next_block);
        
        PUT_WORD(prev_block, PACK(new_size, FREE));
        PUT_WORD(FTRP(next_block), PACK(new_size, FREE));
        bp = prev_block;
    }
    
    return bp;
}

/*
 Relies on mem_sbrk to create a new free block.
 Does not coalesce.
 Does not place into free list.
 Returns pointer to the new block of memory with
 header and footer already defined.
 Returns NULL if we ran out of physical memory.
 */
static void *extend_heap(size_t words) {
    char **bp; // pointer to the free block formed by extending memory
    char **end_pointer; // pointer to the end of the free block
    size_t words_extend = EVENIZE(words); // make sure double aligned
    size_t words_extend_tot = words_extend + HDR_FTR_SIZE; // add header and footer
    
    // extend memory by so many words
    // multiply words by WORD_SIZE because mem_sbrk takes input as bytes
    if ((long)(bp = mem_sbrk((words_extend_tot) * WORD_SIZE)) == -1) {
        return NULL;
    }
    
    // offset to make use of old epilog and add space for new epilog
    bp -= EPILOG_SIZE;
    
    // set new block header/footer to size (in words)
    PUT_WORD(bp, PACK(words_extend, FREE));
    PUT_WORD(FTRP(bp), PACK(words_extend, FREE));
    
    // add epilog to the end
    end_pointer = bp + words_extend_tot;
    PUT_WORD(end_pointer, PACK(0, TAKEN));
    PUT_WORD(FTRP(end_pointer), PACK(0, TAKEN));
    
    return bp;
}

/*
 Finds the block from the free lists that is large
 enough to hold the amount of words specified.
 Returns the pointer to that block.
 Does not take the block out of the free list.
 Does not extend heap.
 Returns the pointer to the block.
 Returns NULL if block large enough is not found.
 */
static void *find_free_block(size_t words) {
    char **bp;
    size_t index = find_free_list_index(words);
    
    // check if first free list can contain large enough block
    if ((bp = GET_FREE_LIST_PTR(index)) != NULL && GET_SIZE(bp) >= words) {
        // iterate through blocks
        while(1) {
            // if block is of exact size, return right away
            if (GET_SIZE(bp) == words) {
                return bp;
            }
            
            // if next block is not possible, return current one
            if (GET_SUCC(bp) == NULL || GET_SIZE(GET_SUCC(bp)) < words) {
                return bp;
            } else {
                bp = GET_SUCC(bp);
            }
        }
    }
    
    // move on from current free list
    index++;
    
    // find a large enough non-empty free list
    while (GET_FREE_LIST_PTR(index) == NULL && index < MAX_POWER) {
        index++;
    }
    
    // if there is a non-NULL free list, go until the smallest block in free list
    if ((bp = GET_FREE_LIST_PTR(index)) != NULL) {
        while (GET_SUCC(bp) != NULL) {
            bp = GET_SUCC(bp);
        }
        
        return bp;
    } else { // if no large enough free list available, return NULL
        return NULL;
    }
}

/*
 The function takes free block and changes status to taken.
 The free block is assumed to have been removed from free list.
 The function reduces the size of the free block (splits it) if size is too large.
 Too large is a free block whose size is > needed size + HDR_FTR_SIZE
 The remaining size is either placed in free_list or left hanging if it is > 0.
 If remaining size is 0 it becomes part of the allocated block.
 bp input is the block that you found already that is large enough.
 Assume that size in words given is <= the size of the block at input.
 */
static void alloc_free_block(void *bp, size_t words) {
    size_t bp_size = GET_SIZE(bp);
    size_t bp_tot_size = bp_size + HDR_FTR_SIZE;
    
    size_t needed_size = words;
    size_t needed_tot_size = words + HDR_FTR_SIZE;
    
    int new_block_tot_size = bp_tot_size - needed_tot_size;
    int new_block_size = new_block_tot_size - HDR_FTR_SIZE;
    
    // the block created from extra free space
    char **new_block;
    
    // if size of block is larger than needed size, split the block
    // handle new block by making it part of the free block ecosystem
    if ((int)new_block_size > 0) {
        // set new block pointer at offset from start of bp
        new_block = (char **)(bp) + needed_tot_size;
        
        // set new block's size and status
        PUT_WORD(new_block, PACK(new_block_size, FREE));
        PUT_WORD(FTRP(new_block), PACK(new_block_size, FREE));
        
        // set bp size to exact needed size
        PUT_WORD(bp, PACK(needed_size, TAKEN));
        PUT_WORD(FTRP(bp), PACK(needed_size, TAKEN));
        
        // check if new block can become larger than it is
        new_block = coalesce(new_block);
        
        // handle this new block by putting back into free list
        place_block_into_free_list(new_block);
    } else if (new_block_size == 0) {
        // if the new_block_size is zero there is no point in separating the blocks
        // thus the extra two words are just kept as part of the allocated block
        needed_size += HDR_FTR_SIZE;
        
        PUT_WORD(bp, PACK(needed_size, TAKEN));
        PUT_WORD(FTRP(bp), PACK(needed_size, TAKEN));
    } else {
        // if exact size just change status
        PUT_WORD(bp, PACK(needed_size, TAKEN));
        PUT_WORD(FTRP(bp), PACK(needed_size, TAKEN));
    }
}

/*
 Removes a block from the free list if block size is larger than zero.
 Does nothing if it is zero.
 Does not return the pointer to that block.
 */
static void remove_block_from_free_list(char **bp) {
    char **prev_block = GET_PRED(bp);
    char **next_block = GET_SUCC(bp);
    int index;
    
    if (GET_SIZE(bp) == 0) {
        return;
    }
    
    // if largest block in free list set free list to next ptr
    if (prev_block == NULL) {
        index = find_free_list_index(GET_SIZE(bp));
        GET_FREE_LIST_PTR(index) = next_block;
    } else { // if not largest block update pointer for prev block to next ptr
        SET_PTR(GET_PTR_SUCC_FIELD(prev_block), next_block);
    }
    
    // next_block is not NULL, update the block to point to prev block
    if (next_block != NULL) {
        SET_PTR(GET_PTR_PRED_FIELD(next_block), prev_block);
    }
    
    // clear current block's pointers
    SET_PTR(GET_PTR_PRED_FIELD(bp), NULL);
    SET_PTR(GET_PTR_SUCC_FIELD(bp), NULL);
}

/*
 Places the block into the free list based on block size.
 Keeps free list sorted.
 */
static void place_block_into_free_list(char **bp) {
    size_t size = GET_SIZE(bp);
    int index = find_free_list_index(size);
    
    char **front_ptr = GET_FREE_LIST_PTR(index);
    char **back_ptr = NULL;
    
    // If the block size is zero than it doesn't belong in the free list
    // because it doesn't have enough space for pointers
    if (size == 0) {
        return;
    }
    
    // If the free list is empty
    if (front_ptr == NULL)
    {
        SET_PTR(GET_PTR_SUCC_FIELD(bp), NULL);
        SET_PTR(GET_PTR_PRED_FIELD(bp), NULL);
        SET_FREE_LIST_PTR(index, bp);
        return;
    }
    
    // If the new block is the biggest in the respective free list
    if (size >= GET_SIZE(front_ptr))
    {
        SET_FREE_LIST_PTR(index, bp);
        SET_PTR(GET_PTR_SUCC_FIELD(bp), front_ptr);
        SET_PTR(GET_PTR_PRED_FIELD(front_ptr), bp);
        SET_PTR(GET_PTR_PRED_FIELD(bp), NULL);
        return;
    }
    
    // Keep each free list sorted in descending order of size
    while (front_ptr != NULL && GET_SIZE(front_ptr) > size)
    {
        back_ptr = front_ptr;
        front_ptr = GET_SUCC(front_ptr);
    }
    
    // Reached the end of the free list
    if (front_ptr == NULL)
    {
        SET_PTR(GET_PTR_SUCC_FIELD(back_ptr), bp);
        SET_PTR(GET_PTR_PRED_FIELD(bp), back_ptr);
        SET_PTR(GET_PTR_SUCC_FIELD(bp), NULL);
        return;
    }
    else
    { // Haven't reached the end of the free list
        SET_PTR(GET_PTR_SUCC_FIELD(back_ptr), bp);
        SET_PTR(GET_PTR_PRED_FIELD(bp), back_ptr);
        SET_PTR(GET_PTR_SUCC_FIELD(bp), front_ptr);
        SET_PTR(GET_PTR_PRED_FIELD(front_ptr), bp);
        return;
    }
    
}

/*
 * mm_init - initialize the malloc package.
 Sets the free list to beginning of heap.
 Returns -1 on error.
 Sets initial epilog at the end.
 */
int mm_init(void)
{
    // Store the pointer to the free list on the heap
    int even_max_power = EVENIZE(MAX_POWER); // Maintain alignment
    if ((long)(free_lists = mem_sbrk(even_max_power*sizeof(char *))) == -1)
        return -1;
    
    // Initialize the free list
    for (int i = 0; i <= MAX_POWER; i++) {
        SET_FREE_LIST_PTR(i, NULL);
    }
    
    // align to double word
    mem_sbrk(WORD_SIZE);
    
    if ((long)(heap_ptr = mem_sbrk(4*WORD_SIZE)) == -1) // 2 for prolog, 2 for epilog
        return -1;
    
    PUT_WORD(heap_ptr, PACK(0, TAKEN)); // Prolog header
    PUT_WORD(FTRP(heap_ptr), PACK(0, TAKEN)); // Prolog footer
    
    char ** epilog = NEXT_BLOCK_IN_HEAP(heap_ptr);
    PUT_WORD(epilog, PACK(0, TAKEN)); // Epilog header
    PUT_WORD(FTRP(epilog), PACK(0, TAKEN)); // Epilog footer
    
    heap_ptr += HDR_FTR_SIZE; // Move past prolog
    
    char **new_block;
    if ((new_block = extend_heap(CHUNK)) == NULL)
        return -1;
    
    // need to place into free list because extend_heap does not place it
    place_block_into_free_list(new_block);
    
    return 0;
}

/*
 Input is in bytes
 Returns NULL on no memory or on size 0.
 Uses blocks in free list as free blocks.
 Returns pointer to block content.
 */
void *mm_malloc(size_t size)
{
    if (size <= 1<<12) {
        size = round_up_power_2(size);
    }
    
    size_t words = ALIGN(size) / WORD_SIZE;
    
    size_t extend_size;
    char **bp;
    
    if (size == 0) {
        return NULL;
    }
    
    // check if there is a block that is large enough
    // if not, extend the heap
    if ((bp = find_free_block(words)) == NULL) {
        extend_size = words > CHUNK ? words : CHUNK;
        
        if ((bp = extend_heap(extend_size)) == NULL) {
            return NULL;
        }
        
        // do not remove block from free list because it is not in it
        alloc_free_block(bp, words);
        
        return bp + HDR_SIZE;
    }
    
    remove_block_from_free_list(bp);
    alloc_free_block(bp, words);
    
    return bp + HDR_SIZE;
}

/*
 * mm_free
 * Role:
 - change the status of block to free
 - coalesce the block
 - place block into free_lists
 * Assume: ptr points to the beginning of a block header
 */
void mm_free(void *ptr)
{
    ptr -= WORD_SIZE;
    
    size_t size = GET_SIZE(ptr);
    
    PUT_WORD(ptr, PACK(size, FREE));
    PUT_WORD(FTRP(ptr), PACK(size, FREE));
    
    ptr = coalesce(ptr);
    
    place_block_into_free_list(ptr);
}

int round_to_thousand(size_t x)
{
    return x % 1000 >= 500 ? x + 1000 - x % 1000 : x - x % 1000;
}

// Calculate the diff between previous request size and current request
// Determine the buffer size of the newly reallocated block based on this diff
// Call mm_realloc_wrapped to perform the actual reallocation
void *mm_realloc(void *ptr, size_t size)
{
    static int previous_size;
    int buffer_size;
    int diff = abs(size - previous_size);
    
    if (diff < 1<<12 && diff % round_up_power_2(diff)) {
        buffer_size = round_up_power_2(diff);
    } else {
        buffer_size = round_to_thousand(size);
    }
    
    void * return_value = mm_realloc_wrapped(ptr, size, buffer_size);
    
    previous_size = size;
    return return_value;
}

// Realloc a block
/*
 mm_realloc:
 if the pointer given is NULL, behaves as malloc would
 if the size given is zero, behaves as free would
 As an optamizing, checks if it is possible to use neighboring blocks
 and coalesce so as to avoid allocating new blocks.
 If that is not possible, simple reallocates based on alloc and free.
 Uses buffer to not have to reallocate often.
 */
void *mm_realloc_wrapped(void *ptr, size_t size, int buffer_size)
{
    
    // equivalent to mm_malloc if ptr is NULL
    if (ptr == NULL) {
        return mm_malloc(ptr);
    }
    
    // adjust to be at start of block
    char **old = (char **)ptr - 1;
    char **bp = (char **)ptr - 1;
    
    // get intended and current size
    size_t new_size = ALIGN(size) / WORD_SIZE; // in words
    size_t size_with_buffer = new_size + buffer_size;
    size_t old_size = GET_SIZE(bp); // in words
    
    if (size_with_buffer == old_size && new_size <= size_with_buffer) {
        return bp + HDR_SIZE;
    }
    
    if (new_size == 0) {
        mm_free(ptr);
        return NULL;
    } else if (new_size > old_size) {
        if (GET_SIZE(NEXT_BLOCK_IN_HEAP(bp)) + old_size + 2 >= size_with_buffer &&
            GET_STATUS(PREV_BLOCK_IN_HEAP(bp)) == TAKEN &&
            GET_STATUS(NEXT_BLOCK_IN_HEAP(bp)) == FREE
            ) { // checks if possible to merge with previous block in memory
            PUT_WORD(bp, PACK(old_size, FREE));
            PUT_WORD(FTRP(bp), PACK(old_size, FREE));
            
            bp = coalesce(bp);
            alloc_free_block(bp, size_with_buffer);
        } else if (GET_SIZE(PREV_BLOCK_IN_HEAP(bp)) + old_size + 2 >= size_with_buffer &&
                   GET_STATUS(PREV_BLOCK_IN_HEAP(bp)) == FREE &&
                   GET_STATUS(NEXT_BLOCK_IN_HEAP(bp)) == TAKEN
                   ) { // checks if possible to merge with next block in memory
            PUT_WORD(bp, PACK(old_size, FREE));
            PUT_WORD(FTRP(bp), PACK(old_size, FREE));
            
            bp = coalesce(bp);
            
            memmove(bp + 1, old + 1, old_size * WORD_SIZE);
            alloc_free_block(bp, size_with_buffer);
        } else if (GET_SIZE(PREV_BLOCK_IN_HEAP(bp)) + GET_SIZE(NEXT_BLOCK_IN_HEAP(bp)) + old_size + 4 >= size_with_buffer &&
                   GET_STATUS(PREV_BLOCK_IN_HEAP(bp)) == FREE &&
                   GET_STATUS(NEXT_BLOCK_IN_HEAP(bp)) == FREE
                   ) { // checks if possible to merge with both prev and next block in memory
            PUT_WORD(bp, PACK(old_size, FREE));
            PUT_WORD(FTRP(bp), PACK(old_size, FREE));
            
            bp = coalesce(bp);
            
            memmove(bp + 1, old + 1, old_size * WORD_SIZE);
            alloc_free_block(bp, size_with_buffer);
        } else { // end case: if no optimization possible, just do brute force realloc
            bp = (char **)mm_malloc(size_with_buffer*WORD_SIZE + WORD_SIZE) - 1;
            
            if (bp == NULL) {
                return NULL;
            }
            
            memcpy(bp + 1, old + 1, old_size * WORD_SIZE);
            mm_free(old + 1);
        }
    }
    
    return bp + HDR_SIZE;
}

static void check_free_blocks_in_one_free_list_marked_free(char ** bp)
{
    while (bp) {
        if (GET_STATUS(bp) == TAKEN) {
            printf("There are free blocks that are marked as taken");
            assert(0);
        }
        bp = GET_SUCC(bp);
    }
}

static void check_free_blocks_marked_free()
{
    char ** bp;
    
    for (int i=0; i <= MAX_POWER; i++) {
        if (bp = GET_FREE_LIST_PTR(i)) {
            check_free_blocks_in_one_free_list_marked_free(bp);
        }
    }
    
    printf("check_free_blocks_marked_free passed.\n");
}

static void should_coalesce_with_next_block(char ** bp)
{
    char ** next_block = NEXT_BLOCK_IN_HEAP(bp);
    
    if (GET_STATUS(bp) == FREE && GET_STATUS(next_block) == FREE) {
        printf("Block %p should coalesce with block %p", bp, next_block);
        assert(0);
    }
}

static void check_contiguous_free_block_coalesced()
{
    char ** bp = heap_ptr;
    
    while (GET_STATUS(bp) != TAKEN && GET_SIZE(bp) != 0) { // Haven't hit epilog
        should_coalesce_with_next_block(bp);
        bp = NEXT_BLOCK_IN_HEAP(bp);
    }
    
    printf("check_contiguous_free_block_coalesced passed.\n");
}

static void is_valid_free_block(char ** bp)
{
    size_t size_in_hdr = GET_SIZE(bp);
    size_t size_in_ftr = GET_SIZE(FTRP(bp));
    
    // Check size in hdr == size in ftr
    if (size_in_hdr != size_in_ftr) {
        printf("Free block %p has different sizes in hdr and ftr", bp);
        assert(0);
    }
    
    // Check status in hdr and ftr
    if (GET_STATUS(bp) == TAKEN) {
        printf("Free block %p has status as taken in header", bp);
        assert(0);
    }
    
    if (GET_STATUS(FTRP(bp)) == TAKEN) {
        printf("Free block %p has status as taken in footer", bp);
        assert(0);
    }
}

static void should_be_in_free_list(char ** bp)
{
    int size = GET_SIZE(bp);
    int index = find_free_list_index(size);
    
    if (GET_FREE_LIST_PTR(index) == bp)
        return; // Bp at the beginning of a free list
    
    // Every block after the first block should have either a prev or next_block
    char **prev_block = GET_PRED(bp);
    char **next_block = GET_SUCC(bp);
    
    if (!prev_block && !next_block) {
        printf("Free block %p not in free list", bp);
        assert(0);
    }
}

static void check_all_free_blocks_in_free_list()
{
    char ** bp = heap_ptr;
    
    while (GET_STATUS(bp) != TAKEN && GET_SIZE(bp) != 0) { // Haven't hit epilog
        if (GET_STATUS(bp) == FREE) {
            should_be_in_free_list(bp);
        }
        bp = NEXT_BLOCK_IN_HEAP(bp);
    }
    
    printf("check_all_free_blocks_in_free_list passed.\n");
}

static void check_all_free_blocks_valid_ftr_hdr()
{
    char ** bp = heap_ptr;
    
    while (GET_STATUS(bp) != TAKEN && GET_SIZE(bp) != 0) { // Haven't hit epilog
        if (GET_STATUS(bp) == FREE) {
            is_valid_free_block(bp);
        }
        bp = NEXT_BLOCK_IN_HEAP(bp);
    }
    
    printf("check_all_free_blocks_valid_ftr_hdr passed.\n");
}

static void is_valid_heap_address(char ** bp, void * heap_lo, void * heap_hi)
{
    if (!(heap_lo <= bp <= heap_hi)) {
        printf("%x not in heap range", bp);
        assert(0);
    }
}

static void check_ptrs_valid_heap_address()
{
    void * heap_lo = mem_heap_lo();
    void * heap_hi = mem_heap_hi();
    
    char ** bp = heap_ptr;
    
    do {
        is_valid_heap_address(bp, heap_lo, heap_hi);
        bp = NEXT_BLOCK_IN_HEAP(bp);
    } while (GET_STATUS(bp) != TAKEN && GET_SIZE(bp) != 0); // Haven't hit epilog
    
    printf("check_ptrs_valid_heap_address passed.\n");
    
}

int mm_check()
{
    printf("RUNNING MM CHECK.\n");
    check_free_blocks_marked_free();
    check_contiguous_free_block_coalesced();
    check_all_free_blocks_in_free_list();
    check_all_free_blocks_valid_ftr_hdr();
    check_ptrs_valid_heap_address();
    // Do not check for overlapping block because mdriver helps enforce
    printf("MM CHECK FINISHED SUCCESSFUL.\n\n");
}

