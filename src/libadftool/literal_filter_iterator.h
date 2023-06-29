#ifndef H_ADFTOOL_LITERAL_FILTER_ITERATOR_INCLUDED
# define H_ADFTOOL_LITERAL_FILTER_ITERATOR_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "term.h"
# include "statement.h"

# include <stdlib.h>
# include <assert.h>
# include <string.h>
# include <locale.h>
# include <stdbool.h>
# include <limits.h>

# include "gettext.h"

# ifdef BUILDING_LIBADFTOOL
#  define _(String) dgettext (PACKAGE, (String))
#  define N_(String) (String)
# else
#  define _(String) gettext (String)
#  define N_(String) (String)
# endif

struct adftool_literal_filter;

static inline
  void adftool_literal_filter_init_integer (struct adftool_literal_filter
					    *filter, size_t start, size_t max,
					    long *objects);

static inline
  void adftool_literal_filter_init_double (struct adftool_literal_filter
					   *filter, size_t start, size_t max,
					   double *objects);

static inline
  void adftool_literal_filter_init_date (struct adftool_literal_filter
					 *filter, size_t start, size_t max,
					 struct timespec **objects);

static inline
  void adftool_literal_filter_init_string (struct adftool_literal_filter
					   *filter, size_t storage_size,
					   char *storage, size_t start,
					   size_t max,
					   size_t *langtag_lengths,
					   char **langtags,
					   size_t *value_lengths,
					   char **values);

static inline
  void adftool_literal_filter_deinit_integer (struct adftool_literal_filter
					      *filter, size_t *n_results);

static inline
  void adftool_literal_filter_deinit_double (struct adftool_literal_filter
					     *filter, size_t *n_results);

static inline
  void adftool_literal_filter_deinit_date (struct adftool_literal_filter
					   *filter, size_t *n_results);

static inline
  void adftool_literal_filter_deinit_string (struct adftool_literal_filter
					     *filter, size_t *n_results,
					     size_t *storage_required);

static inline int
adftool_literal_filter_iterate (void *filter, size_t n,
				const struct adftool_statement **statements);

enum adftool_literal_filter_type
{
  LITERAL_FILTER_INTEGER,
  LITERAL_FILTER_DOUBLE,
  LITERAL_FILTER_DATE,
  LITERAL_FILTER_STRING
};

struct adftool_literal_langstring_array
{
  size_t storage_max;
  size_t storage_required;
  char *storage;
  size_t *langtag_lengths;
  char **langtags;
  size_t *value_lengths;
  char **values;
};

union adftool_literal_filter_container
{
  long *integers;
  double *doubles;
  struct timespec **dates;
  struct adftool_literal_langstring_array strings;
};

struct adftool_literal_filter
{
  size_t start;
  size_t max;
  size_t n_results;
  enum adftool_literal_filter_type type;
  union adftool_literal_filter_container container;
};

static inline void
adftool_literal_filter_init (struct adftool_literal_filter *filter,
			     enum adftool_literal_filter_type type,
			     size_t start, size_t max)
{
  filter->start = start;
  filter->max = max;
  filter->n_results = 0;
  filter->type = type;
}

static inline void
adftool_literal_filter_init_integer (struct adftool_literal_filter *filter,
				     size_t start, size_t max, long *objects)
{
  adftool_literal_filter_init (filter, LITERAL_FILTER_INTEGER, start, max);
  filter->container.integers = objects;
}

static inline void
adftool_literal_filter_init_double (struct adftool_literal_filter *filter,
				    size_t start, size_t max, double *objects)
{
  adftool_literal_filter_init (filter, LITERAL_FILTER_DOUBLE, start, max);
  filter->container.doubles = objects;
}

static inline void
adftool_literal_filter_init_date (struct adftool_literal_filter *filter,
				  size_t start, size_t max,
				  struct timespec **objects)
{
  adftool_literal_filter_init (filter, LITERAL_FILTER_DATE, start, max);
  filter->container.dates = objects;
}

