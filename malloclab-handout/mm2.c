/*
 * mm.c
 * 
 * This package uses segmented free lists to keep track of
 * free blocks of memory. Coalescing blocks is delayed until
 * no free blocks are available. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

// DEBUG level sets amount of debug information to print out
//#define DEBUG 4

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
     /* Team name */
    "Trippy Zhu",
    /* First member's full name */
    "Alexander Zhu",
    /* First member's email address */
    "alexanderzhu2017@u.northwestern.edu",
    /* Second member's full name (leave blank if none) */
    "Samantha Trippy",
    /* Second member's email address (leave blank if none) */
    "samanthatrippy2016@u.northestern.edu"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* cutoff for exactly sized lists */
#define MIN_APPROX_SIZE (0x100)
#define SMALL_LIST_SIZE (MIN_APPROX_SIZE / ALIGNMENT)
#define LARGE_LIST_SIZE 23

/* minimum size for a block */
#define MIN_BLOCK_SIZE (SIZE_T_SIZE * 3)

// block traversal macros
#define PAYLOAD(block_header) ((header **)((char *) block_header + SIZE_T_SIZE))
#define HEADER(payload_ptr) ((header *)((char *) payload_ptr - SIZE_T_SIZE))
#define NEXT_FREE(block_header) (*PAYLOAD(block_header))
#define PREV_FREE(block_header) (*(PAYLOAD(block_header)+1))

// size of the block (since the last bit is used as a flag)
#define GET_SIZE(block_header) (*block_header & ~0x1)

// next block (address-wise)
#define NEXT(block_header) ((header *)((char *) block_header + GET_SIZE(block_header)))

// mark blocks as free/allocated
#define MARK_ALLOC(block_header) (*block_header = *block_header | 0x1)
#define MARK_FREE(block_header) (*block_header = (*block_header | 0x1) - 1)

/* returns allocated flag */
#define IS_ALLOCED(block_header) (*block_header % 2)
#define IS_FREE(block_header) (~IS_ALLOCED(block_header))

typedef size_t header;

// function definitions
void print_heap();      // prints every block in heap
void print_block();     // prints details of particular block
void print_all_free();  // prints segmented free lists
void add_to_llist(header * node, header * after);
void add_to_freelist(header * freeblock);
int mm_check();
void distribute(header * to_split);

/* =========  Global Variables  ========= */

/*
 * small items ( <= 256 bytes) are stored in small_free 
 * large items are stored in large_free.  large items are binned 
 *  exponentially. large_free[n] contains items with size greater than
 *  0x100 << n and less than or equal to 0x100 << (n + 1).
 */
header ** small_free;
header ** large_free;

header * heap_lo;
header * heap_hi;

/* how much memory to sbrk (we increase by a factor of 2 every time) */
int sbrkCounter;

/*
 * the beginning of the region of usable memory (since we use the beginning
 * of the heap for internally used arrays).
 */
header * mem_start;

/* 
 * mm_init - initialize the malloc package. set up our segmented free list
 *  headers and create the initial amount of free memory.
 */
 
/************  Doubly Linked List Functions  *************/
/*
 * Remove node from doubly linked list
 */
void remove_from_llist(header * node)
{
    header * nextfree = NEXT_FREE(node);
    header * prevfree = PREV_FREE(node);
    
	if(prevfree != NULL)
		NEXT_FREE(prevfree) = nextfree;
	if(nextfree != NULL)
		PREV_FREE(nextfree) = prevfree;
}

/*
 * Add node to a doubly linked list after after
 */
void add_to_llist(header * node, header * after)
{
	header * next = NEXT_FREE(after);
	
	NEXT_FREE(after) = node;
	PREV_FREE(node) = after;
	
	NEXT_FREE(node) = next;
	if(next != NULL)
		PREV_FREE(next) = node;
}

/********** end linked list functions ********/ 
 
 
 
