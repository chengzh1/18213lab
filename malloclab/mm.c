/*
 * mm.c  -- one of the greatest lab in 18-213
 * 
 * Student Name: Cheng Zhang
 * Andrew ID: chengzh1
 *
 * Overview of the structure of free and allocated blocks:
 *  The minimum size of my blocks is 4 words (16 bytes), 
 *  which includes 1 word header 2 words padding block 
 *  and 1 word footer. For free blocks, header and 
 *  footer are used for coalescing. For allocated blocks, 
 *  the footer can be used to store data. Header contains 
 *  the size of the block, 1 bit to indicate if it is free, 
 *  and 1 bit to indicate if the previous block is free
 *
 * Organization of the free list:
 *  I use segregated list to organize my free list. Each 
 *  free block will in a list of free blocks according 
 *  to its size. As the size of the heap is no more than 2^32, 
 *  I store 2 32 bits offset address in the free block, one 
 *  points to the previous free block, the other points to 
 *  th next free block.
 *
 * How allocator manipulates the free list:
 *  Allocator uses first fit policy to find the proper free 
 *  block in the list, and split it to proper size. The 
 *  allocated block will be deleted from the free list and 
 *  the linke should be changed. A added free list will be
 *  add to the header of the free list
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "contracts.h"

#include "mm.h"
#include "memlib.h"

// Create aliases for driver tests
// DO NOT CHANGE THE FOLLOWING!
#ifdef DRIVER
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif

/*
 *  Logging Functions
 *  -----------------
 *  - dbg_printf acts like printf, but will not be run in a release build.
 *  - checkheap acts like mm_checkheap, but prints the line it failed on and
 *    exits if it fails.
 */

#ifndef NDEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#define checkheap(verbose) do {if (mm_checkheap(verbose)) {  \
                             printf("Checkheap failed on line %d\n", __LINE__);\
                             exit(-1);  \
                        }}while(0)
#else
#define dbg_printf(...)
#define checkheap(...)
#endif

/*
 *  Helper functions
 *  ----------------
 */

// Align p to a multiple of w bytes
static inline void* align(const void const* p, unsigned char w) {
  return (void*)(((uintptr_t)(p) + (w-1)) & ~(w-1));   
}

// Check if the given pointer is 8-byte aligned
static inline int aligned(const void const* p) {
  return align(p, 8) == p;
}

// Return whether the pointer is in the heap.
static int in_heap(const void* p) {
  return p <= mem_heap_hi() && p >= mem_heap_lo();
}


/*
 *  Block Functions
 *  ---------------
 *  TODO: Add your comment describing block functions here.
 *  The functions below act similar to the macros in the book, but calculate
 *  size in multiples of 4 bytes.
 */

#define WSIZE 4    
#define DSIZE 8    
#define LIST_SIZE 13    
#define CHUNKSIZE  160 
#define MIN_BLOCK_SIZE_IN_WORD 4  
#define MIN_MALLOC_SIZE MIN_BLOCK_SIZE_IN_WORD * WSIZE - WSIZE 
typedef uint32_t* word_ptr;

static char *heap_listp = 0;
static char *segrated_list = 0;

/* Put value to header or footer */
static inline void put(word_ptr const block, unsigned int val){
  REQUIRES(block != NULL);
  REQUIRES(in_heap(block));
  block[0] = val;
}

/* Return the value of header of footer */
static inline unsigned int get(void * p){
  return *((unsigned int *)p);
}

/* Return the size of the given block in multiples of the word size */
static inline unsigned int block_size(const word_ptr block) {
  REQUIRES(block != NULL);
  REQUIRES(in_heap(block));
  
  //block[0]:first 32 bit(header), header = flag(2 bit) | size 
  return (block[0] & 0x3FFFFFFF);   
}

/* Return true if the block is free, false otherwise */
static inline int block_free(const word_ptr block) {
  REQUIRES(block != NULL);
  REQUIRES(in_heap(block));
  return !(block[0] & 0x40000000);
}

/* Return true is the previous block is free, false otherwise */
static inline int block_prev_free(const word_ptr block){
  REQUIRES(block != NULL);
  REQUIRES(in_heap(block));
  return !(block[0] & 0x80000000);
}

