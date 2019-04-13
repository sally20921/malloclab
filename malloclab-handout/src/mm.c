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

#define MAXNUMBER     16
#define REALLOC_BUFFER  (1<<7)

#define MAX(x,y) ((x) > (y) ? (x) : (y) )
#define MIN(x, y) ((x) < (y) ? (x) : (y))


/*Pack a size and allocated bit into a word*/
#define PACK(size, alloc) ((size) | (alloc))

/*Read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// Store predecessor or successor pointer for free blocks
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/*Read the size and allocated fields from address p*/
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

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
//void *segregated_free_lists[MAXNUMBER];
static char **free_lists;

/*helper functions*/
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *place(void *bp, size_t asize);
static void add(void *bp, size_t size);
static void delete(void *bp);

int mm_check();

int mm_check(){
    return 0;
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int i;

    //create space for keeping free list 
    if ((long)(free_lists = mem_sbrk(MAXNUMBER*sizeof(char *))) == -1){
        return -1;
    }
    
     // initialize the free list
    for (int i = 0; i <= MAXNUMBER; i++) {
	    SET_FREE_LIST_PTR(i, NULL);
    }

    // Allocate memory for the initial empty heap
    if ((long)(heap_listp = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE); 
    
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
    
    int i = 0;
    size_t searchsize = asize;
    // Search for free block in segregated list
    while (i < MAXNUMBER) {
        if ((i == MAXNUMBER - 1) || ((searchsize <= 1) && 
        (GET_FREE_LIST_PTR(i)!= NULL))) {
            bp = GET_FREE_LIST_PTR(i);
            // Ignore blocks that are too small or marked with the reallocation bit
            while ((bp != NULL) && (asize > GET_SIZE(HDRP(bp))))
            {
                bp = PRED(bp);
            }
            if (bp != NULL)
                break;
        }
        
        searchsize = searchsize >> 1;
        i++;
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
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
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
    void *new_block = bp;
    int remainder;
    if (size == 0)
        return NULL;
    if (size <= DSIZE)
    {
        size = 2 * DSIZE;
    }
    else
    {
        size = ALIGN(size + DSIZE);
    }
    if ((remainder = GET_SIZE(HDRP(bp)) - size) >= 0)
    {
        return bp;
    }
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp))))
    {
        if ((remainder = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - size) < 0)
        {
            if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL)
                return NULL;
            remainder += MAX(-remainder, CHUNKSIZE);
        }
        delete(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size + remainder, 1));
        PUT(FTRP(bp), PACK(size + remainder, 1));
    }
    else
    {
        new_block = mm_malloc(size);
        memcpy(new_block, bp, GET_SIZE(HDRP(bp)));
        mm_free(bp);
    }
    return new_block;
    
}

/*helper function*/
static void *extend_heap(size_t size)
{
    void *bp;
    size_t asize;                // Adjusted size
    
    asize = ALIGN(size);
    
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    
    // Set headers and footer
    PUT(HDRP(bp), PACK(asize, 0));
    PUT(FTRP(bp), PACK(asize, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    add(bp, asize);
    
    return coalesce(bp);
}

static void add(void *bp, size_t size) {
    int i = 0;
    void *curr = bp;
    void *succ = NULL;
    
    // Select segregated list
    while ((i < MAXNUMBER - 1) && (size > 1)) {
        size = size >> 1;
        i++;
    }
    
    // Keep size ascending order and search
    curr = GET_FREE_LIST_PTR(i);
    while ((curr != NULL) && (size > GET_SIZE(HDRP(curr)))) {
        succ = curr;
        curr = PRED(curr);
    }
    
    // Set predecessor and successor
    if (curr != NULL) {
        if (succ != NULL) {
            SET_PTR(PRED_PTR(bp), curr);
            SET_PTR(SUCC_PTR(curr), bp);
            SET_PTR(SUCC_PTR(bp), succ);
            SET_PTR(PRED_PTR(succ), bp);
        } else {
            SET_PTR(PRED_PTR(bp), curr);
            SET_PTR(SUCC_PTR(curr), bp);
            SET_PTR(SUCC_PTR(bp), NULL);
            //segregated_free_lists[i] = bp;
            SET_FREE_LIST_PTR(i, bp);
        }
    } else {
        if (succ != NULL) {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), succ);
            SET_PTR(PRED_PTR(succ), bp);
        } else {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_FREE_LIST_PTR(i, bp);
        }
    }
    
    return;
}


static void delete(void *bp) {
    int i = 0;
    size_t size = GET_SIZE(HDRP(bp));
    
    // Select segregated i
    while ((i < MAXNUMBER - 1) && (size > 1)) {
        size >>= 1;
        i++;
    }
    
    if (PRED(bp) != NULL) {
        if (SUCC(bp) != NULL) {
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
        } else {
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
            SET_FREE_LIST_PTR(i, PRED(bp));
        }
    } else {
        if (SUCC(bp) != NULL) {
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
        } else {
            SET_FREE_LIST_PTR(i, PRED(bp));
        }
    }
    
    return;
}

/*
* this is done in physical heap memory
* returns pointer to new free block
* coalesce is called before it goes in the free list
*/
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) {                         // Case 1
        return bp;
    }
    else if (prev_alloc && !next_alloc) {                   // Case 2
        delete(bp);
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {                 // Case 3
        delete(bp);
        delete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                                // Case 4
        delete(bp);
        delete(PREV_BLKP(bp));
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
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
        PUT(HDRP(bp), PACK(bp_size, 1));
        PUT(FTRP(bp), PACK(bp_size, 1));
    }
    
    else if (asize >= 96) {
        // Split block
        PUT(HDRP(bp), PACK(remainder, 0));
        PUT(FTRP(bp), PACK(remainder, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
        add(bp, remainder);
        return NEXT_BLKP(bp);
        
    }
    
    else {
        // Split block
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        add(NEXT_BLKP(bp), remainder);
    }
    return bp;
}
















