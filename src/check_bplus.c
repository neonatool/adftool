#include <config.h>

#include <bplus.h>

#include <check.h>

#include "gettext.h"
#include "progname.h"
#include "relocatable.h"
#include <locale.h>

#include <stdio.h>
#include <stdlib.h>

#define _(String) gettext (String)
#define N_(String) (String)

static void
do_check_fetch (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (4);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_fetcher *fetcher = bplus_fetcher_alloc (tree);
  if (fetcher == NULL)
    {
      abort ();
    }
  bplus_fetcher_setup (fetcher, 42);
  size_t row_to_fetch, start, length;
  const struct bplus_node *result;
  /* The process should be waiting to read line 42. */
  result = bplus_fetcher_status (fetcher, &row_to_fetch, &start, &length);
  ck_assert_ptr_eq (result, NULL);
  ck_assert_int_eq (row_to_fetch, 42);
  ck_assert_int_eq (start, 0);
  ck_assert_int_eq (length, 9);
  const uint32_t data[9] = {
    /* Keys */
    2, 5, -1,
    /* Children */
    43, 48, 52, 0,
    /* Parent */
    21,
    /* Flags */
    0
  };
  bplus_fetcher_data (fetcher, 42, 0, sizeof (data) / sizeof (data[0]), data);
  /* The process should be finished. */
  result = bplus_fetcher_status (fetcher, NULL, NULL, NULL);
  ck_assert_ptr_ne (result, NULL);
  ck_assert_int_eq (result->n_entries, 2);
  ck_assert_int_eq (result->keys[0], 2);
  ck_assert_int_eq (result->keys[1], 5);
  ck_assert_int_eq (result->values[0], 43);
  ck_assert_int_eq (result->values[1], 48);
  ck_assert_int_eq (result->values[2], 52);
  ck_assert_int_eq (result->is_leaf, 0);
  ck_assert_int_eq (result->parent_node, 21);
  bplus_fetcher_free (fetcher);
  bplus_tree_free (tree);
}

static void
do_check_fetch_with_cache (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (4);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_fetcher *fetcher = bplus_fetcher_alloc (tree);
  if (fetcher == NULL)
    {
      abort ();
    }
  bplus_fetcher_setup (fetcher, 42);
  size_t row_to_fetch, start, length;
  const struct bplus_node *result;
  /* The process should be waiting to read line 42. */
  result = bplus_fetcher_status (fetcher, &row_to_fetch, &start, &length);
  ck_assert_ptr_eq (result, NULL);
  ck_assert_int_eq (row_to_fetch, 42);
  ck_assert_int_eq (start, 0);
  ck_assert_int_eq (length, 9);
  const uint32_t data[9] = {
    /* Keys */
    2, 5, -1,
    /* Children */
    43, 48, 52, 0,
    /* Parent */
    21,
    /* Flags */
    0
  };
  bplus_fetcher_data (fetcher, 42, 0, sizeof (data) / sizeof (data[0]), data);
  /* The process should be finished. */
  result = bplus_fetcher_status (fetcher, NULL, NULL, NULL);
  ck_assert_ptr_ne (result, NULL);
  ck_assert_int_eq (result->n_entries, 2);
  ck_assert_int_eq (result->keys[0], 2);
  ck_assert_int_eq (result->keys[1], 5);
  ck_assert_int_eq (result->values[0], 43);
  ck_assert_int_eq (result->values[1], 48);
  ck_assert_int_eq (result->values[2], 52);
  ck_assert_int_eq (result->is_leaf, 0);
  ck_assert_int_eq (result->parent_node, 21);
  /* Now retry, it should be cached. */
  bplus_fetcher_setup (fetcher, 42);
  result = bplus_fetcher_status (fetcher, NULL, NULL, NULL);
  ck_assert_ptr_ne (result, NULL);
  ck_assert_int_eq (result->n_entries, 2);
  ck_assert_int_eq (result->keys[0], 2);
  ck_assert_int_eq (result->keys[1], 5);
  ck_assert_int_eq (result->values[0], 43);
  ck_assert_int_eq (result->values[1], 48);
  ck_assert_int_eq (result->values[2], 52);
  ck_assert_int_eq (result->is_leaf, 0);
  ck_assert_int_eq (result->parent_node, 21);
  /* Now, fetch node 42 + 251 = 293. It should be a collision with
     42. */
  bplus_fetcher_setup (fetcher, 293);
  result = bplus_fetcher_status (fetcher, &row_to_fetch, &start, &length);
  ck_assert_ptr_eq (result, NULL);
  ck_assert_int_eq (row_to_fetch, 293);
  ck_assert_int_eq (start, 0);
  ck_assert_int_eq (length, 9);
  const uint32_t other_data[9] = {
    /* Keys */
    92, 3, 28,
    /* Values */
    43, 48, 52,
    /* Next leaf */
    5,
    /* Parent */
    21,
    /* Flags */
    (((uint32_t) (1)) << ((uint32_t) (31)))
  };
  bplus_fetcher_data (fetcher, 293, 0, 9, other_data);
  result = bplus_fetcher_status (fetcher, NULL, NULL, NULL);
  ck_assert_ptr_ne (result, NULL);
  ck_assert_int_eq (result->is_leaf, 1);
  /* other_data is saved to cache. */
  bplus_fetcher_setup (fetcher, 293);
  result = bplus_fetcher_status (fetcher, NULL, NULL, NULL);
  ck_assert_ptr_ne (result, NULL);
  ck_assert_int_eq (result->is_leaf, 1);
  /* And now node 42 has been evicted. */
  bplus_fetcher_setup (fetcher, 42);
  result = bplus_fetcher_status (fetcher, NULL, NULL, NULL);
  ck_assert_ptr_eq (result, NULL);
  bplus_fetcher_free (fetcher);
  bplus_tree_free (tree);
}

static void
identity_compare (struct bplus_analyzer *analyzer, int which, int *found,
		  size_t *index)
{
  int done;
  struct bplus_key a, b;
  do
    {
      analyzer_match (analyzer, which, &done, found, index, &a, &b);
      if (!done)
	{
	  ck_assert_int_eq (a.type, BPLUS_KEY_KNOWN);
	  ck_assert_int_eq (b.type, BPLUS_KEY_KNOWN);
	  analyzer_result (analyzer, &a, &b, a.arg.known - b.arg.known);
	}
    }
  while (!done);
}

static void
do_check_dichotomy (void)
{
  static const uint32_t keys[] = { 4, 5, 5, 8, 10, 12, 15, 15, 15, 15 };
  const size_t n_keys = sizeof (keys) / sizeof (keys[0]);
  struct bplus_analyzer *process = analyzer_alloc (n_keys + 1);
  if (process == NULL)
    {
      abort ();
    }
  struct bplus_key pivot = {.type = BPLUS_KEY_KNOWN };
  int found;
  size_t index;
  /* Analyze key 0 */
  pivot.arg.known = 0;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 0);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 0);
  /* Analyze key 4 */
  pivot.arg.known = 4;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 0);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 1);
  /* Analyze key 5 */
  pivot.arg.known = 5;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 1);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 3);
  /* Analyze key 6 */
  pivot.arg.known = 6;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 3);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 3);
  /* Analyze key 8 */
  pivot.arg.known = 8;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 3);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 4);
  /* Analyze key 9 */
  pivot.arg.known = 9;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 4);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 4);
  /* Analyze key 10 */
  pivot.arg.known = 10;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 4);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 5);
  /* Analyze key 11 */
  pivot.arg.known = 11;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 5);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 5);
  /* Analyze key 12 */
  pivot.arg.known = 12;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 5);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 6);
  /* Analyze key 14 */
  pivot.arg.known = 14;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 6);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 6);
  /* Analyze key 15 */
  pivot.arg.known = 15;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 6);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 1);
  ck_assert_int_eq (index, 10);
  /* Analyze key 16 */
  pivot.arg.known = 16;
  analyzer_setup (process, n_keys, keys, &pivot);
  identity_compare (process, BPLUS_ANALYZER_FIRST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 10);
  identity_compare (process, BPLUS_ANALYZER_LAST, &found, &index);
  ck_assert_int_eq (found, 0);
  ck_assert_int_eq (index, 10);
  analyzer_free (process);
}