static inline void
adftool_literal_filter_init_string (struct adftool_literal_filter *filter,
				    size_t storage_size, char *storage,
				    size_t start, size_t max,
				    size_t *langtag_lengths,
				    char **langtags,
				    size_t *value_lengths, char **values)
{
  adftool_literal_filter_init (filter, LITERAL_FILTER_STRING, start, max);
  filter->container.strings.storage_max = storage_size;
  filter->container.strings.storage_required = 0;
  filter->container.strings.storage = storage;
  filter->container.strings.langtag_lengths = langtag_lengths;
  filter->container.strings.langtags = langtags;
  filter->container.strings.value_lengths = value_lengths;
  filter->container.strings.values = values;
}

static inline void
adftool_literal_filter_deinit (struct adftool_literal_filter *filter,
			       size_t *n_results)
{
  *n_results = filter->n_results;
}

static inline void
adftool_literal_filter_deinit_integer (struct adftool_literal_filter *filter,
				       size_t *n_results)
{
  adftool_literal_filter_deinit (filter, n_results);
}

static inline void
adftool_literal_filter_deinit_double (struct adftool_literal_filter *filter,
				      size_t *n_results)
{
  adftool_literal_filter_deinit (filter, n_results);
}

static inline void
adftool_literal_filter_deinit_date (struct adftool_literal_filter *filter,
				    size_t *n_results)
{
  adftool_literal_filter_deinit (filter, n_results);
}

static inline void
adftool_literal_filter_deinit_string (struct adftool_literal_filter *filter,
				      size_t *n_results,
				      size_t *storage_required)
{
  *storage_required = filter->container.strings.storage_required;
  adftool_literal_filter_deinit (filter, n_results);
}

static inline void
adftool_literal_filter_push (struct adftool_literal_filter *filter,
			     bool *should_save, size_t *save_index)
{
  *should_save = false;
  *save_index = 0;
  if (filter->n_results >= filter->start)
    {
      *save_index = filter->n_results - filter->start;
      if (*save_index < filter->max)
	{
	  *should_save = true;
	}
    }
  else
    {
      *save_index = 0;
    }
  filter->n_results += 1;
}

static inline void
adftool_literal_filter_push_integer (struct adftool_literal_filter *filter,
				     long value)
{
  bool should_save;
  size_t save_index;
  adftool_literal_filter_push (filter, &should_save, &save_index);
  if (should_save)
    {
      filter->container.integers[save_index] = value;
    }
}

static inline void
adftool_literal_filter_push_double (struct adftool_literal_filter *filter,
				    double value)
{
  bool should_save;
  size_t save_index;
  adftool_literal_filter_push (filter, &should_save, &save_index);
  if (should_save)
    {
      filter->container.doubles[save_index] = value;
    }
}

static inline void
adftool_literal_filter_push_date (struct adftool_literal_filter *filter,
				  const struct timespec *value)
{
  bool should_save;
  size_t save_index;
  adftool_literal_filter_push (filter, &should_save, &save_index);
  if (should_save)
    {
      memcpy (filter->container.dates[save_index], value,
	      sizeof (struct timespec));
    }
}

static inline char *
adftool_literal_filter_allocate_string (size_t size, size_t max,
					size_t *total_required, char *memory)
{
  const size_t already_reserved = *total_required;
  *total_required += size;
  if (already_reserved + size <= max)
    {
      return &(memory[already_reserved]);
    }
  return NULL;
}

