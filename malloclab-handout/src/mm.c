/*
 *  I implemented
 *  Seglist Allocators described in the ppt 
 *  
 *  - each size class has its own free list
 *  - blocks in size class are ordered by size in ascending order
 *  - one class for each two-power size 
 *  - {1},{2},{3,4},{5-8}, ..., {1025-2048}, ...
 * 
 *
 */

/* ALLOCATED BLOCK, FREE BLOCK, SEGREGATED FREE LIST, HEAP STRUCTURE

 1. Allocated Block 
 
             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    bp ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | 1|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                                                                                               |
            |                                                                                               |
            .                              Payload and padding                                              .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | 1|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 
 
 2. Free block 
 
             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | 0|
      bp--> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to prev block in explicit segregated free list                 |
    bp+4--> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to next block in explicit segregated free list                 |
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | 0|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

 3. Segregated Free Lists 
    +--+--+--+--+--+--+
    |     |     |     |   . . .
    +--+--+--+--+--+--+
       |     |     |
    +--+--+
    |     | ...   ...
    +--+--+ 
       |
    +--+--+
    |     | 
    +--+--+ 

 4. Heap 
              31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
Start of heap +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               0 (Padding)                                            |  |  |  |
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               8 (Prologue block)                                     |  |  | 1|
*heap_listp ->+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               8 (Prologue block)                                     |  |  | 1|
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              .                                                                                               .
              .                                                                                               .
              .                                                                                               .
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                              0 (Epilogue block hdr)                                  |  |  | 1|
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8


/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros*/
#define WSIZE 4 /*Word and header/footer size (bytes) */
#define DSIZE 8 /*Double word size (bytes) */
#define INITCHUNKSIZE (1<<6)
#define CHUNKSIZE (1<<12) /*Extend heap by this amount(bytes)*/

#define MAXNUMBER     20
#define REALLOC_BUFFER  (1<<7)

#define MAX(x,y) ((x) > (y) ? (x) : (y) )
#define MIN(x, y) ((x) < (y) ? (x) : (y))


/*Pack a size and allocated bit into a word*/
#define PACK(size, alloc) ((size) | (alloc))

/*Read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT1(p, val)       (*(unsigned int *)(p) = (val) | GET_RA(p))
#define PUT0(p, val) (*(unsigned int *)(p) = (val))

// Store predecessor or successor pointer for free blocks
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/*Read the size and allocated fields from address p*/
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_RA(p)   (GET(p) & 0x2)
#define SET_RA(p)   (GET(p) |= 0x2)
#define DELETE_RA(p) (GET(p) &= ~0x2)


/*Given block ptr bp, compute address of its header and footer*/
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

/*Given block ptr bp, compute address(bp) of next and previous blocks*/
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// Address of free block's predecessor and successor entries
#define PRED_PTR(bp) ((char *)(bp))
#define SUCC_PTR(bp) ((char *)(bp) + WSIZE)

// Address of free block's predecessor and successor on the segregated list
#define PRED(bp) (*(char **)(bp))
#define SUCC(bp) (*(char **)(SUCC_PTR(bp)))


// End of my additional macros

// Define this so later when we move to store the list in heap,
// we can just change this function
#define GET_FREE_LIST_PTR(i) (*(free_lists+i))
#define SET_FREE_LIST_PTR(i, bp) (*(free_lists+i) = bp)

/*Global variables*/
static char *heap_listp = 0; /*Pointer to first block*/
// Global var
void *segregated_free_lists[MAXNUMBER];
static char **free_lists;

/*helper functions*/
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *place(void *bp, size_t asize);
static void add(void *bp, size_t size);
static void delete(void *bp);


