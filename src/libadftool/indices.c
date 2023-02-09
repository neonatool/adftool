#include <config.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "file.h"
#include "statement.h"
#include "literal_filter_iterator.h"

struct copy_iterator_context
{
  size_t start;
  size_t max;
  size_t *n_results;
  struct adftool_statement **results;
};

static inline int
copy_iterator (void *ctx, size_t n, const struct adftool_statement **results)
{
  struct copy_iterator_context *context = ctx;
  *(context->n_results) += n;
  if (context->start >= n)
    {
      context->start -= n;
      return 0;
    }
  else
    {
      size_t n_written = 0;
      for (size_t i = 0; i < n; i++)
	{
	  if (i >= context->start)
	    {
	      const size_t output_index = i - context->start;
	      if (output_index < context->max)
		{
		  statement_copy (context->results[output_index], results[i]);
		  n_written++;
		}
	    }
	}
      assert (n_written <= context->max);
      context->start = 0;
      context->max -= n_written;
      context->results = &(context->results[n_written]);
    }
  return 0;
}

int
adftool_lookup (struct adftool_file *file,
		const struct adftool_statement *pattern,
		size_t start, size_t max, size_t *n_results,
		struct adftool_statement **results)
{
  *n_results = 0;
  struct copy_iterator_context ctx = {
    .start = start,
    .max = max,
    .n_results = n_results,
    .results = results
  };
  return adftool_file_lookup (file, pattern, copy_iterator, &ctx);
}

struct filter_iterator_context
{
  size_t start;
  size_t max;
  size_t *n_results;
  bool subject;			/* false: extract the object */
  struct adftool_term **results;
};

static inline int
filter_iterator (void *ctx, size_t n,
		 const struct adftool_statement **results)
{
  struct filter_iterator_context *context = ctx;
  for (size_t i = 0; i < n; i++)
    {
      if (results[i]->deletion_date == ((uint64_t) (-1)))
	{
	  /* Push results[i] */
	  const struct adftool_term *to_push = results[i]->subject;
	  if (!context->subject)
	    {
	      to_push = results[i]->object;
	    }
	  if (context->start != 0)
	    {
	      context->start -= 1;
	    }
	  else if (context->max != 0)
	    {
	      term_copy (context->results[0], to_push);
	      context->max -= 1;
	      context->results = &(context->results[1]);
	    }
	  *(context->n_results) += 1;
	}
    }
  return 0;
}

size_t
adftool_lookup_objects (struct adftool_file *file,
			const struct adftool_term *subject,
			const char *predicate, size_t start, size_t max,
			struct adftool_term **objects)
{
  struct adftool_term p = {.type = TERM_NAMED,.str1 =
      (char *) predicate,.str2 = NULL
  };
  struct adftool_statement pattern = {.subject =
      (struct adftool_term *) subject,.predicate = &p,.object = NULL,.graph =
      NULL,.deletion_date = ((uint64_t) (-1))
  };
  size_t n_results = 0;
  struct filter_iterator_context ctx = {
    .start = start,
    .max = max,
    .n_results = &n_results,
    .subject = false,
    .results = objects
  };
  int error = adftool_file_lookup (file, &pattern, filter_iterator, &ctx);
  if (error)
    {
      return 0;
    }
  return n_results;
}

static inline void
adftool_lookup_init_pattern (struct adftool_statement **pattern,
			     const struct adftool_term *subject,
			     const char *predicate)
{
  *pattern = statement_alloc ();
  if (*pattern == NULL)
    {
      return;
    }
  struct adftool_term *p = term_alloc ();
  if (p == NULL)
    {
      statement_free (*pattern);
      *pattern = NULL;
      return;
    }
  term_set_named (p, predicate);
  statement_set (*pattern, (struct adftool_term **) &subject, &p, NULL, NULL,
		 NULL);
  term_free (p);
}