static const uint32_t *
standard_tree_load (size_t row, size_t start, size_t length)
{
  /* This is an example tree. Draw it on paper to understand it. It is of order 3. */
  static const uint32_t rows[][7] = {
    /* Node 0: the root. */
    {
     /* Keys */
     5, 12,
     /* Children */
     1, 2, 3,
     /* Parent */
     ((uint32_t) (-1)),
     /* Flags */
     0},
    /* Nodes 1->3: children. */
    {
     /* Keys */
     2, ((uint32_t) (-1)),
     /* Children */
     4, 5, 0,
     /* Parent */
     0,
     /* Flags */
     0},
    {
     /* Keys */
     10, 10,
     /* Children */
     6, 7, 8,
     /* Parent */
     0,
     /* Flags */
     0},
    {
     /* Keys */
     15, ((uint32_t) (-1)),
     /* Children */
     9, 10,
     /* Parent */
     0,
     /* Flags */
     0},
    /* Nodes 4->10: the leaves. */
    {
     /* Keys */
     1, 2,
     /* Values */
     1, 2,
     /* Next leaf */
     5,
     /* Parent */
     1,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     3, 5,
     /* Values */
     3, 5,
     /* Next leaf */
     6,
     /* Parent */
     1,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     9, 10,
     /* Values */
     9, 10,
     /* Next leaf */
     7,
     /* Parent */
     2,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     10, 10,
     /* Values */
     10, 10,
     /* Next leaf */
     8,
     /* Parent */
     2,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     11, 12,
     /* Values */
     11, 12,
     /* Next leaf */
     9,
     /* Parent */
     2,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     13, 15,
     /* Values */
     13, 15,
     /* Next leaf */
     10,
     /* Parent */
     3,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     20, ((uint32_t) (-1)),
     /* Values */
     20, 0,
     /* Next leaf */
     0,
     /* Parent */
     3,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
  };
  ck_assert_int_lt (row, sizeof (rows) / sizeof (rows[0]));
  const uint32_t *requested_row = rows[row];
  size_t max = sizeof (rows[row]) / sizeof (rows[row][0]);
  ck_assert_int_le (start + length, max);
  return &(requested_row[start]);
}

static struct bplus_explorer *
do_check_explorer (struct bplus_tree *tree, uint32_t first_node,
		   uint32_t last_node, uint32_t key)
{
  struct bplus_key full_key;
  full_key.type = BPLUS_KEY_KNOWN;
  full_key.arg.known = key;
  struct bplus_explorer *explorer = explorer_alloc (tree);
  if (explorer == NULL)
    {
      abort ();
    }
  struct bplus_fetcher *first_fetcher = bplus_fetcher_alloc (tree);
  struct bplus_fetcher *last_fetcher = bplus_fetcher_alloc (tree);
  if (first_fetcher == NULL || last_fetcher == NULL)
    {
      abort ();
    }
  size_t row_to_fetch, start, length;
  const struct bplus_node *fetched_first_node = NULL;
  const struct bplus_node *fetched_last_node = NULL;
  bplus_fetcher_setup (first_fetcher, first_node);
  bplus_fetcher_setup (last_fetcher, last_node);
  while ((fetched_first_node =
	  bplus_fetcher_status (first_fetcher, &row_to_fetch, &start,
				&length)) == NULL)
    {
      bplus_fetcher_data (first_fetcher, row_to_fetch, start, length,
			  standard_tree_load (row_to_fetch, start, length));
    }
  while ((fetched_last_node =
	  bplus_fetcher_status (last_fetcher, &row_to_fetch, &start,
				&length)) == NULL)
    {
      bplus_fetcher_data (last_fetcher, row_to_fetch, start, length,
			  standard_tree_load (row_to_fetch, start, length));
    }
  int setup_error =
    explorer_setup (explorer, &full_key, first_node, fetched_first_node,
		    last_node, fetched_last_node);
  bplus_fetcher_free (last_fetcher);
  bplus_fetcher_free (first_fetcher);
  if (setup_error)
    {
      abort ();
    }
  struct bplus_explorer_status status;
  size_t b_rows_to_fetch[2];
  size_t b_row_starts[2];
  size_t b_row_lengths[2];
  struct bplus_key b_as[2];
  struct bplus_key b_bs[2];
  status.to_fetch.start = 0;
  status.to_fetch.max = 2;
  status.to_fetch.rows_to_fetch = b_rows_to_fetch;
  status.to_fetch.row_starts = b_row_starts;
  status.to_fetch.row_lengths = b_row_lengths;
  status.to_compare.start = 0;
  status.to_compare.max = 2;
  status.to_compare.as = b_as;
  status.to_compare.bs = b_bs;
  do
    {
      explorer_status (explorer, &status);
      if (!status.done)
	{
	  for (size_t i = 0; i < status.to_fetch.n; i++)
	    {
	      size_t rq_row = status.to_fetch.rows_to_fetch[i];
	      size_t rq_start = status.to_fetch.row_starts[i];
	      size_t rq_length = status.to_fetch.row_lengths[i];
	      explorer_data (explorer, rq_row, rq_start, rq_length,
			     standard_tree_load (rq_row, rq_start,
						 rq_length));
	    }
	  for (size_t i = 0; i < status.to_compare.n; i++)
	    {
	      struct bplus_key *a = &(status.to_compare.as[i]);
	      struct bplus_key *b = &(status.to_compare.bs[i]);
	      ck_assert_int_eq (a->type, BPLUS_KEY_KNOWN);
	      ck_assert_int_eq (b->type, BPLUS_KEY_KNOWN);
	      explorer_compared (explorer, a, b, a->arg.known - b->arg.known);
	    }
	}
    }
  while (!status.done);
  return explorer;
}

static void
do_check_explore_root (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_explorer_status status;
  status.to_fetch.start = 0;
  status.to_fetch.max = 0;
  status.to_compare.start = 0;
  status.to_compare.max = 0;
  /* Check for key 0 at the root. */
  struct bplus_explorer *explorer = do_check_explorer (tree, 0, 0, 0);
  explorer_status (explorer, &status);
  ck_assert_int_eq (status.done, 1);
  ck_assert_int_eq (status.first_id, 1);
  ck_assert_int_eq (status.last_id, 1);
  /* Check that we correctly loaded child 1. */
  ck_assert_int_eq (status.first_node->n_entries, 1);
  ck_assert_int_eq (status.first_node->keys[0], 2);
  ck_assert_int_eq (status.last_node->n_entries, 1);
  ck_assert_int_eq (status.last_node->keys[0], 2);
  explorer_free (explorer);
  /* Check for key 5 at the root: now the "last" branch takes the next
     child. */
  explorer = do_check_explorer (tree, 0, 0, 5);
  explorer_status (explorer, &status);
  ck_assert_int_eq (status.done, 1);
  ck_assert_int_eq (status.first_id, 1);
  ck_assert_int_eq (status.last_id, 2);
  /* Check that we correctly loaded children 1 and 2. */
  ck_assert_int_eq (status.first_node->n_entries, 1);
  ck_assert_int_eq (status.first_node->keys[0], 2);
  ck_assert_int_eq (status.last_node->n_entries, 2);
  ck_assert_int_eq (status.last_node->keys[0], 10);
  explorer_free (explorer);
  /* Check for key 15 at the root. */
  explorer = do_check_explorer (tree, 0, 0, 15);
  explorer_status (explorer, &status);
  ck_assert_int_eq (status.done, 1);
  ck_assert_int_eq (status.first_id, 3);
  ck_assert_int_eq (status.last_id, 3);
  /* Check that we correctly loaded child 3. */
  ck_assert_int_eq (status.first_node->n_entries, 1);
  ck_assert_int_eq (status.first_node->keys[0], 15);
  ck_assert_int_eq (status.last_node->n_entries, 1);
  ck_assert_int_eq (status.last_node->keys[0], 15);
  explorer_free (explorer);
  bplus_tree_free (tree);
}

static void
do_check_explore_splitted (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_explorer_status status;
  status.to_fetch.start = 0;
  status.to_fetch.max = 0;
  status.to_compare.start = 0;
  status.to_compare.max = 0;
  /* Check for key 10 between all root child. */
  struct bplus_explorer *explorer = do_check_explorer (tree, 1, 3, 10);
  explorer_status (explorer, &status);
  ck_assert_int_eq (status.done, 1);
  ck_assert_int_eq (status.first_id, 5);
  ck_assert_int_eq (status.last_id, 9);
  /* Check that we correctly loaded children 5 and 9. */
  ck_assert_int_eq (status.first_node->n_entries, 2);
  ck_assert_int_eq (status.first_node->keys[0], 3);
  ck_assert_int_eq (status.last_node->n_entries, 2);
  ck_assert_int_eq (status.last_node->keys[0], 13);
  explorer_free (explorer);
  bplus_tree_free (tree);
}

static void
do_check_explore_splits (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_explorer_status status;
  status.to_fetch.start = 0;
  status.to_fetch.max = 0;
  status.to_compare.start = 0;
  status.to_compare.max = 0;
  /* Check for key 10 in the second root child. */
  struct bplus_explorer *explorer = do_check_explorer (tree, 2, 2, 10);
  explorer_status (explorer, &status);
  ck_assert_int_eq (status.done, 1);
  ck_assert_int_eq (status.first_id, 6);
  ck_assert_int_eq (status.last_id, 8);
  /* Check that we correctly loaded children 6 and 8. */
  ck_assert_int_eq (status.first_node->n_entries, 2);
  ck_assert_int_eq (status.first_node->keys[0], 9);
  ck_assert_int_eq (status.last_node->n_entries, 2);
  ck_assert_int_eq (status.last_node->keys[0], 11);
  explorer_free (explorer);
  bplus_tree_free (tree);
}

static const struct bplus_node *
satisfy_fetcher (struct bplus_fetcher *fetcher)
{
  size_t row_to_fetch, start, length;
  const struct bplus_node *ret;
  while ((ret =
	  bplus_fetcher_status (fetcher, &row_to_fetch, &start,
				&length)) == NULL)
    {
      bplus_fetcher_data (fetcher, row_to_fetch, start, length,
			  standard_tree_load (row_to_fetch, start, length));
    }
  return ret;
}

static void
do_check_range (void)
{
  /* Check that we iterate over all records, except 1 -> 1. */
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_fetcher *first_leaf_fetcher = bplus_fetcher_alloc (tree);
  struct bplus_fetcher *last_leaf_fetcher = bplus_fetcher_alloc (tree);
  if (first_leaf_fetcher == NULL || last_leaf_fetcher == NULL)
    {
      abort ();
    }
  bplus_fetcher_setup (first_leaf_fetcher, 4);
  bplus_fetcher_setup (last_leaf_fetcher, 10);
  struct bplus_range *range = bplus_range_alloc (tree);
  if (range == NULL)
    {
      abort ();
    }
  int setup_err =
    bplus_range_setup (range, 4, satisfy_fetcher (first_leaf_fetcher), 1, 10,
		       satisfy_fetcher (last_leaf_fetcher), 1);
  ck_assert_int_eq (setup_err, 0);
  if (setup_err)
    {
      abort ();
    }
  bplus_fetcher_free (first_leaf_fetcher);
  bplus_fetcher_free (last_leaf_fetcher);
  size_t load_request_row[1];
  size_t load_request_start[1];
  size_t load_request_length[1];
  size_t n_load_requests;
  int has_next;
  struct bplus_key keys[2];
  uint32_t values[2];
  size_t n_keys;
  int adv_error;
  // The first call should yield 2 -> 2 only. I am then asked to fetch
  // node 5 to continue.
  n_keys =
    bplus_range_get (range, 0, 2, keys, values, &has_next, &n_load_requests,
		     0, 2, load_request_row, load_request_start,
		     load_request_length);
  ck_assert_int_eq (n_keys, 1);
  ck_assert_int_eq (keys[0].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[0].arg.known, 2);
  ck_assert_int_eq (values[0], 2);
  ck_assert_int_eq (has_next, 1);
  ck_assert_int_eq (n_load_requests, 1);
  ck_assert_int_eq (load_request_row[0], 5);
  ck_assert_int_eq (load_request_start[0], 0);
  ck_assert_int_eq (load_request_length[0], 7);
  // I cannot directly advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 1);
  // So I prepare the next leaf.
  bplus_range_data (range, 5, 0, 7, standard_tree_load (5, 0, 7));
  // And I advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 0);
  // The next call yields 3 -> 3 and 5 -> 5, and then I have to fetch
  // node 6.
  n_keys =
    bplus_range_get (range, 0, 2, keys, values, &has_next, &n_load_requests,
		     0, 2, load_request_row, load_request_start,
		     load_request_length);
  ck_assert_int_eq (n_keys, 2);
  ck_assert_int_eq (keys[0].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[0].arg.known, 3);
  ck_assert_int_eq (values[0], 3);
  ck_assert_int_eq (keys[1].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[1].arg.known, 5);
  ck_assert_int_eq (values[1], 5);
  ck_assert_int_eq (has_next, 1);
  ck_assert_int_eq (n_load_requests, 1);
  ck_assert_int_eq (load_request_row[0], 6);
  ck_assert_int_eq (load_request_start[0], 0);
  ck_assert_int_eq (load_request_length[0], 7);
  // So I prepare the next leaf.
  bplus_range_data (range, 6, 0, 7, standard_tree_load (6, 0, 7));
  // And I advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 0);
  // The next call yields 9 -> 9 and 10 -> 10, and then I have to fetch
  // node 7.
  n_keys =
    bplus_range_get (range, 0, 2, keys, values, &has_next, &n_load_requests,
		     0, 2, load_request_row, load_request_start,
		     load_request_length);
  ck_assert_int_eq (n_keys, 2);
  ck_assert_int_eq (keys[0].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[0].arg.known, 9);
  ck_assert_int_eq (values[0], 9);
  ck_assert_int_eq (keys[1].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[1].arg.known, 10);
  ck_assert_int_eq (values[1], 10);
  ck_assert_int_eq (has_next, 1);
  ck_assert_int_eq (n_load_requests, 1);
  ck_assert_int_eq (load_request_row[0], 7);
  ck_assert_int_eq (load_request_start[0], 0);
  ck_assert_int_eq (load_request_length[0], 7);
  // So I prepare the next leaf.
  bplus_range_data (range, 7, 0, 7, standard_tree_load (7, 0, 7));
  // And I advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 0);
  // The next call yields 10 -> 10 and 10 -> 10, and then I have to fetch
  // node 8.
  n_keys =
    bplus_range_get (range, 0, 2, keys, values, &has_next, &n_load_requests,
		     0, 2, load_request_row, load_request_start,
		     load_request_length);
  ck_assert_int_eq (n_keys, 2);
  ck_assert_int_eq (keys[0].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[0].arg.known, 10);
  ck_assert_int_eq (values[0], 10);
  ck_assert_int_eq (keys[1].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[1].arg.known, 10);
  ck_assert_int_eq (values[1], 10);
  ck_assert_int_eq (has_next, 1);
  ck_assert_int_eq (n_load_requests, 1);
  ck_assert_int_eq (load_request_row[0], 8);
  ck_assert_int_eq (load_request_start[0], 0);
  ck_assert_int_eq (load_request_length[0], 7);
  // So I prepare the next leaf.
  bplus_range_data (range, 8, 0, 7, standard_tree_load (8, 0, 7));
  // And I advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 0);
  // The next call yields 11 -> 11 and 12 -> 12, and then I have to fetch
  // node 9.
  n_keys =
    bplus_range_get (range, 0, 2, keys, values, &has_next, &n_load_requests,
		     0, 2, load_request_row, load_request_start,
		     load_request_length);
  ck_assert_int_eq (n_keys, 2);
  ck_assert_int_eq (keys[0].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[0].arg.known, 11);
  ck_assert_int_eq (values[0], 11);
  ck_assert_int_eq (keys[1].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[1].arg.known, 12);
  ck_assert_int_eq (values[1], 12);
  ck_assert_int_eq (has_next, 1);
  ck_assert_int_eq (n_load_requests, 1);
  ck_assert_int_eq (load_request_row[0], 9);
  ck_assert_int_eq (load_request_start[0], 0);
  ck_assert_int_eq (load_request_length[0], 7);
  // So I prepare the next leaf.
  bplus_range_data (range, 9, 0, 7, standard_tree_load (9, 0, 7));
  // And I advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 0);
  // The next call yields 13 -> 13 and 15 -> 15, but since node 10 is
  // already known (it is the tail), I don’t have to fetch it.
  n_keys =
    bplus_range_get (range, 0, 2, keys, values, &has_next, &n_load_requests,
		     0, 2, load_request_row, load_request_start,
		     load_request_length);
  ck_assert_int_eq (n_keys, 2);
  ck_assert_int_eq (keys[0].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[0].arg.known, 13);
  ck_assert_int_eq (values[0], 13);
  ck_assert_int_eq (keys[1].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[1].arg.known, 15);
  ck_assert_int_eq (values[1], 15);
  ck_assert_int_eq (has_next, 1);
  ck_assert_int_eq (n_load_requests, 0);
  // So I can directly advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 0);
  // The next call yields 20 -> 20, and this is the end of the range.
  n_keys =
    bplus_range_get (range, 0, 2, keys, values, &has_next, &n_load_requests,
		     0, 2, load_request_row, load_request_start,
		     load_request_length);
  ck_assert_int_eq (n_keys, 1);
  ck_assert_int_eq (keys[0].type, BPLUS_KEY_KNOWN);
  ck_assert_int_eq (keys[0].arg.known, 20);
  ck_assert_int_eq (values[0], 20);
  ck_assert_int_eq (has_next, 0);
  ck_assert_int_eq (n_load_requests, 0);
  // I cannot directly advance to the next leaf.
  adv_error = bplus_range_next (range);
  ck_assert_int_eq (adv_error, 1);
  bplus_range_free (range);
  bplus_tree_free (tree);
}

static void
do_check_range_2 (void)
{
  /* There are 2 leaves in the range. Check that at the start, it has
     a next leaf, but that it does not try to fetch anything. */
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  uint32_t head_id = 1;
  uint32_t head_keys[2] = { 1, 2 };
  uint32_t head_values[3] = { 1, 2, 2 };
  struct bplus_node head = {
    .n_entries = 2,
    .keys = head_keys,
    .values = head_values,
    .parent_node = 0,
    .is_leaf = 1
  };
  size_t first_key = 1;
  uint32_t tail_id = 2;
  uint32_t tail_keys[2] = { 3, 4 };
  uint32_t tail_values[3] = { 3, 4, 0 };
  struct bplus_node tail = {
    .n_entries = 2,
    .keys = tail_keys,
    .values = tail_values,
    .parent_node = 0,
    .is_leaf = 1
  };
  size_t stop_key = 0;
  struct bplus_range *range = bplus_range_alloc (tree);
  if (range == NULL)
    {
      abort ();
    }
  int error =
    bplus_range_setup (range, head_id, &head, first_key, tail_id, &tail,
		       stop_key);
  ck_assert_int_eq (error, 0);
  int has_next = 42;
  size_t n_load_requests;
  size_t n_first =
    bplus_range_get (range, 0, 0, NULL, NULL, &has_next, &n_load_requests, 0,
		     0, NULL, NULL, NULL);
  ck_assert_int_eq (n_first, 1);
  ck_assert_int_eq (has_next, 1);
  ck_assert_int_eq (n_load_requests, 0);
  bplus_range_free (range);
  bplus_tree_free (tree);
}

static void
do_check_range_same_not_end (void)
{
  /* There are 2 leaves in the tree, but only the first is in the
     range. Check that at the start, it has a next leaf, but that it
     does not try to fetch anything. */
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  uint32_t head_id = 1;
  uint32_t head_keys[2] = { 1, 2 };
  uint32_t head_values[3] = { 1, 2, 2 };
  struct bplus_node head = {
    .n_entries = 2,
    .keys = head_keys,
    .values = head_values,
    .parent_node = 0,
    .is_leaf = 1
  };
  size_t first_key = 0;
  size_t stop_key = 1;
  struct bplus_range *range = bplus_range_alloc (tree);
  if (range == NULL)
    {
      abort ();
    }
  int error =
    bplus_range_setup (range, head_id, &head, first_key, head_id, &head,
		       stop_key);
  ck_assert_int_eq (error, 0);
  int has_next;
  size_t n_load_requests;
  size_t n_first =
    bplus_range_get (range, 0, 0, NULL, NULL, &has_next, &n_load_requests, 0,
		     0, NULL, NULL, NULL);
  ck_assert_int_eq (n_first, 1);
  ck_assert_int_eq (has_next, 0);
  ck_assert_int_eq (n_load_requests, 0);
  bplus_range_free (range);
  bplus_tree_free (tree);
}

static void
range_take_all (struct bplus_range *range, uint32_t ** data, size_t *length)
{
  size_t max = 1;
  *length = 0;
  *data = malloc (max * sizeof (uint32_t));
  if (*data == NULL)
    {
      abort ();
    }
  int has_next;
  do
    {
      size_t n_load_requests;
      size_t n_keys =
	bplus_range_get (range, 0, 0, NULL, NULL, &has_next, &n_load_requests,
			 0, 0, NULL, NULL, NULL);
      struct bplus_key *keys = malloc (n_keys * sizeof (struct bplus_key));
      uint32_t *values = malloc (n_keys * sizeof (uint32_t));
      size_t *rows = malloc (n_load_requests * sizeof (size_t));
      size_t *starts = malloc (n_load_requests * sizeof (size_t));
      size_t *lengths = malloc (n_load_requests * sizeof (size_t));
      if (keys == NULL || values == NULL || rows == NULL || starts == NULL
	  || lengths == NULL)
	{
	  abort ();
	}
      size_t n_load_requests_check;
      size_t n_keys_check =
	bplus_range_get (range, 0, n_keys, keys, values, &has_next,
			 &n_load_requests_check, 0, n_load_requests, rows,
			 starts, lengths);
      ck_assert_int_eq (n_load_requests, n_load_requests_check);
      ck_assert_int_eq (n_keys, n_keys_check);
      for (size_t i = 0; i < n_load_requests; i++)
	{
	  bplus_range_data (range, rows[i], starts[i], lengths[i],
			    standard_tree_load (rows[i], starts[i],
						lengths[i]));
	}
      for (size_t i = 0; i < n_keys; i++)
	{
	  if (*length == max)
	    {
	      max *= 2;
	      uint32_t *reallocated =
		realloc (*data, max * sizeof (uint32_t));
	      if (reallocated == NULL)
		{
		  abort ();
		}
	      *data = reallocated;
	    }
	  uint32_t *dest = *data;
	  ck_assert_int_eq (keys[i].type, BPLUS_KEY_KNOWN);
	  ck_assert_int_eq (keys[i].arg.known, values[i]);
	  dest[*length] = values[i];
	  *length = *length + 1;
	}
      free (keys);
      free (values);
      free (rows);
      free (starts);
      free (lengths);
      int next_err = 0;
      if (has_next)
	{
	  next_err = bplus_range_next (range);
	}
      ck_assert_int_eq (next_err, 0);
    }
  while (has_next);
}

static void
do_check_finder (void)
{
  /* Find all the keys that compare equal to a joker key (every key is
     equal to the joker key). Check that the range we obtain can be
     drained to yield every record. Before the range has been looked
     at, only nodes 0, 1, 3, 4 and 10 should be present in the
     cache. Once the range has been exhausted, every node but 2 must
     be present in the cache. */
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_finder *finder = bplus_finder_alloc (tree);
  if (finder == NULL)
    {
      abort ();
    }
  struct bplus_fetcher *root_fetcher = bplus_fetcher_alloc (tree);
  if (root_fetcher == NULL)
    {
      abort ();
    }
  bplus_fetcher_setup (root_fetcher, 0);
  const struct bplus_node *root = satisfy_fetcher (root_fetcher);
  struct bplus_key joker;
  joker.type = BPLUS_KEY_UNKNOWN;
  joker.arg.unknown = NULL;
  bplus_finder_setup (finder, &joker, 0, root);
  bplus_fetcher_free (root_fetcher);
  struct bplus_range *range = bplus_range_alloc (tree);
  if (range == NULL)
    {
      abort ();
    }
  int done = 0;
  do
    {
      size_t n_fetch_requests, n_compare_requests;
      bplus_finder_status (finder, &done, range, &n_fetch_requests, 0, 0,
			   NULL, NULL, NULL, &n_compare_requests, 0, 0, NULL,
			   NULL);
      size_t *rows = malloc (n_fetch_requests * sizeof (size_t));
      size_t *starts = malloc (n_fetch_requests * sizeof (size_t));
      size_t *lengths = malloc (n_fetch_requests * sizeof (size_t));
      struct bplus_key *as =
	malloc (n_compare_requests * sizeof (struct bplus_key));
      struct bplus_key *bs =
	malloc (n_compare_requests * sizeof (struct bplus_key));
      if (rows == NULL || starts == NULL || lengths == NULL || as == NULL
	  || bs == NULL)
	{
	  abort ();
	}
      size_t ck_fetch, ck_compare;
      bplus_finder_status (finder, &done, range, &ck_fetch, 0,
			   n_fetch_requests, rows, starts, lengths,
			   &ck_compare, 0, n_compare_requests, as, bs);
      ck_assert_int_eq (ck_fetch, n_fetch_requests);
      ck_assert_int_eq (ck_compare, n_compare_requests);
      for (size_t i = 0; i < n_fetch_requests; i++)
	{
	  bplus_finder_data (finder, rows[i], starts[i], lengths[i],
			     standard_tree_load (rows[i], starts[i],
						 lengths[i]));
	}
      for (size_t i = 0; i < n_compare_requests; i++)
	{
	  /* This is the joker. All keys compare equal to that. */
	  ck_assert_int_eq (bs[i].type, BPLUS_KEY_UNKNOWN);
	  bplus_finder_compared (finder, &(as[i]), &(bs[i]), 0);
	}
      free (rows);
      free (starts);
      free (lengths);
      free (as);
      free (bs);
    }
  while (!done);
  bplus_finder_free (finder);
  /* The range has been set up. */
  /* These are the nodes that have been touched already: */
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 0), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 1), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 3), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 4), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 10), NULL);
  /* These are the nodes that have NOT been touched yet (notice,
     ptr_eq): */
  ck_assert_ptr_eq (bplus_tree_get_cache (tree, 2), NULL);
  ck_assert_ptr_eq (bplus_tree_get_cache (tree, 5), NULL);
  ck_assert_ptr_eq (bplus_tree_get_cache (tree, 6), NULL);
  ck_assert_ptr_eq (bplus_tree_get_cache (tree, 7), NULL);
  ck_assert_ptr_eq (bplus_tree_get_cache (tree, 8), NULL);
  ck_assert_ptr_eq (bplus_tree_get_cache (tree, 9), NULL);
  uint32_t *everything;
  size_t n_everything;
  range_take_all (range, &everything, &n_everything);
  bplus_range_free (range);
  /* These are the nodes that have been touched already: */
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 0), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 1), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 3), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 4), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 5), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 6), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 7), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 8), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 9), NULL);
  ck_assert_ptr_ne (bplus_tree_get_cache (tree, 10), NULL);
  /* These are the nodes that have NOT been touched yet (notice,
     ptr_eq): */
  ck_assert_ptr_eq (bplus_tree_get_cache (tree, 2), NULL);
  ck_assert_int_eq (n_everything, 13);
  ck_assert_int_eq (everything[0], 1);
  ck_assert_int_eq (everything[1], 2);
  ck_assert_int_eq (everything[2], 3);
  ck_assert_int_eq (everything[3], 5);
  ck_assert_int_eq (everything[4], 9);
  ck_assert_int_eq (everything[5], 10);
  ck_assert_int_eq (everything[6], 10);
  ck_assert_int_eq (everything[7], 10);
  ck_assert_int_eq (everything[8], 11);
  ck_assert_int_eq (everything[9], 12);
  ck_assert_int_eq (everything[10], 13);
  ck_assert_int_eq (everything[11], 15);
  ck_assert_int_eq (everything[12], 20);
  free (everything);
  bplus_tree_free (tree);
}

