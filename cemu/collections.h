// Taken 14th June 2019 - https://github.com/rxi/map
// Last commit 19th Sept 2014 - https://github.com/rxi/map/tree/d6c355fcfb35e5bf71905c0c28ccee34694b4a95
/** 
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#ifndef MAP_H
#define MAP_H

#include <string.h>
#include <stdint.h>

#define MAP_VERSION "0.1.0"

struct map_node_t;
typedef struct map_node_t map_node_t;

typedef struct {
  map_node_t **buckets;
  unsigned nbuckets, nnodes;
} map_base_t;

typedef struct {
  unsigned bucketidx;
  map_node_t *node;
} map_iter_t;


#define map_t(T)\
  struct { map_base_t base; T *ref; T tmp; }


#define map_init(m)\
  memset(m, 0, sizeof(*(m)))


#define map_deinit(m)\
  map_deinit_(&(m)->base)


#define map_get(m, key)\
  ( (m)->ref = map_get_(&(m)->base, key) )


#define map_set(m, key, value)\
  ( (m)->tmp = (value),\
    map_set_(&(m)->base, key, &(m)->tmp, sizeof((m)->tmp)) )


#define map_remove(m, key)\
  map_remove_(&(m)->base, key)


#define map_iter(m)\
  map_iter_()


#define map_next(m, iter)\
  map_next_(&(m)->base, iter)


void map_deinit_(map_base_t *m);
void *map_get_(map_base_t *m, const char *key);
int map_set_(map_base_t *m, const char *key, void *value, int vsize);
void map_remove_(map_base_t *m, const char *key);
map_iter_t map_iter_(void);
const char *map_next_(map_base_t *m, map_iter_t *iter);


typedef map_t(void*) map_void_t;
typedef map_t(char*) map_str_t;
typedef map_t(int) map_int_t;
typedef map_t(uint32_t) map_uint32_t;
typedef map_t(int32_t) map_int32_t;
typedef map_t(char) map_char_t;
typedef map_t(float) map_float_t;
typedef map_t(double) map_double_t;

void map_free_values_and_deinit_(map_void_t* map);
#define map_free_values_and_deinit(m)\
  map_free_values_and_deinit_((map_void_t*)m)

#endif




#ifndef WEBC_ID_POOL_H
#define WEBC_ID_POOL_H

typedef struct
{
    uint32_t counter;
    uint32_t numFreeIds;
    uint32_t* freeIds;
    uint32_t freeIdsSize;// Number of elements in the freeIds array
} idpool;

void idpool_init(idpool* pool, uint32_t startId);
void idpool_destroy(idpool* pool);
void idpool_resize(idpool* pool);
uint32_t idpool_get(idpool* pool);
void idpool_return(idpool* pool, uint32_t value);

#endif




#ifndef WEBC_HANDLE_COLLECTION_H
#define WEBC_HANDLE_COLLECTION_H

typedef struct
{
    idpool Pool;
    map_void_t HandleMap;// id -> handle (where 'id' is the id from the pool, and 'handle' is our desired void* handle)
} HandleCollection;

void handles_init(HandleCollection* handles);
void handles_destroy(HandleCollection* handles);
uint32_t handles_create(HandleCollection* handles, void* handle);
void handles_remove(HandleCollection* handles, uint32_t handleId);
void* handles_find(HandleCollection* handles, uint32_t handleId);

#endif





#ifndef WEBC_MISC_HELPERS_H
#define WEBC_MISC_HELPERS_H

// Some unrelated string helper functions (for a lack of a better place to put these)

void strtolower(char* str);
void strcpylower(char* dest, const char* src);
const char* path_to_short_path(const char* path);
char* path_to_short_path_no_extension(char* path);// Modifies the input string

#endif