int mm_init(void)
{
#if DEBUG >= 3
    printf("MM_INIT\n");
#endif

	int i;
    
    /* intial memory is 1536 bytes, just enough to fit our segregated free 
          * lists and dummy variables
          */
    sbrkCounter = 0x600;
	heap_lo = mem_sbrk(sbrkCounter);
    heap_hi = mem_heap_hi();
    
	/* set up our segregated lists */
	small_free = (header **) heap_lo;
	large_free = small_free + SMALL_LIST_SIZE;

	header * cur = (header *) (large_free + LARGE_LIST_SIZE + 1);
    mem_start = cur;
    
#if DEBUG >= 3
    printf("\n******\nHeap: %p to %p\n", heap_lo, heap_hi);
    printf("Dummies start at %p\n", cur);
#endif

	// setup the exact segments (small sizes)
	for(i = 0; i < SMALL_LIST_SIZE; i++)
	{
		if( (i+1) * ALIGNMENT < MIN_BLOCK_SIZE)
		{
			small_free[i] = NULL;
		} else 
		{
			// the first element of the list is a dummy block
			small_free[i] = (header *) cur;
			*cur = MIN_BLOCK_SIZE;
			MARK_ALLOC(small_free[i]);
			NEXT_FREE(small_free[i]) = NULL;
			PREV_FREE(small_free[i]) = NULL;
			cur = (size_t *) NEXT(small_free[i]);
		}
	}

	// setup the approx segments (large sizes)
	for(i = 0; i < LARGE_LIST_SIZE; i++)
	{
		large_free[i] = (header *) cur;
		*cur = MIN_BLOCK_SIZE;
		MARK_ALLOC(large_free[i]);
		NEXT_FREE(large_free[i]) = NULL;
		PREV_FREE(large_free[i]) = NULL;
		cur = (header *) NEXT(large_free[i]);
	}
	
	// add remaining free memory to free list
	*cur = (header) heap_hi - (header) cur + 1;
#if DEBUG > 2
	printf("Initializing the heap with free block %p (size %lu)\n",
			cur, (unsigned long) *cur);
#endif
	add_to_freelist((header *) cur);
    
#if DEBUG >= 3
    print_heap();
	if(!mm_check())
		assert(0);
#endif
	return 0;
}

/*
 * finds which segment to add the free block to.  Assumes that 
 *  free_block is a valid block, and adds it
 */
void add_to_freelist(header * free_block)
{
	int i;
	size_t size = (size_t) GET_SIZE(free_block);
#if DEBUG >= 3
    printf("Adding to free list %p of size %d\n", free_block, size);
#endif

    // small free block
	if(size <= MIN_APPROX_SIZE)
	{
		i = size / ALIGNMENT - 1;
		add_to_llist(free_block, small_free[i]);
	} 
    
    // large free block
    else 
	{
		for(i = 0; i < LARGE_LIST_SIZE; i ++)
		{
			if( (unsigned long) MIN_APPROX_SIZE << (i + 1) > size )
			{
				add_to_llist(free_block, large_free[i]);
				return;
			}
		}
#if DEBUG >= 1
		assert(0);
#endif
	}
}

/*
 * Mark the block as allocated, possibly split the block, remove
 * the allocated block from the free list, and return a pointer to
 * the payload region of the allocated block.
 * Assumes that block is at least of size size.
 */
void * allocate(header * block, size_t size)
{
#if DEBUG >= 3
	printf("Allocating %p of size %d, for size %lu", block, 
			(size_t)*block, (long unsigned int) size);
#endif
   
#if DEBUG >= 1
    if(*block < size)
	{
        printf("ERROR!! BLOCK SIZE (%d) < ALLOCATE SIZE (%d)", *block, size); 
        assert(0);
    }
#endif

    // remove the block from free lists
    remove_from_llist(block);
	size_t split_size = (size_t)*block - size;
    
    if(split_size >= MIN_BLOCK_SIZE)
    {
#if DEBUG >= 3
		printf(", split size: %lu.\n", (long unsigned int) split_size);
#endif
		// split the block in two
        *block = size;
		header * split = NEXT(block); //second half of the split block
		*split = split_size;
        
        // add new block to free list
        MARK_FREE(split);
        add_to_freelist(split);
    } else 
	{
#if DEBUG >= 3
		printf(", no splitting.\n");
#endif
		remove_from_llist(block);
	}
	
    MARK_ALLOC(block);
    
	// pointer to the payload
    return (void *) PAYLOAD(block);
}