static void
do_check_finder_root_leaf (void)
{
  /* In this example, the tree is just one leaf (the root). */
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_finder *finder = bplus_finder_alloc (tree);
  if (finder == NULL)
    {
      abort ();
    }
  uint32_t root_keys[2] = { 10, 20 };
  uint32_t root_values[3] = { 10, 20, 0 };
  struct bplus_node root = {
    .n_entries = 2,
    .keys = root_keys,
    .values = root_values,
    .parent_node = ((uint32_t) (-1)),
    .is_leaf = 1
  };
  struct bplus_key joker;
  joker.type = BPLUS_KEY_UNKNOWN;
  joker.arg.unknown = NULL;
  bplus_finder_setup (finder, &joker, 0, &root);
  struct bplus_range *range = bplus_range_alloc (tree);
  if (range == NULL)
    {
      abort ();
    }
  int done = 0;
  do
    {
      size_t n_fetch_requests, n_compare_requests;
      bplus_finder_status (finder, &done, range, &n_fetch_requests, 0, 0,
			   NULL, NULL, NULL, &n_compare_requests, 0, 0, NULL,
			   NULL);
      size_t *rows = malloc (n_fetch_requests * sizeof (size_t));
      size_t *starts = malloc (n_fetch_requests * sizeof (size_t));
      size_t *lengths = malloc (n_fetch_requests * sizeof (size_t));
      struct bplus_key *as =
	malloc (n_compare_requests * sizeof (struct bplus_key));
      struct bplus_key *bs =
	malloc (n_compare_requests * sizeof (struct bplus_key));
      if (rows == NULL || starts == NULL || lengths == NULL || as == NULL
	  || bs == NULL)
	{
	  abort ();
	}
      size_t ck_fetch, ck_compare;
      bplus_finder_status (finder, &done, range, &ck_fetch, 0,
			   n_fetch_requests, rows, starts, lengths,
			   &ck_compare, 0, n_compare_requests, as, bs);
      ck_assert_int_eq (ck_fetch, n_fetch_requests);
      ck_assert_int_eq (ck_compare, n_compare_requests);
      for (size_t i = 0; i < n_fetch_requests; i++)
	{
	  bplus_finder_data (finder, rows[i], starts[i], lengths[i],
			     standard_tree_load (rows[i], starts[i],
						 lengths[i]));
	}
      for (size_t i = 0; i < n_compare_requests; i++)
	{
	  /* This is the joker. All keys compare equal to that. */
	  ck_assert_int_eq (bs[i].type, BPLUS_KEY_UNKNOWN);
	  bplus_finder_compared (finder, &(as[i]), &(bs[i]), 0);
	}
      free (rows);
      free (starts);
      free (lengths);
      free (as);
      free (bs);
    }
  while (!done);
  bplus_finder_free (finder);
  /* The range has been set up. */
  uint32_t *everything;
  size_t n_everything;
  range_take_all (range, &everything, &n_everything);
  bplus_range_free (range);
  ck_assert_int_eq (n_everything, 2);
  ck_assert_int_eq (everything[0], 10);
  ck_assert_int_eq (everything[1], 20);
  free (everything);
  bplus_tree_free (tree);
}

static void
do_check_reparentor_leaf (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_node leaf_6;
  uint32_t leaf_keys[2] = { 9, 10 };
  uint32_t leaf_values[3] = { 9, 10, 7 };
  leaf_6.n_entries = 2;
  leaf_6.keys = leaf_keys;
  leaf_6.values = leaf_values;
  leaf_6.is_leaf = 1;
  leaf_6.parent_node = 2;
  struct bplus_reparentor *rep = reparentor_alloc (tree);
  if (rep == NULL)
    {
      abort ();
    }
  reparentor_setup (rep, 6, &leaf_6);
  int done;
  size_t n_fetches_to_do, n_stores_to_do;
  const size_t start_fetch_to_do = 0;
  const size_t max_fetches_to_do = 2;
  size_t fetch_rows[2];
  size_t fetch_starts[2];
  size_t fetch_lengths[2];
  const size_t start_store_to_do = 0;
  const size_t max_stores_to_do = 2;
  size_t store_rows[2];
  size_t store_starts[2];
  size_t store_lengths[2];
  const uint32_t *stores;
  /* Since this is a leaf, nothing will be done. */
  reparentor_status (rep, &done, &n_fetches_to_do, start_fetch_to_do,
		     max_fetches_to_do, fetch_rows, fetch_starts,
		     fetch_lengths, &n_stores_to_do, start_store_to_do,
		     max_stores_to_do, store_rows, store_starts,
		     store_lengths, &stores);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 0);
  reparentor_free (rep);
  bplus_tree_free (tree);
}

static void
do_check_reparentor_noop (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_node node_2;
  uint32_t node_keys[2] = { 10, 10 };
  uint32_t node_children[3] = { 6, 7, 8 };
  node_2.n_entries = 2;
  node_2.keys = node_keys;
  node_2.values = node_children;
  node_2.is_leaf = 0;
  node_2.parent_node = 0;
  struct bplus_reparentor *rep = reparentor_alloc (tree);
  if (rep == NULL)
    {
      abort ();
    }
  reparentor_setup (rep, 2, &node_2);
  int done;
  size_t n_fetches_to_do, n_stores_to_do;
  const size_t start_fetch_to_do = 0;
  const size_t max_fetches_to_do = 3;
  size_t fetch_rows[3];
  size_t fetch_starts[3];
  size_t fetch_lengths[3];
  const size_t start_store_to_do = 0;
  const size_t max_stores_to_do = 3;
  size_t store_rows[3];
  size_t store_starts[3];
  size_t store_lengths[3];
  const uint32_t *stores;
  /* First, the API will tell me to fetch the data (the 3 leaves, 6, 7 and 8). */
  reparentor_status (rep, &done, &n_fetches_to_do, start_fetch_to_do,
		     max_fetches_to_do, fetch_rows, fetch_starts,
		     fetch_lengths, &n_stores_to_do, start_store_to_do,
		     max_stores_to_do, store_rows, store_starts,
		     store_lengths, &stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 3);
  ck_assert_int_eq (n_stores_to_do, 0);
  ck_assert_int_eq (fetch_rows[0], 6);
  ck_assert_int_eq (fetch_rows[1], 7);
  ck_assert_int_eq (fetch_rows[2], 8);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_starts[1], 0);
  ck_assert_int_eq (fetch_starts[2], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  ck_assert_int_eq (fetch_lengths[1], 7);
  ck_assert_int_eq (fetch_lengths[2], 7);
  reparentor_data (rep, 6, 0, 7, standard_tree_load (6, 0, 7));
  reparentor_data (rep, 7, 0, 7, standard_tree_load (7, 0, 7));
  reparentor_data (rep, 8, 0, 7, standard_tree_load (8, 0, 7));
  /* Then, the API will realize that everything is in order. */
  reparentor_status (rep, &done, &n_fetches_to_do, start_fetch_to_do,
		     max_fetches_to_do, fetch_rows, fetch_starts,
		     fetch_lengths, &n_stores_to_do, start_store_to_do,
		     max_stores_to_do, store_rows, store_starts,
		     store_lengths, &stores);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 0);
  reparentor_free (rep);
  bplus_tree_free (tree);
}

static void
do_check_reparentor_nonfull (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  static const uint32_t row_6[7] = {
    9, ((uint32_t) (-1)),
    30, 31, 0,
    /* The parent is incorrect! */
    42,
    0
  };
  static const uint32_t row_7[7] = {
    10, ((uint32_t) (-1)),
    32, 33, 0,
    /* The parent is incorrect! */
    25,
    0
  };
  struct bplus_node node_2;
  uint32_t node_keys[2] = { 10, ((uint32_t) (-1)) };
  uint32_t node_children[3] = { 6, 7, 0 };
  node_2.n_entries = 1;
  node_2.keys = node_keys;
  node_2.values = node_children;
  node_2.is_leaf = 0;
  node_2.parent_node = 0;
  struct bplus_reparentor *rep = reparentor_alloc (tree);
  if (rep == NULL)
    {
      abort ();
    }
  reparentor_setup (rep, 2, &node_2);
  int done;
  size_t n_fetches_to_do, n_stores_to_do;
  const size_t start_fetch_to_do = 0;
  const size_t max_fetches_to_do = 2;
  size_t fetch_rows[2];
  size_t fetch_starts[2];
  size_t fetch_lengths[2];
  const size_t start_store_to_do = 0;
  const size_t max_stores_to_do = 2;
  size_t store_rows[2];
  size_t store_starts[2];
  size_t store_lengths[2];
  const uint32_t *stores[2];
  /* First, the API will tell me to fetch the data (both leaves, 6 and 7). */
  reparentor_status (rep, &done, &n_fetches_to_do, start_fetch_to_do,
		     max_fetches_to_do, fetch_rows, fetch_starts,
		     fetch_lengths, &n_stores_to_do, start_store_to_do,
		     max_stores_to_do, store_rows, store_starts,
		     store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 2);
  ck_assert_int_eq (n_stores_to_do, 0);
  ck_assert_int_eq (fetch_rows[0], 6);
  ck_assert_int_eq (fetch_rows[1], 7);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_starts[1], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  ck_assert_int_eq (fetch_lengths[1], 7);
  /* We pass corrupted data! */
  reparentor_data (rep, 6, 0, 7, row_6);
  reparentor_data (rep, 7, 0, 7, row_7);
  /* Then, the API will realize that everything must change. */
  reparentor_status (rep, &done, &n_fetches_to_do, start_fetch_to_do,
		     max_fetches_to_do, fetch_rows, fetch_starts,
		     fetch_lengths, &n_stores_to_do, start_store_to_do,
		     max_stores_to_do, store_rows, store_starts,
		     store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 2);
  ck_assert_int_eq (store_rows[0], 6);
  ck_assert_int_eq (store_rows[1], 7);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  ck_assert_int_eq (store_lengths[1], 7);
  /* The prescribed stores actually fix the problem. */
  ck_assert_int_eq (stores[0][5], 2);
  ck_assert_int_eq (stores[1][5], 2);
  /* And they don’t alter the data otherwise. */
  for (size_t i = 0; i < 7; i++)
    {
      if (i != 5)
	{
	  ck_assert_int_eq (stores[0][i], row_6[i]);
	  ck_assert_int_eq (stores[1][i], row_7[i]);
	}
    }
  reparentor_updated (rep, 6);
  reparentor_updated (rep, 7);
  /* Now the cache has been updated, it should be correct too. */
  const struct bplus_node *node_6 = bplus_tree_get_cache (tree, 6);
  const struct bplus_node *node_7 = bplus_tree_get_cache (tree, 7);
  if (node_6 != NULL)
    {
      ck_assert_int_eq (node_6->n_entries, 1);
      ck_assert_int_eq (node_6->keys[0], 9);
      ck_assert_int_eq (node_6->parent_node, 2);
    }
  if (node_7 != NULL)
    {
      ck_assert_int_eq (node_7->n_entries, 1);
      ck_assert_int_eq (node_7->keys[0], 10);
      ck_assert_int_eq (node_7->parent_node, 2);
    }
  reparentor_free (rep);
  bplus_tree_free (tree);
}

static void
do_check_reparentor_manyfixes (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  static const uint32_t row_6[7] = {
    9, 10,
    9, 10, 7,
    /* The parent is incorrect! */
    42,
    ((uint32_t) ((uint32_t) 1) << 31)
  };
  static const uint32_t row_7[7] = {
    10, 10,
    10, 10, 8,
    /* The parent is incorrect! */
    25,
    ((uint32_t) ((uint32_t) 1) << 31)
  };
  static const uint32_t row_8[7] = {
    11, 12,
    11, 12, 9,
    /* The parent is incorrect! */
    51,
    ((uint32_t) ((uint32_t) 1) << 31)
  };
  struct bplus_node node_2;
  uint32_t node_keys[2] = { 10, 10 };
  uint32_t node_children[3] = { 6, 7, 8 };
  node_2.n_entries = 2;
  node_2.keys = node_keys;
  node_2.values = node_children;
  node_2.is_leaf = 0;
  node_2.parent_node = 0;
  struct bplus_reparentor *rep = reparentor_alloc (tree);
  if (rep == NULL)
    {
      abort ();
    }
  reparentor_setup (rep, 2, &node_2);
  int done;
  size_t n_fetches_to_do, n_stores_to_do;
  const size_t start_fetch_to_do = 0;
  const size_t max_fetches_to_do = 3;
  size_t fetch_rows[3];
  size_t fetch_starts[3];
  size_t fetch_lengths[3];
  const size_t start_store_to_do = 0;
  const size_t max_stores_to_do = 3;
  size_t store_rows[3];
  size_t store_starts[3];
  size_t store_lengths[3];
  const uint32_t *stores[3];
  /* First, the API will tell me to fetch the data (the 3 leaves, 6, 7 and 8). */
  reparentor_status (rep, &done, &n_fetches_to_do, start_fetch_to_do,
		     max_fetches_to_do, fetch_rows, fetch_starts,
		     fetch_lengths, &n_stores_to_do, start_store_to_do,
		     max_stores_to_do, store_rows, store_starts,
		     store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 3);
  ck_assert_int_eq (n_stores_to_do, 0);
  ck_assert_int_eq (fetch_rows[0], 6);
  ck_assert_int_eq (fetch_rows[1], 7);
  ck_assert_int_eq (fetch_rows[2], 8);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_starts[1], 0);
  ck_assert_int_eq (fetch_starts[2], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  ck_assert_int_eq (fetch_lengths[1], 7);
  ck_assert_int_eq (fetch_lengths[2], 7);
  /* We pass corrupted data! */
  reparentor_data (rep, 6, 0, 7, row_6);
  reparentor_data (rep, 7, 0, 7, row_7);
  reparentor_data (rep, 8, 0, 7, row_8);
  /* Then, the API will realize that everything must change. */
  reparentor_status (rep, &done, &n_fetches_to_do, start_fetch_to_do,
		     max_fetches_to_do, fetch_rows, fetch_starts,
		     fetch_lengths, &n_stores_to_do, start_store_to_do,
		     max_stores_to_do, store_rows, store_starts,
		     store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 3);
  ck_assert_int_eq (store_rows[0], 6);
  ck_assert_int_eq (store_rows[1], 7);
  ck_assert_int_eq (store_rows[2], 8);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_starts[2], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  ck_assert_int_eq (store_lengths[1], 7);
  ck_assert_int_eq (store_lengths[2], 7);
  /* The prescribed stores actually fix the problem. */
  ck_assert_int_eq (stores[0][5], 2);
  ck_assert_int_eq (stores[1][5], 2);
  ck_assert_int_eq (stores[2][5], 2);
  reparentor_updated (rep, 6);
  reparentor_updated (rep, 7);
  reparentor_updated (rep, 8);
  /* Now the cache has been updated, it should be correct too. */
  const struct bplus_node *node_6 = bplus_tree_get_cache (tree, 6);
  const struct bplus_node *node_7 = bplus_tree_get_cache (tree, 7);
  const struct bplus_node *node_8 = bplus_tree_get_cache (tree, 8);
  if (node_6 != NULL)
    {
      ck_assert_int_eq (node_6->n_entries, 2);
      ck_assert_int_eq (node_6->keys[0], 9);
      ck_assert_int_eq (node_6->parent_node, 2);
    }
  if (node_7 != NULL)
    {
      ck_assert_int_eq (node_7->n_entries, 2);
      ck_assert_int_eq (node_7->keys[0], 10);
      ck_assert_int_eq (node_7->parent_node, 2);
    }
  if (node_8 != NULL)
    {
      ck_assert_int_eq (node_8->n_entries, 2);
      ck_assert_int_eq (node_8->keys[0], 11);
      ck_assert_int_eq (node_8->parent_node, 2);
    }
  reparentor_free (rep);
  bplus_tree_free (tree);
}

static void
do_check_grow_leaf (void)
{
  /* In this test, the root is a single leaf, that we grow. */
  uint32_t rows[][7] = {
    /* Node 0: the root-leaf. */
    {
     /* Keys */
     5, ((uint32_t) (-1)),
     /* Values */
     5, 0,
     /* Next leaf */
     0,
     /* Parent */
     ((uint32_t) (-1)),
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    /* Node 1: to be allocated. */
    {42, 42, 42, 42, 42, 42, 42}
  };
  size_t tree_length = 1;
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_growth *growth = growth_alloc (tree);
  if (growth == NULL)
    {
      abort ();
    }
  growth_setup (growth);
  int done;
  uint32_t new_id;
  do
    {
      size_t n_fetches_to_do;
      size_t start_fetch_to_do = 0;
      size_t max_fetches_to_do = 256;
      size_t fetch_rows[256];
      size_t fetch_starts[256];
      size_t fetch_lengths[256];
      size_t n_allocations_to_do;
      size_t n_stores_to_do;
      size_t start_store_to_do = 0;
      size_t max_stores_to_do = 256;
      size_t store_rows[256];
      size_t store_starts[256];
      size_t store_lengths[256];
      const uint32_t *stores[256];
      growth_status (growth, &done, &new_id, &n_fetches_to_do,
		     start_fetch_to_do, max_fetches_to_do, fetch_rows,
		     fetch_starts, fetch_lengths, &n_allocations_to_do,
		     &n_stores_to_do, start_store_to_do, max_stores_to_do,
		     store_rows, store_starts, store_lengths, stores);
      if (!done)
	{
	  for (size_t i = 0; i < 256 && i < n_fetches_to_do; i++)
	    {
	      ck_assert_int_lt (fetch_rows[i], tree_length);
	      ck_assert_int_le (fetch_starts[i], 7);
	      ck_assert_int_le (fetch_lengths[i], 7);
	      ck_assert_int_le (fetch_starts[i] + fetch_lengths[i], 7);
	      growth_data (growth, fetch_rows[i], fetch_starts[i],
			   fetch_lengths[i],
			   rows[fetch_rows[i]] + fetch_starts[i]);
	    }
	  for (size_t i = 0; i < n_allocations_to_do; i++)
	    {
	      ck_assert_int_lt (tree_length,
				sizeof (rows) / sizeof (rows[0]));
	      int claimed = growth_allocated (growth, tree_length);
	      ck_assert_int_eq (claimed, 1);
	      tree_length++;
	    }
	  for (size_t i = 0; i < 256 && i < n_stores_to_do; i++)
	    {
	      ck_assert_int_lt (store_rows[i], tree_length);
	      ck_assert_int_le (store_starts[i], 7);
	      ck_assert_int_le (store_lengths[i], 7);
	      ck_assert_int_le (store_starts[i] + store_lengths[i], 7);
	      memcpy (rows[store_rows[i]] + store_starts[i], stores[i],
		      store_lengths[i] * sizeof (uint32_t));
	      growth_updated (growth, store_rows[i]);
	    }
	}
    }
  while (!done);
  ck_assert_int_eq (new_id, 1);
  /* Check the cache: */
  const struct bplus_node *new_root = bplus_tree_get_cache (tree, 0);
  const struct bplus_node *new_leaf = bplus_tree_get_cache (tree, 1);
  if (new_root != NULL)
    {
      ck_assert_int_eq (new_root->n_entries, 0);
      ck_assert_int_eq (new_root->is_leaf, 0);
      ck_assert_int_eq (new_root->values[0], 1);
      ck_assert_int_eq (new_root->parent_node, (uint32_t) (-1));
    }
  if (new_leaf != NULL)
    {
      ck_assert_int_eq (new_leaf->n_entries, 1);
      ck_assert_int_eq (new_leaf->keys[0], 5);
      ck_assert_int_eq (new_leaf->values[0], 5);
      ck_assert_int_eq (new_leaf->is_leaf, 1);
      ck_assert_int_eq (new_leaf->parent_node, 0);
    }
  /* Check the actual data */
  static const uint32_t expected[2][7] = {
    /* New root! */
    {
     /* Keys */
     ((uint32_t) (-1)), ((uint32_t) (-1)),
     /* Children */
     1, 0, 0,
     /* Parent */
     ((uint32_t) (-1)),
     /* Flags */
     0},
    /* New leaf. */
    {
     /* Keys */
     5, ((uint32_t) (-1)),
     /* Values */
     5, 0,
     /* Next leaf */
     0,
     /* Parent */
     0,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)}
  };
  ck_assert_int_eq (tree_length, sizeof (expected) / sizeof (expected[0]));
  ck_assert_int_eq (sizeof (rows) / sizeof (rows[0]),
		    sizeof (expected) / sizeof (expected[0]));
  ck_assert_int_eq (sizeof (rows[0]) / sizeof (rows[0][0]),
		    sizeof (expected[0]) / sizeof (expected[0][0]));
  for (size_t i = 0; i < sizeof (rows) / sizeof (rows[0]); i++)
    {
      for (size_t j = 0; j < sizeof (rows[0]) / sizeof (rows[0][0]); j++)
	{
	  ck_assert_int_eq (rows[i][j], expected[i][j]);
	}
    }
  growth_free (growth);
  bplus_tree_free (tree);
}