//////////////////////////////////////// Helper functions //////////////////////////////////////////////////////////
static void *extend_heap(size_t size)
{
    void *bp;
    size_t asize;                // Adjusted size
    
    asize = ALIGN(size);
    
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    
    // Set headers and footer
    PUT0(HDRP(bp), PACK(asize, 0));
    PUT0(FTRP(bp), PACK(asize, 0));
    PUT0(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    add(bp, asize);
    
    return coalesce(bp);
}

static void add(void *bp, size_t size) {
    int list = 0;
    void *search_bp = bp;
    void *insert_bp = NULL;
    
    // Select segregated list
    while ((list < MAXNUMBER - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }
    
    // Keep size ascending order and search
    //search_bp = segregated_free_lists[list];
    search_bp = GET_FREE_LIST_PTR(list);
    while ((search_bp != NULL) && (size > GET_SIZE(HDRP(search_bp)))) {
        insert_bp = search_bp;
        search_bp = PRED(search_bp);
    }
    
    // Set predecessor and successor
    if (search_bp != NULL) {
        if (insert_bp != NULL) {
            SET_PTR(PRED_PTR(bp), search_bp);
            SET_PTR(SUCC_PTR(search_bp), bp);
            SET_PTR(SUCC_PTR(bp), insert_bp);
            SET_PTR(PRED_PTR(insert_bp), bp);
        } else {
            SET_PTR(PRED_PTR(bp), search_bp);
            SET_PTR(SUCC_PTR(search_bp), bp);
            SET_PTR(SUCC_PTR(bp), NULL);
            //segregated_free_lists[list] = bp;
            SET_FREE_LIST_PTR(list, bp);
        }
    } else {
        if (insert_bp != NULL) {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), insert_bp);
            SET_PTR(PRED_PTR(insert_bp), bp);
        } else {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), NULL);
            //segregated_free_lists[list] = bp;
            SET_FREE_LIST_PTR(list, bp);
        }
    }
    
    return;
}


