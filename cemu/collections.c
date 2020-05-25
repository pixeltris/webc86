/** 
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "collections.h"

struct map_node_t {
  unsigned hash;
  void *value;
  map_node_t *next;
  /* char key[]; */
  /* char value[]; */
};


static unsigned map_hash(const char *str) {
  unsigned hash = 5381;
  while (*str) {
    hash = ((hash << 5) + hash) ^ *str++;
  }
  return hash;
}


static map_node_t *map_newnode(const char *key, void *value, int vsize) {
  map_node_t *node;
  int ksize = strlen(key) + 1;
  int voffset = ksize + ((sizeof(void*) - ksize) % sizeof(void*));
  node = malloc(sizeof(*node) + voffset + vsize);
  if (!node) return NULL;
  memcpy(node + 1, key, ksize);
  node->hash = map_hash(key);
  node->value = ((char*) (node + 1)) + voffset;
  memcpy(node->value, value, vsize);
  return node;
}


static int map_bucketidx(map_base_t *m, unsigned hash) {
  /* If the implementation is changed to allow a non-power-of-2 bucket count,
   * the line below should be changed to use mod instead of AND */
  return hash & (m->nbuckets - 1);
}


static void map_addnode(map_base_t *m, map_node_t *node) {
  int n = map_bucketidx(m, node->hash);
  node->next = m->buckets[n];
  m->buckets[n] = node;
}


static int map_resize(map_base_t *m, int nbuckets) {
  map_node_t *nodes, *node, *next;
  map_node_t **buckets;
  int i; 
  /* Chain all nodes together */
  nodes = NULL;
  i = m->nbuckets;
  while (i--) {
    node = (m->buckets)[i];
    while (node) {
      next = node->next;
      node->next = nodes;
      nodes = node;
      node = next;
    }
  }
  /* Reset buckets */
  buckets = realloc(m->buckets, sizeof(*m->buckets) * nbuckets);
  if (buckets != NULL) {
    m->buckets = buckets;
    m->nbuckets = nbuckets;
  }
  if (m->buckets) {
    memset(m->buckets, 0, sizeof(*m->buckets) * m->nbuckets);
    /* Re-add nodes to buckets */
    node = nodes;
    while (node) {
      next = node->next;
      map_addnode(m, node);
      node = next;
    }
  }
  /* Return error code if realloc() failed */
  return (buckets == NULL) ? -1 : 0;
}


static map_node_t **map_getref(map_base_t *m, const char *key) {
  unsigned hash = map_hash(key);
  map_node_t **next;
  if (m->nbuckets > 0) {
    next = &m->buckets[map_bucketidx(m, hash)];
    while (*next) {
      if ((*next)->hash == hash && !strcmp((char*) (*next + 1), key)) {
        return next;
      }
      next = &(*next)->next;
    }
  }
  return NULL;
}


void map_deinit_(map_base_t *m) {
  map_node_t *next, *node;
  int i;
  i = m->nbuckets;
  while (i--) {
    node = m->buckets[i];
    while (node) {
      next = node->next;
      free(node);
      node = next;
    }
  }
  // Modified from the original source to allow deinit on a memzero'd map
  // See https://github.com/rxi/map/issues/11 - "map_deinit crash after map_init without add any values #11"
  if (m->buckets) {
    free(m->buckets);
  }
}


void *map_get_(map_base_t *m, const char *key) {
  map_node_t **next = map_getref(m, key);
  return next ? (*next)->value : NULL;
}


int map_set_(map_base_t *m, const char *key, void *value, int vsize) {
  int n, err;
  map_node_t **next, *node;
  /* Find & replace existing node */
  next = map_getref(m, key);
  if (next) {
    memcpy((*next)->value, value, vsize);
    return 0;
  }
  /* Add new node */
  node = map_newnode(key, value, vsize);
  if (node == NULL) goto fail;
  if (m->nnodes >= m->nbuckets) {
    n = (m->nbuckets > 0) ? (m->nbuckets << 1) : 1;
    err = map_resize(m, n);
    if (err) goto fail;
  }
  map_addnode(m, node);
  m->nnodes++;
  return 0;
  fail:
  if (node) free(node);
  return -1;
}


void map_remove_(map_base_t *m, const char *key) {
  map_node_t *node;
  map_node_t **next = map_getref(m, key);
  if (next) {
    node = *next;
    *next = (*next)->next;
    free(node);
    m->nnodes--;
  }
}


map_iter_t map_iter_(void) {
  map_iter_t iter;
  iter.bucketidx = -1;
  iter.node = NULL;
  return iter;
}