static void
do_check_grow_nonleaf (void)
{
  /* In this test, the root is not a leaf. */
  uint32_t rows[][7] = {
    /* Node 0: the root. */
    {
     /* Keys */
     10, 10,
     /* Children */
     1, 2, 3,
     /* Parent */
     ((uint32_t) (-1)),
     /* Flags */
     0},
    /* Node 1-3: the children of 0. */
    {
     /* Keys */
     9, 10,
     /* Values */
     9, 10,
     /* Next leaf */
     2,
     /* Parent */
     0,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     10, 10,
     /* Values */
     10, 10,
     /* Next leaf */
     3,
     /* Parent */
     0,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     11, 12,
     /* Values */
     11, 12,
     /* Next leaf */
     0,
     /* Parent */
     0,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    /* Node 4: to be allocated. */
    {42, 42, 42, 42, 42, 42, 42}
  };
  size_t tree_length = 4;
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_growth *growth = growth_alloc (tree);
  if (growth == NULL)
    {
      abort ();
    }
  growth_setup (growth);
  int done;
  uint32_t new_id;
  do
    {
      size_t n_fetches_to_do;
      size_t start_fetch_to_do = 0;
      size_t max_fetches_to_do = 256;
      size_t fetch_rows[256];
      size_t fetch_starts[256];
      size_t fetch_lengths[256];
      size_t n_allocations_to_do;
      size_t n_stores_to_do;
      size_t start_store_to_do = 0;
      size_t max_stores_to_do = 256;
      size_t store_rows[256];
      size_t store_starts[256];
      size_t store_lengths[256];
      const uint32_t *stores[256];
      growth_status (growth, &done, &new_id, &n_fetches_to_do,
		     start_fetch_to_do, max_fetches_to_do, fetch_rows,
		     fetch_starts, fetch_lengths, &n_allocations_to_do,
		     &n_stores_to_do, start_store_to_do, max_stores_to_do,
		     store_rows, store_starts, store_lengths, stores);
      if (!done)
	{
	  for (size_t i = 0; i < 256 && i < n_fetches_to_do; i++)
	    {
	      ck_assert_int_lt (fetch_rows[i], tree_length);
	      ck_assert_int_le (fetch_starts[i], 7);
	      ck_assert_int_le (fetch_lengths[i], 7);
	      ck_assert_int_le (fetch_starts[i] + fetch_lengths[i], 7);
	      growth_data (growth, fetch_rows[i], fetch_starts[i],
			   fetch_lengths[i],
			   rows[fetch_rows[i]] + fetch_starts[i]);
	    }
	  for (size_t i = 0; i < n_allocations_to_do; i++)
	    {
	      ck_assert_int_lt (tree_length,
				sizeof (rows) / sizeof (rows[0]));
	      int claimed = growth_allocated (growth, tree_length);
	      ck_assert_int_eq (claimed, 1);
	      tree_length++;
	    }
	  for (size_t i = 0; i < 256 && i < n_stores_to_do; i++)
	    {
	      ck_assert_int_lt (store_rows[i], tree_length);
	      ck_assert_int_le (store_starts[i], 7);
	      ck_assert_int_le (store_lengths[i], 7);
	      ck_assert_int_le (store_starts[i] + store_lengths[i], 7);
	      memcpy (rows[store_rows[i]] + store_starts[i], stores[i],
		      store_lengths[i] * sizeof (uint32_t));
	      growth_updated (growth, store_rows[i]);
	    }
	}
    }
  while (!done);
  ck_assert_int_eq (new_id, 4);
  /* Check the cache: */
  const struct bplus_node *new_root = bplus_tree_get_cache (tree, 0);
  const struct bplus_node *old_root = bplus_tree_get_cache (tree, 4);
  const struct bplus_node *first_leaf = bplus_tree_get_cache (tree, 1);
  const struct bplus_node *second_leaf = bplus_tree_get_cache (tree, 2);
  const struct bplus_node *third_leaf = bplus_tree_get_cache (tree, 3);
  if (new_root != NULL)
    {
      ck_assert_int_eq (new_root->n_entries, 0);
      ck_assert_int_eq (new_root->is_leaf, 0);
      ck_assert_int_eq (new_root->values[0], 4);
      ck_assert_int_eq (new_root->parent_node, (uint32_t) (-1));
    }
  if (old_root != NULL)
    {
      ck_assert_int_eq (old_root->n_entries, 2);
      ck_assert_int_eq (old_root->keys[0], 10);
      ck_assert_int_eq (old_root->keys[1], 10);
      ck_assert_int_eq (old_root->values[0], 1);
      ck_assert_int_eq (old_root->values[1], 2);
      ck_assert_int_eq (old_root->values[2], 3);
      ck_assert_int_eq (old_root->is_leaf, 0);
      ck_assert_int_eq (old_root->parent_node, 0);
    }
  if (first_leaf != NULL)
    {
      ck_assert_int_eq (first_leaf->n_entries, 2);
      ck_assert_int_eq (first_leaf->keys[0], 9);
      ck_assert_int_eq (first_leaf->keys[1], 10);
      ck_assert_int_eq (first_leaf->values[0], 9);
      ck_assert_int_eq (first_leaf->values[1], 10);
      ck_assert_int_eq (first_leaf->values[2], 2);
      ck_assert_int_eq (first_leaf->is_leaf, 1);
      ck_assert_int_eq (first_leaf->parent_node, 4);
    }
  if (second_leaf != NULL)
    {
      ck_assert_int_eq (second_leaf->n_entries, 2);
      ck_assert_int_eq (second_leaf->keys[0], 10);
      ck_assert_int_eq (second_leaf->keys[1], 10);
      ck_assert_int_eq (second_leaf->values[0], 10);
      ck_assert_int_eq (second_leaf->values[1], 10);
      ck_assert_int_eq (second_leaf->values[2], 3);
      ck_assert_int_eq (second_leaf->is_leaf, 1);
      ck_assert_int_eq (second_leaf->parent_node, 4);
    }
  if (third_leaf != NULL)
    {
      ck_assert_int_eq (third_leaf->n_entries, 2);
      ck_assert_int_eq (third_leaf->keys[0], 11);
      ck_assert_int_eq (third_leaf->keys[1], 12);
      ck_assert_int_eq (third_leaf->values[0], 11);
      ck_assert_int_eq (third_leaf->values[1], 12);
      ck_assert_int_eq (third_leaf->values[2], 0);
      ck_assert_int_eq (third_leaf->is_leaf, 1);
      ck_assert_int_eq (third_leaf->parent_node, 4);
    }
  /* Check the actual data */
  static const uint32_t expected[5][7] = {
    /* Node 0: the new root. */
    {
     /* Keys */
     ((uint32_t) (-1)), ((uint32_t) (-1)),
     /* Children */
     4, 0, 0,
     /* Parent */
     ((uint32_t) (-1)),
     /* Flags */
     0},
    /* Node 1-3: the children of the old root (the new grandchildren). */
    {
     /* Keys */
     9, 10,
     /* Values */
     9, 10,
     /* Next leaf */
     2,
     /* Parent (CHANGED) */
     4,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     10, 10,
     /* Values */
     10, 10,
     /* Next leaf */
     3,
     /* Parent (CHANGED) */
     4,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    {
     /* Keys */
     11, 12,
     /* Values */
     11, 12,
     /* Next leaf */
     0,
     /* Parent (CHANGED) */
     4,
     /* Flags */
     ((uint32_t) ((uint32_t) 1) << 31)},
    /* Node 4: the old root. */
    {
     /* Keys */
     10, 10,
     /* Children */
     1, 2, 3,
     /* Parent (CHANGED) */
     0,
     /* Flags */
     0}
  };
  ck_assert_int_eq (tree_length, sizeof (expected) / sizeof (expected[0]));
  ck_assert_int_eq (sizeof (rows) / sizeof (rows[0]),
		    sizeof (expected) / sizeof (expected[0]));
  ck_assert_int_eq (sizeof (rows[0]) / sizeof (rows[0][0]),
		    sizeof (expected[0]) / sizeof (expected[0][0]));
  for (size_t i = 0; i < sizeof (rows) / sizeof (rows[0]); i++)
    {
      for (size_t j = 0; j < sizeof (rows[0]) / sizeof (rows[0][0]); j++)
	{
	  ck_assert_int_eq (rows[i][j], expected[i][j]);
	}
    }
  growth_free (growth);
  bplus_tree_free (tree);
}

static void
do_check_grow_nonfull (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_growth *growth = growth_alloc (tree);
  if (growth == NULL)
    {
      abort ();
    }
  growth_setup (growth);
  int done;
  uint32_t new_id;
  size_t n_fetches_to_do;
  size_t start_fetch_to_do = 0;
  size_t max_fetches_to_do = 256;
  size_t fetch_rows[256];
  size_t fetch_starts[256];
  size_t fetch_lengths[256];
  size_t n_allocations_to_do;
  size_t n_stores_to_do;
  size_t start_store_to_do = 0;
  size_t max_stores_to_do = 256;
  size_t store_rows[256];
  size_t store_starts[256];
  size_t store_lengths[256];
  const uint32_t *stores[256];
  /* First step: read the root and allocate. */
  growth_status (growth, &done, &new_id, &n_fetches_to_do, start_fetch_to_do,
		 max_fetches_to_do, fetch_rows, fetch_starts, fetch_lengths,
		 &n_allocations_to_do, &n_stores_to_do, start_store_to_do,
		 max_stores_to_do, store_rows, store_starts, store_lengths,
		 stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 1);
  ck_assert_int_eq (n_allocations_to_do, 1);
  ck_assert_int_eq (n_stores_to_do, 0);
  ck_assert_int_eq (fetch_rows[0], 0);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  static const uint32_t initial_root[7] = {
    /* Keys: */
    53, ((uint32_t) (-1)),
    /* Subtrees: */
    6, 12, 0,
    /* Parent: */
    ((uint32_t) (-1)),
    /* Flags: */
    0
  };
  growth_data (growth, 0, 0, 7, initial_root);
  int claimed = growth_allocated (growth, 14);
  ck_assert_int_eq (claimed, 1);
  /* Now, the root is fixed to have only 1 child, and node 14 is set
     to a copy of the old root. The children of the old root are fetched. */
  growth_status (growth, &done, &new_id, &n_fetches_to_do, start_fetch_to_do,
		 max_fetches_to_do, fetch_rows, fetch_starts, fetch_lengths,
		 &n_allocations_to_do, &n_stores_to_do, start_store_to_do,
		 max_stores_to_do, store_rows, store_starts, store_lengths,
		 stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 2);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 2);
  ck_assert_int_eq (fetch_rows[0], 6);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  ck_assert_int_eq (fetch_rows[1], 12);
  ck_assert_int_eq (fetch_starts[1], 0);
  ck_assert_int_eq (fetch_lengths[1], 7);
  ck_assert_int_eq (store_rows[0], 0);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  /* No keys: */
  ck_assert_int_eq (stores[0][0], ((uint32_t) (-1)));
  ck_assert_int_eq (stores[0][1], ((uint32_t) (-1)));
  /* Only 1 subtree: */
  ck_assert_int_eq (stores[0][2], 14);
  ck_assert_int_eq (stores[0][3], 0);
  ck_assert_int_eq (stores[0][4], 0);
  /* No parent: */
  ck_assert_int_eq (stores[0][5], ((uint32_t) (-1)));
  /* Not a leaf: */
  ck_assert_int_eq (stores[0][6], 0);
  ck_assert_int_eq (store_rows[1], 14);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_lengths[1], 7);
  /* Same keys: */
  ck_assert_int_eq (stores[1][0], 53);
  ck_assert_int_eq (stores[1][1], ((uint32_t) (-1)));
  /* Only 2 subtrees: */
  ck_assert_int_eq (stores[1][2], 6);
  ck_assert_int_eq (stores[1][3], 12);
  ck_assert_int_eq (stores[1][4], 0);
  /* The parent is the root: */
  ck_assert_int_eq (stores[1][5], 0);
  /* Not a leaf: */
  ck_assert_int_eq (stores[1][6], 0);
  static const uint32_t initial_6[7] = {
    /* Keys: */
    46, ((uint32_t) (-1)),
    /* Subtrees: */
    2, 11, 0,
    /* Parent before growth: */
    0,
    /* Flags: */
    0
  };
  static const uint32_t initial_12[7] = {
    /* Keys: */
    99, ((uint32_t) (-1)),
    /* Subtrees: */
    7, 4, 0,
    /* Parent before growth: */
    0,
    /* Flags: */
    0
  };
  growth_data (growth, 6, 0, 7, initial_6);
  growth_data (growth, 12, 0, 7, initial_12);
  growth_updated (growth, 0);
  growth_updated (growth, 14);
  /* Now, the children simply need to be fixed. */
  growth_status (growth, &done, &new_id, &n_fetches_to_do, start_fetch_to_do,
		 max_fetches_to_do, fetch_rows, fetch_starts, fetch_lengths,
		 &n_allocations_to_do, &n_stores_to_do, start_store_to_do,
		 max_stores_to_do, store_rows, store_starts, store_lengths,
		 stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 2);
  ck_assert_int_eq (store_rows[0], 6);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  /* Unchanged keys: */
  ck_assert_int_eq (stores[0][0], initial_6[0]);
  ck_assert_int_eq (stores[0][1], initial_6[1]);
  /* Unchanged subtrees: */
  ck_assert_int_eq (stores[0][2], initial_6[2]);
  ck_assert_int_eq (stores[0][3], initial_6[3]);
  ck_assert_int_eq (stores[0][4], initial_6[4]);
  /* Fixed parent: */
  ck_assert_int_eq (stores[0][5], 14);
  /* Still not a leaf: */
  ck_assert_int_eq (stores[0][6], 0);
  ck_assert_int_eq (store_rows[1], 12);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_lengths[1], 7);
  /* Unchanged keys: */
  ck_assert_int_eq (stores[1][0], initial_12[0]);
  ck_assert_int_eq (stores[1][1], initial_12[1]);
  /* Unchanged subtrees: */
  ck_assert_int_eq (stores[1][2], initial_12[2]);
  ck_assert_int_eq (stores[1][3], initial_12[3]);
  ck_assert_int_eq (stores[1][4], initial_12[4]);
  /* Fixed parent: */
  ck_assert_int_eq (stores[1][5], 14);
  /* Still not a leaf: */
  ck_assert_int_eq (stores[1][6], 0);
  growth_updated (growth, 6);
  growth_updated (growth, 12);
  /* Done. */
  growth_status (growth, &done, &new_id, &n_fetches_to_do, start_fetch_to_do,
		 max_fetches_to_do, fetch_rows, fetch_starts, fetch_lengths,
		 &n_allocations_to_do, &n_stores_to_do, start_store_to_do,
		 max_stores_to_do, store_rows, store_starts, store_lengths,
		 stores);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (new_id, 14);
  growth_free (growth);
  bplus_tree_free (tree);
}

static void
do_check_insert_append_leaf (void)
{
  /* In this very simple example, we can append a record in a leaf
     that still has room. */
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_range *range = bplus_range_alloc (tree);
  struct bplus_insertion *insertion = bplus_insertion_alloc (tree);
  if (range == NULL || insertion == NULL)
    {
      abort ();
    }
  uint32_t root_keys[2] = { 1, ((uint32_t) (-1)) };
  uint32_t root_values[3] = { 1, 0, 0 };
  const struct bplus_node root = {
    .n_entries = 1,
    .keys = root_keys,
    .values = root_values,
    .is_leaf = 1,
    .parent_node = ((uint32_t) (-1))
  };
  int err = bplus_range_setup (range, 0, &root, 1, 0, &root, 1);
  ck_assert_int_eq (err, 0);
  bplus_insertion_setup (insertion, 2, range, 0);
  int done;
  size_t n_fetches_to_do, n_allocations_to_do, n_stores_to_do;
  size_t start_fetch_to_do = 0;
  size_t max_fetches_to_do = 256;
  size_t fetch_rows[256];
  size_t fetch_starts[256];
  size_t fetch_lengths[256];
  size_t start_store_to_do = 0;
  size_t max_stores_to_do = 256;
  size_t store_rows[256];
  size_t store_starts[256];
  size_t store_lengths[256];
  const uint32_t *stores[256];
  /* It should not ask me to fetch anything, or to compare anything,
     because there is no need to bubble 2->2 down the leaf. There is
     still room, so I should not have to allocate anything. In fact, I
     expect just one store. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 1);
  ck_assert_int_eq (store_rows[0], 0);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  /* Keys: Please store 1, 2. */
  ck_assert_int_eq (stores[0][0], 1);
  ck_assert_int_eq (stores[0][1], 2);
  /* Values: 1, 2. */
  ck_assert_int_eq (stores[0][2], 1);
  ck_assert_int_eq (stores[0][3], 2);
  /* Next leaf: no next leaf (0). */
  ck_assert_int_eq (stores[0][4], 0);
  /* Parent: (-1). */
  ck_assert_int_eq (stores[0][5], ((uint32_t) (-1)));
  /* Flags: a leaf. */
  ck_assert_int_eq (stores[0][6], ((uint32_t) ((uint32_t) 1) << 31));
  bplus_range_free (range);
  bplus_insertion_free (insertion);
  bplus_tree_free (tree);
}

