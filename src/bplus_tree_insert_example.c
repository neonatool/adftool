#include <adftool_private.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <stdio.h>

#include "xalloc.h"
#include "gettext.h"

/* In this example, we maintain a map from uint32 to uint32, where the
   key is a number from 0 to 31, and the value is the iteration in
   which it has been included. We also maintain a static index, as a
   mapping from a key to the check sum of all values. We insert 100
   things in the index, checking for consistency. */

struct example_context
{
  int order;
  size_t n_nodes;
  size_t max_nodes;
  uint32_t *data;
};

static int
fetch (uint32_t row_id, size_t *actual_row_length,
       size_t request_start, size_t request_length,
       uint32_t * response, void *ctx)
{
  struct example_context *context = ctx;
  assert (row_id < context->n_nodes);
  const uint32_t *row = (context->data + (row_id * (2 * context->order + 1)));
  *actual_row_length = 2 * context->order + 1;
  size_t i;
  for (i = 0; i < request_length; i++)
    {
      if (i + request_start < 2 * ((size_t) context->order) + 1)
	{
	  response[i] = row[i + request_start];
	}
    }
  return 0;
}

static uint32_t
literal_key (const struct adftool_bplus_key *key)
{
  uint32_t index;
  int error = key_get_known (key, &index);
  assert (error == 0);
  return index;
}

static int
compare (const struct adftool_bplus_key *key_1,
	 const struct adftool_bplus_key *key_2, int *result, void *context)
{
  (void) context;
  *result = literal_key (key_1) - literal_key (key_2);
  return 0;
}

static void
allocate (uint32_t * result, void *ctx)
{
  struct example_context *context = ctx;
  if (context->n_nodes == context->max_nodes)
    {
      context->max_nodes *= 2;
      context->data =
	xrealloc (context->data,
		  (context->max_nodes * (2 * context->order + 1))
		  * sizeof (uint32_t));
    }
  *result = context->n_nodes++;
}

static void
store (uint32_t row_id, size_t start, size_t len, const uint32_t * to_store,
       void *ctx)
{
  struct example_context *context = ctx;
  assert (row_id < context->n_nodes);
  assert (start + len <= 2 * ((size_t) context->order) + 1);
  uint32_t *row = (context->data + (row_id * (2 * context->order + 1)));
  size_t i;
  for (i = 0; i < len; i++)
    {
      row[start + i] = to_store[i];
    }
}

static struct adftool_bplus *bplus = NULL;
static struct example_context ctx;

static void
one_test (int order)
{
  ctx.order = order;
  ctx.n_nodes = 1;
  ctx.max_nodes = 1;
  ctx.data = xmalloc ((ctx.max_nodes * (2 * order + 1)) * sizeof (uint32_t));
  /* First, we initialize the root. */
  int i;
  for (i = 0; i < 2 * order + 1; i++)
    {
      ctx.data[i] = 0;
    }
  for (i = 0; i + 1 < order; i++)
    {
      ctx.data[i] = -1;
    }
  ctx.data[2 * order - 1] = -1;
  uint32_t leaf_flag = 1;
  leaf_flag = leaf_flag << 31;
  ctx.data[2 * order] = leaf_flag;
  uint32_t checksums[32] = { 0 };
  uint32_t *lookup_results = xmalloc (100 * sizeof (uint32_t));
  struct adftool_bplus_key searched_key;
  for (uint32_t iteration = 0; iteration < 100; iteration++)
    {
      uint32_t key = rand () % 32;
      uint32_t value = iteration;
      /* OK so the checksum is last inserted key * 100 + number of
         insertions. It’s not great but it helps. */
      uint32_t number_of_values = (checksums[key] % 100) + 1;
      checksums[key] = value * 100 + number_of_values;
      int error = adftool_bplus_insert (key, value, bplus);
      assert (error == 0);
      /* Check consistency */
      for (uint32_t existing_key = 0; existing_key < 32; existing_key++)
	{
	  key_set_known (&searched_key, existing_key);
	  size_t n_results;
	  int lookup_error =
	    adftool_bplus_lookup (&searched_key, bplus, 0, 100, &n_results,
				  lookup_results);
	  assert (lookup_error == 0);
	  assert (n_results <= 100);
	  uint32_t n_values = 0;
	  uint32_t first_value = 0;
	  for (size_t i = 0; i < n_results; i++)
	    {
	      if (n_values == 0)
		{
		  first_value = lookup_results[i];
		}
	      n_values++;
	    }
	  assert (first_value == checksums[existing_key] / 100);
	  assert (n_values == checksums[existing_key] % 100);
	}
    }
  free (lookup_results);
  free (ctx.data);
}

int
main ()
{
  int order;
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  bplus = adftool_bplus_alloc ();
  adftool_bplus_set_fetch (bplus, fetch, &ctx);
  adftool_bplus_set_compare (bplus, compare, &ctx);
  adftool_bplus_set_allocate (bplus, allocate, &ctx);
  adftool_bplus_set_store (bplus, store, &ctx);
  for (order = 3; order < 6; order++)
    {
      int repetition;
      for (repetition = 0; repetition < 10; repetition++)
	{
	  one_test (order);
	}
    }
  adftool_bplus_free (bplus);
  bplus = NULL;
  return 0;
}