const char *map_next_(map_base_t *m, map_iter_t *iter) {
  if (iter->node) {
    iter->node = iter->node->next;
    if (iter->node == NULL) goto nextBucket;
  } else {
    nextBucket:
    do {
      if (++iter->bucketidx >= m->nbuckets) {
        return NULL;
      }
      iter->node = m->buckets[iter->bucketidx];
    } while (iter->node == NULL);
  }
  return (char*) (iter->node + 1);
}

void map_free_values_and_deinit_(map_void_t* map)
{
    while (1)
    {
        map_iter_t iter = map_iter(map);
        const char* key = map_next(map, &iter);
        if (key == NULL)
        {
            break;
        }
        void** ptr = map_get(map, key);
        if (ptr != NULL)
        {
            free(*ptr);
        }
        map_remove(map, key);
    }
    map_deinit(map);
}


/////////////////////////////////
// idpool
/////////////////////////////////

void idpool_init(idpool* pool, uint32_t startId)
{
    memset(pool, 0, sizeof(*pool));
    pool->counter = startId;
}

void idpool_destroy(idpool* pool)
{
    if (pool->freeIds != NULL)
    {
        free(pool->freeIds);
    }
    memset(pool, 0, sizeof(*pool));
}

void idpool_resize(idpool* pool)
{
    if (pool->freeIds == NULL)
    {
        pool->freeIdsSize = 128;
        pool->freeIds = malloc(pool->freeIdsSize * sizeof(uint32_t));
        if (!pool->freeIds)
        {
            assert(0);// TODO
        }
    }
    else
    {
        pool->freeIdsSize *= 2;
        void* freeIds = realloc(pool->freeIds, pool->freeIdsSize * sizeof(uint32_t));
        if (!freeIds)
        {
            assert(0);// TODO
        }
        pool->freeIds = freeIds;
    }
}

uint32_t idpool_get(idpool* pool)
{
    if (pool->numFreeIds > 0)
    {
        pool->numFreeIds--;
        return pool->freeIds[pool->numFreeIds];
    }
    return pool->counter++;
}

void idpool_return(idpool* pool, uint32_t value)
{
    if (pool->numFreeIds + 1 > pool->freeIdsSize)
    {
        idpool_resize(pool);
    }
    pool->numFreeIds++;
    pool->freeIds[pool->numFreeIds] = value;
}




/////////////////////////////////
// HandleCollection
/////////////////////////////////

#define HANDLE_ID_STR_LEN 12

void handles_init(HandleCollection* handles)
{
    memset(handles, 0, sizeof(*handles));
    idpool_init(&handles->Pool, 1);// Starting ID of 1 (0 is treated as a null handle)
}

void handles_destroy(HandleCollection* handles)
{
    idpool_destroy(&handles->Pool);
    map_deinit(&handles->HandleMap);
}

uint32_t handles_create(HandleCollection* handles, void* handle)
{
    uint32_t handleId = idpool_get(&handles->Pool);
    
    char idStr[HANDLE_ID_STR_LEN];
    sprintf(idStr, "%u", handleId);
    map_set(&handles->HandleMap, idStr, handle);
    
    return handleId;
}

void handles_remove(HandleCollection* handles, uint32_t handleId)
{
    char idStr[HANDLE_ID_STR_LEN];
    sprintf(idStr, "%u", handleId);
    map_remove(&handles->HandleMap, idStr);
}

void* handles_find(HandleCollection* handles, uint32_t handleId)
{
    char idStr[HANDLE_ID_STR_LEN];
    sprintf(idStr, "%u", handleId);
    void** result = map_get(&handles->HandleMap, idStr);
    return result ? *result : NULL;
}




/////////////////////////////////
// Misc
/////////////////////////////////

void strtolower(char* str)
{
    while (*str = tolower(*str))
    {
        str++;
    }
}

void strcpylower(char* dest, const char* src)
{
    while (*dest = tolower(*src))
    {
        dest++;
        src++;
    }
}

const char* path_to_short_path(const char* path)
{
    const char* fileNameWithoutPath = path;
    const char* c = path;
    while (*c != '\0')
    {
        if ((*c == '\\' || *c == '/'))
        {
            fileNameWithoutPath = c + 1;
        }
        c++;
    }
    if (*fileNameWithoutPath == '\0')
    {
        fileNameWithoutPath = path;
    }
    return fileNameWithoutPath;
}

char* path_to_short_path_no_extension(char* path)
{
    char* shortPath = (char*)path_to_short_path(path);
    while (*shortPath)
    {
        if (*shortPath == '.')
        {
            *shortPath = '\0';
        }
    }
    return shortPath;
}