static void
do_check_insert_enough_space (void)
{
  /* In this still very simple example, we insert a record in the
     middle of a leaf that still has room in it. */
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_range *range = bplus_range_alloc (tree);
  struct bplus_insertion *insertion = bplus_insertion_alloc (tree);
  if (range == NULL || insertion == NULL)
    {
      abort ();
    }
  uint32_t root_keys[2] = { 1, ((uint32_t) (-1)) };
  uint32_t root_values[3] = { 1, 0, 0 };
  const struct bplus_node root = {
    .n_entries = 1,
    .keys = root_keys,
    .values = root_values,
    .is_leaf = 1,
    .parent_node = ((uint32_t) (-1))
  };
  /* Now we insert at position 0, and expect the 1->1 record to bubble
     after the 2. */
  int err = bplus_range_setup (range, 0, &root, 0, 0, &root, 0);
  ck_assert_int_eq (err, 0);
  bplus_insertion_setup (insertion, 2, range, 0);
  int done;
  size_t n_fetches_to_do, n_allocations_to_do, n_stores_to_do;
  size_t start_fetch_to_do = 0;
  size_t max_fetches_to_do = 256;
  size_t fetch_rows[256];
  size_t fetch_starts[256];
  size_t fetch_lengths[256];
  size_t start_store_to_do = 0;
  size_t max_stores_to_do = 256;
  size_t store_rows[256];
  size_t store_starts[256];
  size_t store_lengths[256];
  const uint32_t *stores[256];
  /* It should not ask me to fetch anything, because we work on the
     root. There is still room, so I should not have to allocate
     anything. In fact, I expect just one store. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 1);
  ck_assert_int_eq (store_rows[0], 0);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  /* Keys: Please store 2, 1. */
  ck_assert_int_eq (stores[0][0], 2);
  ck_assert_int_eq (stores[0][1], 1);
  /* Values: 2, 1. */
  ck_assert_int_eq (stores[0][2], 2);
  ck_assert_int_eq (stores[0][3], 1);
  /* Next leaf: no next leaf (0). */
  ck_assert_int_eq (stores[0][4], 0);
  /* Parent: (-1). */
  ck_assert_int_eq (stores[0][5], ((uint32_t) (-1)));
  /* Flags: a leaf. */
  ck_assert_int_eq (stores[0][6], ((uint32_t) ((uint32_t) 1) << 31));
  bplus_range_free (range);
  bplus_insertion_free (insertion);
  bplus_tree_free (tree);
}

static void
do_check_parent_fetcher (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_parent_fetcher *fetcher = parent_fetcher_alloc (tree);
  if (fetcher == NULL)
    {
      abort ();
    }
  /* This leaf, number 6, is the second child of its parent. */
  uint32_t node_keys[2] = { 4, 6 };
  uint32_t node_values[3] = { 4, 6, 7 };
  const struct bplus_node node = {
    .n_entries = 2,
    .keys = node_keys,
    .values = node_values,
    .is_leaf = 1,
    .parent_node = 2
  };
  const uint32_t node_parent[7] = {
    /* Keys */
    3, 6,
    /* Subtrees */
    5, 6 /* here */ , 7,
    /* Parent ID */
    0,
    /* Flags */
    0
  };
  parent_fetcher_setup (fetcher, 6, &node);
  const struct bplus_node *parent = NULL;
  uint32_t parent_id;
  size_t parent_position;
  size_t fetch_row, fetch_start, fetch_length;
  parent =
    parent_fetcher_status (fetcher, &parent_id, &parent_position, &fetch_row,
			   &fetch_start, &fetch_length);
  ck_assert_ptr_eq (parent, NULL);
  ck_assert_int_eq (fetch_row, 2);
  ck_assert_int_eq (fetch_start, 0);
  ck_assert_int_eq (fetch_length, 7);
  parent_fetcher_data (fetcher, 2, 0, 7, node_parent);
  /* Fetched: */
  parent =
    parent_fetcher_status (fetcher, &parent_id, &parent_position, &fetch_row,
			   &fetch_start, &fetch_length);
  ck_assert_ptr_ne (parent, NULL);
  ck_assert_int_eq (parent_id, 2);
  ck_assert_int_eq (parent_position, 1);
  ck_assert_int_eq (parent->n_entries, 2);
  ck_assert_int_eq (parent->values[parent_position], 6);
  parent_fetcher_free (fetcher);
  bplus_tree_free (tree);
}

static void
do_check_divider_leaf_noop_odd (void)
{
  /* Check that the divider works correctly when inserting a new
     record in a leaf that has room in it. */
  struct bplus_divider *divider = divider_alloc (3);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 5, ((uint32_t) (-1)) };
  uint32_t orig_values[] = { 5, 0, 4 };
  const struct bplus_node original = {
    .n_entries = 1,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = 0,
    .is_leaf = 1
  };
  divider_setup (divider, 1, &original, 6, 6);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 2);
  ck_assert_int_eq (updated->keys[0], 5);
  ck_assert_int_eq (updated->keys[1], 6);
  ck_assert_int_eq (updated->values[0], 5);
  ck_assert_int_eq (updated->values[1], 6);
  ck_assert_int_eq (updated->values[2], 4);
  ck_assert_int_eq (updated->parent_node, 0);
  ck_assert_int_eq (updated->is_leaf, 1);
  /* There should not be a new node: */
  ck_assert_int_eq (new_node_id, ((uint32_t) (-1)));
  ck_assert_ptr_eq (new_node, NULL);
  divider_free (divider);
}

static void
do_check_divider_leaf_noop_even (void)
{
  /* Same, but with an even order. */
  struct bplus_divider *divider = divider_alloc (4);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 5, ((uint32_t) (-1)), ((uint32_t) (-1)) };
  uint32_t orig_values[] = { 5, 0, 0, 4 };
  const struct bplus_node original = {
    .n_entries = 1,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = 0,
    .is_leaf = 1
  };
  divider_setup (divider, 1, &original, 6, 6);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 2);
  ck_assert_int_eq (updated->keys[0], 5);
  ck_assert_int_eq (updated->keys[1], 6);
  ck_assert_int_eq (updated->values[0], 5);
  ck_assert_int_eq (updated->values[1], 6);
  ck_assert_int_eq (updated->values[3], 4);
  ck_assert_int_eq (updated->parent_node, 0);
  ck_assert_int_eq (updated->is_leaf, 1);
  /* There should not be a new node: */
  ck_assert_int_eq (new_node_id, ((uint32_t) (-1)));
  ck_assert_ptr_eq (new_node, NULL);
  divider_free (divider);
}

static void
do_check_divider_nonleaf_noop_odd (void)
{
  /* Check that the divider works correctly when inserting a new
     record in a node that has room in it. */
  struct bplus_divider *divider = divider_alloc (3);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 2, ((uint32_t) (-1)) };
  uint32_t orig_values[] = { 1, 2, 0 };
  const struct bplus_node original = {
    .n_entries = 1,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = ((uint32_t) (-1)),
    .is_leaf = 0
  };
  divider_setup (divider, 0, &original, 4, 3);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 2);
  ck_assert_int_eq (updated->keys[0], 2);
  ck_assert_int_eq (updated->keys[1], 4);
  ck_assert_int_eq (updated->values[0], 1);
  ck_assert_int_eq (updated->values[1], 2);
  ck_assert_int_eq (updated->values[2], 3);
  ck_assert_int_eq (updated->parent_node, ((uint32_t) (-1)));
  ck_assert_int_eq (updated->is_leaf, 0);
  /* There should not be a new node: */
  ck_assert_int_eq (new_node_id, ((uint32_t) (-1)));
  ck_assert_ptr_eq (new_node, NULL);
  divider_free (divider);
}

static void
do_check_divider_nonleaf_noop_even (void)
{
  /* Same, but with an even order. */
  struct bplus_divider *divider = divider_alloc (4);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 2, ((uint32_t) (-1)), ((uint32_t) (-1)) };
  uint32_t orig_values[] = { 1, 2, 0, 0 };
  const struct bplus_node original = {
    .n_entries = 1,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = ((uint32_t) (-1)),
    .is_leaf = 0
  };
  divider_setup (divider, 0, &original, 4, 3);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 2);
  ck_assert_int_eq (updated->keys[0], 2);
  ck_assert_int_eq (updated->keys[1], 4);
  ck_assert_int_eq (updated->values[0], 1);
  ck_assert_int_eq (updated->values[1], 2);
  ck_assert_int_eq (updated->values[2], 3);
  ck_assert_int_eq (updated->parent_node, ((uint32_t) (-1)));
  ck_assert_int_eq (updated->is_leaf, 0);
  /* There should not be a new node: */
  ck_assert_int_eq (new_node_id, ((uint32_t) (-1)));
  ck_assert_ptr_eq (new_node, NULL);
  divider_free (divider);
}

static void
do_check_divider_leaf_allocate_odd (void)
{
  /* Check that the divider works correctly when inserting a new
     record in a leaf that has no more room in it. */
  struct bplus_divider *divider = divider_alloc (3);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 3, 4 };
  uint32_t orig_values[] = { 3, 4, 0 };
  const struct bplus_node original = {
    .n_entries = 2,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = 0,
    .is_leaf = 1
  };
  divider_setup (divider, 1, &original, 5, 5);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (should_allocate, 1);
  int claimed = divider_allocated (divider, 3);
  ck_assert_int_eq (claimed, 1);
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 2);
  ck_assert_int_eq (updated->keys[0], 3);
  ck_assert_int_eq (updated->keys[1], 4);
  ck_assert_int_eq (updated->values[0], 3);
  ck_assert_int_eq (updated->values[1], 4);
  ck_assert_int_eq (updated->values[2], 3);	/* Now points to the new leaf. */
  ck_assert_int_eq (updated->parent_node, 0);
  ck_assert_int_eq (updated->is_leaf, 1);
  /* Check the new node: */
  ck_assert_int_eq (new_node_key, 4);
  ck_assert_int_eq (new_node_id, 3);
  ck_assert_int_eq (new_node->n_entries, 1);
  ck_assert_int_eq (new_node->keys[0], 5);
  ck_assert_int_eq (new_node->values[0], 5);
  ck_assert_int_eq (new_node->values[2], 0);
  ck_assert_int_eq (new_node->parent_node, 0);
  ck_assert_int_eq (new_node->is_leaf, 1);
  divider_free (divider);
}

static void
do_check_divider_leaf_allocate_even (void)
{
  /* Same, but with an even order. */
  struct bplus_divider *divider = divider_alloc (4);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 3, 4, 5 };
  uint32_t orig_values[] = { 3, 4, 5, 0 };
  const struct bplus_node original = {
    .n_entries = 3,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = 0,
    .is_leaf = 1
  };
  divider_setup (divider, 1, &original, 6, 6);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (should_allocate, 1);
  int claimed = divider_allocated (divider, 3);
  ck_assert_int_eq (claimed, 1);
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 2);
  ck_assert_int_eq (updated->keys[0], 3);
  ck_assert_int_eq (updated->keys[1], 4);
  ck_assert_int_eq (updated->values[0], 3);
  ck_assert_int_eq (updated->values[1], 4);
  ck_assert_int_eq (updated->values[3], 3);	/* Now points to the new leaf. */
  ck_assert_int_eq (updated->parent_node, 0);
  ck_assert_int_eq (updated->is_leaf, 1);
  /* Check the new node: */
  ck_assert_int_eq (new_node_key, 4);
  ck_assert_int_eq (new_node_id, 3);
  ck_assert_int_eq (new_node->n_entries, 2);
  ck_assert_int_eq (new_node->keys[0], 5);
  ck_assert_int_eq (new_node->keys[1], 6);
  ck_assert_int_eq (new_node->values[0], 5);
  ck_assert_int_eq (new_node->values[1], 6);
  ck_assert_int_eq (new_node->values[3], 0);
  ck_assert_int_eq (new_node->parent_node, 0);
  ck_assert_int_eq (new_node->is_leaf, 1);
  divider_free (divider);
}

static void
do_check_divider_nonleaf_allocate_odd (void)
{
  /* Check that the divider works correctly when inserting a new
     record in a node that has room in it. */
  struct bplus_divider *divider = divider_alloc (3);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 2, 3 };
  uint32_t orig_values[] = { 1, 2, 8 };
  const struct bplus_node original = {
    .n_entries = 2,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = ((uint32_t) (-1)),
    .is_leaf = 0
  };
  divider_setup (divider, 0, &original, 4, 3);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_key, &new_node_id,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (should_allocate, 1);
  int claimed = divider_allocated (divider, 3);
  ck_assert_int_eq (claimed, 1);
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 1);
  ck_assert_int_eq (updated->keys[0], 2);
  ck_assert_int_eq (updated->values[0], 1);
  ck_assert_int_eq (updated->values[1], 2);
  ck_assert_int_eq (updated->parent_node, ((uint32_t) (-1)));
  ck_assert_int_eq (updated->is_leaf, 0);
  /* Check the new node: */
  ck_assert_int_eq (new_node_key, 3);
  ck_assert_int_eq (new_node_id, 3);
  ck_assert_int_eq (new_node->n_entries, 1);
  ck_assert_int_eq (new_node->keys[0], 4);
  ck_assert_int_eq (new_node->values[0], 8);
  ck_assert_int_eq (new_node->values[1], 3);
  ck_assert_int_eq (new_node->parent_node, ((uint32_t) (-1)));
  ck_assert_int_eq (new_node->is_leaf, 0);
  divider_free (divider);
}

static void
do_check_divider_nonleaf_allocate_even (void)
{
  /* Same, but with an even order. */
  /* Check that the divider works correctly when inserting a new
     record in a node that has room in it. */
  struct bplus_divider *divider = divider_alloc (4);
  if (divider == NULL)
    {
      abort ();
    }
  uint32_t orig_keys[] = { 2, 3, 4 };
  uint32_t orig_values[] = { 1, 2, 8, 9 };
  const struct bplus_node original = {
    .n_entries = 3,
    .keys = orig_keys,
    .values = orig_values,
    .parent_node = ((uint32_t) (-1)),
    .is_leaf = 0
  };
  divider_setup (divider, 0, &original, 5, 3);
  const struct bplus_node *updated = NULL;
  const struct bplus_node *new_node = NULL;
  uint32_t new_node_id, new_node_key;
  int done, should_allocate;
  divider_status (divider, &done, &updated, &new_node_key, &new_node_id,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (should_allocate, 1);
  int claimed = divider_allocated (divider, 3);
  ck_assert_int_eq (claimed, 1);
  divider_status (divider, &done, &updated, &new_node_id, &new_node_key,
		  &new_node, &should_allocate);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (should_allocate, 0);
  /* Check the updated node: */
  ck_assert_int_eq (updated->n_entries, 2);
  ck_assert_int_eq (updated->keys[0], 2);
  ck_assert_int_eq (updated->keys[1], 3);
  ck_assert_int_eq (updated->values[0], 1);
  ck_assert_int_eq (updated->values[1], 2);
  ck_assert_int_eq (updated->values[2], 8);
  ck_assert_int_eq (updated->parent_node, ((uint32_t) (-1)));
  ck_assert_int_eq (updated->is_leaf, 0);
  /* Check the new node: */
  ck_assert_int_eq (new_node_key, 4);
  ck_assert_int_eq (new_node_id, 3);
  ck_assert_int_eq (new_node->n_entries, 1);
  ck_assert_int_eq (new_node->keys[0], 5);
  ck_assert_int_eq (new_node->values[0], 9);
  ck_assert_int_eq (new_node->values[1], 3);
  ck_assert_int_eq (new_node->parent_node, ((uint32_t) (-1)));
  ck_assert_int_eq (new_node->is_leaf, 0);
  divider_free (divider);
}

static void
do_check_insert_depth_1 (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_range *range = bplus_range_alloc (tree);
  struct bplus_insertion *insertion = bplus_insertion_alloc (tree);
  if (range == NULL || insertion == NULL)
    {
      abort ();
    }
  uint32_t node_2_keys[2] = { 3, 4 };
  uint32_t node_2_values[3] = { 3, 4, 0 };
  const struct bplus_node node_2 = {
    .n_entries = 2,
    .keys = node_2_keys,
    .values = node_2_values,
    .is_leaf = 1,
    .parent_node = 0
  };
  int err = bplus_range_setup (range, 2, &node_2, 2, 2, &node_2, 2);
  ck_assert_int_eq (err, 0);
  bplus_insertion_setup (insertion, 5, range, 0);
  int done;
  size_t n_fetches_to_do, n_allocations_to_do, n_stores_to_do;
  size_t start_fetch_to_do = 0;
  size_t max_fetches_to_do = 256;
  size_t fetch_rows[256];
  size_t fetch_starts[256];
  size_t fetch_lengths[256];
  size_t start_store_to_do = 0;
  size_t max_stores_to_do = 256;
  size_t store_rows[256];
  size_t store_starts[256];
  size_t store_lengths[256];
  const uint32_t *stores[256];
  /* There are multiple steps: first, I will have to save to allocate
     a new node, 3, because 2 overflows. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 1);
  ck_assert_int_eq (n_stores_to_do, 0);
  int claimed = bplus_insertion_allocated (insertion, 3);
  ck_assert_int_eq (claimed, 1);
  /* Then, I will have to save both the old node, with an updated next
     leaf, and the new node. Also, constructing the recursive problem
     will require me to fetch the parent, node 0. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 1);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 2);
  ck_assert_int_eq (fetch_rows[0], 0);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  ck_assert_int_eq (store_rows[0], 2);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  ck_assert_int_eq (stores[0][1], 4);	/* Keep the 4 -> 4 record in the old leaf */
  ck_assert_int_eq (stores[0][4], 3);	/* The next leaf is updated */
  ck_assert_int_eq (store_rows[1], 3);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_lengths[1], 7);
  ck_assert_int_eq (stores[1][0], 5);	/* The new leaf should only have 5 -> 5 */
  ck_assert_int_eq (stores[1][1], (uint32_t) (-1));
  ck_assert_int_eq (stores[1][4], 0);	/* No next leaf */
  ck_assert_int_eq (stores[1][5], 0);	/* Inherit the same parent as 2 */
  const uint32_t initial_root[7] = {
    /* Keys: */
    2, ((uint32_t) (-1)),
    /* Subtrees: */
    1, 2, 0,
    /* Parent: */
    ((uint32_t) (-1)),
    /* Flags: */
    0
  };
  bplus_insertion_data (insertion, 0, 0, 7, initial_root);
  bplus_insertion_updated (insertion, 2);
  bplus_insertion_updated (insertion, 3);
  /* Then, the problem will have to recurse by inserting (4 -> subtree
     3) in node 0. But first, node 0 will be updated to insert 4 ->
     subtree 3, and I will be asked to save it. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 1);
  ck_assert_int_eq (store_rows[0], 0);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  /* Enumerate the keys: */
  ck_assert_int_eq (stores[0][0], 2);
  ck_assert_int_eq (stores[0][1], 4);
  /* Enumerate the values: */
  ck_assert_int_eq (stores[0][2], 1);
  ck_assert_int_eq (stores[0][3], 2);
  ck_assert_int_eq (stores[0][4], 3);
  /* Parent: */
  ck_assert_int_eq (stores[0][5], (uint32_t) (-1));
  /* Flags: */
  ck_assert_int_eq (stores[0][6], 0);
  /* Done. */
  bplus_insertion_updated (insertion, 0);
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 0);
  bplus_insertion_updated (insertion, 0);
  bplus_insertion_free (insertion);
  bplus_range_free (range);
  bplus_tree_free (tree);
}

