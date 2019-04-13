/*
 *  I implemented
 *  Seglist Allocators described in the ppt 
 *  
 *  - each size class has its own free list
 *  - one class for each two-power size 
 *  - {1},{2},{3,4},{5-8}, ..., {1025-2048}, ...
 *  
 *  - It is worth noting the difference between 
 *    (predecessor and successor) and (previous and next)
 *    (predecessor and successor) : pred and succ block in free list, 
 *    (previous and next) : previous and next block in heap memory
 * 
 * - Since we are not allowed to use any global or static arrays,
 *   I allocated a memory space in heap to keep these free lists (Or Seglist Allocators).
 *   This is described in visual text below.
 * 
 * -For blocks, I used the boundary tag coalescing technique.
 */

/* ALLOCATED BLOCK, FREE BLOCK, SEGREGATED FREE LIST, HEAP STRUCTURE

 1. Allocated Block 
 
             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | 1|
    bp ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
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
            |                        pointer to pred block in free list                                     |
    bp+4--> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to succ block in free list                                     |
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | 0|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

 3. Segregated Free Lists 
    (space is allocated in heap to keep this array of pointers)
    (**free_lists points to first pointer in the array)
    ----------------------------------------------------------
    |  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |  |size class 0  |size class 1  |      . . .
    |  |(bp of blck 0 |(bp of blck 0 |              
    |  | is kept here)|is kept here) |
    |  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+-
    -----------------------------------------------------------
              |               |     
         +--+--+--+--+   +--+--+--+--+
         | blck 0    |   | blck 0    |
         +--+--+--+--+   +--+--+--+--+
              |               |     
         +--+--+--+--+   +--+--+--+--+
         | blck 1    |   | blck 1    | 
         +--+--+--+--+   +--+--+--+--+ 

 4. Heap Memory After Free Lists
    (prologue and epilogue blocks are tricks that eliminate the edge conditions
    during coalescing)
              31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               0 (Padding)                                            |  |  |  |
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               8 (Prologue block)                                     |  |  | 1|
*heap_listp ->+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               8 (Prologue block)                                     |  |  | 1|
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              .                                                                                               .
              .                             Both Free and Allocated Blocks inmemory space                     .
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

#define MAXNUMBER 16

#define MAX(x,y) ((x) > (y) ? (x) : (y) )
#define MIN(x, y) ((x) < (y) ? (x) : (y))


/*Pack a size and allocated bit into a word*/
#define PACK(size, alloc) ((size) | (alloc))

/*Read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

//put pointer ptr in p (just simple address)
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

//since we cannot use such thing as Array[i] = k,
//these trivial pointer calculators are used to get and set
//values from/tp free lists
#define GET_FREE_LIST_PTR(i) (*(free_lists+i))
#define SET_FREE_LIST_PTR(i, bp) (*(free_lists+i) = bp)

/*Global variables*/
static char *heap_listp = 0; 
static char **free_lists;

/*helper functions*/
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *place(void *bp, size_t asize);
static void add(void *bp, size_t size);
static void delete(void *bp);

void mm_check(int verbose);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int i;

    //create space for keeping free lists 
    if ((long)(free_lists = mem_sbrk(MAXNUMBER*sizeof(char *))) == -1){
        return -1;
    }
    
     // initialize the free list
    for (int i = 0; i <= MAXNUMBER; i++) {
	    SET_FREE_LIST_PTR(i, NULL);
    }

    // initial empty heap
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
 * 
 *     
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
void *mm_malloc(size_t size)
{
    size_t asize;      //adjust request block size to allow room for header and footer
                      // and to satisfy alignment requirement.
    size_t extendsize; //incase we need to extend heap 
    void *bp = NULL;  
    
   
    if (size == 0) {
        return NULL;
    }
    
    // satisfy alignment requirement
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = ALIGN(size+DSIZE);
    }
    //textbook-add overhead bytes and then round up to nearest multiple of 8
    
    int i = 0;
    size_t searchsize = asize;

    // search free lists for suitable free block
    while (i < MAXNUMBER) {
        if ((i == MAXNUMBER - 1) || ((searchsize <= 1) && 
        (GET_FREE_LIST_PTR(i)!= NULL))) {
            bp = GET_FREE_LIST_PTR(i);
            
            //in each size class
            while ((bp != NULL) && (asize > GET_SIZE(HDRP(bp))))
            {
                bp = PRED(bp);
            }
            //found it!
            if (bp != NULL){
                //place requested block 
                //(if there is an access, it will split in place())
                bp = place(bp, asize);
                //return the pointer of the newly allocated block
                return bp;
            }
        }
        
        searchsize = searchsize >> 1;
        i++;
    }
    
    // if allocator cannot find a fit, extend the heap with 
    //new free block
    if (bp == NULL) {
        extendsize = MAX(asize, CHUNKSIZE);
        //place requested block in the new free block
        if ((bp = extend_heap(extendsize)) == NULL)
            return NULL;
    }
    
    //(place() will optionally split the block)
    bp = place(bp, asize);
    
    
    // return pointer to newly allocated block
    return bp;
}


/*
 * mm_free - Freeing a block
 */
void mm_free(void *bp)
{
   size_t size = GET_SIZE(HDRP(bp));
    
    //change header and footer to free 
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    //add it to free lists
    add(bp, size);

    //incase there is free block in prev or next block 
    coalesce(bp);
    
    return;
}

/*
 * mm_realloc : returns a pointer to an allocated region of 
 * at least size bytes
 * 
 */