static void delete(void *bp) {
    int list = 0;
    size_t size = GET_SIZE(HDRP(bp));
    
    // Select segregated list
    while ((list < MAXNUMBER - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }
    
    if (PRED(bp) != NULL) {
        if (SUCC(bp) != NULL) {
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
        } else {
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
            //segregated_free_lists[list] = PRED(bp);
            SET_FREE_LIST_PTR(list, PRED(bp));
        }
    } else {
        if (SUCC(bp) != NULL) {
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
        } else {
            //segregated_free_lists[list] = NULL;
            SET_FREE_LIST_PTR(list, PRED(bp));
        }
    }
    
    return;
}


static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    
    // Do not coalesce with previous block if the previous block is tagged with Reallocation tag
    if (GET_RA(HDRP(PREV_BLKP(bp))))
        prev_alloc = 1;
    
    if (prev_alloc && next_alloc) {                         // Case 1
        return bp;
    }
    else if (prev_alloc && !next_alloc) {                   // Case 2
        delete(bp);
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT1(HDRP(bp), PACK(size, 0));
        PUT1(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {                 // Case 3
        delete(bp);
        delete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT1(FTRP(bp), PACK(size, 0));
        PUT1(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                                // Case 4
        delete(bp);
        delete(PREV_BLKP(bp));
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT1(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT1(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    add(bp, size);
    
    return bp;
}

static void *place(void *bp, size_t asize)
{
    size_t bp_size = GET_SIZE(HDRP(bp));
    size_t remainder = bp_size - asize;
    
    delete(bp);
    
    
    if (remainder <= DSIZE * 2) {
        // Do not split block
        PUT1(HDRP(bp), PACK(bp_size, 1));
        PUT1(FTRP(bp), PACK(bp_size, 1));
    }
    
    else if (asize >= 100) {
        // Split block
        PUT1(HDRP(bp), PACK(remainder, 0));
        PUT1(FTRP(bp), PACK(remainder, 0));
        PUT0(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
        PUT0(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
        add(bp, remainder);
        return NEXT_BLKP(bp);
        
    }
    
    else {
        // Split block
        PUT1(HDRP(bp), PACK(asize, 1));
        PUT1(FTRP(bp), PACK(asize, 1));
        PUT0(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        PUT0(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        add(NEXT_BLKP(bp), remainder);
    }
    return bp;
}



//////////////////////////////////////// End of Helper functions ////////////////////////////////////////

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int list;
    char *heap_start; // Pointer to beginning of heap

    if ((long)(free_lists = mem_sbrk(MAXNUMBER*sizeof(char *))) == -1)
		return -1;
    
     // Initialize the free list
    for (int i = 0; i <= MAXNUMBER; i++) {
	    SET_FREE_LIST_PTR(i, NULL);
    }

    // Initialize segregated free lists
    /*for (list = 0; list < MAXNUMBER; list++) {
        segregated_free_lists[list] = NULL;
    }*/
    
    // Allocate memory for the initial empty heap
    if ((long)(/*heap_start*/ heap_listp = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    PUT0(heap_listp, 0);                            /* Alignment padding */
    PUT0(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT0(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT0(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    
    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;
    
    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 * * mm_malloc - Allocate a block with at least size bytes of payload 
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    void *bp = NULL;  /* Pointer */
    
    // Ignore size 0 cases
    if (size == 0)
        return NULL;
    
    // Align block size
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = ALIGN(size+DSIZE);
    }
    
    int list = 0;
    size_t searchsize = asize;
    // Search for free block in segregated list
    while (list < MAXNUMBER) {
        if ((list == MAXNUMBER - 1) || ((searchsize <= 1) && 
        (/*segregated_free_lists[list]*/ GET_FREE_LIST_PTR(list)!= NULL))) {
            //bp = segregated_free_lists[list];
            bp = GET_FREE_LIST_PTR(list);
            // Ignore blocks that are too small or marked with the reallocation bit
            while ((bp != NULL) && ((asize > GET_SIZE(HDRP(bp))) || (GET_RA(HDRP(bp)))))
            {
                bp = PRED(bp);
            }
            if (bp != NULL)
                break;
        }
        
        searchsize >>= 1;
        list++;
    }
    
    // if free block is not found, extend the heap
    if (bp == NULL) {
        extendsize = MAX(asize, CHUNKSIZE);
        
        if ((bp = extend_heap(extendsize)) == NULL)
            return NULL;
    }
    
    // Place and divide block
    bp = place(bp, asize);
    
    
    // Return pointer to newly allocated block
    return bp;
}




/*
 * mm_free - Freeing a block (does nothing.)
 */
void mm_free(void *bp)
{
   size_t size = GET_SIZE(HDRP(bp));
    
    DELETE_RA(HDRP(NEXT_BLKP(bp)));
    PUT1(HDRP(bp), PACK(size, 0));
    PUT1(FTRP(bp), PACK(size, 0));
    
    add(bp, size);
    coalesce(bp);
    
    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * 
 * Naive implementation of realloc 
 */
void *mm_realloc(void *bp, size_t size)
{   
    void *new_bp = bp;    /* Pointer to be returned */
    size_t new_size = size; /* Size of new block */
    int remainder;          /* Adequacy of block sizes */
    int extendsize;         /* Size of heap extension */
    int block_buffer;       /* Size of block buffer */
    
    // Ignore size 0 cases
    if (size == 0)
        return NULL;
    
    // Align block size
    if (new_size <= DSIZE) {
        new_size = 2 * DSIZE;
    } else {
        new_size = ALIGN(size+DSIZE);
    }
    
    /* Add overhead requirements to block size */
    new_size += REALLOC_BUFFER;
    
    /* Calculate block buffer */
    block_buffer = GET_SIZE(HDRP(bp)) - new_size;
    
    /* Allocate more space if overhead falls below the minimum */
    if (block_buffer < 0) {
        /* Check if next block is a free block or the epilogue block */
        if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp)))) {
            remainder = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - new_size;
            if (remainder < 0) {
                extendsize = MAX(-remainder, CHUNKSIZE);
                if (extend_heap(extendsize) == NULL)
                    return NULL;
                remainder += extendsize;
            }
            
            delete(NEXT_BLKP(bp));
            
            // Do not split block
            PUT0(HDRP(bp), PACK(new_size + remainder, 1));
            PUT0(FTRP(bp), PACK(new_size + remainder, 1));
        } else {
            new_bp = mm_malloc(new_size - DSIZE);
            memcpy(new_bp, bp, MIN(size, new_size));
            mm_free(bp);
        }
        block_buffer = GET_SIZE(HDRP(new_bp)) - new_size;
    }
    
    // Tag the next block if block overhead drops below twice the overhead
    if (block_buffer < 2 * REALLOC_BUFFER)
        SET_RA(HDRP(NEXT_BLKP(new_bp)));
    
    // Return the reallocated block
    return new_bp;
}













