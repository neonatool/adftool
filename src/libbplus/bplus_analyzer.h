#ifndef H_BPLUS_ANALYZER_INCLUDED
# define H_BPLUS_ANALYZER_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>
# include <assert.h>

# define DEALLOC_ANALYZER \
  ATTRIBUTE_DEALLOC (analyzer_free, 1)

  /* The key analyzer is a special process that takes a set of SORTED
     keys and a searched key. You can ask to get the first or the last
     occurence (or both) of the search key, and it will request you to
     compare keys. */

struct bplus_analyzer;

static int analyzer_init (struct bplus_analyzer *analyzer, size_t order);
static void analyzer_deinit (struct bplus_analyzer *analyzer);

MAYBE_UNUSED static void analyzer_free (struct bplus_analyzer *analyzer);

MAYBE_UNUSED DEALLOC_ANALYZER
  static struct bplus_analyzer *analyzer_alloc (size_t order);

static void analyzer_setup (struct bplus_analyzer *analyzer, size_t n_keys,
			    const uint32_t * keys,
			    const struct bplus_key *search_key);

/* If *done, then nothing more is expected from you. In this case,
 *found contains whether the key has actually been found, *index
 contains the first index that is greater than or equal to the search
 key. Otherwise (if not *done), you are asked to compare keys a and
 b. If which is 1 instead of 0, *index contains the first index that
 is strictly greater than the search key. In any case, *index may be
 equal to the total number of keys if the first match is not found, or
 if the last match is the last key. */

# define BPLUS_ANALYZER_FIRST 0
# define BPLUS_ANALYZER_LAST 1

static void analyzer_match (const struct bplus_analyzer *analyzer, int which,
			    int *done, int *found, size_t *index,
			    struct bplus_key *a, struct bplus_key *b);

static void analyzer_result (struct bplus_analyzer *analyzer,
			     const struct bplus_key *a,
			     const struct bplus_key *b, int result);

struct bplus_analyzer
{
  size_t order;
  struct bplus_key pivot;
  size_t n_keys;
  uint32_t *keys;		/* Our own copy. */
  /* The number of keys that are strictly less than the pivot. */
  size_t n_less;
  /* The number of keys that are strictly greater than the pivot. */
  size_t n_greater;
  /* The first key that has been confirmed equal to the pivot. By
     default, n_keys. */
  size_t first_equal;
  /* The last key that has been confirmed equal to the pivot. By
     default, 0. */
  size_t last_equal;
};

static int
analyzer_init (struct bplus_analyzer *analyzer, size_t order)
{
  analyzer->order = order;
  analyzer->keys = malloc ((order - 1) * sizeof (uint32_t));
  if (analyzer->keys == NULL)
    {
      return 1;
    }
  return 0;
}

static void
analyzer_deinit (struct bplus_analyzer *analyzer)
{
  free (analyzer->keys);
}

static struct bplus_analyzer *
analyzer_alloc (size_t order)
{
  struct bplus_analyzer *analyzer = malloc (sizeof (struct bplus_analyzer));
  if (analyzer != NULL)
    {
      if (analyzer_init (analyzer, order) != 0)
	{
	  free (analyzer);
	  analyzer = NULL;
	}
    }
  return analyzer;
}

static void
analyzer_free (struct bplus_analyzer *analyzer)
{
  if (analyzer != NULL)
    {
      analyzer_deinit (analyzer);
    }
  free (analyzer);
}

static void
analyzer_setup (struct bplus_analyzer *analyzer, size_t n_keys,
		const uint32_t * keys, const struct bplus_key *search_key)
{
  assert (n_keys < analyzer->order);
  memcpy (&(analyzer->pivot), search_key, sizeof (struct bplus_key));
  analyzer->n_keys = n_keys;
  memcpy (analyzer->keys, keys, n_keys * sizeof (uint32_t));
  analyzer->n_less = 0;
  analyzer->n_greater = 0;
  analyzer->first_equal = n_keys;
  analyzer->last_equal = 0;
}

