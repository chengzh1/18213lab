/*
 * cache.h -- Declaretion of some cache related method 
 *			  and variable for 15-213 proxy lab
 *
 * Team Member1: Cheng Zhang, Andrew ID: chengzh1
 * Team Member2: Zhe Qian, Andrew ID: zheq
 *
 */

#ifndef CACHE_H
#define CACHE_H

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Definition of cache block */
typedef struct cacheblock
{
	char *id;
    unsigned int block_size;
    char *content;
    struct cacheblock *next;
    struct cacheblock *prev;
}cache_block;

/* Definition of cache list */
typedef struct
{
	unsigned int total_size;
	cache_block *head;
	cache_block *tail;
}cache_list;

/* Declaration of some method that is used in proxy.c */
void init_cache_list(cache_list *cl);
cache_block *find_cache(cache_list *cl, char *id);
void modify_cache(cache_list *cl, char *id, char *content,  
				  unsigned int block_size);
void free_cache_list(cache_list *cl);
char* read_cache(cache_list *cl, char *id, int* size);

#endif