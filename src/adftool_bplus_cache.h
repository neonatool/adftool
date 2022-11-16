#ifndef H_ADFTOOL_BPLUS_CACHE_INCLUDED
#define H_ADFTOOL_BPLUS_CACHE_INCLUDED

#include <adftool_private.h>

/* Fetching B+ nodes by ID in an HDF5 file is slow, so we want to keep
   them in memory to avoid reading again the same part of the file. */

struct adftool_bplus_cache_row
{
  /* There are less cache rows than nodes in the tree, so there will
     be cache collisions. The policy is to replace in case of a
     collision. */
  uint32_t node;
  /* If node is ((uint32_t) (-1)), then it means the row is unused. */
  uint32_t *row;
  /* There are 2 * order + 1 entries, where order is known in the
     adftool_bplus_cache object. It may be NULL if not allocated, but
     if node is not ((uint32_t) (-1)), then it is allocated and
     readable. */
};

struct bplus_cache
{
  size_t max_cache_ints;
  /* If 0, then the rows are not initialized. Otherwise, you can check
     the "node" field of each row and access their cells if it is
     used. If the order starts to change, then the cache will be
     wiped. */
  size_t order;
  struct adftool_bplus_cache_row rows[8191];
};

static inline void bplus_cache_init (struct bplus_cache *cache);
static inline void bplus_cache_destroy (struct bplus_cache *cache);
static inline void bplus_cache_set_order (struct bplus_cache *cache,
					  size_t order);

/* Fetch will return 1 if the node is not in cache. Otherwise,
   *actual_length and *response will be set. */
static inline int bplus_cache_fetch (const struct bplus_cache *cache,
				     uint32_t id, size_t *actual_length,
				     size_t request_start,
				     size_t request_length,
				     uint32_t * response);

/* Store will update the cache if id is already present in it, or if
   all the cache row is given as an argument (start == 0 and length ==
   2 * order + 1). */
static inline void bplus_cache_store (struct bplus_cache *cache, uint32_t id,
				      size_t start, size_t length,
				      const uint32_t * row);

static inline void
bplus_cache_init (struct bplus_cache *cache)
{
  cache->order = 0;
  for (size_t i = 0; i < sizeof (cache->rows) / sizeof (cache->rows[0]); i++)
    {
      cache->rows[i].node = ((uint32_t) (-1));
      cache->rows[i].row = NULL;
    }
}

static inline void
bplus_cache_destroy (struct bplus_cache *cache)
{
  bplus_cache_set_order (cache, 0);
}

static inline void
bplus_cache_set_order (struct bplus_cache *cache, size_t order)
{
  if (order == 0 || order != cache->order)
    {
      for (size_t i = 0; i < sizeof (cache->rows) / sizeof (cache->rows[0]);
	   i++)
	{
	  free (cache->rows[i].row);
	  cache->rows[i].row = NULL;
	  cache->rows[i].node = ((uint32_t) (-1));
	}
    }
  cache->order = order;
}

static inline int
bplus_cache_fetch (const struct bplus_cache *cache, uint32_t id,
		   size_t *actual_length, size_t request_start,
		   size_t request_length, uint32_t * response)
{
  /* FIXME: bad hash function */
  const size_t row_id = id % (sizeof (cache->rows) / sizeof (cache->rows[0]));
  assert (id != ((uint32_t) (-1)));
  if (cache->rows[row_id].node == id)
    {
      /* Since id is not -1, and it advertises a node that’s equal to
         id (thus, not -1), then the row is allocated. */
      *actual_length = 2 * cache->order + 1;
      if (request_start > *actual_length)
	{
	  request_start = *actual_length;
	}
      if (request_start + request_length > *actual_length)
	{
	  request_length = *actual_length - request_start;
	}
      if (request_length != 0)
	{
	  const uint32_t *full_row = cache->rows[row_id].row;
	  memcpy (response, &(full_row[request_start]),
		  request_length * sizeof (full_row[0]));
	}
      return 0;
    }
  return 1;
}

static inline void
bplus_cache_store (struct bplus_cache *cache, uint32_t id, size_t start,
		   size_t length, const uint32_t * row)
{
  /* FIXME: bad hash function */
  const size_t row_id = id % (sizeof (cache->rows) / sizeof (cache->rows[0]));
  assert (id != ((uint32_t) (-1)));
  const size_t row_length = 2 * cache->order + 1;
  if (cache->rows[row_id].node == id)
    {
      if (start > row_length)
	{
	  start = row_length;
	}
      if (start + length > row_length)
	{
	  length = row_length - start;
	}
      if (length != 0)
	{
	  uint32_t *dest = &(cache->rows[row_id].row[start]);
	  memcpy (dest, row, length * sizeof (uint32_t));
	}
    }
  else if (start == 0 && length == row_length)
    {
      free (cache->rows[row_id].row);
      cache->rows[row_id].row = malloc (row_length * sizeof (uint32_t));
      if (cache->rows[row_id].row == NULL)
	{
	  /* It happens, just don’t cache. */
	  cache->rows[row_id].node = ((uint32_t) (-1));
	}
      else
	{
	  cache->rows[row_id].node = id;
	  memcpy (cache->rows[row_id].row, row,
		  row_length * sizeof (uint32_t));
	}
    }
  else
    {
      /* Otherwise, don’t partially overwrite an existing cache entry. */
      free (cache->rows[row_id].row);
      cache->rows[row_id].row = NULL;
      cache->rows[row_id].node = ((uint32_t) (-1));
    }
}

#endif /* not H_ADFTOOL_BPLUS_CACHE_INCLUDED */