/* 
 * coalesce() - Coalesces free memory until memory of size size is created
 *  at which point coalesce returns a pointer to a block with size at least
 *  size.  Assumes that size is already aligned.  Returns NULL if colaescing
 *  finishes without finding such a block.  If size is 0, then finishes
 *  coalescing and returns NULL.
 */

header * coalesce(size_t size)
{
#if DEBUG >= 3
	printf("Running coalesce...\n");
#endif
    int i = MIN_BLOCK_SIZE / ALIGNMENT - 1;
    for (; i < SMALL_LIST_SIZE + LARGE_LIST_SIZE - 1; i++)
	{
        header * cur = NEXT_FREE(small_free[i]);
        header * next;
        
        while (cur != NULL)
		{
            next = NEXT(cur);
            if (next != NULL && next < heap_hi && 
				GET_SIZE(next) >= MIN_BLOCK_SIZE && 
				(long unsigned int) IS_ALLOCED(next) == 0)
			{
                
                #if DEBUG >= 3
                printf("    coalescing %p (%d) and %p (%d)...\n", 
						cur, *cur, next, *next);
				print_block(next);
                #endif
                
                remove_from_llist(cur);                
                remove_from_llist(next);
                
                *cur += *next;
                add_to_freelist(cur);
                
                if (size && *cur >= size)
                    return cur;
            } else
			{
                cur = NEXT_FREE(cur);
            }
        }
    }

	return NULL;
}

/*
 * find_free() - finds a free node of size at least size. starts by
 *  searching the smallest possible bin and moves up.
 */
header * find_free(size_t size)
{
#if DEBUG >= 3
    if(!mm_check())
		assert(0);
    printf("    finding free block of size %lu... ", (unsigned long) size);
#endif
    int i;
    
    // try to fit into small fixed size buckets first
    if (size <= MIN_APPROX_SIZE)
	{
        for (i = size/ALIGNMENT-1; 
			 i < SMALL_LIST_SIZE && NEXT_FREE(small_free[i]) == NULL;
			 i++){}
        
        if (i < SMALL_LIST_SIZE) 
		{
#if DEBUG >= 3
            printf("%d, %p (%lu) found in small free list!\n", 
				   i, NEXT_FREE(small_free[i]), 
				   (unsigned long)*NEXT_FREE(small_free[i]));
#endif
            return NEXT_FREE(small_free[i]);
        }

		i = 0;
    } else 
	{
    	// either no small free blocks exist, or size is too large
        for(i = 0; i < LARGE_LIST_SIZE; i ++)
		{
			if( MIN_APPROX_SIZE << (i + 1) > size )
				break;
		}
    }
    
    // find large free block
    while(i < LARGE_LIST_SIZE)
	{
        if (NEXT_FREE(large_free[i]) != NULL)
		{
            header * curr = NEXT_FREE(large_free[i]);
            while (curr != NULL)
			{
                if (GET_SIZE(curr) >= size)
				{
                    #if DEBUG >= 3
                    printf("%d, %p (%x) found in large free list!\n", 
						   i, curr, GET_SIZE(curr));
                    print_block(curr);
                    #endif
                    return curr;
                }
                curr = NEXT_FREE(curr);
            }
        }
		i++;
    }
    
    // no sufficiently large free block could be found
#if DEBUG >= 3
    printf("\n\nNEED MORE MEMORY!! size = %d\n", size);
    print_all_free();
    print_heap();
#endif
    return NULL;
}