/* Mark the given block as free(1)/alloced(0) */
static inline void block_mark(word_ptr block, int free) {
  REQUIRES(block != NULL);
  REQUIRES(in_heap(block));
  size_t size = block_size(block);

  //pointer to footer
  unsigned int next = (size == 0) ? 0 : (size - 1); 
  
  //free(0), allocated(1)
  block[0] = free ? block[0] & 0xBFFFFFFF : block[0] | 0x40000000; 
  block[next] = block[0];   //mark footer

  // mark previous block's satus
  if (!free && size > 0){
    block[next + 1] |= 0x80000000; //allocated(1)
  }
  else{
    block[next + 1] &= 0x7FFFFFFF; //free(0)
  }
}

/* Give block ptr bp, return header */
static inline word_ptr header(word_ptr const bp){
  REQUIRES(bp != NULL);
  REQUIRES(in_heap(bp - 1));
  return bp - 1;
}

/* Give block ptr bp, return footer */
static inline word_ptr footer(word_ptr const bp){
  REQUIRES(bp != NULL);
  REQUIRES(in_heap(bp));
  return bp + block_size(header(bp)) - 2;
}

/* Set the head's size */
static inline void set_head_size(void* bp, size_t size){
  if (block_prev_free(header(bp)))
    put(header(bp), size); 
  else
    put(header(bp), 0x80000000 | size);
}

/* Return next block */
static inline word_ptr block_next(word_ptr const bp) {
  REQUIRES(bp != NULL);
  REQUIRES(in_heap(bp));
  return bp + block_size(bp - 1); 
}

/* Return the previous block */
static inline word_ptr block_prev(word_ptr const bp) {
  REQUIRES(bp != NULL);
  REQUIRES(in_heap(bp));
  return bp - block_size(bp - 2);
}

/* Return previous free block */
static inline void* free_prev(char* bp){
  /* heap is less than 2^32, 
    use 4bytes to store offset is enough */
  int diff = * ((int *)bp);
  REQUIRES(diff >= 0);
  if (diff == 0)
    return NULL;
  else
    return (void *)(heap_listp + diff);
}
/* Return next free block */
static inline void* free_next(char* bp){
  int diff = * ((int *)(bp + WSIZE));
  REQUIRES(diff >= 0);
  if (diff == 0)
    return NULL;
  else
    return (void *)(heap_listp + diff);
}
/* Set the previous free block */
static inline void set_free_prev(int* bp, char* p){
  REQUIRES(in_heap(bp));
  bp[0] = (p == NULL) ? 0 : p - heap_listp;
}
/* Set the next free block */
static inline void set_free_next(int* bp, char* p){
  REQUIRES(in_heap(bp));
  bp[1] = (p == NULL) ? 0 : p - heap_listp;
}
/* Return the header of the nth segrated list */
static inline void* get_segrated_list(char* segrated_list, int num){
  return * (void **)(segrated_list + DSIZE * num);
}

/* Set the header of the nth segrate list */
static inline void set_segrated_list(char* segrated_list, int num, void* p)
{
  * (void **)(segrated_list + DSIZE * num) = p;
}

/*
 * Declaration of some major functions
 */
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
// For mm_check
static int checkblock(void *bp);
static void printblock(void *bp);
// For free list
static void* add_to_free_list(void* bp);
static void delete_from_free_list(void* bp);
// For segrated list
static int get_segrated_list_num(size_t size);


/*
 *  Malloc Implementation
 *  ---------------------
 *  The following functions deal with the user-facing malloc implementation.
 */

/*
 *  Function Name: mm_init
 *
 *  Usage: to initialize the allocator
 *         return -1 on error, 0 on success.
 */
int mm_init(void)
{
  // Create the initial empty heap
  heap_listp = 0;  

  //allocate space of arrays for segrated list
  if ((segrated_list = mem_sbrk(LIST_SIZE * DSIZE)) == (void *)-1)
    return -1;

  //initialize
  for (int i = 0; i < LIST_SIZE; i ++){
    set_segrated_list(segrated_list, i, 0);
  }
  
  //allocate heap list
  if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    return -1;
  
  //initialize
  put((word_ptr)heap_listp, 0);                          
  put((word_ptr)(heap_listp + (1*WSIZE)), DSIZE / WSIZE); 
  // mark Prologue footer
  block_mark((word_ptr)(heap_listp + (1*WSIZE)), 0);  
  // Epilogue header   
  put((word_ptr)(heap_listp + (3*WSIZE)), 0xC0000000);   
  heap_listp += (2 * WSIZE);

  // Extend the empty heap with a free block of CHUNKSIZE bytes
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    return -1;

  return 0;
}