static void
do_check_insert_depth_2 (void)
{
  struct bplus_tree *tree = bplus_tree_alloc (3);
  if (tree == NULL)
    {
      abort ();
    }
  struct bplus_range *range = bplus_range_alloc (tree);
  struct bplus_insertion *insertion = bplus_insertion_alloc (tree);
  if (range == NULL || insertion == NULL)
    {
      abort ();
    }
  uint32_t node_6_keys[2] = { 4, 6 };
  uint32_t node_6_values[3] = { 4, 6, 7 };
  const struct bplus_node node_6 = {
    .n_entries = 2,
    .keys = node_6_keys,
    .values = node_6_values,
    .is_leaf = 1,
    .parent_node = 2
  };
  int err = bplus_range_setup (range, 6, &node_6, 1, 6, &node_6, 1);
  ck_assert_int_eq (err, 0);
  bplus_insertion_setup (insertion, 5, range, 0);
  int done;
  size_t n_fetches_to_do, n_allocations_to_do, n_stores_to_do;
  size_t start_fetch_to_do = 0;
  size_t max_fetches_to_do = 256;
  size_t fetch_rows[256];
  size_t fetch_starts[256];
  size_t fetch_lengths[256];
  size_t start_store_to_do = 0;
  size_t max_stores_to_do = 256;
  size_t store_rows[256];
  size_t store_starts[256];
  size_t store_lengths[256];
  const uint32_t *stores[256];
  /* After the initial bubble, the algorithm realizes that there is
     not enough room in the leaf, so asks to allocate another leaf. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 1);
  ck_assert_int_eq (n_stores_to_do, 0);
  int claimed = bplus_insertion_allocated (insertion, 8);
  ck_assert_int_eq (claimed, 1);
  /* Now, both leaves are saved, and the parent is fetched. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 1);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 2);
  ck_assert_int_eq (fetch_rows[0], 2);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  ck_assert_int_eq (store_rows[0], 6);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  ck_assert_int_eq (stores[0][1], 5);	/* 5 -> 5 is added to the old leaf, replacing 6 -> 6 */
  ck_assert_int_eq (stores[0][4], 8);	/* The next leaf is updated */
  ck_assert_int_eq (store_rows[1], 8);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_lengths[1], 7);
  ck_assert_int_eq (stores[1][0], 6);	/* 6 -> 6 is the only record */
  /* given to the new leaf. */
  ck_assert_int_eq (stores[1][1], ((uint32_t) (-1)));
  const uint32_t initial_2[7] = {
    /* Keys */
    3, 6,
    /* Children */
    5, 6, 7,
    /* Parent */
    0,
    /* Flags */
    0
  };
  bplus_insertion_data (insertion, 2, 0, 7, initial_2);
  bplus_insertion_updated (insertion, 6);
  bplus_insertion_updated (insertion, 8);
  /* The insertion finds out that this node is not large enough. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 1);
  ck_assert_int_eq (n_stores_to_do, 0);
  claimed = bplus_insertion_allocated (insertion, 9);
  ck_assert_int_eq (claimed, 1);
  /* Both nodes 2 and 9 need to be saved. Also, the new children of 9
     (i.e. node 8 and 7) are fetched, so that their parents can be set
     to 9. Since the insertion currently evicts updated nodes from the
     cache, node 8 is not present so it has to be fetched. In the
     future, if the insertion updates the cache when it updates a
     node, then node 8 will be in the cache already so it will appear
     as a third update to do. Finally, the parent is fetched to
     prepare the recursive call. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 3);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 2);
  ck_assert_int_eq (fetch_rows[0], 8);
  ck_assert_int_eq (fetch_starts[0], 0);
  ck_assert_int_eq (fetch_lengths[0], 7);
  ck_assert_int_eq (fetch_rows[1], 7);
  ck_assert_int_eq (fetch_starts[1], 0);
  ck_assert_int_eq (fetch_lengths[1], 7);
  ck_assert_int_eq (fetch_rows[2], 0);
  ck_assert_int_eq (fetch_starts[2], 0);
  ck_assert_int_eq (fetch_lengths[2], 7);
  ck_assert_int_eq (store_rows[0], 2);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  /* Keys: */
  ck_assert_int_eq (stores[0][0], 3);
  ck_assert_int_eq (stores[0][1], ((uint32_t) (-1)));
  /* Subtrees: */
  ck_assert_int_eq (stores[0][2], 5);
  ck_assert_int_eq (stores[0][3], 6);
  ck_assert_int_eq (stores[0][4], 0);
  /* Parent: */
  ck_assert_int_eq (stores[0][5], 0);
  /* Flags: */
  ck_assert_int_eq (stores[0][6], 0);
  ck_assert_int_eq (store_rows[1], 9);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_lengths[1], 7);
  /* Keys: */
  ck_assert_int_eq (stores[1][0], 6);
  ck_assert_int_eq (stores[1][1], ((uint32_t) (-1)));
  /* Subtrees: */
  ck_assert_int_eq (stores[1][2], 8);
  ck_assert_int_eq (stores[1][3], 7);
  ck_assert_int_eq (stores[1][4], 0);
  /* Parent: */
  ck_assert_int_eq (stores[1][5], 0);
  /* Flags: */
  ck_assert_int_eq (stores[1][6], 0);
  const uint32_t initial_root[7] = {
    /* Keys */
    2, ((uint32_t) (-1)),
    /* Subtrees */
    1, 2, 0,
    /* Parent */
    ((uint32_t) (-1)),
    /* Flags */
    0
  };
  const uint32_t updated_8[7] = {
    /* Keys: */
    6, ((uint32_t) (-1)),
    /* Values: */
    6, 0,
    /* Next leaf: */
    7,
    /* Parent (inherited from cell 6, needs to be fixed): */
    2,
    /* Flags: */
    ((uint32_t) ((uint32_t) 1) << 31)
  };
  const uint32_t updated_7[7] = {
    /* Keys: */
    7, ((uint32_t) (-1)),
    /* Values: */
    7, 0,
    /* Next leaf: */
    0,
    /* Parent (needs to be fixed): */
    2,
    /* Flags: */
    ((uint32_t) ((uint32_t) 1) << 31)
  };
  bplus_insertion_data (insertion, 0, 0, 7, initial_root);
  bplus_insertion_data (insertion, 8, 0, 7, updated_8);
  bplus_insertion_data (insertion, 7, 0, 7, updated_7);
  bplus_insertion_updated (insertion, 2);
  bplus_insertion_updated (insertion, 9);
  /* Now we need to save the parent of nodes 8 and 7, to 9. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 2);
  ck_assert_int_eq (store_rows[0], 8);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  ck_assert_int_eq (stores[0][5], 9);
  ck_assert_int_eq (store_rows[1], 7);
  ck_assert_int_eq (store_starts[1], 0);
  ck_assert_int_eq (store_lengths[1], 7);
  ck_assert_int_eq (stores[1][5], 9);
  bplus_insertion_updated (insertion, 8);
  bplus_insertion_updated (insertion, 7);
  /* Finally, the 9 subtree need to be inserted in the root. The
     subtree fits in the root, no need to go further up. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 0);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 1);
  ck_assert_int_eq (store_rows[0], 0);
  ck_assert_int_eq (store_starts[0], 0);
  ck_assert_int_eq (store_lengths[0], 7);
  /* Keys: */
  ck_assert_int_eq (stores[0][0], 2);
  ck_assert_int_eq (stores[0][1], 5);
  /* Subtrees: */
  ck_assert_int_eq (stores[0][2], 1);
  ck_assert_int_eq (stores[0][3], 2);
  ck_assert_int_eq (stores[0][4], 9);
  /* Parent: */
  ck_assert_int_eq (stores[0][5], ((uint32_t) (-1)));
  /* Flags: */
  ck_assert_int_eq (stores[0][6], 0);
  bplus_insertion_updated (insertion, 0);
  /* Finished. */
  bplus_insertion_status (insertion, &done, &n_fetches_to_do,
			  start_fetch_to_do, max_fetches_to_do, fetch_rows,
			  fetch_starts, fetch_lengths, &n_allocations_to_do,
			  &n_stores_to_do, start_store_to_do,
			  max_stores_to_do, store_rows, store_starts,
			  store_lengths, stores);
  ck_assert_int_eq (done, 1);
  ck_assert_int_eq (n_fetches_to_do, 0);
  ck_assert_int_eq (n_allocations_to_do, 0);
  ck_assert_int_eq (n_stores_to_do, 0);
  bplus_insertion_free (insertion);
  bplus_range_free (range);
  bplus_tree_free (tree);
}

