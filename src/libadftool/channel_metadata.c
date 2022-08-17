#include <adftool_private.h>
#include <time.h>

#ifdef HAVE_MPFR_H
#include <mpfr.h>
#endif /* HAVE_MPFR_H */

static struct adftool_statement *
column_number_finder (size_t i)
{
  int error = 0;
  /* Construct: ? lyto:column-number "…"^^xsd:integer */
  struct adftool_statement *pattern = adftool_statement_alloc ();
  struct adftool_term *predicate = adftool_term_alloc ();
  struct adftool_term *object = adftool_term_alloc ();
  if (pattern == NULL || predicate == NULL || object == NULL)
    {
      if (pattern)
	{
	  adftool_statement_free (pattern);
	}
      if (predicate)
	{
	  adftool_term_free (predicate);
	}
      if (object)
	{
	  adftool_term_free (object);
	}
      error = 1;
      goto wrapup;
    }
  char column_number[256];
  sprintf (column_number, "%lu", i);
  if ((adftool_term_set_named
       (predicate, "https://localhost/lytonepal#column-number") != 0)
      ||
      (adftool_term_set_literal
       (object, column_number, "http://www.w3.org/2001/XMLSchema#integer",
	NULL) != 0))
    {
      error = 1;
      goto cleanup;
    }
  if ((adftool_statement_set_predicate (pattern, predicate) != 0)
      || (adftool_statement_set_object (pattern, object) != 0))
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  if (error)
    {
      adftool_statement_free (pattern);
    }
  adftool_term_free (predicate);
  adftool_term_free (object);
wrapup:
  return pattern;
}

int
adftool_find_channel_identifier (const struct adftool_file *file,
				 size_t channel_index,
				 struct adftool_term *identifier)
{
  int error = 0;
  /* Construct: ? lyto:column-number "…"^^xsd:integer */
  struct adftool_statement *pattern = column_number_finder (channel_index);
  struct adftool_term *subject = adftool_term_alloc ();
  struct adftool_results *results = adftool_results_alloc ();
  if (pattern == NULL || subject == NULL || results == NULL)
    {
      if (pattern)
	{
	  adftool_statement_free (pattern);
	}
      if (subject)
	{
	  adftool_term_free (subject);
	}
      if (results)
	{
	  adftool_results_free (results);
	}
      error = 1;
      goto wrapup;
    }
  if (adftool_lookup (file, pattern, results) != 0)
    {
      error = 1;
      goto cleanup;
    }
  size_t n_results = adftool_results_count (results);
  size_t n_live_results = 0;
  for (size_t i = 0; i < n_results; i++)
    {
      const struct adftool_statement *candidate =
	adftool_results_get (results, i);
      int has_subject;
      int has_deletion_date;
      uint64_t deletion_date;
      if ((adftool_statement_get_subject (candidate, &has_subject, subject) !=
	   0)
	  ||
	  (adftool_statement_get_deletion_date
	   (candidate, &has_deletion_date, &deletion_date) != 0))
	{
	  error = 1;
	  goto cleanup;
	}
      assert (has_subject);
      if (!has_deletion_date)
	{
	  n_live_results++;
	  if (adftool_term_copy (identifier, subject) != 0)
	    {
	      error = 1;
	      goto cleanup;
	    }
	}
    }
  if (n_live_results != 1)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  adftool_statement_free (pattern);
  adftool_term_free (subject);
  adftool_results_free (results);
wrapup:
  return error;
}

int
adftool_set_channel_identifier (struct adftool_file *file,
				size_t channel_index,
				const struct adftool_term *identifier)
{
  int error = 0;
  struct adftool_statement *pattern = column_number_finder (channel_index);
  if (pattern == NULL)
    {
      error = 1;
      goto wrapup;
    }
  if (adftool_delete (file, pattern, time (NULL) * 1000) != 0)
    {
      error = 1;
      goto cleanup;
    }
  if (adftool_statement_set_subject (pattern, identifier) != 0)
    {
      error = 1;
      goto cleanup;
    }
  if (adftool_insert (file, pattern) != 0)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  adftool_statement_free (pattern);
wrapup:
  return error;
}

static inline double
locale_independent_strtod (const char *text, char **end)
{
#ifdef HAVE_MPFR_STRTOFR
  mpfr_t number;
  mpfr_init2 (number, 53);
  mpfr_strtofr (number, text, end, 10, MPFR_RNDN);
  const double ret = mpfr_get_d (number, MPFR_RNDN);
  mpfr_clear (number);
  return ret;
#else
  char *old_locale = setlocale (LC_NUMERIC, "C");
  const double ret = strtod (text, end);
  setlocale (LC_NUMERIC, old_locale);
  return ret;
#endif
}