static void
analyzer_match (const struct bplus_analyzer *analyzer, int which, int *done,
		int *found, size_t *index, struct bplus_key *a,
		struct bplus_key *b)
{
  if (analyzer->n_less + analyzer->n_greater == analyzer->n_keys)
    {
      // We covered everything; no match at all, so nothing to cover.
      *done = 1;
      *found = 0;
      *index = analyzer->n_less;
      return;
    }
  if (analyzer->last_equal < analyzer->first_equal)
    {
      // No match found yet, but not everything is explored. Try the
      // middle.
      size_t first_possible = analyzer->n_less;
      size_t n_possible =
	analyzer->n_keys - analyzer->n_less - analyzer->n_greater;
      size_t next = first_possible + n_possible / 2;
      *done = 0;
      // Here, found and index are just hints.
      *found = 0;
      *index = analyzer->n_less;
      if (which == 1)
	{
	  // We want the last, so fix the *index hint.
	  *index = analyzer->n_keys - analyzer->n_greater;
	}
      a->type = BPLUS_KEY_KNOWN;
      a->arg.known = analyzer->keys[next];
      memcpy (b, &(analyzer->pivot), sizeof (struct bplus_key));
      return;
    }
  assert (analyzer->first_equal <= analyzer->last_equal);
  switch (which)
    {
    case 0:
      // Find the first match.
      if (analyzer->first_equal == analyzer->n_less)
	{
	  // Already found.
	  *done = 1;
	  *found = 1;
	  *index = analyzer->n_less;
	}
      else
	{
	  size_t first_possible = analyzer->n_less;
	  size_t n_possible = analyzer->first_equal - analyzer->n_less;
	  size_t next = first_possible + n_possible / 2;
	  *done = 0;
	  *found = 1;
	  *index = analyzer->n_less;
	  a->type = BPLUS_KEY_KNOWN;
	  a->arg.known = analyzer->keys[next];
	  memcpy (b, &(analyzer->pivot), sizeof (struct bplus_key));
	}
      break;
    case 1:
      // Find the last match.
      if (analyzer->last_equal + 1 + analyzer->n_greater == analyzer->n_keys)
	{
	  // Already found.
	  *done = 1;
	  *found = 1;
	  *index = analyzer->last_equal + 1;
	  return;
	}
      else
	{
	  size_t first_possible = analyzer->last_equal + 1;
	  size_t n_possible =
	    analyzer->n_keys - analyzer->n_greater - first_possible;
	  size_t next = first_possible + n_possible / 2;
	  *done = 0;
	  *found = 1;
	  *index = first_possible;
	  a->type = BPLUS_KEY_KNOWN;
	  a->arg.known = analyzer->keys[next];
	  memcpy (b, &(analyzer->pivot), sizeof (struct bplus_key));
	  return;
	}
      break;
    }
}

static void
analyzer_result (struct bplus_analyzer *analyzer, const struct bplus_key *a,
		 const struct bplus_key *b, int result)
{
  if (a->type == BPLUS_KEY_KNOWN
      && bplus_key_identical (b, &(analyzer->pivot)))
    {
      uint32_t key_compared = a->arg.known;
      const size_t lower_bound = analyzer->n_less;
      const size_t upper_bound = analyzer->n_keys - analyzer->n_greater;
      for (size_t i = lower_bound; i < upper_bound; i++)
	{
	  if (analyzer->keys[i] == key_compared)
	    {
	      if (result < 0)
		{
		  /* increase n_less */
		  analyzer->n_less = i + 1;
		  /* Maybe the same key will be found later? So,
		     continue. */
		}
	      else if (result > 0)
		{
		  analyzer->n_greater = analyzer->n_keys - i;
		  /* We won’t be able to do anything more with that
		     comparison result. */
		  return;
		}
	      else if (result == 0 && i < analyzer->first_equal)
		{
		  analyzer->first_equal = i;
		  if (analyzer->last_equal < analyzer->first_equal)
		    {
		      analyzer->last_equal = analyzer->first_equal;
		    }
		  /* If the same key is found again later, we won’t
		     have anymore information to process. */
		  return;
		}
	      else if (result == 0 && i > analyzer->last_equal)
		{
		  analyzer->last_equal = i;
		  /* If the same key is found again later, we will
		     have to update last_match, so continue. */
		}
	    }
	}
    }
  else if (a->type == BPLUS_KEY_KNOWN
	   && bplus_key_identical (a, &(analyzer->pivot)))
    {
      /* Just switch the key order. */
      analyzer_result (analyzer, b, a, -result);
    }
}

#endif /* H_BPLUS_ANALYZER_INCLUDED */
