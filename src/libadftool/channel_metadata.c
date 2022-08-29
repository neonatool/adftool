#include <adftool_private.h>
#include <time.h>

#ifdef HAVE_MPFR_H
#include <mpfr.h>
#endif /* HAVE_MPFR_H */

static struct adftool_statement *
column_number_finder (size_t i)
{
  /* Construct: ? lyto:column-number "…"^^xsd:integer */
  struct adftool_statement *pattern = adftool_statement_alloc ();
  struct adftool_term *subject = NULL;
  struct adftool_term *predicate = adftool_term_alloc ();
  struct adftool_term *object = adftool_term_alloc ();
  struct adftool_term *graph = NULL;
  uint64_t deletion_date = (uint64_t) (-1);
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
      return NULL;
    }
  char column_number[256];
  sprintf (column_number, "%lu", i);
  adftool_term_set_named
    (predicate, "https://localhost/lytonepal#column-number");
  adftool_term_set_literal
    (object, column_number, "http://www.w3.org/2001/XMLSchema#integer", NULL);
  adftool_statement_set (pattern, &subject, &predicate, &object, &graph,
			 &deletion_date);
  adftool_term_free (predicate);
  adftool_term_free (object);
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
  struct adftool_results *results = adftool_results_alloc ();
  if (pattern == NULL || results == NULL)
    {
      abort ();
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
      struct adftool_term *subject;
      const struct adftool_statement *candidate =
	adftool_results_get (results, i);
      uint64_t deletion_date;
      adftool_statement_get (candidate, &subject, NULL, NULL, NULL,
			     &deletion_date);
      const int has_subject = (subject != NULL);
      const int has_deletion_date = (deletion_date != ((uint64_t) (-1)));
      assert (has_subject);
      if (!has_deletion_date)
	{
	  n_live_results++;
	  adftool_term_copy (identifier, subject);
	}
    }
  if (n_live_results != 1)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  adftool_statement_free (pattern);
  adftool_results_free (results);
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
      abort ();
    }
  if (adftool_delete (file, pattern, time (NULL) * 1000) != 0)
    {
      error = 1;
      goto cleanup;
    }
  adftool_statement_set (pattern, (struct adftool_term **) &identifier, NULL,
			 NULL, NULL, NULL);
  if (adftool_insert (file, pattern) != 0)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  adftool_statement_free (pattern);
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

static inline int
term_to_literal_double (const struct adftool_term *term, double *value)
{
  int error = 0;
  if (!adftool_term_is_typed_literal (term))
    {
      error = 1;
      goto wrapup;
    }
  size_t value_length = adftool_term_value (term, 0, 0, NULL);
  char *value_str = malloc (value_length + 1);
  if (value_str == NULL)
    {
      error = 1;
      goto wrapup;
    }
  if (adftool_term_value (term, 0, value_length + 1, value_str) !=
      value_length)
    {
      abort ();
    }
  char meta[256];
  size_t meta_length = adftool_term_meta (term, 0, sizeof (meta), meta);
  if (meta_length > 255)
    {
      /* Not xsd:double. */
      error = 1;
      goto cleanup_value;
    }
  meta[meta_length] = '\0';
  if (strcmp (meta, "http://www.w3.org/2001/XMLSchema#double") != 0)
    {
      error = 1;
      goto cleanup_value;
    }
  char *end;
  *value = locale_independent_strtod (value_str, &end);
  if (end == value_str)
    {
      error = 1;
      goto cleanup_value;
    }
cleanup_value:
  free (value_str);
wrapup:
  return error;
}

static struct adftool_statement *
channel_decoder_finder (const struct adftool_term *identifier,
			const char *scale_or_offset)
{
  /* Construct: identifier lyto:has-channel-decoder-scale/offset ? */
  struct adftool_statement *pattern = adftool_statement_alloc ();
  struct adftool_term *predicate = adftool_term_alloc ();
  if (pattern == NULL || predicate == NULL)
    {
      if (pattern)
	{
	  adftool_statement_free (pattern);
	}
      if (predicate)
	{
	  adftool_term_free (predicate);
	}
      return NULL;
    }
  char predicate_str[256];
  sprintf (predicate_str,
	   "https://localhost/lytonepal#has-channel-decoder-%s",
	   scale_or_offset);
  adftool_term_set_named (predicate, predicate_str);
  adftool_statement_set (pattern, (struct adftool_term **) &identifier,
			 &predicate, NULL, NULL, NULL);
  adftool_term_free (predicate);
  return pattern;
}