/* 
 * mm_malloc - Allocate a block by finding a free node in one of the segmented
 *     lists. Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
#if DEBUG >= 3
    printf("MM_MALLOC %d\n", size);    
    if(!mm_check())
		assert(0);
#endif
    
    size_t block_size = ALIGN(size + SIZE_T_SIZE);
    if (block_size < MIN_BLOCK_SIZE) {block_size = MIN_BLOCK_SIZE;}
    
    /* find a free block in the lists */
    header * block = find_free(block_size);
    
    // no free block could be found
    if(block == NULL)
    {
		// first try to coalesce
		block = coalesce(block_size);
		
        // coalesce worked!
        if (block != NULL) {
#if DEBUG >= 3
            printf("coalesce returning: ");
            print_block(block);
#endif
        }
        
        // coalesce didn't work, need to sbrk more memory
        else
		{
            #if DEBUG >= 3
            printf("Coalesce did not work... need to SBRK!\n");
			#endif
			#if DEBUG >= 1
			printf("Can't find %lu of memory after coalesce.  sbrking...\n", (unsigned long) size);
			#endif
			#if DEBUG >= 2
			print_all_free();
            #endif
            
            // increase sbrkCounter to sufficent size (doubling each time)
            for( ; block_size > sbrkCounter; sbrkCounter *= 2) {}
            
            // sbrk more memory and add it to the free lists
            block = mem_sbrk(sbrkCounter);
            heap_hi = mem_heap_hi();
            *block = (header) heap_hi - (header) block + 1;
            MARK_FREE(block);
            add_to_freelist(block);
            
            #if DEBUG >= 3
            printf("SBROKE %d...\n", sbrkCounter);
            #endif
        }
    }
    
	// we are sure block is of sufficient size, so we can allocate
    void * ptr = allocate(block, block_size);
#if DEBUG >= 3
	if(!mm_check())
		assert(0);
#endif
    return ptr;
}

/*
 * mm_free - Free a block by marking it as free and putting it back into
 *  one of the free lists.
 */
void mm_free(void *ptr)
{
    header * block = HEADER(ptr);
    
#if DEBUG >= 3
    printf("MM_FREE %p\n", ptr);
	printf("Freeing %p, w/ size %lu\n", ptr, 
		   (long unsigned int) GET_SIZE(block));
#endif

	MARK_FREE(block);
	add_to_freelist(block);
	
#if DEBUG >= 3
	if(!mm_check())
		assert(0);
#endif
}

/*
 * mm_realloc - 
 * 1) If new size is less than old size, then we just split
 * 2) Otherwise, we first try to coalesce any free blocks that follow
 * 3) If that doesn't work, a full malloc/copy is required
 */
void *mm_realloc(void *ptr, size_t size)
{
    void * oldptr = ptr;    
    header * old_block = HEADER(ptr);
    
    size_t block_size = ALIGN(size + SIZE_T_SIZE);
    if (block_size < MIN_BLOCK_SIZE)
        block_size = MIN_BLOCK_SIZE;
    
    // size = 0 -> free
    if (size == 0) 
	{
        mm_free(ptr);
        return NULL;
    }
    else if (block_size > GET_SIZE(old_block))
	{
        // try to coalesce first,
        header * next = NEXT(old_block);
        while(next != NULL && GET_SIZE(next) >= MIN_BLOCK_SIZE 
			  && next < heap_hi && IS_ALLOCED(next) == 0)
		{
            *old_block += GET_SIZE(next);
			remove_from_llist(next);
            next = NEXT(old_block);
        }
        
        // if coalesce didn't get enough, then have to malloc/copy, & we're done
        if (block_size > GET_SIZE(old_block))
		{
            void * newptr = mm_malloc(size);
            if (newptr == NULL)
                return NULL;
            size_t copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
            if (size < copySize)
                copySize = size;
            memcpy(newptr, oldptr, copySize);
            mm_free(oldptr);
            return newptr;
        }
    }
        
    // If new block size is less than old, we just split off remainder and mark it as free
    if (block_size <= GET_SIZE(old_block))
	{
        size_t split_size = block_size - size;
        
        if(split_size >= MIN_BLOCK_SIZE) {
            // split the block in two
            *old_block = block_size;
            header * split = NEXT(old_block); //second half of the split block
            *split = split_size;
            
            // add split block to free list
            MARK_FREE(split);
            add_to_freelist(split);
        } 
    }
    
    MARK_ALLOC(old_block);
    return oldptr;
}