static inline void
adftool_literal_filter_push_string (struct adftool_literal_filter *filter,
				    size_t langtag_length,
				    const char *langtag, size_t value_length,
				    const char *value)
{
  bool should_save;
  size_t save_index;
  adftool_literal_filter_push (filter, &should_save, &save_index);
  char *my_langtag = NULL;
  const size_t storage_max = filter->container.strings.storage_max;
  size_t *storage_required = &(filter->container.strings.storage_required);
  char *storage = filter->container.strings.storage;
  if (langtag)
    {
      my_langtag =
	adftool_literal_filter_allocate_string (langtag_length + 1,
						storage_max, storage_required,
						storage);
    }
  if (my_langtag)
    {
      memcpy (my_langtag, langtag, langtag_length);
      my_langtag[langtag_length] = '\0';
    }
  char *my_value =
    adftool_literal_filter_allocate_string (value_length + 1, storage_max,
					    storage_required, storage);
  if (my_value)
    {
      memcpy (my_value, value, value_length);
      my_value[value_length] = '\0';
    }
  if (should_save)
    {
      filter->container.strings.langtag_lengths[save_index] = langtag_length;
      filter->container.strings.langtags[save_index] = my_langtag;
      filter->container.strings.value_lengths[save_index] = value_length;
      filter->container.strings.values[save_index] = my_value;
    }
}

static inline void
adftool_literal_filter_try_push_integer (struct adftool_literal_filter
					 *filter,
					 const struct adftool_term *term)
{
  long value;
  if (term_as_integer (term, &value) == 0)
    {
      adftool_literal_filter_push_integer (filter, value);
    }
}

static inline void
adftool_literal_filter_try_push_double (struct adftool_literal_filter *filter,
					const struct adftool_term *term)
{
  double value;
  if (term_as_double (term, &value) == 0)
    {
      adftool_literal_filter_push_double (filter, value);
    }
}

static inline void
adftool_literal_filter_try_push_date (struct adftool_literal_filter *filter,
				      const struct adftool_term *term)
{
  struct timespec value;
  if (term_as_date (term, &value) == 0)
    {
      adftool_literal_filter_push_date (filter, &value);
    }
}

static inline void
adftool_literal_filter_try_push_string (struct adftool_literal_filter *filter,
					const struct adftool_term *term)
{
  /* FIXME: add a term API to get a copy-less access to the str1 and
     str2 fields */
  const char *value = term->str1;
  const char *meta = term->str2;
  if (term->type == TERM_TYPED
      && STREQ (meta, "http://www.w3.org/2001/XMLSchema#string"))
    {
      assert (value != NULL);
      adftool_literal_filter_push_string (filter, 0, NULL, strlen (value),
					  value);
    }
  else if (term->type == TERM_LANGSTRING)
    {
      assert (meta != NULL);
      assert (value != NULL);
      adftool_literal_filter_push_string (filter, strlen (meta), meta,
					  strlen (value), value);
    }
}

static inline void
adftool_literal_filter_try_push (struct adftool_literal_filter *filter,
				 const struct adftool_term *term)
{
  switch (filter->type)
    {
    case LITERAL_FILTER_INTEGER:
      adftool_literal_filter_try_push_integer (filter, term);
      break;
    case LITERAL_FILTER_DOUBLE:
      adftool_literal_filter_try_push_double (filter, term);
      break;
    case LITERAL_FILTER_DATE:
      adftool_literal_filter_try_push_date (filter, term);
      break;
    case LITERAL_FILTER_STRING:
      adftool_literal_filter_try_push_string (filter, term);
      break;
    default:
      abort ();
    }
}

static inline int
adftool_literal_filter_iterate (void *ctx, size_t n,
				const struct adftool_statement **statements)
{
  struct adftool_literal_filter *filter = ctx;
  for (size_t i = 0; i < n; i++)
    {
      uint64_t deletion_date;
      struct adftool_term *object;
      statement_get (statements[i], NULL, NULL, &object, NULL,
		     &deletion_date);
      if (deletion_date == ((uint64_t) (-1)))
	{
	  adftool_literal_filter_try_push (filter, object);
	}
    }
  return 0;
}

#endif /* not H_ADFTOOL_LITERAL_FILTER_ITERATOR_INCLUDED */
