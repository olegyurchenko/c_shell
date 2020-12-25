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
#ifndef C_CACHE_H_1607351803
#define C_CACHE_H_1607351803
/*----------------------------------------------------------------------------*/
typedef struct C_CACHE_HEADER_TAG C_CACHE;


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

C_CACHE *cache_init(void *buffer, unsigned size);
void *cache_alloc(C_CACHE *h, unsigned size);
void *cache_realloc(C_CACHE *h, void *buffer, unsigned size);
int cache_free(C_CACHE *h, void *buffer);
void cache_stat(C_CACHE *h, unsigned *allocated/*=NULL*/, unsigned *free/*=NULL*/, unsigned *block_count /*=NULL*/);


#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_CACHE_H_1607351803*/