/*
 * Function Name: extend_heap
 *
 * Usage: to extension
 *        extend heap by words or CHUNKSIZE
 */
static void *extend_heap(size_t words)
{
  void *bp;
  size_t size;

  // Allocate an even number of words to maintain alignment 
  size = (words % 2) ? (words+1) : words;
  if ((long)(bp = mem_sbrk(size * WSIZE)) == -1)
    return NULL;
   
  // Initialize free block header/footer and the epilogue header
  set_head_size(bp, size);

  // Free block footer
  block_mark(header(bp), 1); 

  // New epilogue header(the previous block must be free)
  put(header(block_next(bp)), 0x40000000); 
    
  // Add new allocated block to free list
  bp = add_to_free_list(bp); 
  return bp;
}

/*
 * Function Name: coalescs
 * Usage: check the previous and next block
 *        and combine them if they are free
 */
static void *coalesce(void *bp)
{
  size_t prev_free = block_prev_free(header(bp));
  size_t next_free = block_free(header(block_next(bp)));
  size_t size = block_size(header(bp));

  // Case 1 both are free
  if (prev_free && next_free){     
     //delete both previous and next free list
    delete_from_free_list(block_prev(bp));
    delete_from_free_list(block_next(bp)); 

    size += block_size(header(block_next(bp))) 
            + block_size(header(block_prev(bp)));

    //remark the block
    set_head_size(block_prev(bp), size);
    block_mark(header(block_prev(bp)), 1);
    bp = block_prev(bp);
  }

  // Case 2 only next is free
  else if (!prev_free && next_free){ 
    //delet next block in free list  
    delete_from_free_list(block_next(bp)); 

    size += block_size(header(block_next(bp)));  

    set_head_size(bp, size);
    block_mark(header(bp), 1);
  }

  // Case 3 only prev is free
  else if (prev_free && !next_free){   
    bp = block_prev(bp);
    //delet this previous block in free list
    delete_from_free_list(bp);  

    size += block_size(header(bp));

    set_head_size(bp, size);
    block_mark(header(bp), 1);
  }
  
  // Case 4 prev and next both allocated, do nothing     
  return bp;
}

/*
 * Funciton Name: malloc
 *
 * Usage: Contains all the work for malloc
 */
void *malloc (size_t size) 
{
  size_t asize;       // Adjusted block size
  size_t extendsize;  // Amount to extend heap if no fit
  size_t awords;
  char *bp;

  if (heap_listp == 0)
    mm_init();

  // Ignore spurious request 
  if (size == 0)
    return NULL;

  // Adjust block size to include overhead and alignment reqs.
  if (size <= MIN_MALLOC_SIZE)
    asize = MIN_BLOCK_SIZE_IN_WORD * WSIZE;
  else
    asize = DSIZE * ((size + WSIZE + DSIZE - 1) / DSIZE);

  //convert to word size
  awords = asize / WSIZE;

  // Search the free list for a fit
  if ((bp = find_fit(awords)) != NULL) {
    place(bp, awords);
    return bp;
  }
  // No fit found. Get more memory and place the block
  extendsize = asize > CHUNKSIZE ? asize : CHUNKSIZE;  
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    return NULL;
  
  place(bp, awords);
  // heap check
  checkheap(1);      
  return bp;
}

/*
 * Function Name: free
 *
 * Usage: free a allocated block
 */
void free (void *ptr) 
{
  if (ptr == NULL)
    return;

  size_t size = block_size(header(ptr));

  //mark header and footer to be free
  set_head_size(ptr, size);
  block_mark(header(ptr), 1); 

  //add block the free list
  add_to_free_list(ptr);
}

/*
 * Function Name: place
 * 
 * Usage: split larger size of block
 */
static void place(void *bp, size_t size)
{   
  // delet the whole block
  delete_from_free_list(bp);
  size_t csize = block_size(header(bp));
  
  //when rest size >= minimum size
  if ((csize - size) >= MIN_BLOCK_SIZE_IN_WORD){ 
    //mark header and footer to be allocated
    set_head_size(bp, size);
    block_mark(header(bp), 0); 

    void* next = block_next(bp);
    set_head_size(next, csize-size);
    block_mark(header(next), 1);
    //add the split block
    add_to_free_list(next);
  }
  else{
    //mark header and footer to be allocated
    set_head_size(bp, csize);
    block_mark(header(bp), 0); 
  }
}

/*
 * Function Name: find fit
 * Usage: use first fit policy to find a proper free block
 */
