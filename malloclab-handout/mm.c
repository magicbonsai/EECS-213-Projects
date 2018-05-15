/*
 * mm.c
 * 
 * This allocator utilizes the concept of explicit free list to track
 * free blocks of memory. Until all blocks are unavailable delay coalesce.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"


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


#define ALIGNMENT 8

#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))




#define MIN_BLOCK_SIZE (SIZE_T_SIZE * 3)


#define MIN_APPROX_SIZE (0x100)
#define SMALL_LIST_SIZE (MIN_APPROX_SIZE / ALIGNMENT)
#define LARGE_LIST_SIZE 23

// size of the block 
#define GET_SIZE(block_header) (*block_header & ~0x1)

// block traversal
#define LOAD(block_header) ((header **)((char *) block_header + SIZE_T_SIZE))
#define HEADER(load_ptr) ((header *)((char *) load_ptr - SIZE_T_SIZE))
#define NEXT_FREE(block_header) (*LOAD(block_header))
#define PREV_FREE(block_header) (*(LOAD(block_header)+1))



#define IS_ALLOCATED(block_header) (*block_header % 2)
#define IS_FREE(block_header) (~IS_ALLOCATED(block_header))

// next block 
#define NEXT(block_header) ((header *)((char *) block_header + GET_SIZE(block_header)))

// marking blocks
#define MARK_ALLOCATED(block_header) (*block_header = *block_header | 0x1)
#define MARK_FREE(block_header) (*block_header = (*block_header | 0x1) - 1)



typedef size_t header;

// function defs

void add_to_linked_list(header * node, header * after);
void add_to_freelist(header * freeblock);
void distribute(header * to_split);

/* Globals */


header * heap_low;
header * heap_high;

header ** small_free;
header ** large_free;
int sbrkCount;

/*
 * the beginning of the region of usable memory (since we use the beginning
 * of the heap for internally used arrays).
 */
header * memory_start;

void add_to_linked_list(header * node, header * after)
{
	header * next = NEXT_FREE(after);
	
	PREV_FREE(node) = after;
	NEXT_FREE(after) = node;
	
	NEXT_FREE(node) = next;
	if(next != NULL)
		PREV_FREE(next) = node;
}


void remove_from_linked_list(header * node)
{
    header * prevfree = PREV_FREE(node);
	header * nextfree = NEXT_FREE(node);
    
	if(prevfree != NULL)
		NEXT_FREE(prevfree) = nextfree;
	if(nextfree != NULL)
		PREV_FREE(nextfree) = prevfree;
}


/* 
 * mm_init - initialize the malloc package. set up our segmented free list
 *  headers and create the initial amount of free memory.
 */
 
 
int mm_init(void)
{

	int i;
    
    /* starting memory = 1 5 3 6 bytes */
    sbrkCount = 0x600;
	heap_low = mem_sbrk(sbrkCount);
    heap_high = mem_heap_hi();
    
	
	small_free = (header **) heap_low;
	large_free = small_free + SMALL_LIST_SIZE;

	header * current = (header *) (large_free + LARGE_LIST_SIZE + 1);
    memory_start = current;
    
	
	for(i = 0; i < SMALL_LIST_SIZE; i++)
	{
		if( (i+1) * ALIGNMENT < MIN_BLOCK_SIZE)
		{
			small_free[i] = NULL;
		} else 
		{
		
			small_free[i] = (header *) current;
			*current = MIN_BLOCK_SIZE;
			MARK_ALLOCATED(small_free[i]);
			
			PREV_FREE(small_free[i]) = NULL;
			NEXT_FREE(small_free[i]) = NULL;
			current = (size_t *) NEXT(small_free[i]);
		}
	}


	for(i = 0; i < LARGE_LIST_SIZE; i++)
	{
		large_free[i] = (header *) current;
		*current = MIN_BLOCK_SIZE;
		MARK_ALLOCATED(large_free[i]);
		
		PREV_FREE(large_free[i]) = NULL;
		NEXT_FREE(large_free[i]) = NULL;
		current = (header *) NEXT(large_free[i]);
	}
	
	
	*current = (header) heap_high - (header) current + 1;

	add_to_freelist((header *) current);
    

	return 0;
}


void add_to_freelist(header * free_block)
{
	int i;
	size_t size = (size_t) GET_SIZE(free_block);


  
	if(size <= MIN_APPROX_SIZE)
	{
		i = size / ALIGNMENT - 1;
		add_to_linked_list(free_block, small_free[i]);
	} 
    
   
    else 
	{
		for(i = 0; i < LARGE_LIST_SIZE; i ++)
		{
			if( (unsigned long) MIN_APPROX_SIZE << (i + 1) > size )
			{
				add_to_linked_list(free_block, large_free[i]);
				return;
			}
		}

	}
}


void * allocate(header * block, size_t size)
{

    remove_from_linked_list(block);
	size_t split_size = (size_t)*block - size;
    
    if(split_size >= MIN_BLOCK_SIZE)
    {

		
        *block = size;
		header * split = NEXT(block);
		*split = split_size;
        
      
        MARK_FREE(split);
        add_to_freelist(split);
    } else 
	{

		remove_from_linked_list(block);
	}
	
    MARK_ALLOCATED(block);
   
    return (void *) LOAD(block);
}