void *mm_realloc(void *bp, size_t size)
{   
    void *new_block = bp;
    int remainder;

    if (bp == NULL) { //the call is equivalent to mm_malloc(size)
        return mm_malloc(size);
    }
    if (size == 0) { //the call is equivalent to mm_free(ptr)
        mm_free(bp);
        return NULL;
    }

    //to satisfy alignment
    if (size <= DSIZE){
        size = 2 * DSIZE;
    }
    else{
        size = ALIGN(size + DSIZE);
    }

    //old block size is bigger
    //I think just return the original block
    if ((remainder = GET_SIZE(HDRP(bp)) - size) >= 0){
        //PUT(HDRP(bp), PACK(size, 1));
        //PUT(FTRP(bp), PACK(size, 1));
        //mm_free(NEXT_BLKP(bp));
        return bp;
    }
    //new block size is bigger and next block is not allocated
    //use adjacent block to minimize external fragmentation
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp)))){
        if ((remainder = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - size) < 0){
            if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL){
                return NULL;
            }
            remainder += MAX(-remainder, CHUNKSIZE);
        }
        delete(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size + remainder, 1));
        PUT(FTRP(bp), PACK(size + remainder, 1));
    } 
    //in this case where new block's size is bigger
    //but we cannot use adjacent block, address will be different 
    else{
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
    size_t asize; //adjusted size to maintain alignment               
    
    asize = ALIGN(size);
    
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    
    /*Initialize free block header/footer and the epilogue header*/
    PUT(HDRP(bp), PACK(asize, 0)); //Free block header
    PUT(FTRP(bp), PACK(asize, 0)); //Free block footer 
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //New epilouge header
    add(bp, asize);
    
    //in case previous block was free 
    return coalesce(bp);
}

//add to free lists
static void add(void *bp, size_t size) {
    int i = 0;
    void *curr = bp;
    void *succ = NULL;
    
    // Select size class 
    while ((i < MAXNUMBER - 1) && (size > 1)) {
        size = size >> 1;
        i++;
    }
    
    //now we are in particular size class
    //pred <- curr <- succ (we move that way)
    curr = GET_FREE_LIST_PTR(i);
    while ((curr != NULL) && (size > GET_SIZE(HDRP(curr)))) {
        succ = curr;
        curr = PRED(curr);
    }
    
    // adding bp 
    if (curr != NULL) {
        if (succ != NULL) { //curr - bp - succ
            SET_PTR(PRED_PTR(bp), curr);
            SET_PTR(SUCC_PTR(curr), bp);
            SET_PTR(SUCC_PTR(bp), succ);
            SET_PTR(PRED_PTR(succ), bp);
        } else { //adding at the end
            SET_PTR(PRED_PTR(bp), curr);
            SET_PTR(SUCC_PTR(curr), bp);
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_FREE_LIST_PTR(i, bp);
        }
    } else {
        if (succ != NULL) { //adding at the other end
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), succ);
            SET_PTR(PRED_PTR(succ), bp);
        } else { //this size class has no nodes
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_FREE_LIST_PTR(i, bp);
        }
    }
    
    return;
}

//delete from free list
static void delete(void *bp) {
    int i = 0;
    size_t size = GET_SIZE(HDRP(bp));
    
    // select size class
    while ((i < MAXNUMBER - 1) && (size > 1)) {
        size >>= 1;
        i++;
    }
    //delete node making changes to pred, succ pointers 
    //pred <- curr <- succ (we move that way)
    if (PRED(bp) != NULL) {
        if (SUCC(bp) != NULL) {//pred - bp - succ => pred - succ
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
        } else {//deleting at the end
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
            SET_FREE_LIST_PTR(i, PRED(bp));
        }
    } else {
        if (SUCC(bp) != NULL) { //deleting at the other end
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
        } else {//this size class has only bp node
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
    
    if (prev_alloc && next_alloc) { //both allocated                    
        return bp;
    }
    else if (prev_alloc && !next_alloc) { //merge next block
        delete(bp);
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { //merge with prev block            
        delete(bp);
        delete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                      // merge with both
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

/*place - place block of asize bytes at free block bp
          split if remainder would be at least minimum block size*/
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
    /*
     ususally we just do else here and split the block.
     But there are cases like in binary-bal.rep, binary2-bal.rep

    case :  
     allocated <blocks> - (small size block or big size bock)
     small - big - small - big - small - big

     if we free this blocks in order small-big-small-big-small-bg 
     it causes no problems.

     However, if we free only the big blocks like below, 
     small - big(freed) - small - big(freed) - small- 

     Even if we want to use the big freed blocks together, we can't because
     there are small allocated blocks in between.

     If there is a allocate call to size bigger than big block,
     - we have to find another free block.

     So what I am trying to do here is put allocated blocks 
     in continuous places so we can use the freed space 

     small-small-small-small-big(freed)-big(freed)-

     I set the size 96 to get the best result for binary-bal.rep 
     and binary2-bal.rep
     */
     else if (asize >= 96) {
        PUT(HDRP(bp), PACK(remainder, 0));
        PUT(FTRP(bp), PACK(remainder, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
        add(bp, remainder);
        return NEXT_BLKP(bp);
        
    }
    
    else {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        add(NEXT_BLKP(bp), remainder);
    }
    return bp;
}

static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    mm_check(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
	printf("%p: EOL\n", bp);
	return;
    }

    printf("%p: header: [%p:%c] footer: [%p:%c]\n", bp, 
	hsize, (halloc ? 'a' : 'f'), 
	fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
	printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
	printf("Error: header does not match footer\n");
}


void mm_check(int verbose){
    char *bp = heap_listp;

    if (verbose)
	printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
	printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
	if (verbose) 
	    printblock(bp);
	checkblock(bp);
    }

    if (verbose)
	printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
	printf("Bad epilogue header\n");
}
