/*
 * mm_check - Returns 1 if everything is consistent. Checks for:
 * 1) all blocks are at least size MIN_BLOCK_SIZE
 * 2) blocks in free list are indeed all free
 */
int mm_check()
{
	printf("\nChecking consistency of the heap...\n");
	header * cur = mem_start;
    
	// All blocks are at least MIN_BLOCK_SIZE
    while(cur < heap_hi - 7)
	{
		size_t s = GET_SIZE(cur);
		if(*cur < MIN_BLOCK_SIZE || ALIGN(s) != s)
		{
			printf("ERROR: block at %p has size %lu\n", 
				   cur, (unsigned long) s);
			
            printf("ERROR: block at %p has size %lu\n", 
				   cur, (unsigned long) s);
            print_block(cur);
            return 0;
		}
		cur = NEXT(cur);
	}
    
    // blocks in free list are all free
    int i;
    for(i = MIN_BLOCK_SIZE / ALIGNMENT - 1; i < SMALL_LIST_SIZE; i++)
	{
        for(cur = NEXT_FREE(small_free[i]); cur != NULL; cur = NEXT_FREE(cur))
		{
			if (IS_ALLOCED(cur) == 1) {
                printf("ERROR: block at %p is in free list but not free\n", cur);
                print_block(cur);
                return 0;
            }
		}
	}
	
	for(i = 0; i < LARGE_LIST_SIZE; i++)
	{
		for(cur = NEXT_FREE(large_free[i]); cur != NULL; cur = NEXT_FREE(cur))
		{
			if (IS_ALLOCED(cur) == 1) {
                printf("ERROR: block at %p is in free list but not free\n", cur);
                print_block(cur);
                return 0;
            }
		}
	}
    
	return 1;    
}

/*
 * print_heap - Prints all blocks
 */
void print_heap() 
{
    printf("***********\nCURRENT MEMORY: \nHeap %p:%p\n************\n", heap_lo, heap_hi);
    header * cur = mem_start;
    while (cur < heap_hi-7) {
        print_block(cur);
        cur = NEXT(cur);
    }
    printf("\n\n");
}

/*
 * print_block - Prints detailed information of particular block
 */
void print_block(header * block)
{
	printf("%p | prev: %8p | next: %8p | size: %6u %x | allocated: %1lu\n", 
		   block, PREV_FREE(block), NEXT_FREE(block),
		   GET_SIZE(block), GET_SIZE(block), (long unsigned int) IS_ALLOCED(block)
	);
}

/*
 * print_all_free() - Prints all free memory blocks in free lists
 */
void print_all_free() {
    int i; header * cur;
	size_t total = 0;
    printf("****\n");
    
    for(i = MIN_BLOCK_SIZE / ALIGNMENT - 1; i < SMALL_LIST_SIZE; i++)
	{
		int n = 0;
        printf("small free blocks of size %d * 8: ", i+1);
        for(cur = NEXT_FREE(small_free[i]); cur != NULL; cur = NEXT_FREE(cur))
		{
			total += *cur;
			n++;
			#if DEBUG >= 3
            printf("%p ", cur);
			#endif
		}
        printf(" (%d nodes)\n", n);
	}
	
	for(i = 0; i < LARGE_LIST_SIZE; i++)
	{
		int n = 0;
        printf("large free blocks of size > %d: ", 256 * (0x1 << i));
		for(cur = NEXT_FREE(large_free[i]); cur != NULL; cur = NEXT_FREE(cur))
		{
			total += *cur;
			n++;
			#if DEBUG >=3
            printf("%p ", cur);
			#endif
		}
        printf(" (%d nodes)\n", n);
	}
	printf("TOTAL FREE MEMORY: %lu\n", (unsigned long) total);
}