size_t
adftool_lookup_integer (struct adftool_file *file,
			const struct adftool_term *subject,
			const char *predicate,
			size_t start, size_t max, long *objects)
{
  struct adftool_statement *pattern;
  adftool_lookup_init_pattern (&pattern, subject, predicate);
  size_t ret;
  struct adftool_literal_filter filter;
  adftool_literal_filter_init_integer (&filter, start, max, objects);
  int error =
    adftool_file_lookup (file, pattern, adftool_literal_filter_iterate,
			 &filter);
  adftool_literal_filter_deinit_integer (&filter, &ret);
  statement_free (pattern);
  if (error)
    {
      return 0;
    }
  return ret;
}

size_t
adftool_lookup_double (struct adftool_file *file,
		       const struct adftool_term *subject,
		       const char *predicate,
		       size_t start, size_t max, double *objects)
{
  struct adftool_statement *pattern;
  adftool_lookup_init_pattern (&pattern, subject, predicate);
  size_t ret;
  struct adftool_literal_filter filter;
  adftool_literal_filter_init_double (&filter, start, max, objects);
  int error =
    adftool_file_lookup (file, pattern, adftool_literal_filter_iterate,
			 &filter);
  adftool_literal_filter_deinit_double (&filter, &ret);
  statement_free (pattern);
  if (error)
    {
      return 0;
    }
  return ret;
}

size_t
adftool_lookup_date (struct adftool_file *file,
		     const struct adftool_term *subject,
		     const char *predicate,
		     size_t start, size_t max, struct timespec **objects)
{
  struct adftool_statement *pattern;
  adftool_lookup_init_pattern (&pattern, subject, predicate);
  size_t ret;
  struct adftool_literal_filter filter;
  adftool_literal_filter_init_date (&filter, start, max, objects);
  int error =
    adftool_file_lookup (file, pattern, adftool_literal_filter_iterate,
			 &filter);
  adftool_literal_filter_deinit_date (&filter, &ret);
  statement_free (pattern);
  if (error)
    {
      return 0;
    }
  return ret;
}

size_t
adftool_lookup_string (struct adftool_file *file,
		       const struct adftool_term *subject,
		       const char *predicate,
		       size_t *storage_required,
		       size_t storage_size,
		       char *storage,
		       size_t start,
		       size_t max,
		       size_t *langtag_length,
		       char **langtags, size_t *object_length, char **objects)
{
  struct adftool_statement *pattern;
  adftool_lookup_init_pattern (&pattern, subject, predicate);
  size_t ret;
  struct adftool_literal_filter filter;
  adftool_literal_filter_init_string (&filter, storage_size, storage, start,
				      max, langtag_length, langtags,
				      object_length, objects);
  int error =
    adftool_file_lookup (file, pattern, adftool_literal_filter_iterate,
			 &filter);
  adftool_literal_filter_deinit_string (&filter, &ret, storage_required);
  statement_free (pattern);
  if (error)
    {
      return 0;
    }
  return ret;
}

size_t
adftool_lookup_subjects (struct adftool_file *file,
			 const struct adftool_term *object,
			 const char *predicate, size_t start, size_t max,
			 struct adftool_term **subjects)
{
  struct adftool_term p = {.type = TERM_NAMED,.str1 =
      (char *) predicate,.str2 = NULL
  };
  struct adftool_statement pattern = {.subject = NULL,.predicate =
      &p,.object = (struct adftool_term *) object,.graph =
      NULL,.deletion_date = ((uint64_t) (-1))
  };
  size_t n_results = 0;
  struct filter_iterator_context ctx = {
    .start = start,
    .max = max,
    .n_results = &n_results,
    .subject = true,
    .results = subjects
  };
  int error = adftool_file_lookup (file, &pattern, filter_iterator, &ctx);
  if (error)
    {
      return 0;
    }
  return n_results;
}

int
adftool_delete (struct adftool_file *file,
		const struct adftool_statement *pattern,
		uint64_t deletion_date)
{
  return adftool_file_delete (file, pattern, deletion_date);
}

int
adftool_insert (struct adftool_file *file,
		const struct adftool_statement *statement)
{
  return adftool_file_insert (file, statement);
}