static void *find_fit(size_t size)
{
  int num = get_segrated_list_num(size);
  int i;
  void *bp = NULL;
  
  // First fit search 
  for (i = num; i < LIST_SIZE; i ++){
    for (bp = get_segrated_list(segrated_list, i); 
          bp != NULL; bp = free_next(bp))
    {
      if (size <= block_size(header(bp)))
        return bp;
    }
  }
  return NULL; 
}

/*
 * Function: get_segrated_list_num
 *
 * Usage: return the no of segrated according to
 *        the size of the block
 */
static int get_segrated_list_num(size_t size)
{
  if (size <= 4)    // 0 to 16 bytes
    return 0;
  if (size <= 6)    // 17 to 24 bytes
    return 1;
  if (size <= 8)    // 25 to 32 bytes
    return 2;
  if (size <= 16)   // 33 to 64 bytes
    return 3;
  if (size <= 48)   // 65 to 192 bytes 
    return 4;
  if (size <= 112)  // 193 to 448 bytes
    return 5;
  if (size <= 240)  // 449 to 960 bytes
    return 6;
  if (size <= 474)  // 961 to 1896 bytes
    return 7;
  if (size <= 960)  // 1897 to 3840 bytes
    return 8;
  if (size <= 1920) // 3841 to 7680 bytes
    return 9;
  if (size <= 3840) // 7681 to 15360 bytes
    return 10;
  if (size <= 7694) // 15361 to 30766 bytes
    return 11;
  return 12;        // > 30766 bytes
}

/*
 * Function Name: add_to_free_list
 *
 * Usage: add free block to segarted list
 */
static void* add_to_free_list(void* bp)
{ 
  REQUIRES(bp != NULL);

  // Coalesce if the previous block was free in add_to_free_list 
  bp = coalesce(bp);

  //Set segrated list
  int num = get_segrated_list_num(block_size(header(bp)));
  void* current_segrated_list = get_segrated_list(segrated_list, num);
  
  //Change the previous and next pointer
  if (current_segrated_list == NULL){
    set_free_prev(bp, NULL);
    set_free_next(bp, NULL);
  }
  else{
    set_free_prev(bp, NULL);
    set_free_next(bp, current_segrated_list);
    set_free_prev(current_segrated_list, bp);
  }
  set_segrated_list(segrated_list, num, bp);

  return bp;
}

/*
 * Function Name; delete_from_free_list
 *
 * Usage: delete block from segrated list
 */
static void delete_from_free_list(void* bp)
{
  void * prev = free_prev(bp);
  void * next = free_next(bp);

  // when bp is header
  if (prev == NULL){ 
    int num = get_segrated_list_num(block_size(header(bp)));
    set_segrated_list(segrated_list, num, next);
  }else{
    set_free_next(prev, next);
  }
  if (next != NULL){
    set_free_prev(next, prev);
  }
  // in case the pointer is confusing
  set_free_next(bp, NULL);
  set_free_prev(bp, NULL);
}

/*
 * Function Name: realloc
 *
 * Usage: realloc a block with size
 */
void *realloc(void *oldptr, size_t size) 
{  
  size_t oldsize;
  void *newptr;

  // If size == 0 then this is just free, and we return NULL
  if(size == 0){
    free(oldptr);
    return NULL;
  }

  // If oldptr is NULL, then this is just malloc
  if(oldptr == NULL)
    return malloc(size);

  newptr = malloc(size);

  // If realloc() fails the original block is left untouched
  if(!newptr)
    return NULL;

  // Copy the old data
  oldsize = block_size(header(oldptr)) * WSIZE;
  if(size < oldsize) 
    oldsize = size;

  memcpy(newptr, oldptr, oldsize);

  // Free the old block
  free(oldptr);
  checkheap(1);

  return newptr;
}

/*
 * Function Name: calloc
 * 
 * Usage: my calloc version
 */
void *calloc (size_t nmemb, size_t size) 
{
  size_t bytes = nmemb * size;
  void *newptr;
  newptr = malloc(bytes);
  memset(newptr, 0, bytes);
  return newptr;
}

/*
 * Function Name: mm_checkheap
 *
 * Usage: heap check
 *        Returns 0 if no errors were found, 
 *        otherwise returns the error
 */ 
