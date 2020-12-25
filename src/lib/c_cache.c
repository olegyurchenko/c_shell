/*----------------------------------------------------------------------------*/
/**
* @pkg c_cache
*/
/**
* Handle memory allocate/free without system call.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2020.<br>
* started 07.12.2020 16:36:43<br>
* @pkgdoc c_cache
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#include "c_cache.h"
#include <stddef.h>
/*----------------------------------------------------------------------------*/
struct C_CACHE_HEADER_TAG {
  unsigned size; /*Size of block include sizeof(C_CASHE_HEADER). bit 1 is set if block free*/
  struct C_CACHE_HEADER_TAG *next; /*point to next block or NULL if last*/
};

typedef struct C_CACHE_HEADER_TAG C_CACHE_HEADER;
/*----------------------------------------------------------------------------*/
#define SIZE_ALIGNMENT 4
#define ALIGN_SIZE(len) ( (unsigned) ((len)+(SIZE_ALIGNMENT-1)) & (unsigned) ~(SIZE_ALIGNMENT-1))
#define SIZE_MASK ((unsigned) (~1))
#define FREE_MASK ((unsigned)(1))
/*----------------------------------------------------------------------------*/
#define BLOCK_SIZE(h) (SIZE_MASK & h->size)
#define BLOCK_IS_FREE(h) (FREE_MASK & h->size)
#define BLOCK_BUFFER(h) (sizeof(C_CACHE_HEADER) + (char *)h)
/*----------------------------------------------------------------------------*/
C_CACHE *cache_init(void *buffer, unsigned size)
{
  C_CACHE *h;
  if(size <= sizeof(C_CACHE_HEADER))
    return NULL;
  h = (C_CACHE *) buffer;
  h->size = (size & SIZE_MASK) | FREE_MASK;
  h->next = NULL;
  return h;
}
/*----------------------------------------------------------------------------*/
static C_CACHE_HEADER *find_free(C_CACHE_HEADER *n, unsigned size)
{
  while(n != NULL) {
    if(BLOCK_IS_FREE(n) && BLOCK_SIZE(n) >= size)
      break;
    n = n->next;
  }
  return n;
}
/*----------------------------------------------------------------------------*/
static void free_block(C_CACHE_HEADER *n, C_CACHE_HEADER *prior)
{
  C_CACHE_HEADER *next;

  n->size |= FREE_MASK;
  if(prior != NULL && BLOCK_IS_FREE(prior)) {
    //Combine with previous block
    prior->next = n->next;
    prior->size = (BLOCK_SIZE(prior) + BLOCK_SIZE(n)) | FREE_MASK;
    n = prior;
  }

  next = n->next;
  if(next != NULL && BLOCK_IS_FREE(next)) {
    //Combine with next block
    n->next = next->next;
    n->size = (BLOCK_SIZE(next) + BLOCK_SIZE(n)) | FREE_MASK;
  }
}
/*----------------------------------------------------------------------------*/
static void alloc_block(C_CACHE_HEADER *n, unsigned size)
{
  C_CACHE_HEADER *next;
  unsigned next_size;
  void *p;

  next_size = BLOCK_SIZE(n) - size;
  if(next_size > sizeof(C_CACHE_HEADER)) {
    n->size = size;
    p = ((char *) n + size);

    next = (C_CACHE_HEADER *) p;
    next->size = next_size | FREE_MASK;
    next->next = n->next;
    n->next = next;
    free_block(next, NULL);
  }
  n->size &= ~FREE_MASK;
}
/*----------------------------------------------------------------------------*/
void *cache_alloc(C_CACHE *h, unsigned size)
{
  C_CACHE_HEADER *n;
  unsigned block_size;
  if(!size)
    return NULL;

  block_size = ALIGN_SIZE(size + sizeof(C_CACHE_HEADER));
  if(block_size < size)
    return NULL;

  n = find_free(h, block_size);

  if(n == NULL) {
    return NULL;
  }

  alloc_block(n, block_size);
  return BLOCK_BUFFER(n);
}
/*----------------------------------------------------------------------------*/
void *cache_realloc(C_CACHE *h, void *buffer, unsigned size)
{
  C_CACHE_HEADER *n, *next, *prev = NULL;
  char *src, *dst;
  unsigned i, src_size;

  if(!size)
    return NULL;

  size = ALIGN_SIZE(size + sizeof(C_CACHE_HEADER));

  n = h;
  while(n != NULL) {
    if(!BLOCK_IS_FREE(n) && BLOCK_BUFFER(n) == buffer)
      break;
    prev = n;
    n = n->next;
  }
  if(n == NULL) {
    return NULL;
  }

  if(BLOCK_SIZE(n) >= size) {
    alloc_block(n, size);
    return BLOCK_BUFFER(n);
  }

  next = n->next;
  if(next != NULL && BLOCK_IS_FREE(next) && BLOCK_SIZE(n) + BLOCK_SIZE(next) >= size) {
    //Combine 2 block
    n->next = next->next;
    n->size += BLOCK_SIZE(next);
    alloc_block(n, size);
    return BLOCK_BUFFER(n);
  }

  next = find_free(h, size);
  if(next == NULL) {
    return NULL;
  }
  alloc_block(next, size);
  src_size = BLOCK_SIZE(n) - sizeof(C_CACHE_HEADER);
  src = (char *)BLOCK_BUFFER(n);
  dst = (char *)BLOCK_BUFFER(next);
  for(i = 0; i < src_size; i++) {
    *dst = *src;
    src ++;
    dst ++;
  }

  free_block(n, prev);
  return BLOCK_BUFFER(next);
}
/*----------------------------------------------------------------------------*/
int cache_free(C_CACHE *h, void *buffer)
{
  C_CACHE_HEADER *n, *prior = NULL;
  n = h;
  while(n != NULL) {
    if(!BLOCK_IS_FREE(n) && BLOCK_BUFFER(n) == buffer)
      break;
    prior = n;
    n = n->next;
  }

  if(n == NULL) {
    return -1;
  }

  free_block(n, prior);
  return 0;
}
/*----------------------------------------------------------------------------*/
void cache_stat(C_CACHE *h, unsigned *_allocated/*=NULL*/, unsigned *_free/*=NULL*/, unsigned *_count /*=NULL*/)
{
  unsigned allocated = 0, free = 0, count = 0;
  C_CACHE_HEADER *n;

  n = h;
  while(n != NULL) {
    count ++;
    if(BLOCK_IS_FREE(n))
      free += BLOCK_SIZE(n);
    else
      allocated += BLOCK_SIZE(n);
    n = n->next;
  }

  if(_allocated != NULL)
    *_allocated = allocated;
  if(_free != NULL)
    *_free = free;
  if(_count != NULL)
    *_count = count;
}
/*----------------------------------------------------------------------------*/