static void
do_check_insert_and_grow (size_t order, int back)
{
  static const uint32_t records[] = {
    99, 141, 173, 99, 46, 150, 125, 53, 213, 138, 23, 39, 223, 51, 108, 134,
    189, 201, 73, 34, 223, 60, 151, 64, 250, 72, 105, 149, 212, 44, 160, 29,
    190, 5, 245, 54, 193, 130, 234, 170, 203, 252, 143, 88, 206, 151, 143,
    33, 189, 40, 95, 154, 3, 75, 7, 93, 90, 250, 178, 111, 113, 186, 136,
    35, 232, 106, 130, 14, 190, 238, 1, 230, 18, 206, 107, 250, 44, 133, 86,
    219, 231, 136, 19, 79, 80, 68, 159, 82, 100, 33, 116, 189, 65, 161, 8,
    97, 30, 144, 139, 215, 234, 51, 52, 137, 29, 211, 68, 240, 231, 26, 65,
    133, 127, 35, 204, 10, 81, 38, 166, 202, 108, 160, 229, 24, 220, 64, 42,
    233
#ifdef LOTS_OF_INSERTIONS_TESTS
      , 155, 88, 244, 169, 42, 255, 158, 130, 16, 82, 149, 3, 131, 168, 77,
    133, 100, 130, 112, 23, 232, 180, 43, 50, 104, 199, 2, 23, 65, 202, 118,
    119, 137, 16, 132, 129, 244, 99, 140, 248, 37, 223, 78, 82, 86, 139, 78,
    238, 2, 207, 215, 61, 165, 89, 153, 122, 144, 12, 65, 48, 176, 189, 146,
    187, 2, 123, 218, 206, 36, 196, 219, 37, 220, 66, 73, 232, 89, 187, 182,
    99, 54, 145, 196, 4, 121, 140, 38, 52, 162, 107, 251, 177, 152, 83, 232,
    54, 134, 191, 226, 70, 23, 219, 205, 182, 72, 162, 42, 35, 17, 140, 225,
    22, 209, 13, 106, 255, 183, 25, 10, 91, 196, 50, 234, 85, 239, 70, 28,
    116, 250, 92, 254, 164, 249, 149, 166, 42, 130, 36, 7, 179, 59, 168,
    214, 164, 140, 113, 57, 182, 183, 14, 123, 121, 0, 203, 219, 240, 224,
    197, 86, 246, 254, 62, 20, 36, 178, 189, 122, 81, 226, 228, 103, 35, 54,
    34, 121, 120, 89, 43, 156, 22, 169, 72, 95, 24, 231, 41, 38, 4, 129,
    100, 56, 127, 66, 251, 195, 106, 235, 200, 147, 244, 52, 228, 98, 239,
    72, 189, 250, 21, 107, 20, 204, 133, 8, 197, 85, 236, 169, 52, 150, 111,
    184, 172, 103, 54, 130, 236, 37, 166, 213, 157, 17, 115, 31, 223, 70,
    226, 212, 84, 47, 224, 100, 238, 3, 115, 143, 103, 168, 130, 235, 178,
    19, 165, 248, 126, 33, 51, 167, 210, 189, 38, 209, 196, 215, 23, 166,
    103, 24, 246, 74, 242, 75, 252, 226, 128, 95, 210, 51, 44, 122, 171, 65,
    75, 1, 76, 57, 53, 185, 14, 129, 129, 208, 13, 234, 143, 232, 248, 42,
    165, 105, 62, 38, 39, 181, 247, 164, 101, 98, 54, 220, 21, 39, 41, 61,
    7, 151, 97, 2, 225, 215, 123, 28, 215, 24, 9, 231, 209, 69, 7, 167, 98,
    253, 53, 142, 63, 6, 222, 84, 174, 195, 200, 101, 155, 18, 70, 84, 246,
    162, 26, 242, 171, 228, 179, 144, 81, 233, 247, 237, 209, 82, 193, 107,
    64, 215, 178, 117, 190, 152, 49, 181, 222, 219, 172, 24, 92, 35, 100,
    184, 19, 100, 122, 85, 157, 146, 246, 56, 150, 166, 108, 226, 155, 186,
    173, 72, 166, 255, 185, 109, 9, 138, 95, 121, 141, 138, 218, 150, 81,
    216, 168, 172, 77, 225, 193, 207, 127, 171, 201, 209, 13, 128, 176, 252,
    0, 123, 241, 196, 3, 233, 86, 155, 12, 136, 187, 86, 26, 108, 190, 21,
    88, 88, 84, 7, 3, 187, 100, 210, 57, 104, 218, 109, 11, 230, 42, 56,
    164, 194, 118, 100, 131, 163, 81, 48, 249, 251, 132, 125, 114, 119, 123,
    220, 38, 255, 99, 53, 108, 156, 49, 247, 218, 30, 162, 155, 144, 100,
    40, 163, 250, 110, 28, 207, 43, 81, 29, 100, 87, 49, 225, 189, 7, 230,
    123, 236, 24, 171, 31, 155, 204, 72, 130, 229, 32, 0, 92, 53, 138, 244,
    74, 17, 196, 204, 160, 222, 107, 119, 97, 107, 27, 112, 205, 8, 21, 90,
    177, 36, 54, 29, 69, 13, 61, 198, 27, 160, 176, 85, 248, 197, 77, 198,
    173, 54, 104, 77, 195, 196, 222, 106, 15, 254, 117, 73, 41, 235, 188,
    248, 14, 123, 223, 229, 216, 232, 68, 119, 73, 98, 247, 113, 72, 53, 46,
    114, 81, 197, 68, 102, 20, 0, 138, 61, 34, 160, 122, 2, 251, 108, 84,
    200, 91, 209, 125, 140, 234, 156, 93, 84, 249, 140, 242, 172, 230, 60,
    89, 17, 84, 237, 14, 154, 107, 242, 78, 29, 67, 4, 180, 105, 8, 75, 22,
    13, 54, 28, 188, 195, 26, 61, 223, 254, 209, 69, 206, 136, 67, 35, 34,
    8, 93, 77, 44, 49, 177, 68, 252, 102, 1, 99, 177, 35, 48, 171, 30, 42,
    225, 19, 104, 112, 82, 99, 235, 103, 137, 229, 53, 227, 67, 57, 89, 228,
    135, 129, 19, 250, 142, 213, 36, 108, 7, 134, 204, 246, 64, 197, 71, 16,
    230, 248, 182, 141, 15, 197, 3, 174, 74, 11, 63, 5, 57, 66, 37, 22, 188,
    230, 207, 208, 204, 190, 135, 254, 125, 67, 5, 208, 14, 114, 83, 203,
    224, 179, 63, 221, 42, 225, 162, 12, 127, 176, 208, 59, 121, 156, 231,
    30, 249, 94, 150, 63, 214, 199, 142, 25, 50, 65, 188, 93, 88, 74, 220,
    240, 161, 147, 92, 197, 9, 183, 152, 30, 70, 33, 112, 117, 214, 79, 178,
    248, 22, 169, 141, 166, 128, 194, 229, 159, 210, 161, 38, 211, 178, 91,
    163, 53, 107, 52, 79, 224, 48, 130, 125, 175, 111, 194, 74, 125, 117,
    155, 186, 164, 22, 16, 60, 36, 51, 237, 132, 101, 169, 153, 122, 133,
    161, 107, 70, 156, 17, 81, 99, 227, 129, 44, 51, 228, 75, 169, 251, 215,
    3, 237, 159, 29, 100, 124, 243, 229, 103, 165, 106, 19, 62, 208, 7, 251,
    33, 125, 87, 207, 54, 175, 120, 136, 44, 15, 220, 185, 174, 189, 9, 110,
    9, 205, 211, 4, 91, 236, 234, 93, 65, 253, 135, 42, 225, 207, 33, 159,
    86, 186, 80, 122, 20, 179, 51, 171, 97, 117, 97, 196, 175, 112, 206,
    181, 28
#endif /* LOTS_OF_INSERTIONS_TESTS */
  };
  const size_t n_records = sizeof (records) / sizeof (records[0]);
  size_t n_occurences[256] = { 0 };
  struct bplus_tree *tree = bplus_tree_alloc (order);
  /* For an order 4 tree, B+ has at most n_records / 2 leaves (because
     leaves can be no less than half full), so counting the inner
     nodes, it has less than n_records leaves. However, for an order
     3, it is possible to have 1 record per leaf, so n_records
     leaves. So it has less than n_records * 2 leaves in total. */
  uint32_t *storage =
    malloc (n_records * 2 * (2 * order + 1) * sizeof (uint32_t));
  if (tree == NULL || storage == NULL)
    {
      abort ();
    }
  struct bplus_finder *finder = bplus_finder_alloc (tree);
  struct bplus_fetcher *root_fetcher = bplus_fetcher_alloc (tree);
  struct bplus_range *range = bplus_range_alloc (tree);
  struct bplus_insertion *insertion = bplus_insertion_alloc (tree);
  if (finder == NULL || root_fetcher == NULL || range == NULL
      || insertion == NULL)
    {
      abort ();
    }
  size_t n_nodes = 1;
  /* Set the initial root. */
  bplus_prime (order, storage);
  for (size_t i_record = 0; i_record < n_records; i_record++)
    {
      /* First, check that we see each key the right number of times. */
      for (uint32_t key = 0; key < 256; key++)
	{
	  struct bplus_key k;
	  k.type = BPLUS_KEY_KNOWN;
	  k.arg.known = key;
	  bplus_fetcher_setup (root_fetcher, 0);
	  const struct bplus_node *root = NULL;
	  size_t fetch_row, fetch_start, fetch_length;
	  while ((root =
		  bplus_fetcher_status (root_fetcher, &fetch_row,
					&fetch_start, &fetch_length)) == NULL)
	    {
	      ck_assert_int_lt (fetch_row, n_nodes);
	      ck_assert_int_le (fetch_start + fetch_length, 2 * order + 1);
	      bplus_fetcher_data (root_fetcher, fetch_row, fetch_start,
				  fetch_length,
				  &(storage
				    [fetch_row * (2 * order + 1) +
				     fetch_start]));
	    }
	  bplus_finder_setup (finder, &k, 0, root);
	  int finder_done;
	  do
	    {
	      size_t n_fetches_to_do;
	      size_t start_fetch = 0;
	      size_t max_fetches = 256;
	      size_t fetch_rows[256];
	      size_t fetch_starts[256];
	      size_t fetch_lengths[256];
	      size_t n_compares_to_do;
	      size_t start_compare = 0;
	      size_t max_compares = 256;
	      struct bplus_key as[256];
	      struct bplus_key bs[256];
	      bplus_finder_status (finder, &finder_done, range,
				   &n_fetches_to_do, start_fetch, max_fetches,
				   fetch_rows, fetch_starts, fetch_lengths,
				   &n_compares_to_do, start_compare,
				   max_compares, as, bs);
	      for (size_t i = 0; i < n_fetches_to_do && i < max_fetches; i++)
		{
		  const size_t fetch_row = fetch_rows[i];
		  const size_t fetch_start = fetch_starts[i];
		  const size_t fetch_length = fetch_lengths[i];
		  ck_assert_int_lt (fetch_row, n_nodes);
		  ck_assert_int_le (fetch_start + fetch_length,
				    2 * order + 1);
		  bplus_finder_data (finder, fetch_row, fetch_start,
				     fetch_length,
				     &(storage
				       [fetch_row * (2 * order + 1) +
					fetch_start]));
		}
	      for (size_t i = 0; i < n_compares_to_do && i < max_compares;
		   i++)
		{
		  const struct bplus_key a = as[i];
		  const struct bplus_key b = bs[i];
		  ck_assert_int_eq (a.type, BPLUS_KEY_KNOWN);
		  ck_assert_int_eq (b.type, BPLUS_KEY_KNOWN);
		  if (a.arg.known < b.arg.known)
		    {
		      bplus_finder_compared (finder, &a, &b, -1);
		    }
		  else if (a.arg.known > b.arg.known)
		    {
		      bplus_finder_compared (finder, &a, &b, +1);
		    }
		  else
		    {
		      bplus_finder_compared (finder, &a, &b, 0);
		    }
		}
	    }
	  while (!finder_done);
	  size_t n_total = 0;
	  while (1)
	    {
	      size_t start = 0;
	      size_t max = 256;
	      struct bplus_key sample_keys[256];
	      uint32_t sample_values[256];
	      int has_next;
	      size_t n_load_requests;
	      size_t start_load_requests = 0;
	      size_t max_load_requests = 256;
	      size_t rows[256];
	      size_t starts[256];
	      size_t lengths[256];
	      size_t n_added = 0;
	      do
		{
		  n_added =
		    bplus_range_get (range, start, max, sample_keys,
				     sample_values, &has_next,
				     &n_load_requests, start_load_requests,
				     max_load_requests, rows, starts,
				     lengths);
		  for (size_t i = 0;
		       i < n_load_requests && i < max_load_requests; i++)
		    {
		      const size_t fetch_row = rows[i];
		      const size_t fetch_start = starts[i];
		      const size_t fetch_length = lengths[i];
		      ck_assert_int_lt (fetch_row, n_nodes);
		      ck_assert_int_le (fetch_start + fetch_length,
					2 * order + 1);
		      bplus_range_data (range, fetch_row, fetch_start,
					fetch_length,
					&(storage
					  [fetch_row * (2 * order + 1) +
					   fetch_start]));
		    }
		}
	      while (n_load_requests != 0);
	      for (size_t i = 0; i < n_added && i < max; i++)
		{
		  /* Check that all the keys are equal to the search
		     key, and that all keys are equal to the
		     values. */
		  ck_assert_int_eq (sample_keys[i].type, BPLUS_KEY_KNOWN);
		  ck_assert_int_eq (sample_keys[i].arg.known, key);
		  ck_assert_int_eq (sample_values[i],
				    sample_keys[i].arg.known);
		}
	      n_total += n_added;
	      if (has_next)
		{
		  int error_next = bplus_range_next (range);
		  ck_assert_int_eq (error_next, 0);
		}
	      else
		{
		  break;
		}
	    }
	  ck_assert_int_eq (n_total, n_occurences[key]);
	}
      const uint32_t key = records[i_record];
      /* Then, we add 1 occurence of key. */
      n_occurences[key] += 1;
      struct bplus_key k;
      k.type = BPLUS_KEY_KNOWN;
      k.arg.known = key;
      bplus_fetcher_setup (root_fetcher, 0);
      const struct bplus_node *root = NULL;
      size_t fetch_row, fetch_start, fetch_length;
      while ((root =
	      bplus_fetcher_status (root_fetcher, &fetch_row, &fetch_start,
				    &fetch_length)) == NULL)
	{
	  ck_assert_int_lt (fetch_row, n_nodes);
	  ck_assert_int_le (fetch_start + fetch_length, 2 * order + 1);
	  bplus_fetcher_data (root_fetcher, fetch_row, fetch_start,
			      fetch_length,
			      &(storage
				[fetch_row * (2 * order + 1) + fetch_start]));
	}
      bplus_finder_setup (finder, &k, 0, root);
      int finder_done;
      do
	{
	  size_t n_fetches_to_do;
	  size_t start_fetch = 0;
	  size_t max_fetches = 256;
	  size_t fetch_rows[256];
	  size_t fetch_starts[256];
	  size_t fetch_lengths[256];
	  size_t n_compares_to_do;
	  size_t start_compare = 0;
	  size_t max_compares = 256;
	  struct bplus_key as[256];
	  struct bplus_key bs[256];
	  bplus_finder_status (finder, &finder_done, range, &n_fetches_to_do,
			       start_fetch, max_fetches, fetch_rows,
			       fetch_starts, fetch_lengths, &n_compares_to_do,
			       start_compare, max_compares, as, bs);
	  for (size_t i = 0; i < n_fetches_to_do && i < max_fetches; i++)
	    {
	      const size_t fetch_row = fetch_rows[i];
	      const size_t fetch_start = fetch_starts[i];
	      const size_t fetch_length = fetch_lengths[i];
	      ck_assert_int_lt (fetch_row, n_nodes);
	      ck_assert_int_le (fetch_start + fetch_length, 2 * order + 1);
	      bplus_finder_data (finder, fetch_row, fetch_start, fetch_length,
				 &(storage
				   [fetch_row * (2 * order + 1) +
				    fetch_start]));
	    }
	  for (size_t i = 0; i < n_compares_to_do && i < max_compares; i++)
	    {
	      const struct bplus_key a = as[i];
	      const struct bplus_key b = bs[i];
	      ck_assert_int_eq (a.type, BPLUS_KEY_KNOWN);
	      ck_assert_int_eq (b.type, BPLUS_KEY_KNOWN);
	      if (a.arg.known < b.arg.known)
		{
		  bplus_finder_compared (finder, &a, &b, -1);
		}
	      else if (a.arg.known > b.arg.known)
		{
		  bplus_finder_compared (finder, &a, &b, +1);
		}
	      else
		{
		  bplus_finder_compared (finder, &a, &b, 0);
		}
	    }
	}
      while (!finder_done);
      /* Now, add something in the range. */
      bplus_insertion_setup (insertion, key, range, back);
      int insertion_done;
      do
	{
	  size_t n_fetches_to_do;
	  size_t start_fetch_to_do = 0;
	  size_t max_fetches_to_do = 256;
	  size_t fetch_rows[256];
	  size_t fetch_starts[256];
	  size_t fetch_lengths[256];
	  size_t n_allocations_to_do;
	  size_t n_stores_to_do;
	  size_t start_store_to_do = 0;
	  size_t max_stores_to_do = 256;
	  size_t store_rows[256];
	  size_t store_starts[256];
	  size_t store_lengths[256];
	  const uint32_t *stores[256];
	  bplus_insertion_status (insertion, &insertion_done,
				  &n_fetches_to_do, start_fetch_to_do,
				  max_fetches_to_do, fetch_rows, fetch_starts,
				  fetch_lengths, &n_allocations_to_do,
				  &n_stores_to_do, start_store_to_do,
				  max_stores_to_do, store_rows, store_starts,
				  store_lengths, stores);
#ifdef VERBOSE_INSERTION_CHECK
	  fprintf (stderr, "%s:%d: %lu, %d: \
%lu fetches, %lu allocations, %lu stores…\n", __FILE__, __LINE__, order, back, n_fetches_to_do, n_allocations_to_do, n_stores_to_do);
#endif /* VERBOSE_INSERTION_CHECK */
	  for (size_t i = 0; i < n_fetches_to_do && i < max_fetches_to_do;
	       i++)
	    {
	      const size_t fetch_row = fetch_rows[i];
	      const size_t fetch_start = fetch_starts[i];
	      const size_t fetch_length = fetch_lengths[i];
	      ck_assert_int_lt (fetch_row, n_nodes);
	      ck_assert_int_le (fetch_start + fetch_length, 2 * order + 1);
#ifdef VERBOSE_INSERTION_CHECK
	      fprintf (stderr, "%s:%d: %lu, %d: \
Fetch %lu: [", __FILE__, __LINE__, order, back, fetch_row);
	      for (size_t j = 0; j < fetch_start; j++)
		{
		  fprintf (stderr, " .");
		}
	      for (size_t j = 0;
		   j < fetch_length && j + fetch_start < 2 * order + 1; j++)
		{
		  fprintf (stderr, " %u",
			   storage[fetch_row * (2 * order + 1) + fetch_start +
				   j]);
		}
	      for (size_t j = fetch_start + fetch_length; j < 2 * order + 1;
		   j++)
		{
		  fprintf (stderr, " .");
		}
	      fprintf (stderr, "].\n");
#endif /* VERBOSE_INSERTION_CHECK */
	      bplus_insertion_data (insertion, fetch_row, fetch_start,
				    fetch_length,
				    &(storage
				      [fetch_row * (2 * order + 1) +
				       fetch_start]));
	    }
	  for (size_t i = 0; i < n_allocations_to_do; i++)
	    {
	      /* We allocated 2 * n_records nodes: */
	      ck_assert_int_lt (n_nodes, 2 * n_records);
	      uint32_t next = n_nodes;
	      n_nodes++;
#ifdef VERBOSE_INSERTION_CHECK
	      fprintf (stderr, "%s:%d: %lu, %d: \
Allocate: %u.\n", __FILE__, __LINE__, order, back, next);
#endif /* VERBOSE_INSERTION_CHECK */
	      int claimed = bplus_insertion_allocated (insertion, next);
	      ck_assert_int_eq (claimed, 1);
	    }
	  for (size_t i = 0; i < n_stores_to_do && i < max_stores_to_do; i++)
	    {
	      const size_t store_row = store_rows[i];
	      const size_t store_start = store_starts[i];
	      const size_t store_length = store_lengths[i];
	      ck_assert_int_lt (store_row, n_nodes);
	      ck_assert_int_le (store_start + store_length, 2 * order + 1);
	      memcpy (&(storage[store_row * (2 * order + 1) + store_start]),
		      stores[i], store_length * sizeof (uint32_t));
#ifdef VERBOSE_INSERTION_CHECK
	      fprintf (stderr, "%s:%d: %lu, %d: \
Update %lu (%lu, %lu, [", __FILE__, __LINE__, order, back, store_row, store_start, store_length);
	      for (size_t j = 0; j < store_start; j++)
		{
		  fprintf (stderr, " .");
		}
	      for (size_t j = 0; j < store_length; j++)
		{
		  fprintf (stderr, " %u", stores[i][j]);
		}
	      for (size_t j = store_start + store_length; j < 2 * order + 1;
		   j++)
		{
		  fprintf (stderr, " .");
		}
	      fprintf (stderr, "]).\n");
#endif /* VERBOSE_INSERTION_CHECK */
	      bplus_insertion_updated (insertion, store_row);
	    }
#ifdef VERBOSE_INSERTION_CHECK
	  fprintf (stderr, "%s:%d: %lu, %d: \
%lu fetches, %lu allocations, %lu stores done.\n", __FILE__, __LINE__, order, back, n_fetches_to_do, n_allocations_to_do, n_stores_to_do);
#endif /* VERBOSE_INSERTION_CHECK */
	}
      while (!insertion_done);
#ifdef VERBOSE_INSERTION_CHECK
      fprintf (stderr, "%s:%d: %lu, %d: \
I inserted another occurence of %u.\n", __FILE__, __LINE__, order, back, key);
      for (size_t i = 0; i < n_nodes; i++)
	{
	  fprintf (stderr, "%s:%d: %lu, %d: \
Node %lu:", __FILE__, __LINE__, order, back, i);
	  for (size_t j = 0; j < 2 * order + 1; j++)
	    {
	      fprintf (stderr, " %u", storage[i * (2 * order + 1) + j]);
	    }
	  fprintf (stderr, "\n");
	}
#endif /* VERBOSE_INSERTION_CHECK */
    }
  bplus_insertion_free (insertion);
  bplus_range_free (range);
  bplus_fetcher_free (root_fetcher);
  bplus_finder_free (finder);
  free (storage);
  bplus_tree_free (tree);
}

/* Check the pull API. */

struct in_memory_storage
{
  size_t order;
  uint32_t n_nodes;
  uint32_t max_nodes;
  uint32_t *storage;
};

static int
in_memory_fetch (void *context, size_t row, size_t start, size_t length,
		 size_t *n, uint32_t * destination)
{
  struct in_memory_storage *storage = context;
  assert (row < storage->n_nodes);
  assert (start + length <= 2 * storage->order + 1);
  *n = 2 * storage->order + 1 - start;
  memcpy (destination,
	  &(storage->storage[row * (2 * storage->order + 1) + start]),
	  *n * sizeof (uint32_t));
  return 0;
}

static int
default_compare (void *context, const struct bplus_key *a,
		 const struct bplus_key *b, int *result)
{
  (void) context;
  /* Only works for known keys with the identity lookup table. */
  assert (a->type == BPLUS_KEY_KNOWN);
  assert (b->type == BPLUS_KEY_KNOWN);
  *result = (a->arg.known - b->arg.known);
  if (*result > 0)
    {
      *result = 1;
    }
  if (*result < 0)
    {
      *result = -1;
    }
  return 0;
}

static void
in_memory_allocate (void *context, uint32_t * new_id)
{
  struct in_memory_storage *storage = context;
  if (storage->n_nodes >= storage->max_nodes)
    {
      storage->max_nodes *= 2;
      if (storage->max_nodes == 0)
	{
	  storage->max_nodes = 1;
	}
      storage->storage =
	realloc (storage->storage,
		 ((storage->max_nodes * (2 * storage->order + 1)) *
		  sizeof (uint32_t)));
      if (storage->storage == NULL)
	{
	  abort ();
	}
    }
  *new_id = storage->n_nodes;
  storage->n_nodes += 1;
}

static void
in_memory_store (void *context, size_t row, size_t start, size_t length,
		 const uint32_t * data)
{
  struct in_memory_storage *storage = context;
  assert (row < storage->n_nodes);
  assert (start + length <= 2 * storage->order + 1);
  memcpy (&(storage->storage[row * (2 * storage->order + 1) + start]), data,
	  length * sizeof (uint32_t));
}

struct same_value_iterator
{
  uint32_t expected_value;
  size_t n_iterated;
};

static int
iterate_same_value (void *context, size_t n, const struct bplus_key *keys,
		    const uint32_t * values)
{
  struct same_value_iterator *it = context;
  for (size_t i = 0; i < n; i++)
    {
      assert (keys[i].type == BPLUS_KEY_KNOWN);
      ck_assert_int_eq (keys[i].arg.known, it->expected_value);
      ck_assert_int_eq (values[i], it->expected_value);
    }
  it->n_iterated += n;
  return 0;
}

static int
decide_to_insert (void *context, int present, const struct bplus_key *key,
		  uint32_t * known, int *back)
{
  const int *to_back = context;
  (void) present;
  /* Only works for known keys with the identity lookup table. */
  assert (key->type == BPLUS_KEY_KNOWN);
  *known = key->arg.known;
  *back = *to_back;
  return 0;
}

static size_t
in_memory_count_default_compare_same_value (struct bplus_tree *tree,
					    struct in_memory_storage *storage,
					    const struct bplus_key *key,
					    uint32_t expected_value)
{
  struct same_value_iterator it;
  it.expected_value = expected_value;
  it.n_iterated = 0;
  int error =
    bplus_find (tree, in_memory_fetch, storage, default_compare, NULL,
		iterate_same_value, &it, key);
  ck_assert_int_eq (error, 0);
  return it.n_iterated;
}

static void
in_memory_insert_default_compare (struct bplus_tree *tree,
				  struct in_memory_storage *storage,
				  const struct bplus_key *key, int back)
{
  int error =
    bplus_insert (tree, in_memory_fetch, storage, default_compare, NULL,
		  in_memory_allocate, storage, in_memory_store, storage,
		  decide_to_insert, &back, key);
  ck_assert_int_eq (error, 0);
}