int mm_checkheap(int verbose) 
{
  word_ptr bp = (word_ptr) heap_listp;
  int count_in_free_list = 0;
  int count_free = 0;

  word_ptr prologue = header(bp);

  //Check prologue blocks
  if (block_size(prologue) * WSIZE != DSIZE || block_free(prologue)){  
    printf("Bad prologue header %d and free %d\n",
          block_size(prologue), block_free(prologue));
    if (verbose)
      printblock(bp);
    return 1;
  }

  //Check block's alignment as well as header and footer
  if(checkblock(heap_listp)){
    printblock(heap_listp);
    return 1;
  }

  //Check each block
  for (bp = block_next((word_ptr) heap_listp); 
      block_size(header(bp)) > 0; bp = block_next(bp))
  {
    
    //Check heap boundaries
    if(!in_heap(bp)){
      printf("boundaries error\n");
      if (verbose)
        printblock(bp);
        return 1;
    }

    //Check minimum size
    if (block_size(header(bp)) < MIN_BLOCK_SIZE_IN_WORD){
      printf( "block minimum size error, the size is %u\n",
               block_size(header(bp)));
      if (verbose)
        printblock(bp);
      return 1;
    }

    //Check alignment as well as header and footer
    if(checkblock(bp)){
      if (verbose)
        printblock(bp);
      return 1;
    }

    if (block_free(header(bp))){
      count_free ++;  //count free block
    
      //Check coalescing
      if( in_heap(block_prev(bp)) && block_prev_free(header(bp))){
        printf("coalesce error\n");
        if (verbose)
          printblock(bp);
        return 1;
      }
    }

    //Check previous/next allocate/free bit consistency
    if (block_prev_free(header(bp)) 
        && !block_free(header(block_prev(bp)))){
      printf("coalesce error\n");
      if (verbose)
        printblock(bp);
      return 1;
    }
  }

  //Check epilogue block
  word_ptr epilogue = header(bp);
  if (block_size(epilogue) != 0 || block_free(epilogue)){
    printf("Bad epilogue header with size %d and free %d\n",
            block_size(epilogue), block_free(epilogue));
    if (verbose)
      printblock(bp);
    return 1;
  } 

  //Check free list
  for (int i = 0; i < LIST_SIZE; i ++){
    void * free_check = get_segrated_list(segrated_list, i);
    void * pre = NULL;
    
    while(free_check != NULL){
      //Check free_list in heap
      if (! in_heap(free_check)){
        printf("%s\n", "free list not in heap error");
        printf("num: %d, address: %p\n", i, free_check);
        return 1;
      }
      //Check fnext/previous pointers are consistent
      if (free_prev(free_check) != pre){
        printf("%s\n", "free pre error");
        printf("prev: %p, next's prev: %p free check is: %p\n", 
                pre, free_prev(free_check), free_check);
        return 1;
      }

      //Check segarted list within bucket size range
      if (get_segrated_list_num(block_size(header(free_check))) != i)
        printf("%s\n", "bucket size range error");

      pre = free_check;
      free_check = free_next(free_check);
      count_in_free_list ++;
    }
  }

  //Check free blocks count match
  if (count_free != count_in_free_list){
    printf("free block count error");
    printf("in free list: %d, in heap list: %d\n", 
            count_in_free_list, count_free);
    return 1;
  }

  return 0;
}

/*
 * Function Name: checkblock
 *
 * Usage: Check block's header and footer's alignment, 
 *        free flag and size
 */
static int checkblock(void *bp) 
{
  //Check block alignment
  if ((size_t)bp % 8){
    printf("Error: %p is not doubleword aligned\n", bp);
    return 1;
  }

  /* Check footer and header matching each other
    (remove the pre block flag) */
  if (block_free(header(bp)) && 
    ((get(header(bp)) & 0x7FFFFFFF) != (get(footer(bp)) & 0x7FFFFFFF)))
  {
    printf("Error: header does not match footer\n");
    return 1;
  }
  return 0;
}

/*
 * Function Name: printblock
 *
 * Usage: printf debug info
 */
static void printblock(void *bp) 
{
  unsigned int hsize, halloc, fsize, falloc;

  hsize = block_size(header(bp));
  halloc = !block_free(header(bp));  
  fsize =  block_size(footer(bp));
  falloc = !block_free(footer(bp));  

  if (hsize == 0) {
    printf("%p: EOL\n", bp);
    return;
  }

  printf("%p: header: [%u:%c] footer: [%u:%c]\n", bp, 
      hsize, (halloc ? 'a' : 'f'), 
      fsize, (falloc ? 'a' : 'f')); 
}

