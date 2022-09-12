#include <adftool_private.h>
#include <time.h>

int
adftool_find_channel_identifier (const struct adftool_file *file,
				 size_t channel_index,
				 struct adftool_term *identifier)
{
  struct adftool_term *object = adftool_term_alloc ();
  mpz_t i;
  mpz_init_set_ui (i, channel_index);
  if (object == NULL)
    {
      abort ();
    }
  adftool_term_set_integer (object, i);
  mpz_clear (i);
  static const char *p = "https://localhost/lytonepal#column-number";
  size_t n_results =
    adftool_lookup_subjects (file, object, p, 0, 1, &identifier);
  adftool_term_free (object);
  if (n_results == 0)
    {
      return 1;
    }
  return 0;
}

static inline int
term_to_literal_double (const struct adftool_term *term, double *value)
{
  mpf_t double_value;
  mpf_init (double_value);
  int error = adftool_term_as_double (term, double_value);
  if (error == 0)
    {
      *value = mpf_get_d (double_value);
    }
  mpf_clear (double_value);
  return error;
}

static int
channel_decoder_find (struct adftool_file *file,
		      const struct adftool_term *identifier,
		      const char *scale_or_offset, double *value)
{
  char predicate_str[256];
  sprintf (predicate_str,
	   "https://localhost/lytonepal#has-channel-decoder-%s",
	   scale_or_offset);
  struct adftool_term *object = adftool_term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  size_t n_results =
    adftool_lookup_objects (file, identifier, predicate_str, 0, 1, &object);
  if (n_results > 0)
    {
      int error = term_to_literal_double (object, value);
      if (error)
	{
	  n_results = 0;
	}
    }
  adftool_term_free (object);
  return (n_results == 0);
}

static int
channel_decoder_replace (struct adftool_file *file,
			 const struct adftool_term *identifier,
			 const char *scale_or_offset,
			 const struct adftool_term *replacement)
{
  int error = 0;
  char predicate_str[256];
  sprintf (predicate_str,
	   "https://localhost/lytonepal#has-channel-decoder-%s",
	   scale_or_offset);
  struct adftool_term p = {
    .type = TERM_NAMED,
    .str1 = predicate_str,
    .str2 = NULL
  };
  struct adftool_statement pattern = {
    .subject = (struct adftool_term *) identifier,
    .predicate = &p,
    .object = NULL,
    .graph = NULL,
    .deletion_date = ((uint64_t) (-1))
  };
  if (adftool_delete (file, &pattern, time (NULL) * 1000) != 0)
    {
      error = 1;
      goto cleanup;
    }
  pattern.object = (struct adftool_term *) replacement;
  if (adftool_insert (file, &pattern) != 0)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  return error;
}

static inline void
term_make_literal_double (struct adftool_term *term, double value)
{
  mpf_t double_value;
  mpf_init_set_d (double_value, value);
  adftool_term_set_double (term, double_value);
  mpf_clear (double_value);
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

int
adftool_get_channel_column (const struct adftool_file *file,
			    const struct adftool_term *identifier,
			    size_t *column)
{
  struct adftool_term *object = adftool_term_alloc ();
  if (object == NULL)
    {
      abort ();
    }
  static const char *p = "https://localhost/lytonepal#column-number";
  size_t n_results =
    adftool_lookup_objects (file, identifier, p, 0, 1, &object);
  mpz_t i;
  mpz_init (i);
  if (n_results > 0)
    {
      if (adftool_term_as_integer (object, i) != 0)
	{
	  n_results = 0;
	}
    }
  if (n_results > 0)
    {
      *column = mpz_get_ui (i);
    }
  mpz_clear (i);
  adftool_term_free (object);
  return n_results == 0;
}

int
adftool_add_channel_type (struct adftool_file *file,
			  const struct adftool_term *channel,
			  const struct adftool_term *type)
{
  static const char *const rdf_type =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
  static const struct adftool_term predicate = {
    .type = TERM_NAMED,
    .str1 = (char *) rdf_type,
    .str2 = NULL
  };
  const struct adftool_statement statement = {
    .subject = (struct adftool_term *) channel,
    .predicate = (struct adftool_term *) &predicate,
    .object = (struct adftool_term *) type,
    .graph = NULL,
    .deletion_date = ((uint64_t) (-1))
  };
  int error = adftool_insert (file, &statement);
  return error;
}

size_t
adftool_get_channel_types (const struct adftool_file *file,
			   const struct adftool_term *channel, size_t start,
			   size_t max, struct adftool_term **types)
{
  static const char *const rdf_type =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
  return adftool_lookup_objects (file, channel, rdf_type, start, max, types);
}

size_t
adftool_find_channels_by_type (const struct adftool_file *file,
			       const struct adftool_term *type, size_t start,
			       size_t max, struct adftool_term **channels)
{
  static const char *const rdf_type =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
  return adftool_lookup_subjects (file, type, rdf_type, start, max, channels);
}