static int
channel_decoder_find (struct adftool_file *file,
		      const struct adftool_term *identifier,
		      const char *scale_or_offset, double *value)
{
  int error = 0;
  struct adftool_statement *pattern =
    channel_decoder_finder (identifier, scale_or_offset);
  struct adftool_results *results = adftool_results_alloc ();
  struct adftool_term *literal_value = adftool_term_alloc ();
  if (pattern == NULL || results == NULL || literal_value == NULL)
    {
      abort ();
    }
  if (adftool_lookup (file, pattern, results) != 0)
    {
      error = 1;
      goto cleanup;
    }
  size_t n_results = adftool_results_count (results);
  size_t i;
  for (i = 0; i < n_results; i++)
    {
      const struct adftool_statement *candidate =
	adftool_results_get (results, i);
      struct adftool_term *literal_value;
      uint64_t deletion_date;
      adftool_statement_get (candidate, NULL, NULL, &literal_value, NULL,
			     &deletion_date);
      const int has_object = (literal_value != NULL);
      const int has_deletion_date = (deletion_date != ((uint64_t) (-1)));
      assert (has_object);
      if (!has_deletion_date
	  && term_to_literal_double (literal_value, value) == 0)
	{
	  error = 0;
	  goto cleanup;
	}
    }
  /* No value. */
  error = 1;
cleanup:
  adftool_term_free (literal_value);
  adftool_results_free (results);
  adftool_statement_free (pattern);
  return error;
}

static int
channel_decoder_replace (struct adftool_file *file,
			 const struct adftool_term *identifier,
			 const char *scale_or_offset,
			 const struct adftool_term *replacement)
{
  int error = 0;
  struct adftool_statement *pattern =
    channel_decoder_finder (identifier, scale_or_offset);
  if (pattern == NULL)
    {
      abort ();
    }
  if (adftool_delete (file, pattern, time (NULL) * 1000) != 0)
    {
      error = 1;
      goto cleanup;
    }
  adftool_statement_set (pattern, NULL, NULL,
			 (struct adftool_term **) &replacement, NULL, NULL);
  if (adftool_insert (file, pattern) != 0)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  adftool_statement_free (pattern);
  return error;
}

static inline void
term_make_literal_double (struct adftool_term *term, double value)
{
  static const char *meta = "http://www.w3.org/2001/XMLSchema#double";
  char value_str[256];
#ifdef HAVE_MPFR_STRTOFR
  mpfr_t number;
  mpfr_init2 (number, 53);
  mpfr_set_d (number, value, MPFR_RNDN);
  mpfr_exp_t exp;
  char *the_value = mpfr_get_str (NULL, &exp, 10, 0, number, MPFR_RNDN);
  assert (strlen (the_value) < sizeof (value_str));
  assert (strlen (the_value) > 0);
  if (the_value[0] == '-' || the_value[0] == '+')
    {
      assert (strlen (the_value) > 1);
      sprintf (value_str, "%c%c.%sE%ld", the_value[0], the_value[1],
	       the_value + 2, exp - 1);
    }
  else
    {
      sprintf (value_str, "%c.%sE%ld", the_value[0], the_value + 1, exp - 1);
    }
  mpfr_clear (number);
#else
  char *old_locale = setlocale (LC_NUMERIC, "C");
  sprintf (value_str, "%.20e", value);
  setlocale (LC_NUMERIC, old_locale);
#endif
  adftool_term_set_literal (term, value_str, meta, NULL);
}

int
adftool_get_channel_decoder (const struct adftool_file *file,
			     const struct adftool_term *identifier,
			     double *scale, double *offset)
{
  int scale_error =
    channel_decoder_find ((struct adftool_file *) file, identifier, "scale",
			  scale);
  int offset_error =
    channel_decoder_find ((struct adftool_file *) file, identifier, "offset",
			  offset);
  if (scale_error || offset_error)
    {
      return 1;
    }
  return 0;
}

int
adftool_set_channel_decoder (struct adftool_file *file,
			     const struct adftool_term *identifier,
			     double scale, double offset)
{
  int error = 0;
  struct adftool_term *literal_scale = adftool_term_alloc ();
  struct adftool_term *literal_offset = adftool_term_alloc ();
  if (literal_scale == NULL || literal_offset == NULL)
    {
      if (literal_scale)
	{
	  adftool_term_free (literal_scale);
	}
      if (literal_offset)
	{
	  adftool_term_free (literal_offset);
	}
      return 1;
    }
  term_make_literal_double (literal_scale, scale);
  term_make_literal_double (literal_offset, offset);
  int scale_error =
    channel_decoder_replace (file, identifier, "scale", literal_scale);
  int offset_error =
    channel_decoder_replace (file, identifier, "offset", literal_offset);
  if (scale_error || offset_error)
    {
      error = 1;
      goto wrapup;
    }
wrapup:
  adftool_term_free (literal_scale);
  adftool_term_free (literal_offset);
  return error;
}
