/*
 * cache.c -- Cache module for the 15-213 proxy lab
 * 
 * Team Member1: Cheng Zhang, Andrew ID: chengzh1
 * Team Member2: Zhe Qian, Andrew ID: zheq
 *
 * Overview of cache structure:
 *	The completed cache structure and some methods that 
 *	should be used in proxy.c are defined in cache.h, the 
 *  rest methods for cache are defined here. Bascially,
 *  we use a linked list to cache web content, each cache 
 *  block includes the request header as its id, the block szie,
 *  the cached content and poniter to previous and next cache block.
 *  To implemented the LRU policy, we add new cache block 
 *  at the head of the cache list, and remove old block from 
 *  the tail of the list. When there is a cache hit, we also 
 *  move this cache block to the head of the list. For thread-safe, 
 *  we lock the cache list each time we manipulate the cache block.
 */

#include "csapp.h"
#include "cache.h"

/*
 * Declaration of the methods and variables that only used
 * in cache.c
 */
static cache_block *new_cache(char *id, char *content, 
				unsigned int block_size);
static void insert_cache(cache_list *cl, cache_block *cb);
static void replace_cache(cache_list *cl, cache_block *new_cb);
static cache_block *delete_cache(cache_list *cl, cache_block *cb);
static void update_cache(cache_list *cl, cache_block *cb);
static sem_t sem;


/*
 * Initialize caceh list
 */
void init_cache_list(cache_list *cl)
{
	cl->total_size = 0;

	/* initialize the two cache block as head and tail */
	cl->head = new_cache(NULL, NULL, 0);
	cl->tail = new_cache(NULL, NULL, 0);

	cl->head->next = cl->tail;
	cl->tail->prev = cl->head;

	/* initialize lock */
	Sem_init(&sem, 0, 1); 

	return;
}

/*
 * Free cache list
 */
void free_cache_list(cache_list *cl)
{
	/* delete all cache block */
	cache_block *cb;
	for(cb = cl->tail->prev; cb != cl->head;)
	{
		cb = delete_cache(cl, cb);
	}

	/* free heap */
	free(cl->head);
	free(cl->tail);
	free(cl);
	return;
}

/*
 * Create a new cache block
 */
static cache_block *new_cache(char *id, char *content, 
				unsigned int block_size)
{
	cache_block *cb;
	cb = (cache_block *)malloc(sizeof(cache_block));

	/* 
	 * copy cache id, if id == NULL, 
	 * it is header and tail
	 */
	if (id != NULL)
	{
		cb->id = (char *) malloc(sizeof(char) * (strlen(id) + 1));
		strcpy(cb->id, id);
	}

	cb->block_size = block_size;

	/* 
	 * copy cache content, if content == NULL, 
	 * it is header and tail
	 */
	if (content != NULL)
	{
		cb->content = (char *) malloc(sizeof(char) * block_size);
		memcpy(cb->content, content, sizeof(char) * block_size);
	}

	cb->prev = NULL;
	cb->next = NULL;

	return cb;
}

/*
 * Insert a cache block into cache list
 */
static void insert_cache(cache_list *cl, cache_block *cb)
{
	/* manipulate pointer */
	cb->prev = cl->head;
	cb->next = cl->head->next;
	cl->head->next->prev = cb;
	cl->head->next = cb;

    /* change total size */
	cl->total_size += cb->block_size;
	return;
}

/*
 * Delete cache block
 */
static cache_block *delete_cache(cache_list *cl, cache_block *cb)
{
	/* remove cache block from list */
	cache_block *prev_cb;
	cb->next->prev = cb->prev;
	cb->prev->next = cb->next;
	cl->total_size -= cb->block_size;
	prev_cb = cb->prev;

	cb->prev = NULL;
	cb->next = NULL;

	/* Free heap */
	free(cb->id);
	free(cb->content);
	free(cb);

	return prev_cb;
}

/*
 * Update cache list, put a new cache block
 * to the head of the cache list.
 */
static void update_cache(cache_list *cl, cache_block *cb)
{
	/* put cache block to the head */
	cb->next->prev = cb->prev;
	cb->prev->next = cb->next;
	cb->prev = cl->head;
	cb->next = cl->head->next;
	cl->head->next = cb;
	cb->next->prev = cb;

	return;
}

/*
 * When eviction, delete cache block from tail, 
 * make room from new cache block, then insert
 * the new cache block into head of the list.
 */
static void replace_cache(cache_list *cl, cache_block *new_cb)
{
	cache_block *cb;
	
	/* delete from tail, make room for new cache block */
	for(cb = cl->tail->prev; cb != cl->head;)
	{
		cb = delete_cache(cl, cb);
		if(cl->total_size + new_cb->block_size <= MAX_CACHE_SIZE)
		{
			break;
		}
	}

	/* inster new cache block */
	insert_cache(cl, new_cb);

	return;
}

/*
 * Find a cache block in cache list by id
 */
cache_block *find_cache(cache_list *cl, char *id)
{
	cache_block *cb;

	/*
	 * serach the cache list, if there is a hit, move
	 * the cache block to the head of cache list
	 */
	for(cb = cl->head->next; cb != cl->tail; cb = cb->next)
	{
    	if(!strcmp(cb->id, id))
    	{
    		update_cache(cl, cb);
    		return cb; 
    	} 			
    }
    return NULL;
}

/*
 * Check cache list, if there exist the request content,
 * read from it.
 */
char* read_cache(cache_list *cl, char *id, int* size)
{
	cache_block *cache = NULL;

	/* 
	 * As we would manipulate the cache list 
	 * when there is cache hit, we first lock 
	 * it for thread safety
	 */
	P(&sem);
	char* content_copy;

	cache = find_cache(cl, id);

	/* if cache hit, copy the content */
	if (cache != NULL)
	{
		*size = cache->block_size;
		content_copy = (char*) malloc(sizeof(char)*cache->block_size);

		memcpy(content_copy, cache->content,
			   sizeof(char) * cache->block_size);
		V(&sem);
        return content_copy;

	}
	else
	{
		V(&sem);
		return NULL;
	}
}

/*
 * Write a new cache block to cache list
 */
void modify_cache(cache_list *cl, char *id, char *content,
		unsigned int block_size)
{
	cache_block *new_cb = NULL;

	/* 
	 * Write operation should lock the cache list
	 * for thread safety
	 */
	P(&sem);
	new_cb = new_cache(id, content, block_size);

    /* 
     * When there is enough room, insert the cache,
	 * else replace old cache block.
     */
    if(cl->total_size + block_size <= MAX_CACHE_SIZE)
    {
    	insert_cache(cl, new_cb);
    }
    else
    {
    	replace_cache(cl, new_cb);
    }

    V(&sem);
    return;

}