header * find_free(size_t size)
{

    int i;
    

    if (size <= MIN_APPROX_SIZE)
	{
        for (i = size/ALIGNMENT-1; 
			 i < SMALL_LIST_SIZE && NEXT_FREE(small_free[i]) == NULL;
			 i++){}
        
        if (i < SMALL_LIST_SIZE) 
		{

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
            header * current = NEXT_FREE(large_free[i]);
            while (current != NULL)
			{
                if (GET_SIZE(current) >= size)
				{
                   
                    return current;
                }
                current = NEXT_FREE(current);
            }
        }
		i++;
    }
    
 
    return NULL;
}

header * merge(size_t size)
{

    int i = MIN_BLOCK_SIZE / ALIGNMENT - 1;
    for (; i < SMALL_LIST_SIZE + LARGE_LIST_SIZE - 1; i++)
	{
        header * current = NEXT_FREE(small_free[i]);
        header * next;
        
        while (current != NULL)
		{
            next = NEXT(current);
            if (next != NULL && next < heap_high && 
				GET_SIZE(next) >= MIN_BLOCK_SIZE && 
				(long unsigned int) IS_ALLOCATED(next) == 0)
			{
                
                
                
                remove_from_linked_list(current);                
                remove_from_linked_list(next);
                
                *current += *next;
                add_to_freelist(current);
                
                if (size && *current >= size)
                    return current;
            } else
			{
                current = NEXT_FREE(current);
            }
        }
    }

	return NULL;
}



/* 
 * mm_malloc - Allocate a block by finding a free node in one of the segmented
 *     lists. Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    
    size_t block_size = ALIGN(size + SIZE_T_SIZE);
    if (block_size < MIN_BLOCK_SIZE) {block_size = MIN_BLOCK_SIZE;}
    
   
    header * block = find_free(block_size);
    
 
    if(block == NULL)
    {
		
		block = merge(block_size);
		
        if (block != NULL) {

        }
        
        
        else
		{
            
            
            
            for( ; block_size > sbrkCount; sbrkCount = sbrkCount*2) {}
            
            // sbrk more memory and add it to the free lists
            block = mem_sbrk(sbrkCount);
            heap_high = mem_heap_hi();
            *block = (header) heap_high - (header) block + 1;
            MARK_FREE(block);
            add_to_freelist(block);
            
           
        }
    }
    
	// we are sure block is of sufficient size, so we can allocate
    void * ptr = allocate(block, block_size);

    return ptr;
}


/*
 * mm_free - Free a block by marking it as free and putting it back into
 *  one of the free lists.
 */
void mm_free(void *ptr)
{
    header * block = HEADER(ptr);

	MARK_FREE(block);
	add_to_freelist(block);
	
}

/*
 * mm_realloc - 
 * 1) If new size is less than old size, then we just split
 * 2) Otherwise, we first try to merge any free blocks that follow
 * 3) If that doesn't work, a full malloc/copy is required
 */
 
 
void *mm_realloc(void *ptr, size_t size)
{
    void * ptr_old = ptr;    
    header * block_old = HEADER(ptr);
    
    size_t block_size = ALIGN(size + SIZE_T_SIZE);
    if (block_size < MIN_BLOCK_SIZE)
        block_size = MIN_BLOCK_SIZE;
    
    // size = 0 -> free
    if (size == 0) 
	{
        mm_free(ptr);
        return NULL;
    }
    else if (block_size > GET_SIZE(block_old))
	{
        // try to merge first,
        header * next = NEXT(block_old);
        while(next != NULL && GET_SIZE(next) >= MIN_BLOCK_SIZE 
			  && next < heap_high && IS_ALLOCATED(next) == 0)
		{
            *block_old += GET_SIZE(next);
			remove_from_linked_list(next);
            next = NEXT(block_old);
        }
        
        // if merge didn't get enough, then have to malloc/copy, & we're done
        if (block_size > GET_SIZE(block_old))
		{
            void * newptr = mm_malloc(size);
            if (newptr == NULL)
                return NULL;
            size_t copySize = *(size_t *)((char *)ptr_old - SIZE_T_SIZE);
            if (size < copySize)
                copySize = size;
            memcpy(newptr, ptr_old, copySize);
            mm_free(ptr_old);
            return newptr;
        }
    }
        
    // If new block size is less than old, we just split off remainder and mark it as free
    if (block_size <= GET_SIZE(block_old))
	{
        size_t split_size = block_size - size;
        
        if(split_size >= MIN_BLOCK_SIZE) {
            // split the block in two
            *block_old = block_size;
            header * split = NEXT(block_old); //second half of the split block
            *split = split_size;
            
            // add split block to free list
            MARK_FREE(split);
            add_to_freelist(split);
        } 
    }
    
    MARK_ALLOCATED(block_old);
    return ptr_old;
}