static void
do_check_insert_and_grow_pull (size_t order, int back)
{
  static const uint32_t records[] = {
    99, 141, 173, 99, 46, 150, 125, 53, 213, 138, 23, 39, 223, 51, 108, 134,
    189, 201, 73, 34, 223, 60, 151, 64, 250, 72, 105, 149, 212, 44, 160, 29,
    190, 5, 245, 54, 193, 130, 234, 170, 203, 252, 143, 88, 206, 151, 143,
    33, 189, 40, 95, 154, 3, 75, 7, 93, 90, 250, 178, 111, 113, 186, 136,
    35, 232, 106, 130, 14, 190, 238, 1, 230, 18, 206, 107, 250, 44, 133, 86,
    219, 231, 136, 19, 79, 80, 68, 159, 82, 100, 33, 116, 189, 65, 161, 8,
    97, 30, 144, 139, 215, 234, 51, 52, 137, 29, 211, 68, 240, 231, 26, 65,
    133, 127, 35, 204, 10, 81, 38, 166, 202, 108, 160, 229, 24, 220, 64, 42,
    233
#ifdef LOTS_OF_INSERTIONS_TESTS
      , 155, 88, 244, 169, 42, 255, 158, 130, 16, 82, 149, 3, 131, 168, 77,
    133, 100, 130, 112, 23, 232, 180, 43, 50, 104, 199, 2, 23, 65, 202, 118,
    119, 137, 16, 132, 129, 244, 99, 140, 248, 37, 223, 78, 82, 86, 139, 78,
    238, 2, 207, 215, 61, 165, 89, 153, 122, 144, 12, 65, 48, 176, 189, 146,
    187, 2, 123, 218, 206, 36, 196, 219, 37, 220, 66, 73, 232, 89, 187, 182,
    99, 54, 145, 196, 4, 121, 140, 38, 52, 162, 107, 251, 177, 152, 83, 232,
    54, 134, 191, 226, 70, 23, 219, 205, 182, 72, 162, 42, 35, 17, 140, 225,
    22, 209, 13, 106, 255, 183, 25, 10, 91, 196, 50, 234, 85, 239, 70, 28,
    116, 250, 92, 254, 164, 249, 149, 166, 42, 130, 36, 7, 179, 59, 168,
    214, 164, 140, 113, 57, 182, 183, 14, 123, 121, 0, 203, 219, 240, 224,
    197, 86, 246, 254, 62, 20, 36, 178, 189, 122, 81, 226, 228, 103, 35, 54,
    34, 121, 120, 89, 43, 156, 22, 169, 72, 95, 24, 231, 41, 38, 4, 129,
    100, 56, 127, 66, 251, 195, 106, 235, 200, 147, 244, 52, 228, 98, 239,
    72, 189, 250, 21, 107, 20, 204, 133, 8, 197, 85, 236, 169, 52, 150, 111,
    184, 172, 103, 54, 130, 236, 37, 166, 213, 157, 17, 115, 31, 223, 70,
    226, 212, 84, 47, 224, 100, 238, 3, 115, 143, 103, 168, 130, 235, 178,
    19, 165, 248, 126, 33, 51, 167, 210, 189, 38, 209, 196, 215, 23, 166,
    103, 24, 246, 74, 242, 75, 252, 226, 128, 95, 210, 51, 44, 122, 171, 65,
    75, 1, 76, 57, 53, 185, 14, 129, 129, 208, 13, 234, 143, 232, 248, 42,
    165, 105, 62, 38, 39, 181, 247, 164, 101, 98, 54, 220, 21, 39, 41, 61,
    7, 151, 97, 2, 225, 215, 123, 28, 215, 24, 9, 231, 209, 69, 7, 167, 98,
    253, 53, 142, 63, 6, 222, 84, 174, 195, 200, 101, 155, 18, 70, 84, 246,
    162, 26, 242, 171, 228, 179, 144, 81, 233, 247, 237, 209, 82, 193, 107,
    64, 215, 178, 117, 190, 152, 49, 181, 222, 219, 172, 24, 92, 35, 100,
    184, 19, 100, 122, 85, 157, 146, 246, 56, 150, 166, 108, 226, 155, 186,
    173, 72, 166, 255, 185, 109, 9, 138, 95, 121, 141, 138, 218, 150, 81,
    216, 168, 172, 77, 225, 193, 207, 127, 171, 201, 209, 13, 128, 176, 252,
    0, 123, 241, 196, 3, 233, 86, 155, 12, 136, 187, 86, 26, 108, 190, 21,
    88, 88, 84, 7, 3, 187, 100, 210, 57, 104, 218, 109, 11, 230, 42, 56,
    164, 194, 118, 100, 131, 163, 81, 48, 249, 251, 132, 125, 114, 119, 123,
    220, 38, 255, 99, 53, 108, 156, 49, 247, 218, 30, 162, 155, 144, 100,
    40, 163, 250, 110, 28, 207, 43, 81, 29, 100, 87, 49, 225, 189, 7, 230,
    123, 236, 24, 171, 31, 155, 204, 72, 130, 229, 32, 0, 92, 53, 138, 244,
    74, 17, 196, 204, 160, 222, 107, 119, 97, 107, 27, 112, 205, 8, 21, 90,
    177, 36, 54, 29, 69, 13, 61, 198, 27, 160, 176, 85, 248, 197, 77, 198,
    173, 54, 104, 77, 195, 196, 222, 106, 15, 254, 117, 73, 41, 235, 188,
    248, 14, 123, 223, 229, 216, 232, 68, 119, 73, 98, 247, 113, 72, 53, 46,
    114, 81, 197, 68, 102, 20, 0, 138, 61, 34, 160, 122, 2, 251, 108, 84,
    200, 91, 209, 125, 140, 234, 156, 93, 84, 249, 140, 242, 172, 230, 60,
    89, 17, 84, 237, 14, 154, 107, 242, 78, 29, 67, 4, 180, 105, 8, 75, 22,
    13, 54, 28, 188, 195, 26, 61, 223, 254, 209, 69, 206, 136, 67, 35, 34,
    8, 93, 77, 44, 49, 177, 68, 252, 102, 1, 99, 177, 35, 48, 171, 30, 42,
    225, 19, 104, 112, 82, 99, 235, 103, 137, 229, 53, 227, 67, 57, 89, 228,
    135, 129, 19, 250, 142, 213, 36, 108, 7, 134, 204, 246, 64, 197, 71, 16,
    230, 248, 182, 141, 15, 197, 3, 174, 74, 11, 63, 5, 57, 66, 37, 22, 188,
    230, 207, 208, 204, 190, 135, 254, 125, 67, 5, 208, 14, 114, 83, 203,
    224, 179, 63, 221, 42, 225, 162, 12, 127, 176, 208, 59, 121, 156, 231,
    30, 249, 94, 150, 63, 214, 199, 142, 25, 50, 65, 188, 93, 88, 74, 220,
    240, 161, 147, 92, 197, 9, 183, 152, 30, 70, 33, 112, 117, 214, 79, 178,
    248, 22, 169, 141, 166, 128, 194, 229, 159, 210, 161, 38, 211, 178, 91,
    163, 53, 107, 52, 79, 224, 48, 130, 125, 175, 111, 194, 74, 125, 117,
    155, 186, 164, 22, 16, 60, 36, 51, 237, 132, 101, 169, 153, 122, 133,
    161, 107, 70, 156, 17, 81, 99, 227, 129, 44, 51, 228, 75, 169, 251, 215,
    3, 237, 159, 29, 100, 124, 243, 229, 103, 165, 106, 19, 62, 208, 7, 251,
    33, 125, 87, 207, 54, 175, 120, 136, 44, 15, 220, 185, 174, 189, 9, 110,
    9, 205, 211, 4, 91, 236, 234, 93, 65, 253, 135, 42, 225, 207, 33, 159,
    86, 186, 80, 122, 20, 179, 51, 171, 97, 117, 97, 196, 175, 112, 206,
    181, 28
#endif /* LOTS_OF_INSERTIONS_TESTS */
  };
  const size_t n_records = sizeof (records) / sizeof (records[0]);
  size_t n_occurences[256] = { 0 };
  struct bplus_tree *tree = bplus_tree_alloc (order);
  uint32_t *storage_mem = malloc (1 * (2 * order + 1) * sizeof (uint32_t));
  struct in_memory_storage storage = {.order = order,.n_nodes = 1,.max_nodes =
      1,.storage = storage_mem
  };
  if (tree == NULL || storage_mem == NULL)
    {
      abort ();
    }
  /* Set the initial root. */
  bplus_prime (order, storage_mem);
  for (size_t i_record = 0; i_record < n_records; i_record++)
    {
      /* First, check that we see each key the right number of times. */
      for (uint32_t key = 0; key < 256; key++)
	{
	  struct bplus_key k;
	  k.type = BPLUS_KEY_KNOWN;
	  k.arg.known = key;
	  size_t n_total =
	    in_memory_count_default_compare_same_value (tree, &storage, &k,
							key);
	  ck_assert_int_eq (n_total, n_occurences[key]);
	}
      const uint32_t key = records[i_record];
      /* Then, we add 1 occurence of key. */
      n_occurences[key] += 1;
      struct bplus_key k;
      k.type = BPLUS_KEY_KNOWN;
      k.arg.known = key;
      in_memory_insert_default_compare (tree, &storage, &k, back);
    }
  free (storage.storage);
  bplus_tree_free (tree);
}

static void
do_check_hdf5_operations (void)
{
  remove ("check-bplus-hdf5.h5");
  hid_t fcpl = H5Pcreate (H5P_FILE_CREATE);
  ck_assert_int_ne (fcpl, H5I_INVALID_HID);
  hid_t fapl = H5Pcreate (H5P_FILE_ACCESS);
  ck_assert_int_ne (fapl, H5I_INVALID_HID);
  hid_t file = H5Fcreate ("check-bplus-hdf5.h5", H5F_ACC_EXCL, fcpl, fapl);
  ck_assert_int_ne (file, H5I_INVALID_HID);
  hsize_t dims[2] = { 0, 9 };	/* Order: 4 */
  hsize_t maxdims[2] = { H5S_UNLIMITED, 9 };
  hid_t fspace = H5Screate_simple (2, dims, maxdims);
  ck_assert_int_ne (fspace, H5I_INVALID_HID);
  hid_t lcpl = H5Pcreate (H5P_LINK_CREATE);
  ck_assert_int_ne (lcpl, H5I_INVALID_HID);
  hid_t dcpl = H5Pcreate (H5P_DATASET_CREATE);
  ck_assert_int_ne (dcpl, H5I_INVALID_HID);
  hsize_t chunkdims[2] = { 1, 9 };
  if (H5Pset_chunk (dcpl, 2, chunkdims) < 0)
    {
      abort ();
    }
  hid_t dataset =
    H5Dcreate2 (file, "test", H5T_STD_U32LE, fspace, lcpl, dcpl, H5P_DEFAULT);
  ck_assert_int_ne (dataset, H5I_INVALID_HID);
  struct bplus_hdf5_table *table = bplus_hdf5_table_alloc ();
  if (table == NULL)
    {
      abort ();
    }
  int err = bplus_hdf5_table_set (table, dataset);
  ck_assert_int_eq (err, 0);
  size_t order = bplus_hdf5_table_order (table);
  ck_assert_int_eq (order, 4);
  uint32_t parent_of_root = 42;
  size_t actual_length;
  err = bplus_hdf5_fetch (table, 0, 7, 1, &actual_length, &parent_of_root);
  ck_assert_int_eq (err, 0);
  ck_assert_int_eq (actual_length, 9);
  ck_assert_int_eq (parent_of_root, ((uint32_t) (-1)));
  uint32_t new_id;
  bplus_hdf5_allocate (table, &new_id);
  ck_assert_int_eq (new_id, 1);
  const uint32_t example_node[9] = {
    /* Keys: */
    99, ((uint32_t) (-1)), ((uint32_t) (-1)),
    /* Children: */
    2, 5, 0, 0,
    /* Parent: */
    0,
    /* Flags: */
    0
  };
  bplus_hdf5_update (table, 1, 0, 9, example_node);
  uint32_t row[999] = { 0 };
  err = bplus_hdf5_fetch (table, 1, 0, 999, &actual_length, row);
  ck_assert_int_eq (err, 0);
  ck_assert_int_eq (actual_length, 9);
  ck_assert_int_eq (row[0], 99);
  ck_assert_int_eq (row[4], 5);
  err = bplus_hdf5_fetch (table, 1, 99, 999, &actual_length, row);
  ck_assert_int_eq (err, 0);
  ck_assert_int_eq (actual_length, 9);
  bplus_hdf5_table_free (table);
  H5Dclose (dataset);
  H5Pclose (dcpl);
  H5Pclose (lcpl);
  H5Sclose (fspace);
  H5Fclose (file);
  H5Sclose (fapl);
  H5Sclose (fcpl);
  int clean_error = remove ("check-bplus-hdf5.h5");
  ck_assert_int_eq (clean_error, 0);
}

/* *INDENT-OFF* */

START_TEST (check_fetch)
{
  do_check_fetch ();
}
END_TEST

START_TEST (check_fetch_with_cache)
{
  do_check_fetch_with_cache ();
}
END_TEST

START_TEST (check_dichotomy)
{
  do_check_dichotomy ();
}
END_TEST

START_TEST (check_explore_nonsplitted_nonsplits)
{
  do_check_explore_root ();
}
END_TEST

START_TEST (check_explore_splitted)
{
  do_check_explore_splitted ();
}
END_TEST

START_TEST (check_explore_nonsplitted_splits)
{
  do_check_explore_splits ();
}
END_TEST

START_TEST (check_range)
{
  do_check_range ();
}
END_TEST

START_TEST (check_range_2)
{
  do_check_range_2 ();
}
END_TEST

START_TEST (check_range_same_not_end)
{
  do_check_range_same_not_end ();
}
END_TEST

START_TEST (check_finder)
{
  do_check_finder ();
}
END_TEST

START_TEST (check_finder_root_leaf)
{
  do_check_finder_root_leaf ();
}
END_TEST

START_TEST (check_reparentor_noop)
{
  do_check_reparentor_noop ();
}
END_TEST

START_TEST (check_reparentor_leaf)
{
  do_check_reparentor_leaf ();
}
END_TEST

START_TEST (check_reparentor_nonfull)
{
  do_check_reparentor_nonfull ();
}
END_TEST

START_TEST (check_reparentor_manyfixes)
{
  do_check_reparentor_manyfixes ();
}
END_TEST

START_TEST (check_grow_leaf)
{
  do_check_grow_leaf ();
}
END_TEST

START_TEST (check_grow_nonleaf)
{
  do_check_grow_nonleaf ();
}
END_TEST

START_TEST (check_grow_nonfull)
{
  do_check_grow_nonfull ();
}
END_TEST

START_TEST (check_parent_fetcher)
{
  do_check_parent_fetcher ();
}
END_TEST

START_TEST (check_divider_leaf_noop_odd)
{
  do_check_divider_leaf_noop_odd ();
}
END_TEST

START_TEST (check_divider_leaf_noop_even)
{
  do_check_divider_leaf_noop_even ();
}
END_TEST

START_TEST (check_divider_leaf_allocate_odd)
{
  do_check_divider_leaf_allocate_odd ();
}
END_TEST

START_TEST (check_divider_leaf_allocate_even)
{
  do_check_divider_leaf_allocate_even ();
}
END_TEST

START_TEST (check_divider_nonleaf_noop_odd)
{
  do_check_divider_nonleaf_noop_odd ();
}
END_TEST

START_TEST (check_divider_nonleaf_noop_even)
{
  do_check_divider_nonleaf_noop_even ();
}
END_TEST

START_TEST (check_divider_nonleaf_allocate_odd)
{
  do_check_divider_nonleaf_allocate_odd ();
}
END_TEST

START_TEST (check_divider_nonleaf_allocate_even)
{
  do_check_divider_nonleaf_allocate_even ();
}
END_TEST

START_TEST (check_insert_append_leaf)
{
  do_check_insert_append_leaf ();
}
END_TEST

START_TEST (check_insert_enough_space)
{
  do_check_insert_enough_space ();
}
END_TEST

START_TEST (check_insert_depth_1)
{
  do_check_insert_depth_1 ();
}
END_TEST

START_TEST (check_insert_depth_2)
{
  do_check_insert_depth_2 ();
}
END_TEST

START_TEST (check_insert_front_odd_and_grow)
{
  do_check_insert_and_grow (3, 0);
}
END_TEST

START_TEST (check_insert_front_even_and_grow)
{
  do_check_insert_and_grow (4, 0);
}
END_TEST

START_TEST (check_insert_back_odd_and_grow)
{
  do_check_insert_and_grow (3, 1);
}
END_TEST

START_TEST (check_insert_back_even_and_grow)
{
  do_check_insert_and_grow (4, 1);
}
END_TEST

START_TEST (check_insert_front_odd_and_grow_pull)
{
  do_check_insert_and_grow_pull (3, 0);
}
END_TEST

START_TEST (check_insert_front_even_and_grow_pull)
{
  do_check_insert_and_grow_pull (4, 0);
}
END_TEST

START_TEST (check_insert_back_odd_and_grow_pull)
{
  do_check_insert_and_grow_pull (3, 1);
}
END_TEST

START_TEST (check_insert_back_even_and_grow_pull)
{
  do_check_insert_and_grow_pull (4, 1);
}
END_TEST

START_TEST (check_hdf5_operations)
{
  do_check_hdf5_operations ();
}
END_TEST

/* *INDENT-ON* */

Suite *
bplus_suite (void)
{
  Suite *s = suite_create (_("Libbplus test suite"));
  TCase *fetch = tcase_create (_("Fetching a node"));
  tcase_add_test (fetch, check_fetch);
  tcase_add_test (fetch, check_fetch_with_cache);
  suite_add_tcase (s, fetch);
  TCase *dicho = tcase_create (_("Finding a key with dichotomy"));
  tcase_add_test (dicho, check_dichotomy);
  suite_add_tcase (s, dicho);
  TCase *explore = tcase_create (_("Search step for both first \
 and last occurence of a key"));
  tcase_add_test (explore, check_explore_nonsplitted_nonsplits);
  tcase_add_test (explore, check_explore_splitted);
  tcase_add_test (explore, check_explore_nonsplitted_splits);
  suite_add_tcase (s, explore);
  TCase *range = tcase_create (_("Check the range iterator"));
  tcase_add_test (range, check_range);
  tcase_add_test (range, check_range_2);
  tcase_add_test (range, check_range_same_not_end);
  suite_add_tcase (s, range);
  TCase *finder = tcase_create (_("Check the \
recursive key finder"));
  tcase_add_test (finder, check_finder);
  tcase_add_test (finder, check_finder_root_leaf);
  suite_add_tcase (s, finder);
  TCase *reparentor = tcase_create (_("Fix the \
children’s parent"));
  tcase_add_test (reparentor, check_reparentor_noop);
  tcase_add_test (reparentor, check_reparentor_leaf);
  tcase_add_test (reparentor, check_reparentor_nonfull);
  tcase_add_test (reparentor, check_reparentor_manyfixes);
  suite_add_tcase (s, reparentor);
  TCase *grow = tcase_create (_("Grow the B+ tree"));
  tcase_add_test (grow, check_grow_leaf);
  tcase_add_test (grow, check_grow_nonleaf);
  tcase_add_test (grow, check_grow_nonfull);
  suite_add_tcase (s, grow);
  TCase *parent_fetcher = tcase_create (_("Fetch the parent of a node"));
  tcase_add_test (grow, check_parent_fetcher);
  suite_add_tcase (s, parent_fetcher);
  TCase *divider = tcase_create (_("Give some of your keys to a new node"));
  tcase_add_test (divider, check_divider_leaf_noop_odd);
  tcase_add_test (divider, check_divider_leaf_noop_even);
  tcase_add_test (divider, check_divider_leaf_allocate_odd);
  tcase_add_test (divider, check_divider_leaf_allocate_even);
  tcase_add_test (divider, check_divider_nonleaf_noop_odd);
  tcase_add_test (divider, check_divider_nonleaf_noop_even);
  tcase_add_test (divider, check_divider_nonleaf_allocate_odd);
  tcase_add_test (divider, check_divider_nonleaf_allocate_even);
  suite_add_tcase (s, divider);
  TCase *insertion = tcase_create (_("Insert in the B+ tree"));
  tcase_add_test (insertion, check_insert_append_leaf);
  tcase_add_test (insertion, check_insert_enough_space);
  tcase_add_test (insertion, check_insert_depth_1);
  tcase_add_test (insertion, check_insert_depth_2);
  tcase_add_test (insertion, check_insert_front_odd_and_grow);
  tcase_add_test (insertion, check_insert_front_even_and_grow);
  tcase_add_test (insertion, check_insert_back_odd_and_grow);
  tcase_add_test (insertion, check_insert_back_even_and_grow);
  suite_add_tcase (s, insertion);
  TCase *insertion_pull =
    tcase_create (_("Insert in the B+ tree with the pull API"));
  tcase_add_test (insertion_pull, check_insert_front_odd_and_grow_pull);
  tcase_add_test (insertion_pull, check_insert_front_even_and_grow_pull);
  tcase_add_test (insertion_pull, check_insert_back_odd_and_grow_pull);
  tcase_add_test (insertion_pull, check_insert_back_even_and_grow_pull);
  suite_add_tcase (s, insertion_pull);
  TCase *hdf5_callbacks = tcase_create (_("HDF5 callbacks for the API"));
  tcase_add_test (hdf5_callbacks, check_hdf5_operations);
  suite_add_tcase (s, hdf5_callbacks);
  return s;
}

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  Suite *s = bplus_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  int error = srunner_ntests_failed (sr);
  srunner_free (sr);
  if (error != 0)
    {
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
